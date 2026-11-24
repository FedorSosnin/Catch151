#pragma once
// Waveshare ESP32-S3-Touch-LCD-2.1 (480x480 round IPS): the classes knomi_hal.h gives the game on the Knomi, for this
// board. To the game it is a Knomi (BOARD_KNOMI_V2 is also set): the canvas stays 466x466 and touches come in canvas
// coordinates. docs/WAVESHARE_21.md has the board, the pins and the bring-up plan.
//
// First run on a board on 2026-11-20: it boots, the panel lights and all its I2C parts answer. What is still to be
// checked on the board is marked TODO(bring-up).
#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "autorot.h"
#include <esp_heap_caps.h>
#include <esp_lcd_panel_rgb.h>
#include <esp_lcd_panel_ops.h>
#include <esp_cache.h>
#include "esp32s3/rom/cache.h"  // (Cache_WriteBack_All: ws21Writeback())
#include <esp_timer.h>
#include <esp_async_memcpy.h>
#include "esp_private/gdma.h"       // (the refill's own DMA: Ws21Prefetch)
#include "esp_private/gdma_link.h"
#include <Preferences.h>
#include "soc/lcd_cam_struct.h"
#include "soc/gpio_periph.h"  // (the console's PDMCHECK reads the sound pin back)
#include "soc/io_mux_reg.h"
#include "soc/gpio_reg.h"
#include <SensorQMI8658.hpp>
#include "ws21_exio.h"  // the IO expander: the panel's reset and CS, the touch's reset
#define KNOMI_RIM_ROWS LCD_HEIGHT  // the rim is made for the 466 canvas, 1:1 (below)
#include "knomi_rim.h"

// The canvas sits in the middle of the panel, pixel for pixel: 7 px of black all round. The 2 px grid the V2 UI
// draws on stays sharp, and nothing is scaled (docs/WAVESHARE_21.md, "The screen").
#define WS21_BORDER ((PANEL_SIZE - LCD_WIDTH) / 2)

// The picture's turn on the panel, clockwise, at any angle (WS21_ROT_DEG in pin_config.h; the console's ROTDEG): the
// board's USB-C ports point out at 45 degrees, so turning the picture 45 puts one of them straight down (Fedor: "Can
// we rotate the screen 45* clockwise?"). 0 is a plain copy; any other angle samples the canvas per panel pixel (the
// nearest one). Touches and tilt turn with it.
inline int16_t gWs21RotDeg = 0;
// Turned, each panel pixel blends the four canvas pixels round its point (smooth), or takes the nearest one (sharp
// but pixel art's lines come out dotted and its dithering in zigzags at 45 degrees: Fedor saw them as streaks).
// The console's SMOOTH switches it; SMOOTH 2 blends only across the 2 px grid the game draws on (the same picture for
// what's drawn on it, with less work: between the two pixels of one grid cell there's nothing to blend)
inline uint8_t gWs21Smooth = WS21_SMOOTH;
inline int32_t gWs21RotC = 65536, gWs21RotS = 0;  // its cosine and sine, 16.16
inline void ws21SetRotation(int deg) {
  deg %= 360;
  if (deg < 0) deg += 360;
  gWs21RotDeg = deg;
  float a = deg * 0.017453293f;
  gWs21RotC = lroundf(cosf(a) * 65536);
  gWs21RotS = lroundf(sinf(a) * 65536);
}
// the canvas point shown at a canvas-sized point (x, y) of the panel's middle (both 0..465)
inline void ws21Unturn(int32_t x, int32_t y, int32_t *u, int32_t *v) {
  int64_t X2 = 2 * x + 1 - LCD_WIDTH, Y2 = 2 * y + 1 - LCD_HEIGHT;  // twice the distance from the centre
  *u = (int32_t)(((X2 * gWs21RotC + Y2 * gWs21RotS) >> 1) + ((int64_t)(LCD_WIDTH / 2) << 16)) >> 16;
  *v = (int32_t)(((Y2 * gWs21RotC - X2 * gWs21RotS) >> 1) + ((int64_t)(LCD_HEIGHT / 2) << 16)) >> 16;
}

// The panel's setup commands go over a 9-bit 3-wire SPI on GPIO 1 and 2, its CS on the expander. begin() also
// resets the panel (EXIO1), as Waveshare's demo does before its setup.
class Ws21InitBus : public Arduino_SWSPI {
public:
  Ws21InitBus() : Arduino_SWSPI(GFX_NOT_DEFINED /*9-bit*/, GFX_NOT_DEFINED /*CS: expander*/, LCD_SPI_SCK, LCD_SPI_MOSI) {}
  bool begin(int32_t speed = GFX_NOT_DEFINED, int8_t mode = GFX_NOT_DEFINED) override {
    if (!gExio.begin()) Serial.println("TCA9554 not detected");
    gExio.write(EXIO_LCD_RST, false);
    delay(10);
    gExio.write(EXIO_LCD_RST, true);
    delay(50);
    return Arduino_SWSPI::begin(speed, mode);
  }
  void beginWrite() override {
    gExio.write(EXIO_LCD_CS, false);
    Arduino_SWSPI::beginWrite();
  }
  void endWrite() override {
    Arduino_SWSPI::endWrite();
    gExio.write(EXIO_LCD_CS, true);
  }
};

// The panel's setup, as Waveshare's demo sends it (Arduino/examples/LVGL_Arduino/Display_ST7701.cpp, ST7701_Init()).
// Arduino_GFX's TL021WVC02 sequence is close but not the same panel tuning: other gamma, VCOM and inversion settings.
// TODO(bring-up): if colours look wrong, this is the first place to look.
static const uint8_t WS21_INIT_OPS[] = {
    BEGIN_WRITE,
    WRITE_C8_BYTES, 0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x10,
    WRITE_C8_D16, 0xC0, 0x3B, 0x00,
    WRITE_C8_D16, 0xC1, 0x0B, 0x02,
    WRITE_C8_D16, 0xC2, 0x07, 0x02,
    WRITE_C8_D8, 0xCC, 0x10,
    WRITE_C8_D8, 0xCD, 0x08,
    WRITE_C8_BYTES, 0xB0, 16, 0x00, 0x11, 0x16, 0x0E, 0x11, 0x06, 0x05, 0x09, 0x08, 0x21, 0x06, 0x13, 0x10, 0x29, 0x31, 0x18,
    WRITE_C8_BYTES, 0xB1, 16, 0x00, 0x11, 0x16, 0x0E, 0x11, 0x07, 0x05, 0x09, 0x09, 0x21, 0x05, 0x13, 0x11, 0x2A, 0x31, 0x18,
    WRITE_C8_BYTES, 0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x11,
    WRITE_C8_D8, 0xB0, 0x6D,
    WRITE_C8_D8, 0xB1, 0x37,
    WRITE_C8_D8, 0xB2, 0x81,
    WRITE_C8_D8, 0xB3, 0x80,
    WRITE_C8_D8, 0xB5, 0x43,
    WRITE_C8_D8, 0xB7, 0x85,
    WRITE_C8_D8, 0xB8, 0x20,
    WRITE_C8_D8, 0xC1, 0x78,
    WRITE_C8_D8, 0xC2, 0x78,
    WRITE_C8_D8, 0xD0, 0x88,
    WRITE_C8_BYTES, 0xE0, 3, 0x00, 0x00, 0x02,
    WRITE_C8_BYTES, 0xE1, 11, 0x03, 0xA0, 0x00, 0x00, 0x04, 0xA0, 0x00, 0x00, 0x00, 0x20, 0x20,
    WRITE_C8_BYTES, 0xE2, 13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    WRITE_C8_BYTES, 0xE3, 4, 0x00, 0x00, 0x11, 0x00,
    WRITE_C8_D16, 0xE4, 0x22, 0x00,
    WRITE_C8_BYTES, 0xE5, 16, 0x05, 0xEC, 0xA0, 0xA0, 0x07, 0xEE, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    WRITE_C8_BYTES, 0xE6, 4, 0x00, 0x00, 0x11, 0x00,
    WRITE_C8_D16, 0xE7, 0x22, 0x00,
    WRITE_C8_BYTES, 0xE8, 16, 0x06, 0xED, 0xA0, 0xA0, 0x08, 0xEF, 0xA0, 0xA0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    WRITE_C8_BYTES, 0xEB, 7, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00,
    WRITE_C8_BYTES, 0xED, 16, 0xFF, 0xFF, 0xFF, 0xBA, 0x0A, 0xBF, 0x45, 0xFF, 0xFF, 0x54, 0xFB, 0xA0, 0xAB, 0xFF, 0xFF, 0xFF,
    WRITE_C8_BYTES, 0xEF, 6, 0x10, 0x0D, 0x04, 0x08, 0x3F, 0x1F,
    WRITE_C8_BYTES, 0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x13,
    WRITE_C8_D8, 0xEF, 0x08,
    WRITE_C8_BYTES, 0xFF, 5, 0x77, 0x01, 0x00, 0x00, 0x00,
    WRITE_C8_D8, 0x36, 0x00,
    WRITE_C8_D8, 0x3A, 0x66,
    WRITE_COMMAND_8, 0x11,
    END_WRITE,
    DELAY, 255,
    DELAY, 225,
    BEGIN_WRITE,
    WRITE_COMMAND_8, 0x20,
    END_WRITE,
    DELAY, 120,
    BEGIN_WRITE,
    WRITE_COMMAND_8, 0x29,
    END_WRITE};

