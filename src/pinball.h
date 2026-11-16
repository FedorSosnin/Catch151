#pragma once
// Pinball, the seventh game: its rules and the ball's physics, with no drawing (pinball_v2.h draws it from this; the
// plan, with Fedor's choices, is docs/PINBALL.md; test/test_pinball.cpp checks it on a computer).
//
// The round screen is the table and its rim the wall. Gravity pulls the ball down toward two flippers at the bottom,
// over the drain between them. On the floor: an orbit lane round the top with six berry lights in it, three bumpers
// with Diglett's hole between them, slingshots over the flippers, a ring painted in the middle, and a patch of tall
// grass by the right wall: the ball in its gap brings a wild Pokemon out into the ring, to battle there with the ball
// (Fedor: "The battles need to happen inside the pinball game"). Raised over the floor: a ramp straight up the left into
// the top-left corner that turns over into the wall and becomes a wire rail down the rim, over the orbit lane, to the
// left inlane. A gentle table: the Knomi draws about 10 frames a second, so the ball's top speed is one the eye can follow there, and the physics runs in many small steps between
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
#define PB_RING_X 233       // the ring in the middle, where a wild one stands to battle (its feet)
#define PB_RING_Y 278
#define PB_GRASS_X 396      // the tall grass by the right wall: small, so the way up the right side stays open (Fedor:
#define PB_GRASS_Y 236      // "very hard to get to the upper right size of the screen since the island blocks it")
#define PB_GRASS_RX 28
#define PB_GRASS_RY 30
#define PB_DIG_X 233        // Diglett's hole, between the three bumpers
#define PB_DIG_Y 178
#define PB_BUMP_R 23
// the way into the grass: a gap in its lower-left side, from its mouth (on the floor) to its end (in the grass)
static const float PB_NOTCH[2][2] = { { 366, 264 }, { 388, 244 } };
static const int16_t PB_BUMP[3][2] = { { 178, 180 }, { 288, 180 }, { 233, 104 } };  // a Poke, a Great, an Ultra Ball
static const int16_t PB_BERRY_A[6] = { 212, 234, 246, 294, 306, 328 };  // the berry lights, in the orbit lane
// the ramp (Fedor chose it from four, "C", then "slightly away from that one pin"): from its mouth left of the middle
// straight up into the top-left corner, clear of the Poke Ball bumper, then round over the top (a turn, not a corner:
// "The ramp's sharp angle makes it feel like there is a physics issue") into the wall, where the wire rail takes over,
// over the orbit lane. The turn is a circle touching the wall from inside at PB_RAMP_JOIN degrees round
static const float PB_RAMP_MOUTH[2] = { 142, 272 };
static const float PB_RAMP_TOP[2] = { 114.0f, 113.2f };          // where the climb ends and the turn starts
static const float PB_RAMP_ARC[3] = { 94.35f, 116.65f, 20.0f };  // the turn: its middle, and its radius
#define PB_RAMP_JOIN 220.0f
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
#define PB_KICK_WILD 220.0f
#define PB_BALLS 3
#define PB_LOST_S 1.6f       // a lost ball: this long before the next
#define PB_DIG_S 12.0f       // Diglett stays up this long (it was 7: Fedor, "too quick to leave")
#define PB_CATCH_S 0.9f      // into the grass: the "!", then the wild one comes out and the ball with it
#define PB_BTL_S 20.0f       // the battle's time, to knock it out with the ball
#define PB_BTL_IN_S 0.7f     // its jump from the grass to the ring
#define PB_BTL_HIT_S 0.4f    // a hit on it: its flash, and your move's
#define PB_BTL_OUT_S 1.4f    // its outcome on show (knocked out, or off back into the grass)
#define PB_RAMP_IN 120.0f    // the speed up the ramp's way it needs to go in
#define PB_RAMP_CATCH 22.0f  // ...and how near its mouth (as many shots go in as before it climbed higher)
#define PB_HINT_S 4.0f       // the first ball: the hint over the flippers this long
// the points (times the multiplier the berries give)
#define PB_PTS_BUMP 10
#define PB_PTS_SLING 5
#define PB_PTS_ORBIT 50
#define PB_PTS_BTL_HIT 50    // a hit on the wild one
#define PB_PTS_RAMP 250
#define PB_PTS_BATTLE 500
#define PB_PTS_DIG 1000

