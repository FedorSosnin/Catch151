#include "audio.h"
#include "savename.h"
#include "pin_config.h"

// The Waveshare 2.1 sounds through an amplifier added by hand (docs/WAVESHARE_21.md): a PAM8302A fed a 1-bit stream
// (WS21_AUDIO_PDM) or a MAX98357A fed I2S (WS21_AUDIO_I2S), in pin_config.h. It plays the effects, the cries and the
// music, mixed as on a computer. Without either it's silent, as the Knomi, unless WS21_BUZZER beeps on its buzzer.
#if defined(BOARD_WS21) && (defined(WS21_AUDIO_I2S) || defined(WS21_AUDIO_PDM))
#define WS21_AMP 1
#endif
#if defined(BOARD_KNOMI_V2) && !defined(WS21_AMP) && !(defined(BOARD_WS21) && defined(WS21_BUZZER))
// The Knomi V2 has no codec or speaker (nor the Waveshare 2.1 without an amplifier): the game calls the same and
// nothing sounds.
void audioBegin() {}
void sfxPlay(uint8_t) {}
void audioSetEnabled(bool) {}
bool audioEnabled() { return false; }
void audioSetSleeping(bool) {}
bool audioBusy() { return false; }
void audioSetIdle(bool) {}
#elif defined(BOARD_WS21) && !defined(WS21_AMP)
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
#elif defined(WS21_AMP)
// The Waveshare 2.1 with an amplifier added by hand (docs/WAVESHARE_21.md, "Sound"; Fedor: "let's add music and sound
// through that amp"): the effects, the Pokemon's cries and the music, mixed as on a computer (host_audio.cpp) by a task
// on core 0 that streams to the amplifier without a break. A PAM8302A gets the mix as a 1-bit stream on one pin,
// smoothed by a resistor and a capacitor (WS21_AUDIO_PDM); a MAX98357A gets it as I2S (WS21_AUDIO_I2S).
#include <Arduino.h>
#include <Preferences.h>
#include <atomic>
#include <new>
#include "driver/i2s_std.h"
#include "driver/i2s_pdm.h"
#include "soc/usb_serial_jtag_reg.h"
#include "sfx_notes.h"
#include "music_synth.h"
#include "cry.h"
#ifdef WS21_AMP_EXIO
#include "ws21_exio.h"
#endif

#define SAMPLE_RATE 16000  // (a multiple of 100: the 1-bit stream then runs at its fixed 6.144 MHz)
#define BLOCK 128          // frames mixed at a time (8 ms); 4 of them wait in the DMA, so a tap sounds within ~40 ms
#define QUEUE_LEN 8
#define AMP_WAKE_MS 50     // the amplifier just switched on: an effect waits this long (it swallowed short ones)

static i2s_chan_handle_t gTx = nullptr;
static std::atomic<bool> gOn{true}, gSleeping{false}, gIdle{false}, gBusy{false};
static std::atomic<bool> gHold{false};          // an effect asked for at idle keeps the amplifier on until gHoldUntil
static std::atomic<uint32_t> gHoldUntil{0};
static uint8_t gQueue[QUEUE_LEN];               // the effects: one producer (the game), one consumer (the task)
static std::atomic<uint32_t> gHead{0}, gTail{0};

// the music: the game says which track its screen wants (musicWant), the task plays it. Silent with the Music switch
// off and while the pet sleeps (SOUND off leaves it playing)
static MusicPlayer gMusic;
static std::atomic<uint8_t> gMusicWant{MUS_NONE};
static std::atomic<bool> gMusicOn{true};
static std::atomic<bool> gMusicForced{false};  // the console's MUSIC n: plays while the pet sleeps too (testing)
static float gDuck = 1.0f;  // the music's level under the effects
#define DUCK_LEVEL 0.45f    // ...while one plays
#define DUCK_IN_MS 4.0f
#define DUCK_OUT_MS 120.0f
// the settings page's levels, Low, Medium and High (1..3), as on a computer: SOUND's the effects' and the cries'
// (saved as sndv), MUSIC's the music's (musv)
static const float LEVEL_GAIN[4] = { 0.0f, 0.3f, 0.55f, 1.0f };
static std::atomic<uint8_t> gSndLv{3}, gMusLv{3};
static float gSndGain = 1.0f, gMusGain = 1.0f;  // (the task's, eased toward the levels: no click)
#define LEVEL_EASE_MS 25.0f
static std::atomic<float> gFightTempo{1.0f};

