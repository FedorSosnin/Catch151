// V2: Pinball's MOCKUPS (PINMOCK n): still screens drawn in the real game, so the look can be judged before any of it is
// built (as Reel Fishing's were, FISHMOCK n). The plan is docs/PINBALL.md: the round screen is the table, its rim the
// wall. On the floor: an orbit lane round the top with berry lights in it, three bumpers, your own Pokemon on a pad as
// a target, a rock cave where a wild Pokemon waits (in goes the ball, a short battle, out it comes), slingshots, and
// two flippers over the drain. Raised over the floor (Fedor, 2026-11-15: "ramps and areas the ball can go under and
// over"): a ramp up the left that crosses over the orbit and runs down the rim as a wire rail to the left inlane, so the
// ball rolls over the orbit on it, or under the wire in the orbit. The ball is a Poke Ball. scene_v2.h includes this
// after fishing_v2.h, whose painters it borrows (fsSay, fsBoard, fsThumb, fsCircle, fsSq, fsBlend). On a board nothing
// calls them: they're a computer's only.

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
#define PB_PET_FEET 292     // your Pokemon on its pad, in the middle
#define PB_CAVE_X 350       // the cave: a rock mound on the right, its mouth turned down to the left flipper
#define PB_CAVE_Y 228
#define PB_MOUTH_X 330
#define PB_MOUTH_Y 250
static bool pbDay = false;  // the second look: a meadow on a wooden rim (the first: a night-blue table on a chrome one)
static const int16_t PB_BUMP[3][2] = { { 186, 162 }, { 280, 162 }, { 233, 116 } };
static const Rgb PB_BUMP_COL[3] = { 0xE8503A, 0x4FB0E8, 0xF2C230 };
static const int16_t PB_BERRY_A[6] = { 212, 234, 246, 294, 306, 328 };  // the berry lights, in the orbit lane
// the ramp: from its mouth left of the middle, up the left and over the orbit lane, where the wire rail takes over
static const float PB_RAMP[3][2] = { { 148, 262 }, { 106, 196 }, { 98, 84 } };  // start, the curve's pull, end

static inline float pbRad(float deg) { return deg * 0.01745329f; }
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

