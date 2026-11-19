#pragma once
#include <math.h>
#include <stdint.h>
#include <string.h>

// BerryMatch: a match-3 on the round screen, its rules without hardware (test/test_berrymatch.cpp); berrymatch_v2.h
// draws it and the sketch wires it in. The board is 6 x 6 with its four corners off, so it sits round on the round
// screen, its squares BM_CELL px: big enough for a finger on the Knomi (its pieces are the food icons at 3x).
// Tap a piece, then one beside it: the two swap. Three or more of a kind in a row or a column are caught; the pieces
// above fall into the gaps and new ones drop in from the top, and what lines up as they land is caught too, each step
// of a chain scoring more. A swap that lines nothing up swaps back. Four in a line leave a Great Ball, a row and a
// column crossing (an L, a T, a +) an Ultra Ball, five in a line a Master Ball; swapped with a piece beside it, a
// ball goes off where it lands: a Great Ball catches its row and its column, an Ultra Ball everything two steps from
// it, a Master Ball the whole board. A ball caught by another goes off too. BM_MOVES swaps a game; when no swap
// lines anything up, the board shuffles.

#define BM_N 6            // the board: BM_N x BM_N squares, the four corners off
#define BM_KINDS 5        // Razz Berry, Oran Berry, Pinap Berry, Rare Candy, Poke Ball
#define BM_MOVES 20       // swaps a game (the how-to cards say 20)
#define BM_CELL 60        // a square, in px: the pieces are 18 x 18 icons at 3x (54 px)
#define BM_X0 (233 - BM_N * BM_CELL / 2)  // the board's top left (the corner square's, though it's off)
#define BM_Y0 (233 - BM_N * BM_CELL / 2)
#define BM_PTS 10         // a piece caught, times the chain's step

#define BM_SWAP_S 0.16f   // a swap sliding (and sliding back)
#define BM_POP_S 0.30f    // the caught pieces popping
#define BM_SHUFFLE_S 0.80f
#define BM_HINT_S 10.0f   // idle this long: two pieces that swap wiggle...
#define BM_HINTS 3        // ...this many times a game at most, then no more (Fedor: they came too soon and too often)
#define BM_FALL_V0 5.0f   // the falling pieces: squares a second at the start...
#define BM_FALL_G 60.0f   // ...speeding up by this a second

// a square's piece: 0 nothing (a corner off, or caught and not refilled yet), 1..BM_KINDS a kind, then the balls
enum : uint8_t { BM_EMPTY = 0, BM_RAZZ = 1, BM_ORAN, BM_PINAP, BM_CANDY, BM_POKE, BM_GREAT = 8, BM_ULTRA, BM_MASTER };
inline bool bmIsBall(uint8_t p) { return p >= BM_GREAT; }
inline bool bmIsKind(uint8_t p) { return p >= 1 && p <= BM_KINDS; }

// the squares in play: all but the four corners
inline bool bmOn(int x, int y) {
  if (x < 0 || y < 0 || x >= BM_N || y >= BM_N) return false;
  bool edgeX = x == 0 || x == BM_N - 1, edgeY = y == 0 || y == BM_N - 1;
  return !(edgeX && edgeY);
}
inline int bmColTop(int x) { return x == 0 || x == BM_N - 1 ? 1 : 0; }  // a column's top square in play
inline int bmColBot(int x) { return x == 0 || x == BM_N - 1 ? BM_N - 2 : BM_N - 1; }
inline bool bmNext(int x0, int y0, int x1, int y1) {  // side by side (not corner to corner)
  int dx = x1 - x0, dy = y1 - y0;
  return bmOn(x0, y0) && bmOn(x1, y1) && ((dx == 0 && (dy == 1 || dy == -1)) || (dy == 0 && (dx == 1 || dx == -1)));
}
// the square under a point on the screen, or false
inline bool bmCellAt(int px, int py, int &x, int &y) {
  if (px < BM_X0 || py < BM_Y0) return false;
  x = (px - BM_X0) / BM_CELL;
  y = (py - BM_Y0) / BM_CELL;
  return bmOn(x, y);
}

