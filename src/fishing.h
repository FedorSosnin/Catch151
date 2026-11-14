#pragma once
#include <math.h>
#include <stdint.h>
// Reel Fishing: rules without hardware (test/test_fishing.cpp). Logical pixels of the 466 canvas and seconds.
// docs/FISHING.md is the plan these follow. Seen from the side: the pier on the left, the water's surface at
// FISH_SURF_Y, blobs (fish, no shapes to give them away) under it. Five casts: cast by a blob, wait out the nibbles,
// tap when the float goes under, then tap at the fish's own pace to reel it in. Each one is weighed and let go; the
// score is the total weight. Nothing here depends on sound: every moment has its own look (fishing_v2.h).

#define FISH_CASTS 5
#define FISH_SURF_Y 208      // the water's surface
#define FISH_PIER_X 168      // the pier's end
#define FISH_CAST_X0 196     // where a cast can land: just past the pier...
#define FISH_CAST_X1 436     // ...to the far right
#define FISH_SHALLOW_X 262   // the water's three parts: the shallows to here, open water to FISH_DEEP_X, then the deep
#define FISH_DEEP_X 352
#define FISH_HOOK_DY 52      // the hook under the float
#define FISH_FLY_S 0.55f     // the float's flight
#define FISH_REACH 90.0f     // a fish this near the float (px, across) notices it
#define FISH_SLIP_S 1.5f     // the line slack (the notch under the green) this long and it slips off
#define FISH_WARN_S 0.4f     // a thrash is warned this long before it
#define FISH_LANDED_S 1.2f   // the leap
#define FISH_WEIGH_S 2.2f    // the weigh-in (a tap moves on after FISH_WEIGH_TAP_S)
#define FISH_WEIGH_TAP_S 0.6f
#define FISH_AWAY_S 1.6f     // got away, too slow
#define FISH_SOON_S 1.0f     // TOO SOON! shows this long; the cast goes on
#define FISH_RESPAWN_S 2.5f  // a fish gone (landed, scared, lost) is back as another of its size after this
#define FISH_GOAL_BRONZE 500   // the goal lines, in tenths of a kg (Fedor: 50, 150 and 300 kg)
#define FISH_GOAL_SILVER 1500
#define FISH_GOAL_GOLD 3000
#define FISH_SHINY_PCT 1     // as in the wild

enum FishClass : uint8_t { FC_SMALL, FC_MID, FC_BIG, FC_HEAVY, FC_GIANT, FC_N };

// How each weight fights (the plan's pace table): its pace (taps a second, one a beat of the fight song, which plays
// at pace x 60 bpm), how much a tap tightens the line (% of the LINE arc), its green band, the good taps that land it,
// the strike window, how often it thrashes and for how long, and its blob's size (px a unit)
struct FishClassDef {
  float pace, pull;
  uint8_t lo, hi, taps;
  float window, thrashEvery, thrashLen;
  uint8_t size;
};
static const FishClassDef FISH_CLASS[FC_N] = {
  { 3.2f, 10, 20, 80, 10, 0.90f, 5.0f, 0.6f, 3 },   // small, under 15 kg
  { 2.5f, 12, 25, 78, 12, 0.75f, 4.0f, 0.8f, 4 },   // middling, 15 to 50
  { 2.0f, 14, 30, 75, 14, 0.65f, 3.4f, 1.0f, 5 },   // big, 50 to 100
  { 1.6f, 16, 36, 72, 15, 0.60f, 3.0f, 1.2f, 6 },   // heavy, 100 to 150
  { 1.25f, 18, 42, 70, 16, 0.60f, 2.6f, 1.4f, 8 },  // giant, 200 and up
};
#define FISH_THRASH_PULL 0.6f  // a thrash tightens the line by this share of the ease a second: tap slower through it