// the playfield and its rim
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
      } else if (d >= PB_R - 2) {
        c = 0x1B1B25;
      } else {
        c = scMix(mid, edge, d / PB_R);
        bool lane = d > PB_ORBIT_R0 && a >= PB_ORBIT_A0 && a <= PB_ORBIT_A1;
        if (lane) c = scMix(c, pbDay ? 0x2E6A2A : 0x101236, .35);  // the orbit lane, a shade darker
        if (pbDay) {
          if (((x * 7 + y * 13) / 2) % 23 == 0) c = scMix(c, 0x2E7A2A, .5);           // grass tufts
          if (((x * 11 + y * 5) / 2) % 97 == 0) c = (x / 2) % 2 ? 0xFFFFFF : 0xF8C8E0;  // a flower
        } else if (x % 24 == 0 && y % 24 == 0) {
          c = scMix(c, 0x8A8AE0, .5);  // the floor's little lights
        }
      }
      pbPx2(x, y, sc565(c));
    }
  // the art on the floor: a ring round your Pokemon's pad, chevrons up the outlanes
  fsCircle(CX, PB_PET_FEET - 10, 58, pbDay ? 0x5AA84A : 0x4A4EA8, 4);
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
// the orbit's inner rail, an arc of short bars
static void pbOrbit() {
  for (int pass = 0; pass < 2; pass++)
    for (int a = PB_ORBIT_A0; a < PB_ORBIT_A1; a += 4) {
      float a0 = pbRad(a), a1 = pbRad(a + 4 > PB_ORBIT_A1 ? PB_ORBIT_A1 : a + 4);
      float x0 = CX + PB_ORBIT_R0 * cosf(a0), y0 = CY + PB_ORBIT_R0 * sinf(a0);
      float x1 = CX + PB_ORBIT_R0 * cosf(a1), y1 = CY + PB_ORBIT_R0 * sinf(a1);
      if (pass == 0) pbCapsule(x0, y0, x1, y1, 6, 6, 0x1B1B25);
      else pbCapsule(x0, y0, x1, y1, 4, 4, pbSteel()), pbCapsule(x0, y0 - 1, x1, y1 - 1, 1, 1, pbSteelLit());
    }
}
static void pbRail(float ax, float ay, float bx, float by) {  // a guide on the floor (wood by day), darker than flippers
  pbCapsule(ax, ay, bx, by, 6, 6, 0x1B1B25);
  pbCapsule(ax, ay, bx, by, 4, 4, pbSteel());
  pbCapsule(ax, ay - 2, bx, by - 2, 1, 1, pbSteelLit());
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
static void pbSlings() {
  for (int s = -1; s <= 1; s += 2) {
    int ax = s < 0 ? 82 : V2_W - 1 - 82, bx = s < 0 ? 122 : V2_W - 1 - 122;
    gfx->fillTriangle(ax - s * 4, 286, ax - s * 4, 344, bx + s * 4, 368, sc565(0x1B1B25));
    gfx->fillTriangle(ax, 294, ax, 340, bx, 362, sc565(0xE8503A));
    pbCapsule(ax, 294, bx, 362, 3, 3, 0xF4F4F8);
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
  gfx->fillCircle(cx + 2, cy + 4, 23, sc565(pbFloorDark()));  // its shadow
  gfx->fillCircle(cx, cy, 23, sc565(0x1B1B25));
  gfx->fillCircle(cx, cy, 21, sc565(lit ? 0xFFFFFF : 0xE4E8F0));
  gfx->fillCircle(cx, cy, 16, sc565(0x1B1B25));
  gfx->fillCircle(cx, cy, 14, sc565(lit ? scMix(cap, 0xFFFFFF, .35) : cap));
  gfx->fillCircle(cx - 4, cy - 5, 4, sc565(scMix(cap, 0xFFFFFF, .7)));
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
// the cave: a rock mound, its dark mouth turned to the left flipper and a wild Pokemon's eyes in it; lit: its mouth
// glowing (the ball going in, or coming out)
static void pbCave(bool lit) {
  const Rgb rock = pbDay ? 0x9A8A6A : 0x6E6A84, rockLit = pbDay ? 0xC0B08A : 0x9A94B0, rockDark = pbDay ? 0x6E6044 : 0x4A4660;
  pbEllipse(PB_CAVE_X + 4, PB_CAVE_Y + 8, 46, 40, pbFloorDark());  // its shadow
  pbEllipse(PB_CAVE_X, PB_CAVE_Y, 46, 40, 0x1B1B25);
  pbEllipse(PB_CAVE_X, PB_CAVE_Y, 43, 37, rock);
  pbEllipse(PB_CAVE_X + 6, PB_CAVE_Y - 12, 30, 20, rockLit);  // the light on its top
  pbEllipse(PB_CAVE_X + 18, PB_CAVE_Y - 18, 10, 7, scMix(rockLit, 0xFFFFFF, .3));
  pbEllipse(PB_CAVE_X + 22, PB_CAVE_Y + 14, 12, 8, rockDark);  // its facets
  pbEllipse(PB_CAVE_X - 4, PB_CAVE_Y - 26, 8, 5, rockDark);
  if (lit) twGlow(PB_MOUTH_X, PB_MOUTH_Y, 50, 0xFFE27A, 180);
  pbEllipse(PB_MOUTH_X, PB_MOUTH_Y, 24, 18, 0x1B1B25);  // the mouth
  pbEllipse(PB_MOUTH_X, PB_MOUTH_Y + 2, 20, 14, lit ? 0x8A5A20 : 0x0A0A12);
  if (!lit) {  // two eyes in the dark
    gfx->fillRect(PB_MOUTH_X - 8, PB_MOUTH_Y - 2, 4, 4, sc565(0xFFE27A));
    gfx->fillRect(PB_MOUTH_X + 4, PB_MOUTH_Y - 2, 4, 4, sc565(0xFFE27A));
  }
  pbArrow(PB_MOUTH_X - 44, PB_MOUTH_Y + 40, 0.75f, -0.66f, lit ? 0xFFE27A : 0xB06AE0);  // the shot's way in
}
// the ball: a Poke Ball, (x, y) its middle, r its size (on the ramp, nearer, bigger); trail: where it came from; dark:
// in the shade under the wire
static void pbBall(int x, int y, int tdx = 0, int tdy = 0, int r = PB_BALL_R, int sh = 4, bool dark = false) {
  for (int k = 3; k >= 1 && (tdx || tdy); k--)
    fsBlend(x + tdx * k - 6, y + tdy * k - 6, x + tdx * k + 5, y + tdy * k + 5, 0xFFFFFF, 40 * (4 - k));
  gfx->fillCircle(x + sh / 2, y + sh, r, sc565(pbFloorDark()));  // its shadow (further off on the ramp)
  for (int j = -r - 1; j <= r + 1; j++)
    for (int i = -r - 1; i <= r + 1; i++) {
      int d2 = i * i + j * j;
      if (d2 > (r + 1) * (r + 1)) continue;
      Rgb c = d2 > (r - 1) * (r - 1) ? 0x1B1B25 : abs(j) <= 1 ? 0x1B1B25 : j < 0 ? 0xE8503A : 0xF4F4F8;
      if (j < 0 && d2 < 30 && i < -1 && j < -3) c = 0xFF9C7A;  // its shine
      if (dark) c = scMix(c, 0x101030, .4);
      pbPx1(x + i, y + j, sc565(c));
    }
  gfx->fillCircle(x, y, 3, sc565(0x1B1B25));
  gfx->fillCircle(x, y, 2, sc565(dark ? 0xA0A0B0 : 0xFFFFFF));
}
// the ramp's middle line at t (0 its mouth, 1 its top)
static void pbRampAt(float t, float &x, float &y) {
  float u = 1 - t;
  x = u * u * PB_RAMP[0][0] + 2 * u * t * PB_RAMP[1][0] + t * t * PB_RAMP[2][0];
  y = u * u * PB_RAMP[0][1] + 2 * u * t * PB_RAMP[1][1] + t * t * PB_RAMP[2][1];
}
// the wire rail's middle at angle a (degrees)
static void pbWireAt(float a, float r, float &x, float &y) { x = CX + r * cosf(pbRad(a)), y = CY + r * sinf(pbRad(a)); }
// what's raised over the floor: the ramp (a see-through plastic lane between two steel rails, rising, so its shadow
// falls further off toward its top) and the wire rail it becomes, two thin wires on ties down the rim over the orbit,
// to where it lets the ball down into the left inlane. The ball under it shows through
static void pbRaised(bool rampLit = false) {
  const int N = 20;
  float px[N + 1], py[N + 1];
  for (int i = 0; i <= N; i++) pbRampAt(i / (float)N, px[i], py[i]);
  for (int i = 0; i < N; i++) {  // the ramp's shadow
    float t = i / (float)N, ox = 4 + 8 * t, oy = 6 + 10 * t;
    pbCapsule(px[i] + ox, py[i] + oy, px[i + 1] + ox, py[i + 1] + oy, 16, 16, pbFloorDark(), 110);
  }
  for (float a = PB_WIRE_A1; a < PB_WIRE_A0; a += 3) {  // the wire's shadow
    float x0, y0, x1, y1;
    pbWireAt(a, PB_WIRE_R, x0, y0);
    pbWireAt(a + 3, PB_WIRE_R, x1, y1);
    pbCapsule(x0 + 10, y0 + 14, x1 + 10, y1 + 14, 9, 9, pbFloorDark(), 70);
  }
  for (int i = 0; i < N; i++)  // the ramp's see-through floor
    pbCapsule(px[i], py[i], px[i + 1], py[i + 1], 14, 14, rampLit ? 0xFFF3B0 : (pbDay ? 0xF8ECC0 : 0xA8C4F8), 95);
  for (int side = -1; side <= 1; side += 2)  // its two rails
    for (int pass = 0; pass < 2; pass++)
      for (int i = 0; i < N; i++) {
        float dx = px[i + 1] - px[i], dy = py[i + 1] - py[i], L = hypotf(dx, dy), nx = -dy / L * 15 * side, ny = dx / L * 15 * side;
        if (pass == 0) pbCapsule(px[i] + nx, py[i] + ny, px[i + 1] + nx, py[i + 1] + ny, 4, 4, 0x1B1B25);
        else pbCapsule(px[i] + nx, py[i] + ny, px[i + 1] + nx, py[i + 1] + ny, 2, 2, side < 0 ? 0xE4E8F0 : 0xB8BED0);
      }
  // its mouth: a lit lip across it, and the arrow on the floor showing the shot up it
  float mx = px[0], my = py[0], dx = px[1] - px[0], dy = py[1] - py[0], L = hypotf(dx, dy);
  pbCapsule(mx - dy / L * 16, my + dx / L * 16, mx + dy / L * 16, my - dx / L * 16, 3, 3, rampLit ? 0xFFFFFF : 0xFFE27A);
  pbArrow((int)(mx - dx / L * 44), (int)(my - dy / L * 44), dx, dy, rampLit ? 0xFFE27A : 0x4FB0E8);
  // the wire rail: two wires on ties, from the ramp's top down the rim
  for (float a = PB_WIRE_A1; a < PB_WIRE_A0; a += 10) {  // the ties
    float x0, y0, x1, y1;
    pbWireAt(a, PB_WIRE_R - 9, x0, y0);
    pbWireAt(a, PB_WIRE_R + 9, x1, y1);
    pbCapsule(x0, y0, x1, y1, 2, 2, 0x1B1B25);
    pbCapsule(x0, y0, x1, y1, 1, 1, 0x9AA0B4);
  }
  for (int w = -1; w <= 1; w += 2)
    for (int pass = 0; pass < 2; pass++)
      for (float a = PB_WIRE_A1; a < PB_WIRE_A0; a += 3) {
        float x0, y0, x1, y1;
        pbWireAt(a, PB_WIRE_R + w * 8, x0, y0);
        pbWireAt(a + 3, PB_WIRE_R + w * 8, x1, y1);
        if (pass == 0) pbCapsule(x0, y0, x1, y1, 3, 3, 0x1B1B25);
        else pbCapsule(x0, y0, x1, y1, 1, 1, 0xF0F2F8);
      }
  float ex, ey;  // where it lets the ball down: a little cup at its end
  pbWireAt(PB_WIRE_A1, PB_WIRE_R, ex, ey);
  gfx->fillCircle((int)ex, (int)ey, 11, sc565(0x1B1B25));
  gfx->fillCircle((int)ex, (int)ey, 8, sc565(0x9AA0B4));
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
    for (int j = -5; j <= 5; j++)
      for (int k = -5; k <= 5; k++)
        if (k * k + j * j <= 25)
          pbPx1(x + k, y + j, sc565(i >= ballsLeft ? 0x4A4A6A : j == 0 ? 0x1B1B25 : j < 0 ? 0xE8503A : 0xF4F4F8));
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
// everything on the floor: the table, the orbit, the berries, guides, slingshots, bumpers, your Pokemon, the cave, the
// flippers. The ball on the floor goes after this, then pbRaised() over it, then a ball on the ramp
static void pbStill(bool upL, bool upR, int berriesLit, int petHits, bool petHit = false, int bumpLit = -1, bool caveLit = false) {
  pbTable();
  pbOrbit();
  for (int k = 0; k < 6; k++) pbBerry(k, k < berriesLit);
  const float wa = 2.583f;  // the inlane guides: from the wall (148 degrees round) in to just above each flipper
  float wx = CX + PB_R * cosf(wa), wy = CY + PB_R * sinf(wa);
  pbRail(wx, wy, PB_FLIP_LX - 8, PB_FLIP_Y - 6);
  pbRail(V2_W - 1 - wx, wy, V2_W - 1 - PB_FLIP_LX + 8, PB_FLIP_Y - 6);
  pbSlings();
  for (int k = 0; k < 3; k++) pbBumper(k, k == bumpLit);
  pbPet(petHits, petHit);
  pbCave(caveLit);
  pbFlipper(-1, upL);
  pbFlipper(1, upR);
}

// ===================================================================== the battle in the cave
// In goes the ball, and a window opens over the dimmed table: the cave inside, a wild Pokemon of the zone you're in
// and yours facing it. Tap (or any key) to attack, as fast as you like, for a few seconds: the ring round the window
// is the time left. Knocked out in time: +500, and the ball comes flying out; not: it comes out anyway, no bonus
#define PB_WIN_R 150
static void pbBattleBack(int16_t wild, bool won, float timeLeft) {
  v2DimFrame(150);
  const int cx = CX, cy = 236;
  gfx->fillCircle(cx, cy, PB_WIN_R + 8, sc565(0x1B1B25));
  for (int k = 0; k < 360; k += 2) {  // the time left: a gold ring, emptying the way a clock goes
    if (k > timeLeft * 360) break;
    float a = pbRad(k - 90);
    fsSq(cx + (int)((PB_WIN_R + 4) * cosf(a)), cy + (int)((PB_WIN_R + 4) * sinf(a)), 6, 0xF2C230);
  }
  for (int y = cy - PB_WIN_R; y < cy + PB_WIN_R; y += 2)  // the cave inside: rock, darker at the top
    for (int x = cx - PB_WIN_R; x < cx + PB_WIN_R; x += 2) {
      float d = hypotf(x + 1 - cx, y + 1 - cy);
      if (d > PB_WIN_R) continue;
      Rgb c = scMix(0x2A2638, 0x5A5470, (y - (cy - PB_WIN_R)) / (2.0f * PB_WIN_R));
      if (((x * 5 + y * 3) / 2) % 41 == 0) c = scMix(c, 0x8A84A0, .5);
      pbPx2(x, y, sc565(c));
    }
  for (int k = 0; k < 6; k++) {  // stalactites
    int x = cx - 110 + k * 44, top = cy - (int)sqrtf((float)(PB_WIN_R * PB_WIN_R - (x - cx) * (x - cx)));
    gfx->fillTriangle(x - 10, top, x + 10, top, x, top + 26 + (k % 2) * 14, sc565(0x3A3650));
  }
  pbEllipse(cx + 58, cy - 6, 50, 12, 0x3A3650);    // where the wild one stands
  pbEllipse(cx - 58, cy + 84, 58, 14, 0x3A3650);   // and yours
  const uint8_t *tw = thumbs.getForm(wild, false);
  if (won) fsThumb(tw, cx + 58, cy - 4, 3, false, 0xFFFFFF, 200);  // knocked out: a white flash, fading
  else fsThumb(tw, cx + 58, cy - 4, 3);
  int16_t d = pet.speciesId > 0 ? pet.speciesId : 25;
  fsThumb(thumbs.getForm(d, pet.shiny), cx - 58, cy + 86, 3, true);
  // its name and HP, over it
  char nm[24];
  v2Caps(dexName(wild), nm, sizeof(nm));
  setSize(2);
  int nx = cx + 6, ny = cy - 112;
  gfx->setTextColor(UI_WHITE);
  setCur(nx, ny);
  printT(nm);
  gfx->fillRoundRect(nx - 2, ny + 20, 104, 12, 4, sc565(0x1B1B25));
  gfx->fillRoundRect(nx, ny + 22, 100, 8, 3, sc565(0x4A4660));
  int hp = won ? 0 : 40;
  if (hp) gfx->fillRoundRect(nx, ny + 22, hp, 8, 3, sc565(0xE8503A));
}
#endif  // BOARD_HOST

// ===================================================================== the mock screens
#ifdef BOARD_HOST
// 1: the start: the ball rolling in along the orbit at the top, nothing lit yet, the hint over the flippers
static void pbMockStart() {
  pbStill(false, false, 0, 0);
  pbRaised();
  pbBall(392, 108, 7, -5);
  pbHud("0", 3);
  fsSay("TAP LEFT OR RIGHT", CX, 334, 0xFFFFFF);
}
// 2: its how-to card over the table
static void pbMockHow() {
  pbStill(false, false, 0, 0);
  pbRaised();
  pbHud("0", 3);
  v2IntroCard(CX, CY, true, "PINBALL", T(S_HOW_PINBALL));
}
// 3: a flip: a finger on the left half (its rim glows), the left flipper up, the ball off it toward the ramp
static void pbMockFlip() {
  pbStill(true, false, 2, 1);
  pbBall(176, 322, 5, 8);
  pbRaised();
  pbHalfGlow(-1);
  pbHud("1,250", 3);
}
// 4: over: the ball up the ramp and over the orbit lane, bigger (nearer) and its shadow further off; RAMP! +250
static void pbMockOver() {
  pbStill(false, false, 2, 1);
  pbRaised(true);
  float x, y;
  pbRampAt(0.86f, x, y);
  pbBall((int)x, (int)y, 1, 8, 12, 14);
  fsSay("RAMP! +250", 170, 196, 0xFFE27A, 3);
  pbHud("1,500", 3);
}
// 5: under: the ball round the orbit lane, under the wire rail (in its shade, the wires across it), lighting a berry
static void pbMockUnder() {
  pbStill(false, false, 1, 1);
  float x, y;
  pbWireAt(206, PB_WIRE_R, x, y);
  twGlow((int)x, (int)y, 30, 0xFFF3B0, 120);
  pbBall((int)x, (int)y, 3, 8, PB_BALL_R, 4, true);
  pbRaised();
  fsSay("ORBIT +50", 192, 232, 0xFFE27A, 2);
  pbHud("1,550", 3);
}
// 6: into the cave: the ball at its mouth, which glows; BATTLE! on a sign over the table
static void pbMockCave() {
  pbStill(false, false, 3, 1, false, -1, true);
  pbBall(PB_MOUTH_X - 6, PB_MOUTH_Y + 4, -6, 6);
  pbRaised();
  fsBoard("BATTLE!", 196);
  pbHud("1,800", 3);
}
// 7: the battle: a wild Zubat in the cave, yours facing it, TAP TO ATTACK!, the time ring two thirds full
static void pbMockBattle() {
  pbStill(false, false, 3, 1, false, -1, true);
  pbRaised();
  pbHud("1,800", 3);
  pbBattleBack(41, false, 0.66f);
  for (int k = 0; k < 6; k++) {  // a hit landing on it
    float a = k * 1.047f;
    fsSq(CX + 58 + (int)(26 * cosf(a)), 196 + (int)(26 * sinf(a)), 6, 0xFFF3B0);
  }
  fsSay("TAP TO ATTACK!", CX, 350, 0xFFFFFF, 2);
}
// 8: won: the wild one knocked out (a white flash), +500
static void pbMockWon() {
  pbStill(false, false, 3, 1, false, -1, true);
  pbRaised();
  pbHud("1,800", 3);
  pbBattleBack(41, true, 0.3f);
  fsSay("WIN! +500", CX, 330, 0xFFE27A, 4);
}
// 9: out it comes: the ball shot out of the cave's mouth toward the left flipper, the mouth glowing, +500 over it
static void pbMockSpit() {
  pbStill(false, false, 3, 1, false, -1, true);
  for (int k = 0; k < 5; k++) {  // the burst from its mouth
    float a = pbRad(120 + k * 18);
    pbCapsule(PB_MOUTH_X + 28 * cosf(a), PB_MOUTH_Y + 28 * sinf(a), PB_MOUTH_X + 44 * cosf(a), PB_MOUTH_Y + 44 * sinf(a), 2, 2, 0xFFF3B0);
  }
  pbBall(272, 300, 10, -9);
  pbRaised();
  fsSay("+500", PB_CAVE_X, 166, 0xFFE27A, 3);
  pbHud("2,300", 3);
}
// 10: a bumper: the ball off the right one, which lights up and kicks it, +10
static void pbMockBumper() {
  pbStill(false, false, 2, 1, false, 1);
  pbBall(250, 196, 6, -6);
  pbRaised();
  for (int k = 0; k < 6; k++) {  // sparks
    float a = k * 1.047f + 0.3f;
    fsSq(PB_BUMP[1][0] + (int)(36 * cosf(a)), PB_BUMP[1][1] + (int)(36 * sinf(a)), 4, 0xFFF3B0);
  }
  fsSay("+10", PB_BUMP[1][0] + 52, 118, 0xFFE27A, 3);
  pbHud("1,260", 3);
}
// 11: your Pokemon hit: it lights up, +100, the second of three toward its bonus
static void pbMockPet() {
  pbStill(false, false, 3, 2, true);
  pbBall(274, 252, 6, 4);
  pbRaised();
  fsSay("+100", CX, 200, 0xFFE27A, 3);
  for (int k = 0; k < 4; k++) {  // stars round it
    static const int8_t ST[4][2] = { { -50, -70 }, { 48, -78 }, { 60, -30 }, { -58, -26 } };
    int x = CX + ST[k][0], y = PB_PET_FEET - 30 + ST[k][1];
    scGridRect(x, y - 8, x + 4, y + 12, 0xFFFFFF);
    scGridRect(x - 8, y, x + 12, y + 4, 0xFFFFFF);
  }
  pbHud("1,460", 3);
}
// 12: every berry lit: the score doubles from then on (x2 by the score)
static void pbMockBerries() {
  pbStill(false, true, 6, 2);
  pbBall(296, 330, 4, 8);
  pbRaised();
  pbHalfGlow(1);
  fsSay("BERRIES x2!", CX, 196, 0xFFE27A, 4);
  pbHud("3,840", 3, "x2");
}
// 13: the ball lost: down the drain between the flippers, the bottom glowing red, BALL 2 next
static void pbMockDrain() {
  pbStill(false, false, 4, 1);
  for (int y = 300; y < V2_W; y += 4)
    for (int x = 0; x < V2_W; x += 4) {
      float d = 233 - hypotf(x + 2 - CX, y + 2 - CY);
      if (d < 0 || d > 30) continue;
      fsBlend(x, y, x + 3, y + 3, 0xE8503A, (int)(170 * (1 - d / 30)));
    }
  pbBall(CX, 446, 0, -8);
  pbRaised();
  fsBoard("BALL 2", 196);
  pbHud("2,110", 2);
}
// 14: the end: the game's result as every game shows it (the score, NEW RECORD!, the joy), PLAY AGAIN! and the X
static void pbMockEnd() {
  pbStill(false, false, 4, 1);
  pbRaised();
  pbHud("12,340", 0);
  v2GameResult("PINBALL", nullptr, "12,340", true, "BEST: 9,870", "+23 HAPPINESS");
}
// 15: the other look, by day: a meadow on a wooden rim (the same moment as 4, over the ramp)
static void pbMockDay() {
  pbDay = true;
  pbMockOver();
  pbDay = false;
}
// 16: on a computer: the keys in place of the touch hint (Left and Right flip, Space both)
static void pbMockKeys() {
  pbStill(false, false, 0, 0);
  pbBall(392, 108, 7, -5);
  pbRaised();
  setSize(2);
  int w = 30 + 6 + 30 + 10 + textW("FLIP", 2), x = CX - w / 2, y = 318;
  x += pbKey(x, y, -1) + 6;
  x += pbKey(x, y, 1) + 10;
  fsSay("FLIP", x + textW("FLIP", 2) / 2, y + 6, 0xFFFFFF);
  int w2 = textW("SPACE", 2) + 16 + 10 + textW("BOTH", 2), x2 = CX - w2 / 2;
  x2 += pbKey(x2, y + 38, 0, "SPACE") + 10;
  fsSay("BOTH", x2 + textW("BOTH", 2) / 2, y + 44, 0xFFFFFF);
  pbHud("0", 3);
}
// 17: on a computer, its how-to card says the keys
static void pbMockHowKeys() {
  pbStill(false, false, 0, 0);
  pbRaised();
  pbHud("0", 3);
  v2IntroCard(CX, CY, true, "PINBALL", T(S_HOW_PINBALL_KEYS));
}

static void pinMockRender() {
  switch (pbMock) {
    case 1: pbMockStart(); break;
    case 2: pbMockHow(); break;
    case 3: pbMockFlip(); break;
    case 4: pbMockOver(); break;
    case 5: pbMockUnder(); break;
    case 6: pbMockCave(); break;
    case 7: pbMockBattle(); break;
    case 8: pbMockWon(); break;
    case 9: pbMockSpit(); break;
    case 10: pbMockBumper(); break;
    case 11: pbMockPet(); break;
    case 12: pbMockBerries(); break;
    case 13: pbMockDrain(); break;
    case 14: pbMockEnd(); break;
    case 15: pbMockDay(); break;
    case 16: pbMockKeys(); break;
    case 17: pbMockHowKeys(); break;
  }
}
#else
static void pinMockRender() {}
#endif
