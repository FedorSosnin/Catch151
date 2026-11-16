#pragma once
// Pinball, the seventh game: its rules and the ball's physics, with no drawing (pinball_v2.h draws it from this; the
// plan, with Fedor's choices, is docs/PINBALL.md; test/test_pinball.cpp checks it on a computer).
//
// The round screen is the table and its rim the wall. Gravity pulls the ball down toward two flippers at the bottom,
// over the drain between them. On the floor: an orbit lane round the top with six berry lights in it, three bumpers
// with Diglett's hole between them, your own Pokemon on a pad, slingshots over the flippers, and a patch of tall grass
// on the right whose gap takes the ball into a battle. Raised over the floor: a ramp up the left that becomes a wire
// rail down the rim, over the orbit lane, to the left inlane. A gentle table: the Knomi draws about 10 frames a
// second, so the ball's top speed is one the eye can follow there, and the physics runs in many small steps between
// frames so the ball never passes through a flipper.
#include <math.h>
#include <stdint.h>
#include <string.h>

// ---- the table (its drawing uses the same numbers) ----
#define PB_CX 233.0f
#define PB_CY 233.0f
#define PB_R 218            // the table's inside; the rim from here out
#define PB_BALL_R 10
#define PB_FLIP_LX 148      // the left flipper's pivot (the right one's mirrored about the middle)
#define PB_FLIP_Y 390
#define PB_FLIP_LEN 70
#define PB_FLIP_REST 0.50f  // its angle at rest (radians below level, the tip down toward the middle)
#define PB_FLIP_UP -0.42f   // raised
#define PB_ORBIT_R0 180     // the orbit lane: between its inner rail (this far out) and the wall,
#define PB_ORBIT_A0 200     // from here round the top to here (degrees, 0 to the right, y down: 270 is the top):
#define PB_ORBIT_A1 340     // its mouths open wide enough down each side for a shot up the side to go in
#define PB_WIRE_R 201       // the wire rail: down the left rim over the orbit lane, from where the ramp meets it
#define PB_WIRE_A0 228
#define PB_WIRE_A1 166      // ...to where it lets the ball down into the left inlane
#define PB_PET_FEET 296     // your Pokemon on its pad, in the middle
#define PB_GRASS_X 358      // the tall grass on the right
#define PB_GRASS_Y 246
#define PB_GRASS_RX 50
#define PB_GRASS_RY 36
#define PB_DIG_X 233        // Diglett's hole, between the three bumpers
#define PB_DIG_Y 178
#define PB_BUMP_R 23
// the way into the grass: a gap in its lower-left side, from its mouth (on the floor) to its end (in the grass)
static const float PB_NOTCH[2][2] = { { 314, 280 }, { 344, 254 } };
static const int16_t PB_BUMP[3][2] = { { 158, 176 }, { 308, 176 }, { 233, 104 } };  // a Poke, a Great, an Ultra Ball
static const int16_t PB_BERRY_A[6] = { 212, 234, 246, 294, 306, 328 };  // the berry lights, in the orbit lane
// the ramp: from its mouth left of the middle, up the left and over the orbit lane, where the wire rail takes over
static const float PB_RAMP[3][2] = { { 146, 266 }, { 104, 198 }, { 98, 84 } };  // start, the curve's pull, end
// the guides down to the flippers, from the wall (148 degrees round) to just over the pivot: the guide's top runs on
// smoothly over the flipper's round end (no dip between them for the ball to sit in)
#define PB_GUIDE_A 2.583f
static const float PB_GUIDE_END[2] = { 150.5f, 383.5f };  // the left one's; the right one's mirrored
// the slingshots: a triangle each side over the flippers, its long side (the first to the third corner) the rubber
// (high enough over the guide below it for the inlane: the ball rolls down between them to the flipper)
static const float PB_SLING[3][2] = { { 82, 272 }, { 82, 318 }, { 122, 340 } };  // the left one; the right mirrored