// The fish: the Beach's water Pokemon (zones.h), their Pokedex weights (tenths of a kg), their size and where they
// swim (1 the shallows, 2 open water, 4 the deep)
struct FishKind {
  int16_t dex;
  uint16_t kg10;
  uint8_t cls, water;
};
static const FishKind FISH_KINDS[] = {
  { 90, 40, FC_SMALL, 1 },     // Shellder
  { 98, 65, FC_SMALL, 1 },     // Krabby
  { 116, 80, FC_SMALL, 3 },    // Horsea
  { 129, 100, FC_SMALL, 7 },   // Magikarp
  { 60, 124, FC_SMALL, 1 },    // Poliwag
  { 54, 196, FC_MID, 1 },      // Psyduck
  { 79, 360, FC_MID, 1 },      // Slowpoke
  { 118, 150, FC_MID, 2 },     // Goldeen
  { 61, 200, FC_MID, 2 },      // Poliwhirl
  { 117, 250, FC_MID, 2 },     // Seadra
  { 120, 345, FC_MID, 2 },     // Staryu
  { 119, 390, FC_MID, 2 },     // Seaking
  { 72, 455, FC_MID, 2 },      // Tentacool
  { 73, 550, FC_BIG, 2 },      // Tentacruel
  { 99, 600, FC_BIG, 2 },      // Kingler
  { 55, 766, FC_BIG, 4 },      // Golduck
  { 80, 785, FC_BIG, 4 },      // Slowbro
  { 121, 800, FC_BIG, 4 },     // Starmie
  { 86, 900, FC_BIG, 4 },      // Seel
  { 87, 1200, FC_HEAVY, 4 },   // Dewgong
  { 91, 1325, FC_HEAVY, 4 },   // Cloyster
  { 131, 2200, FC_GIANT, 4 },  // Lapras
  { 130, 2350, FC_GIANT, 4 },  // Gyarados
};
#define FISH_KIND_N ((int)(sizeof(FISH_KINDS) / sizeof(FISH_KINDS[0])))

// the water always holds these: 3 small, 2 middling, 1 big (or now and then a heavy one), and the giant's place,
// filled for one cast in every game (casts 3 to 5)
#define FISH_BLOBS 7
#define FISH_GIANT 6
static const uint8_t FISH_BLOB_CLASS[FISH_BLOBS] = { FC_SMALL, FC_SMALL, FC_SMALL, FC_MID, FC_MID, FC_BIG, FC_GIANT };
// where each size swims (x from, to; its depth from, to)
static const int16_t FISH_SWIM[FC_N][4] = {
  { 204, 340, 252, 290 }, { 214, 392, 270, 318 }, { 300, 430, 296, 342 }, { 350, 432, 318, 358 }, { 372, 426, 338, 366 } };

struct FishBlob {
  bool on;
  uint8_t cls, kind;
  float x, y, vx, r;   // r: 0..1, how big it is for its kind (heavier and a bigger blob)
  uint16_t kg10;
  bool shiny;
  float gone;          // > 0: away (landed, scared, lost), back as another of its size when it runs out
  float fleeV;         // swimming off fast (scared, lost)
};

enum FishPhase : uint8_t {
  FP_AIM,     // waiting for a cast
  FP_FLY,     // the float on its way
  FP_WAIT,    // out, a fish coming round; nibbles
  FP_BITE,    // pulled under: the strike window
  FP_REEL,    // hooked: tap at its pace
  FP_LANDED,  // it leaps out
  FP_WEIGH,   // on the scale
  FP_AWAY,    // got away (snapped, slipped off) or too slow to strike
  FP_END      // the five are done
};
enum FishWhy : uint8_t { FW_NONE, FW_SNAP, FW_SLIP, FW_SLOW };

// what happened in a step, for the sounds, buzzes and flashes (a bit each)
enum : uint32_t {
  FE_CAST = 1u << 0, FE_SPLASH = 1u << 1, FE_NIBBLE = 1u << 2, FE_BITE = 1u << 3, FE_HOOK = 1u << 4,
  FE_TOO_SOON = 1u << 5, FE_TOO_SLOW = 1u << 6, FE_REEL = 1u << 7, FE_WARN = 1u << 8, FE_THRASH = 1u << 9,
  FE_RED = 1u << 10, FE_BLUE = 1u << 11, FE_SNAP = 1u << 12, FE_SLIP = 1u << 13, FE_LANDED = 1u << 14,
  FE_WEIGH = 1u << 15, FE_NEXT = 1u << 16, FE_END = 1u << 17, FE_GIANT = 1u << 18, FE_BEAT = 1u << 19,
};

