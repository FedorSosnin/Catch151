// V2: Pinball's MOCKUPS (PINMOCK n): still screens drawn in the real game, so the look can be judged before any of it is
// built (as Reel Fishing's were, FISHMOCK n). The plan is docs/PINBALL.md: the round screen is the table, its rim the
// wall. On the floor: an orbit lane round the top with berry lights in it, three bumpers (a Poke Ball, a Great Ball
// and an Ultra Ball) with Diglett's hole between them, your own Pokemon on a pad as a target, a patch of tall grass
// where a wild Pokemon waits (in goes the ball, a battle in the game's own battle scene, out it comes), slingshots, and
// two flippers over the drain. Raised over the floor (Fedor: "ramps and areas the ball can go under and over"): a ramp up the left that
// crosses over the orbit and runs down the rim as a wire rail to the left inlane. Two looks, one each game at random
// (Fedor: "Maybe its random which you get with music to match"): a night-blue table on a chrome rim, or a meadow on a
// wooden one. The ball is a Poke Ball that rolls (Fedor: "make sure the ball spins as it moves"). scene_v2.h includes
// this after fishing_v2.h, whose painters it borrows (fsSay, fsBoard, fsThumb, fsCircle, fsSq, fsBlend). On a board
// nothing calls them: they're a computer's only.

static uint8_t pbMock = 0;  // the mockup on screen (0: none)
void pinMockSet(uint8_t n) { pbMock = n; }

#ifdef BOARD_HOST
// ===================================================================== the table
#define PB_R 218            // the table's inside; the rim wall from here out to the screen's edge
#define PB_FLIP_LX 148      // the left flipper's pivot (the right one's mirrored about the middle)
#define PB_FLIP_Y 390
#define PB_FLIP_LEN 70
#define PB_FLIP_REST 0.50f  // its angle at rest (radians below level, the tip down toward the middle)
#define PB_FLIP_UP -0.42f   // raised
#define PB_BALL_R 10
#define PB_ORBIT_R0 184     // the orbit lane: between its inner rail (this far out) and the wall,
#define PB_ORBIT_A0 192     // from here round the top to here (degrees, 0 to the right, y down: 270 is the top)
#define PB_ORBIT_A1 348
#define PB_WIRE_R 201       // the wire rail: down the left rim over the orbit lane, from where the ramp meets it
#define PB_WIRE_A0 228
#define PB_WIRE_A1 166      // ...to where it drops the ball into the left inlane
#define PB_PET_FEET 296     // your Pokemon on its pad, in the middle
#define PB_GRASS_X 358      // the tall grass on the right
#define PB_GRASS_Y 246
#define PB_GRASS_RX 50
#define PB_GRASS_RY 36
// the way into it: a trodden notch in its lower-left side, from its mouth (on the floor) to its end (in the grass),
// turned to the left flipper
static const float PB_NOTCH[2][2] = { { 314, 280 }, { 344, 254 } };
#define PB_DIG_X 233        // Diglett's hole, between the three bumpers
#define PB_DIG_Y 178
static bool pbDay = false;   // the meadow on a wooden rim (else the night-blue table on a chrome one)
static const int16_t PB_BUMP[3][2] = { { 158, 176 }, { 308, 176 }, { 233, 104 } };
static const int16_t PB_BERRY_A[6] = { 212, 234, 246, 294, 306, 328 };  // the berry lights, in the orbit lane
// the ramp: from its mouth left of the middle, up the left and over the orbit lane, where the wire rail takes over
static const float PB_RAMP[3][2] = { { 146, 266 }, { 104, 198 }, { 98, 84 } };  // start, the curve's pull, end

static inline float pbRad(float deg) { return deg * 0.01745329f; }
static inline void pbPx2(int x, int y, uint16_t c) {  // a Knomi pixel (2 x 2)
  if (x < 0 || y < 0 || x > V2_W - 2 || y > V2_W - 2) return;
  uint16_t *q = scFb + y * V2_W + x;
  q[0] = q[1] = q[V2_W] = q[V2_W + 1] = c;
}
static inline void pbPx1(int x, int y, uint16_t c) {  // one pixel (the ball's fine outline)
  if (x >= 0 && y >= 0 && x < V2_W && y < V2_W) scFb[y * V2_W + x] = c;
}
static void pbEllipse(int cx, int cy, int rx, int ry, Rgb col, int a256 = 0) {
  uint16_t c = sc565(col);
  for (int y = SNAP2(cy - ry); y <= cy + ry; y += 2)
    for (int x = SNAP2(cx - rx); x <= cx + rx; x += 2) {
      float dx = (x + 1 - cx) / (float)rx, dy = (y + 1 - cy) / (float)ry;
      if (dx * dx + dy * dy > 1) continue;
      if (a256) fsBlend(x, y, x + 1, y + 1, col, a256);
      else pbPx2(x, y, c);
    }
}
// a thick bar from a to b, ra to rb round (rails, flippers, the ramp), on the Knomi's grid: solid, or (a256 > 0) col
// laid over what's there that much
static void pbCapsule(float ax, float ay, float bx, float by, float ra, float rb, Rgb col, int a256 = 0) {
  float dx = bx - ax, dy = by - ay, L2 = dx * dx + dy * dy, R = ra > rb ? ra : rb;
  int x0 = (int)fminf(ax, bx) - (int)R - 2, x1 = (int)fmaxf(ax, bx) + (int)R + 2;
  int y0 = (int)fminf(ay, by) - (int)R - 2, y1 = (int)fmaxf(ay, by) + (int)R + 2;
  uint16_t c = sc565(col);
  for (int y = SNAP2(y0); y <= y1; y += 2)
    for (int x = SNAP2(x0); x <= x1; x += 2) {
      float px = x + 1 - ax, py = y + 1 - ay, t = L2 > 0 ? (px * dx + py * dy) / L2 : 0;
      t = t < 0 ? 0 : t > 1 ? 1 : t;
      float ex = px - t * dx, ey = py - t * dy, r = ra + (rb - ra) * t;
      if (ex * ex + ey * ey > r * r) continue;
      if (a256) fsBlend(x, y, x + 1, y + 1, col, a256);
      else pbPx2(x, y, c);
    }
}
static Rgb pbFloorDark() { return pbDay ? 0x3E7A30 : 0x15163A; }  // shadows on the floor
static Rgb pbSteel() { return pbDay ? 0x8A5A30 : 0x6A7084; }
static Rgb pbSteelLit() { return pbDay ? 0xB07A45 : 0x9AA0B4; }