// a Pokemon's cry (cry.h): built on the game's core, handed over once the task has taken the last one
static CryPlayer gVoice;
static Cry gVoiceNext;
static std::atomic<bool> gVoiceReady{false};

// the console's AUDIO: how much of core 0 the mixing takes, and how often the DMA ran dry
static uint32_t gStatFrom = 0, gStatUs = 0, gStatWorstUs = 0;
static std::atomic<uint32_t> gUnderruns{0};
static bool gAmpLevel = false;
static uint32_t gAmpOnAt = 0;

static void saveLevel(const char *key, uint8_t v) {
  Preferences p;
  p.begin(SAVE_NS, false);
  p.putUChar(key, v);
  p.end();
}

// the amplifier's on/off line, from the task only
static void ampSet(bool on) {
  if (on == gAmpLevel) return;
  gAmpLevel = on;
  if (on) gAmpOnAt = millis();
#ifdef WS21_AMP_EXIO
  gExio.write(WS21_AMP_EXIO, on);
#else
  digitalWrite(PA, on ? HIGH : LOW);
#endif
}

// ---- the effects' square waves (as host_audio.cpp) ----
static const SfxDef *gDef = nullptr;
static uint8_t gNote = 0;
static int gDone = 0, gTotal = 0;
static uint32_t gStep = 0, gAcc = 0;
static bool gLegato = false;
#define V2_ATTACK (SAMPLE_RATE * 4 / 1000)
#define V2_DECAY (SAMPLE_RATE * 6 / 1000)

static void startNote() {
  const Note &n = gDef->n[gNote];
  gTotal = SAMPLE_RATE * n.ms / 1000;
  gDone = 0;
  gStep = n.f ? (uint32_t)(((uint64_t)n.f << 32) / SAMPLE_RATE) : 0;
  if (!gLegato || gNote == 0) gAcc = 0;
}

