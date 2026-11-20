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
#include <SensorQMI8658.hpp>
#include "ws21_exio.h"  // the IO expander: the panel's reset and CS, the touch's reset
#define KNOMI_RIM_ROWS LCD_HEIGHT  // the rim is made for the 466 canvas, 1:1 (below)
#include "knomi_rim.h"

// The canvas sits in the middle of the panel, pixel for pixel: 7 px of black all round. The 2 px grid the V2 UI
// draws on stays sharp, and nothing is scaled (docs/WAVESHARE_21.md, "The screen").
#define WS21_BORDER ((PANEL_SIZE - LCD_WIDTH) / 2)

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
    if (esp_lcd_new_rgb_panel(&c, &handle) != ESP_OK) {
      handle = nullptr;
      return false;
    }
    esp_lcd_rgb_panel_event_callbacks_t cb = {};
    cb.on_frame_buf_complete = frameDone;
    esp_lcd_rgb_panel_register_event_callbacks(handle, &cb, this);
    esp_lcd_panel_reset(handle);
    esp_lcd_panel_init(handle);
    void *f0 = nullptr, *f1 = nullptr;
    esp_lcd_rgb_panel_get_frame_buffer(handle, 2, &f0, &f1);
    fbs[0] = (uint16_t *)f0;
    fbs[1] = (uint16_t *)f1;
    if (!fbs[0] || !fbs[1]) return false;
    for (uint16_t *f : fbs) memset(f, 0, (size_t)PANEL_SIZE * PANEL_SIZE * 2);  // the border is black, and stays so
    return true;
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
  // shows fb from the next refresh on (no copy: esp_lcd switches buffers when given one of its own)
  void present(uint16_t *fb) {
    esp_lcd_panel_draw_bitmap(handle, 0, 0, PANEL_SIZE, PANEL_SIZE, fb);
    askedAt = frames;  // (read after the switch was asked for: the next refresh to start reads the new one)
    shown = fb == fbs[1];
    pending = true;
  }
  uint16_t *frontBuffer() const { return fbs[shown]; }

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
  static bool IRAM_ATTR frameDone(esp_lcd_panel_handle_t, const esp_lcd_rgb_panel_event_data_t *, void *ctx) {
    ((Ws21Panel *)ctx)->frames++;
    return false;
  }
  Ws21InitBus *bus;
  esp_lcd_panel_handle_t handle = nullptr;
  uint16_t *fbs[2] = { nullptr, nullptr };
  bool shown = 0;  // which one is on show
  bool pending = false;
  volatile uint32_t frames = 0;
  uint32_t askedAt = 0;
  bool blReady = false;
};

void pressOverlay();  // the sketch's press preview, drawn over every frame

// The 466x466 canvas in PSRAM. flush() copies it into the panel's frame buffer not on show (also PSRAM), in the
// middle, with the current rotation and the Knomi's soft rim (knomi_rim.h) made for the canvas's own circle, then
// shows it.
class Ws21Canvas : public Arduino_Canvas {
public:
  Ws21Canvas(Ws21Panel *out) : Arduino_Canvas(LCD_WIDTH, LCD_HEIGHT, out), panel(out) {}

  // the RGB panel keeps its own pixel clock (LCD_PCLK_HZ): the speed the sketch asks for is the Knomi's SPI's
  bool begin(int32_t = GFX_NOT_DEFINED) override {
    if (!Arduino_Canvas::begin(GFX_NOT_DEFINED)) return false;
    if (!panel->frontBuffer()) return false;
    knomiRimBegin(LCD_WIDTH);
    return true;
  }

  void flush(bool = false) override {
    pressOverlay();
    uint16_t *fb = panel->backBuffer();
    if (!fb) return;
    const RotBlit r = rotBlit(gPanelRot, PANEL_SIZE);
    const int dx = r.dx;
    for (int y = 0; y < LCD_HEIGHT; y++) {
      const uint16_t *src = _framebuffer + y * LCD_WIDTH;
      uint16_t *out = fb + r.base + (y + WS21_BORDER) * r.dy + WS21_BORDER * dx;
      const uint8_t *rim = gKnomiRim ? gKnomiRim + y * LCD_WIDTH : nullptr;
      const int x0 = rim ? gKnomiRimX0[y] : 0, x1 = rim ? gKnomiRimX1[y] : LCD_WIDTH;
      const int i0 = rim ? gKnomiRimI0[y] : 0, i1 = rim ? gKnomiRimI1[y] : LCD_WIDTH;
      int x = 0;
      for (; x < x0; x++) out[x * dx] = 0;
      for (; x < i0; x++) out[x * dx] = edge(src[x], rim, x, y);
      if (dx == 1) {  // rotation 0: the middle of the row in one go
        memcpy(out + x, src + x, (size_t)(i1 - x) * 2);
        x = i1;
      } else {
        for (; x < i1; x++) out[x * dx] = src[x];
      }
      for (; x < x1; x++) out[x * dx] = edge(src[x], rim, x, y);
      for (; x < LCD_WIDTH; x++) out[x * dx] = 0;
    }
    // (no cache write-back: the bounce buffers are filled by the CPU, through the same cache)
    panel->present(fb);
  }