struct FishState {
  FishPhase phase;
  float t;                 // time in this phase
  uint8_t cast;            // 0..4: this cast
  int16_t slot[FISH_CASTS];  // what each cast landed: its dex, -1 got away, 0 not yet
  uint16_t slotKg10[FISH_CASTS];
  bool slotShiny[FISH_CASTS];
  uint32_t total;          // tenths of a kg
  FishBlob b[FISH_BLOBS];
  int8_t eager;            // the blob with bubbles over it (about to bite: it bites sooner), -1 none
  float eagerT;
  float aimX;              // the aim ring (tilt or the arrow keys move it; the button casts there)
  float floatX, fromX;     // the float, and where it flew from
  int8_t target;           // the blob after the hook (waiting) or on it (hooked)
  float biteAt;            // waiting: when it bites
  float nibbleAt[3];
  uint8_t nibbles, nibbled;
  float lastNibble;        // when the last nibble came (for its dip)
  float soonT;             // TOO SOON! on show (counting down)
  // the fight
  float T;                 // the line's tension, 0..100 (the LINE arc)
  uint8_t good;            // good taps so far
  bool clean;              // struck in the window's first third
  float thrashIn, thrashLeft, warnLeft;
  float slackT;            // time under the green band
  float beat;              // 0..1 through the fish's beat
  float x0, y0;            // where it was hooked
  uint8_t zone;            // the LINE arc's zone now: 0 blue, 1 green, 2 red
  FishWhy why;
  int8_t giantCast;        // the cast the giant comes for (2..4, counted from 0)
  bool giantLanded;
  uint32_t rng;
};

inline uint32_t fishRand(FishState &s) {
  uint32_t x = s.rng ? s.rng : 1;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return s.rng = x;
}
inline float fishRandF(FishState &s) { return (fishRand(s) >> 8) / 16777216.0f; }  // 0..1
inline float fishRange(FishState &s, float a, float b) { return a + (b - a) * fishRandF(s); }

inline uint8_t fishGoal(uint32_t kg10) {  // 0 none, 1 bronze, 2 silver, 3 gold
  return kg10 >= FISH_GOAL_GOLD ? 3 : kg10 >= FISH_GOAL_SILVER ? 2 : kg10 >= FISH_GOAL_BRONZE ? 1 : 0;
}
inline uint8_t fishWaterAt(float x) { return x < FISH_SHALLOW_X ? 1 : x < FISH_DEEP_X ? 2 : 4; }
// how big its blob is drawn (px a unit), bigger for a heavier one of its kind
inline float fishBlobSize(const FishBlob &b) { return FISH_CLASS[b.cls].size * (0.9f + 0.2f * b.r); }
inline float fishEase(uint8_t cls) { return FISH_CLASS[cls].pull * FISH_CLASS[cls].pace; }  // % a second

// a new fish in blob i: its size's, somewhere in its water, a kind that swims there
inline void fishSpawn(FishState &s, int i) {
  FishBlob &b = s.b[i];
  uint8_t cls = FISH_BLOB_CLASS[i];
  if (i == 5 && fishRand(s) % 3 == 0) cls = FC_HEAVY;  // the big one's place: now and then a heavy one
  const int16_t *w = FISH_SWIM[cls];
  float x = fishRange(s, w[0], w[1]);
  for (int tries = 0; tries < 6; tries++) {  // not on top of another
    bool clear = true;
    for (int j = 0; j < FISH_BLOBS; j++)
      if (j != i && s.b[j].on && s.b[j].gone <= 0 && fabsf(s.b[j].x - x) < 34) clear = false;
    if (clear) break;
    x = fishRange(s, w[0], w[1]);
  }
  uint8_t water = fishWaterAt(x), pick[FISH_KIND_N], n = 0;
  for (int k = 0; k < FISH_KIND_N; k++)
    if (FISH_KINDS[k].cls == cls && (FISH_KINDS[k].water & water)) pick[n++] = (uint8_t)k;
  if (!n)
    for (int k = 0; k < FISH_KIND_N; k++)
      if (FISH_KINDS[k].cls == cls) pick[n++] = (uint8_t)k;
  b.on = true;
  b.cls = cls;
  b.kind = pick[fishRand(s) % n];
  b.x = x;
  b.y = fishRange(s, w[2], w[3]);
  b.vx = fishRange(s, 6, 14) * (fishRand(s) & 1 ? 1 : -1);
  b.r = fishRandF(s);
  b.kg10 = (uint16_t)(FISH_KINDS[b.kind].kg10 * (80 + 40 * b.r) / 100);
  b.shiny = fishRand(s) % 100 < FISH_SHINY_PCT;
  b.gone = 0;
  b.fleeV = 0;
}

