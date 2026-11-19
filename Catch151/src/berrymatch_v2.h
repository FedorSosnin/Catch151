// V2: BerryMatch, drawn: the game from berrymatch.h's state (bmDraw()). A lawn seen from above, with the board on it
// in the home's see-through dark tint (one even tint, and a fainter rim round it), cached together; yours comes and
// goes round its cut corners (bmPetDraw()). The pieces are the food icons at 4x (BM_PIECE px, the home food picker's size): the Razz,
// Oran and Pinap Berries and the Rare Candy; the balls four or five in a line make are the Great, Ultra and Master Balls. The score over the
// board, the moves left under it.
// scene_v2.h includes this after pinball_v2.h, whose painters it borrows (fsBlend, fsSq, fsCircle, fsSay, twGlow).

// ----- the Great Ball: the Ultra Ball's 18 x 18 sprite (master_v2.h), its cap blue and its marks red, as Pinball's
// bumper has them -----
static const char *const SPR_V2_GREAT_BALL[18] = {
  "......kkkkkk......",
  "....kkyqqqqykk....",
  "...kwyyqqqqyyQk...",
  "..kqwyyqqqqyyQQk..",
  ".kqqqyykkkkyyQQQk.",
  ".kqqqykkkkkkyQQQk.",
  "kqqqqkkkwwkkkQQQQk",
  "kkkkkkkwwwskkkkkkk",
  "kkkkkkkwwsskkkkkkk",
  "kwwwwkkksskkkwwwSk",
  "kwwwwwkkkkkkwwwSSk",
  ".kwwwwwkkkkwwwwSk.",
  ".kwwwwwwwwwwwwSSk.",
  "..kwwwwwwwwwwSSk..",
  "...kSwwwwwwwSSk...",
  "....kkSSSSSSkk....",
  "......kkkkkk......",
  "..................",
};
static uint16_t v2GreatInk(char c) {
  switch (c) {
    case 'q': return C565(0x3a, 0x78, 0xd8);
    case 'Q': return C565(0x26, 0x52, 0xa0);
    case 'y': return C565(0xe8, 0x50, 0x3a);
    case 'w': return UI_WHITE;
    case 's': return C565(0xaa, 0xb0, 0xbc);
    case 'S': return C565(0x78, 0x7e, 0x8c);
    default: return C565(0x1b, 0x1b, 0x25);
  }
}

#define BM_HALF (BM_CELL / 2)
#define BM_GOLD 0xF2C230
static int bmPx(float x) { return BM_X0 + (int)lroundf(x * BM_CELL) + BM_HALF; }  // a square's middle (x may be between)
static int bmPy(float y) { return BM_Y0 + (int)lroundf(y * BM_CELL) + BM_HALF; }

// a piece, its middle at (cx, cy), BM_PIECE px: turned deg, smaller for step > 1; clipY: none of it above that row (a
// new piece coming in at the top of its column). Drawn in 2 px cells (the Knomi's own pixels), each the icon's pixel
// under its middle
static void bmPiece(uint8_t p, int cx, int cy, float deg = 0, float step = 1, int clipY = -1) {
  const float st = step * 2.0f * HOME_ICON_N / BM_PIECE, mid = HOME_ICON_N / 2.0f;  // the icon's pixels a cell moves
  if (bmIsBall(p)) {
    const char *const *spr = p == BM_GREAT ? SPR_V2_GREAT_BALL : p == BM_ULTRA ? SPR_V2_ULTRA_BALL : SPR_V2_MASTER_BALL;
    uint16_t (*ink)(char) = p == BM_GREAT ? v2GreatInk : p == BM_ULTRA ? v2UltraInk : v2MasterInk;
    v2BallSprTurned(spr, ink, 2, deg, SNAP2(cx), SNAP2(cy), mid, mid, st);
    return;
  }
  static const char *const *const SPR[BM_KINDS] = { SPR_HOME_FOOD, SPR_BERRY_ORAN, SPR_BERRY_PINAP, SPR_CANDY };
  if (!bmIsKind(p)) return;
  const char *const *spr = SPR[p - 1];
  if (deg != 0 || step != 1) {
    drawMapTurnedStep(spr, HOME_ICON_N, 2, st, deg, SNAP2(cx), SNAP2(cy), mid, mid);
    return;
  }
  // its top left on an even pixel, as the turned ones' cells are: n (29) is odd, and from SNAP2(cx) - n every cell lay
  // across two of the Knomi's pixels and blurred (Fedor: "all the graphics seem blurry on the knomi")
  const int n = BM_PIECE / 2, x0 = SNAP2(cx - n), y0 = SNAP2(cy - n);
  for (int j = 0; j < n; j++) {
    int y = y0 + 2 * j;
    if (y < clipY) continue;
    const char *row = spr[(int)((j + 0.5f) * st)];
    forRuns(n, [&](int c) { return charKey(row[(int)((c + 0.5f) * st)], false); },
            [&](int c0, int len, int k) { gfx->fillRect(x0 + 2 * c0, y, 2 * len, 2, spriteColor((char)k)); });
  }
}

