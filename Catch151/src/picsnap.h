#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>

// Pic Snap: a camera on Knomi Island, its rules without hardware (test/test_picsnap.cpp); picsnap_v2.h draws it and
// the sketch wires it in. Fedor (2026-11-22): "a bunch (8-10) scenes ... where wild pokemon show up. Its your job to
// snap a photo that is in focus. You do this by pressing anywhere on the screen to take a photo. The camera would
// automatically move around slightly and go in and out of focus. So you have to time things correctly to take a good
// photo. You are given a score based on how focused and how centered the pokemon is in each photo you take."
//
// A roll of PS_SHOTS photos, one in each of the PS_SCENES scenes, in a new order every game. In each, a wild Pokemon of
// that place shows up (out of the grass, in from the sky, out of the dark) and stays a few seconds. The camera drifts
// on its own, and its lens hunts for focus: the sharp plane sweeps from near (0) to far (1) and back, through the
// Pokemon's depth on every sweep, and now and then it settles on the Pokemon for a moment. A tap anywhere takes the
// photo. It scores up to PS_PTS_FOCUS for focus and PS_PTS_CENTRE for the Pokemon in the middle: 100 a photo, 1,000 a
// roll. Wait too long and it's gone (no points for that one).

#define PS_SHOTS 10        // photos a roll
#define PS_SCENES 10       // places (two in each zone)
#define PS_CAST 8          // who can turn up in a place: the first PS_CAST_COMMON often, the rest seldom
#define PS_CAST_COMMON 6

// how a Pokemon shows up in its place, and leaves if it isn't snapped
enum PsHow : uint8_t {
  PSH_POP,   // up from behind what's at its feet (tall grass, a rock, crystals, the sea), and back down
  PSH_FLY,   // in from the sky to its spot, hovering, and off the other way
  PSH_FADE,  // out of the dark, slowly, as ghosts do, and fading again
};

struct PsSceneDef {
  uint8_t zone;      // zones.h's (Z_FOREST 0 .. Z_FIELD 4), for its music and its look
  uint8_t how;       // PsHow
  uint8_t depth;     // where it stands, 0 near .. 100 far: the lens is sharp on it there
  int16_t feetX, feetY;  // where its feet are in the picture (px, the camera at rest)
  int16_t cast[PS_CAST];  // dex numbers (0: none), all from the zone
};

// The places, in picsnap_v2.h's order (psPaint()). Each zone's own Pokemon, picked to suit the spot: the bugs and the
// grass ones in the tall grass, the swimmers in the sea, the birds in the air
enum : uint8_t {
  PS_GRASS,     // the Forest: down in the tall grass
  PS_PINES,     // the Forest: under the pines, mushrooms in front
  PS_SHORE,     // the Beach: the sand at the water's edge, a starfish in front
  PS_WAVES,     // the Beach: out on the water, a wave in front
  PS_CRYSTAL,   // the Cave: crystals glowing in the dark
  PS_LEDGE,     // the Cave: a ledge of rock
  PS_GRAVES,    // the Mansion: the graveyard under the moon
  PS_WINDOW,    // the Mansion: under its lit window
  PS_SKY,       // the Field: in the air over its fence
  PS_FLOWERS,   // the Field: flowers at sunset
};
static const PsSceneDef PS_SCENE[PS_SCENES] = {
  { 0, PSH_POP, 50, 233, 292, { 43, 69, 10, 13, 46, 1, 123, 127 } },    // Oddish, Bellsprout, Caterpie, Weedle, Paras, Bulbasaur; Scyther, Pinsir
  { 0, PSH_POP, 45, 233, 290, { 25, 52, 56, 48, 23, 102, 37, 114 } },   // Pikachu, Meowth, Mankey, Venonat, Ekans, Exeggcute; Vulpix, Tangela
  { 1, PSH_POP, 55, 233, 292, { 98, 7, 90, 120, 54, 79, 124, 134 } },   // Krabby, Squirtle, Shellder, Staryu, Psyduck, Slowpoke; Jynx, Vaporeon
  { 1, PSH_POP, 60, 233, 286, { 72, 86, 116, 118, 129, 60, 131, 0 } },  // Tentacool, Seel, Horsea, Goldeen, Magikarp, Poliwag; Lapras
  { 2, PSH_POP, 50, 233, 290, { 35, 74, 27, 41, 138, 140, 147, 0 } },   // Clefairy, Geodude, Sandshrew, Zubat, Omanyte, Kabuto; Dratini
  { 2, PSH_POP, 45, 233, 288, { 66, 104, 74, 50, 27, 41, 111, 95 } },   // Machop, Cubone, Geodude, Diglett, Sandshrew, Zubat; Rhyhorn, Onix
  { 3, PSH_FADE, 55, 233, 268, { 92, 93, 109, 92, 93, 109, 94, 110 } }, // Gastly, Haunter, Koffing (floating); Gengar, Weezing
  { 3, PSH_POP, 50, 233, 290, { 63, 96, 81, 100, 88, 92, 122, 137 } },  // Abra, Drowzee, Magnemite, Voltorb, Grimer, Gastly; Mr. Mime, Porygon
  { 4, PSH_FLY, 50, 233, 262, { 16, 21, 17, 16, 21, 22, 83, 18 } },     // Pidgey, Spearow, Pidgeotto, Fearow (in the air); Farfetch'd, Pidgeot
  { 4, PSH_POP, 45, 233, 292, { 133, 39, 4, 19, 16, 77, 58, 113 } },    // Eevee, Jigglypuff, Charmander, Rattata, Pidgey, Ponyta; Growlithe, Chansey
};