// a fish gone: swimming off (fleeing) or simply not there, back as another of its size after a while
inline void fishGone(FishState &s, int i, bool flee) {
  FishBlob &b = s.b[i];
  b.gone = i == FISH_GIANT ? 1e9f : FISH_RESPAWN_S;  // the giant comes once
  b.fleeV = flee ? 260.0f : 0;  // off out to sea
  if (i == FISH_GIANT && !flee) b.on = false;
  if (s.eager == i) s.eager = -1;
}

// waiting: the fish nearest the float (not one gone) comes round to the hook; nearer and with bubbles over it, sooner
inline void fishPickTarget(FishState &s, float extra) {
  int best = -1;
  float bd = 1e9f;
  for (int i = 0; i < FISH_BLOBS; i++) {
    const FishBlob &b = s.b[i];
    if (!b.on || b.gone > 0) continue;
    float d = fabsf(b.x - s.floatX);
    if (d < bd) bd = d, best = i;
  }
  s.target = (int8_t)best;
  float wait = fishRange(s, 1.6f, 4.0f) + (bd > FISH_REACH ? (bd - FISH_REACH) / 40.0f + 1.0f : bd / 60.0f) + extra;
  if (best >= 0 && best == s.eager) wait *= 0.5f;
  if (best < 0) wait = 2.0f;  // nobody about yet: look again soon
  s.biteAt = s.t + wait;
  // up to three nibbles first, the last at least 0.7 s before the bite
  s.nibbles = best >= 0 ? (uint8_t)(fishRand(s) % 4) : 0;
  float room = wait - 0.9f - 0.7f;
  if (room < 0.6f) s.nibbles = 0;
  else if (s.nibbles * 0.9f > room) s.nibbles = (uint8_t)(room / 0.9f);
  for (int k = 0; k < s.nibbles; k++) s.nibbleAt[k] = s.t + 0.9f + room * (k + fishRange(s, .2f, .8f)) / s.nibbles;
  s.nibbled = 0;
}

inline void fishStart(FishState &s, uint32_t seed) {
  s = FishState();
  s.rng = seed ? seed : 0x9E3779B9u;
  for (int i = 0; i < FISH_BLOBS; i++) {
    s.b[i].on = false;
    if (i != FISH_GIANT) fishSpawn(s, i);
  }
  s.giantCast = (int8_t)(2 + fishRand(s) % 3);
  s.eager = -1;
  s.aimX = 300;
  s.target = -1;
  s.phase = FP_AIM;
}

inline void fishPhase(FishState &s, FishPhase p) {
  s.phase = p;
  s.t = 0;
}

// on to the next cast, or the end; the giant comes in for its cast
inline uint32_t fishNextCast(FishState &s) {
  if (s.cast + 1 >= FISH_CASTS) {
    fishPhase(s, FP_END);
    return FE_END;
  }
  s.cast++;
  s.target = -1;
  fishPhase(s, FP_AIM);
  uint32_t ev = FE_NEXT;
  if (s.cast == s.giantCast && !s.b[FISH_GIANT].on) {
    fishSpawn(s, FISH_GIANT);
    ev |= FE_GIANT;
  }
  return ev;
}

// a cast to x (a tap on the water; the button and the keys cast at the aim ring)
inline uint32_t fishCastAt(FishState &s, float x) {
  if (s.phase != FP_AIM) return 0;
  s.floatX = x < FISH_CAST_X0 ? FISH_CAST_X0 : x > FISH_CAST_X1 ? FISH_CAST_X1 : x;
  s.aimX = s.floatX;
  s.fromX = FISH_PIER_X + 84;
  fishPhase(s, FP_FLY);
  return FE_CAST;
}

// the line's zone for a tension: 0 too slack (blue), 1 the green, 2 too tight (red)
inline uint8_t fishZone(const FishState &s, float T) {
  const FishClassDef &c = FISH_CLASS[s.b[s.target >= 0 ? s.target : 0].cls];
  return T < c.lo ? 0 : T > c.hi ? 2 : 1;
}

inline void fishHook(FishState &s) {
  FishBlob &b = s.b[s.target];
  const FishClassDef &c = FISH_CLASS[b.cls];
  s.clean = s.t < c.window / 3;
  // clean: half a pull under the middle, so each tap at the pace lifts it to half a pull over and the beat's swing
  // sits in the green (the giant's pull is more than half its narrow band); late: near the slack edge
  s.T = s.clean ? (c.lo + c.hi - c.pull) * 0.5f : c.lo + 4.0f;
  s.good = 0;
  s.slackT = 0;
  s.beat = 0;
  s.thrashIn = c.thrashEvery * fishRange(s, .8f, 1.2f);
  s.thrashLeft = s.warnLeft = 0;
  s.x0 = b.x = s.floatX;
  s.y0 = b.y = FISH_SURF_Y + FISH_HOOK_DY + 20;
  s.zone = fishZone(s, s.T);
  if (s.eager == s.target) s.eager = -1;
  fishPhase(s, FP_REEL);
}