// ----- the board on the clearing, painted once for each light of the day (v2Cached) -----
// in the board's shape (a square in play), or how far outside it (px; up to 9)
static int bmShapeDist(int px, int py) {
  int cx = (px - BM_X0 + BM_CELL) / BM_CELL - 1, cy = (py - BM_Y0 + BM_CELL) / BM_CELL - 1;
  if (bmOn(cx, cy) && px >= BM_X0 && py >= BM_Y0) return 0;
  int best = 99;
  for (int j = cy - 1; j <= cy + 1; j++)
    for (int i = cx - 1; i <= cx + 1; i++) {
      if (!bmOn(i, j)) continue;
      int x0 = BM_X0 + i * BM_CELL, y0 = BM_Y0 + j * BM_CELL;
      int dx = px < x0 ? x0 - px : px >= x0 + BM_CELL ? px - (x0 + BM_CELL - 1) : 0;
      int dy = py < y0 ? y0 - py : py >= y0 + BM_CELL ? py - (y0 + BM_CELL - 1) : 0;
      int d = (int)sqrtf((float)(dx * dx + dy * dy));
      if (d < best) best = d;
    }
  return best;
}
#define BM_RIM 8
// the lawn under the board (Fedor's pick of four: Berry Drop's clearing put a light sky under the board's top half, and
// its tint over the sky came out grey-blue, the Oran Berry's and the Rare Candy's own colours, "hard to see on the
// background"; a wooden table and a berry patch were the others)
static uint32_t bmHash(int a, int b) {
  uint32_t h = (uint32_t)(a * 73856093) ^ (uint32_t)(b * 19349663);
  h ^= h >> 13;
  h *= 0x5bd1e995;
  return h ^ (h >> 15);
}
// a lawn seen from above: the grass in two greens with darker tufts, clover and a few flowers round the board
static void bmPaintGrass() {
  for (int y = 0; y < V2_W; y += 4)
    for (int x = 0; x < V2_W; x += 4) {
      uint32_t h = bmHash(x / 4, y / 4) % 100;
      scCell(x, y, h < 12 ? 0x5A9A3A : h < 20 ? 0x86C860 : ((x / 4 + y / 4 * 3) % 7 == 0 ? 0x64A846 : 0x6DB04A));
    }
  for (int i = 0; i < 260; i++) {  // tufts: a little v
    int x = (int)(bmHash(i, 1) % 116) * 4, y = (int)(bmHash(i, 2) % 116) * 4;
    scCell(x, y, 0x4A8A34);
    scCell(x + 8, y, 0x4A8A34);
    scCell(x + 4, y + 4, 0x4A8A34);
  }
  for (int i = 0; i < 70; i++) {  // flowers, only off the board
    int x = (int)(bmHash(i, 5) % 112) * 4 + 4, y = (int)(bmHash(i, 6) % 112) * 4 + 4;
    if (bmShapeDist(x, y) < 14) continue;
    Rgb petal = i % 3 == 0 ? 0xF4F4F8 : i % 3 == 1 ? 0xF8C8D8 : 0xF8E070;
    scCell(x - 4, y, petal), scCell(x + 4, y, petal), scCell(x, y - 4, petal), scCell(x, y + 4, petal);
    scCell(x, y, i % 3 == 2 ? 0xE8803A : 0xF2C230);
  }
}
static void bmPaintBack(int tod) {
  scGradeTod = tod;  // (dusk and night graded as the other scenes)
  bmPaintGrass();
  scGradeTod = 0;
  uint16_t ink = sc565(0x1B1B25);
  for (int y = BM_Y0 - BM_RIM; y < BM_Y0 + BM_N * BM_CELL + BM_RIM; y += 2)
    for (int x = BM_X0 - BM_RIM; x < BM_X0 + BM_N * BM_CELL + BM_RIM; x += 2) {
      int d = bmShapeDist(x, y);
      if (d > BM_RIM) continue;
      // the board one even tint, its rim fainter (Fedor didn't like seeing each square: they were two shades, a
      // checkerboard)
      int a = d == 0 ? 120 : V2_TRACK_A;
      v2Cell(scFb, x, y, ink, a);
    }
}
static void bmBack() { uint8_t t = v2HourTod(); v2Cached(0x3B000000 + t, bmPaintBack, t); }

