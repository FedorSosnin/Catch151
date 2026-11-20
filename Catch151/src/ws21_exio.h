#pragma once
// Waveshare ESP32-S3-Touch-LCD-2.1: the TCA9554 IO expander (I2C 0x20). Its eight lines: the panel's reset and setup
// CS, the touch's reset, the microSD's D3 (the amplifier's on/off line here: docs/WAVESHARE_21.md), the IMU's and
// clock's interrupts, and the buzzer. All eight are outputs, as in Waveshare's demo; every write goes through here,
// which keeps the one copy of them. Its own interrupt line isn't wired to the ESP32.
// The I2C bus must already be started (Wire.begin in setup()); call only from the game's loop.
#include <Arduino.h>
#include <Wire.h>
#include "pin_config.h"

class Ws21Exio {
public:
  bool begin() {
    out = 0xFF & ~(1 << EXIO_BUZZER);  // all high (resets released, CS idle), the buzzer off
    ok = reg(0x01, out) && reg(0x03, 0x00);  // the output values first, then all eight as outputs
    return ok;
  }
  void write(uint8_t bit, bool high) {
    uint8_t v = high ? out | (1 << bit) : out & ~(1 << bit);
    if (v == out) return;
    out = v;
    if (ok) reg(0x01, out);
  }
  bool ready() const { return ok; }
private:
  static bool reg(uint8_t r, uint8_t v) {
    Wire.beginTransmission(EXIO_ADDR);
    Wire.write(r);
    Wire.write(v);
    return Wire.endTransmission() == 0;
  }
  uint8_t out = 0xFF;
  bool ok = false;
};
inline Ws21Exio gExio;