// A frame: the 466x466 picture, as the canvas (no border, no rim). Each is allocated a little long, to a whole number
// of 64-byte lines, so the DMA and the cache can take it in whole lines.
#define WS21_FRAME_BYTES ((size_t)LCD_WIDTH * LCD_HEIGHT * 2)
#define WS21_FRAME_ALLOC ((WS21_FRAME_BYTES + 63) & ~(size_t)63)
// Each frame-sized picture (the frames, the canvas, Pinball's table, Pic Snap's still) has a guard on each side:
// WS21_GUARD bytes of WS21_GUARD_FILL that nothing should write. Something draws a few rows past the screen's top
// (or bottom) now and then, and the game draws straight into the frames: the first frame lies just after the I2C
// driver's memory, which was overwritten with dark pixels, and the touch task crashed on it (Fedor, 2026-11-23: "quit
// to a black screen" in a battle, and once in Pinball; LoadProhibited at 0x08841108 in i2c_master, the I2C bus at
// 0x3c202edc, the first frame at 0x3c203400). A stray row now lands in a guard: ws21GuardCheck() says where, and in
// what screen, and fills it again.
#define WS21_GUARD 16384  // (17 rows of the screen each side)
#define WS21_GUARD_FILL 0x5A
#define WS21_GUARDS_MAX 8
inline uint8_t *gWs21Guarded[WS21_GUARDS_MAX];  // each picture's allocation (its guard before it at the start)
inline uint16_t *ws21FrameAlloc() {
  uint8_t *b = (uint8_t *)heap_caps_aligned_calloc(64, 1, WS21_FRAME_ALLOC + 2 * WS21_GUARD, MALLOC_CAP_SPIRAM);
  if (!b) return nullptr;
  memset(b, WS21_GUARD_FILL, WS21_GUARD);
  memset(b + WS21_GUARD + WS21_FRAME_ALLOC, WS21_GUARD_FILL, WS21_GUARD);
  for (auto &g : gWs21Guarded)
    if (!g) {
      g = b;
      break;
    }
  return (uint16_t *)(b + WS21_GUARD);
}
inline void ws21FrameFree(uint16_t *p) {
  if (!p) return;
  uint8_t *b = (uint8_t *)p - WS21_GUARD;
  for (auto &g : gWs21Guarded)
    if (g == b) g = nullptr;
  heap_caps_free(b);
}
// How far into a guard something wrote: the bytes from the picture's edge out to the farthest one changed (0: none)
inline int ws21GuardReach(const uint8_t *g, bool before) {
  if (before) {
    for (int i = 0; i < WS21_GUARD; i++)
      if (g[i] != WS21_GUARD_FILL) return WS21_GUARD - i;
  } else {
    for (int i = WS21_GUARD - 1; i >= 0; i--)
      if (g[i] != WS21_GUARD_FILL) return i + 1;
  }
  return 0;
}
// The guards looked at (the 2 KB next to each picture: a row or two past an edge lands there first; all of a guard
// once one is hit), any hit said with where (the screen open) and filled again. True if one was hit
inline bool ws21GuardCheck(const char *where) {
  constexpr int NEAR = 2048;
  bool hit = false;
  for (uint8_t *b : gWs21Guarded) {
    if (!b) continue;
    uint8_t *pre = b, *post = b + WS21_GUARD + WS21_FRAME_ALLOC;
    bool dirty = false;
    for (int i = WS21_GUARD - NEAR; i < WS21_GUARD && !dirty; i++) dirty = pre[i] != WS21_GUARD_FILL;
    for (int i = 0; i < NEAR && !dirty; i++) dirty = post[i] != WS21_GUARD_FILL;
    if (!dirty) continue;
    hit = true;
    int rb = ws21GuardReach(pre, true), ra = ws21GuardReach(post, false);
    uint16_t px = rb ? *(uint16_t *)(pre + WS21_GUARD - 2) : *(uint16_t *)post;
    Serial.printf("GUARD: the picture at %p was written past its %s%s: %d bytes before, %d after (%.1f / %.1f rows), "
                  "pixel %04X, in %s\n",
                  b + WS21_GUARD, rb ? "top" : "", ra ? (rb ? " and bottom" : "bottom") : "", rb, ra,
                  rb / (2.f * LCD_WIDTH), ra / (2.f * LCD_WIDTH), px, where);
    memset(pre, WS21_GUARD_FILL, WS21_GUARD);
    memset(post, WS21_GUARD_FILL, WS21_GUARD);
  }
  return hit;
}
// The cache and the frames. PSRAM is only reached through the 32 KB data cache, and the DMA and the panel's refill go
// round it. Writing back by address walks every line of the range: 17 ms for a frame (2026-11-23). Dropping by
// address is quick, and writing back the whole cache only walks the cache. So: everything the CPU drew goes out with
// ws21Writeback() (the whole cache), and a frame's old lines are dropped with ws21Drop() where the DMA or the refill
// will go round them.
// The ROM's whole-cache write-back has no lock of its own: another cache operation started while it runs (the refill's
// drop, from its interrupt) stops the chip ("Dcache sync parameter configuration error", 2026-11-23). So it runs
// with this core's interrupts held off (about 0.7 ms), and the refill takes the same lock round its drop, in case
// it runs on the other core (REFILLCORE 0).
static portMUX_TYPE gWs21CacheMux = portMUX_INITIALIZER_UNLOCKED;
inline void ws21Writeback() {
  portENTER_CRITICAL(&gWs21CacheMux);
  Cache_WriteBack_All();
  portEXIT_CRITICAL(&gWs21CacheMux);
}
inline void ws21Drop(const void *p, size_t bytes) {  // (p and bytes on 32-byte lines; anything dirty there is lost)
  esp_cache_msync((void *)p, bytes, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
}

// Panel rows py0.. (n of them, PANEL_SIZE px each) into out: the 7 px border and the corners outside the circle black,
// the rest from rows, where frame row y is rows + (y - first) * LCD_WIDTH (the frame itself: first 0; a band the DMA
// brought into internal RAM: its first row). (The rim's shading is in the frame already: Ws21Canvas::shadeRim().)
// The panel's refill makes every row this way, and the console's SHOT too.
// Upside down (flip: the frame drawn upright, the board turned over), panel row y shows frame row 465 - y, right to
// left. The circle and the rim are the same either way up, so only the copy changes.
static IRAM_ATTR void ws21PanelRows(const uint16_t *rows, int first, uint16_t *out, int py0, int n, bool flip = false) {
  for (int r = 0; r < n; r++, out += PANEL_SIZE) {
    const int y = py0 + r - WS21_BORDER;
    if (y < 0 || y >= LCD_HEIGHT) {
      memset(out, 0, PANEL_SIZE * 2);
      continue;
    }
    const int x0 = gKnomiRim ? gKnomiRimX0[y] : 0, x1 = gKnomiRim ? gKnomiRimX1[y] : LCD_WIDTH;
    memset(out, 0, (size_t)(WS21_BORDER + x0) * 2);
    if (!flip) {
      memcpy(out + WS21_BORDER + x0, rows + (y - first) * LCD_WIDTH + x0, (size_t)(x1 - x0) * 2);
    } else {
      const uint16_t *src = rows + (LCD_HEIGHT - 1 - y - first) * LCD_WIDTH + (LCD_WIDTH - 1);
      uint16_t *o = out + WS21_BORDER;
      for (int x = x0; x < x1; x++) o[x] = src[-x];
    }
    memset(out + WS21_BORDER + x1, 0, (size_t)(LCD_WIDTH - x1 + WS21_BORDER) * 2);
  }
}

// The refill reading ahead by DMA (2026-11-23). Copying a band out of PSRAM with the CPU took about 350 us of every
// 900, a third of the game's core. So each refill, once its band is made, has the DMA bring the next band's rows
// into internal RAM (stage); the next refill finds them there and only copies them across, which is quick. If they
// aren't there (the frame changed in between, or the DMA isn't done), the CPU reads PSRAM as before. Its own channel
// pair, driven directly: esp_async_memcpy writes back the cache over the source by address first, every time (slow:
// the whole frame took 17 ms), and the frame on show never has anything in the cache to write back (present()).
// Done is counted by the DMA's own interrupt (the descriptors' owner bits weren't handed back, and a check on them
// made every later mount fail, logging from the refill's interrupt until the watchdog stopped the chip). A copy is
// never started while one is under way, and the refill never waits for one.
struct Ws21Prefetch {
  gdma_channel_handle_t tx = nullptr, rx = nullptr;
  gdma_link_list_handle_t txl = nullptr, rxl = nullptr;
  uint8_t *stage = nullptr;  // internal RAM
  size_t stageBytes = 0, extAlign = 32;
  const uint16_t *src = nullptr;  // what's coming or come: the frame,
  int py0 = -1;                   // its band (the first panel row; -1 none),
  int c0 = 0;                     // the band's first frame row,
  size_t off = 0;                 // where that row starts in stage
  bool empty = false;             // (only border rows: nothing to bring)
  bool flip = false;              // (upside down: the rows from the bottom)
  volatile uint32_t issued = 0, done = 0;
  volatile uint32_t hits = 0, misses = 0, late = 0;
};
static Ws21Prefetch gWs21Pre;
static IRAM_ATTR bool ws21PrefetchDone(gdma_channel_handle_t, gdma_event_data_t *, void *) {
  gWs21Pre.done++;
  return false;
}

static bool ws21PrefetchBegin(int bandRows) {
  Ws21Prefetch &p = gWs21Pre;
  if (p.tx) return true;
  gdma_channel_alloc_config_t tc = {};
  tc.direction = GDMA_CHANNEL_DIRECTION_TX;
  tc.flags.reserve_sibling = 1;
  if (gdma_new_ahb_channel(&tc, &p.tx) != ESP_OK) return false;
  gdma_channel_alloc_config_t rc = {};
  rc.direction = GDMA_CHANNEL_DIRECTION_RX;
  rc.sibling_chan = p.tx;
  if (gdma_new_ahb_channel(&rc, &p.rx) != ESP_OK) {
    gdma_del_channel(p.tx);
    p.tx = nullptr;
    return false;
  }
  // a free memory-to-memory trigger, as esp_async_memcpy takes one
  gdma_trigger_t trig = GDMA_MAKE_TRIGGER(GDMA_TRIG_PERIPH_M2M, 0);
  uint32_t ids = 0;
  esp_err_t e = gdma_get_free_m2m_trig_id_mask(p.rx, &ids);
  if (e == ESP_OK && ids) trig.instance_id = __builtin_ctz(ids);
  esp_err_t e1 = gdma_connect(p.rx, trig), e2 = gdma_connect(p.tx, trig);
  gdma_strategy_config_t st = {};
  st.owner_check = false;
  st.auto_update_desc = true;
  st.eof_till_data_popped = true;
  esp_err_t e3 = gdma_apply_strategy(p.tx, &st), e4 = gdma_apply_strategy(p.rx, &st);
  gdma_transfer_config_t xc = {};
  xc.max_data_burst_size = 32;
  xc.access_ext_mem = true;
  esp_err_t e5 = gdma_config_transfer(p.tx, &xc), e6 = gdma_config_transfer(p.rx, &xc);
  Serial.printf("ws21: refill DMA: trigger %d (mask %x, %d), connect %d %d, strategy %d %d, transfer %d %d\n",
                trig.instance_id, (unsigned)ids, e, e1, e2, e3, e4, e5, e6);
  size_t ia = 4, ea = 32;
  gdma_get_alignment_constraints(p.tx, &ia, &ea);
  p.extAlign = ea < 32 ? 32 : ea;
  p.stageBytes = (((size_t)bandRows * LCD_WIDTH * 2 + 2 * p.extAlign) + 63) & ~(size_t)63;
  p.stage = (uint8_t *)heap_caps_aligned_calloc(64, 1, p.stageBytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
  gdma_link_list_config_t lc = {};
  lc.num_items = p.stageBytes / 4032 + 2;
  lc.item_alignment = 4;
  lc.flags.check_owner = 0;
  gdma_rx_event_callbacks_t cb = {};
  cb.on_recv_eof = ws21PrefetchDone;
  if (!p.stage || gdma_new_link_list(&lc, &p.txl) != ESP_OK || gdma_new_link_list(&lc, &p.rxl) != ESP_OK ||
      gdma_register_rx_event_callbacks(p.rx, &cb, nullptr) != ESP_OK) {
    Serial.println("ws21: no room for the refill's DMA: the CPU refills");
    p.stage = nullptr;  // (the channels stay unused)
    return false;
  }
  return true;
}
// the DMA's copy of frame f's rows for the band starting at panel row py0 (n rows), into stage (upside down: the
// frame's rows from the bottom)
static void ws21PrefetchKick(const uint16_t *f, int py0, int n, bool flip) {
  Ws21Prefetch &p = gWs21Pre;
  p.py0 = -1;
  if (!p.stage || !f || p.issued != p.done) return;  // (one still under way: none this time)
  int c0 = py0 - WS21_BORDER, c1 = py0 + n - WS21_BORDER;
  c0 = c0 < 0 ? 0 : c0;
  c1 = c1 > LCD_HEIGHT ? LCD_HEIGHT : c1;
  if (flip && c1 > c0) {
    const int a = LCD_HEIGHT - c1, b = LCD_HEIGHT - c0;
    c0 = a, c1 = b;
  }
  p.flip = flip;
  p.src = f;
  p.c0 = c0;
  p.empty = c1 <= c0;
  if (p.empty) {
    p.py0 = py0;
    return;
  }
  const uintptr_t m = p.extAlign - 1;
  const uintptr_t a = (uintptr_t)(f + c0 * LCD_WIDTH) & ~m, b = ((uintptr_t)(f + c1 * LCD_WIDTH) + m) & ~m;
  if (b - a > p.stageBytes) return;
  p.off = (uintptr_t)(f + c0 * LCD_WIDTH) - a;
  gdma_buffer_mount_config_t tb = {};
  tb.buffer = (void *)a;
  tb.buffer_alignment = p.extAlign;
  tb.length = b - a;
  tb.flags.mark_eof = 1;
  tb.flags.mark_final = GDMA_FINAL_LINK_TO_NULL;
  gdma_buffer_mount_config_t rb = {};
  rb.buffer = p.stage;
  rb.buffer_alignment = 4;
  rb.length = b - a;
  rb.flags.mark_final = GDMA_FINAL_LINK_TO_NULL;
  if (gdma_link_mount_buffers(p.txl, 0, &tb, 1, nullptr) != ESP_OK) return;
  if (gdma_link_mount_buffers(p.rxl, 0, &rb, 1, nullptr) != ESP_OK) return;
  p.issued++;
  gdma_start(p.rx, gdma_link_get_head_addr(p.rxl));
  gdma_start(p.tx, gdma_link_get_head_addr(p.txl));
  p.py0 = py0;
}
// whether the band (f, py0) has come into stage
static bool ws21PrefetchReady(const uint16_t *f, int py0, bool flip) {
  Ws21Prefetch &p = gWs21Pre;
  if (p.py0 != py0 || p.src != f || p.flip != flip) return false;
  if (p.empty) return true;
  if (p.issued == p.done) return true;
  p.late++;
  return false;
}

// What the refill reads: the frames, and which is on show. Kept outside the panel so the refill can be in IRAM (a
// static member placed there won't link).
struct Ws21Show {
  uint16_t *frm[3] = { nullptr, nullptr, nullptr };
  volatile bool flip[3] = { false, false, false };  // each frame shown upside down (drawn upright, the board turned over)
  volatile uint8_t show = 0, next = 0;  // the frame being sent; the one to send from the next refresh
  volatile uint32_t fillUs = 0, fillN = 0, fillMax = 0;  // the refill's time (the console's LCD)
};
static Ws21Show gWs21Show;

// A bounce buffer to refill: the next band of the panel's rows (pos on), from the frame on show. A refresh's first
// band moves on to the frame waiting, if any. From what the DMA read ahead if it's there; if not, the CPU reads the
// frame, and the lines it read leave the cache afterwards (the refill would otherwise push the game's own data out
// of the 32 KB cache all the time). Then the DMA goes for the next band. In IRAM, and nothing per pixel: it runs
// over a thousand times a second.
static IRAM_ATTR bool ws21Fill(esp_lcd_panel_handle_t, void *bb, int pos, int len, void *) {
  Ws21Show &w = gWs21Show;
  Ws21Prefetch &p = gWs21Pre;
  const int64_t t0 = esp_timer_get_time();
  if (pos == 0) w.show = w.next;
  const uint16_t *f = w.frm[w.show];
  const bool flip = w.flip[w.show];
  const int py0 = pos / PANEL_SIZE, n = len / (PANEL_SIZE * 2);
  if (ws21PrefetchReady(f, py0, flip)) {
    ws21PanelRows((const uint16_t *)(p.stage + p.off), p.c0, (uint16_t *)bb, py0, n, flip);
    p.hits++;
  } else {
    ws21PanelRows(f, 0, (uint16_t *)bb, py0, n, flip);
    p.misses++;
    int c0 = py0 - WS21_BORDER, c1 = py0 + n - WS21_BORDER;
    c0 = c0 < 0 ? 0 : c0;
    c1 = c1 > LCD_HEIGHT ? LCD_HEIGHT : c1;
    if (flip && c1 > c0) {
      const int a = LCD_HEIGHT - c1, b = LCD_HEIGHT - c0;
      c0 = a, c1 = b;
    }
    if (c1 > c0) {  // (the frame on show has nothing dirty in the cache: present())
      const uintptr_t a = (uintptr_t)(f + c0 * LCD_WIDTH) & ~(uintptr_t)31;
      const uintptr_t b = ((uintptr_t)(f + c1 * LCD_WIDTH) + 31) & ~(uintptr_t)31;
      portENTER_CRITICAL_ISR(&gWs21CacheMux);
      esp_cache_msync((void *)a, b - a, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
      portEXIT_CRITICAL_ISR(&gWs21CacheMux);
    }
  }
  // the next band: this frame's, or the first of the next refresh, from the frame waiting by then
  int nx = pos + len / 2;
  const uint16_t *nf = f;
  bool nflip = flip;
  if (nx >= PANEL_SIZE * PANEL_SIZE) nx = 0, nf = w.frm[w.next], nflip = w.flip[w.next];
  ws21PrefetchKick(nf, nx / PANEL_SIZE, n, nflip);
  const uint32_t us = (uint32_t)(esp_timer_get_time() - t0);
  w.fillUs += us;
  w.fillN++;
  if (us > w.fillMax) w.fillMax = us;
  return false;
}

// The panel: the ST7701S's setup over the 3-wire bus, then the ESP32-S3's RGB peripheral. The RGB peripheral sends
// the panel's rows from two small bounce buffers in internal RAM, and each time one has gone out it is refilled from
// the frame on show by fill(): the black border and corners and the rim's shading are added there, row by row
// (ws21PanelRows()), so a frame is only the 466x466 picture. That saves the full-size copy every frame that a
// 480x480 frame buffer needed, and the refill reads only the circle out of PSRAM (Fedor, 2026-11-23: "feels so
// much slower than the Knomi"). Three frames in turn: one on show, one waiting for the next refresh, one being
// drawn, so the game never waits for a refresh and nothing is shown half drawn. The timings are Waveshare's
// (pin_config.h). The backlight is PWM on LCD_BL (a MOSFET: high = on).
class Ws21Panel : public Arduino_G {
public:
  explicit Ws21Panel(Ws21InitBus *b) : Arduino_G(PANEL_SIZE, PANEL_SIZE), bus(b) {}

  bool begin(int32_t = GFX_NOT_DEFINED) override {
    if (handle) return true;
    if (!bus->begin()) return false;
    bus->sendCommand(0x01);  // a software reset after the expander's, as Arduino_GFX's RGB display does
    delay(120);
    bus->batchOperation(WS21_INIT_OPS, sizeof(WS21_INIT_OPS));
#ifndef WS21_REFILL_DMA
#define WS21_REFILL_DMA 1
#endif
    if (WS21_REFILL_DMA && !ws21PrefetchBegin(LCD_BOUNCE_PX / PANEL_SIZE))
      Serial.println("ws21: no DMA channel for the refill: the CPU refills");
    for (auto &f : frm) {  // black (calloc), before the refill first reads one
      if (!f) f = ws21FrameAlloc();
      if (!f) return false;
      esp_cache_msync(f, WS21_FRAME_ALLOC, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_INVALIDATE);
    }
    // The core the refill runs on (its interrupt goes where the panel is made): the console's REFILLCORE, 1 (the
    // game's) unless set. On the other core the game's core gets about 7% more done, but the game's PSRAM work then
    // holds up the refill (bring-up, 2026-11-20)
    Preferences p;
    if (p.begin("ws21", true)) {
      refillCore = p.getUChar("rc", 1) ? 1 : 0;
      p.end();
    }
    if (refillCore == xPortGetCoreID()) {
      make();
    } else {  // made by a short task on the other core
      TaskHandle_t self = xTaskGetCurrentTaskHandle();
      void *args[2] = { this, self };
      xTaskCreatePinnedToCore([](void *a) {
        void **v = (void **)a;
        ((Ws21Panel *)v[0])->make();
        xTaskNotifyGive((TaskHandle_t)v[1]);
        vTaskDelete(nullptr);
      }, "ws21mk", 4096, args, 5, nullptr, refillCore);
      ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(2000));
    }
    return handle != nullptr;
  }
  uint8_t refillCore = 1;

  // the RGB panel and its interrupts, on the core this runs on
  void make() {
    esp_lcd_rgb_panel_config_t c = {};
    c.clk_src = LCD_CLK_SRC_DEFAULT;
    c.timings.pclk_hz = LCD_PCLK_HZ;
    c.timings.h_res = c.timings.v_res = PANEL_SIZE;
    c.timings.hsync_pulse_width = LCD_HSYNC_PW;
    c.timings.hsync_back_porch = LCD_HSYNC_BP;
    c.timings.hsync_front_porch = LCD_HSYNC_FP;
    c.timings.vsync_pulse_width = LCD_VSYNC_PW;
    c.timings.vsync_back_porch = LCD_VSYNC_BP;
    c.timings.vsync_front_porch = LCD_VSYNC_FP;
    c.timings.flags.hsync_idle_low = LCD_HSYNC_POL == 0;
    c.timings.flags.vsync_idle_low = LCD_VSYNC_POL == 0;
    c.timings.flags.pclk_active_neg = LCD_PCLK_NEG;
    c.data_width = 16;
    c.bits_per_pixel = 16;
    c.flags.no_fb = 1;  // the frames are ours (frm), and fill() makes the panel's rows from them
    c.bounce_buffer_size_px = LCD_BOUNCE_PX;
    c.dma_burst_size = 64;
    c.hsync_gpio_num = LCD_HSYNC;
    c.vsync_gpio_num = LCD_VSYNC;
    c.de_gpio_num = LCD_DE;
    c.pclk_gpio_num = LCD_PCLK;
    c.disp_gpio_num = -1;
    static const int8_t DATA[16] = { LCD_B0, LCD_B1, LCD_B2, LCD_B3, LCD_B4, LCD_G0, LCD_G1, LCD_G2,
                                     LCD_G3, LCD_G4, LCD_G5, LCD_R0, LCD_R1, LCD_R2, LCD_R3, LCD_R4 };
    for (int i = 0; i < 16; i++) c.data_gpio_nums[i] = DATA[i];
    // Made on the game's core (unless REFILLCORE says otherwise), so its refill interrupt runs there too: then the
    // refill (each band copied out of PSRAM) and the game's own heavy PSRAM work never run at the same moment. With
    // the refill on the other core and flush() shared by both, they did, the refill fell behind and the streaks came
    // back (2026-11-20, twice: the second time with flush() copying by DMA and leaving the cache clean).
    esp_lcd_panel_handle_t h = nullptr;
    if (esp_lcd_new_rgb_panel(&c, &h) != ESP_OK) return;
    esp_lcd_rgb_panel_event_callbacks_t cb = {};
    cb.on_frame_buf_complete = frameDone;
    cb.on_vsync = frameStart;
    cb.on_bounce_empty = ws21Fill;
    esp_lcd_rgb_panel_register_event_callbacks(h, &cb, this);
    findEofCount(h);
    esp_lcd_panel_reset(h);
    esp_lcd_panel_init(h);
    handle = h;
  }

  // The frame to draw the next one into: neither the one on show nor the one waiting to be. With three there is
  // always one, at once. (The refill only ever moves on to the waiting one, so the one returned stays free.)
  uint16_t *backBuffer() {
    const uint8_t a = gWs21Show.show, b = gWs21Show.next;
    for (uint8_t k = 0; k < 3; k++)
      if (k != a && k != b) return frm[k];
    return frm[(a + 1) % 3];  // (never)
  }
  // f is shown from the next refresh on. Anything the CPU drew into it goes out of the cache to PSRAM first, where the
  // refill reads it, and its lines leave the cache (the refill's reads mustn't find old copies there).
  // flip: f is upright and the board is upside down, so the refill turns it over
  void present(uint16_t *f, bool flip = false) {
    ws21Writeback();
    ws21Drop(f, WS21_FRAME_ALLOC);
    const uint8_t k = f == frm[0] ? 0 : f == frm[1] ? 1 : 2;
    gWs21Show.flip[k] = flip;
    gWs21Show.next = k;
    presented = f;
  }
  // the panel's rows py0.. from the frame on show (the console's SHOT)
  void shownRows(uint16_t *out, int py0, int n) {
    const uint8_t k = gWs21Show.show;
    ws21PanelRows(frm[k], 0, out, py0, n, gWs21Show.flip[k]);
  }
  uint16_t *frontBuffer() const { return frm[gWs21Show.show]; }
  uint16_t *lastPresented() const { return presented; }
  bool isFrame(const uint16_t *f) const { return f == frm[0] || f == frm[1] || f == frm[2]; }
  // the pixel clock, from the next refresh on (the console's PCLK, to find the fastest one that doesn't glitch)
  bool setPclk(uint32_t hz) { return handle && esp_lcd_rgb_panel_set_pclk(handle, hz) == ESP_OK; }
  // which edge of PCLK the data changes on (the console's PCLKEDGE, bring-up): 1 = falling
  void setPclkEdge(bool falling) { LCD_CAM.lcd_clock.lcd_ck_out_edge = falling; }
  // the refills' timing since the last call (the console's LCD): frames sent, and the longest gap between two ends of
  // a frame, in us (a frame is 1e6 / refresh rate; much more means the refill was held up)
  void timing(uint32_t *n, uint32_t *maxGap, uint32_t *late) {
    *n = frames - statFrom;
    *maxGap = gapMax;
    *late = gapLate;
    statFrom = frames;
    gapMax = gapLate = 0;
  }
  // Refreshes since the last call where a refill signal was lost (eofMiss), and where the refill was then out of
  // step (eofOdd: the picture 20 lines up, with streaks). With the fix on, each is put right at the next refresh.
  void eofStats(uint32_t *miss, uint32_t *odd, bool *found) {
    *miss = eofMiss;
    *odd = eofOdd;
    *found = eofCount != nullptr;
    eofMiss = eofOdd = 0;
  }
  volatile bool eofFix = true;  // the console's EOFFIX (testing): 0 leaves the driver as it is

  void setBrightness(uint8_t v) {
    if (!blReady) blReady = ledcAttach(LCD_BL, LCD_BL_PWM_HZ, 8);
    if (blReady) ledcWrite(LCD_BL, v);  // TODO(bring-up): no flicker at low levels (the IDF demo uses 5 kHz)
  }

  // (the canvas draws into the frame buffers itself: nothing comes through these)
  void drawBitmap(int16_t, int16_t, uint8_t *, int16_t, int16_t, uint16_t, uint16_t) override {}
  void drawIndexedBitmap(int16_t, int16_t, uint8_t *, uint16_t *, int16_t, int16_t, int16_t) override {}
  void draw3bitRGBBitmap(int16_t, int16_t, uint8_t *, int16_t, int16_t) override {}
  void draw16bitRGBBitmap(int16_t, int16_t, uint16_t *, int16_t, int16_t) override {}
  void draw24bitRGBBitmap(int16_t, int16_t, uint8_t *, int16_t, int16_t) override {}

private:
  // the RGB driver's interrupt, each time it has sent a whole buffer: from then on it reads the one last presented
  // (not IRAM_ATTR: this core's RGB interrupt isn't IRAM-safe either, and inline it won't link)
  static bool frameDone(esp_lcd_panel_handle_t, const esp_lcd_rgb_panel_event_data_t *, void *ctx) {
    Ws21Panel *p = (Ws21Panel *)ctx;
    p->frames++;
    int64_t t = esp_timer_get_time();
    if (p->lastFrameUs) {
      uint32_t gap = (uint32_t)(t - p->lastFrameUs);
      if (gap > p->gapMax) p->gapMax = gap;
      if (gap > 1000000u / 20) p->gapLate++;  // (more than 50 ms: a refill badly held up)
    }
    p->lastFrameUs = t;
    return false;
  }

  // The driver's fault, put right here (2026-11-21: overnight, the picture 20 lines up, with streaks).
  // The driver refills its two bounce buffers in turn. It picks which one by counting the DMA's "buffer sent"
  // signals: even = the first, odd = the second. A flash write (the save) holds up interrupts. If it lasts more
  // than one buffer (0.9 ms), two signals arrive as one and the count is off by one. From then on, each refill goes
  // into the buffer being sent, not the one just sent. The picture is a buffer (20 lines) late, and streaks show
  // wherever a refill loses the race. The driver restarts the DMA from the first buffer at every refresh, but in
  // IDF 5.5.4 it never resets the count, so it stays wrong until another hold-up happens to put it back.
  // Espressif's fix (IDF master): the count goes to 0 at that restart. This callback runs just before it, in the
  // same interrupt, and does the same.
  static bool frameStart(esp_lcd_panel_handle_t, const esp_lcd_rgb_panel_event_data_t *, void *ctx) {
    Ws21Panel *p = (Ws21Panel *)ctx;
    volatile size_t *c = p->eofCount;
    if (!c) return false;
    size_t n = *c;
    if (n - p->eofLast != EOFS_PER_FRAME) p->eofMiss++;
    if (n & 1) p->eofOdd++;  // the next refresh would refill the wrong buffer
    if (p->eofFix) n = 0, *c = 0;
    p->eofLast = n;
    return false;
  }
  // Finds the driver's count. The driver's state is private, so it is found by what sits after it: the number of
  // signals a frame should have, then the callbacks (frame sent, refresh start, refill) and ctx just registered. Not found (another IDF), nothing changes.
  void findEofCount(esp_lcd_panel_handle_t h) {
    const uint32_t *w = (const uint32_t *)h;
    for (int i = 1; i < 256; i++) {
      if (w[i] == EOFS_PER_FRAME && w[i + 1] == 0 && w[i + 2] == (uint32_t)(uintptr_t)frameDone &&
          w[i + 3] == (uint32_t)(uintptr_t)frameStart && w[i + 4] == (uint32_t)(uintptr_t)ws21Fill &&
          w[i + 5] == (uint32_t)(uintptr_t)this) {
        eofLast = w[i - 1];
        eofCount = (volatile size_t *)&w[i - 1];
        return;
      }
    }
    Serial.println("ws21: the driver's refill count wasn't found: no fix for lost refill signals");
  }
  static constexpr uint32_t EOFS_PER_FRAME = (uint32_t)PANEL_SIZE * PANEL_SIZE / LCD_BOUNCE_PX;
  volatile size_t *eofCount = nullptr;
  size_t eofLast = 0;
  volatile uint32_t eofMiss = 0, eofOdd = 0;
  Ws21InitBus *bus;
  esp_lcd_panel_handle_t handle = nullptr;
  uint16_t *(&frm)[3] = gWs21Show.frm;  // the frames
  uint16_t *presented = nullptr;        // the last one given to present()
  volatile uint32_t frames = 0;
  int64_t lastFrameUs = 0;
  volatile uint32_t gapMax = 0, gapLate = 0;
  uint32_t statFrom = 0;
  bool blReady = false;
};

void pressOverlay();  // the sketch's press preview, drawn over every frame

// a DMA copy of the frame done (its interrupt; ctx: the count). In IRAM, in case the interrupt runs while the flash is
// busy (a free function: inline in the class it won't link)
static IRAM_ATTR bool ws21DmaDone(async_memcpy_handle_t, async_memcpy_event_t *, void *ctx) {
  (*(volatile uint32_t *)ctx)++;
  return false;
}

// The 466x466 canvas in PSRAM, where the game draws. flush() puts it into a frame (Ws21Panel) and shows that:
// - straight (the usual): the whole canvas by DMA, PSRAM to PSRAM. The CPU only waits (it would stall on PSRAM).
// - turned (gWs21RotDeg, the console's ROTDEG): the CPU makes each band of the turned picture in internal RAM and the
//   DMA takes it out to the frame while the next band is made. The console's SPLIT 1 has a task on the other core
//   make every other band (each core with its own DMA): 135 ms a frame turned and blended instead of 160.
// - direct (setDirect(), the usual): the game draws straight into the frame to be shown, so there's nothing to copy
//   at all. The canvas then is that frame, a different one each time, and what's in it is from three frames back.
//   Every screen paints all of itself each time it's drawn (render(); one that hasn't changed isn't drawn or sent
//   at all), and so does the update's progress screen, so all can draw this way. Upside down too (the refill turns
//   the frame over); not sideways.
// The panel's refill adds the border and the rim (ws21PanelRows()).
//
// Why DMA (bring-up, 2026-11-20): the CPU writing PSRAM through the cache reads every line in first and writes it
// back after (68 ms a frame just to copy). The DMA writes PSRAM straight and leaves nothing in the cache.
inline bool gWs21Split = false;
inline bool gWs21DirectOk = true;  // the console's DIRECT 0: no direct drawing (comparing)
// Frame profiling (the console's PROF): marks inside a game's frame, the time since the last mark added to slot k
inline uint32_t gWs21ProfUs[12];
inline uint32_t gWs21ProfN = 0;
inline int64_t gWs21ProfT = 0;
inline void ws21ProfStart() { gWs21ProfT = esp_timer_get_time(); gWs21ProfN++; }
inline void ws21ProfMark(int k) {
  int64_t t = esp_timer_get_time();
  gWs21ProfUs[k] += (uint32_t)(t - gWs21ProfT);
  gWs21ProfT = t;
}
class Ws21Canvas : public Arduino_Canvas {
public:
  Ws21Canvas(Ws21Panel *out) : Arduino_Canvas(LCD_WIDTH, LCD_HEIGHT, out), panel(out) {}

  // the RGB panel keeps its own pixel clock (LCD_PCLK_HZ): the speed the sketch asks for is the Knomi's SPI's
  bool begin(int32_t = GFX_NOT_DEFINED) override {
    if (!Arduino_Canvas::begin(GFX_NOT_DEFINED)) return false;
    if (!panel->frontBuffer()) return false;
    uint16_t *c = ws21FrameAlloc();  // the canvas on whole 64-byte lines, as the frames, for the DMA
    if (!c) return false;
    memcpy(c, _framebuffer, WS21_FRAME_BYTES);
    free(_framebuffer);
    _framebuffer = canvas = c;
    knomiRimBegin(LCD_WIDTH);
    ws21SetRotation(WS21_ROT_DEG);
    installDma(side[0]);  // (the other core's only once SPLIT asks for it)
    go = xSemaphoreCreateBinary();
    back = xSemaphoreCreateBinary();
    // below the touch task (5) and the buzzer (4), so the half frame made there never holds up a tap or a beep
    if (go && back) xTaskCreatePinnedToCore(workerTask, "ws21half", 3072, this, 3, &worker, 0);
    return true;
  }

  void flush(bool = false) override {
    const int64_t t0 = esp_timer_get_time();
    if (direct && !directTurn()) setDirect(false);  // (just turned sideways: the canvas, with the last frame, goes out turned)
    pressOverlay();
    const int64_t t1 = esp_timer_get_time();
    int64_t waited = 0;
    uint16_t *f = _framebuffer;  // direct: drawn into already
    if (!direct) {
      f = panel->backBuffer();
      if (!gWs21RotDeg) {
        const int64_t w0 = esp_timer_get_time();
        ws21Writeback();  // what the game left of the canvas in the cache goes out to PSRAM first, where the DMA reads it
        prof[5] += esp_timer_get_time() - w0;
        waited = copy(f, canvas, WS21_FRAME_ALLOC);
      } else {
        if (!allocBands(0)) return;
        const bool split = gWs21Split && worker && allocBands(1);
        if (split) installDma(side[1]);
        curFb = f;
        if (split) xSemaphoreGive(go);
        waited = doBands(0, f, split ? 2 : 1, 0);
        if (split && xSemaphoreTake(back, pdMS_TO_TICKS(300)) != pdTRUE) {
          Serial.println("ws21: the other core's half of the frame didn't come back; one core from now on");
          gWs21Split = false;
        }
      }
    }
    const int64_t t2 = esp_timer_get_time();
    if (!direct) ws21Drop(f, WS21_FRAME_ALLOC);  // (written by the DMA: no old lines of it to read below)
    shadeRim(f);
    panel->present(f, direct && gWs21RotDeg == 180);
    if (direct) handOut();  // the next frame is drawn into another
    const int64_t t3 = esp_timer_get_time();
    prof[0] += t1 - t0;           // the press overlay
    prof[1] += t2 - t1 - waited;  // the CPU's part of the copy (turned: making the rows)
    prof[2] += waited;            // waiting for the DMA (and the other core)
    prof[3] += t3 - t2;           // the frame out of the cache and handed over
    profN++;
  }

  // Direct drawing on or off (above). On: the game draws into a frame from the next flush (this one's, if it hasn't
  // drawn yet). Off: the canvas is back, with the last frame shown copied into it, for anything that builds on what's
  // there (a page sliding). Not while turned: the frame is the turned picture, not the canvas.
  // upright or upside down (the refill turns it over); sideways the frame has to be made from the canvas
  static bool directTurn() { return gWs21RotDeg == 0 || gWs21RotDeg == 180; }
  void setDirect(bool on) {
    if (!directTurn()) on = false;
    if (on == direct) return;
    if (on) {
      direct = true;
      handOut();
      return;
    }
    direct = false;
    ws21Drop(_framebuffer, WS21_FRAME_ALLOC);  // (half drawn, never shown: dropped)
    ws21Drop(canvas, WS21_FRAME_ALLOC);        // (all of it is about to be written)
    if (const uint16_t *last = panel->lastPresented()) copy(canvas, last, WS21_FRAME_ALLOC);
    _framebuffer = canvas;
  }
  bool isDirect() const { return direct; }

  // A whole picture (src: on 64-byte lines, WS21_FRAME_ALLOC long and written back, ws21Writeback()) into the canvas,
  // by DMA: Pinball's table under what moves. The CPU's copy reads every line of both and took 36 ms a frame.
  void restoreFrom(const uint16_t *src) {
    ws21Drop(_framebuffer, WS21_FRAME_ALLOC);  // (all of it is about to be written)
    const int64_t t = esp_timer_get_time();
    copy(_framebuffer, src, WS21_FRAME_ALLOC);
    prof[4] += esp_timer_get_time() - t;
  }

  // the console's SHOT: the panel's 480 rows as shown, RGB565
  void shotWrite(Print &out) {
    static uint16_t row[PANEL_SIZE];
    for (int y = 0; y < PANEL_SIZE; y++) {
      panel->shownRows(row, y, 1);
      out.write((const uint8_t *)row, sizeof row);
    }
  }

  // the console's FPS: flush()'s parts since the last call, in ms per frame
  void profPrint() {
    const float n = profN ? profN : 1;
    Serial.printf("flush n=%u overlay=%.1f cpu=%.1f (writeback %.1f) dma=%.1f present=%.1f total=%.1f restore=%.1f ms %s "
                  "split=%d refill=%d\n",
                  (unsigned)profN, prof[0] / 1000.f / n, prof[1] / 1000.f / n, prof[5] / 1000.f / n, prof[2] / 1000.f / n,
                  prof[3] / 1000.f / n, (prof[0] + prof[1] + prof[2] + prof[3]) / 1000.f / n, prof[4] / 1000.f / n,
                  direct ? "direct" : side[0].dma ? "dma" : "cpu", gWs21Split && worker ? 1 : 0, panel->refillCore);
    memset(prof, 0, sizeof prof);
    profN = 0;
  }

private:
  // 8 rows a band (turned)
  static constexpr int BAND = 8;
  static constexpr size_t BAND_BYTES = (size_t)BAND * LCD_WIDTH * 2;  // (a multiple of 32, as the DMA to PSRAM wants)
  // each core's own: its DMA and the counts of its copies
  struct Side {
    async_memcpy_handle_t dma = nullptr;
    volatile uint32_t done = 0;    // DMA copies finished (its interrupt counts them)
    uint32_t issued = 0;           // ...and asked for
    uint32_t ticket[2] = { 0, 0 };  // the count each of its two band buffers' last copy finishes at
    bool tried = false;            // its DMA asked for (installDma)
  };
  // a side's DMA, installed the first time it's needed. Each takes one of the S3's five DMA channel pairs, and with
  // the amplifier's output (2026-11-22) there was none left for the TLS encryption ("no free gdma channel", HTTPS
  // failed): so the other core's (SPLIT), off, no longer takes one at start-up. Without one, that side's copy is the
  // CPU's
  void installDma(Side &d) {
    if (d.dma || d.tried) return;
    d.tried = true;
    async_memcpy_config_t mc = ASYNC_MEMCPY_DEFAULT_CONFIG();
    mc.backlog = 4;
    mc.dma_burst_size = 32;  // (the frames start on 64-byte lines)
    if (esp_async_memcpy_install_gdma_ahb(&mc, &d.dma) != ESP_OK) {
      d.dma = nullptr;
      Serial.println("ws21: no DMA for the frame copy, the CPU copies it");
    }
  }
  Side side[2];
  uint16_t *canvas = nullptr;  // the canvas proper (direct drawing points _framebuffer at a frame instead)
  bool direct = false;
  uint16_t *bands[4] = { nullptr, nullptr, nullptr, nullptr };  // internal RAM: two for each side (turned only)
  bool allocBands(int k) {
    for (int i = 2 * k; i < 2 * k + 2; i++) {
      if (!bands[i]) bands[i] = (uint16_t *)heap_caps_aligned_calloc(64, 1, BAND_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);
      if (!bands[i]) return false;
    }
    return true;
  }
  TaskHandle_t worker = nullptr;
  SemaphoreHandle_t go = nullptr, back = nullptr;
  uint16_t *volatile curFb = nullptr;
  int64_t prof[6] = { 0, 0, 0, 0, 0, 0 };
  uint32_t profN = 0;

  // The Knomi's soft rim, shaded into the frame (knomi_rim.h): only its edge pixels, once a frame. A need bar's label
  // drawn over it this frame stays as it is (Fedor: "make the food, energy, joy, and hyg lables be above the shadow").
  // Turned, the rim is the frame's (the panel's), and a label's lift by where it lands, not where it was drawn.
  // The rim is the same in all four quarters, so a quarter of it is kept, in internal RAM (rimPrepare()). Where it
  // darkens by less than 8/256 it's left out: on the panel's 5- and 6-bit colours that's at most one step, and it
  // takes the ring from 45 px wide to 29 (59 to 40 thousand pixels). Shaded from the PSRAM table, it was 19 ms a frame.
  static constexpr int RIM_KEEP = 248;
  static constexpr int HALF = LCD_HEIGHT / 2;  // (233: the canvas is even, so the halves mirror exactly)
  uint8_t *rimQ = nullptr;                     // each quarter row's darkening, from its first shaded column inwards
  uint16_t rimOff[HALF];
  int16_t rimA[HALF], rimB[HALF];              // quarter row y: columns rimA..rimB-1 are shaded
  void rimPrepare() {
    if (rimQ || !gKnomiRim) return;
    size_t total = 0;
    for (int y = 0; y < HALF; y++) {
      const uint8_t *r = gKnomiRim + y * LCD_WIDTH;
      int a = gKnomiRimX0[y], b = a;
      while (b < HALF && r[b] < RIM_KEEP) b++;
      if (a > HALF) a = b = HALF;
      rimA[y] = a, rimB[y] = b, rimOff[y] = total;
      total += b - a;
    }
    uint8_t *q = (uint8_t *)heap_caps_malloc(total ? total : 1, MALLOC_CAP_INTERNAL);
    if (!q) return;
    for (int y = 0; y < HALF; y++) memcpy(q + rimOff[y], gKnomiRim + y * LCD_WIDTH + rimA[y], rimB[y] - rimA[y]);
    rimQ = q;
  }
  void shadeRim(uint16_t *f) {
    rimPrepare();
    if (!rimQ) return;
    const bool lift = gKnomiLiftOn;
    for (int y = 0; y < LCD_HEIGHT; y++) {
      const int qy = y < HALF ? y : LCD_HEIGHT - 1 - y, a = rimA[qy], n = rimB[qy] - a;
      if (n <= 0) continue;
      const uint8_t *k = rimQ + rimOff[qy];
      uint16_t *l = f + y * LCD_WIDTH + a, *r = f + y * LCD_WIDTH + (LCD_WIDTH - 1 - a);  // (r runs leftwards)
      if (!lift) {
        for (int i = 0; i < n; i++) l[i] = knomiRimPixel(l[i], k[i]);
        for (int i = 0; i < n; i++) r[-i] = knomiRimPixel(r[-i], k[i]);
        continue;
      }
      for (int i = 0; i < n; i++) {
        const int xl = a + i, xr = LCD_WIDTH - 1 - xl;
        if (!knomiLifted(xl >> 1, y >> 1)) l[i] = knomiRimPixel(l[i], k[i]);
        if (!knomiLifted(xr >> 1, y >> 1)) r[-i] = knomiRimPixel(r[-i], k[i]);
      }
    }
  }

  // direct: the next frame to draw into, out of the cache first (an old copy of its lines there would be drawn on)
  void handOut() {
    _framebuffer = panel->backBuffer();
    ws21Drop(_framebuffer, WS21_FRAME_ALLOC);
  }

  // dst <- src, bytes (whole 32-byte lines, both starting on one), by DMA in 32 KB pieces, three at a time; returns
  // the time spent waiting, in us. Without a DMA the CPU copies, and writes it back for the refill
  int64_t copy(uint16_t *dst, const uint16_t *src, size_t bytes) {
    Side &d = side[0];
    int64_t waited = 0;
    static constexpr size_t PIECE = 32768;
    for (size_t off = 0; off < bytes; off += PIECE) {
      const size_t n = bytes - off < PIECE ? bytes - off : PIECE;
      uint8_t *to = (uint8_t *)dst + off;
      const uint8_t *from = (const uint8_t *)src + off;
      if (d.dma) {
        if (d.issued - d.done >= 3) waited += waitDma(d, d.issued - 2);
        if (esp_async_memcpy(d.dma, to, (void *)from, n, ws21DmaDone, (void *)&d.done) == ESP_OK) {
          d.issued++;
          continue;
        }
        Serial.printf("ws21: DMA copy refused (%p <- %p, %u bytes), the CPU copies from now on\n", to, from, (unsigned)n);
        waited += waitDma(d, d.issued);
        d.dma = nullptr;
      }
      memcpy(to, from, n);
      ws21Writeback();
    }
    return waited + waitDma(d, d.issued);
  }

  static void workerTask(void *arg) {
    Ws21Canvas *c = (Ws21Canvas *)arg;
    for (;;) {
      if (xSemaphoreTake(c->go, portMAX_DELAY) != pdTRUE) continue;
      c->doBands(1, c->curFb, 2, 1);
      xSemaphoreGive(c->back);
    }
  }

  // turned: bands first, first + step, ... of the frame into f, by side k (0: the game's core, 1: the other); returns
  // the time spent waiting for its DMA, in us. All its copies are done when it returns
  int64_t doBands(int k, uint16_t *f, int step, int first) {
    Side &d = side[k];
    int64_t waited = 0;
    int j = 0;
    for (int i = first; i * BAND < LCD_HEIGHT; i += step, j++) {
      const int y0 = i * BAND, n = LCD_HEIGHT - y0 < BAND ? LCD_HEIGHT - y0 : BAND;
      uint16_t *buf = bands[2 * k + (j & 1)];
      waited += waitDma(d, d.ticket[j & 1]);  // the band before last, from this buffer, is out
      makeBand(buf, y0, n);
      // (the last band, 2 rows, rounded up to whole 32-byte lines: it runs into the frame's spare end)
      d.ticket[j & 1] = send(d, f + (size_t)y0 * LCD_WIDTH, buf, ((size_t)n * LCD_WIDTH * 2 + 31) & ~(size_t)31);
    }
    return waited + waitDma(d, d.issued);
  }

  // a band out to the frame: by DMA, or by the CPU if that's refused (then written back from the cache at once, for
  // the refill). Returns the count to wait for.
  uint32_t send(Side &d, uint16_t *to, uint16_t *from, size_t bytes) {
    if (d.dma) {
      if (esp_async_memcpy(d.dma, to, from, bytes, ws21DmaDone, (void *)&d.done) == ESP_OK) return ++d.issued;
      Serial.printf("ws21: DMA copy refused (%p, %u bytes), the CPU copies from now on\n", to, (unsigned)bytes);
      waitDma(d, d.issued);
      d.dma = nullptr;
    }
    memcpy(to, from, bytes);
    ws21Writeback();
    return d.issued;
  }
  // waits for the count t
  int64_t waitDma(Side &d, uint32_t t) {
    if ((int32_t)(d.done - t) >= 0) return 0;
    const int64_t t0 = esp_timer_get_time();
    while ((int32_t)(d.done - t) < 0) {
      if (esp_timer_get_time() - t0 > 100000) {  // never: a lost interrupt would otherwise hang the game
        Serial.println("ws21: DMA copy timed out, the CPU copies from now on");
        d.done = d.issued;
        d.dma = nullptr;
        break;
      }
    }
    return esp_timer_get_time() - t0;
  }

  // A quarter turn either way (q 1: 90 degrees clockwise, 3: 270): each frame row is a canvas column. Each canvas
  // row gives the band's rows their pixel in one column, so each canvas line is read once a band
  void quarterBand(uint16_t *out, int y0, int n, int q) {
    const uint16_t *cv = _framebuffer;
    for (int x = 0; x < LCD_WIDTH; x++) {
      if (q == 1) {  // frame (x, y) shows canvas (y, 465 - x)
        const uint16_t *src = cv + (LCD_HEIGHT - 1 - x) * LCD_WIDTH + y0;
        for (int r = 0; r < n; r++) out[r * LCD_WIDTH + x] = src[r];
      } else {  // frame (x, y) shows canvas (465 - y, x)
        const uint16_t *src = cv + x * LCD_WIDTH + (LCD_WIDTH - 1 - y0);
        for (int r = 0; r < n; r++) out[r * LCD_WIDTH + x] = src[-r];
      }
    }
  }

  // the turned picture's rows y0..y0+n-1 into out (rows LCD_WIDTH apart). The rim is the refill's, by the frame's
  // position (so a label lifted over it lifts where it lands on the panel, not where it was drawn: close enough)
  void makeBand(uint16_t *out, int y0, int n) {
    if (gWs21RotDeg == 90 || gWs21RotDeg == 270) {
      quarterBand(out, y0, n, gWs21RotDeg / 90);
      return;
    }
    if (gWs21RotDeg == 180) {
      for (int r = 0; r < n; r++) {
        const uint16_t *src = _framebuffer + (LCD_HEIGHT - 1 - y0 - r) * LCD_WIDTH;
        uint16_t *o = out + r * LCD_WIDTH;
        for (int x = 0; x < LCD_WIDTH; x++) o[x] = src[LCD_WIDTH - 1 - x];
      }
      return;
    }
    // Turned: each frame pixel takes the canvas point turned back onto it, blended from its four pixels (smooth) or
    // the nearest one. In tiles 32 wide, so the canvas lines a tile reads (a small turned square) stay in the cache
    // from one of its rows to the next
    const uint8_t sm = gWs21RotDeg % 90 ? gWs21Smooth : 0;  // (a quarter turn lands on whole pixels: nothing to blend)
    for (int tx = 0; tx < LCD_WIDTH; tx += TILE) {
      const int te = tx + TILE < LCD_WIDTH ? tx + TILE : LCD_WIDTH;
      for (int r = 0; r < n; r++) turnPart(out + r * LCD_WIDTH, tx, te, nullptr, y0 + r, sm);
    }
  }
  static constexpr int TILE = 32;

  // pixels a..b-1 of canvas row y, turned
  void turnPart(uint16_t *o, int a, int b, const uint8_t *rim, int y, uint8_t sm) {
    if (a >= b) return;
    const int32_t c = gWs21RotC, s = gWs21RotS;
    const int32_t X2 = 2 * a + 1 - LCD_WIDTH, Y2 = 2 * y + 1 - LCD_HEIGHT;  // twice the distance from the centre
    int32_t fu = ((X2 * c + Y2 * s) >> 1) + ((LCD_WIDTH / 2) << 16);
    int32_t fv = ((Y2 * c - X2 * s) >> 1) + ((LCD_HEIGHT / 2) << 16);
    if (sm) {  // (from the nearest pixel's centre to the top-left one's)
      fu -= 32768;
      fv -= 32768;
    }
    turnRun(o, a, b, fu, fv, rim, y, sm);
  }

  // panel pixels a..b-1 of a row, the first one's canvas point at (fu, fv) (16.16); rim: the row's darkening, or none
  __attribute__((optimize("O2"))) void turnRun(uint16_t *out, int a, int b, int32_t fu, int32_t fv,
                                               const uint8_t *rim, int y, uint8_t sm) {
    if (a >= b) return;
    const int32_t c = gWs21RotC, s = gWs21RotS;
    const uint16_t *src = _framebuffer;
    uint16_t *o = out + a;
    const int n = b - a;
    // All of it well inside the canvas (and the pixel past it, for the blend)? Its two ends say: it's a straight line
    const int32_t lim = (sm ? LCD_WIDTH - 2 : LCD_WIDTH - 1) << 16 | 0xFFFF;
    const int32_t fuB = fu + (n - 1) * c, fvB = fv - (n - 1) * s;
    if (!rim && fu >= 0 && fv >= 0 && fuB >= 0 && fvB >= 0 && fu <= lim && fv <= lim && fuB <= lim && fvB <= lim) {
      if (sm == 2) {  // blended only where the point falls between two cells of the 2 px grid
        for (int k = 0; k < n; k++, fu += c, fv -= s) {
          const uint32_t u = (uint32_t)fu >> 16, v = (uint32_t)fv >> 16;
          const uint16_t *p = src + v * LCD_WIDTH + u;
          uint32_t top = spread(p[0]);
          if (u & 1) top = mix(top, spread(p[1]), ((uint32_t)fu >> 11) & 31);
          if (v & 1) {
            const uint16_t *q = p + LCD_WIDTH;
            uint32_t bot = spread(q[0]);
            if (u & 1) bot = mix(bot, spread(q[1]), ((uint32_t)fu >> 11) & 31);
            top = mix(top, bot, ((uint32_t)fv >> 11) & 31);
          }
          o[k] = pack(top);
        }
      } else if (sm) {
        for (int k = 0; k < n; k++, fu += c, fv -= s) {
          const uint32_t u = (uint32_t)fu >> 16, v = (uint32_t)fv >> 16;
          const uint32_t wu = ((uint32_t)fu >> 11) & 31, wv = ((uint32_t)fv >> 11) & 31;
          const uint16_t *p = src + v * LCD_WIDTH + u;
          const uint32_t top = mix(spread(p[0]), spread(p[1]), wu);
          const uint32_t bot = mix(spread(p[LCD_WIDTH]), spread(p[LCD_WIDTH + 1]), wu);
          o[k] = pack(mix(top, bot, wv));
        }
      } else {
        for (int k = 0; k < n; k++, fu += c, fv -= s) o[k] = src[((uint32_t)fv >> 16) * LCD_WIDTH + ((uint32_t)fu >> 16)];
      }
      return;
    }
    for (int k = 0; k < n; k++, fu += c, fv -= s) {  // near the edge: each point kept on the canvas
      int u = fu >> 16, v = fv >> 16;
      uint16_t px;
      if (sm) {
        const uint32_t wu = (fu >> 11) & 31, wv = (fv >> 11) & 31;
        if (u < 0) u = 0;
        if (v < 0) v = 0;
        if (u > LCD_WIDTH - 1) u = LCD_WIDTH - 1;
        if (v > LCD_HEIGHT - 1) v = LCD_HEIGHT - 1;
        const int u1 = u + 1 < LCD_WIDTH ? u + 1 : u, v1 = v + 1 < LCD_HEIGHT ? v + 1 : v;
        const uint16_t *r0 = src + v * LCD_WIDTH, *r1 = src + v1 * LCD_WIDTH;
        const uint32_t top = mix(spread(r0[u]), spread(r0[u1]), wu), bot = mix(spread(r1[u]), spread(r1[u1]), wu);
        px = pack(mix(top, bot, wv));
      } else {
        if ((unsigned)u >= LCD_WIDTH) u = u < 0 ? 0 : LCD_WIDTH - 1;
        if ((unsigned)v >= LCD_HEIGHT) v = v < 0 ? 0 : LCD_HEIGHT - 1;
        px = src[v * LCD_WIDTH + u];
      }
      if (rim && !(gKnomiLiftOn && knomiLifted(u >> 1, v >> 1))) px = knomiRimPixel(px, rim[a + k]);
      o[k] = px;
    }
  }

  // RGB565 spread out (green above, red and blue below, 5 spare bits over each) so the three blend in one multiply
  static inline uint32_t spread(uint16_t c) { return (c | (uint32_t)c << 16) & 0x07E0F81Fu; }
  static inline uint16_t pack(uint32_t x) { return (uint16_t)(x | x >> 16); }
  static inline uint32_t mix(uint32_t a, uint32_t b, uint32_t w) { return ((a * (32 - w) + b * w) >> 5) & 0x07E0F81Fu; }

  Ws21Panel *panel;
};

// CST820 touch by registers, with the interface handleTouch() uses (the same as KnomiTouch on the Knomi). The CST820
// reads like the Knomi's CST816S: finger count at 0x02, then X and Y.
//
// The chip is read by a small task of its own on the other core, every 8 ms while a finger is on it (or its INT line
// says one came), and each press and lift is queued: the game reads them in order, so a quick tap is never lost while
// a frame is being drawn. Read only by the game's loop, a tap that came and went within one frame (on this board a
// frame took up to 200 ms) was never seen (Fedor: "the response is sluggish. Unplayable.", 2026-11-20).
extern volatile bool gTouchIrq;          // the sketch's: a touch to read
inline volatile bool gWs21TouchEdge = false;  // the INT line fell (the sketch's touchIsr())...
inline volatile uint32_t gWs21TouchEdgeMs = 0;  // ...at this millis(), the first time since the touch task looked
class KnomiTouch {
public:
  void setPins(int, int irq) { intPin = irq; }  // its reset is on the expander (EXIO2)
  bool begin(TwoWire &w, uint8_t a, int, int) {
    wire = &w;
    addr = a;
    reset();
    wire->beginTransmission(addr);
    bool ok = wire->endTransmission() == 0;
    if (!sampler) xTaskCreatePinnedToCore(samplerTask, "touch", 3072, this, 5, &sampler, 0);
    return ok;
  }
  void reset() {
    gExio.write(EXIO_TP_RST, false);
    delay(10);
    gExio.write(EXIO_TP_RST, true);
    delay(100);  // (50 was too soon for it to take its setup, perhaps: with only 0xFE written, taps went sluggish a
                 // few seconds after each start, as if it dozed off. The Knomi's CST816S waits 100 ms)
    // as the Knomi's: no gestures (with double taps on, the chip holds a tap back to see if a second one follows),
    // INT on touch, on each change and on release, no auto-reset, and no auto-sleep (Waveshare's demo: 0xFE only)
    writeReg(0xEC, 0x00);
    writeReg(0xFA, 0x60);
    writeReg(0xFB, 0x00);
    writeReg(0xFC, 0x00);
    writeReg(0xFE, 0x01);
    Serial.printf("touch: EC=%02X FA=%02X FB=%02X FC=%02X FE=%02X\n", readReg(0xEC), readReg(0xFA), readReg(0xFB),
                  readReg(0xFC), readReg(0xFE));
  }
  // the console's TOUCHLOG: each press and lift as the chip saw it, and when the game took it
  bool log = false;
  void setMaxCoordinates(int, int) {}  // fixed: the panel is 480
  void setMirrorXY(bool, bool) {}      // the rotation is handled in getPoint()

  // simulated touch from the serial console, in canvas coordinates
  void inject(int16_t x, int16_t y, uint32_t ms) { swipe(x, y, x, y, ms); }
  void swipe(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t ms) {
    ix = x0; iy = y0; jx = x1; jy = y1; iMs = ms ? ms : 1; iUntil = millis() + iMs; iOn = true;
  }

  // the oldest press or lift not read yet, or else the finger as it is now
  uint8_t getPoint(int16_t *x, int16_t *y, uint8_t) {
    if (iOn) {
      int32_t left = (int32_t)(iUntil - millis());
      if (left > 0) {
        *x = jx - (jx - ix) * left / (int32_t)iMs;
        *y = jy - (jy - iy) * left / (int32_t)iMs;
        return 1;
      }
      iOn = false;
      return 0;
    }
    if (!sampler) return readChip(x, y) ? 1 : 0;
    Ev e;
    portENTER_CRITICAL(&mux);
    if (qn) {
      e = q[qh];
      qh = (qh + 1) % QN;
      qn--;
    } else {
      e = { sx, sy, sDown, 0 };
    }
    portEXIT_CRITICAL(&mux);
    if (log && e.ms) Serial.printf("touch: %s read by the game %lu ms after the chip showed it\n", e.down ? "press" : "lift",
                                   (unsigned long)(millis() - e.ms));
    if (!e.down) return 0;
    *x = e.x;
    *y = e.y;
    return 1;
  }

private:
  struct Ev { int16_t x, y; bool down; uint32_t ms; };
  static constexpr int QN = 8;

  static void samplerTask(void *arg) {
    KnomiTouch *t = (KnomiTouch *)arg;
    for (;;) {
      vTaskDelay(pdMS_TO_TICKS(8));
      if (!gWs21TouchEdge && !gTouchIrq && !t->sDown && !(t->intPin >= 0 && digitalRead(t->intPin) == LOW)) continue;
      if (gWs21TouchEdge) t->edgeMs = gWs21TouchEdgeMs;
      gWs21TouchEdge = false;
      int16_t x = 0, y = 0;
      const bool down = t->readChip(&x, &y);
      bool changed;
      portENTER_CRITICAL(&t->mux);
      changed = down != t->sDown;
      if (changed) {
        if (t->qn >= QN - 1) {  // (full: the game hasn't read for a long while) the oldest press and lift go
          t->qh = (t->qh + 2) % QN;
          t->qn -= 2;
        }
        t->q[(t->qh + t->qn) % QN] = { x, y, down, millis() ? millis() : 1 };
        t->qn++;
      }
      t->sDown = down;
      if (down) t->sx = x, t->sy = y;
      portEXIT_CRITICAL(&t->mux);
      if (changed) gTouchIrq = true;  // (handleTouch() reads when told to, or while a finger is down)
      if (changed && t->log) {
        Serial.printf("touch: chip shows %s at %lu, %lu ms after its INT\n", down ? "press" : "lift",
                      (unsigned long)millis(), (unsigned long)(millis() - t->edgeMs));
      }
    }
  }

  // the chip now: a finger on it, and where on the canvas
  bool readChip(int16_t *x, int16_t *y) {
    wire->beginTransmission(addr);
    wire->write(0x02);
    if (wire->endTransmission(false) != 0 || wire->requestFrom(addr, (uint8_t)5) != 5) return false;
    uint8_t n = wire->read(), xh = wire->read(), xl = wire->read(), yh = wire->read(), yl = wire->read();
    if ((n & 0x0F) == 0) return false;
    int32_t px = (xh & 0x0F) << 8 | xl, py = (yh & 0x0F) << 8 | yl;  // panel pixels, 0..479
    int32_t lx, ly;
    rotTouch(gPanelRot, px, py, PANEL_SIZE, &lx, &ly);
    lx -= WS21_BORDER;  // into the canvas in the middle
    ly -= WS21_BORDER;
    if (gWs21RotDeg) ws21Unturn(lx, ly, &lx, &ly);  // turned back with the picture
    *x = lx < 0 ? 0 : lx >= LCD_WIDTH ? LCD_WIDTH - 1 : lx;
    *y = ly < 0 ? 0 : ly >= LCD_HEIGHT ? LCD_HEIGHT - 1 : ly;
    return true;
  }
  void writeReg(uint8_t r, uint8_t v) {
    wire->beginTransmission(addr);
    wire->write(r);
    wire->write(v);
    wire->endTransmission();
  }
  int readReg(uint8_t r) {
    wire->beginTransmission(addr);
    wire->write(r);
    if (wire->endTransmission(false) != 0 || wire->requestFrom(addr, (uint8_t)1) != 1) return -1;
    return wire->read();
  }
  TwoWire *wire = nullptr;
  uint8_t addr = TOUCH_I2C_ADDR;
  int intPin = -1;
  uint32_t edgeMs = 0;
  TaskHandle_t sampler = nullptr;
  portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
  Ev q[QN];
  uint8_t qh = 0, qn = 0;
  volatile bool sDown = false;
  int16_t sx = 0, sy = 0;
  int16_t ix = 0, iy = 0, jx = 0, jy = 0;
  uint32_t iUntil = 0, iMs = 1;
  bool iOn = false;
};

// QMI8658 accelerometer (SensorLib's driver), with KnomiAccel's interface: gravity in g along the panel's axes
// (+X right, +Y down, as autorot.h wants), for tilting and turning the screen.
// TODO(bring-up): the chip's axes against the panel's. Hold it upright and read the console's ACC, then set
// WS21_ACC_MAP in pin_config.h.
class KnomiAccel {
public:
  bool begin(TwoWire &w) {
    if (!imu.begin(w, IMU_I2C_ADDR, IIC_SDA, IIC_SCL)) return false;
    imu.configAccelerometer(SensorQMI8658::ACC_RANGE_2G, SensorQMI8658::ACC_ODR_62_5Hz, SensorQMI8658::LPF_MODE_0);
    ok = imu.enableAccelerometer();
    return ok;
  }
  bool ready() const { return ok; }
  // gravity in the picture's own axes (+X its right, +Y its down) as it's turned now: tilt in the games
  bool read(float g[3]) { return readAt(g, gWs21RotC / 65536.0f, gWs21RotS / 65536.0f); }
  // ...in the axes of the picture at WS21_ROT_DEG: which quarter turn from there is up (autoRotateTick())
  bool readBase(float g[3]) {
    static const float c = cosf(WS21_ROT_DEG * 0.017453293f), s = sinf(WS21_ROT_DEG * 0.017453293f);
    return readAt(g, c, s);
  }
  bool readRaw(float a[3]) { return ok && imu.getAccelerometer(a[0], a[1], a[2]); }

private:
  bool readAt(float g[3], float c, float s) {
    float a[3];
    if (!readRaw(a)) return false;
    WS21_ACC_MAP(a, g);  // the panel's axes (+X right, +Y down as the panel is)
    const float x = g[0], y = g[1];  // turned as the picture is on the panel
    g[0] = x * c + y * s;
    g[1] = y * c - x * s;
    return true;
  }
  SensorQMI8658 imu;
  bool ok = false;
};