// ===================================================================== the ball: a Poke Ball that rolls
// Seen from above, it turns as it goes: each pixel of it is a point on the sphere, turned back by how far the ball has
// rolled (about the axis across its way), and coloured by where that lands on the ball: the red half, the white half,
// the black band round its middle and the button on it. Lit from the top left, so it reads as round however it's
// turned. (dx, dy): its way; roll: how far it has gone, in its own radii
static void pbBallSphere(int x, int y, int r, float dx, float dy, float roll, bool dark) {
  float L = hypotf(dx, dy);
  float kx = L > 0 ? -dy / L : 0, ky = L > 0 ? dx / L : 1;  // the axis it turns about: across its way
  float c = cosf(-roll), s = sinf(-roll);
  const float lx = -0.45f, ly = -0.55f, lz = 0.70f;  // the light
  for (int j = -r - 1; j <= r + 1; j++)
    for (int i = -r - 1; i <= r + 1; i++) {
      float d2 = (float)(i * i + j * j);
      if (d2 > (r + 1) * (r + 1)) continue;
      if (d2 > (r - 0.5f) * (r - 0.5f)) {  // its dark edge
        pbPx1(x + i, y + j, sc565(0x1B1B25));
        continue;
      }
      float px = i / (float)r, py = j / (float)r, pz = sqrtf(fmaxf(0, 1 - px * px - py * py));
      // turned back by the roll (Rodrigues' formula, about k = (kx, ky, 0))
      float kp = kx * px + ky * py;
      float cx_ = ky * pz, cy_ = -kx * pz, cz_ = kx * py - ky * px;  // k x p
      float qx = px * c + cx_ * s + kx * kp * (1 - c);
      float qy = py * c + cy_ * s + ky * kp * (1 - c);
      float qz = pz * c + cz_ * s;
      (void)qx;
      // the ball's own markings: red to its top (-y), white below, the band round the middle, the button facing out (+z)
      Rgb col = qy < -0.13f ? 0xE8503A : qy > 0.13f ? 0xF4F4F8 : 0x1B1B25;
      if (qz > 0.93f) col = 0xF4F4F8;
      else if (qz > 0.84f) col = 0x1B1B25;
      float lit = px * lx + py * ly + pz * lz;  // shading: darker away from the light, a shine near it
      col = scMix(col, 0x101020, (1 - lit) * 0.42f);
      if (lit > 0.93f) col = scMix(col, 0xFFFFFF, 0.55f);
      if (dark) col = scMix(col, 0x101030, .4);
      pbPx1(x + i, y + j, sc565(col));
    }
}
// the ball on the table: its shadow, a faint streak behind it, then it, rolled
static void pbBall(int x, int y, float dx, float dy, float roll, int r = PB_BALL_R, int sh = 4, bool dark = false) {
  float L = hypotf(dx, dy);
  if (L > 0)
    for (int k = 3; k >= 1; k--) {
      int tx = x - (int)(dx / L * 7 * k), ty = y - (int)(dy / L * 7 * k);
      fsBlend(tx - 6, ty - 6, tx + 5, ty + 5, 0xFFFFFF, 30 * (4 - k));
    }
  gfx->fillCircle(x + sh / 2, y + sh, r, sc565(pbFloorDark()));  // its shadow (further off on the ramp)
  pbBallSphere(x, y, r, dx, dy, roll, dark);
}