enum BmPhase : uint8_t { BMP_PLAY, BMP_SWAP, BMP_BACK, BMP_POP, BMP_FALL, BMP_SHUFFLE, BMP_END };

// what happened, for the sounds (bmTap(), bmStep())
#define BE_PICK 1u       // a piece picked (lit up)
#define BE_UNPICK 2u     // ...put down again
#define BE_SWAP 4u       // a swap starts
#define BE_BACK 8u       // ...that lined nothing up: back they go
#define BE_POP 16u       // pieces caught
#define BE_CHAIN 32u     // ...by a chain (its second step on)
#define BE_BALL 64u      // a ball made
#define BE_FIRE 128u     // a ball gone off
#define BE_MASTER 256u   // ...a Master Ball
#define BE_LAND 512u     // the falling pieces landed
#define BE_SHUFFLE 1024u // no swap left: shuffled
#define BE_END 2048u     // the last move played out
#define BE_LAST 4096u    // one move left

struct BmFx {  // a ball gone off in this pop, for its flash
  uint8_t ball, x, y;
};

struct BmState {
  uint8_t b[BM_N][BM_N];     // [y][x]
  uint8_t gone[BM_N][BM_N];  // the pop: what each caught square held (0: not caught)
  uint8_t made[BM_N][BM_N];  // the pop: the ball made here
  uint8_t drop[BM_N][BM_N];  // the fall: how many squares the piece now here falls
  BmFx fx[BM_N * BM_N];
  uint8_t fxN = 0;
  uint8_t phase = BMP_PLAY;
  float t = 0;               // seconds in this phase
  float fallEnd = 0;         // the fall's length
  float idle = 0;            // seconds since the board was last ready (the hint)
  int8_t selX = -1, selY = -1;  // the piece picked
  int8_t ax = -1, ay = -1, bx = -1, by = -1;  // the swap: a's piece goes to b and b's to a
  bool swapOk = false;
  int8_t queueX = -1, queueY = -1;  // a tap during a fall, played once the board is ready
  int8_t hint[4] = { -1, -1, -1, -1 };  // a swap that works (shown once idle for BM_HINT_S)
  bool hintOn = false;        // ...showing now
  uint8_t hintsLeft = BM_HINTS;
  int8_t curX = 2, curY = 2;  // the keys' cursor (a computer, the web)
  bool keys = false;          // the keys have been used: the cursor shows
  uint8_t chain = 0, bestChain = 0;
  uint8_t moves = BM_MOVES;
  uint8_t balls = 0;          // balls made this game
  uint32_t score = 0;
  uint16_t pts = 0;           // the last pop's points...
  float ptsX = 0, ptsY = 0;   // ...and where (in squares), for its number floating up
  uint32_t rng = 1;
};

inline uint32_t bmRand(BmState &s) {  // xorshift32
  uint32_t x = s.rng ? s.rng : 0x9E3779B9u;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return s.rng = x;
}
inline uint8_t bmRandKind(BmState &s) { return (uint8_t)(1 + bmRand(s) % BM_KINDS); }