// the roll's pace: the Pokemon shows up, stays (less long as the roll goes on), leaves
#define PS_IN_S 0.8f
#define PS_STAY_S 7.0f        // the first photo's...
#define PS_STAY_LAST_S 5.0f   // ...down to the last's
#define PS_OUT_S 0.7f
#define PS_SNAP_S 0.55f       // the flash, the picture held
#define PS_PRINT_S 3.0f       // the photo and its score...
#define PS_PRINT_MIN_S 0.7f   // ...a tap sends it off from here
#define PS_AWAY_S 1.4f        // IT GOT AWAY! over the place

// the lens: each sweep crosses the Pokemon's depth, from a little past it on one side to the other
#define PS_SWEEP_S 1.15f      // a sweep's time at the first photo...
#define PS_SWEEP_LAST_S 0.75f // ...and at the last
#define PS_SWEEP_PAST 0.16f   // a sweep ends at least this far past the Pokemon...
#define PS_SWEEP_MAX 0.50f    // ...and at most this far
#define PS_LOCK_PCT 30        // a sweep in this many in 100 stops on the Pokemon instead...
#define PS_LOCK_S 0.45f       // ...and holds there this long
#define PS_LOCK_OFF 0.015f    // (about: within this of it)

// the camera's drift: two slow waves each way, a little wider as the roll goes on (px, the canvas's)
#define PS_DRIFT_X 32.0f
#define PS_DRIFT_X_LAST 44.0f
#define PS_DRIFT_Y 22.0f
#define PS_DRIFT_Y_LAST 32.0f

// the score: focus (how far the sharp plane was from the Pokemon) and centre (how far it was from the middle, px)
#define PS_PTS_FOCUS 50
#define PS_PTS_CENTRE 50
#define PS_F_FULL 0.04f   // as sharp as it gets: all its points
#define PS_F_ZERO 0.22f   // a blur: none
#define PS_C_FULL 4.0f    // (px)
#define PS_C_ZERO 32.0f   // (tuned with a patient bot and a random tapper, test_picsnap.cpp: with these a roll
                          // scores about 770 waiting for the moment, about 380 tapping at any old one; at 90 px and
                          // 0.30 the random tapper scored three quarters of the best)
#define PS_GOOD 80        // GREAT SHOT!
#define PS_PERFECT 95     // PERFECT!