// ----- what's said over the board for a moment (SHUFFLE!, LAST MOVE!) -----
static StrId bmSayId = S_BM_SHUFFLE;
static uint32_t bmSayT = 0;
static void bmSay(StrId id) {
  bmSayId = id;
  bmSayT = millis() | 1;
}
// moves earned (BE_EARN): "+2" rising beside the moves left, which show green for a moment
static uint8_t bmEarnN = 0;
static uint32_t bmEarnT = 0;
#define BM_EARN_MS 1300
#define BM_GREEN 0x9AF07A
static void bmEarn(uint8_t n) {
  bmEarnN = n;
  bmEarnT = millis() | 1;
}
// how long ago t was, for a frame drawn at now: 0 for a t set after now (an event handled in the frame being drawn,
// a millisecond or so on: as a uint32_t subtraction it came out huge, and the word or the +1 was cleared unseen)
static uint32_t bmAge(uint32_t now, uint32_t t) { return (int32_t)(now - t) > 0 ? now - t : 0; }

static float bmEase(float k) { return k <= 0 ? 0 : k >= 1 ? 1 : k * k * (3 - 2 * k); }

// a ball gone off, its flash over the pop (k: 0 to 1): a Great Ball's beams along its row and column, an Ultra
// Ball's ring spreading, a Master Ball's light over the whole board
static void bmFlash(const BmFx &f, float k) {
  int a = (int)(200 * (1 - k));
  int cx = bmPx(f.x), cy = bmPy(f.y);
  if (f.ball == BM_GREAT) {
    int w = 20 - (int)(12 * k);
    for (int i = 0; i < BM_N; i++) {
      if (bmOn(i, f.y)) fsBlend(BM_X0 + i * BM_CELL, cy - w, BM_X0 + (i + 1) * BM_CELL - 1, cy + w, 0xFFF3B0, a);
      if (bmOn(f.x, i) && i != f.y) fsBlend(cx - w, BM_Y0 + i * BM_CELL, cx + w, BM_Y0 + (i + 1) * BM_CELL - 1, 0xFFF3B0, a);
    }
  } else if (f.ball == BM_ULTRA) {
    int r = 20 + (int)(2.2f * BM_CELL * k);
    fsCircle(cx, cy, r, BM_GOLD, 4);
    fsCircle(cx, cy, r * 2 / 3, 0xFFF3B0, 2);
    twGlow(cx, cy, 70, 0xFFF3B0, a);
  } else {
    for (int j = 0; j < BM_N; j++)
      for (int i = 0; i < BM_N; i++)
        if (bmOn(i, j)) fsBlend(BM_X0 + i * BM_CELL, BM_Y0 + j * BM_CELL, BM_X0 + (i + 1) * BM_CELL - 1, BM_Y0 + (j + 1) * BM_CELL - 1, 0xE8C8FF, a * 3 / 4);
  }
}