// (x, y) is in a line of three or more of its kind
inline bool bmMatchAt(const BmState &s, int x, int y) {
  uint8_t k = s.b[y][x];
  if (!bmIsKind(k)) return false;
  int h = 1, v = 1;
  for (int i = x - 1; i >= 0 && s.b[y][i] == k; i--) h++;
  for (int i = x + 1; i < BM_N && s.b[y][i] == k; i++) h++;
  for (int j = y - 1; j >= 0 && s.b[j][x] == k; j--) v++;
  for (int j = y + 1; j < BM_N && s.b[j][x] == k; j++) v++;
  return h >= 3 || v >= 3;
}
inline bool bmAnyMatch(const BmState &s) {
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++)
      if (bmMatchAt(s, x, y)) return true;
  return false;
}
inline void bmSwapCells(BmState &s, int x0, int y0, int x1, int y1) {
  uint8_t t = s.b[y0][x0];
  s.b[y0][x0] = s.b[y1][x1];
  s.b[y1][x1] = t;
}
// swapping these two catches something: a ball goes off, or a line forms through either
inline bool bmSwapWorks(BmState &s, int x0, int y0, int x1, int y1) {
  if (!bmNext(x0, y0, x1, y1)) return false;
  if (bmIsBall(s.b[y0][x0]) || bmIsBall(s.b[y1][x1])) return true;
  bmSwapCells(s, x0, y0, x1, y1);
  bool ok = bmMatchAt(s, x0, y0) || bmMatchAt(s, x1, y1);
  bmSwapCells(s, x0, y0, x1, y1);
  return ok;
}
// the swaps that work, up to max of them in out (x0, y0, x1, y1 each); how many there are in all
inline int bmMoves(BmState &s, int8_t (*out)[4] = nullptr, int max = 0) {
  int n = 0;
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++)
      for (int d = 0; d < 2; d++) {
        int x1 = x + (d == 0), y1 = y + (d == 1);
        if (!bmSwapWorks(s, x, y, x1, y1)) continue;
        if (out && n < max) out[n][0] = (int8_t)x, out[n][1] = (int8_t)y, out[n][2] = (int8_t)x1, out[n][3] = (int8_t)y1;
        n++;
      }
  return n;
}
inline bool bmHasMove(BmState &s) { return bmMoves(s) > 0; }

// a fresh board: every square a kind, no line of three, and a swap that works
inline void bmDeal(BmState &s) {
  for (int tries = 0; tries < 200; tries++) {
    memset(s.b, 0, sizeof(s.b));
    for (int y = 0; y < BM_N; y++)
      for (int x = 0; x < BM_N; x++) {
        if (!bmOn(x, y)) continue;
        uint8_t k;
        do k = bmRandKind(s);
        while ((x >= 2 && s.b[y][x - 1] == k && s.b[y][x - 2] == k) || (y >= 2 && s.b[y - 1][x] == k && s.b[y - 2][x] == k));
        s.b[y][x] = k;
      }
    if (bmHasMove(s)) return;
  }
}

// the pieces on the board in another order: no line of three, and a swap that works (a new deal if no order does)
inline void bmShuffle(BmState &s) {
  uint8_t p[BM_N * BM_N];
  int n = 0;
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++)
      if (bmOn(x, y)) p[n++] = s.b[y][x];
  for (int tries = 0; tries < 100; tries++) {
    for (int i = n - 1; i > 0; i--) {
      int j = (int)(bmRand(s) % (uint32_t)(i + 1));
      uint8_t t = p[i];
      p[i] = p[j];
      p[j] = t;
    }
    int k = 0;
    for (int y = 0; y < BM_N; y++)
      for (int x = 0; x < BM_N; x++)
        if (bmOn(x, y)) s.b[y][x] = p[k++];
    if (!bmAnyMatch(s) && bmHasMove(s)) return;
  }
  bmDeal(s);
}

