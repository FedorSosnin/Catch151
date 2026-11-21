#include "audio.h"
#include "savename.h"
#include "pin_config.h"

// The Waveshare 2.1 sounds through an amplifier added by hand (docs/WAVESHARE_21.md): a PAM8302A fed a 1-bit stream
// (WS21_AUDIO_PDM) or a MAX98357A fed I2S (WS21_AUDIO_I2S), in pin_config.h. It plays the same tones as the AMOLED
// 1.75, without a codec to set up. Without either, its buzzer beeps them.
#if defined(BOARD_KNOMI_V2) && !defined(BOARD_WS21)
// The Knomi V2 has no codec or speaker: the game calls the same and nothing sounds.
void audioBegin() {}
void sfxPlay(uint8_t) {}
void audioSetEnabled(bool) {}
bool audioEnabled() { return false; }
void audioSetSleeping(bool) {}
bool audioBusy() { return false; }
void audioSetIdle(bool) {}
#elif defined(BOARD_WS21) && !defined(WS21_AUDIO_I2S) && !defined(WS21_AUDIO_PDM)
// The Waveshare 2.1 without a speaker: its own buzzer (Fedor: "what about the onboard buzzer, can we use that for audio
// and clicks?"). It's an active one on the IO expander: on or off, one pitch, no volume. So a tap is a click (a 4 ms
// pulse), and every other effect beeps the rhythm of its notes, each note a beep. No tune can be played on it. It
// follows the Sound switch and is quiet while your Pokemon sleeps. It plays in its own task (each switch is an I2C
// write), so the game never waits on it.
#include <Arduino.h>
#include <Preferences.h>
#include "sfx_notes.h"
#include "ws21_exio.h"
static QueueHandle_t gQ = nullptr;
static volatile bool gOn = true, gSleeping = false, gPlaying = false;

static void buzz(uint16_t on, uint16_t off) {
  gExio.write(EXIO_BUZZER, true);
  vTaskDelay(pdMS_TO_TICKS(on));
  gExio.write(EXIO_BUZZER, false);
  if (off) vTaskDelay(pdMS_TO_TICKS(off));
}

static void buzzerTask(void *) {
  uint8_t id;
  for (;;) {
    if (!xQueueReceive(gQ, &id, portMAX_DELAY) || !gOn || gSleeping || !gExio.ready() || id >= SFX_COUNT) continue;
    gPlaying = true;
    if (id == SFX_TAP || id == SFX_FISH_REEL || id == SFX_FISH_NIBBLE) {
      buzz(4, 0);  // a click
    } else {
      const SfxDef &d = SFX[id];
      for (uint8_t i = 0; i < d.len; i++) {
        uint16_t ms = d.n[i].ms;
        if (!d.n[i].f) {  // a rest
          vTaskDelay(pdMS_TO_TICKS(ms));
          continue;
        }
        // a beep as long as the note, less a short gap so two notes in a row stay two (a third of the note, 45 ms at
        // most, sounded clipped: Fedor, "The buzzer almost cuts out at the end of tones")
        uint16_t on = ms > 33 ? ms - 25 : 8;
        buzz(on, ms - on < 25 ? 25 : ms - on);
      }
    }
    gPlaying = false;
  }
}

void audioBegin() {
  Preferences p;
  p.begin(SAVE_NS, true);
  gOn = p.getBool("snd", true);
  p.end();
  gQ = xQueueCreate(4, sizeof(uint8_t));
  // (above the half frame made on that core, ws21_hal.h: a beep ends on time)
  if (gQ) xTaskCreatePinnedToCore(buzzerTask, "buzzer", 3072, nullptr, 4, nullptr, 0);
}
void sfxPlay(uint8_t id) {
  if (gQ && gOn && !gSleeping) xQueueSend(gQ, &id, 0);  // dropped if four are waiting already
}
void audioSetEnabled(bool on) {
  gOn = on;
  Preferences p;
  p.begin(SAVE_NS, false);
  p.putBool("snd", on);
  p.end();
}
bool audioEnabled() { return gOn; }
void audioSetSleeping(bool sleeping) { gSleeping = sleeping; }
bool audioBusy() { return gPlaying; }
void audioSetIdle(bool) {}
#else
#include <Arduino.h>
#include <Wire.h>
#include <ESP_I2S.h>
#ifdef WS21_AMP_EXIO
#include "ws21_exio.h"  // the Waveshare 2.1's amplifier is turned on and off by an expander line
#endif
#include <Preferences.h>
#include "sfx_notes.h"

// ---------------------------------------------------------------------------
// Catch 151 audio: ES8311 codec (DAC -> PA amplifier -> speaker) over I2S.
// ES8311 init ported from Espressif's official driver (esp-bsp), fixed to
// MCLK=4.096MHz (256*fs), 16kHz, 16-bit, I2S slave. The effects are square
// tones (Game Boy style) synthesized in a separate task so as not to
// block the game loop.
// ---------------------------------------------------------------------------

