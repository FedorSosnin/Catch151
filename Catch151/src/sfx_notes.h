#pragma once
#include <stdint.h>
#include "audio.h"

// Sound effects as square-wave tone notes (Game Boy style); f == 0 is a rest.
// The SFX order matches the Sfx enum.
struct Note { uint16_t f, ms; };

// V2: the tap is Fedor's "confirm" blip, as measured: 2049 and 2731 Hz (a fourth apart), low, high, low in 16 ms
// steps, then the high one held 83 ms and fading; its notes run into each other (sfxLegato())
static const Note N_TAP[]    = {{2049, 17}, {2731, 16}, {2049, 16}, {2731, 83}};
#define N_TAP_LEN 4
static const Note N_EAT[]    = {{660, 45}, {0, 12}, {660, 45}};
static const Note N_PLAY[]   = {{784, 45}, {988, 60}};
static const Note N_HEART[]  = {{1047, 55}, {1319, 90}};
static const Note N_HATCH[]  = {{523, 80}, {659, 80}, {784, 110}, {1047, 170}};
static const Note N_EVOLVE[] = {{523, 80}, {659, 80}, {784, 80}, {1047, 90}, {1319, 230}};
static const Note N_MEDAL[]  = {{784, 70}, {0, 25}, {784, 70}, {0, 25}, {1047, 200}};
static const Note N_DENY[]   = {{300, 110}, {200, 170}};
static const Note N_BYE[]    = {{784, 150}, {659, 150}, {523, 280}};
static const Note N_LEVEL[]  = {{784, 70}, {1047, 130}};
static const Note N_SHAKE1[] = {{523, 110}};  // the count to three, rising
static const Note N_SHAKE2[] = {{659, 110}};
static const Note N_SHAKE3[] = {{784, 110}};

// Reel Fishing's
static const Note N_FISH_CAST[]   = {{1568, 20}, {1319, 20}, {1047, 20}, {880, 25}, {698, 30}};
static const Note N_FISH_PLIP[]   = {{1760, 25}, {0, 10}, {1175, 40}};
static const Note N_FISH_NIBBLE[] = {{2093, 18}};
static const Note N_FISH_BITE[]   = {{196, 50}, {0, 15}, {1319, 70}, {1760, 110}};
static const Note N_FISH_HOOK[]   = {{587, 30}, {784, 30}, {1047, 30}, {1397, 60}};
static const Note N_FISH_REEL[]   = {{2637, 10}};
static const Note N_FISH_WARN[]   = {{330, 40}, {311, 40}, {294, 60}};
static const Note N_FISH_STRAIN[] = {{2349, 40}, {0, 30}, {2349, 40}};
static const Note N_FISH_SLACK[]  = {{880, 40}, {784, 40}, {698, 40}, {587, 60}};
static const Note N_FISH_SNAP[]   = {{3136, 15}, {1568, 15}, {392, 40}, {196, 80}};
static const Note N_FISH_LANDED[] = {{784, 60}, {988, 60}, {1175, 60}, {1568, 160}};
static const Note N_FISH_SCALE[]  = {{1047, 30}, {0, 40}, {1047, 30}, {0, 40}, {1568, 140}};

struct SfxDef { const Note *n; uint8_t len; };
static const SfxDef SFX[SFX_COUNT] = {
  {N_TAP, N_TAP_LEN}, {N_EAT, 3}, {N_PLAY, 2}, {N_HEART, 2}, {N_HATCH, 4},
  {N_EVOLVE, 5}, {N_MEDAL, 5}, {N_DENY, 2}, {N_BYE, 3}, {N_LEVEL, 2},
  {N_SHAKE1, 1}, {N_SHAKE2, 1}, {N_SHAKE3, 1},
  {N_FISH_CAST, 5}, {N_FISH_PLIP, 3}, {N_FISH_NIBBLE, 1}, {N_FISH_BITE, 4}, {N_FISH_HOOK, 4}, {N_FISH_REEL, 1},
  {N_FISH_WARN, 3}, {N_FISH_STRAIN, 3}, {N_FISH_SLACK, 4}, {N_FISH_SNAP, 4}, {N_FISH_LANDED, 4}, {N_FISH_SCALE, 5},
};
// an effect whose notes run into each other: no fade between them, the wave carried on, and its last note fading
// to about half by its end (the tap)
static inline bool sfxLegato(uint8_t id) { return id == SFX_TAP; }