// ----- catching: the lines, the balls they leave, the balls that go off -----
struct BmRun {
  uint8_t x, y, len, kind;
  bool across;  // a row (else a column)
};
inline bool bmRunHas(const BmRun &r, int x, int y) {
  return r.across ? (y == r.y && x >= r.x && x < r.x + r.len) : (x == r.x && y >= r.y && y < r.y + r.len);
}
inline int bmRuns(const BmState &s, BmRun *out) {  // every line of three or more (up to 2 * BM_N * BM_N / 3)
  int n = 0;
  for (int d = 0; d < 2; d++)
    for (int a = 0; a < BM_N; a++)
      for (int i = 0; i < BM_N;) {
        int x = d ? a : i, y = d ? i : a;
        uint8_t k = s.b[y][x];
        int len = 1;
        while (i + len < BM_N && s.b[d ? y + len : y][d ? x : x + len] == k) len++;
        if (bmIsKind(k) && len >= 3) out[n++] = { (uint8_t)x, (uint8_t)y, (uint8_t)len, k, d == 0 };
        i += len;
      }
  return n;
}
// what a ball catches, going off at (x, y): marks those squares
inline void bmBlast(uint8_t ball, int x, int y, uint8_t mark[BM_N][BM_N]) {
  for (int j = 0; j < BM_N; j++)
    for (int i = 0; i < BM_N; i++) {
      int dx = i > x ? i - x : x - i, dy = j > y ? j - y : y - j;
      bool hit = ball == BM_MASTER || (ball == BM_GREAT && (i == x || j == y)) || (ball == BM_ULTRA && dx + dy <= 2);
      if (hit && bmOn(i, j)) mark[j][i] = 1;
    }
}

// One step of catching: every line on the board goes (each group of lines that touch leaving its ball, where the swap
// put a piece if it's in it), and the balls swapped (fire, n of them: their squares) and any ball caught go off.
// gone/made/fx say what went, for the pop. The score goes up by BM_PTS a piece times the chain's step. Returns how
// many pieces went (0: none, the board has settled).
inline int bmCatch(BmState &s, const int8_t (*fire)[2], int nFire, const int8_t (*swapAt)[2], int nSwap, uint32_t *ev = nullptr) {
  BmRun r[2 * BM_N * BM_N / 3 + 2];
  int n = bmRuns(s, r);
  uint8_t mark[BM_N][BM_N] = {}, fired[BM_N][BM_N] = {};
  memset(s.gone, 0, sizeof(s.gone));
  memset(s.made, 0, sizeof(s.made));
  s.fxN = 0;
  for (int i = 0; i < n; i++)
    for (int k = 0; k < r[i].len; k++) mark[r[i].across ? r[i].y : r[i].y + k][r[i].across ? r[i].x + k : r[i].x] = 1;
  // the groups: lines of a kind that share a square
  int grp[2 * BM_N * BM_N / 3 + 2];
  for (int i = 0; i < n; i++) grp[i] = i;
  for (int i = 0; i < n; i++)
    for (int j = i + 1; j < n; j++) {
      if (r[i].kind != r[j].kind || r[i].across == r[j].across) continue;
      const BmRun &h = r[i].across ? r[i] : r[j], &v = r[i].across ? r[j] : r[i];
      if (!bmRunHas(h, v.x, h.y) || !bmRunHas(v, v.x, h.y)) continue;
      int a = grp[i], b = grp[j];
      for (int k = 0; k < n; k++)
        if (grp[k] == b) grp[k] = a;
    }
  uint32_t e = 0;
  for (int g = 0; g < n; g++) {
    int longest = 0, rows = 0, cols = 0, cx = -1, cy = -1, at = -1;
    for (int i = 0; i < n; i++) {
      if (grp[i] != g) continue;
      if (r[i].len > longest) longest = r[i].len, at = i;
      (r[i].across ? rows : cols)++;
    }
    if (at < 0) continue;
    uint8_t ball = longest >= 5 ? BM_MASTER : rows && cols ? BM_ULTRA : longest == 4 ? BM_GREAT : 0;
    if (!ball) continue;
    for (int k = 0; k < nSwap && cx < 0; k++)  // where the swap put a piece, if it's in the group
      for (int i = 0; i < n; i++)
        if (grp[i] == g && bmRunHas(r[i], swapAt[k][0], swapAt[k][1])) {
          cx = swapAt[k][0], cy = swapAt[k][1];
          break;
        }
    if (cx < 0 && ball == BM_ULTRA)  // else where the lines cross...
      for (int i = 0; i < n && cx < 0; i++)
        for (int j = 0; j < n && cx < 0; j++)
          if (grp[i] == g && grp[j] == g && r[i].across && !r[j].across && bmRunHas(r[i], r[j].x, r[i].y) && bmRunHas(r[j], r[j].x, r[i].y))
            cx = r[j].x, cy = r[i].y;
    if (cx < 0) {  // ...or the longest line's middle
      int m = (r[at].len - 1) / 2;
      cx = r[at].across ? r[at].x + m : r[at].x;
      cy = r[at].across ? r[at].y : r[at].y + m;
    }
    s.made[cy][cx] = ball;
    e |= BE_BALL;
  }
  // the balls: the ones swapped go off, and every ball caught by anything
  int8_t q[BM_N * BM_N + 4][2];
  int qn = 0;
  for (int i = 0; i < nFire && i < 2; i++) q[qn][0] = fire[i][0], q[qn++][1] = fire[i][1];
  while (qn > 0) {
    int x = q[--qn][0], y = q[qn][1];
    if (fired[y][x] || !bmIsBall(s.b[y][x])) continue;
    fired[y][x] = 1;
    mark[y][x] = 1;
    s.fx[s.fxN++] = { s.b[y][x], (uint8_t)x, (uint8_t)y };
    e |= BE_FIRE | (s.b[y][x] == BM_MASTER ? BE_MASTER : 0);
    uint8_t m[BM_N][BM_N] = {};
    bmBlast(s.b[y][x], x, y, m);
    for (int j = 0; j < BM_N; j++)
      for (int i = 0; i < BM_N; i++) {
        if (!m[j][i]) continue;
        mark[j][i] = 1;
        if (bmIsBall(s.b[j][i]) && !fired[j][i] && qn < BM_N * BM_N + 4) q[qn][0] = (int8_t)i, q[qn++][1] = (int8_t)j;
      }
  }
  int caught = 0;
  float sx = 0, sy = 0;
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++) {
      if (!mark[y][x] || !s.b[y][x]) continue;
      s.gone[y][x] = s.b[y][x];
      s.b[y][x] = BM_EMPTY;
      caught++;
      sx += x, sy += y;
    }
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++)
      if (s.made[y][x]) s.b[y][x] = s.made[y][x], s.balls++;
  if (caught) {
    if (!s.chain) s.chain = 1;
    s.pts = (uint16_t)(caught * BM_PTS * s.chain);
    s.score += s.pts;
    s.ptsX = sx / caught, s.ptsY = sy / caught;
    e |= BE_POP | (s.chain > 1 ? BE_CHAIN : 0);
  }
  if (ev) *ev |= e;
  return caught;
}