// ---- how it plays ----
#define PB_G 300.0f          // gravity, px/s^2
#define PB_VMAX 560.0f       // the ball's top speed, px/s
#define PB_STEPS 240         // physics steps a second
#define PB_FLIP_W_UP 18.0f   // a flipper's swing, rad/s: up
#define PB_FLIP_W_DOWN 10.0f // and back down
#define PB_E_WALL 0.55f      // how much of its speed into a wall the ball keeps
#define PB_E_FLIP 0.25f
#define PB_KICK_BUMP 260.0f  // a bumper's kick, px/s
#define PB_KICK_SLING 230.0f
#define PB_KICK_PET 200.0f
#define PB_BALLS 3
#define PB_LOST_S 1.6f       // a lost ball: this long before the next
#define PB_DIG_S 7.0f        // Diglett stays up this long
#define PB_CATCH_S 0.9f      // into the grass: the "!", then the battle
#define PB_BTL_S 6.0f        // the battle's time
#define PB_BTL_HIT_S 0.35f   // a move landing: the next tap waits for it
#define PB_BTL_OUT_S 1.4f    // after the battle, its outcome shown: then the ball comes out
#define PB_RAMP_IN 120.0f    // the speed up the ramp's way it needs to go in
#define PB_HINT_S 4.0f       // the first ball: the hint over the flippers this long
// the points (times the multiplier the berries give)
#define PB_PTS_BUMP 10
#define PB_PTS_SLING 5
#define PB_PTS_ORBIT 50
#define PB_PTS_PET 100
#define PB_PTS_PET3 500      // every third hit on your Pokemon
#define PB_PTS_RAMP 250
#define PB_PTS_BATTLE 500
#define PB_PTS_DIG 1000

enum PinPhase : uint8_t {
  PIN_PLAY,    // the ball on the table
  PIN_CATCH,   // into the grass: the "!"
  PIN_BATTLE,  // the battle (the sketch sets up the wild one: pinBattleStart())
  PIN_LOST,    // down the drain: the next ball soon
  PIN_END      // all three lost
};
// what happened in a step, for the sounds, buzzes and flashes (a bit each)
enum : uint32_t {
  PE_FLIP = 1u << 0, PE_BUMP = 1u << 1, PE_SLING = 1u << 2, PE_RAMP_IN = 1u << 3, PE_RAMP = 1u << 4,
  PE_DROP = 1u << 5, PE_ORBIT = 1u << 6, PE_BERRY = 1u << 7, PE_BERRIES = 1u << 8, PE_PET = 1u << 9,
  PE_PET3 = 1u << 10, PE_DIG_UP = 1u << 11, PE_DIG_HIT = 1u << 12, PE_DIG_GONE = 1u << 13, PE_GRASS = 1u << 14,
  PE_BATTLE = 1u << 15, PE_BTL_HIT = 1u << 16, PE_BTL_WON = 1u << 17, PE_BTL_OUT = 1u << 18, PE_LOST = 1u << 19,
  PE_BALL = 1u << 20, PE_END = 1u << 21, PE_NUDGE = 1u << 22,
};

struct PinState {
  PinPhase phase;
  float t;                      // time in this phase
  float playT;                  // time this ball has been in play
  bool day;                     // the look: a meadow on a wooden rim (else the night table on a chrome one)
  float x, y, vx, vy, roll;     // the ball; roll: how far it has rolled, in its radii (it's drawn turned by it)
  bool onTrack;                 // on the ramp or the wire rail:
  float s, sv;                  // how far along them, and how fast
  bool rampTop;                 // over the top of the ramp this time (its points given)
  float flipA[2], flipW[2];     // the flippers (0 left, 1 right): their angle and how fast it's changing
  bool hold[2];                 // held up (a finger on that half, the key, the button)
  uint32_t score;
  uint8_t mult;                 // the berries' multiplier: 1, then 2 and 3
  uint8_t ballsLeft;            // after this one
  uint8_t ball;                 // which ball this is (1..3)
  uint8_t bumpLit;              // the bumpers hit since Diglett last came up, a bit each
  float bumpFlash[3];
  uint8_t berryLit;             // the berry lights lit, a bit each
  float berryFlash;             // all six just lit: BERRIES x2!
  bool digUp;                   // Diglett up out of its hole,
  float digT, digHitT;          // for this long; knocked dizzy, this long ago
  uint8_t petHits;
  float petFlash;
  float grassT;                 // the grass rustling
  float stillT;                 // the ball nearly still this long (a nudge frees it)
  float orbitCool, lastAng;     // the orbit's top: its points once a pass; the ball's angle round the middle
  // the battle
  int16_t wild;                 // the wild one (0: not set up yet)
  uint16_t wildHp, wildMax;
  bool won;
  float btlLeft, hitT, outT;    // time left; a move landing; the outcome on show
  uint8_t lastMove;
  // the latest points, shown where they were made
  uint16_t popPts;
  float popX, popY, popT;
  uint32_t seed;
};

