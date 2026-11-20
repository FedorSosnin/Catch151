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
#include <esp_timer.h>
#include <esp_async_memcpy.h>
#include <Preferences.h>
#include "soc/lcd_cam_struct.h"
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

// The panel: the ST7701S's setup over the 3-wire bus, then the ESP32-S3's RGB peripheral, reading out two frame
// buffers in PSRAM in turn. The game's next frame goes into the one not on show, which takes over at the start of
// the next refresh: nothing is ever shown half drawn (with one buffer it tore). Arduino_GFX's RGB panel keeps one
// buffer and can't be told otherwise, so this talks to esp_lcd itself. The timings are Waveshare's (pin_config.h).
// The bounce buffers copy each band of lines from PSRAM to internal RAM before it goes out, so the game's own PSRAM
// traffic is less likely to make the picture drift. The backlight is PWM on LCD_BL (a MOSFET: high = on).
class Ws21Panel : public Arduino_G {
public:
  explicit Ws21Panel(Ws21InitBus *b) : Arduino_G(PANEL_SIZE, PANEL_SIZE), bus(b) {}

  bool begin(int32_t = GFX_NOT_DEFINED) override {
    if (handle) return true;
    if (!bus->begin()) return false;
    bus->sendCommand(0x01);  // a software reset after the expander's, as Arduino_GFX's RGB display does
    delay(120);
    bus->batchOperation(WS21_INIT_OPS, sizeof(WS21_INIT_OPS));
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
    if (!handle) return false;
    void *f0 = nullptr, *f1 = nullptr;
    esp_lcd_rgb_panel_get_frame_buffer(handle, 2, &f0, &f1);
    fbs[0] = (uint16_t *)f0;
    fbs[1] = (uint16_t *)f1;
    if (!fbs[0] || !fbs[1]) return false;
    for (uint16_t *f : fbs) {  // the border is black, and stays so
      memset(f, 0, FB_BYTES);
      esp_cache_msync(f, FB_BYTES, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    }
    return true;
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
    c.num_fbs = 2;
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
    c.flags.fb_in_psram = 1;
    // after each band is copied out, its cache lines are dropped: the refill (27 MB/s) would otherwise push the game's
    // own data out of the 32 KB cache all the time. (The frame buffers are written by DMA, past the cache: flush())
    c.flags.bb_invalidate_cache = 1;
    // Made on the game's core (unless REFILLCORE says otherwise), so its refill interrupt runs there too: then the
    // refill (each band copied out of PSRAM) and the game's own heavy PSRAM work never run at the same moment. With
    // the refill on the other core and flush() shared by both, they did, the refill fell behind and the streaks came
    // back (2026-11-20, twice: the second time with flush() copying by DMA and leaving the cache clean).
    esp_lcd_panel_handle_t h = nullptr;
    if (esp_lcd_new_rgb_panel(&c, &h) != ESP_OK) return;
    esp_lcd_rgb_panel_event_callbacks_t cb = {};
    cb.on_frame_buf_complete = frameDone;
    esp_lcd_rgb_panel_register_event_callbacks(h, &cb, this);
    esp_lcd_panel_reset(h);
    esp_lcd_panel_init(h);
    handle = h;
  }

  // The buffer to draw the next frame into: the one not on show. Right after a swap the old one may still be read out
  // until the refresh that was using it ends, so it waits for that (a frame is 17 ms; the game rarely asks that soon).
  uint16_t *backBuffer() {
    if (!fbs[0]) return nullptr;
    if (pending) {
      uint32_t t0 = millis();
      while (frames == askedAt && millis() - t0 < 50) vTaskDelay(1);
      pending = false;
    }
    return fbs[!shown];
  }
  // shows fb from the next refresh on (no copy: esp_lcd switches buffers when given one of its own). Whatever wrote
  // it went past the cache (DMA: Ws21Canvas::flush()), so the cache must hold none of its old lines
  void present(uint16_t *fb) {
    esp_cache_msync(fb, FB_BYTES, ESP_CACHE_MSYNC_FLAG_DIR_M2C);
    esp_lcd_panel_draw_bitmap(handle, 0, 0, PANEL_SIZE, PANEL_SIZE, fb);
    askedAt = frames;  // (read after the switch was asked for: the next refresh to start reads the new one)
    shown = fb == fbs[1];
    pending = true;
  }
  uint16_t *frontBuffer() const { return fbs[shown]; }
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
  static constexpr size_t FB_BYTES = (size_t)PANEL_SIZE * PANEL_SIZE * 2;

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
  Ws21InitBus *bus;
  esp_lcd_panel_handle_t handle = nullptr;
  uint16_t *fbs[2] = { nullptr, nullptr };
  bool shown = 0;  // which one is on show
  bool pending = false;
  volatile uint32_t frames = 0;
  uint32_t askedAt = 0;
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

// The 466x466 canvas in PSRAM. flush() makes the panel's rows from it a band at a time in internal RAM: in the middle,
// turned (gWs21RotDeg) and with the Knomi's soft rim (knomi_rim.h). Each band goes out to the frame buffer not on show
// by DMA while the next one is made, then that buffer is shown. All on the game's core, where the panel's refill also
// runs. The console's SPLIT 1 has a task on the other core make every other band (each core with its own DMA): 135 ms
// a frame turned and blended instead of 160, but the glitch lines came back (Fedor, 2026-11-20). Two cores reading
// PSRAM at once hold up the refill. So it stays off.
//
// Why DMA (bring-up, 2026-11-20): the CPU writing the frame buffer through the cache read every line of it in first
// and wrote it back after, and then each line had to be flushed: 68 ms a frame just to copy, 138 ms turned. The DMA
// writes PSRAM straight, while the CPU goes on to the next band, and leaves nothing dirty in the cache for the
// panel's refill to wait on. Turned, each pixel is still about 50 instructions and a wait on PSRAM: 84 ms a frame on
// one core.
inline bool gWs21Split = false;
class Ws21Canvas : public Arduino_Canvas {
public:
  Ws21Canvas(Ws21Panel *out) : Arduino_Canvas(LCD_WIDTH, LCD_HEIGHT, out), panel(out) {}

  // the RGB panel keeps its own pixel clock (LCD_PCLK_HZ): the speed the sketch asks for is the Knomi's SPI's
  bool begin(int32_t = GFX_NOT_DEFINED) override {
    if (!Arduino_Canvas::begin(GFX_NOT_DEFINED)) return false;
    if (!panel->frontBuffer()) return false;
    knomiRimBegin(LCD_WIDTH);
    ws21SetRotation(WS21_ROT_DEG);
    if (!allocBands(0)) return false;
    for (Side &d : side) {
      async_memcpy_config_t mc = ASYNC_MEMCPY_DEFAULT_CONFIG();
      mc.backlog = 4;
      mc.dma_burst_size = 32;  // (the frame buffers start on 32-byte lines; 64-byte bursts want 64-byte starts)
      if (esp_async_memcpy_install_gdma_ahb(&mc, &d.dma) != ESP_OK) {
        d.dma = nullptr;
        Serial.println("ws21: no DMA for the frame copy, the CPU copies it");
      }
    }
    go = xSemaphoreCreateBinary();
    back = xSemaphoreCreateBinary();
    // below the touch task (5) and the buzzer (4), so the half frame made there never holds up a tap or a beep
    if (go && back) xTaskCreatePinnedToCore(workerTask, "ws21half", 3072, this, 3, &worker, 0);
    return true;
  }

  void flush(bool = false) override {
    const int64_t t0 = esp_timer_get_time();
    pressOverlay();
    uint16_t *fb = panel->backBuffer();
    if (!fb) return;
    const int64_t t1 = esp_timer_get_time();
    const bool split = gWs21Split && worker && allocBands(1);
    curFb = fb;
    if (split) xSemaphoreGive(go);
    int64_t waited = doBands(0, fb, split ? 2 : 1, 0);
    const int64_t t2 = esp_timer_get_time();
    if (split && xSemaphoreTake(back, pdMS_TO_TICKS(300)) != pdTRUE) {
      Serial.println("ws21: the other core's half of the frame didn't come back; one core from now on");
      gWs21Split = false;
    }
    const int64_t t3 = esp_timer_get_time();
    panel->present(fb);
    const int64_t t4 = esp_timer_get_time();
    prof[0] += t1 - t0;          // the press overlay and waiting for the buffer
    prof[1] += t2 - t1 - waited;  // making this core's rows
    prof[2] += waited + t3 - t2;  // waiting for the DMA and the other core
    prof[3] += t4 - t3;          // the cache dropped, the buffer handed over
    profN++;
  }

  uint16_t *panelBuffer() { return panel->frontBuffer(); }  // the console's SHOT: the whole 480x480 panel

  // the console's FPS: flush()'s parts since the last call, in ms per frame
  void profPrint() {
    const float n = profN ? profN : 1;
    Serial.printf("flush n=%u wait=%.1f rows=%.1f dma+other=%.1f present=%.1f total=%.1f ms dma=%s split=%d refill=%d\n",
                  (unsigned)profN, prof[0] / 1000.f / n, prof[1] / 1000.f / n, prof[2] / 1000.f / n,
                  prof[3] / 1000.f / n, (prof[0] + prof[1] + prof[2] + prof[3]) / 1000.f / n,
                  side[0].dma ? "on" : "off", gWs21Split && worker ? 1 : 0, panel->refillCore);
    memset(prof, 0, sizeof prof);
    profN = 0;
  }

private:
  static constexpr int BAND = 16;  // (8 was 9 ms a frame slower: more of the canvas read twice, band to band)
  static constexpr size_t BAND_BYTES = (size_t)BAND * PANEL_SIZE * 2;  // (a multiple of 64, as the DMA to PSRAM wants)
  // each core's own: its DMA and the counts of its copies
  struct Side {
    async_memcpy_handle_t dma = nullptr;
    volatile uint32_t done = 0;    // DMA copies finished (its interrupt counts them)
    uint32_t issued = 0;           // ...and asked for
    uint32_t ticket[2] = { 0, 0 };  // the count each of its two band buffers' last copy finishes at
  };
  Side side[2];
  uint16_t *bands[4] = { nullptr, nullptr, nullptr, nullptr };  // internal RAM: two for each side
  // side k's two band buffers, made the first time they're needed (the other core's only with SPLIT): whole panel
  // rows, so the 7 px of border either side stay black
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
  int64_t prof[4] = { 0, 0, 0, 0 };
  uint32_t profN = 0;

  static void workerTask(void *arg) {
    Ws21Canvas *c = (Ws21Canvas *)arg;
    for (;;) {
      if (xSemaphoreTake(c->go, portMAX_DELAY) != pdTRUE) continue;
      c->doBands(1, c->curFb, 2, 1);
      xSemaphoreGive(c->back);
    }
  }

  // bands first, first + step, ... of the frame into fb, by side k (0: the game's core, 1: the other); returns the
  // time spent waiting for its DMA, in us. All its copies are done when it returns
  int64_t doBands(int k, uint16_t *fb, int step, int first) {
    Side &d = side[k];
    int64_t waited = 0;
    int j = 0;
    for (int i = first; i * BAND < LCD_HEIGHT; i += step, j++) {
      const int y0 = i * BAND, n = LCD_HEIGHT - y0 < BAND ? LCD_HEIGHT - y0 : BAND;
      uint16_t *buf = bands[2 * k + (j & 1)];
      waited += waitDma(d, d.ticket[j & 1]);  // the band before last, from this buffer, is out
      makeBand(buf + WS21_BORDER, y0, n);
      d.ticket[j & 1] = send(d, fb + (size_t)(WS21_BORDER + y0) * PANEL_SIZE, buf, (size_t)n * PANEL_SIZE * 2);
    }
    return waited + waitDma(d, d.issued);
  }

  // a band out to the frame buffer: by DMA, or by the CPU if that's refused (then written back from the cache at
  // once, since present() drops the cache's copy). Returns the count to wait for.
  uint32_t send(Side &d, uint16_t *to, uint16_t *from, size_t bytes) {
    if (d.dma) {
      if (esp_async_memcpy(d.dma, to, from, bytes, ws21DmaDone, (void *)&d.done) == ESP_OK) return ++d.issued;
      Serial.printf("ws21: DMA copy refused (%p, %u bytes), the CPU copies from now on\n", to, (unsigned)bytes);
      waitDma(d, d.issued);
      d.dma = nullptr;
    }
    memcpy(to, from, bytes);
    esp_cache_msync(to, bytes, ESP_CACHE_MSYNC_FLAG_DIR_C2M | ESP_CACHE_MSYNC_FLAG_UNALIGNED);
    return d.issued;
  }
  // waits for the count t (the DMA does a band in well under the time a band takes to make: it hardly ever waits)
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

  // canvas rows y0..y0+n-1 into out (each 466 px, the row's part in the panel's middle; rows PANEL_SIZE apart)
  void makeBand(uint16_t *out, int y0, int n) {
    for (int r = 0; r < n; r++) {  // the black outside the circle; unturned, the whole row
      const int y = y0 + r;
      uint16_t *o = out + r * PANEL_SIZE;
      const uint8_t *rim = gKnomiRim ? gKnomiRim + y * LCD_WIDTH : nullptr;
      const int x0 = rim ? gKnomiRimX0[y] : 0, x1 = rim ? gKnomiRimX1[y] : LCD_WIDTH;
      if (x0 > 0) memset(o, 0, x0 * 2);
      if (x1 < LCD_WIDTH) memset(o + x1, 0, (LCD_WIDTH - x1) * 2);
      if (gWs21RotDeg) continue;
      const int i0 = rim ? gKnomiRimI0[y] : 0, i1 = rim ? gKnomiRimI1[y] : LCD_WIDTH;
      const uint16_t *src = _framebuffer + y * LCD_WIDTH;
      for (int x = x0; x < i0; x++) o[x] = edge(src[x], rim, x, y);
      memcpy(o + i0, src + i0, (size_t)(i1 - i0) * 2);
      for (int x = i1; x < x1; x++) o[x] = edge(src[x], rim, x, y);
    }
    if (!gWs21RotDeg) return;
    // Turned: each panel pixel takes the canvas point turned back onto it, blended from its four pixels (smooth) or
    // the nearest one. The rim is round, so its darkening is the panel pixel's own; whether a need bar's label lifts
    // it is the canvas pixel's. In tiles 32 wide, so the canvas lines a tile reads (a small turned square) stay in
    // the cache from one of its rows to the next. Each tile row in up to three runs: the rim, the middle (no
    // darkening, and nothing near the canvas's edge, so nothing to check per pixel), the rim.
    const uint8_t sm = gWs21Smooth;
    for (int tx = 0; tx < LCD_WIDTH; tx += TILE) {
      const int te = tx + TILE < LCD_WIDTH ? tx + TILE : LCD_WIDTH;
      for (int r = 0; r < n; r++) {
        const int y = y0 + r;
        uint16_t *o = out + r * PANEL_SIZE;
        const uint8_t *rim = gKnomiRim ? gKnomiRim + y * LCD_WIDTH : nullptr;
        const int x0 = rim ? gKnomiRimX0[y] : 0, x1 = rim ? gKnomiRimX1[y] : LCD_WIDTH;
        const int i0 = rim ? gKnomiRimI0[y] : 0, i1 = rim ? gKnomiRimI1[y] : LCD_WIDTH;
        turnPart(o, x0 > tx ? x0 : tx, i0 < te ? i0 : te, rim, y, sm);
        turnPart(o, i0 > tx ? i0 : tx, i1 < te ? i1 : te, nullptr, y, sm);
        turnPart(o, i1 > tx ? i1 : tx, x1 < te ? x1 : te, rim, y, sm);
      }
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

  static inline uint16_t edge(uint16_t c, const uint8_t *rim, int x, int y) {
    return gKnomiLiftOn && knomiLifted(x >> 1, y >> 1) ? c : knomiRimPixel(c, rim[x]);
  }
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