// the pieces fall into the gaps, each column on its own, and new ones fill it from the top: drop says how far the
// piece now in each square falls (a new one from as far above the column as there were gaps)
inline int bmFall(BmState &s) {
  memset(s.drop, 0, sizeof(s.drop));
  int most = 0;
  for (int x = 0; x < BM_N; x++) {
    int top = bmColTop(x), w = bmColBot(x);
    for (int y = w; y >= top; y--) {
      if (!s.b[y][x]) continue;
      if (w != y) s.b[w][x] = s.b[y][x], s.b[y][x] = BM_EMPTY;
      s.drop[w][x] = (uint8_t)(w - y);
      w--;
    }
    int gaps = w - top + 1;
    for (int y = w; y >= top; y--) s.b[y][x] = bmRandKind(s), s.drop[y][x] = (uint8_t)gaps;
    for (int y = top; y <= bmColBot(x); y++)
      if (s.drop[y][x] > most) most = s.drop[y][x];
  }
  return most;
}
// how far a piece falling d squares still has to go after t seconds (0: landed), and how long d squares take
inline float bmFallLeft(float d, float t) {
  float gone = BM_FALL_V0 * t + BM_FALL_G * t * t / 2;
  return gone >= d ? 0 : d - gone;
}
inline float bmFallTime(float d) { return d <= 0 ? 0 : (-BM_FALL_V0 + sqrtf(BM_FALL_V0 * BM_FALL_V0 + 2 * BM_FALL_G * d)) / BM_FALL_G; }