inline float pinRand(PinState &s) {
  s.seed = s.seed * 1664525u + 1013904223u;
  return (s.seed >> 8) / 16777216.0f;
}

// ---- the ramp and the wire rail, as one track ----
#define PB_RAMP_SEGS 24
#define PB_WIRE_SEGS 31
#define PB_TRACK_N (PB_RAMP_SEGS + PB_WIRE_SEGS + 1)
struct PinTrack {
  float x[PB_TRACK_N], y[PB_TRACK_N], s[PB_TRACK_N];
  float rampLen, len;
};
inline void pinRampAt(float t, float &x, float &y) {
  float u = 1 - t;
  x = u * u * PB_RAMP[0][0] + 2 * u * t * PB_RAMP[1][0] + t * t * PB_RAMP[2][0];
  y = u * u * PB_RAMP[0][1] + 2 * u * t * PB_RAMP[1][1] + t * t * PB_RAMP[2][1];
}
inline const PinTrack &pinTrack() {
  static PinTrack t;
  static bool built = false;
  if (built) return t;
  for (int i = 0; i <= PB_RAMP_SEGS; i++) pinRampAt(i / (float)PB_RAMP_SEGS, t.x[i], t.y[i]);
  for (int i = 1; i <= PB_WIRE_SEGS; i++) {  // the wire: round the rim, the angle going down from its top
    float a = (PB_WIRE_A0 - (PB_WIRE_A0 - PB_WIRE_A1) * i / (float)PB_WIRE_SEGS) * 0.01745329f;
    t.x[PB_RAMP_SEGS + i] = PB_CX + PB_WIRE_R * cosf(a);
    t.y[PB_RAMP_SEGS + i] = PB_CY + PB_WIRE_R * sinf(a);
  }
  t.s[0] = 0;
  for (int i = 1; i < PB_TRACK_N; i++) t.s[i] = t.s[i - 1] + hypotf(t.x[i] - t.x[i - 1], t.y[i] - t.y[i - 1]);
  t.rampLen = t.s[PB_RAMP_SEGS];
  t.len = t.s[PB_TRACK_N - 1];
  built = true;
  return t;
}
// where s along the track is, and its way there (a unit vector)
inline void pinTrackAt(float s, float &x, float &y, float &dx, float &dy) {
  const PinTrack &t = pinTrack();
  int i = 1;
  while (i < PB_TRACK_N - 1 && t.s[i] < s) i++;
  float seg = t.s[i] - t.s[i - 1], k = seg > 0 ? (s - t.s[i - 1]) / seg : 0;
  k = k < 0 ? 0 : k > 1 ? 1 : k;
  x = t.x[i - 1] + (t.x[i] - t.x[i - 1]) * k;
  y = t.y[i - 1] + (t.y[i] - t.y[i - 1]) * k;
  dx = seg > 0 ? (t.x[i] - t.x[i - 1]) / seg : 0;
  dy = seg > 0 ? (t.y[i] - t.y[i - 1]) / seg : 1;
}

// ---- points ----
inline void pinAdd(PinState &s, uint16_t pts, float x, float y) {
  uint32_t p = (uint32_t)pts * s.mult;
  s.score += p;
  s.popPts = (uint16_t)(p > 65535 ? 65535 : p);
  s.popX = x, s.popY = y, s.popT = 1.0f;
}

// a ball rolling in along the orbit at the top right, down toward the field
inline void pinServe(PinState &s) {
  const float a = 300 * 0.01745329f;
  s.x = PB_CX + PB_WIRE_R * cosf(a), s.y = PB_CY + PB_WIRE_R * sinf(a);
  s.vx = 0.866f * 200, s.vy = 0.5f * 200;
  s.onTrack = false;
  s.stillT = 0;
  s.playT = 0;
  s.lastAng = 300;
  s.phase = PIN_PLAY;
  s.t = 0;
}