// an effect's sample with the cry and the music added (playing: an effect sounds, so the music ducks)
static inline int16_t mix(int16_t s, bool playing) {
  static const float duckIn = 1.0f - expf(-1000.0f / (DUCK_IN_MS * SAMPLE_RATE));
  static const float duckOut = 1.0f - expf(-1000.0f / (DUCK_OUT_MS * SAMPLE_RATE));
  static const float ease = 1.0f - expf(-1000.0f / (LEVEL_EASE_MS * SAMPLE_RATE));
  float voice = gVoice.sample();
  gSndGain += (LEVEL_GAIN[gSndLv.load(std::memory_order_relaxed)] - gSndGain) * ease;
  float target = playing || gVoice.active() ? 1.0f - (1.0f - DUCK_LEVEL) * gSndGain : 1.0f;
  gDuck += (target - gDuck) * (target < gDuck ? duckIn : duckOut);
  gMusGain += (LEVEL_GAIN[gMusLv.load(std::memory_order_relaxed)] - gMusGain) * ease;
  float m = 0;
  gMusic.render(&m, 1, gDuck * gMusGain);
  int32_t v = (int32_t)lrintf((s + voice * 32767.0f) * gSndGain + m * 32767.0f);
  return (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
}

static void IRAM_ATTR noteUnderrun() { gUnderruns.fetch_add(1, std::memory_order_relaxed); }
static bool IRAM_ATTR onSendQOvf(i2s_chan_handle_t, i2s_event_data_t *, void *) {
  noteUnderrun();
  return false;
}

static void audioTask(void *) {
  static int16_t out[BLOCK * 2];  // stereo, L = R
  for (;;) {
    uint32_t t0 = micros(), now = millis();
    // the amplifier: on while anything sounds, or awake and in use with SOUND or the music on; off asleep or idle
    bool queued = gTail.load(std::memory_order_acquire) != gHead.load(std::memory_order_acquire);
    bool hold = gHold && (int32_t)(gHoldUntil.load() - now) > 0;
    bool sounding = gDef || queued || gVoice.active() || gVoiceReady.load() || gMusic.audible();
    ampSet(sounding || (!gSleeping && (hold || ((gOn || gMusicOn) && !gIdle))));
    bool warm = gAmpLevel && now - gAmpOnAt >= AMP_WAKE_MS;
    gBusy = sounding || hold;

    bool quiet = !gMusicOn;
    gMusic.want(quiet || (gSleeping && !gMusicForced) ? (uint8_t)MUS_NONE : gMusicWant.load(std::memory_order_relaxed),
                quiet);
    gMusic.fightTempo(gFightTempo.load(std::memory_order_relaxed));
    if (warm && gVoiceReady.load(std::memory_order_acquire)) {
      gVoice.start(gVoiceNext);
      gVoiceReady.store(false, std::memory_order_release);
    }
    for (int i = 0; i < BLOCK; i++) {
      if (!gDef) {
        uint32_t tail = gTail.load(std::memory_order_relaxed);
        if (!warm || tail == gHead.load(std::memory_order_acquire)) {
          out[i * 2] = out[i * 2 + 1] = mix(0, false);
          continue;
        }
        uint8_t id = gQueue[tail % QUEUE_LEN];
        gDef = &SFX[id];
        gLegato = sfxLegato(id);
        gTail.store(tail + 1, std::memory_order_release);
        gNote = 0;
        startNote();
      }
      int16_t s = 0;
      if (gStep) {
        bool first = gNote == 0, last = gNote + 1 >= gDef->len;
        int32_t amp = 5000;
        if (gLegato && last) amp -= amp * 45 / 100 * gDone / (gTotal ? gTotal : 1);  // the held one fades to 55%
        s = (int16_t)((gAcc & 0x80000000u) ? amp : -amp);
        if ((!gLegato || first) && gDone < V2_ATTACK) s = (int16_t)(s * gDone / V2_ATTACK);
        else if ((!gLegato || last) && gDone > gTotal - V2_DECAY) s = (int16_t)(s * (gTotal - gDone) / V2_DECAY);
        gAcc += gStep;
      }
      out[i * 2] = out[i * 2 + 1] = mix(s, true);
      if (++gDone >= gTotal) {
        if (++gNote >= gDef->len) gDef = nullptr;
        else startNote();
      }
    }
    uint32_t us = micros() - t0;
    gStatUs += us;
    if (us > gStatWorstUs) gStatWorstUs = us;
    size_t w = 0;
    i2s_channel_write(gTx, out, sizeof(out), &w, portMAX_DELAY);
  }
}

static bool outputStart() {
  i2s_chan_config_t cc = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
  cc.dma_desc_num = 4;
  cc.dma_frame_num = BLOCK;
  cc.auto_clear = true;  // run dry: silence, not the last block again
  esp_err_t e = i2s_new_channel(&cc, &gTx, nullptr);
#ifdef WS21_AUDIO_PDM
  if (e == ESP_OK) {
    // one data line and no clock line: the stream's density is the sound, the 1 kOhm and 10 nF turn it back into a
    // voltage for the PAM8302A's A+
    i2s_pdm_tx_config_t pc = {
      .clk_cfg = I2S_PDM_TX_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
      .slot_cfg = I2S_PDM_TX_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
      .gpio_cfg = {},
    };
    pc.clk_cfg.up_sample_fs = SAMPLE_RATE / 100;  // the stream at 128 x 48 kHz whatever the rate (the header's advice)
    pc.gpio_cfg.clk = I2S_GPIO_UNUSED;
    pc.gpio_cfg.dout = (gpio_num_t)PDM_DO_IO;
    pc.gpio_cfg.dout2 = I2S_GPIO_UNUSED;
    e = i2s_channel_init_pdm_tx_mode(gTx, &pc);
  }
#else
  if (e == ESP_OK) {
    i2s_std_config_t sc = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
      .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
      .gpio_cfg = {},
    };
    sc.gpio_cfg.mclk = I2S_GPIO_UNUSED;  // (the MAX98357A needs no master clock)
    sc.gpio_cfg.bclk = (gpio_num_t)I2S_BCK_IO;
    sc.gpio_cfg.ws = (gpio_num_t)I2S_WS_IO;
    sc.gpio_cfg.dout = (gpio_num_t)I2S_DO_IO;
    sc.gpio_cfg.din = I2S_GPIO_UNUSED;
    e = i2s_channel_init_std_mode(gTx, &sc);
  }
#endif
  if (e == ESP_OK) {
    i2s_event_callbacks_t cb = {};
    cb.on_send_q_ovf = onSendQOvf;
    i2s_channel_register_event_callback(gTx, &cb, nullptr);
    e = i2s_channel_enable(gTx);
  }
  if (e != ESP_OK) {
    Serial.printf("audio: the amplifier's output didn't start (%s): silent\n", esp_err_to_name(e));
    if (gTx) i2s_del_channel(gTx);
    gTx = nullptr;
    return false;
  }
  return true;
}

