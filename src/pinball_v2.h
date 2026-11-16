// V2: Pinball's MOCKUPS (PINMOCK n): still screens drawn in the real game, so the look can be judged before any of it is
// built (as Reel Fishing's were, FISHMOCK n). The plan is docs/PINBALL.md: the round screen is the table, its rim the
// wall; three bumpers, your own Pokemon as the target in the middle, berry lights round the sides, two flippers at the
// bottom over the drain, and a Poke Ball for the ball. scene_v2.h includes this after fishing_v2.h, whose painters it
// borrows (fsSay, fsBoard, fsThumb, fsCircle, fsSq, fsBlend). On a board nothing calls them: they're a computer's only.

static uint8_t pbMock = 0;  // the mockup on screen (0: none)
void pinMockSet(uint8_t n) { pbMock = n; }

#ifdef BOARD_HOST
// ===================================================================== the table
#define PB_R 218           // the table's inside; the rim wall from here out to the screen's edge
#define PB_FLIP_LX 148     // the left flipper's pivot (the right one's mirrored about the middle)
#define PB_FLIP_Y 390
#define PB_FLIP_LEN 70
#define PB_FLIP_REST 0.50f  // its angle at rest (radians below level, the tip down toward the middle)
#define PB_FLIP_UP -0.42f   // raised
#define PB_BALL_R 10
static bool pbDay = false;  // the second look: a meadow on a wooden rim (the first: a night-blue table on a chrome one)
static const int16_t PB_BUMP[3][2] = { { 160, 168 }, { 306, 168 }, { 233, 124 } };
static const Rgb PB_BUMP_COL[3] = { 0xE8503A, 0x4FB0E8, 0xF2C230 };
#define PB_PET_FEET 292    // your Pokemon on its pad, in the middle
// the six berry lights: three up each side, near the wall
static const int16_t PB_BERRY[6][2] = { { 53, 299 }, { 42, 216 }, { 67, 137 }, { 413, 299 }, { 424, 216 }, { 399, 137 } };