enum PsPhase : uint8_t {
  PSP_LOAD,   // the place being made ready (picsnap_v2.h, a little each frame): psReady() when it is
  PSP_IN,     // the Pokemon showing up
  PSP_AIM,    // there: a tap takes the photo
  PSP_OUT,    // ...it's leaving, not snapped
  PSP_AWAY,   // IT GOT AWAY!
  PSP_SNAP,   // the flash, the picture held
  PSP_PRINT,  // the photo and its score
  PSP_END,
};

// What the Pokemon does while it's there, so the place looks alive (Fedor: "The pokemon should be animated"): stands
// breathing, takes a few steps one way or the other (which moves it off the middle: something more to time), hops, or
// does one of its own moves (a nod, a pose, a big breath: the drawing picks it by look). Swimmers bob and drift instead
// of walking; flyers and ghosts weave as they hover
enum PsAct : uint8_t { PSA_IDLE, PSA_WALK, PSA_HOP, PSA_LOOK };
#define PS_WALK_MAX 28.0f   // how far from its spot it walks (px)...
#define PS_WALK_V 40.0f     // ...and how fast (px a second)
#define PS_HOP_S 0.5f
#define PS_HOP_H 14.0f
#define PS_LOOK_S 1.3f

// what happened, for the sounds and words (psStep(), psSnap())
#define PSE_IN 1u        // it's showing up (its cry)
#define PSE_SNAP 2u      // a photo taken
#define PSE_GOOD 4u      // ...a great one
#define PSE_PERFECT 8u   // ...perfect
#define PSE_AWAY 16u     // it got away
#define PSE_LOAD 32u     // on to the next place: make it ready
#define PSE_END 64u      // the roll is done
#define PSE_LOCK 128u    // the lens settled on it

struct PsState {
  uint8_t order[PS_SHOTS];  // the places, in this roll's order
  uint8_t shot = 0;         // the photo being taken (0..PS_SHOTS-1)
  uint8_t phase = PSP_LOAD;
  float t = 0;              // seconds in this phase
  float st = 0;             // seconds in this place (the drift's clock)
  int16_t dex = 0;          // who's here
  bool shiny = false;
  // the lens: the sharp plane now (0 near .. 1 far), on its way from fa to fb (ft of fdur seconds), then holding
  float f = 0, fa = 0, fb = 0, ft = 0, fdur = 1, fhold = 0;
  bool fLock = false;       // this sweep stops on the Pokemon
  // the drift: the camera's wander now (px), and its waves
  float wx = 0, wy = 0;
  float wph[4] = {}, wfr[4] = {};
  float ampX = PS_DRIFT_X, ampY = PS_DRIFT_Y;
  // the Pokemon: its own move from its spot (px), and how much of it shows (0..1: a ghost fading, one rising)
  float ox = 0, oy = 0, vis = 0;
  uint8_t act = PSA_IDLE;   // what it's doing (PsAct), for actT of actDur seconds
  uint8_t look = 0;         // ...which of its own moves (PSA_LOOK: 0..2)
  float actT = 0, actDur = 1;
  float walkFrom = 0, walkTo = 0, walkX = 0;  // its walk: from, to, where it is (px from its spot)
  // where its middle is, off the picture's middle with the camera at rest (px): the camera aims at it, but only so far
  // (picsnap_v2.h's PS_AIM_MAX), so a small one low down sits a little under the middle (psAimed())
  float resX = 0, resY = 0;
  float stay = PS_STAY_S;
  // this photo, as taken
  float snapErrF = 0, snapDist = 0;
  uint8_t focusPts = 0, centrePts = 0;
  // the roll
  uint8_t pts[PS_SHOTS] = {};
  int16_t dexOf[PS_SHOTS] = {};
  bool got[PS_SHOTS] = {};
  uint16_t score = 0;
  uint32_t rng = 1;
};

inline uint32_t psRand(PsState &s) {  // xorshift32
  uint32_t x = s.rng ? s.rng : 0x9E3779B9u;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return s.rng = x;
}
inline float psRandF(PsState &s) { return (psRand(s) >> 8) / 16777216.0f; }  // 0..1
inline float psRange(PsState &s, float a, float b) { return a + (b - a) * psRandF(s); }
inline float psEase(float k) { return k <= 0 ? 0 : k >= 1 ? 1 : k * k * (3 - 2 * k); }