enum PinPhase : uint8_t {
  PIN_PLAY,    // the ball on the table (a battle may be on in the ring meanwhile)
  PIN_CATCH,   // into the grass: the "!", then out comes a wild one (the sketch sets it up: pinBattleStart())
  PIN_LOST,    // down the drain: the next ball soon
  PIN_END      // all three lost
};
// what happened in a step, for the sounds, buzzes and flashes (a bit each)
enum : uint32_t {
  PE_FLIP = 1u << 0, PE_BUMP = 1u << 1, PE_SLING = 1u << 2, PE_RAMP_IN = 1u << 3, PE_RAMP = 1u << 4,
  PE_DROP = 1u << 5, PE_ORBIT = 1u << 6, PE_BERRY = 1u << 7, PE_BERRIES = 1u << 8, PE_DIG_UP = 1u << 11,
  PE_DIG_HIT = 1u << 12, PE_DIG_GONE = 1u << 13, PE_GRASS = 1u << 14, PE_BATTLE = 1u << 15, PE_BTL_HIT = 1u << 16,
  PE_BTL_WON = 1u << 17, PE_BTL_GONE = 1u << 18, PE_LOST = 1u << 19, PE_BALL = 1u << 20, PE_END = 1u << 21,
  PE_NUDGE = 1u << 22,
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
  float grassT;                 // the grass rustling
  float stillT;                 // the ball nearly still this long (a nudge frees it)
  float orbitCool, lastAng;     // the orbit's top: its points once a pass; the ball's angle round the middle
  // the battle in the ring
  bool btlOn;                   // a wild one in the ring
  int16_t wild;                 // which
  uint16_t wildHp, wildMax, dmg;  // its HP; what a hit on it takes off (your move's damage, the sketch works it out)
  uint8_t move;                 // that move (MOVE_TBL's), for what it says
  bool won;
  float btlLeft, inT, hitT, outT;  // time left; its jump in; a hit's flash; its outcome on show (0: none yet)
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
#define PB_RAMP_TURN_SEGS 6  // ...of them its turn over the top
#define PB_WIRE_SEGS 31
#define PB_TRACK_N (PB_RAMP_SEGS + PB_WIRE_SEGS + 1)
struct PinTrack {
  float x[PB_TRACK_N], y[PB_TRACK_N], s[PB_TRACK_N];
  float rampLen, len;
};
inline const PinTrack &pinTrack() {
  static PinTrack t;
  static bool built = false;
  if (built) return t;
  // the ramp: the climb (most of its steps, it being most of its length), then the turn, from where the climb meets it
  // round to the wall (y down: the angle going down), each step about as long
  const int climb = PB_RAMP_SEGS - PB_RAMP_TURN_SEGS;
  for (int i = 0; i <= climb; i++) {
    float k = i / (float)climb;
    t.x[i] = PB_RAMP_MOUTH[0] + (PB_RAMP_TOP[0] - PB_RAMP_MOUTH[0]) * k;
    t.y[i] = PB_RAMP_MOUTH[1] + (PB_RAMP_TOP[1] - PB_RAMP_MOUTH[1]) * k;
  }
  float a0 = atan2f(PB_RAMP_TOP[1] - PB_RAMP_ARC[1], PB_RAMP_TOP[0] - PB_RAMP_ARC[0]), a1 = PB_RAMP_JOIN * 0.01745329f;
  while (a1 > a0) a1 -= 6.2831853f;
  for (int i = 1; i <= PB_RAMP_TURN_SEGS; i++) {
    float a = a0 + (a1 - a0) * i / PB_RAMP_TURN_SEGS;
    t.x[climb + i] = PB_RAMP_ARC[0] + PB_RAMP_ARC[2] * cosf(a);
    t.y[climb + i] = PB_RAMP_ARC[1] + PB_RAMP_ARC[2] * sinf(a);
  }
  // the wire: round the rim and down, from where the ramp meets the wall (its angle and how far out) to its own end
  float ex = t.x[PB_RAMP_SEGS] - PB_CX, ey = t.y[PB_RAMP_SEGS] - PB_CY;
  float w0 = atan2f(ey, ex) * 57.29578f, r0 = sqrtf(ex * ex + ey * ey);
  if (w0 < 0) w0 += 360;
  for (int i = 1; i <= PB_WIRE_SEGS; i++) {
    float k = i / (float)PB_WIRE_SEGS, a = (w0 + (PB_WIRE_A1 - w0) * k) * 0.01745329f, r = r0 + (PB_WIRE_R - r0) * k;
    t.x[PB_RAMP_SEGS + i] = PB_CX + r * cosf(a);
    t.y[PB_RAMP_SEGS + i] = PB_CY + r * sinf(a);
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

// ---- the battle in the ring ----
// a wild one out of the grass, its HP, and what each hit of the ball takes off (your Pokemon's move, worked out by the
// sketch as a battle would: move is MOVE_TBL's)
inline void pinBattleStart(PinState &s, int16_t dex, uint16_t hp, uint16_t dmg, uint8_t move) {
  s.btlOn = true;
  s.wild = dex;
  s.wildHp = s.wildMax = hp < 1 ? 1 : hp;
  s.dmg = dmg < 1 ? 1 : dmg;
  s.move = move;
  s.won = false;
  s.btlLeft = PB_BTL_S;
  s.inT = PB_BTL_IN_S;
  s.hitT = s.outT = 0;
}
// can the ball hit it now: in the ring (landed), not yet knocked out or gone
inline bool pinWildThere(const PinState &s) { return s.btlOn && s.wildMax && s.inT <= 0 && s.outT <= 0; }
// the ball into the grass, out of sight: the "!", then a wild one comes out (also the console's PINGRASS, testing)
inline uint32_t pinIntoGrass(PinState &s) {
  s.phase = PIN_CATCH;
  s.t = 0;
  s.grassT = PB_CATCH_S + 0.5f;
  s.vx = s.vy = 0;
  s.x = PB_NOTCH[1][0] + 6, s.y = PB_NOTCH[1][1] - 6;
  return PE_GRASS;
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
      pinAdd(s, PB_PTS_RAMP, 170, 112);  // (right of the ramp's top)
      ev |= PE_RAMP;
    }
    if (s.s <= 0) {  // not fast enough: back down and out of its mouth
      float ax = PB_RAMP_MOUTH[0], ay = PB_RAMP_MOUTH[1], ux = PB_RAMP_TOP[0] - ax, uy = PB_RAMP_TOP[1] - ay, L = hypotf(ux, uy);
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
  // into the grass's gap: caught (before it can bounce off the grass), unless a wild one's already out
  if (!s.btlOn && pinDistTo(s, PB_NOTCH[0][0], PB_NOTCH[0][1], PB_NOTCH[1][0], PB_NOTCH[1][1]) < 13)
    return ev | pinIntoGrass(s);
  // up the ramp's mouth, going its way fast enough
  {
    float ux = PB_RAMP_TOP[0] - PB_RAMP_MOUTH[0], uy = PB_RAMP_TOP[1] - PB_RAMP_MOUTH[1], L = hypotf(ux, uy);
    ux /= L, uy /= L;
    float along = s.vx * ux + s.vy * uy;
    if (hypotf(s.x - PB_RAMP_MOUTH[0], s.y - PB_RAMP_MOUTH[1]) < PB_RAMP_CATCH && along > PB_RAMP_IN) {
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
  // the wild one in the ring: each hit is your Pokemon's move on it
  if (pinWildThere(s) && pinBar(s, PB_RING_X, PB_RING_Y - 22, PB_RING_X, PB_RING_Y - 22, 22, 0.5f, PB_KICK_WILD) > 30) {
    s.hitT = PB_BTL_HIT_S;
    s.wildHp = s.dmg >= s.wildHp ? 0 : s.wildHp - s.dmg;
    pinAdd(s, PB_PTS_BTL_HIT, PB_RING_X + 66, PB_RING_Y - 36);  // (beside the ring, clear of its HP bar)
    ev |= PE_BTL_HIT;
    if (!s.wildHp) {
      s.won = true;
      s.outT = 0.001f;
      pinAdd(s, PB_PTS_BATTLE, PB_RING_X + 66, PB_RING_Y - 36);
      ev |= PE_BTL_WON;
    }
  }
  pinBar(s, PB_GRASS_X, PB_GRASS_Y - 8, PB_GRASS_X, PB_GRASS_Y + 8, PB_GRASS_RX - 2, PB_E_WALL);  // the grass's body
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
  if (s.y > PB_FLIP_Y + 46) {  // down the drain (a wild one in the ring goes too)
    s.phase = PIN_LOST;
    s.t = 0;
    s.digUp = false;
    s.btlOn = false;
    ev |= PE_LOST;
  }
  return ev;
}

// the flippers swing toward up or rest, h along (resting while the ball's in a battle). In the ball's own small steps
// while it's in play, so a flipper never jumps past it between frames
inline void pinFlippers(PinState &s, float h) {
  for (int k = 0; k < 2; k++) {
    bool up = s.hold[k] && s.phase != PIN_END;
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
  down(s.berryFlash), down(s.digHitT), down(s.grassT), down(s.popT), down(s.orbitCool), down(s.hitT), down(s.inT);
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
    case PIN_CATCH:  // the "!": then out comes a wild one, into the ring, and the ball out of the gap
      if (s.t < PB_CATCH_S) return 0;
      s.phase = PIN_PLAY;
      s.t = 0;
      s.x = PB_NOTCH[0][0] - 24, s.y = PB_NOTCH[0][1] + 18;  // (out of the gap, so it isn't caught again), dropped onto
      s.vx = -60, s.vy = 40;                                  // the right flipper to roll down it, clear of the ring
      s.grassT = 0.8f;
      s.stillT = 0;
      s.btlOn = true;  // on its way out (the grass takes no ball meanwhile): the sketch picks it, pinBattleStart()
      s.wild = 0, s.wildMax = 0;
      s.inT = PB_BTL_IN_S, s.outT = 0;
      return PE_BATTLE;
    case PIN_PLAY:
      break;
  }
  if (s.btlOn && s.wildMax) {  // the battle in the ring: the time running out, then its outcome on show
    if (s.outT > 0) {
      s.outT += dt;
      if (s.outT > PB_BTL_OUT_S) s.btlOn = false;
    } else if (s.inT <= 0) {
      s.btlLeft -= dt;
      if (s.btlLeft <= 0) {
        s.btlLeft = 0;
        s.outT = 0.001f;
        ev |= PE_BTL_GONE;
      }
    }
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