// ===================================================================== the floor
static void pbTable() {
  scScreen();
  const Rgb mid = pbDay ? 0x7CC85A : 0x34388A, edge = pbDay ? 0x4E9A3E : 0x1C1E4A;
  for (int y = 0; y < V2_W; y += 2)
    for (int x = 0; x < V2_W; x += 2) {
      float d = hypotf(x + 1 - CX, y + 1 - CY);
      float a = atan2f(y + 1 - CY, x + 1 - CX) * 57.29578f;
      if (a < 0) a += 360;
      Rgb c;
      if (d >= 233) {  // past the round screen (only a screenshot shows it)
        c = 0x101018;
      } else if (d >= PB_R + 2) {  // the rim: chrome by night, wood by day, its inner edge lit
        float t = (d - PB_R - 2) / (233 - PB_R - 2);
        c = pbDay ? scMix(0xB07A45, 0x6E4426, t) : scMix(0xC8CCD8, 0x6A7084, t);
        if (t < 0.25f) c = pbDay ? 0xD8A060 : 0xF0F2F8;
        if (pbDay && ((int)(a * 3) % 7 == 0)) c = scMix(c, 0x5A3820, .4);  // the wood's grain
      } else if (d >= PB_R - 2) {
        c = 0x1B1B25;
      } else {
        c = scMix(mid, edge, d / PB_R);
        bool lane = d > PB_ORBIT_R0 && a >= PB_ORBIT_A0 && a <= PB_ORBIT_A1;
        if (lane) c = scMix(c, pbDay ? 0x2E6A2A : 0x101236, .35);  // the orbit lane, a shade darker
        if (pbDay) {
          if (((x * 7 + y * 13) / 2) % 23 == 0) c = scMix(c, 0x2E7A2A, .5);           // grass tufts
          if (((x * 11 + y * 5) / 2) % 97 == 0) c = (x / 2) % 2 ? 0xFFFFFF : 0xF8C8E0;  // a flower
        } else {
          if (x % 24 == 0 && y % 24 == 0) c = scMix(c, 0x8A8AE0, .5);  // the floor's little lights
          if (((x * 13 + y * 7) / 2) % 151 == 0) c = 0xC8CCF8;           // a star
        }
      }
      pbPx2(x, y, sc565(c));
    }
  // the art on the floor: a ring round your Pokemon's pad, chevrons up the outlanes
  fsCircle(CX, PB_PET_FEET - 10, 54, pbDay ? 0x5AA84A : 0x4A4EA8, 4);
  for (int k = 0; k < 3; k++)
    for (int s = -1; s <= 1; s += 2) {
      int cx = CX + s * 150, cy = 330 - k * 18;
      for (int i = 0; i < 10; i += 2) {
        pbPx2(cx - i, cy + i / 2, sc565(pbDay ? 0xA8E080 : 0x6A6ED0));
        pbPx2(cx + i, cy + i / 2, sc565(pbDay ? 0xA8E080 : 0x6A6ED0));
      }
    }
}
// chevrons on the floor at (x, y) pointing along (dx, dy): a lit arrow insert showing a shot's way in
static void pbArrow(int x, int y, float dx, float dy, Rgb col) {
  float L = hypotf(dx, dy), ux = dx / L, uy = dy / L, nx = -uy, ny = ux;
  for (int k = 0; k < 3; k++) {
    float cx = x + ux * k * 12, cy = y + uy * k * 12;
    pbCapsule(cx - ux * 6 + nx * 8, cy - uy * 6 + ny * 8, cx, cy, 2, 2, col);
    pbCapsule(cx - ux * 6 - nx * 8, cy - uy * 6 - ny * 8, cx, cy, 2, 2, col);
  }
}
// the orbit's inner rail, an arc of short bars, with a post at each end
static void pbOrbit() {
  for (int pass = 0; pass < 2; pass++)
    for (int a = PB_ORBIT_A0; a < PB_ORBIT_A1; a += 4) {
      float a0 = pbRad(a), a1 = pbRad(a + 4 > PB_ORBIT_A1 ? PB_ORBIT_A1 : a + 4);
      float x0 = CX + PB_ORBIT_R0 * cosf(a0), y0 = CY + PB_ORBIT_R0 * sinf(a0);
      float x1 = CX + PB_ORBIT_R0 * cosf(a1), y1 = CY + PB_ORBIT_R0 * sinf(a1);
      if (pass == 0) {
        pbCapsule(x0 + 2, y0 + 4, x1 + 2, y1 + 4, 5, 5, pbFloorDark(), 120);  // its shadow
      } else {
        pbCapsule(x0, y0, x1, y1, 6, 6, 0x1B1B25);
        pbCapsule(x0, y0, x1, y1, 4, 4, pbSteel());
        pbCapsule(x0, y0 - 1, x1, y1 - 1, 1, 1, pbSteelLit());
      }
    }
  for (int e = 0; e < 2; e++) {  // its two posts, a rubber ring round each
    float a = pbRad(e ? PB_ORBIT_A1 : PB_ORBIT_A0);
    int x = (int)(CX + PB_ORBIT_R0 * cosf(a)), y = (int)(CY + PB_ORBIT_R0 * sinf(a));
    gfx->fillCircle(x, y, 8, sc565(0x1B1B25));
    gfx->fillCircle(x, y, 6, sc565(0xE8503A));
    gfx->fillCircle(x, y, 3, sc565(0xF4F4F8));
  }
}
static void pbRail(float ax, float ay, float bx, float by) {  // a guide on the floor (wood by day), darker than flippers
  pbCapsule(ax + 2, ay + 4, bx + 2, by + 4, 5, 5, pbFloorDark(), 120);
  pbCapsule(ax, ay, bx, by, 6, 6, 0x1B1B25);
  pbCapsule(ax, ay, bx, by, 4, 4, pbSteel());
  pbCapsule(ax, ay - 2, bx, by - 2, 1, 1, pbSteelLit());
}
// a flipper: side -1 left, 1 right; up: raised
static void pbFlipper(int side, bool up) {
  float px = side < 0 ? PB_FLIP_LX : V2_W - 1 - PB_FLIP_LX, a = up ? PB_FLIP_UP : PB_FLIP_REST;
  float tx = px - side * PB_FLIP_LEN * cosf(a), ty = PB_FLIP_Y + PB_FLIP_LEN * sinf(a);
  pbCapsule(px + 2, PB_FLIP_Y + 5, tx + 2, ty + 5, 11, 6, pbFloorDark(), 130);  // its shadow
  pbCapsule(px, PB_FLIP_Y, tx, ty, 13, 8, 0x1B1B25);
  pbCapsule(px, PB_FLIP_Y, tx, ty, 11, 6, 0xE8503A);  // its red rubber
  pbCapsule(px, PB_FLIP_Y, tx, ty, 8, 3, 0xF4F4F8);
  pbCapsule(px, PB_FLIP_Y - 3, tx, ty - 3, 3, 1, 0xFFFFFF);
  pbCapsule(px, PB_FLIP_Y + 4, tx, ty + 3, 3, 1, 0xC8CCD8);  // its shaded underside
  gfx->fillCircle((int)px, PB_FLIP_Y, 5, sc565(0xE8503A));  // the pivot's cap
  gfx->fillCircle((int)px - 1, PB_FLIP_Y - 1, 2, sc565(0xFF9C7A));
}
// the slingshots: a red triangle each side over the flippers, a lit corner, its long side a white rubber that kicks
static void pbSlings() {
  for (int s = -1; s <= 1; s += 2) {
    int ax = s < 0 ? 82 : V2_W - 1 - 82, bx = s < 0 ? 122 : V2_W - 1 - 122;
    gfx->fillTriangle(ax - s * 4 + 2, 290, ax - s * 4 + 2, 348, bx + s * 4 + 2, 372, sc565(pbFloorDark()));
    gfx->fillTriangle(ax - s * 4, 286, ax - s * 4, 344, bx + s * 4, 368, sc565(0x1B1B25));
    gfx->fillTriangle(ax, 294, ax, 340, bx, 362, sc565(0xE8503A));
    gfx->fillTriangle(ax, 294, ax, 312, ax + (bx - ax) / 3, 316, sc565(0xFF8A6A));  // its lit corner
    pbCapsule(ax, 294, bx, 362, 3, 3, 0xF4F4F8);
  }
}
// a bumper, its cap painted as a ball: 0 a Poke Ball, 1 a Great Ball, 2 an Ultra Ball; lit: bright, glowing, a ring of
// light round it
static void pbBumper(int k, bool lit) {
  int cx = PB_BUMP[k][0], cy = PB_BUMP[k][1];
  static const Rgb TOP[3] = { 0xE8503A, 0x3A78D8, 0x2A2A34 };
  if (lit) {
    twGlow(cx, cy, 54, 0xFFF3B0, 170);
    fsCircle(cx, cy, 31, 0xFFF3B0, 2);
  }
  gfx->fillCircle(cx + 3, cy + 5, 23, sc565(pbFloorDark()));  // its shadow
  gfx->fillCircle(cx, cy, 23, sc565(0x1B1B25));
  gfx->fillCircle(cx, cy, 21, sc565(lit ? 0xFFF3B0 : 0xC8CCD8));  // its skirt
  gfx->fillCircle(cx, cy - 1, 19, sc565(lit ? 0xFFFFFF : 0xE4E8F0));
  gfx->fillCircle(cx, cy, 16, sc565(0x1B1B25));
  for (int j = -14; j <= 14; j++)  // the cap: the ball's top half, its white half, the band
    for (int i = -14; i <= 14; i++) {
      if (i * i + j * j > 14 * 14) continue;
      Rgb c = abs(j) <= 1 ? 0x1B1B25 : j < 0 ? TOP[k] : 0xF4F4F8;
      if (j < 0 && k == 1 && (abs(i + 7) <= 2 || abs(i - 7) <= 2) && j > -11) c = 0xE8503A;  // the Great Ball's marks
      if (j < 0 && k == 2 && (abs(i + 7) <= 2 || abs(i - 7) <= 2) && j < -2) c = 0xF2C230;   // the Ultra Ball's
      if (lit) c = scMix(c, 0xFFFFFF, .3);
      pbPx1(cx + i, cy + j, sc565(c));
    }
  gfx->fillCircle(cx, cy, 5, sc565(0x1B1B25));
  gfx->fillCircle(cx, cy, 3, sc565(0xF4F4F8));
  gfx->fillCircle(cx - 6, cy - 8, 2, sc565(0xFFFFFF));  // its shine
}
// a berry light in the orbit lane: lit, the berry in full with a glow; not yet, dimmed into the floor
static void pbBerry(int k, bool lit) {
  float a = pbRad(PB_BERRY_A[k]);
  int x = (int)(CX + PB_WIRE_R * cosf(a)), y = (int)(CY + PB_WIRE_R * sinf(a));
  static const char *const *const SPR[6] = { SPR_BERRY_ORAN, SPR_BERRY_PINAP, SPR_HOME_FOOD, SPR_HOME_FOOD, SPR_BERRY_PINAP, SPR_BERRY_ORAN };
  if (lit) twGlow(x, y, 24, 0xFFF3B0, 150);
  gfx->fillCircle(x, y, 12, sc565(lit ? 0xFFF3B0 : pbFloorDark()));
  drawMap(SPR[k], HOME_ICON_N, x - 9, y - 9, 1, false);
  if (!lit) fsBlend(x - 12, y - 12, x + 12, y + 12, pbDay ? 0x4E9A3E : 0x1E2056, 150);
}
// your Pokemon on its pad, and under it how many hits toward its bonus (of 3)
static void pbPet(int hits, bool hit) {
  int16_t d = pet.speciesId > 0 ? pet.speciesId : 25;
  const uint8_t *t = thumbs.getForm(d, pet.shiny);
  if (hit) twGlow(CX, PB_PET_FEET - 30, 70, 0xFFF3B0, 190);
  pbEllipse(CX + 2, PB_PET_FEET + 4, 40, 13, pbFloorDark(), 130);
  pbEllipse(CX, PB_PET_FEET, 40, 13, 0x1B1B25);
  pbEllipse(CX, PB_PET_FEET, 37, 11, hit ? 0xFFE27A : (pbDay ? 0xE8D8A0 : 0x8A8AE0));
  pbEllipse(CX, PB_PET_FEET - 2, 28, 6, hit ? 0xFFF3B0 : (pbDay ? 0xF4ECC8 : 0xAAAAF0));
  fsThumb(t, CX, PB_PET_FEET + 2, 2);
  for (int i = 0; i < 3; i++) {
    int x = CX - 16 + i * 16;
    gfx->fillCircle(x, PB_PET_FEET + 22, 6, sc565(0x1B1B25));
    gfx->fillCircle(x, PB_PET_FEET + 22, 4, sc565(i < hits ? 0xFFE27A : 0x4A4A6A));
  }
}
// Diglett's hole between the bumpers: a ring of dirt; up: Diglett out of it, the time to hit it a gold ring round it
// (hit: knocked dizzy, stars round its head)
static void pbDiglett(bool up, bool hit = false, float timeLeft = 0) {
  const Rgb dirt = pbDay ? 0x8A6A44 : 0x6A5A6E, dirtLit = pbDay ? 0xB08A5A : 0x8A7A90;
  if (up && !hit)
    for (int k = 0; k < 360; k += 3) {
      if (k > timeLeft * 360) break;
      float a = pbRad(k - 90);
      fsSq(PB_DIG_X + (int)(30 * cosf(a)), PB_DIG_Y - 18 + (int)(30 * sinf(a)), 4, 0xF2C230);
    }
  pbEllipse(PB_DIG_X, PB_DIG_Y, 20, 8, 0x1B1B25);
  pbEllipse(PB_DIG_X, PB_DIG_Y, 18, 6, dirtLit);
  pbEllipse(PB_DIG_X, PB_DIG_Y + 1, 12, 4, up ? dirt : 0x1B1B25);
  if (!up) return;
  const uint8_t *t = thumbs.getForm(50, false);
  fsThumb(t, PB_DIG_X, PB_DIG_Y + 2, 2);
  if (hit) fsThumb(t, PB_DIG_X, PB_DIG_Y + 2, 2, false, 0xFFFFFF, 110);  // a white flash over it
  pbEllipse(PB_DIG_X, PB_DIG_Y + 3, 18, 5, dirtLit);  // the dirt round its foot
  if (!hit)
    for (int k = 0; k < 6; k++)  // clods thrown up as it pops out
      pbEllipse(PB_DIG_X - 26 + k * 10 + (k % 2) * 3, PB_DIG_Y - 6 - (k % 3) * 7, 3, 3, dirtLit);
  if (hit)
    for (int k = 0; k < 5; k++) {  // dizzy stars
      float a = k * 1.2566f;
      int x = PB_DIG_X + (int)(22 * cosf(a)), y = PB_DIG_Y - 44 + (int)(8 * sinf(a));
      scGridRect(x, y - 4, x + 4, y + 8, 0xFFE27A);
      scGridRect(x - 4, y, x + 8, y + 4, 0xFFE27A);
    }
}
// Tall grass in the spirit of the Game Boy games (Fedor: "grass that resembles grass from Pokemon Blue"): an original
// 16 px tile, 8 x 8 Knomi pixels of staggered tufts, each a dark edge round light blades, on a mid green. Tiled to a
// fixed grid so neighbouring patches line up; rustling, every other row of tiles shifts a Knomi pixel
static const char PB_TUFT[8][9] = {
  "........",
  ".D...D..",
  "DLD.DLD.",
  "DLLDLLD.",
  "....D...",
  "..D...D.",
  ".DLD.DLD",
  ".DLLDLLD",
};
static void pbGrassCols(Rgb &base, Rgb &lite, Rgb &dark) {
  base = pbDay ? 0x4E9A3E : 0x2A6A4E;
  lite = pbDay ? 0xA8E080 : 0x6ED09A;
  dark = pbDay ? 0x2E5A22 : 0x14402E;
}
static inline uint16_t pbTuftAt(int x, int y, bool rustle, uint16_t b, uint16_t l, uint16_t d) {
  if (rustle && (y / 16) % 2) x += 2;
  char c = PB_TUFT[(y / 2) & 7][(x / 2) & 7];
  return c == 'L' ? l : c == 'D' ? d : b;
}
// the tiles inside an oval (cx, cy, rx, ry), from row y0 down; skip: leave the notch out (the table's patch)
static void pbGrassOval(int cx, int cy, int rx, int ry, bool rustle, int y0 = 0, bool skip = false) {
  Rgb b, l, d;
  pbGrassCols(b, l, d);
  const uint16_t B = sc565(b), L = sc565(l), D = sc565(d);
  for (int y = SNAP2(cy - ry); y <= cy + ry; y += 2) {
    if (y < y0) continue;
    for (int x = SNAP2(cx - rx); x <= cx + rx; x += 2) {
      float ex = (x + 1 - cx) / (float)rx, ey = (y + 1 - cy) / (float)ry;
      if (ex * ex + ey * ey > 1) continue;
      if (skip) {  // in the notch: the trodden way in
        float ax = PB_NOTCH[0][0], ay = PB_NOTCH[0][1], dx = PB_NOTCH[1][0] - ax, dy = PB_NOTCH[1][1] - ay;
        float t = ((x + 1 - ax) * dx + (y + 1 - ay) * dy) / (dx * dx + dy * dy);
        t = t < 0 ? 0 : t > 1 ? 1 : t;
        float qx = x + 1 - ax - t * dx, qy = y + 1 - ay - t * dy;
        if (qx * qx + qy * qy < 12 * 12) continue;
      }
      pbPx2(x, y, pbTuftAt(x, y, rustle, B, L, D));
    }
  }
}
// tufts poking up along an oval's top edge (from a0 to a1 degrees round it), for height
static void pbGrassTufts(int cx, int cy, int rx, int ry, int a0, int a1, bool rustle) {
  Rgb b, l, d;
  pbGrassCols(b, l, d);
  for (int a = a0; a <= a1; a += 9) {
    float r = pbRad(a);
    int x = SNAP2(cx + (int)(rx * cosf(r))), y = SNAP2(cy + (int)(ry * sinf(r)));
    int sway = rustle ? ((a / 9) % 2 ? 2 : -2) : 0;
    for (int k = 0; k < 2; k++) {  // a tuft: two blades, a dark edge round each
      int bx = x - 4 + k * 6 + sway;
      scRect(bx, y - 8, bx + 1, y - 7, d);
      scRect(bx - 2, y - 6, bx + 3, y - 1, d);
      scRect(bx, y - 6, bx + 1, y - 1, l);
    }
  }
}
// the floor's own colour at (x, y), under where something was drawn (the notch shows it through the grass)
static Rgb pbFloorAt(int x, int y) {
  const Rgb mid = pbDay ? 0x7CC85A : 0x34388A, edge = pbDay ? 0x4E9A3E : 0x1C1E4A;
  return scMix(mid, edge, hypotf(x + 1 - CX, y + 1 - CY) / PB_R);
}
// the patch on the table, where wild Pokemon wait (lit: rustling, something's in it). The way in is a gap where the
// grass parts on its lower-left side (Fedor: the stone hole "still doesnt look good"): the floor shows through it, the
// grass's dark edge runs down both its sides, tufts lean out over them, and it darkens toward its end, where the grass
// closes over the ball. A lit arrow on the floor points up it from the left flipper
static void pbGrass(bool lit) {
  pbEllipse(PB_GRASS_X + 4, PB_GRASS_Y + 8, PB_GRASS_RX + 2, PB_GRASS_RY + 2, pbFloorDark(), 140);  // its shadow
  pbEllipse(PB_GRASS_X, PB_GRASS_Y, PB_GRASS_RX + 3, PB_GRASS_RY + 3, 0x1B1B25);
  pbGrassOval(PB_GRASS_X, PB_GRASS_Y, PB_GRASS_RX, PB_GRASS_RY, lit, 0, true);
  float ax = PB_NOTCH[0][0], ay = PB_NOTCH[0][1], bx = PB_NOTCH[1][0], by = PB_NOTCH[1][1];
  float dx = bx - ax, dy = by - ay, L2 = dx * dx + dy * dy, L = sqrtf(L2);
  for (int y = SNAP2((int)fminf(ay, by) - 12); y <= fmaxf(ay, by) + 12; y += 2)  // the gap: the floor, shaded deeper in
    for (int x = SNAP2((int)fminf(ax, bx) - 12); x <= fmaxf(ax, bx) + 12; x += 2) {
      float ex = (x + 1 - PB_GRASS_X) / (float)(PB_GRASS_RX + 3), ey = (y + 1 - PB_GRASS_Y) / (float)(PB_GRASS_RY + 3);
      if (ex * ex + ey * ey > 1) continue;
      float t = ((x + 1 - ax) * dx + (y + 1 - ay) * dy) / L2;
      t = t < 0 ? 0 : t > 1 ? 1 : t;
      float qx = x + 1 - ax - t * dx, qy = y + 1 - ay - t * dy;
      if (qx * qx + qy * qy > 9.5f * 9.5f) continue;
      pbPx2(x, y, sc565(scMix(pbFloorAt(x, y), 0x0A0A12, 0.15f + 0.55f * t)));
    }
  Rgb b, l, d;
  pbGrassCols(b, l, d);
  for (int side = -1; side <= 1; side += 2)  // tufts leaning out over the gap's two sides
    for (float t = 0.35f; t <= 1.0f; t += 0.3f) {
      int x = SNAP2((int)(ax + dx * t - dy / L * 11 * side)), y = SNAP2((int)(ay + dy * t + dx / L * 11 * side));
      scRect(x - 2, y - 6, x + 3, y - 1, d);
      scRect(x + side * 2, y - 8, x + side * 2 + 1, y - 7, d);
      scRect(x, y - 6, x + 1, y - 1, l);
    }
  pbGrassTufts(PB_GRASS_X, PB_GRASS_Y, PB_GRASS_RX, PB_GRASS_RY, 196, 344, lit);
  pbArrow((int)(ax - dx * 1.05f), (int)(ay - dy * 1.05f), dx, dy, lit ? 0xFFE27A : 0xB06AE0);
}
// the patch's front, over a ball that has gone into it (from row y down): only its lower half shows over the blades
static void pbGrassFront(int y, bool lit) { pbGrassOval(PB_GRASS_X, PB_GRASS_Y, PB_GRASS_RX, PB_GRASS_RY, lit, SNAP2(y), true); }
// the ramp's middle line at t (0 its mouth, 1 its top)
static void pbRampAt(float t, float &x, float &y) {
  float u = 1 - t;
  x = u * u * PB_RAMP[0][0] + 2 * u * t * PB_RAMP[1][0] + t * t * PB_RAMP[2][0];
  y = u * u * PB_RAMP[0][1] + 2 * u * t * PB_RAMP[1][1] + t * t * PB_RAMP[2][1];
}
// the wire rail's middle at angle a (degrees)
static void pbWireAt(float a, float r, float &x, float &y) { x = CX + r * cosf(pbRad(a)), y = CY + r * sinf(pbRad(a)); }
// what's raised over the floor: the ramp (a see-through plastic lane between two walls, rising, so its shadow falls
// further off toward its top) and the wire rail it becomes, two wires on ties down the rim over the orbit, to a cup
// where it lets the ball down into the left inlane
static void pbRaised(bool rampLit = false) {
  const int N = 22;
  float px[N + 1], py[N + 1];
  for (int i = 0; i <= N; i++) pbRampAt(i / (float)N, px[i], py[i]);
  for (int i = 0; i < N; i++) {  // the ramp's shadow
    float t = i / (float)N, ox = 4 + 9 * t, oy = 6 + 12 * t;
    pbCapsule(px[i] + ox, py[i] + oy, px[i + 1] + ox, py[i + 1] + oy, 17, 17, pbFloorDark(), 110);
  }
  for (float a = PB_WIRE_A1; a < PB_WIRE_A0; a += 3) {  // the wire's shadow
    float x0, y0, x1, y1;
    pbWireAt(a, PB_WIRE_R, x0, y0);
    pbWireAt(a + 3, PB_WIRE_R, x1, y1);
    pbCapsule(x0 + 10, y0 + 14, x1 + 10, y1 + 14, 9, 9, pbFloorDark(), 70);
  }
  for (int i = 0; i < N; i++)  // the ramp's see-through floor, with lit stripes across it
    pbCapsule(px[i], py[i], px[i + 1], py[i + 1], 14, 14, rampLit ? 0xFFF3B0 : (pbDay ? 0xF8ECC0 : 0xA8C4F8), 105);
  for (int i = 2; i < N; i += 3) {
    float dx = px[i + 1] - px[i], dy = py[i + 1] - py[i], L = hypotf(dx, dy), nx = -dy / L * 11, ny = dx / L * 11;
    pbCapsule(px[i] + nx, py[i] + ny, px[i] - nx, py[i] - ny, 1, 1, rampLit ? 0xFFFFFF : (pbDay ? 0xFFF8E0 : 0xD8E4FF), 140);
  }
  for (int side = -1; side <= 1; side += 2)  // its two walls: a dark edge, then the wall (the outer lit, the inner shaded)
    for (int pass = 0; pass < 2; pass++)
      for (int i = 0; i < N; i++) {
        float dx = px[i + 1] - px[i], dy = py[i + 1] - py[i], L = hypotf(dx, dy), nx = -dy / L * 15 * side, ny = dx / L * 15 * side;
        if (pass == 0) {
          pbCapsule(px[i] + nx, py[i] + ny, px[i + 1] + nx, py[i + 1] + ny, 5, 5, 0x1B1B25);
        } else {
          pbCapsule(px[i] + nx, py[i] + ny, px[i + 1] + nx, py[i + 1] + ny, 3, 3, side < 0 ? 0xE4E8F0 : 0x9AA0B4);
          pbCapsule(px[i] + nx, py[i] + ny - 2, px[i + 1] + nx, py[i + 1] + ny - 2, 1, 1, 0xFFFFFF);
        }
      }
  // its mouth: a lit lip across it, and the arrow on the floor showing the shot up it
  float mx = px[0], my = py[0], dx = px[1] - px[0], dy = py[1] - py[0], L = hypotf(dx, dy);
  pbCapsule(mx - dy / L * 16, my + dx / L * 16, mx + dy / L * 16, my - dx / L * 16, 4, 4, 0x1B1B25);
  pbCapsule(mx - dy / L * 16, my + dx / L * 16, mx + dy / L * 16, my - dx / L * 16, 2, 2, rampLit ? 0xFFFFFF : 0xFFE27A);
  pbArrow((int)(mx - dx / L * 44), (int)(my - dy / L * 44), dx, dy, rampLit ? 0xFFE27A : 0x4FB0E8);
  // the wire rail: two wires on ties, from the ramp's top down the rim
  for (float a = PB_WIRE_A1; a < PB_WIRE_A0; a += 9) {  // the ties
    float x0, y0, x1, y1;
    pbWireAt(a, PB_WIRE_R - 10, x0, y0);
    pbWireAt(a, PB_WIRE_R + 10, x1, y1);
    pbCapsule(x0, y0, x1, y1, 3, 3, 0x1B1B25);
    pbCapsule(x0, y0, x1, y1, 1, 1, 0x9AA0B4);
  }
  for (int w = -1; w <= 1; w += 2)
    for (int pass = 0; pass < 2; pass++)
      for (float a = PB_WIRE_A1; a < PB_WIRE_A0; a += 3) {
        float x0, y0, x1, y1;
        pbWireAt(a, PB_WIRE_R + w * 9, x0, y0);
        pbWireAt(a + 3, PB_WIRE_R + w * 9, x1, y1);
        if (pass == 0) {
          pbCapsule(x0, y0, x1, y1, 3, 3, 0x1B1B25);
        } else {
          pbCapsule(x0, y0, x1, y1, 2, 2, 0xD8DCE8);
          pbCapsule(x0, y0 - 1, x1, y1 - 1, 1, 1, 0xFFFFFF);
        }
      }
  float ex, ey;  // where it lets the ball down: a little cup at its end
  pbWireAt(PB_WIRE_A1, PB_WIRE_R, ex, ey);
  gfx->fillCircle((int)ex, (int)ey, 12, sc565(0x1B1B25));
  gfx->fillCircle((int)ex, (int)ey, 9, sc565(0xB8BED0));
  gfx->fillCircle((int)ex, (int)ey, 5, sc565(0x4A4E66));
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
    if (i < ballsLeft) pbBallSphere(x, y, 6, 0, 1, 0, false);
    else gfx->fillCircle(x, y, 5, sc565(0x4A4A6A));
  }
}
// the left or right half glowing at its rim: where a finger is holding (or the key down, on a computer)
static void pbHalfGlow(int side) {
  for (int y = 0; y < V2_W; y += 4)
    for (int x = side < 0 ? 0 : CX; x < (side < 0 ? CX : V2_W); x += 4) {
      float d = 233 - hypotf(x + 2 - CX, y + 2 - CY);
      if (d < 0 || d > 26 || y < 120) continue;
      fsBlend(x, y, x + 3, y + 3, 0xFFE27A, (int)(200 * (1 - d / 26)));
    }
}
// a key on a computer's keyboard, (x, y) its top left: a pale cap, a dark edge, and on it an arrow (-1 left, 1 right)
// or a word; its width back
static int pbKey(int x, int y, int arrow, const char *word = nullptr) {
  setSize(2);
  int w = word ? textW(word, 2) + 16 : 30, h = 28;
  gfx->fillRoundRect(x - 2, y - 2, w + 4, h + 6, 7, sc565(0x1B1B25));
  gfx->fillRoundRect(x, y, w, h + 2, 6, sc565(0xA8ACBC));  // its side
  gfx->fillRoundRect(x, y, w, h - 2, 6, sc565(0xF0F2F8));  // its top
  if (word) {
    gfx->setTextColor(sc565(0x1B1B25));
    setCur(x + 8, y + 6);
    printT(word);
  } else {
    int cx = x + w / 2, cy = y + (h - 2) / 2;
    if (arrow < 0) gfx->fillTriangle(cx - 6, cy, cx + 5, cy - 7, cx + 5, cy + 7, sc565(0x1B1B25));
    else gfx->fillTriangle(cx + 6, cy, cx - 5, cy - 7, cx - 5, cy + 7, sc565(0x1B1B25));
  }
  return w;
}
// everything on the floor, in its look: the table, the orbit, the berries, guides, slingshots, Diglett's hole, the
// bumpers, your Pokemon, the battle spot, the flippers. A ball on the floor goes after this, then pbRaised() over it,
// then a ball on the ramp
struct PbState {
  bool upL = false, upR = false;
  int berries = 0, petHits = 0, bumpLit = -1;  // bumpLit: one bumper lit; 3: all three
  bool petHit = false, spotLit = false, digUp = false, digHit = false;
  float digTime = 0;
};
static void pbStill(const PbState &s) {
  pbTable();
  pbOrbit();
  for (int k = 0; k < 6; k++) pbBerry(k, k < s.berries);
  const float wa = 2.583f;  // the inlane guides: from the wall (148 degrees round) in to just above each flipper
  float wx = CX + PB_R * cosf(wa), wy = CY + PB_R * sinf(wa);
  pbRail(wx, wy, PB_FLIP_LX - 8, PB_FLIP_Y - 6);
  pbRail(V2_W - 1 - wx, wy, V2_W - 1 - PB_FLIP_LX + 8, PB_FLIP_Y - 6);
  pbSlings();
  pbDiglett(s.digUp, s.digHit, s.digTime);
  for (int k = 0; k < 3; k++) pbBumper(k, s.bumpLit == k || s.bumpLit == 3);
  pbPet(s.petHits, s.petHit);
  pbGrass(s.spotLit);
  pbFlipper(-1, s.upL);
  pbFlipper(1, s.upR);
}