inline const PsSceneDef &psScene(const PsState &s) { return PS_SCENE[s.order[s.shot < PS_SHOTS ? s.shot : PS_SHOTS - 1]]; }
inline uint8_t psPlace(const PsState &s) { return s.order[s.shot < PS_SHOTS ? s.shot : PS_SHOTS - 1]; }
inline float psDepth(const PsState &s) { return psScene(s).depth / 100.0f; }
// 0 at the roll's first photo .. 1 at its last
inline float psRamp(const PsState &s) { return s.shot / (float)(PS_SHOTS - 1); }

// how far the sharp plane is from the Pokemon
inline float psFocusErr(const PsState &s) { return fabsf(s.f - psDepth(s)); }
// how far the Pokemon is from the middle of the picture (px): its own move against the camera's
inline float psCentreDist(const PsState &s) { return hypotf(s.ox - s.wx + s.resX, s.oy - s.wy + s.resY); }
// the drawing's word on where its middle sits with the camera at rest (set as each place is ready)
inline void psAimed(PsState &s, float resX, float resY) { s.resX = resX, s.resY = resY; }

// a photo's points for focus and for centre
inline uint8_t psFocusPts(float err) {
  if (err <= PS_F_FULL) return PS_PTS_FOCUS;
  if (err >= PS_F_ZERO) return 0;
  return (uint8_t)lroundf(PS_PTS_FOCUS * (PS_F_ZERO - err) / (PS_F_ZERO - PS_F_FULL));
}
inline uint8_t psCentrePts(float dist) {
  if (dist <= PS_C_FULL) return PS_PTS_CENTRE;
  if (dist >= PS_C_ZERO) return 0;
  return (uint8_t)lroundf(PS_PTS_CENTRE * (PS_C_ZERO - dist) / (PS_C_ZERO - PS_C_FULL));
}

// The blur each part of the picture gets (in the Knomi's pixels, its 2 x 2 blocks), for the drawing: the Pokemon by
// how far the sharp plane is from it, one pixel for every 0.05 (none while it scores all its focus points); the near
// things at the picture's edge sharp when the lens is at 0, the far ones when it's at 1, PS_BLUR_MAX px at the other
#define PS_BLUR_MAX 6
#define PS_BLUR_MON_MAX 8
inline int psMonBlur(float err) {
  int r = (int)floorf((err - PS_F_FULL) / 0.05f) + 1;
  return err <= PS_F_FULL ? 0 : r > PS_BLUR_MON_MAX ? PS_BLUR_MON_MAX : r;
}
// the lens's step for the near and far things: they're made ready at PS_LENS_STEPS sharp planes, 0 to 1
#define PS_LENS_STEPS (PS_BLUR_MAX + 1)
inline int psLensStep(float f) {
  int i = (int)lroundf(f * PS_BLUR_MAX);
  return i < 0 ? 0 : i > PS_BLUR_MAX ? PS_BLUR_MAX : i;
}

// who turns up: a common one most of the time, now and then one of the rest; 1 in 100 shiny
inline int16_t psPickDex(PsState &s, uint8_t place) {
  const PsSceneDef &d = PS_SCENE[place];
  int n = 0, w[PS_CAST], sum = 0;
  for (int i = 0; i < PS_CAST; i++) {
    w[i] = d.cast[i] ? (i < PS_CAST_COMMON ? 6 : 1) : 0;
    sum += w[i];
    if (d.cast[i]) n++;
  }
  if (!n) return 25;
  int r = (int)(psRand(s) % (uint32_t)sum);
  for (int i = 0; i < PS_CAST; i++) {
    if (r < w[i]) return d.cast[i];
    r -= w[i];
  }
  return d.cast[0];
}