// a tap anywhere, the Knomi's button or Space: cast at the aim ring, strike, reel, or move on from the weigh-in
inline uint32_t fishTap(FishState &s) {
  switch (s.phase) {
    case FP_AIM: return fishCastAt(s, s.aimX);
    case FP_WAIT: {  // too soon: that fish is scared off, the cast goes on with the next (Fedor: "Scare the fish off")
      if (s.target < 0 || s.b[s.target].gone > 0) return 0;  // (nobody at the hook yet)
      fishGone(s, s.target, true);
      s.soonT = FISH_SOON_S;
      fishPickTarget(s, 1.5f);
      return FE_TOO_SOON;
    }
    case FP_BITE: fishHook(s); return FE_HOOK;
    case FP_REEL: {
      const FishClassDef &c = FISH_CLASS[s.b[s.target].cls];
      s.T += c.pull;
      uint32_t ev = FE_REEL;
      if (s.T >= 100) {  // snapped
        s.T = 100;
        s.why = FW_SNAP;
        fishGone(s, s.target, true);
        fishPhase(s, FP_AWAY);
        s.slot[s.cast] = -1;
        return ev | FE_SNAP;
      }
      if (s.T >= c.lo && s.T <= c.hi) s.good++;
      uint8_t z = fishZone(s, s.T);
      if (z == 2 && s.zone != 2) ev |= FE_RED;
      s.zone = z;
      if (z != 0) s.slackT = 0;
      if (s.good >= c.taps) {  // landed
        FishBlob &b = s.b[s.target];
        s.slot[s.cast] = FISH_KINDS[b.kind].dex;
        s.slotKg10[s.cast] = b.kg10;
        s.slotShiny[s.cast] = b.shiny;
        s.total += b.kg10;
        if (s.target == FISH_GIANT) s.giantLanded = true;
        fishPhase(s, FP_LANDED);
        ev |= FE_LANDED;
      }
      return ev;
    }
    case FP_WEIGH:
      if (s.t >= FISH_WEIGH_TAP_S) {
        fishGone(s, s.target, false);
        return fishNextCast(s);
      }
      return 0;
    default: return 0;
  }
}