// ----- a game -----
inline void bmFindHint(BmState &s);
inline void bmStart(BmState &s, uint32_t seed) {
  s = BmState();
  s.rng = seed ? seed : 1;
  for (int i = 0; i < 4; i++) bmRand(s);
  bmDeal(s);
  bmFindHint(s);  // (the first board has its hint too)
}
inline void bmFindHint(BmState &s) {
  int8_t m[64][4];
  int n = bmMoves(s, m, 64);
  if (n > 64) n = 64;
  if (n <= 0) {
    s.hint[0] = -1;
    return;
  }
  memcpy(s.hint, m[bmRand(s) % (uint32_t)n], 4);
}
inline bool bmHintShows(const BmState &s) { return s.phase == BMP_PLAY && s.hintOn; }
inline void bmBusy(BmState &s) {  // something was done: idle from now (the hint showing stops)
  s.idle = 0;
  s.hintOn = false;
}

inline uint32_t bmTap(BmState &s, int x, int y);
// the board is still: the game's end, a shuffle, or ready for the next swap (and a tap that came in while it moved)
inline uint32_t bmSettle(BmState &s) {
  s.chain = 0;
  s.t = 0;
  bmBusy(s);
  s.hint[0] = -1;
  if (s.moves == 0) {
    s.phase = BMP_END;
    s.selX = s.selY = -1;
    return BE_END;
  }
  if (!bmHasMove(s)) {
    bmShuffle(s);
    s.phase = BMP_SHUFFLE;
    s.selX = s.selY = -1;
    return BE_SHUFFLE;
  }
  s.phase = BMP_PLAY;
  bmFindHint(s);
  uint32_t ev = 0;
  if (s.queueX >= 0) {  // the swap asked for while it moved
    int qx = s.queueX, qy = s.queueY;
    s.queueX = s.queueY = -1;
    if (s.selX >= 0 && bmNext(s.selX, s.selY, qx, qy)) ev |= bmTap(s, qx, qy);
  }
  return ev;
}
// a catching step (the swap's own when the chain is 1): the pop, or the board settles
inline uint32_t bmResolve(BmState &s, const int8_t (*fire)[2], int nFire) {
  uint32_t ev = 0;
  s.chain++;
  int8_t at[2][2] = { { s.bx, s.by }, { s.ax, s.ay } };  // (the piece picked went to b)
  if (bmCatch(s, fire, nFire, at, s.chain == 1 ? 2 : 0, &ev) > 0) {
    if (s.chain > s.bestChain) s.bestChain = s.chain;
    s.phase = BMP_POP;
    s.t = 0;
    return ev;
  }
  return ev | bmSettle(s);
}

// a square tapped (or Space on the cursor): pick it, put it down, or swap it with the one picked beside it. While the
// board moves a piece can still be picked; a swap waits for it to be ready (bmSettle())
inline uint32_t bmTap(BmState &s, int x, int y) {
  if (s.phase == BMP_END) return 0;
  bool moving = s.phase != BMP_PLAY;
  if (!bmOn(x, y)) {
    bool had = s.selX >= 0;
    s.selX = s.selY = -1;
    return had ? BE_UNPICK : 0;
  }
  bmBusy(s);
  if (s.selX == x && s.selY == y) {
    s.selX = s.selY = -1;
    return BE_UNPICK;
  }
  if (s.selX >= 0 && bmNext(s.selX, s.selY, x, y) && moving) {  // moving: the swap waits for the board to be ready
    s.queueX = (int8_t)x, s.queueY = (int8_t)y;
    s.curX = (int8_t)x, s.curY = (int8_t)y;
    return 0;
  }
  if (s.selX >= 0 && bmNext(s.selX, s.selY, x, y)) {
    s.ax = s.selX, s.ay = s.selY, s.bx = (int8_t)x, s.by = (int8_t)y;
    s.selX = s.selY = -1;
    s.swapOk = bmSwapWorks(s, s.ax, s.ay, s.bx, s.by);
    s.phase = BMP_SWAP;
    s.t = 0;
    s.curX = (int8_t)x, s.curY = (int8_t)y;
    return BE_SWAP;
  }
  s.selX = (int8_t)x, s.selY = (int8_t)y;
  s.queueX = s.queueY = -1;
  return BE_PICK;
}

