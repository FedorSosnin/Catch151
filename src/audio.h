#pragma once
#include <stdint.h>

// Game sound effects (queue, non-blocking). The order matches the
// SFX table in audio.cpp.
enum Sfx : uint8_t {
  SFX_TAP = 0,  // tap / button
  SFX_EAT,      // eat
  SFX_PLAY,     // minigame point / hit
  SFX_HEART,    // likes it / cuddle
  SFX_HATCH,    // hatch
  SFX_EVOLVE,   // evolution
  SFX_MEDAL,    // medal / milestone
  SFX_DENY,     // action not allowed
  SFX_BYE,      // goodbye
  SFX_LEVEL,    // levels up
  SFX_SHAKE1,   // the ball rocks: one...
  SFX_SHAKE2,   // ...two...
  SFX_SHAKE3,   // ...three
  // Reel Fishing (fishing.h): each moment has its own (and its own buzz on a phone or a controller)
  SFX_FISH_CAST,    // the rod swings
  SFX_FISH_PLIP,    // the float lands
  SFX_FISH_NIBBLE,  // a nibble: a soft tik
  SFX_FISH_BITE,    // the bite: a plunk and a rising alarm
  SFX_FISH_HOOK,    // struck in time
  SFX_FISH_REEL,    // a tap reels: the click
  SFX_FISH_WARN,    // a thrash is coming: a creak
  SFX_FISH_STRAIN,  // too tight: a warning beep (again and again while it stays so)
  SFX_FISH_SLACK,   // too slack: the line running out
  SFX_FISH_SNAP,    // the line snaps
  SFX_FISH_LANDED,  // landed: a fanfare
  SFX_FISH_SCALE,   // the weigh-in: tick, tick, ding
  SFX_COUNT
};

void audioBegin();          // init ES8311 + I2S + amplifier + audio task
void sfxPlay(uint8_t id);   // queues an effect (doesn't block the loop)
void audioSetEnabled(bool on);
bool audioEnabled();
void audioSetSleeping(bool sleeping);  // asleep: amplifier off
bool audioBusy();                      // an effect requested at idle may still be playing: don't sleep the CPU
void audioSetIdle(bool idle);          // no recent use: amplifier off (an effect turns it on for 4 s)