// ===================================================================== the battle
// In goes the ball, and the game's own battle takes the screen (Fedor: "the battle scene should be a bit stronger"):
// the wild battle's scene with its two bases, a wild Pokemon of the zone you're in standing in tall grass on its base,
// yours from behind, both HP boxes, and your Pokemon's two real moves as the buttons (Fedor: "Tapping to attack should
// do your attack skill"). Tap the left or right half (or press Left or Right) to use that move, as often as you can;
// the gold ring round the rim is the time left, a few seconds. Knocked out in time: +500; then the ball flies back
// out of the grass. Tall grass frames the bottom of the scene too
#define PB_BTL_FOE_X 330     // where the battle puts them (BTL_FOE_X and the rest, further on in the sketch)
#define PB_BTL_FOE_GROUND 186
#define PB_BTL_ME_X 128
#define PB_BTL_ME_GROUND 322
#define PB_WILD 43           // the mockups' wild one: Oddish
static void fxBurst(uint8_t ty, int cx, int cy, float k);  // (the battle's, further on in the sketch)
void drawBattlePanel(int x, int y, const BtlFighter &f, float shown, bool numbers);
void battleMoveRect(uint8_t i, int &x0, int &x1);
BtlFighter petFighter(uint8_t lvl);
int pmdTopY(const PmdMon &m, int groundY, uint8_t maxS);
static bool pbWildLoaded = false;
// a line over the scene in a text box, as the battle says things
static void pbBtlLine(const char *s, int y) {
  gfx->setTextColor(UI_INK);
  setSize(2);
  v2LineBox(s, y);
  setCur(centerX(s, 2), y);
  printT(s);
}
// phase: 0 it appears, 1 pick a move, 2 your move landing, 3 won
static void pbBattle(int phase, float timeLeft) {
  if (!pbWildLoaded) pbWildLoaded = wildPmd.load(PB_WILD, false);
  uint8_t lvl = btlCapLevel(pet.level());
  BtlFighter me = petFighter(lvl), foe = btlMakeWild(PB_WILD, lvl, 0);
  bMeMoves = btlMovesFor(me.dex, lvl, PB_WILD);
  uint8_t used = bMeMoves.count > 1 ? 1 : 0;  // the move landing: its own type's (the right one)
  v2BattleScene(-1, true, PB_BTL_FOE_X, PB_BTL_FOE_GROUND, PB_BTL_ME_X, PB_BTL_ME_GROUND);
  uint32_t now = millis();
  // tall grass on the wild one's base, tufts round its back edge, then it, then the grass in front of its feet
  const int gy = PB_BTL_FOE_GROUND - 4 + v2BtlBobFoe;
  pbGrassOval(PB_BTL_FOE_X, gy, 74, 15, phase == 0);
  pbGrassTufts(PB_BTL_FOE_X, gy, 74, 15, 192, 348, phase == 0);
  if (phase != 3 && wildPmd.loaded)
    drawPmdActM(wildPmd, phase == 2 && wildPmd.has(PMD_HURT) ? PMD_HURT : PMD_IDLE, PB_BTL_FOE_X, PB_BTL_FOE_GROUND + v2BtlBobFoe, now, true, false, 3);
  pbGrassOval(PB_BTL_FOE_X, gy, 74, 15, phase == 0, gy + 2);
  pbGrassTufts(PB_BTL_FOE_X, gy + 6, 60, 8, 200, 340, phase == 0);
  // yours, from behind, lunging as its move goes
  uint8_t act = phase == 2 ? PMD_ATTACKUR : PMD_IDLEUR;
  if (!pmd.has(act)) act = phase == 2 && pmd.has(PMD_ATTACK) ? PMD_ATTACK : PMD_IDLE;
  drawPmdActM(pmd, act, PB_BTL_ME_X + (phase == 2 ? 34 : 0), PB_BTL_ME_GROUND + (phase == 2 ? -14 : 0) + v2BtlBobMe, now, true, false, 4);
  if (phase == 2 && wildPmd.loaded)
    fxBurst(MOVE_TBL[bMeMoves.ids[used]].type, PB_BTL_FOE_X, (PB_BTL_FOE_GROUND + pmdTopY(wildPmd, PB_BTL_FOE_GROUND, 3)) / 2, 0.5f);
  drawBattlePanel(72, 64, foe, phase == 3 ? 0 : phase == 2 ? foe.hp * 0.45f : foe.hp, false);
  drawBattlePanel(226, 256, me, me.hp, true);
  // the tall grass along the bottom, framing the clearing
  Rgb b, l, d;
  pbGrassCols(b, l, d);
  for (int y = 394; y < V2_W; y += 2)
    for (int x = 0; x < V2_W; x += 2)
      if (hypotf(x + 1 - CX, y + 1 - CY) < 234) pbPx2(x, y, pbTuftAt(x, y, false, sc565(b), sc565(l), sc565(d)));
  for (int x = 50; x < V2_W - 50; x += 12) {  // its top edge, in tufts
    int y = 394;
    scRect(x, y - 8, x + 1, y - 7, d);
    scRect(x - 2, y - 6, x + 3, y - 1, d);
    scRect(x, y - 6, x + 1, y - 1, l);
  }
  // the time left: a gold ring round the rim, emptying the way a clock goes
  for (int k = 0; k < 360; k += 2) {
    if (k > timeLeft * 360) break;
    float a = pbRad(k - 90);
    fsSq(CX + (int)(226 * cosf(a)), CY + (int)(226 * sinf(a)), 6, 0xF2C230);
  }
  char line[48], nm[24];
  switch (phase) {
    case 0:
      snprintf(line, sizeof(line), T(S_BTL_WILD), dexName(PB_WILD));
      pbBtlLine(line, 380);
      break;
    case 1:
      for (uint8_t i = 0; i < bMeMoves.count; i++) {  // your two moves, as in a battle: the left half, the right half
        BtlMove m = MOVE_TBL[bMeMoves.ids[i]];
        int x0, x1;
        battleMoveRect(i, x0, x1);
        v2Button(x0, 346, x1 - x0, 40, TYPE_ACCENT[m.type]);
        gfx->setTextColor(UI_WHITE);
        setSize(2);
        setCur((x0 + x1) / 2 - (int)strlen(m.name) * 6, 359);
        printT(m.name);
      }
      pbBtlLine("TAP A MOVE!", 410);
      break;
    case 2:
      v2Caps(dexName(me.dex), nm, sizeof(nm));
      snprintf(line, sizeof(line), "%s: %s!", nm, MOVE_TBL[bMeMoves.ids[used]].name);
      pbBtlLine(line, 380);
      break;
    case 3:
      snprintf(line, sizeof(line), T(S_BTL_LOST), dexName(PB_WILD));
      pbBtlLine(line, 380);
      fsSay("WIN! +500", CX, 196, 0xFFE27A, 4);
      break;
  }
}