#define ES8311_ADDR 0x18
#define SAMPLE_RATE 16000

static I2SClass i2s;
static bool gReady = false;
static bool gOn = true;
static bool gSleeping = false;
static bool gIdle = false;          // no recent interaction (see power.h): the amplifier draws power at idle
static uint32_t gAmpHoldUntil = 0;  // an effect requested at idle keeps the amplifier on until here
static bool gAmpHold = false;       // (separate flag: so the comparison doesn't fail when millis() wraps around)
static uint32_t gAmpOnAt = 0;       // when the amplifier was turned on
static QueueHandle_t gQ = nullptr;

// The NS4150B takes well over 8 ms to settle after each power-off, so
// turning it on right before each effect swallowed the short ones: the startup
// jingle (440 ms) was heard and the 35 ms beeps were not. It is left on while
// there is sound and the pet is awake, as in Waveshare's verified
// example, and only turned off when sleeping or muting.
static void updateAmplifierPower() {
  // gReady avoids turning it on on a board where the codec didn't start: there the
  // amplifier would only add hiss, because nothing is going to sound.
  if (gAmpHold && (int32_t)(gAmpHoldUntil - millis()) <= 0) gAmpHold = false;
  bool idleOff = gIdle && !gAmpHold;
  static int last = -1;  // the pin is only written if it changes
  int want = (gReady && gOn && !gSleeping && !idleOff) ? HIGH : LOW;
  if (want != last) {
    last = want;
    if (want == HIGH) gAmpOnAt = millis();
#ifdef WS21_AMP_EXIO
    gExio.write(WS21_AMP_EXIO, want == HIGH);
#else
    digitalWrite(PA, want);
#endif
  }
}