void audioBegin() {
#ifdef WS21_AMP_EXIO
  gExio.write(WS21_AMP_EXIO, false);
#else
  // GPIO 19 and 20 are the "USB" port's data lines: the S3's own USB lets go of them (and of its pull-up on 20, which
  // would hold the amplifier on)
  CLEAR_PERI_REG_MASK(USB_SERIAL_JTAG_CONF0_REG, USB_SERIAL_JTAG_USB_PAD_ENABLE | USB_SERIAL_JTAG_DP_PULLUP);
  pinMode(PA, OUTPUT);
  digitalWrite(PA, LOW);
#endif
  Preferences p;
  p.begin(SAVE_NS, true);
  gOn = p.getBool("snd", true);
  gMusicOn = p.getBool("mus", true);
  uint8_t sv = p.getUChar("sndv", 3), mv = p.getUChar("musv", 3);
  p.end();
  gSndLv = sv >= 1 && sv <= 3 ? sv : 3;
  gMusLv = mv >= 1 && mv <= 3 ? mv : 3;
  gSndGain = LEVEL_GAIN[gSndLv];
  gMusGain = LEVEL_GAIN[gMusLv];
  gMusic.begin(SAMPLE_RATE);
  gVoice.begin(SAMPLE_RATE);
  if (!outputStart()) return;
  gStatFrom = millis();
  // core 0, under the touch task (5): the DMA holds 32 ms, so a touch read in between costs nothing
  xTaskCreatePinnedToCore(audioTask, "audio", 6144, nullptr, 4, nullptr, 0);
  sfxPlay(SFX_HATCH);  // the start-up jingle: it sounds
}

