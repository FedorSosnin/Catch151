#pragma once
// Waveshare ESP32-S3-Touch-LCD-2.1 (480x480 round IPS): the classes knomi_hal.h gives the game on the Knomi, for this
// board. To the game it is a Knomi (BOARD_KNOMI_V2 is also set): the canvas stays 466x466 and touches come in canvas
// coordinates. docs/WAVESHARE_21.md has the board, the pins and the bring-up plan.
//
// UNTESTED: written before the boards arrived. It compiles; nothing here has driven a real panel yet. Each guess is
// marked TODO(bring-up).
#include <Arduino.h>
#include <Wire.h>
#include "Arduino_GFX_Library.h"
#include "pin_config.h"
#include "autorot.h"
#include <esp_heap_caps.h>
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

// The RGB bus: 16 data lines (RGB565), with the timings and the bounce buffer from Waveshare's demo (pin_config.h).
// The bounce buffer copies each band of lines from PSRAM to internal RAM before it goes out, so the game's own PSRAM
// traffic is less likely to make the picture drift.
static Arduino_ESP32RGBPanel *ws21RgbPanel() {
  return new Arduino_ESP32RGBPanel(
    LCD_DE, LCD_VSYNC, LCD_HSYNC, LCD_PCLK,
    LCD_R0, LCD_R1, LCD_R2, LCD_R3, LCD_R4,
    LCD_G0, LCD_G1, LCD_G2, LCD_G3, LCD_G4, LCD_G5,
    LCD_B0, LCD_B1, LCD_B2, LCD_B3, LCD_B4,
    LCD_HSYNC_POL, LCD_HSYNC_FP, LCD_HSYNC_PW, LCD_HSYNC_BP,
    LCD_VSYNC_POL, LCD_VSYNC_FP, LCD_VSYNC_PW, LCD_VSYNC_BP,
    LCD_PCLK_NEG, LCD_PCLK_HZ, false /*little endian*/, 0 /*DE idle low*/, 0 /*PCLK idle low*/, LCD_BOUNCE_PX);
}

// The panel, with the backlight the game sets (0 off .. 255 full) as PWM on LCD_BL (a MOSFET: high = on)
class Ws21Panel : public Arduino_RGB_Display {
public:
  using Arduino_RGB_Display::Arduino_RGB_Display;
  void setBrightness(uint8_t v) {
    if (!blReady) blReady = ledcAttach(LCD_BL, LCD_BL_PWM_HZ, 8);
    if (blReady) ledcWrite(LCD_BL, v);  // TODO(bring-up): no flicker at low levels (the IDF demo uses 5 kHz)
  }
private:
  bool blReady = false;
};

void pressOverlay();  // the sketch's press preview, drawn over every frame

// The 466x466 canvas in PSRAM. flush() copies it into the panel's frame buffer (also PSRAM, which the RGB peripheral
// reads out by itself all the time), in the middle, with the current rotation and the Knomi's soft rim
// (knomi_rim.h) made for the canvas's own circle.
// TODO(bring-up): this writes into the frame buffer while it is being shown. If that tears, use two frame buffers
// (esp_lcd's num_fbs = 2, swapped on vsync), as Waveshare's Arduino demo does.
class Ws21Canvas : public Arduino_Canvas {
public:
  Ws21Canvas(Ws21Panel *out) : Arduino_Canvas(LCD_WIDTH, LCD_HEIGHT, out), panel(out) {}

  // the RGB panel keeps its own pixel clock (LCD_PCLK_HZ): the speed the sketch asks for is the Knomi's SPI's
  bool begin(int32_t = GFX_NOT_DEFINED) override {
    if (!Arduino_Canvas::begin(GFX_NOT_DEFINED)) return false;
    fb = panel->getFramebuffer();
    if (!fb) return false;
    memset(fb, 0, (size_t)PANEL_SIZE * PANEL_SIZE * 2);  // the border is black, and stays so
    panel->flush(true);
    knomiRimBegin(LCD_WIDTH);
    return true;
  }

  void flush(bool = false) override {
    pressOverlay();
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
    panel->flush(true);  // out of the CPU's cache into PSRAM, where the panel reads it
  }

  uint16_t *panelBuffer() { return fb; }  // the console's SHOT: the whole 480x480 panel

private:
  // near the edge: darkened by the rim, unless a need bar's label is over the shadow there (its 2 x 2 cell)
  static inline uint16_t edge(uint16_t c, const uint8_t *rim, int x, int y) {
    return gKnomiLiftOn && knomiLifted(x >> 1, y >> 1) ? c : knomiRimPixel(c, rim[x]);
  }
  Ws21Panel *panel;
  uint16_t *fb = nullptr;
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