#if defined(WS21_AUDIO_I2S) || defined(WS21_AUDIO_PDM)
static bool es8311Init() { return true; }  // a MAX98357A or a PAM8302A takes it as it comes: no codec to set up
#else
// ---- codec I2C ----
static bool esW(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(ES8311_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}
static uint8_t esR(uint8_t reg) {
  Wire.beginTransmission(ES8311_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(ES8311_ADDR, 1);
  return Wire.available() ? Wire.read() : 0;
}

// INIT sequence VERIFIED ON THIS BOARD (PlaneRadar2.0 project, same
// Waveshare 1.75). Key: clock DERIVED FROM BCLK (reg01=0xBF, no external MCLK)
// and internal reference feeding the DAC (reg44=0x58); without those two the codec
// answered over I2C but no audio came out. 16kHz, 16-bit, I2S slave.
static bool es8311Init() {
  Wire.beginTransmission(ES8311_ADDR);
  if (Wire.endTransmission() != 0) return false;

  // open()
  esW(0x0D, 0xFA); esW(0x44, 0x08); esW(0x44, 0x08);  // power up + first-write quirk
  esW(0x01, 0x30); esW(0x02, 0x00); esW(0x03, 0x10); esW(0x16, 0x24);
  esW(0x04, 0x10); esW(0x05, 0x00); esW(0x0B, 0x00); esW(0x0C, 0x00);
  esW(0x10, 0x1F); esW(0x11, 0x7F);
  esW(0x00, 0x80); esW(0x00, 0x80);                   // reset clock, slave
  esW(0x01, 0xBF);                                    // clk src = BCLK (no external MCLK)
  { uint8_t r = esR(0x06); r &= ~0x20; esW(0x06, r); }  // SCLK not inverted
  esW(0x13, 0x10); esW(0x1B, 0x0A); esW(0x1C, 0x6A);
  esW(0x44, 0x58);                                    // internal reference -> feeds the DAC

  // config_sample(): BCLK*8 = DIG_MCLK
  esW(0x02, 0x18); esW(0x05, 0x00); esW(0x03, 0x10); esW(0x04, 0x20);
  { uint8_t r = esR(0x07); r &= 0xC0; esW(0x07, r); }
  esW(0x08, 0xFF);
  { uint8_t r = esR(0x06); r &= 0xE0; r |= 0x03; esW(0x06, r); }  // bclk_div=4

  // 16-bit I2S format
  esW(0x09, 0x0C); esW(0x0A, 0x0C);

  // start() DAC slave
  esW(0x00, 0x80); esW(0x01, 0xBF); esW(0x09, 0x0C); esW(0x0A, 0x0C);
  esW(0x17, 0xBF); esW(0x0E, 0x02); esW(0x12, 0x00); esW(0x14, 0x1A);
  esW(0x0D, 0x01); esW(0x15, 0x40); esW(0x37, 0x08); esW(0x45, 0x00);

  // volume + unmute
  esW(0x32, 0xBF);                                    // DAC volume ~0 dB
  { uint8_t r = esR(0x31); r &= 0x9F; esW(0x31, r); }  // unmute
  return true;
}
#endif  // WS21_AUDIO_I2S || WS21_AUDIO_PDM

// ---- square-wave tone synthesizer ----
static int16_t buf[256 * 2];  // interleaved stereo (L=R)

// V2: plays a tone (or silence if f==0), its frequency kept to a fraction of a sample (a 32-bit phase), so high notes
// stay in tune; a legato effect's notes (the tap) run into each other, the wave carried on, the ramps only where it
// starts and ends, and its last note fading to 55% (sfx_notes.h, sfxLegato())
static uint32_t gAcc = 0;
static void playTone(uint16_t f, uint16_t ms, bool legato, bool first, bool last) {
  int total = SAMPLE_RATE * ms / 1000;
  uint32_t step = f ? (uint32_t)(((uint64_t)f << 32) / SAMPLE_RATE) : 0;
  if (!legato || first) gAcc = 0;
  int done = 0;
  while (done < total) {
    int n = total - done; if (n > 256) n = 256;
    for (int i = 0; i < n; i++) {
      int16_t s = 0;
      if (step) {
        int idx = done + i;
        int32_t amp = 5000;
        if (legato && last) amp -= amp * 45 / 100 * idx / total;
        s = (int16_t)((gAcc & 0x80000000u) ? amp : -amp);
        if ((!legato || first) && idx < 64) s = (int16_t)(s * idx / 64);                     // attack
        else if ((!legato || last) && idx > total - 96) s = (int16_t)(s * (total - idx) / 96);  // decay
        gAcc += step;
      }
      buf[i * 2] = s; buf[i * 2 + 1] = s;
    }
    i2s.write((uint8_t *)buf, n * 4);
    done += n;
  }
}

static void audioTask(void *) {
  uint8_t id;
  for (;;) {
    if (xQueueReceive(gQ, &id, portMAX_DELAY) && gOn && !gSleeping && gReady &&
        id < SFX_COUNT) {
      const SfxDef &d = SFX[id];
      // the NS4150B takes more than 8 ms to settle: if it was just turned on, the effect waits
      int32_t since = (int32_t)(millis() - gAmpOnAt);
      if (since >= 0 && since < 50) vTaskDelay(pdMS_TO_TICKS(50 - since));
      for (uint8_t i = 0; i < d.len; i++) playTone(d.n[i].f, d.n[i].ms, sfxLegato(id), i == 0, i + 1 == d.len);
    }
  }
}

void audioBegin() {
  // I2S first: starts the MCLK the codec needs to lock on
#ifdef WS21_AMP_EXIO
  gExio.write(WS21_AMP_EXIO, false);  // until we know whether sound is enabled
#else
  pinMode(PA, OUTPUT);
  digitalWrite(PA, LOW);   // until we know whether sound is enabled
#endif

#if defined(WS21_AUDIO_PDM)
  // the sound as a 1-bit stream on one pin, smoothed by a resistor and a capacitor into the PAM8302A: no clock line
  // (-1), and both channels, as the effects are written, so each half of the stream carries the same sample
  i2s.setPinsPdmTx(-1, PDM_DO_IO);
  if (!i2s.begin(I2S_MODE_PDM_TX, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO)) {
#else
#ifdef WS21_AUDIO_I2S
  i2s.setPins(I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO);  // (no MCLK and no input: the MAX98357A needs neither)
#else
  i2s.setPins(I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO, I2S_DI_IO, I2S_MCK_IO);
#endif
  if (!i2s.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT,
                 I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
#endif
    Serial.println("I2S begin failed");
    return;
  }
  if (!es8311Init()) { Serial.println("ES8311 not responding (audio off)"); return; }

  Preferences p;
  p.begin(SAVE_NS, true);
  gOn = p.getBool("snd", true);
  p.end();

  gReady = true;
  updateAmplifierPower();
  gQ = xQueueCreate(8, sizeof(uint8_t));
  xTaskCreatePinnedToCore(audioTask, "audio", 4096, nullptr, 1, nullptr, 0);
  sfxPlay(SFX_HATCH);  // startup jingle (confirms it sounds)
}

bool audioBusy() { return gAmpHold && (int32_t)(gAmpHoldUntil - millis()) > 0; }

void sfxPlay(uint8_t id) {
  if (gReady && gOn && gQ) {
    if (gIdle) {  // an alert at idle (e.g. "ready to evolve") has to be heard
      gAmpHoldUntil = millis() + 4000;
      gAmpHold = true;
      updateAmplifierPower();
    }
    xQueueSend(gQ, &id, 0);  // drops if the queue is full
  }
}

void audioSetEnabled(bool on) {
  gOn = on;
  updateAmplifierPower();
  Preferences p;
  p.begin(SAVE_NS, false);
  p.putBool("snd", on);
  p.end();
}
bool audioEnabled() { return gOn; }

void audioSetSleeping(bool sleeping) {
  if (gSleeping == sleeping) return;
  gSleeping = sleeping;
  updateAmplifierPower();  // asleep there are no effects: amp off, no power draw
}

void audioSetIdle(bool idle) {
  gIdle = idle;
  updateAmplifierPower();  // also when an alert's hold expires
}

#endif  // BOARD_KNOMI_V2