// ----- yours at the board's edge (Fedor: "a sprite of your pokemon ... coming in and out of the frame around the edge
// as you play. That way it feels like you are still playing with your pokemon"): it walks in from the round screen's
// rim to one of the board's four cut corners, watches, hops for a ball made or set off, moves earned, a long chain,
// and walks back out; a while later it comes in at another corner. Off the rim it can't be seen, so it comes "in and
// out of the frame". Drawn before the pieces, so it stands behind the board's edge -----
enum : uint8_t { BM_PET_AWAY, BM_PET_IN, BM_PET_WATCH, BM_PET_OUT };
#define BM_PET_SPEED 70.0f  // px a second
#define BM_PET_OFF 140      // how far out along its side it starts and goes back to (off the round screen)
// the feet at each corner, and the side it comes from (-1 the left, 1 the right): inside the screen, clear of the
// score and the moves left
static const int16_t BM_PET_SPOT[4][3] = { { 96, 408, -1 }, { 370, 408, 1 }, { 96, 116, -1 }, { 370, 116, 1 } };
struct BmPet {
  uint8_t phase = BM_PET_AWAY, spot = 0, scale = 2;
  float x = 0;
  uint32_t until = 0, cheerT = 0, lastT = 0, seed = 1;
};
static BmPet bmPet;
static uint32_t bmPetRand(uint32_t n) {
  bmPet.seed = bmPet.seed * 1103515245u + 12345u;
  return (bmPet.seed >> 16) % n;
}
// a game starting: away, coming in a moment after the card's gone; 4x if that fits a corner, else 2x (an even scale:
// each of its pixels whole on the Knomi's 2 x 2 ones; at 2x a Squirtle was a speck)
static void bmPetStart(uint32_t now) {
  bmPet = BmPet();
  bmPet.until = now + 1500;
  bmPet.seed = now | 1;
  uint8_t fit = pmd.loaded ? pmdFitScale(pmd, PMD_IDLE, 96, 84) : 2;
  bmPet.scale = fit >= 4 ? 4 : 2;
}
static void bmPetIn(uint32_t now) {  // to a corner it wasn't at last
  uint8_t s = (uint8_t)((bmPet.spot + 1 + bmPetRand(3)) % 4);
  bmPet.spot = s;
  bmPet.x = BM_PET_SPOT[s][0] + BM_PET_SPOT[s][2] * BM_PET_OFF;
  bmPet.phase = BM_PET_IN;
}
// a good moment: a hop if it's there, and it comes at once if it's away
static void bmPetCheer(uint32_t now) {
  bmPet.cheerT = now | 1;
  if (bmPet.phase == BM_PET_AWAY) bmPetIn(now);
  else if (bmPet.phase == BM_PET_WATCH && bmPet.until < now + 2500) bmPet.until = now + 2500;
  else if (bmPet.phase == BM_PET_OUT) bmPet.phase = BM_PET_IN;  // (it turns round)
}
static void bmPetDraw(uint32_t now) {
  float dt = bmPet.lastT ? (now - bmPet.lastT) / 1000.0f : 0;
  bmPet.lastT = now;
  if (dt > 0.1f) dt = 0.1f;
  const int16_t *sp = BM_PET_SPOT[bmPet.spot];
  float home = sp[0], out = sp[0] + sp[2] * BM_PET_OFF;
  switch (bmPet.phase) {
    case BM_PET_AWAY:
      if ((int32_t)(now - bmPet.until) >= 0) bmPetIn(now);
      return;
    case BM_PET_IN:
      bmPet.x += (home > bmPet.x ? 1 : -1) * BM_PET_SPEED * dt;
      if ((out < home) == (bmPet.x >= home)) {
        bmPet.x = home;
        bmPet.phase = BM_PET_WATCH;
        bmPet.until = now + 5000 + bmPetRand(4000);
      }
      break;
    case BM_PET_WATCH:
      if ((int32_t)(now - bmPet.until) >= 0) bmPet.phase = BM_PET_OUT;
      break;
    default:  // BM_PET_OUT
      bmPet.x += (out > bmPet.x ? 1 : -1) * BM_PET_SPEED * dt;
      if ((out < home) == (bmPet.x <= out)) {
        bmPet.phase = BM_PET_AWAY;
        bmPet.until = now + 4000 + bmPetRand(5000);
        return;
      }
      break;
  }
  if (!pmd.loaded) return;
  bool right = (bmPet.phase == BM_PET_IN) == (sp[2] < 0);  // the way it walks
  uint8_t act = bmPet.phase == BM_PET_WATCH ? (bmPet.cheerT && now - bmPet.cheerT < 1400 ? PMD_HOP : PMD_IDLE)
                                            : right ? PMD_WALKR : PMD_WALKL;
  if (!pmd.has(act)) act = PMD_IDLE;
  gPmdForceScale = bmPet.scale;
  drawPmdAct(act, (int)bmPet.x, sp[1], now, true, false, bmPet.scale);
  gPmdForceScale = 0;
}