  uint16_t *panelBuffer() { return panel->frontBuffer(); }  // the console's SHOT: the whole 480x480 panel

private:
  // near the edge: darkened by the rim, unless a need bar's label is over the shadow there (its 2 x 2 cell)
  static inline uint16_t edge(uint16_t c, const uint8_t *rim, int x, int y) {
    return gKnomiLiftOn && knomiLifted(x >> 1, y >> 1) ? c : knomiRimPixel(c, rim[x]);
  }
  Ws21Panel *panel;
};

// CST820 touch by registers, with the interface handleTouch() uses (the same as KnomiTouch on the Knomi). The CST820
// reads like the Knomi's CST816S: finger count at 0x02, then X and Y.
// TODO(bring-up): Waveshare's driver reads 0..479 on both axes with no swap or mirror; check that against the panel.
class KnomiTouch {
public:
  void setPins(int, int irq) { (void)irq; }  // its reset is on the expander (EXIO2)
  bool begin(TwoWire &w, uint8_t a, int, int) {
    wire = &w;
    addr = a;
    reset();
    wire->beginTransmission(addr);
    return wire->endTransmission() == 0;
  }
  void reset() {
    gExio.write(EXIO_TP_RST, false);
    delay(10);
    gExio.write(EXIO_TP_RST, true);
    delay(50);
    // auto-sleep off, so the chip always answers over I2C (as Waveshare's demo: 0xFE, DisAutoSleep). TODO(bring-up):
    // the Knomi's CST816S also sets no gestures and INT on touch and release (0xEC, 0xFA): try those if taps are missed
    writeReg(0xFE, 0x01);
  }
  void setMaxCoordinates(int, int) {}  // fixed: the panel is 480
  void setMirrorXY(bool, bool) {}      // the rotation is handled in getPoint()

  // simulated touch from the serial console, in canvas coordinates
  void inject(int16_t x, int16_t y, uint32_t ms) { swipe(x, y, x, y, ms); }
  void swipe(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint32_t ms) {
    ix = x0; iy = y0; jx = x1; jy = y1; iMs = ms ? ms : 1; iUntil = millis() + iMs; iOn = true;
  }

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
    wire->beginTransmission(addr);
    wire->write(0x02);
    if (wire->endTransmission(false) != 0 || wire->requestFrom(addr, (uint8_t)5) != 5) return 0;
    uint8_t n = wire->read(), xh = wire->read(), xl = wire->read(), yh = wire->read(), yl = wire->read();
    if ((n & 0x0F) == 0) return 0;
    int32_t px = (xh & 0x0F) << 8 | xl, py = (yh & 0x0F) << 8 | yl;  // panel pixels, 0..479
    int32_t lx, ly;
    rotTouch(gPanelRot, px, py, PANEL_SIZE, &lx, &ly);
    lx -= WS21_BORDER;  // into the canvas in the middle
    ly -= WS21_BORDER;
    *x = lx < 0 ? 0 : lx >= LCD_WIDTH ? LCD_WIDTH - 1 : lx;
    *y = ly < 0 ? 0 : ly >= LCD_HEIGHT ? LCD_HEIGHT - 1 : ly;
    return 1;
  }

private:
  void writeReg(uint8_t r, uint8_t v) {
    wire->beginTransmission(addr);
    wire->write(r);
    wire->write(v);
    wire->endTransmission();
  }
  TwoWire *wire = nullptr;
  uint8_t addr = TOUCH_I2C_ADDR;
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
  bool read(float g[3]) {
    if (!ok) return false;
    float a[3];
    if (!imu.getAccelerometer(a[0], a[1], a[2])) return false;
    WS21_ACC_MAP(a, g);
    return true;
  }
private:
  SensorQMI8658 imu;
  bool ok = false;
};