inline void pinStart(PinState &s, uint32_t seed) {
  memset(&s, 0, sizeof(s));
  s.seed = seed ? seed : 1;
  pinRand(s);
  s.day = pinRand(s) < 0.5f;  // one look or the other, at random (Fedor: "Maybe its random which you get")
  s.flipA[0] = s.flipA[1] = PB_FLIP_REST;
  s.mult = 1;
  s.ballsLeft = PB_BALLS - 1;
  s.ball = 1;
  pinServe(s);
}

// a flipper held up or let go (side 0 left, 1 right): PE_FLIP as it goes up
inline uint32_t pinHold(PinState &s, int side, bool down) {
  bool was = s.hold[side];
  s.hold[side] = down;
  return down && !was ? PE_FLIP : 0;
}

// ---- collisions ----
// how much of its speed into a wall the ball keeps: e if it hit it, none if it was only sliding along it (rolling round
// the rim or the orbit's rail mustn't lose speed to a bounce every step)
inline float pinBounce(float vn, float e) { return vn < -60 ? e : 0; }
// the ball against a bar from a to b, rad round: pushed out of it, its speed into it turned back (e of it kept), and a
// kick off it if it came in fast enough. Returns how fast it came in (0: no touch)
inline float pinBar(PinState &s, float ax, float ay, float bx, float by, float rad, float e, float kick = 0) {
  float dx = bx - ax, dy = by - ay, L2 = dx * dx + dy * dy;
  float t = L2 > 0 ? ((s.x - ax) * dx + (s.y - ay) * dy) / L2 : 0;
  t = t < 0 ? 0 : t > 1 ? 1 : t;
  float cx = ax + t * dx, cy = ay + t * dy, ex = s.x - cx, ey = s.y - cy, d2 = ex * ex + ey * ey, lim = rad + PB_BALL_R;
  if (d2 >= lim * lim) return 0;
  float d = sqrtf(d2), nx, ny;
  if (d < 1e-3f) {
    float L = sqrtf(L2 > 0 ? L2 : 1);
    nx = L2 > 0 ? -dy / L : 0, ny = L2 > 0 ? dx / L : -1;
  } else {
    nx = ex / d, ny = ey / d;
  }
  s.x = cx + nx * lim, s.y = cy + ny * lim;
  float vn = s.vx * nx + s.vy * ny;
  if (vn >= 0) return 0.01f;
  e = pinBounce(vn, e);
  s.vx -= (1 + e) * vn * nx, s.vy -= (1 + e) * vn * ny;
  if (kick > 0 && vn < -20) s.vx += nx * kick, s.vy += ny * kick;
  return -vn;
}
// a flipper: the bar from its pivot to its tip, its rubber 11 round at the pivot and 6 at the tip, moving: the ball
// takes the speed of the point of it that it touches (a swing up throws it)
inline bool pinFlipper(PinState &s, int side) {
  float px = side ? 465 - PB_FLIP_LX : PB_FLIP_LX, a = s.flipA[side], dir = side ? -1.0f : 1.0f;
  float tx = px + dir * PB_FLIP_LEN * cosf(a), ty = PB_FLIP_Y + PB_FLIP_LEN * sinf(a);
  float dx = tx - px, dy = ty - PB_FLIP_Y, L2 = dx * dx + dy * dy;
  float t = ((s.x - px) * dx + (s.y - PB_FLIP_Y) * dy) / L2;
  t = t < 0 ? 0 : t > 1 ? 1 : t;
  float cx = px + t * dx, cy = PB_FLIP_Y + t * dy, ex = s.x - cx, ey = s.y - cy, d2 = ex * ex + ey * ey;
  float lim = 11 + (6 - 11) * t + PB_BALL_R;
  if (d2 >= lim * lim) return false;
  float d = sqrtf(d2), nx = d > 1e-3f ? ex / d : 0, ny = d > 1e-3f ? ey / d : -1;
  s.x = cx + nx * lim, s.y = cy + ny * lim;
  float W = side ? -s.flipW[side] : s.flipW[side];  // its turn on the screen (clockwise positive, y being down)
  float vsx = -W * (cy - PB_FLIP_Y), vsy = W * (cx - px);
  float rvx = s.vx - vsx, rvy = s.vy - vsy, vn = rvx * nx + rvy * ny;
  if (vn < 0) {
    rvx -= (1 + PB_E_FLIP) * vn * nx, rvy -= (1 + PB_E_FLIP) * vn * ny;
    s.vx = rvx + vsx, s.vy = rvy + vsy;
  }
  return true;
}
// the rim, a wall all round but at the bottom between the flippers, the drain
inline void pinRim(PinState &s) {
  if (s.y > PB_FLIP_Y + 10 && fabsf(s.x - PB_CX) < 60) return;
  float ex = s.x - PB_CX, ey = s.y - PB_CY, d = sqrtf(ex * ex + ey * ey), lim = PB_R - PB_BALL_R;
  if (d <= lim) return;
  float nx = ex / d, ny = ey / d;
  s.x = PB_CX + nx * lim, s.y = PB_CY + ny * lim;
  float vn = s.vx * nx + s.vy * ny;
  float e = pinBounce(-vn, PB_E_WALL);
  if (vn > 0) s.vx -= (1 + e) * vn * nx, s.vy -= (1 + e) * vn * ny;
}
// the orbit's inner rail: an arc, the ball kept off it on either side (its ends are round; the posts drawn on them
// are markers, not in the way: one sat in the lane's mouth and took half the ball's speed)
inline void pinOrbitRail(PinState &s) {
  float ex = s.x - PB_CX, ey = s.y - PB_CY, d = sqrtf(ex * ex + ey * ey);
  float a = atan2f(ey, ex) * 57.29578f;
  if (a < 0) a += 360;
  if (a > PB_ORBIT_A0 && a < PB_ORBIT_A1 && fabsf(d - PB_ORBIT_R0) < 4 + PB_BALL_R && d > 1) {
    float nx = ex / d, ny = ey / d, side = d > PB_ORBIT_R0 ? 1.0f : -1.0f, lim = PB_ORBIT_R0 + side * (4 + PB_BALL_R);
    s.x = PB_CX + nx * lim, s.y = PB_CY + ny * lim;
    float vn = (s.vx * nx + s.vy * ny) * side, e = pinBounce(vn, PB_E_WALL);
    if (vn < 0) s.vx -= (1 + e) * vn * nx * side, s.vy -= (1 + e) * vn * ny * side;
  }
  for (int e = 0; e < 2; e++) {  // its round ends
    float pa = (e ? PB_ORBIT_A1 : PB_ORBIT_A0) * 0.01745329f;
    float px = PB_CX + PB_ORBIT_R0 * cosf(pa), py = PB_CY + PB_ORBIT_R0 * sinf(pa);
    pinBar(s, px, py, px, py, 4, PB_E_WALL);
  }
}
// the distance from the ball to the bar from a to b
inline float pinDistTo(const PinState &s, float ax, float ay, float bx, float by) {
  float dx = bx - ax, dy = by - ay, L2 = dx * dx + dy * dy;
  float t = L2 > 0 ? ((s.x - ax) * dx + (s.y - ay) * dy) / L2 : 0;
  t = t < 0 ? 0 : t > 1 ? 1 : t;
  return hypotf(s.x - ax - t * dx, s.y - ay - t * dy);
}