// the whole game: the board, the pieces where they are in their move, the score and the moves left; on a computer,
// the keys' cursor once an arrow has been used
static void bmDraw(const BmState &s, uint32_t now) {
  bmBack();
  bmPetDraw(now);  // yours, behind the board's edge
  float k = 0;
  switch (s.phase) {
    case BMP_SWAP:
    case BMP_BACK: k = bmEase(s.t / BM_SWAP_S); break;
    case BMP_POP: k = s.t / BM_POP_S; break;
    case BMP_SHUFFLE: k = s.t / BM_SHUFFLE_S; break;
    default: break;
  }
  if (k > 1) k = 1;
  bool hint = bmHintShows(s);
  // the pieces
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++) {
      if (!bmOn(x, y)) continue;
      uint8_t p = s.b[y][x];
      float fx = x, fy = y, deg = 0, step = 1;
      int clip = -1;
      bool isA = x == s.ax && y == s.ay, isB = x == s.bx && y == s.by;
      if (s.phase == BMP_SWAP && (isA || isB)) {  // sliding over to the other's square
        fx = isA ? s.ax + (s.bx - s.ax) * k : s.bx + (s.ax - s.bx) * k;
        fy = isA ? s.ay + (s.by - s.ay) * k : s.by + (s.ay - s.by) * k;
      } else if (s.phase == BMP_BACK && (isA || isB)) {  // and back again
        fx = isA ? s.bx + (s.ax - s.bx) * k : s.ax + (s.bx - s.ax) * k;
        fy = isA ? s.by + (s.ay - s.by) * k : s.ay + (s.by - s.ay) * k;
      } else if (s.phase == BMP_FALL) {
        fy = y - bmFallLeft(s.drop[y][x], s.t);
        clip = BM_Y0 + bmColTop(x) * BM_CELL;
      } else if (s.phase == BMP_POP) {
        if (s.gone[y][x]) {  // caught: it spins away to nothing
          bmPiece(s.gone[y][x], bmPx(x), bmPy(y), 200 * k, 1 + 4 * k);
          if (!s.made[y][x]) continue;
        }
        if (s.made[y][x]) step = 1 + 2 * (1 - bmEase(k));  // a ball made here grows out of it
      } else if (s.phase == BMP_SHUFFLE) {
        deg = 360 * bmEase(k);
        step = 1 + 0.7f * sinf(3.1416f * k);
      } else if (s.phase == BMP_PLAY) {
        if (x == s.selX && y == s.selY) fy -= 0.06f * fabsf(sinf(now * 0.008f));  // picked: it hops
        bool h = hint && ((x == s.hint[0] && y == s.hint[1]) || (x == s.hint[2] && y == s.hint[3]));
        if (h) deg = 14 * sinf(now * 0.012f);  // the hint: two that swap wiggle
        else if (bmIsBall(p)) deg = 8 * sinf(now * 0.004f + x + y);  // a ball rocks, ready to go off
      }
      bmPiece(p, bmPx(fx), bmPy(fy), deg, step, clip);
    }
  // the keys' cursor and the piece picked: an outline round its square (nothing behind it)
  if (s.keys && s.phase != BMP_END) {
    int x0 = BM_X0 + s.curX * BM_CELL, y0 = BM_Y0 + s.curY * BM_CELL;
    for (int w = 1; w < 3; w++) gfx->drawRoundRect(x0 + w, y0 + w, BM_CELL - 2 * w, BM_CELL - 2 * w, 11 - w, UI_WHITE);
  }
  if (s.selX >= 0 && s.phase != BMP_END) {
    int x0 = BM_X0 + s.selX * BM_CELL, y0 = BM_Y0 + s.selY * BM_CELL;
    for (int w = 0; w < 4; w++) gfx->drawRoundRect(x0 + w, y0 + w, BM_CELL - 2 * w, BM_CELL - 2 * w, 12 - w, sc565(w == 0 || w == 3 ? 0x1B1B25 : BM_GOLD));
  }
  // the pop: sparkles from each caught square, the balls' flashes, and the points floating up
  if (s.phase == BMP_POP) {
    for (int y = 0; y < BM_N; y++)
      for (int x = 0; x < BM_N; x++) {
        if (!s.gone[y][x]) continue;
        int r = 8 + (int)(30 * k);
        for (int i = 0; i < 4; i++) {
          float a = 0.785f + i * 1.5708f + (x + y) * 0.4f;
          fsSq(bmPx(x) + (int)(r * cosf(a)), bmPy(y) + (int)(r * sinf(a)), k < 0.6f ? 6 : 4, i % 2 ? 0xFFF3B0 : BM_GOLD);
        }
      }
    for (int i = 0; i < s.fxN; i++) bmFlash(s.fx[i], k);
  }
  if ((s.phase == BMP_POP || (s.phase == BMP_FALL && s.t < 0.4f)) && s.pts) {
    float up = s.phase == BMP_POP ? k : 1 + s.t * 2;
    char b[16];
    snprintf(b, sizeof(b), "+%u", (unsigned)s.pts);
    int y = bmPy(s.ptsY) - 16 - (int)(24 * up);
    fsSay(b, bmPx(s.ptsX), y, s.chain > 1 ? BM_GOLD : 0xFFFFFF, s.chain > 2 ? 3 : 2);
  }
  // the score over the board, the moves left under it (gold for the last few)
  char b[32];
  snprintf(b, sizeof(b), "%lu", (unsigned long)s.score);
  fsSay(b, CX, 16, 0xFFFFFF, 3);
  snprintf(b, sizeof(b), T(S_BM_MOVES_FMT), (unsigned)s.moves);
  const int my = BM_Y0 + BM_N * BM_CELL + 6;
  bool earning = bmEarnT && bmAge(now, bmEarnT) < BM_EARN_MS;
  fsSay(b, CX, my, earning && bmAge(now, bmEarnT) < BM_EARN_MS / 2 ? BM_GREEN : s.moves <= 3 ? BM_GOLD : 0xFFFFFF, 2);
  if (earning) {  // the moves just earned, rising off the count's right end
    setSize(2);
    char e[8];
    snprintf(e, sizeof(e), "+%u", (unsigned)bmEarnN);
    float k = bmAge(now, bmEarnT) / (float)BM_EARN_MS;
    fsSay(e, CX + textW(b, 2) / 2 + 10 + textW(e, 2) / 2, my - (int)(26 * k), BM_GREEN, 2);
  } else {
    bmEarnT = 0;
  }
  // a word for a moment: SHUFFLE! (as long as it takes), LAST MOVE!
  if (bmSayT && bmAge(now, bmSayT) < 1300) {
    float j = bmAge(now, bmSayT) / 1300.0f;
    int y = CY - 14 - (j < 0.15f ? (int)(30 * (0.15f - j) / 0.15f) : 0);
    fsSay(T(bmSayId), CX, y, BM_GOLD, 3);
  } else {
    bmSayT = 0;
  }
}