// the lens's next sweep: from where it is to past the Pokemon on the other side (or, now and then, onto it)
inline void psSweep(PsState &s) {
  float d = psDepth(s), dur = PS_SWEEP_S + (PS_SWEEP_LAST_S - PS_SWEEP_S) * psRamp(s);
  s.fa = s.f;
  s.ft = 0;
  s.fhold = 0;
  s.fLock = !s.fLock && (psRand(s) % 100) < PS_LOCK_PCT && fabsf(s.f - d) > PS_SWEEP_PAST * 0.8f;
  if (s.fLock) {
    s.fb = d + psRange(s, -PS_LOCK_OFF, PS_LOCK_OFF);
    s.fdur = dur * psRange(s, 0.55f, 0.8f);
    s.fhold = PS_LOCK_S;
  } else {
    float past = psRange(s, PS_SWEEP_PAST, PS_SWEEP_MAX);
    float to = s.f < d ? d + past : d - past;
    if (to > 1) to = 1;
    if (to < 0) to = 0;
    if (fabsf(to - d) < PS_SWEEP_PAST * 0.75f) to = s.f < d ? d - past : d + past;  // no room that side: back across
    s.fb = to;
    s.fdur = dur * psRange(s, 0.8f, 1.25f) * (0.6f + fabsf(to - s.f));  // a longer way takes longer
  }
}

// the camera's wander at st seconds in the place (px)
inline void psDrift(PsState &s) {
  s.wx = s.ampX * (0.62f * sinf(s.wfr[0] * s.st + s.wph[0]) + 0.38f * sinf(s.wfr[1] * s.st + s.wph[1]));
  s.wy = s.ampY * (0.6f * sinf(s.wfr[2] * s.st + s.wph[2]) + 0.4f * sinf(s.wfr[3] * s.st + s.wph[3]));
}

// the next place: who's there, where the lens and the camera start (the drawing makes it ready: PSP_LOAD)
inline void psBeginPlace(PsState &s) {
  s.phase = PSP_LOAD;
  s.t = s.st = 0;
  s.dex = psPickDex(s, psPlace(s));
  s.shiny = psRand(s) % 100 == 0;
  s.ox = s.oy = 0;
  s.vis = 0;
  s.resX = s.resY = 0;
  s.act = PSA_IDLE, s.actT = 0, s.actDur = 1.2f, s.walkFrom = s.walkTo = s.walkX = 0;
  s.stay = PS_STAY_S + (PS_STAY_LAST_S - PS_STAY_S) * psRamp(s);
  float d = psDepth(s);
  s.f = psRandF(s) < 0.5f ? d - psRange(s, 0.3f, 0.5f) : d + psRange(s, 0.3f, 0.45f);  // well out of focus
  s.f = s.f < 0 ? 0 : s.f > 1 ? 1 : s.f;
  s.fLock = true;  // (the first sweep never stops on it)
  psSweep(s);
  for (int i = 0; i < 4; i++) s.wph[i] = psRange(s, 0, 6.2832f);
  s.wfr[0] = psRange(s, 0.9f, 1.4f), s.wfr[1] = psRange(s, 2.0f, 2.9f);
  s.wfr[2] = psRange(s, 1.0f, 1.5f), s.wfr[3] = psRange(s, 2.2f, 3.1f);
  s.ampX = PS_DRIFT_X + (PS_DRIFT_X_LAST - PS_DRIFT_X) * psRamp(s);
  s.ampY = PS_DRIFT_Y + (PS_DRIFT_Y_LAST - PS_DRIFT_Y) * psRamp(s);
  psDrift(s);
}

// a new roll: the ten places in a new order
inline void psStart(PsState &s, uint32_t seed) {
  s = PsState();
  s.rng = seed ? seed : 1;
  for (int i = 0; i < 4; i++) psRand(s);
  for (int i = 0; i < PS_SHOTS; i++) s.order[i] = (uint8_t)(i % PS_SCENES);
  for (int i = PS_SHOTS - 1; i > 0; i--) {
    int j = (int)(psRand(s) % (uint32_t)(i + 1));
    uint8_t t = s.order[i];
    s.order[i] = s.order[j];
    s.order[j] = t;
  }
  psBeginPlace(s);
}