void sfxPlay(uint8_t id) {
  if (!gTx || gSleeping || !gOn || id >= SFX_COUNT) return;
  if (gIdle) {  // an alert at idle (e.g. "ready to evolve") has to be heard
    gHoldUntil = millis() + 4000;
    gHold = true;
  }
  uint32_t head = gHead.load(std::memory_order_relaxed);
  if (head - gTail.load(std::memory_order_acquire) >= QUEUE_LEN) return;  // full: dropped
  gQueue[head % QUEUE_LEN] = id;
  gHead.store(head + 1, std::memory_order_release);
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
void audioSetIdle(bool idle) { gIdle = idle; }
bool audioBusy() { return gBusy; }

void voiceSay(int16_t dex, uint8_t mood) {
  if (!gTx || gSleeping || !gOn || gVoiceReady.load(std::memory_order_acquire)) return;
  cryBuild(gVoiceNext, dex, mood);
  gVoiceReady.store(true, std::memory_order_release);
}
void musicWant(uint8_t track) { gMusicWant.store(track, std::memory_order_relaxed); }
void audioForceMusic(bool on) { gMusicForced = on; }
void musicTempo(float f) { gFightTempo.store(f, std::memory_order_relaxed); }
bool musicEnabled() { return gMusicOn; }
void musicSetEnabled(bool on) {
  gMusicOn = on;
  Preferences p;
  p.begin(SAVE_NS, false);
  p.putBool("mus", on);
  p.end();
}
uint8_t soundVolume() { return gSndLv; }
void soundSetVolume(uint8_t v) {
  gSndLv = v < 1 ? 1 : v > 3 ? 3 : v;
  saveLevel("sndv", gSndLv);
  if (!gOn) audioSetEnabled(true);
}
uint8_t musicVolume() { return gMusLv; }
void musicSetVolume(uint8_t v) {
  gMusLv = v < 1 ? 1 : v > 3 ? 3 : v;
  saveLevel("musv", gMusLv);
  if (!gMusicOn) musicSetEnabled(true);
}

// the console's AUDIO: since the last one, how much of core 0 the mixing took, its longest block, and how often the
// stream ran dry
void audioReport() {
  uint32_t now = millis(), span = now - gStatFrom;
  Serial.printf("audio: %s, %u Hz; mixing %.1f%% of core 0 (longest block %.2f of %.2f ms); ran dry %u times; "
                "amplifier %s; music %s, track %u%s; sound %s\n",
                gTx ? "running" : "NOT running", SAMPLE_RATE, span ? gStatUs / 10.0f / span : 0.0f, gStatWorstUs / 1000.0f,
                BLOCK * 1000.0f / SAMPLE_RATE, (unsigned)gUnderruns.exchange(0), gAmpLevel ? "on" : "off",
                gMusicOn ? "on" : "off", (unsigned)gMusic.playing(), gMusic.audible() ? " (playing)" : "",
                gOn ? "on" : "off");
  gStatUs = gStatWorstUs = 0;
  gStatFrom = now;
}

// the console's AUDIOBENCH n: 2 s of track n mixed by a player of its own (not the one playing), timed, so the music's
// cost on this chip can be seen whatever the pet is doing (asleep, nothing plays). It holds the game for that long
void audioBench(uint8_t track) {
  MusicPlayer *mp = new (std::nothrow) MusicPlayer();
  if (!mp) {
    Serial.println("bench: no memory");
    return;
  }
  mp->begin(SAMPLE_RATE);
  mp->want(track);
  static float b[BLOCK];
  float peak = 0;
  double sq = 0;
  const int blocks = 2 * SAMPLE_RATE / BLOCK;
  uint32_t worst = 0, t0 = micros();
  for (int k = 0; k < blocks; k++) {
    uint32_t t1 = micros();
    memset(b, 0, sizeof b);
    mp->render(b, BLOCK, 1.0f);
    uint32_t us = micros() - t1;
    if (us > worst) worst = us;
    for (float v : b) {
      sq += v * v;
      if (fabsf(v) > peak) peak = fabsf(v);
    }
  }
  uint32_t us = micros() - t0;
  Serial.printf("bench: track %u, 2 s of music mixed in %.1f ms (%.1f%% of a core; longest block %.2f of %.2f ms); "
                "peak %.2f, rms %.3f of full scale\n", (unsigned)track, us / 1000.0f, us / 20000.0f, worst / 1000.0f,
                BLOCK * 1000.0f / SAMPLE_RATE, peak, sqrt(sq / (blocks * BLOCK)));
  delete mp;
}

#else
#include <Arduino.h>
#include <Wire.h>
#include <ESP_I2S.h>
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
    digitalWrite(PA, want);
  }
}

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
  pinMode(PA, OUTPUT);
  digitalWrite(PA, LOW);   // until we know whether sound is enabled

  i2s.setPins(I2S_BCK_IO, I2S_WS_IO, I2S_DO_IO, I2S_DI_IO, I2S_MCK_IO);
  if (!i2s.begin(I2S_MODE_STD, SAMPLE_RATE, I2S_DATA_BIT_WIDTH_16BIT,
                 I2S_SLOT_MODE_STEREO, I2S_STD_SLOT_BOTH)) {
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