// the game moves on by dt seconds
inline uint32_t fishStep(FishState &s, float dt) {
  uint32_t ev = 0;
  s.t += dt;
  if (s.soonT > 0) s.soonT -= dt;
  // the water: fish drift to and fro in their water, the gone swim off and come back as others
  for (int i = 0; i < FISH_BLOBS; i++) {
    FishBlob &b = s.b[i];
    if (!b.on) continue;
    if (b.gone > 0) {
      b.x += b.fleeV * dt;
      if (b.x > 520) {
        b.fleeV = 0;
        if (i == FISH_GIANT) b.on = false;
      }
      b.gone -= dt;
      if (b.gone <= 0) fishSpawn(s, i);
      continue;
    }
    if (i == s.target && (s.phase == FP_REEL || s.phase == FP_LANDED || s.phase == FP_WEIGH)) continue;
    if (i == s.target && (s.phase == FP_WAIT || s.phase == FP_BITE)) {  // coming round to the hook
      float hx = s.floatX + (b.x < s.floatX ? -10.0f : 10.0f), hy = FISH_SURF_Y + FISH_HOOK_DY + 14;
      b.x += (hx - b.x) * (1 - expf(-dt * 0.9f));
      b.y += (hy - b.y) * (1 - expf(-dt * 0.9f));
      continue;
    }
    const int16_t *w = FISH_SWIM[b.cls];
    b.x += b.vx * dt;
    if ((b.x < w[0] && b.vx < 0) || (b.x > w[1] && b.vx > 0)) b.vx = -b.vx;
  }
  s.eagerT -= dt;  // bubbles over one fish now and then: it's about to bite
  if (s.eagerT <= 0) {
    s.eagerT = fishRange(s, 5, 8);
    int i = (int)(fishRand(s) % FISH_BLOBS);
    s.eager = s.b[i].on && s.b[i].gone <= 0 && i != s.target ? (int8_t)i : -1;
  }
  switch (s.phase) {
    case FP_FLY:
      if (s.t >= FISH_FLY_S) {
        fishPhase(s, FP_WAIT);
        fishPickTarget(s, 0);
        ev |= FE_SPLASH;
      }
      break;
    case FP_WAIT:
      if (s.target < 0 || s.b[s.target].gone > 0) {  // nobody about: look again
        if (s.t >= s.biteAt) fishPickTarget(s, 0);
        break;
      }
      if (s.nibbled < s.nibbles && s.t >= s.nibbleAt[s.nibbled]) {
        s.nibbled++;
        s.lastNibble = s.t;
        ev |= FE_NIBBLE;
      }
      if (s.t >= s.biteAt) {
        fishPhase(s, FP_BITE);
        ev |= FE_BITE;
      }
      break;
    case FP_BITE:
      if (s.t >= FISH_CLASS[s.b[s.target].cls].window) {  // too slow: it took the bait and went
        s.why = FW_SLOW;
        s.slot[s.cast] = -1;
        fishGone(s, s.target, true);
        fishPhase(s, FP_AWAY);
        ev |= FE_TOO_SLOW;
      }
      break;
    case FP_REEL: {
      FishBlob &b = s.b[s.target];
      const FishClassDef &c = FISH_CLASS[b.cls];
      float ease = fishEase(b.cls);
      s.beat += dt * c.pace;
      if (s.beat >= 1) {
        s.beat -= 1;
        ev |= FE_BEAT;
      }
      if (s.warnLeft > 0) {
        s.warnLeft -= dt;
        if (s.warnLeft <= 0) {
          s.thrashLeft = c.thrashLen * fishRange(s, .8f, 1.2f);
          ev |= FE_THRASH;
        }
      } else if (s.thrashLeft > 0) {
        s.thrashLeft -= dt;
        if (s.thrashLeft <= 0) s.thrashIn = c.thrashEvery * fishRange(s, .8f, 1.2f);
      } else {
        s.thrashIn -= dt;
        if (s.thrashIn <= 0) {
          s.warnLeft = FISH_WARN_S;
          ev |= FE_WARN;
        }
      }
      s.T -= ease * dt;
      if (s.thrashLeft > 0) s.T += ease * FISH_THRASH_PULL * dt;
      if (s.T < 0) s.T = 0;
      if (s.T >= 100) {  // a thrash on a line already too tight
        s.T = 100;
        s.why = FW_SNAP;
        s.slot[s.cast] = -1;
        fishGone(s, s.target, true);
        fishPhase(s, FP_AWAY);
        ev |= FE_SNAP;
        break;
      }
      uint8_t z = fishZone(s, s.T);
      if (z == 2 && s.zone != 2) ev |= FE_RED;
      if (z == 0 && s.zone != 0) ev |= FE_BLUE;
      s.zone = z;
      s.slackT = z == 0 ? s.slackT + dt : 0;
      if (s.slackT >= FISH_SLIP_S) {  // too slow: it slipped off
        s.why = FW_SLIP;
        s.slot[s.cast] = -1;
        fishGone(s, s.target, true);
        fishPhase(s, FP_AWAY);
        ev |= FE_SLIP;
        break;
      }
      // reeled in toward the pier as the good taps add up, rising toward the surface at the end
      float p = (float)s.good / c.taps;
      b.x = s.x0 + (FISH_PIER_X + 40 - s.x0) * p + (s.thrashLeft > 0 ? sinf(s.t * 40) * 3 : 0);
      b.y = s.y0 + (FISH_SURF_Y + 30 - s.y0) * p * p;
      break;
    }
    case FP_LANDED:
      if (s.t >= FISH_LANDED_S) {
        fishPhase(s, FP_WEIGH);
        ev |= FE_WEIGH;
      }
      break;
    case FP_WEIGH:
      if (s.t >= FISH_WEIGH_S) {
        fishGone(s, s.target, false);
        ev |= fishNextCast(s);
      }
      break;
    case FP_AWAY:
      if (s.t >= FISH_AWAY_S) ev |= fishNextCast(s);
      break;
    default: break;
  }
  return ev;
}

// what the hooked (or coming) fish is, for the screens
inline const FishKind &fishKindOf(const FishState &s, int i) { return FISH_KINDS[s.b[i].kind]; }