// the place is ready to show (picsnap_v2.h has made its pictures): the Pokemon shows up
inline uint32_t psReady(PsState &s) {
  if (s.phase != PSP_LOAD) return 0;
  s.phase = PSP_IN;
  s.t = 0;
  return PSE_IN;
}

// swimming: the places in the water (they bob and drift; no walking, no hops)
inline bool psSwims(const PsState &s) { return psPlace(s) == PS_WAVES; }
// its next thing to do, once it's there: mostly standing a while, then a walk, a hop or one of its own moves
inline void psMonNext(PsState &s) {
  uint8_t how = psScene(s).how;
  s.actT = 0;
  uint32_t r = psRand(s) % 100;
  bool walks = how == PSH_POP && !psSwims(s);
  if (s.act != PSA_IDLE) r = 0;  // between two things, it stands a moment
  if (r < 40) {
    s.act = PSA_IDLE;
    s.actDur = psRange(s, 0.9f, 2.0f);
  } else if (r < 65 && walks) {
    s.act = PSA_WALK;
    s.walkFrom = s.walkX;
    s.walkTo = psRange(s, -PS_WALK_MAX, PS_WALK_MAX);
    if (fabsf(s.walkTo - s.walkFrom) < 12) s.walkTo = s.walkFrom > 0 ? s.walkFrom - 20 : s.walkFrom + 20;
    s.actDur = fabsf(s.walkTo - s.walkFrom) / PS_WALK_V;
  } else if (r < 80 && walks) {
    s.act = PSA_HOP;
    s.actDur = PS_HOP_S;
  } else {
    s.act = PSA_LOOK;
    s.look = (uint8_t)(psRand(s) % 3);
    s.actDur = PS_LOOK_S;
  }
}
// the Pokemon's own move: how it shows up (k 0..1), then what it does while it's there
inline void psMonMove(PsState &s) {
  uint8_t how = psScene(s).how;
  float k = 1, hover = 0;
  if (s.phase == PSP_IN) k = psEase(s.t / PS_IN_S);
  else if (s.phase == PSP_OUT) k = 1 - psEase(s.t / PS_OUT_S);
  else if (s.phase == PSP_LOAD || s.phase == PSP_AWAY) k = 0;
  if (how != PSH_POP) hover = 5 * sinf(s.st * 2.3f);  // flyers and ghosts bob
  s.ox = 0, s.oy = hover, s.vis = k;
  if (how == PSH_POP) {
    s.oy = (1 - k) * 90;  // up from behind what's at its feet (the drawing hides what's under them)
    if (s.phase == PSP_IN && k > 0.7f) s.oy -= 10 * sinf((k - 0.7f) / 0.3f * 3.14159f);  // a little hop as it's up
    if (s.act == PSA_WALK) s.walkX = s.walkFrom + (s.walkTo - s.walkFrom) * psEase(s.actT / s.actDur);
    s.ox = s.walkX;
    if (s.act == PSA_HOP) s.oy -= PS_HOP_H * sinf(s.actT / PS_HOP_S * 3.14159f);
    if (psSwims(s)) s.ox += 8 * sinf(s.st * 0.9f), s.oy += 4 * sinf(s.st * 1.9f);  // on the swell
  } else if (how == PSH_FLY) {
    float side = s.phase == PSP_OUT ? -1.0f : 1.0f;  // in from the right, off to the left
    s.ox = side * (1 - k) * 300 + 12 * sinf(s.st * 1.1f);  // weaving as it hovers
    s.oy += -(1 - k) * 150 + (1 - k) * k * 60;
  } else {
    s.ox = 8 * sinf(s.st * 0.8f);  // a ghost drifts
  }
}