static inline void pbPx2(int x, int y, uint16_t c) {  // a Knomi pixel (2 x 2)
  if (x < 0 || y < 0 || x > V2_W - 2 || y > V2_W - 2) return;
  uint16_t *q = scFb + y * V2_W + x;
  q[0] = q[1] = q[V2_W] = q[V2_W + 1] = c;
}
static inline void pbPx1(int x, int y, uint16_t c) {  // one pixel (the ball's fine outline)
  if (x >= 0 && y >= 0 && x < V2_W && y < V2_W) scFb[y * V2_W + x] = c;
}
static void pbEllipse(int cx, int cy, int rx, int ry, Rgb col) {
  uint16_t c = sc565(col);
  for (int y = SNAP2(cy - ry); y <= cy + ry; y += 2)
    for (int x = SNAP2(cx - rx); x <= cx + rx; x += 2) {
      float dx = (x + 1 - cx) / (float)rx, dy = (y + 1 - cy) / (float)ry;
      if (dx * dx + dy * dy <= 1) pbPx2(x, y, c);
    }
}
// the playfield and its rim
static void pbTable() {
  scScreen();
  const Rgb mid = pbDay ? 0x7CC85A : 0x34388A, edge = pbDay ? 0x4E9A3E : 0x1C1E4A;
  for (int y = 0; y < V2_W; y += 2)
    for (int x = 0; x < V2_W; x += 2) {
      float d = hypotf(x + 1 - CX, y + 1 - CY);
      Rgb c;
      if (d >= 233) {  // past the round screen (only a screenshot shows it)
        c = 0x101018;
      } else if (d >= PB_R + 2) {  // the rim: chrome by night, wood by day, its inner edge dark
        float t = (d - PB_R - 2) / (233 - PB_R - 2);
        c = pbDay ? scMix(0xB07A45, 0x6E4426, t) : scMix(0xC8CCD8, 0x6A7084, t);
        if (t < 0.25f) c = pbDay ? 0xD8A060 : 0xF0F2F8;  // its lit inner lip
      } else if (d >= PB_R - 2) {
        c = 0x1B1B25;
      } else {
        c = scMix(mid, edge, d / PB_R);
        if (pbDay) {
          if (((x * 7 + y * 13) / 2) % 23 == 0) c = scMix(c, 0x2E7A2A, .5);   // grass tufts
          if (((x * 11 + y * 5) / 2) % 97 == 0) c = (x / 2) % 2 ? 0xFFFFFF : 0xF8C8E0;  // a flower
        } else if (x % 24 == 0 && y % 24 == 0) {
          c = scMix(c, 0x8A8AE0, .5);  // the floor's little lights
        }
      }
      pbPx2(x, y, sc565(c));
    }
  // the art on the floor: a ring round the middle and chevrons pointing up the lanes
  fsCircle(CX, 248, 74, pbDay ? 0x5AA84A : 0x4A4EA8, 4);
  fsCircle(CX, 248, 82, pbDay ? 0x5AA84A : 0x4A4EA8, 2);
  for (int k = 0; k < 3; k++) {
    for (int s = -1; s <= 1; s += 2) {
      int cx = CX + s * 132, cy = 300 - k * 22;
      for (int i = 0; i < 10; i += 2) {
        pbPx2(cx - i, cy + i / 2, sc565(pbDay ? 0xA8E080 : 0x6A6ED0));
        pbPx2(cx + i, cy + i / 2, sc565(pbDay ? 0xA8E080 : 0x6A6ED0));
      }
    }
  }
}
// a thick bar from a to b, ra to rb round (the rails and the flippers), on the Knomi's grid
static void pbCapsule(float ax, float ay, float bx, float by, float ra, float rb, Rgb col) {
  float dx = bx - ax, dy = by - ay, L2 = dx * dx + dy * dy, R = ra > rb ? ra : rb;
  int x0 = (int)fminf(ax, bx) - (int)R - 2, x1 = (int)fmaxf(ax, bx) + (int)R + 2;
  int y0 = (int)fminf(ay, by) - (int)R - 2, y1 = (int)fmaxf(ay, by) + (int)R + 2;
  uint16_t c = sc565(col);
  for (int y = SNAP2(y0); y <= y1; y += 2)
    for (int x = SNAP2(x0); x <= x1; x += 2) {
      float px = x + 1 - ax, py = y + 1 - ay, t = L2 > 0 ? (px * dx + py * dy) / L2 : 0;
      t = t < 0 ? 0 : t > 1 ? 1 : t;
      float ex = px - t * dx, ey = py - t * dy, r = ra + (rb - ra) * t;
      if (ex * ex + ey * ey <= r * r) pbPx2(x, y, c);
    }
}
static void pbRail(float ax, float ay, float bx, float by) {  // a steel guide (wood by day), darker than the flippers
  pbCapsule(ax, ay, bx, by, 6, 6, 0x1B1B25);
  pbCapsule(ax, ay, bx, by, 4, 4, pbDay ? 0x8A5A30 : 0x6A7084);
  pbCapsule(ax, ay - 2, bx, by - 2, 1, 1, pbDay ? 0xB07A45 : 0x9AA0B4);  // its lit top
}
// a flipper: side -1 left, 1 right; up: raised
static void pbFlipper(int side, bool up) {
  float px = side < 0 ? PB_FLIP_LX : V2_W - 1 - PB_FLIP_LX, a = up ? PB_FLIP_UP : PB_FLIP_REST;
  float tx = px - side * PB_FLIP_LEN * cosf(a), ty = PB_FLIP_Y + PB_FLIP_LEN * sinf(a);
  pbCapsule(px, PB_FLIP_Y, tx, ty, 13, 8, 0x1B1B25);
  pbCapsule(px, PB_FLIP_Y, tx, ty, 11, 6, 0xE8503A);  // its red rubber
  pbCapsule(px, PB_FLIP_Y, tx, ty, 8, 3, 0xF4F4F8);
  pbCapsule(px, PB_FLIP_Y - 3, tx, ty - 3, 3, 1, 0xFFFFFF);
  pbCapsule(px, PB_FLIP_Y + 4, tx, ty + 3, 3, 1, 0xC8CCD8);  // its shaded underside
  gfx->fillCircle((int)px, PB_FLIP_Y, 5, sc565(0xE8503A));  // the pivot's cap
  gfx->fillCircle((int)px - 1, PB_FLIP_Y - 1, 2, sc565(0xFF9C7A));
}
// the slingshots: a red triangle each side over the flippers, its long side a white rubber that kicks
static void pbSlings(bool litL = false) {
  for (int s = -1; s <= 1; s += 2) {
    int ax = s < 0 ? 82 : V2_W - 1 - 82, bx = s < 0 ? 122 : V2_W - 1 - 122;
    gfx->fillTriangle(ax - s * 4, 286, ax - s * 4, 344, bx + s * 4, 368, sc565(0x1B1B25));
    gfx->fillTriangle(ax, 294, ax, 340, bx, 362, sc565(s < 0 && litL ? 0xFF8A6A : 0xE8503A));
    pbCapsule(ax, 294, bx, 362, 3, 3, s < 0 && litL ? 0xFFF3B0 : 0xF4F4F8);
  }
}
// a bumper: a dark ring, a white collar, its cap; lit: brighter, glowing, a ring of light round it
static void pbBumper(int k, bool lit) {
  int cx = PB_BUMP[k][0], cy = PB_BUMP[k][1];
  Rgb cap = PB_BUMP_COL[k];
  if (lit) {
    twGlow(cx, cy, 52, scMix(cap, 0xFFFFFF, .4), 170);
    fsCircle(cx, cy, 31, 0xFFF3B0, 2);
  }
  gfx->fillCircle(cx + 2, cy + 4, 23, sc565(pbDay ? 0x3E7A30 : 0x15163A));  // its shadow
  gfx->fillCircle(cx, cy, 23, sc565(0x1B1B25));
  gfx->fillCircle(cx, cy, 21, sc565(lit ? 0xFFFFFF : 0xE4E8F0));
  gfx->fillCircle(cx, cy, 16, sc565(0x1B1B25));
  gfx->fillCircle(cx, cy, 14, sc565(lit ? scMix(cap, 0xFFFFFF, .35) : cap));
  gfx->fillCircle(cx - 4, cy - 5, 4, sc565(scMix(cap, 0xFFFFFF, .7)));
}
// a berry light: lit, the berry in full with a glow; not yet, the berry dimmed into the floor
static void pbBerry(int k, bool lit) {
  int x = PB_BERRY[k][0], y = PB_BERRY[k][1];
  static const char *const *const SPR[6] = { SPR_BERRY_ORAN, SPR_BERRY_PINAP, SPR_HOME_FOOD, SPR_BERRY_ORAN, SPR_BERRY_PINAP, SPR_HOME_FOOD };
  if (lit) twGlow(x, y, 26, 0xFFF3B0, 150);
  gfx->fillCircle(x, y, 13, sc565(lit ? 0xFFF3B0 : (pbDay ? 0x3E7A30 : 0x15163A)));
  drawMap(SPR[k], HOME_ICON_N, x - 9, y - 9, 1, false);
  if (!lit) fsBlend(x - 12, y - 12, x + 12, y + 12, pbDay ? 0x4E9A3E : 0x262A6A, 150);
}
// your Pokemon on its pad in the middle, and under it how many hits toward its bonus (of 3)
static void pbPet(int hits, bool hit) {
  int16_t d = pet.speciesId > 0 ? pet.speciesId : 25;
  const uint8_t *t = thumbs.getForm(d, pet.shiny);
  if (hit) twGlow(CX, PB_PET_FEET - 30, 70, 0xFFF3B0, 190);
  pbEllipse(CX, PB_PET_FEET, 42, 13, 0x1B1B25);
  pbEllipse(CX, PB_PET_FEET, 39, 11, hit ? 0xFFE27A : (pbDay ? 0xE8D8A0 : 0x8A8AE0));
  pbEllipse(CX, PB_PET_FEET - 2, 30, 6, hit ? 0xFFF3B0 : (pbDay ? 0xF4ECC8 : 0xAAAAF0));
  fsThumb(t, CX, PB_PET_FEET + 2, 2);
  for (int i = 0; i < 3; i++) {
    int x = CX - 16 + i * 16;
    gfx->fillCircle(x, PB_PET_FEET + 22, 6, sc565(0x1B1B25));
    gfx->fillCircle(x, PB_PET_FEET + 22, 4, sc565(i < hits ? 0xFFE27A : 0x4A4A6A));
  }
}
// the ball: a Poke Ball, (x, y) its middle; trail: where it came from (dx, dy per streak)
static void pbBall(int x, int y, int tdx = 0, int tdy = 0) {
  for (int k = 3; k >= 1 && (tdx || tdy); k--)
    fsBlend(x + tdx * k - 6, y + tdy * k - 6, x + tdx * k + 5, y + tdy * k + 5, 0xFFFFFF, 40 * (4 - k));
  gfx->fillCircle(x + 2, y + 4, PB_BALL_R, sc565(pbDay ? 0x3E7A30 : 0x15163A));  // its shadow
  for (int j = -PB_BALL_R - 1; j <= PB_BALL_R + 1; j++)
    for (int i = -PB_BALL_R - 1; i <= PB_BALL_R + 1; i++) {
      int d2 = i * i + j * j;
      if (d2 > (PB_BALL_R + 1) * (PB_BALL_R + 1)) continue;
      Rgb c = d2 > (PB_BALL_R - 1) * (PB_BALL_R - 1) ? 0x1B1B25 : abs(j) <= 1 ? 0x1B1B25 : j < 0 ? 0xE8503A : 0xF4F4F8;
      if (j < 0 && d2 < 30 && i < -1 && j < -3) c = 0xFF9C7A;  // its shine
      pbPx1(x + i, y + j, sc565(c));
    }
  gfx->fillCircle(x, y, 3, sc565(0x1B1B25));
  gfx->fillCircle(x, y, 2, sc565(0xFFFFFF));
}
// the score and the balls left, at the top
static void pbHud(const char *score, int ballsLeft, const char *mult = nullptr) {
  setSize(3);
  int w = textW(score, 3) + 28;
  gfx->fillRoundRect(CX - w / 2 - 3, 27, w + 6, 36, 10, sc565(0x1B1B25));
  gfx->fillRoundRect(CX - w / 2, 30, w, 30, 8, sc565(0x5A3FA0));
  gfx->setTextColor(UI_WHITE);
  setCur(CX - textW(score, 3) / 2, 34);
  printT(score);
  if (mult) {  // the multiplier, gold, beside it
    setSize(2);
    int mx = CX + w / 2 + 8;
    gfx->fillRoundRect(mx - 3, 33, textW(mult, 2) + 12, 24, 8, sc565(0x1B1B25));
    gfx->fillRoundRect(mx, 36, textW(mult, 2) + 6, 18, 6, sc565(0xF2C230));
    gfx->setTextColor(sc565(0x1B1B25));
    setCur(mx + 3, 38);
    printT(mult);
  }
  for (int i = 0; i < 3; i++) {  // the balls: the ones left in full, the ones used dimmed
    int x = CX - 18 + i * 18, y = 76;
    gfx->fillCircle(x, y, 7, sc565(0x1B1B25));
    if (i < ballsLeft) {
      for (int j = -5; j <= 5; j++)
        for (int i = -5; i <= 5; i++)
          if (i * i + j * j <= 25) pbPx1(x + i, y + j, sc565(j == 0 ? 0x1B1B25 : j < 0 ? 0xE8503A : 0xF4F4F8));
    } else {
      gfx->fillCircle(x, y, 5, sc565(0x4A4A6A));
    }
  }
}
// the left or right half glowing at its rim: where a finger is holding (the Knomi has no buttons for flippers)
static void pbHalfGlow(int side) {
  for (int y = 0; y < V2_W; y += 4)
    for (int x = side < 0 ? 0 : CX; x < (side < 0 ? CX : V2_W); x += 4) {
      float d = 233 - hypotf(x + 2 - CX, y + 2 - CY);
      if (d < 0 || d > 26 || y < 120) continue;
      fsBlend(x, y, x + 3, y + 3, 0xFFE27A, (int)(200 * (1 - d / 26)));
    }
}
// everything that stays put: the table, rails, slingshots, bumpers, berries, your Pokemon, the flippers
static void pbStill(bool upL, bool upR, int berriesLit, int petHits, bool petHit = false, int bumpLit = -1, bool slingL = false) {
  pbTable();
  const float wa = 2.583f;  // the guides from the wall (148 degrees round) in to just above each flipper
  float wx = CX + PB_R * cosf(wa), wy = CY + PB_R * sinf(wa);
  pbRail(wx, wy, PB_FLIP_LX - 8, PB_FLIP_Y - 6);
  pbRail(V2_W - 1 - wx, wy, V2_W - 1 - PB_FLIP_LX + 8, PB_FLIP_Y - 6);
  pbSlings(slingL);
  for (int k = 0; k < 6; k++) pbBerry(k, k < berriesLit || (berriesLit < 0 && k == 1));
  for (int k = 0; k < 3; k++) pbBumper(k, k == bumpLit);
  pbPet(petHits, petHit);
  pbFlipper(-1, upL);
  pbFlipper(1, upR);
}