// ---- the battle in the grass (the sketch sets up the wild one and works out each move's damage) ----
inline void pinBattleStart(PinState &s, int16_t dex, uint16_t hp) {
  s.wild = dex;
  s.wildHp = s.wildMax = hp < 1 ? 1 : hp;
  s.won = false;
  s.btlLeft = PB_BTL_S;
  s.hitT = s.outT = 0;
}
// a move used (a tap on that half, or the key): it lands, unless the last one is still landing. Returns the events
inline uint32_t pinBattleHit(PinState &s, uint8_t move, uint16_t dmg) {
  if (s.phase != PIN_BATTLE || !s.wildMax || s.won || s.btlLeft <= 0 || s.hitT > 0) return 0;
  s.lastMove = move;
  s.hitT = PB_BTL_HIT_S;
  s.wildHp = dmg >= s.wildHp ? 0 : s.wildHp - dmg;
  uint32_t ev = PE_BTL_HIT;
  if (!s.wildHp) {
    s.won = true;
    pinAdd(s, PB_PTS_BATTLE, PB_GRASS_X, PB_GRASS_Y - 50);
    ev |= PE_BTL_WON;
  }
  return ev;
}

// ---- a step ----
// the ball, a small step h along: gravity, then everything it can touch. Returns the events
inline uint32_t pinBallStep(PinState &s, float h) {
  uint32_t ev = 0;
  if (s.onTrack) {  // on the ramp or the wire: along them, gravity speeding it down and slowing it up
    float x, y, dx, dy;
    pinTrackAt(s.s, x, y, dx, dy);
    s.sv += PB_G * dy * h;
    s.sv -= s.sv * 0.2f * h;
    s.s += s.sv * h;
    const PinTrack &tr = pinTrack();
    if (!s.rampTop && s.s >= tr.rampLen) {
      s.rampTop = true;
      pinAdd(s, PB_PTS_RAMP, 150, 120);
      ev |= PE_RAMP;
    }
    if (s.s <= 0) {  // not fast enough: back down and out of its mouth
      float ax = PB_RAMP[0][0], ay = PB_RAMP[0][1], ux = PB_RAMP[1][0] - ax, uy = PB_RAMP[1][1] - ay, L = hypotf(ux, uy);
      s.onTrack = false;
      s.x = ax - ux / L * 6, s.y = ay - uy / L * 6;
      s.vx = -ux / L * fabsf(s.sv), s.vy = -uy / L * fabsf(s.sv);
      return ev;
    }
    if (s.s >= tr.len) {  // the wire's end: let down into the left inlane
      pinTrackAt(tr.len, x, y, dx, dy);
      s.onTrack = false;
      s.x = x + 4, s.y = y + 6;
      s.vx = dx * s.sv * 0.6f, s.vy = dy * s.sv * 0.6f;
      return ev | PE_DROP;
    }
    pinTrackAt(s.s, x, y, dx, dy);
    s.x = x, s.y = y, s.vx = dx * s.sv, s.vy = dy * s.sv;
    s.roll += fabsf(s.sv) * h / PB_BALL_R;
    return ev;
  }
  s.vy += PB_G * h;
  s.vx -= s.vx * 0.12f * h, s.vy -= s.vy * 0.12f * h;  // rolling, a little slower
  s.x += s.vx * h, s.y += s.vy * h;
  s.roll += hypotf(s.vx, s.vy) * h / PB_BALL_R;
  // into the grass's gap: caught (before it can bounce off the grass)
  if (pinDistTo(s, PB_NOTCH[0][0], PB_NOTCH[0][1], PB_NOTCH[1][0], PB_NOTCH[1][1]) < 13) {
    s.phase = PIN_CATCH;
    s.t = 0;
    s.grassT = PB_CATCH_S + 0.5f;
    s.vx = s.vy = 0;
    s.x = PB_NOTCH[1][0] + 12, s.y = PB_NOTCH[1][1] - 8;
    return ev | PE_GRASS;
  }
  // up the ramp's mouth, going its way fast enough
  {
    float ux = PB_RAMP[1][0] - PB_RAMP[0][0], uy = PB_RAMP[1][1] - PB_RAMP[0][1], L = hypotf(ux, uy);
    ux /= L, uy /= L;
    float along = s.vx * ux + s.vy * uy;
    if (hypotf(s.x - PB_RAMP[0][0], s.y - PB_RAMP[0][1]) < 18 && along > PB_RAMP_IN) {
      s.onTrack = true;
      s.s = 1, s.sv = along;
      s.rampTop = false;
      return ev | PE_RAMP_IN;
    }
  }
  pinRim(s);
  pinOrbitRail(s);
  for (int m = 0; m < 2; m++) {  // the guides down to the flippers
    float wx = PB_CX + PB_R * cosf(PB_GUIDE_A), wy = PB_CY + PB_R * sinf(PB_GUIDE_A), fx = PB_GUIDE_END[0];
    if (m) wx = 465 - wx, fx = 465 - fx;
    pinBar(s, wx, wy, fx, PB_GUIDE_END[1], 4, PB_E_WALL);
  }
  for (int m = 0; m < 2; m++) {  // the slingshots: their two plain sides, then the rubber that kicks
    float ax = PB_SLING[0][0], bx = PB_SLING[1][0], cx = PB_SLING[2][0];
    if (m) ax = 465 - ax, bx = 465 - bx, cx = 465 - cx;
    pinBar(s, ax, PB_SLING[0][1], bx, PB_SLING[1][1], 3, PB_E_WALL);
    pinBar(s, bx, PB_SLING[1][1], cx, PB_SLING[2][1], 3, PB_E_WALL);
    if (pinBar(s, ax, PB_SLING[0][1], cx, PB_SLING[2][1], 3, PB_E_WALL, PB_KICK_SLING) > 20) {
      pinAdd(s, PB_PTS_SLING, (ax + cx) / 2, PB_SLING[0][1] - 10);
      ev |= PE_SLING;
    }
  }
  for (int k = 0; k < 3; k++)  // the bumpers
    if (pinBar(s, PB_BUMP[k][0], PB_BUMP[k][1], PB_BUMP[k][0], PB_BUMP[k][1], PB_BUMP_R, 0.6f, PB_KICK_BUMP) > 20) {
      s.bumpFlash[k] = 0.25f;
      pinAdd(s, PB_PTS_BUMP, PB_BUMP[k][0], PB_BUMP[k][1] - 40);
      ev |= PE_BUMP;
      if (!s.digUp) {
        s.bumpLit |= 1 << k;
        if (s.bumpLit == 7) {  // all three: Diglett pops up between them
          s.bumpLit = 0;
          s.digUp = true;
          s.digT = PB_DIG_S;
          ev |= PE_DIG_UP;
        }
      }
    }
  if (s.digUp && pinBar(s, PB_DIG_X, PB_DIG_Y - 18, PB_DIG_X, PB_DIG_Y - 18, 14, 0.6f, 200) > 0.5f) {
    s.digUp = false;
    s.digHitT = 1.2f;
    pinAdd(s, PB_PTS_DIG, PB_DIG_X, PB_DIG_Y - 60);
    ev |= PE_DIG_HIT;
  }
  if (pinBar(s, PB_CX, PB_PET_FEET - 28, PB_CX, PB_PET_FEET - 28, 20, 0.5f, PB_KICK_PET) > 40) {  // your Pokemon
    s.petFlash = 0.4f;
    s.petHits++;
    pinAdd(s, PB_PTS_PET, PB_CX, PB_PET_FEET - 80);
    ev |= PE_PET;
    if (s.petHits % 3 == 0) {
      pinAdd(s, PB_PTS_PET3, PB_CX, PB_PET_FEET - 80);
      ev |= PE_PET3;
    }
  }
  pinBar(s, PB_GRASS_X + 2, PB_GRASS_Y - 2, PB_GRASS_X + 2, PB_GRASS_Y - 2, 38, PB_E_WALL);  // the grass's body
  bool cradled = pinFlipper(s, 0) && s.hold[0];  // (on a flipper held up: a player keeping it there, not stuck)
  cradled |= pinFlipper(s, 1) && s.hold[1];
  float v = hypotf(s.vx, s.vy);
  if (v > PB_VMAX) s.vx *= PB_VMAX / v, s.vy *= PB_VMAX / v;
  // round the orbit: over its top, and over the berry lights in it
  float ex = s.x - PB_CX, ey = s.y - PB_CY, d = sqrtf(ex * ex + ey * ey), a = atan2f(ey, ex) * 57.29578f;
  if (a < 0) a += 360;
  if (d > PB_ORBIT_R0 + 4) {
    if (s.orbitCool <= 0 && fabsf(a - s.lastAng) < 20 && ((s.lastAng < 270) != (a < 270))) {
      s.orbitCool = 0.6f;
      pinAdd(s, PB_PTS_ORBIT, PB_CX, 70);
      ev |= PE_ORBIT;
    }
    for (int k = 0; k < 6 && s.berryFlash <= 0; k++) {  // (just all lit: they start over a moment later)
      if (s.berryLit & (1 << k)) continue;
      float ba = PB_BERRY_A[k] * 0.01745329f;
      if (hypotf(s.x - (PB_CX + PB_WIRE_R * cosf(ba)), s.y - (PB_CY + PB_WIRE_R * sinf(ba))) < 14) {
        s.berryLit |= 1 << k;
        ev |= PE_BERRY;
        if (s.berryLit == 63) {  // all six: the multiplier up (to 3 at most), and they start over
          s.berryLit = 0;
          s.mult = s.mult < 3 ? s.mult + 1 : 3;
          s.berryFlash = 1.5f;
          ev |= PE_BERRIES;
        }
      }
    }
  }
  s.lastAng = a;
  // nearly still and not held on a flipper (stuck somewhere): a nudge frees it
  if (v < 15 && !cradled) s.stillT += h;
  else s.stillT = 0;
  if (s.stillT > 2.5f) {
    s.stillT = 0;
    s.vx += (pinRand(s) - 0.5f) * 160, s.vy -= 160;
    ev |= PE_NUDGE;
  }
  if (s.y > PB_FLIP_Y + 46) {  // down the drain
    s.phase = PIN_LOST;
    s.t = 0;
    s.digUp = false;
    ev |= PE_LOST;
  }
  return ev;
}