// ===================================================================== the mock screens
// 1, 2: the start in each look (one of the two at random each game, each with its own song): the ball rolling in along
// the orbit at the top, nothing lit yet, the hint over the flippers
static void pbMockStart() {
  pbStill(PbState());
  pbRaised();
  pbBall(392, 108, -7, 5, 2.1f);
  pbHud("0", 3);
  fsSay("TAP LEFT OR RIGHT", CX, 336, 0xFFFFFF);
}
// 3: its how-to card over the table
static void pbMockHow() {
  pbStill(PbState());
  pbRaised();
  pbHud("0", 3);
  v2IntroCard(CX, CY, true, "PINBALL", T(S_HOW_PINBALL));
}
// 4: a flip: a finger on the left half (its rim glows), the left flipper up, the ball off it toward the ramp
static void pbMockFlip() {
  PbState s;
  s.upL = true, s.berries = 2, s.petHits = 1;
  pbStill(s);
  pbBall(176, 324, -5, -8, 0.9f);
  pbRaised();
  pbHalfGlow(-1);
  pbHud("1,250", 3);
}
// 5: over: the ball up the ramp and over the orbit lane, bigger (nearer) and its shadow further off; RAMP! +250
static void pbMockOver() {
  PbState s;
  s.berries = 2, s.petHits = 1;
  pbStill(s);
  pbRaised(true);
  float x, y;
  pbRampAt(0.86f, x, y);
  pbBall((int)x, (int)y, -1, -8, 4.0f, 12, 14);
  fsSay("RAMP! +250", 184, 216, 0xFFE27A, 3);
  pbHud("1,500", 3);
}
// 6: under: the ball round the orbit lane, under the wire rail (in its shade, the wires across it), lighting a berry
static void pbMockUnder() {
  PbState s;
  s.berries = 1, s.petHits = 1;
  pbStill(s);
  float x, y;
  pbWireAt(206, PB_WIRE_R, x, y);
  twGlow((int)x, (int)y, 30, 0xFFF3B0, 120);
  pbBall((int)x, (int)y, -3, -8, 5.5f, PB_BALL_R, 4, true);
  pbRaised();
  fsSay("ORBIT +50", 192, 236, 0xFFE27A, 2);
  pbHud("1,550", 3);
}
// 7: a bumper: the ball off the Great Ball, which lights up and kicks it, +10
static void pbMockBumper() {
  PbState s;
  s.berries = 2, s.petHits = 1, s.bumpLit = 1;
  pbStill(s);
  pbBall(272, 222, -6, 6, 3.3f);
  pbRaised();
  for (int k = 0; k < 6; k++) {  // sparks
    float a = k * 1.047f + 0.3f;
    fsSq(PB_BUMP[1][0] + (int)(36 * cosf(a)), PB_BUMP[1][1] + (int)(36 * sinf(a)), 4, 0xFFF3B0);
  }
  fsSay("+10", PB_BUMP[1][0] + 30, 120, 0xFFE27A, 3);
  pbHud("1,260", 3);
}
// 8: all three bumpers hit: they light, and Diglett pops up out of its hole between them; hit it with the ball before
// the ring round it runs out
static void pbMockDigUp() {
  PbState s;
  s.berries = 2, s.petHits = 1, s.bumpLit = 3, s.digUp = true, s.digTime = 0.7f;
  pbStill(s);
  pbBall(150, 262, 6, -6, 1.4f);
  pbRaised();
  fsBoard("HIT DIGLETT!", 206);
  pbHud("1,290", 3);
}
// 9: Diglett hit: knocked dizzy, +1000, then back down its hole
static void pbMockDigHit() {
  PbState s;
  s.berries = 2, s.petHits = 1, s.digUp = true, s.digHit = true;
  pbStill(s);
  pbBall(206, 166, -7, 4, 2.8f);
  pbRaised();
  fsSay("+1000", CX, 210, 0xFFE27A, 4);
  pbHud("2,290", 3);
}
// 10: into the grass: the ball up the notch and into it (the blades in front of it), the grass rustling, a "!" over it
static void pbMockSpotIn() {
  PbState s;
  s.berries = 3, s.petHits = 1, s.spotLit = true;
  pbStill(s);
  pbBall(360, 244, 6, -5, 2.5f);
  pbGrassFront(244, true);
  pbRaised();
  fsBang(392, 182);
  pbHud("2,290", 3);
}
// 11 to 14: the battle: a wild Oddish appears in the grass; pick a move; your move landing; won
static void pbMockBtlIntro() { pbBattle(0, 1.0f); }
static void pbMockBattle() { pbBattle(1, 0.7f); }
static void pbMockBtlHit() { pbBattle(2, 0.55f); }
static void pbMockWon() { pbBattle(3, 0.4f); }
// 15: out it comes: the ball shot back down the notch toward the left flipper, the grass rustling, +500 over it
static void pbMockSpit() {
  PbState s;
  s.berries = 3, s.petHits = 1, s.spotLit = true;
  pbStill(s);
  for (int k = 0; k < 5; k++) {  // the burst out of the notch
    float a = pbRad(110 + k * 22), x = PB_NOTCH[0][0], y = PB_NOTCH[0][1];
    pbCapsule(x + 18 * cosf(a), y + 18 * sinf(a), x + 32 * cosf(a), y + 32 * sinf(a), 2, 2, 0xFFF3B0);
  }
  pbBall(282, 306, -10, 9, 1.0f);
  pbRaised();
  fsSay("+500", PB_GRASS_X - 6, 196, 0xFFE27A, 3);
  pbHud("2,790", 3);
}
// 16: your Pokemon hit: it lights up, +100, the second of three toward its bonus
static void pbMockPet() {
  PbState s;
  s.berries = 3, s.petHits = 2, s.petHit = true;
  pbStill(s);
  pbBall(276, 260, 6, 4, 3.7f);
  pbRaised();
  fsSay("+100", CX, 210, 0xFFE27A, 3);
  for (int k = 0; k < 4; k++) {  // stars round it
    static const int8_t ST[4][2] = { { -50, -70 }, { 48, -78 }, { 60, -30 }, { -58, -26 } };
    int x = CX + ST[k][0], y = PB_PET_FEET - 30 + ST[k][1];
    scGridRect(x, y - 8, x + 4, y + 12, 0xFFFFFF);
    scGridRect(x - 8, y, x + 12, y + 4, 0xFFFFFF);
  }
  pbHud("2,890", 3);
}
// 17: every berry lit: the score doubles from then on (x2 by the score)
static void pbMockBerries() {
  PbState s;
  s.upR = true, s.berries = 6, s.petHits = 2;
  pbStill(s);
  pbBall(296, 330, 4, -8, 0.6f);
  pbRaised();
  pbHalfGlow(1);
  fsSay("BERRIES x2!", CX, 214, 0xFFE27A, 4);
  pbHud("3,840", 3, "x2");
}
// 18: the ball lost: down the drain between the flippers, the bottom glowing red, BALL 2 next
static void pbMockDrain() {
  PbState s;
  s.berries = 4, s.petHits = 1;
  pbStill(s);
  for (int y = 300; y < V2_W; y += 4)
    for (int x = 0; x < V2_W; x += 4) {
      float d = 233 - hypotf(x + 2 - CX, y + 2 - CY);
      if (d < 0 || d > 30) continue;
      fsBlend(x, y, x + 3, y + 3, 0xE8503A, (int)(170 * (1 - d / 30)));
    }
  pbBall(CX, 446, 0, 8, 1.7f);
  pbRaised();
  fsBoard("BALL 2", 214);
  pbHud("2,110", 2);
}
// 19: the end: the game's result as every game shows it (the score, NEW RECORD!, the joy), PLAY AGAIN! and the X
static void pbMockEnd() {
  PbState s;
  s.berries = 4, s.petHits = 1;
  pbStill(s);
  pbRaised();
  pbHud("12,340", 0);
  v2GameResult("PINBALL", nullptr, "12,340", true, "BEST: 9,870", "+23 HAPPINESS");
}
// 20: on a computer: the keys in place of the touch hint (Left and Right flip, Space both)
static void pbMockKeys() {
  pbStill(PbState());
  pbBall(392, 108, -7, 5, 2.1f);
  pbRaised();
  setSize(2);
  int w = 30 + 6 + 30 + 10 + textW("FLIP", 2), x = CX - w / 2, y = 322;
  x += pbKey(x, y, -1) + 6;
  x += pbKey(x, y, 1) + 10;
  fsSay("FLIP", x + textW("FLIP", 2) / 2, y + 6, 0xFFFFFF);
  int w2 = textW("SPACE", 2) + 16 + 10 + textW("BOTH", 2), x2 = CX - w2 / 2;
  x2 += pbKey(x2, y + 38, 0, "SPACE") + 10;
  fsSay("BOTH", x2 + textW("BOTH", 2) / 2, y + 44, 0xFFFFFF);
  pbHud("0", 3);
}
// 21: on a computer, its how-to card says the keys
static void pbMockHowKeys() {
  pbStill(PbState());
  pbRaised();
  pbHud("0", 3);
  v2IntroCard(CX, CY, true, "PINBALL", T(S_HOW_PINBALL_KEYS));
}
// 22: the ball rolling round the top of the table, moving (for a moving picture)
static void pbMockRoll() {
  pbStill(PbState());
  float t = (millis() % 3000) / 3000.0f, a = pbRad(200 + 140 * t);
  float x = CX + PB_WIRE_R * cosf(a), y = CY + PB_WIRE_R * sinf(a);
  float dx = -sinf(a), dy = cosf(a);  // round the top, left to right
  float roll = (140 * t) * 0.01745329f * PB_WIRE_R / PB_BALL_R;
  pbBall((int)x, (int)y, dx, dy, roll);
  pbRaised();
  pbHud("0", 3);
}
// 23: the ball close up, rolling right, then down, then left (a second each): how it turns as it goes
static void pbMockRollBig() {
  pbTable();
  uint32_t ms = millis() % 3000;
  float t = (ms % 1000) / 1000.0f * 6.2832f;
  float dx = ms < 1000 ? 1 : ms < 2000 ? 0 : -1, dy = ms < 1000 ? 0 : ms < 2000 ? 1 : 0;
  gfx->fillCircle(CX + 6, CY + 14, 70, sc565(pbFloorDark()));
  pbBallSphere(CX, CY, 70, dx, dy, t, false);
  pbArrow(CX + (int)(dx * 100), CY + (int)(dy * 100), dx, dy, 0xFFE27A);
  fsSay(ms < 1000 ? "ROLLING RIGHT" : ms < 2000 ? "ROLLING DOWN" : "ROLLING LEFT", CX, 352, 0xFFFFFF, 2);
}