// ----- the cartridge's label (menus_v2.h v2PaintGameLabels(), at the canvas's top left, its own size) -----
// a little board on the meadow: two rows of pieces at 1x in the tint's squares, three Oran Berries lined up and
// shining in the middle of the bottom row, Squirtle watching from the right
static void bmPaintLabel() {
  scScreen();  // (fsBlend paints where the scene painters do: the screen)
  v2LblSky(0x7FBDE6, 0xD8EEE8, 30, 0x79BD4F, 0x5A9A3A);
  static const uint8_t P[2][3] = { { BM_PINAP, BM_RAZZ, BM_CANDY }, { BM_ORAN, BM_ORAN, BM_ORAN } };
  for (int j = 0; j < 2; j++)
    for (int i = 0; i < 3; i++) {
      int x = 2 + i * 19, y = 3 + j * 19;
      fsBlend(x, y, x + 18, y + 18, 0x1B1B25, (i + j) % 2 ? 92 : 118);
    }
  fsBlend(2, 22, 58, 40, 0xFFF3B0, 90);  // the three in a line, lit
  static const char *const *const SPR[BM_KINDS] = { SPR_HOME_FOOD, SPR_BERRY_ORAN, SPR_BERRY_PINAP, SPR_CANDY };
  for (int j = 0; j < 2; j++)
    for (int i = 0; i < 3; i++) drawMap(SPR[P[j][i] - 1], HOME_ICON_N, 2 + i * 19, 3 + j * 19, 1, false);
  for (int k = 0; k < 3; k++) gfx->fillRect(8 + k * 19, 19, 2, 2, UI_WHITE);  // sparkles over them
  v2LblThumb(7, 69, 41, false);
}