// the keys' cursor: an arrow moves it (past a corner that's off, to the next square in play); with a piece picked,
// an arrow swaps it that way instead. Space picks (bmTap on the cursor)
inline uint32_t bmKey(BmState &s, int dx, int dy) {
  s.keys = true;
  if (s.phase == BMP_END) return 0;
  if (s.selX >= 0) {
    int x = s.selX + dx, y = s.selY + dy;
    if (bmOn(x, y)) return bmTap(s, x, y);
    return 0;
  }
  int x = s.curX + dx, y = s.curY + dy;
  while (x >= 0 && y >= 0 && x < BM_N && y < BM_N && !bmOn(x, y)) x += dx, y += dy;
  if (bmOn(x, y)) s.curX = (int8_t)x, s.curY = (int8_t)y;
  return 0;
}
inline uint32_t bmKeyPick(BmState &s) {
  s.keys = true;
  return bmTap(s, s.curX, s.curY);
}

// the game moving on by dt seconds
inline uint32_t bmStep(BmState &s, float dt) {
  uint32_t ev = 0;
  s.t += dt;
  switch (s.phase) {
    case BMP_PLAY:
      s.idle += dt;
      if (s.idle >= BM_HINT_S && !s.hintOn && s.hintsLeft > 0 && s.hint[0] >= 0) {  // one of the game's few hints
        s.hintOn = true;
        s.hintsLeft--;
      }
      break;
    case BMP_SWAP:
      if (s.t < BM_SWAP_S) break;
      if (!s.swapOk) {
        s.phase = BMP_BACK;
        s.t = 0;
        ev |= BE_BACK;
        break;
      }
      {
        bmSwapCells(s, s.ax, s.ay, s.bx, s.by);
        s.moves--;
        if (s.moves == 1) ev |= BE_LAST;
        int8_t fire[2][2];
        int nf = 0;
        if (bmIsBall(s.b[s.by][s.bx])) fire[nf][0] = s.bx, fire[nf++][1] = s.by;
        if (bmIsBall(s.b[s.ay][s.ax])) fire[nf][0] = s.ax, fire[nf++][1] = s.ay;
        // a Master Ball with another ball beside it: still the whole board (bmBlast); with a piece, the same
        s.chain = 0;
        ev |= bmResolve(s, fire, nf);
      }
      break;
    case BMP_BACK:
      if (s.t >= BM_SWAP_S) {
        s.phase = BMP_PLAY;
        s.t = 0;
        bmBusy(s);
        if (s.queueX >= 0) {
          int qx = s.queueX, qy = s.queueY;
          s.queueX = s.queueY = -1;
          if (s.selX >= 0 && bmNext(s.selX, s.selY, qx, qy)) ev |= bmTap(s, qx, qy);
        }
      }
      break;
    case BMP_POP:
      if (s.t >= BM_POP_S) {
        s.fallEnd = bmFallTime((float)bmFall(s));
        s.phase = BMP_FALL;
        s.t = 0;
      }
      break;
    case BMP_FALL:
      if (s.t >= s.fallEnd) {
        ev |= BE_LAND;
        s.ax = s.ay = s.bx = s.by = -1;
        ev |= bmResolve(s, nullptr, 0);
      }
      break;
    case BMP_SHUFFLE:
      if (s.t >= BM_SHUFFLE_S) ev |= bmSettle(s);
      break;
    default:
      break;
  }
  return ev;
}