// a frame: dt seconds on
inline uint32_t psStep(PsState &s, float dt) {
  uint32_t ev = 0;
  s.t += dt;
  if (s.phase == PSP_LOAD || s.phase == PSP_END) return 0;
  if (s.phase == PSP_SNAP) {  // (the camera and the lens wait while the picture is held)
    if (s.t >= PS_SNAP_S) s.phase = PSP_PRINT, s.t = 0;
    return 0;
  }
  if (s.phase == PSP_PRINT) {
    if (s.t >= PS_PRINT_S) {
      s.shot++;
      if (s.shot >= PS_SHOTS) {
        s.shot = PS_SHOTS - 1;
        s.phase = PSP_END;
        s.t = 0;
        return PSE_END;
      }
      psBeginPlace(s);
      return PSE_LOAD;
    }
    return 0;
  }
  if (s.phase == PSP_AWAY) {
    if (s.t >= PS_AWAY_S) {
      s.shot++;
      if (s.shot >= PS_SHOTS) {
        s.shot = PS_SHOTS - 1;
        s.phase = PSP_END;
        s.t = 0;
        return PSE_END;
      }
      psBeginPlace(s);
      return PSE_LOAD;
    }
    return 0;
  }
  // the lens and the camera move on
  s.st += dt;
  s.ft += dt;
  if (s.ft < s.fdur) {
    s.f = s.fa + (s.fb - s.fa) * psEase(s.ft / s.fdur);
  } else {
    s.f = s.fb;
    if (s.fLock && s.ft - dt < s.fdur) ev |= PSE_LOCK;
    if (s.ft >= s.fdur + s.fhold) psSweep(s);
  }
  psDrift(s);
  if (s.phase == PSP_AIM || s.phase == PSP_OUT) {  // what it does while it's there
    s.actT += dt;
    if (s.actT >= s.actDur) {
      if (s.act == PSA_WALK) s.walkX = s.walkTo;
      psMonNext(s);
    }
  }
  if (s.phase == PSP_IN && s.t >= PS_IN_S) s.phase = PSP_AIM, s.t = 0;
  else if (s.phase == PSP_AIM && s.t >= s.stay) s.phase = PSP_OUT, s.t = 0;
  else if (s.phase == PSP_OUT && s.t >= PS_OUT_S) {
    s.phase = PSP_AWAY;
    s.t = 0;
    s.pts[s.shot] = 0;
    s.dexOf[s.shot] = s.dex;
    s.got[s.shot] = false;
    ev |= PSE_AWAY;
  }
  psMonMove(s);
  return ev;
}

// a tap: the photo, scored as it is this moment (while it's showing up too, a little early; while it's leaving, too
// late). On the print a tap sends the print off (after PS_PRINT_MIN_S)
inline uint32_t psSnap(PsState &s) {
  if (s.phase == PSP_PRINT) {
    if (s.t >= PS_PRINT_MIN_S) s.t = PS_PRINT_S;
    return 0;
  }
  if (s.phase != PSP_IN && s.phase != PSP_AIM && s.phase != PSP_OUT) return 0;
  s.snapErrF = psFocusErr(s);
  s.snapDist = psCentreDist(s);
  if (s.vis < 1) s.snapDist += (1 - s.vis) * PS_C_ZERO;  // half out of sight is half a photo
  s.focusPts = psFocusPts(s.snapErrF);
  s.centrePts = psCentrePts(s.snapDist);
  uint8_t p = (uint8_t)(s.focusPts + s.centrePts);
  s.pts[s.shot] = p;
  s.dexOf[s.shot] = s.dex;
  s.got[s.shot] = true;
  s.score += p;
  s.phase = PSP_SNAP;
  s.t = 0;
  return PSE_SNAP | (p >= PS_PERFECT ? PSE_PERFECT : p >= PS_GOOD ? PSE_GOOD : 0);
}

inline uint8_t psLastPts(const PsState &s) { return s.pts[s.shot]; }

// what a photo would score this moment
inline int psPtsNow(const PsState &s) { return psFocusPts(psFocusErr(s)) + psCentrePts(psCentreDist(s)); }
// PSBOT, and the tests' player: a moment worth a photo, as a patient player picks one: 95 at first, taking less as its
// time runs out (55 at the end), and the last moment before it goes
inline bool psBotWants(const PsState &s) {
  if (s.phase != PSP_AIM) return s.phase == PSP_PRINT && s.t >= PS_PRINT_MIN_S + 0.4f;
  return psPtsNow(s) >= 95 - 40 * s.t / s.stay || s.t > s.stay - 0.25f;
}