static void pinMockRender() {
  int n = pbMock >= 24 ? (pbMock == 24 ? 5 : pbMock == 25 ? 8 : 10) : pbMock;  // 24 to 26: the meadow look of 5, 8, 10
  pbDay = pbMock >= 24 || pbMock == 2;
  switch (n) {
    case 1: case 2: pbMockStart(); break;
    case 3: pbMockHow(); break;
    case 4: pbMockFlip(); break;
    case 5: pbMockOver(); break;
    case 6: pbMockUnder(); break;
    case 7: pbMockBumper(); break;
    case 8: pbMockDigUp(); break;
    case 9: pbMockDigHit(); break;
    case 10: pbMockSpotIn(); break;
    case 11: pbMockBtlIntro(); break;
    case 12: pbMockBattle(); break;
    case 13: pbMockBtlHit(); break;
    case 14: pbMockWon(); break;
    case 15: pbMockSpit(); break;
    case 16: pbMockPet(); break;
    case 17: pbMockBerries(); break;
    case 18: pbMockDrain(); break;
    case 19: pbMockEnd(); break;
    case 20: pbMockKeys(); break;
    case 21: pbMockHowKeys(); break;
    case 22: pbMockRoll(); break;
    case 23: pbMockRollBig(); break;
  }
  pbDay = false;
}
#else
static void pinMockRender() {}
#endif