// ===================================================================== the mock screens
// 1: the start: the ball rolling in at the top, nothing lit yet, a hint over the flippers
static void pbMockStart() {
  pbStill(false, false, 0, 0);
  pbBall(CX, 96, 0, -8);
  pbHud("0", 3);
  fsSay("TAP LEFT OR RIGHT", CX, 332, 0xFFFFFF);
}
// 2: its how-to card over the table
static void pbMockHow() {
  pbStill(false, false, 0, 0);
  pbHud("0", 3);
  v2IntroCard(CX, CY, true, "PINBALL", T(S_HOW_PINBALL));
}
// 3: a flip: a finger on the left half (its rim glows), the left flipper up, the ball flying off it
static void pbMockFlip() {
  pbStill(true, false, 2, 1);
  pbHalfGlow(-1);
  pbBall(196, 330, -4, 8);
  pbHud("1,250", 3);
}
// 4: a bumper: the ball off the right one, which lights up and kicks it, +10
static void pbMockBumper() {
  pbStill(false, false, 2, 1, false, 1);
  pbBall(272, 196, 6, -6);
  for (int k = 0; k < 6; k++) {  // sparks
    float a = k * 1.047f + 0.3f;
    fsSq(306 + (int)(36 * cosf(a)), 168 + (int)(36 * sinf(a)), 4, 0xFFF3B0);
  }
  fsSay("+10", 306, 112, 0xFFE27A, 3);
  pbHud("1,260", 3);
}
// 5: your Pokemon hit: it lights up, +100, the second of three toward its bonus
static void pbMockPet() {
  pbStill(false, false, 3, 2, true);
  pbBall(276, 238, 6, 4);
  fsSay("+100", CX, 196, 0xFFE27A, 3);
  for (int k = 0; k < 4; k++) {  // stars round it
    static const int8_t ST[4][2] = { { -50, -70 }, { 48, -78 }, { 60, -30 }, { -58, -26 } };
    int x = CX + ST[k][0], y = PB_PET_FEET - 30 + ST[k][1];
    scGridRect(x, y - 8, x + 4, y + 12, 0xFFFFFF);
    scGridRect(x - 8, y, x + 12, y + 4, 0xFFFFFF);
  }
  pbHud("1,460", 3);
}
// 6: every berry lit: the score doubles from now on (x2 by the score), the berries all glowing
static void pbMockBerries() {
  pbStill(false, true, 6, 2);
  pbHalfGlow(1);
  pbBall(300, 330, 4, 8);
  fsSay("BERRIES x2!", CX, 196, 0xFFE27A, 4);
  pbHud("3,840", 3, "x2");
}
// 7: the ball lost: down the drain between the flippers, the bottom glowing red, BALL 2 next
static void pbMockDrain() {
  pbStill(false, false, 4, 1);
  for (int y = 300; y < V2_W; y += 4)
    for (int x = 0; x < V2_W; x += 4) {
      float d = 233 - hypotf(x + 2 - CX, y + 2 - CY);
      if (d < 0 || d > 30) continue;
      fsBlend(x, y, x + 3, y + 3, 0xE8503A, (int)(170 * (1 - d / 30)));
    }
  pbBall(CX, 446, 0, -8);
  fsBoard("BALL 2", 196);
  pbHud("2,110", 2);
}
// 8: the end: the game's result as every game shows it (the score, NEW RECORD!, the joy), PLAY AGAIN! and the X
static void pbMockEnd() {
  pbStill(false, false, 4, 1);
  pbHud("12,340", 0);
  v2GameResult("PINBALL", nullptr, "12,340", true, "BEST: 9,870", "+23 HAPPINESS");
}
// 9: the other look, by day: a meadow on a wooden rim (the same moment as 4)
static void pbMockDay() {
  pbDay = true;
  pbMockBumper();
  pbDay = false;
}

static void pinMockRender() {
  switch (pbMock) {
    case 1: pbMockStart(); break;
    case 2: pbMockHow(); break;
    case 3: pbMockFlip(); break;
    case 4: pbMockBumper(); break;
    case 5: pbMockPet(); break;
    case 6: pbMockBerries(); break;
    case 7: pbMockDrain(); break;
    case 8: pbMockEnd(); break;
    case 9: pbMockDay(); break;
  }
}
#else
static void pinMockRender() {}
#endif