// the flippers swing toward up or rest, h along (resting while the ball's in a battle). In the ball's own small steps
// while it's in play, so a flipper never jumps past it between frames
inline void pinFlippers(PinState &s, float h) {
  for (int k = 0; k < 2; k++) {
    bool up = s.hold[k] && s.phase != PIN_BATTLE && s.phase != PIN_END;
    float target = up ? PB_FLIP_UP : PB_FLIP_REST, w = up ? PB_FLIP_W_UP : PB_FLIP_W_DOWN, a0 = s.flipA[k];
    float step = w * h;
    s.flipA[k] = fabsf(target - a0) <= step ? target : a0 + (target > a0 ? step : -step);
    s.flipW[k] = h > 0 ? (s.flipA[k] - a0) / h : 0;
  }
}

inline uint32_t pinStep(PinState &s, float dt) {
  uint32_t ev = 0;
  s.t += dt;
  if (s.phase != PIN_PLAY) pinFlippers(s, dt);
  auto down = [&](float &v) { v = v > dt ? v - dt : 0; };
  for (int k = 0; k < 3; k++) down(s.bumpFlash[k]);
  down(s.berryFlash), down(s.digHitT), down(s.petFlash), down(s.grassT), down(s.popT), down(s.orbitCool);
  switch (s.phase) {
    case PIN_END:
      return 0;
    case PIN_LOST:
      if (s.t < PB_LOST_S) return 0;
      if (!s.ballsLeft) {
        s.phase = PIN_END;
        s.t = 0;
        return PE_END;
      }
      s.ballsLeft--;
      s.ball++;
      pinServe(s);
      return PE_BALL;
    case PIN_CATCH:
      if (s.t < PB_CATCH_S) return 0;
      s.phase = PIN_BATTLE;
      s.t = 0;
      s.wild = 0, s.wildMax = 0;
      return PE_BATTLE;
    case PIN_BATTLE:
      if (!s.wildMax) return 0;  // (the sketch is setting it up)
      if (s.hitT > 0) s.hitT = s.hitT > dt ? s.hitT - dt : 0;
      if (!s.won && s.btlLeft > 0) s.btlLeft = s.btlLeft > dt ? s.btlLeft - dt : 0;
      if (s.won || s.btlLeft <= 0) s.outT += dt;
      if (s.outT < PB_BTL_OUT_S) return 0;
      // out it comes: shot from the grass's gap toward the left flipper
      s.phase = PIN_PLAY;
      s.t = 0;
      s.x = 306, s.y = 290;
      s.vx = -0.84f * 280, s.vy = 0.55f * 280;
      s.wild = 0, s.wildMax = 0;
      s.grassT = 0.8f;
      s.stillT = 0;
      return PE_BTL_OUT;
    case PIN_PLAY:
      break;
  }
  s.playT += dt;
  if (s.digUp) {
    s.digT -= dt;
    if (s.digT <= 0) {
      s.digUp = false;
      ev |= PE_DIG_GONE;
    }
  }
  int n = (int)ceilf(dt * PB_STEPS);
  if (n < 1) n = 1;
  if (n > 60) n = 60;  // (a long stall: no more than a quarter second's worth)
  float h = (n == 60 ? 0.25f : dt) / n;
  for (int i = 0; i < n; i++) {
    pinFlippers(s, h);
    if (s.phase == PIN_PLAY) ev |= pinBallStep(s, h);
  }
  return ev;
}

// what the end is worth to your Pokemon, as the other games give it: a point for every 400 (pet.exerciseReward())
inline uint8_t pinRewardPoints(uint32_t score) { return score / 400 > 255 ? 255 : (uint8_t)(score / 400); }
