// V2: Pinball, drawn: the game from pinball.h's state (pinDraw()). The plan is docs/PINBALL.md: the round screen is
// the table, its rim the wall. On the floor: an orbit lane round the top with berry lights in it, three bumpers (a
// Poke Ball, a Great Ball and an Ultra Ball) with Diglett's hole between them, a ring in the middle, a patch of tall
// grass by the right wall where wild Pokemon wait (in goes the ball, and one jumps out into the ring, to battle with
// the ball: Fedor, "The battles need to happen inside the pinball game"), slingshots, and two flippers over the drain.
// Raised over the floor (Fedor: "ramps and areas the ball can go under and over"): a ramp straight up the left, arrows
// chasing up it, that turns over into the wall in the top-left corner, crossing over the orbit, and runs down the rim
// as a wire rail to the left inlane. Two
// looks, one each game at random: a night-blue table on a chrome rim, or a meadow on a wooden one. The ball is a Poke
// Ball that rolls (Fedor: "make sure the ball spins as it moves").
// scene_v2.h includes this after fishing_v2.h, whose painters it borrows (fsSay, fsBoard, fsThumb, fsCircle, fsSq,
// fsBlend). The table's numbers are pinball.h's.

static bool pbDay = false;   // the meadow on a wooden rim (else the night-blue table on a chrome one)

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
// the ball on the table: its shadow, then it, rolled (no streak behind it: Fedor, "I dont like the trail the ball
// leaves")
static void pbBall(int x, int y, float dx, float dy, float roll, int r = PB_BALL_R, int sh = 4, bool dark = false) {
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
  // the art on the floor: the ring in the middle, where a wild one stands to battle, chevrons up the outlanes
  fsCircle(PB_RING_X, PB_RING_Y - 18, 40, pbDay ? 0x5AA84A : 0x4A4EA8, 4);
  fsCircle(PB_RING_X, PB_RING_Y - 18, 48, pbDay ? 0x5AA84A : 0x4A4EA8, 2);
  pbEllipse(PB_RING_X, PB_RING_Y, 30, 9, pbDay ? 0x5AA84A : 0x2C3070);  // the spot it stands on
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
// the corners where the wall meets the guides, rounded (pinball.h's PinFillet): the rim carried round the curve, its
// dark inner edge and lit band following it, over the guide's top as far as the curve touches it (the guide comes out
// of the rim there)
static void pbFillets() {
  for (int m = 0; m < 2; m++) {
    const PinFillet &f = pinFillet(m);
    int x0 = (int)(f.fx - PB_FILLET_R - 20) & ~1, y0 = (int)(f.fy - 10) & ~1;
    for (int y = y0; y < f.fy + PB_FILLET_R + 10; y += 2)
      for (int x = x0; x < f.fx + PB_FILLET_R + 20; x += 2) {
        float px = x + 1, py = y + 1, dd = hypotf(px - f.fx, py - f.fy) - PB_FILLET_R;
        if (dd < -2 || hypotf(px - CX, py - CY) >= 233 || !pinInFillet(f, px, py, 2)) continue;
        Rgb c;
        if (dd < 2) {
          c = 0x1B1B25;
        } else {  // (as pbTable() colours the rim, by how far into it)
          float t = fminf(1, (dd - 2) / (233 - PB_R - 2));
          c = pbDay ? scMix(0xB07A45, 0x6E4426, t) : scMix(0xC8CCD8, 0x6A7084, t);
          if (t < 0.25f) c = pbDay ? 0xD8A060 : 0xF0F2F8;
          float a = atan2f(py - CY, px - CX) * 57.29578f;
          if (a < 0) a += 360;
          if (pbDay && ((int)(a * 3) % 7 == 0)) c = scMix(c, 0x5A3820, .4);
        }
        pbPx2(x, y, sc565(c));
      }
  }
}
// a flipper: side -1 left, 1 right, at angle a (PB_FLIP_REST down, PB_FLIP_UP raised)
static void pbFlipperAt(int side, float a) {
  float px = side < 0 ? PB_FLIP_LX : V2_W - 1 - PB_FLIP_LX;
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
static void pbFlipper(int side, bool up) { pbFlipperAt(side, up ? PB_FLIP_UP : PB_FLIP_REST); }
// the slingshots: a red triangle each side over the flippers, a lit corner, its long side a white rubber that kicks
static void pbSlings() {
  for (int m = -1; m <= 1; m += 2) {
    int ax = (int)PB_SLING[0][0], ay = (int)PB_SLING[0][1], by = (int)PB_SLING[1][1], cx = (int)PB_SLING[2][0], cy = (int)PB_SLING[2][1];
    if (m > 0) ax = 465 - ax, cx = 465 - cx;
    int o = m < 0 ? -4 : 4;  // its dark edge, out from the triangle
    gfx->fillTriangle(ax + o + 2, ay - 4, ax + o + 2, by + 4, cx - o + 2, cy + 6, sc565(pbFloorDark()));
    gfx->fillTriangle(ax + o, ay - 8, ax + o, by + 4, cx - o, cy + 6, sc565(0x1B1B25));
    gfx->fillTriangle(ax, ay, ax, by, cx, cy, sc565(0xE8503A));
    gfx->fillTriangle(ax, ay, ax, ay + 18, ax + (cx - ax) / 3, ay + 22, sc565(0xFF8A6A));  // its lit corner
    pbCapsule(ax, ay, cx, cy, 3, 3, 0xF4F4F8);
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
// Diglett's hole in the dirt between the bumpers
static void pbDigHole(int x, int y, int rx, int ry, bool up) {
  const Rgb dirt = pbDay ? 0x8A6A44 : 0x6A5A6E, dirtLit = pbDay ? 0xB08A5A : 0x8A7A90;
  pbEllipse(x, y, rx, ry, 0x1B1B25);
  pbEllipse(x, y, rx - 2, ry - 2, dirtLit);
  pbEllipse(x, y + 1, rx * 3 / 5, ry / 2, up ? dirt : 0x1B1B25);
}
// Diglett's hole between the bumpers; up: Diglett (dex 50) out of it, or Dugtrio (51), the time to hit it a gold ring
// round it (hit: knocked dizzy, stars round its head); heads: Dugtrio's still up,
// a gold dot each under it
static void pbDiglett(bool up, bool hit = false, float timeLeft = 0, int dex = 50, int heads = 0) {
  const Rgb dirtLit = pbDay ? 0xB08A5A : 0x8A7A90;
  int ring = dex == 50 ? 30 : 34;  // (Dugtrio wider, still clear of the bumpers)
  if (up && !hit)
    for (int k = 0; k < 360; k += 3) {
      if (k > timeLeft * 360) break;
      float a = pbRad(k - 90);
      fsSq(PB_DIG_X + (int)(ring * cosf(a)), PB_DIG_Y - 18 + (int)(ring * sinf(a)), 4, 0xF2C230);
    }
  pbDigHole(PB_DIG_X, PB_DIG_Y, 20, 8, up);
  if (!up) return;
  const uint8_t *t = thumbs.getForm(dex, false);
  fsThumb(t, PB_DIG_X, PB_DIG_Y + 2, 2);  // always solid: no white wash over it when hit (Fedor: it looked see-through)
  pbEllipse(PB_DIG_X, PB_DIG_Y + 3, dex == 50 ? 18 : 26, 5, dirtLit);  // the dirt round its foot
  if (!hit)
    for (int k = 0; k < 6; k++)  // clods thrown up as it pops out
      pbEllipse(PB_DIG_X - 26 + k * 10 + (k % 2) * 3, PB_DIG_Y - 6 - (k % 3) * 7, 3, 3, dirtLit);
  for (int k = 0; k < heads; k++) {  // Dugtrio's heads still to hit
    int x = PB_DIG_X - 14 + k * 14, y = PB_DIG_Y + 30;
    pbEllipse(x, y, 5, 5, 0x1B1B25);
    pbEllipse(x, y, 3, 3, 0xF2C230);
  }
  if (hit)
    for (int k = 0; k < 5; k++) {  // dizzy stars
      float a = k * 1.2566f;
      int x = PB_DIG_X + (int)(22 * cosf(a)), y = PB_DIG_Y - 44 + (int)(8 * sinf(a));
      scGridRect(x, y - 4, x + 4, y + 8, 0xFFE27A);
      scGridRect(x - 4, y, x + 8, y + 4, 0xFFE27A);
    }
}
// Diglett evolving into Dugtrio, k of the way through (0 to 1): as a Pokemon evolves at home, a gold glow round it and
// it blinks white between the old form and the new, faster and faster, white sparkles twinkling round it
static void pbDigEvolve(float k) {
  const Rgb dirtLit = pbDay ? 0xB08A5A : 0x8A7A90;
  int g = (int)(3 * sinf(k * 40)) * 2;  // the glow breathes
  twGlow(PB_DIG_X, PB_DIG_Y - 20, 46 + g, 0xFFD95E, 190);
  pbDigHole(PB_DIG_X, PB_DIG_Y, 20, 8, true);
  bool old = k < 0.92f && (int)(k * k * 16) % 2 == 0;  // (the blinks come faster as it goes)
  const uint8_t *t = thumbs.getForm(old ? 50 : 51, false);
  static const int8_t RIM[4][2] = { { -2, 0 }, { 2, 0 }, { 0, -2 }, { 0, 2 } };
  for (int i = 0; i < 4; i++)  // a dark rim round the white, so its shape reads
    fsThumb(t, PB_DIG_X + RIM[i][0], PB_DIG_Y + 2 + RIM[i][1], 2, false, 0x1B1B25, 256);
  fsThumb(t, PB_DIG_X, PB_DIG_Y + 2, 2, false, 0xFFFFFF, 256);
  pbEllipse(PB_DIG_X, PB_DIG_Y + 3, old ? 18 : 26, 5, dirtLit);
  static const int8_t SP[4][2] = { { -40, -50 }, { 38, -44 }, { -30, -6 }, { 44, -10 } };
  for (int i = 0; i < 4; i++) {
    if ((int)(k * 12 + i * 2) % 3 == 0) continue;
    int x = PB_DIG_X + SP[i][0], y = PB_DIG_Y + SP[i][1];
    scGridRect(x, y - 6, x + 2, y + 8, 0xFFFFFF);
    scGridRect(x - 6, y, x + 8, y + 2, 0xFFFFFF);
  }
}
// Tall grass in the spirit of the Game Boy games (Fedor: "grass that resembles grass from Pokemon Blue"): an original
// 16 px tile, 8 x 8 Knomi pixels: two staggered tufts, each a dark edge round light blades, on a mid green (half as
// many as the first tile had: Fedor, "Maybe its the density of the grass thats a bit much"). Tiled to a fixed grid so
// neighbouring patches line up; rustling, every other row of tiles shifts a Knomi pixel
static const char PB_TUFT[8][9] = {
  "........",
  ".D.D....",
  "DLDLD...",
  "DLLLD...",
  "........",
  ".....D.D",
  "....DLDL",
  "D...DLLL",
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
static void pbCave(bool lit);
static void pbGrass(bool lit) {
  if (!pbDay) {  // the night table's is a cave
    pbCave(lit);
    return;
  }
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
// The night table's cave (Fedor: "On the darker version of the game, Lets change the grassy patch with the entrance to
// a cave entrance"), where the meadow's grass is and as big, the same to the ball: a moonlit mound of rock, lumpy along
// its top, its mouth on its lower-left side where the grass's gap is, turned to the left flipper. Two eyes glint in the
// dark of it; lit (the ball going in, a wild one coming out), its mouth glows. A lit arrow on the floor points into it
// an arch: the oval (cx, cy, rx, ry) down to row flat, where it stands on the floor
static void pbArch(int cx, int cy, int rx, int ry, int flat, Rgb col) {
  uint16_t c = sc565(col);
  for (int y = SNAP2(cy - ry); y <= flat; y += 2)
    for (int x = SNAP2(cx - rx); x <= cx + rx; x += 2) {
      float dx = (x + 1 - cx) / (float)rx, dy = y + 1 < cy ? (y + 1 - cy) / (float)ry : 0;
      if (dx * dx + dy * dy <= 1) pbPx2(x, y, c);
    }
}
// the cave's mouth (its middle), and the eyes in it: open, or shut (blinking, or nobody in there: dark)
static void pbCaveMouth(int &mx, int &my) {
  mx = SNAP2((int)(PB_NOTCH[0][0] + (PB_NOTCH[1][0] - PB_NOTCH[0][0]) * 0.55f));
  my = SNAP2((int)(PB_NOTCH[0][1] + (PB_NOTCH[1][1] - PB_NOTCH[0][1]) * 0.55f));
}
static void pbCaveEyes(bool open) {
  int mx, my;
  pbCaveMouth(mx, my);
  Rgb c = open ? 0xFFE27A : 0x16141E;
  scRect(mx - 6, my + 2, mx - 3, my + 5, c);
  scRect(mx + 4, my + 2, mx + 7, my + 5, c);
}
static void pbCave(bool lit) {
  const Rgb rock = 0x5E5A78, rockLit = 0x8A84A8, rockHi = 0xB4AED0, rockDark = 0x3A3652, edge = 0x1B1B25;
  const int cx = PB_GRASS_X, cy = PB_GRASS_Y, rx = PB_GRASS_RX, ry = PB_GRASS_RY;
  static const int8_t BUMP[4][3] = { { -13, -18, 14 }, { 8, -22, 13 }, { 21, -6, 10 }, { -24, 2, 8 } };
  pbEllipse(cx + 4, cy + 8, rx + 2, ry + 2, pbFloorDark(), 140);  // its shadow
  for (int k = 0; k < 4; k++) pbEllipse(cx + BUMP[k][0], cy + BUMP[k][1], BUMP[k][2] + 3, BUMP[k][2] + 3, edge);
  pbEllipse(cx, cy, rx + 3, ry + 3, edge);
  for (int k = 0; k < 4; k++) {  // boulders along its top: each dark on its far side, lit from the top left
    int bx = cx + BUMP[k][0], by = cy + BUMP[k][1], r = BUMP[k][2];
    pbEllipse(bx, by, r, r, rockDark);
    pbEllipse(bx - 2, by - 2, r - 2, r - 2, rock);
    pbEllipse(bx - 4, by - 4, r - 6, r - 6, rockLit);
  }
  pbEllipse(cx, cy, rx, ry, rockDark);  // the mound
  pbEllipse(cx - 3, cy - 3, rx - 4, ry - 4, rock);
  pbEllipse(cx - 6, cy - 10, rx - 14, ry - 16, rockLit);
  pbEllipse(cx - 10, cy - 14, 4, 3, rockHi);
  pbEllipse(cx - 18, cy - 26, 3, 2, rockHi);
  pbEllipse(cx + 2, cy - 32, 3, 2, rockHi);
  scRect(cx + 8, cy - 12, cx + 9, cy - 2, edge);  // cracks
  scRect(cx + 10, cy - 2, cx + 11, cy + 4, edge);
  scRect(cx - 4, cy - 22, cx + 1, cy - 21, edge);
  // the mouth, where the grass's gap is: an arch standing on the floor, a lit lip of rock round it, darker deeper in
  float ax = PB_NOTCH[0][0], ay = PB_NOTCH[0][1], bx = PB_NOTCH[1][0], by = PB_NOTCH[1][1];
  int mx, my;
  pbCaveMouth(mx, my);
  const int flat = my + 12;
  if (lit) twGlow(mx, my, 40, 0xFFE27A, 170);
  pbArch(mx, my, 17, 17, flat, rockLit);
  pbArch(mx, my, 15, 15, flat, edge);
  pbArch(mx, my + 2, 12, 12, flat, 0x16141E);
  pbArch(mx + 1, my + 2, 8, 8, flat, 0x0A0A12);
  if (lit) twGlow(mx, my + 4, 16, 0xFFE27A, 110);  // (lit: a gold haze in the dark, no eyes)
  pbEllipse(mx, flat + 2, 14, 3, pbFloorDark());  // the way in, worn and shaded
  for (int k = 0; k < 3; k++) {  // rocks hanging from the arch
    int tx = SNAP2(mx - 7 + k * 7), ty = SNAP2(my - 12 + (k == 1 ? -2 : 0));
    scRect(tx - 2, ty, tx + 3, ty + 3, edge);
    scRect(tx, ty + 4, tx + 1, ty + 5, edge);
  }
  if (!lit) pbCaveEyes(true);  // two eyes in the dark: something waits in there
  for (int k = 0; k < 3; k++)  // pebbles by the way in
    pbEllipse(mx - 22 + k * 8, flat + 6 + (k % 2) * 4, 3, 2, k == 1 ? rockLit : rock);
  float dx = bx - ax, dy = by - ay;
  pbArrow((int)(ax - dx * 1.05f), (int)(ay - dy * 1.05f), dx, dy, lit ? 0xFFE27A : 0xB06AE0);
}
// the patch's front, over a ball that has gone into it (from row y down): only its lower half shows over the blades (at
// night, the cave: the ball gone into it)
static void pbGrassFront(int y, bool lit) {
  if (!pbDay) pbCave(lit);
  else pbGrassOval(PB_GRASS_X, PB_GRASS_Y, PB_GRASS_RX, PB_GRASS_RY, lit, SNAP2(y), true);
}
// Something in the grass (Fedor: "make it look like there is a pokemon rustling through the grass there"): unseen,
// it wanders round the patch, two seconds on the move, then a moment still. Where it is, the blades shake over a
// shadow, and bits of leaf jump up while it moves; still, they only twitch now and then. t: seconds
#define PB_RUSTLE_R 10
static void pbGrassRustle(float t) {
  const float period = 3.2f, move = 2.0f;
  float cyc = fmodf(t, period), way = floorf(t / period) * move + fminf(cyc, move);  // (how far it has gone)
  bool moving = cyc < move;
  float sx = PB_GRASS_X + 4 + 12 * sinf(way * 0.9f), sy = PB_GRASS_Y - 6 + 12 * sinf(way * 1.37f + 1);
  int shake = moving ? ((int)(t * 10) % 2 ? 4 : -4) : ((int)(t * 4) % 6 == 0 ? 2 : 0);
  Rgb b, l, d;
  pbGrassCols(b, l, d);
  const uint16_t B = sc565(b), L = sc565(l), D = sc565(d), S = sc565(scMix(b, d, 0.8f));
  float ax = PB_NOTCH[0][0], ay = PB_NOTCH[0][1], dx = PB_NOTCH[1][0] - ax, dy = PB_NOTCH[1][1] - ay;
  for (int y = SNAP2((int)sy - PB_RUSTLE_R); y <= sy + PB_RUSTLE_R; y += 2)
    for (int x = SNAP2((int)sx - PB_RUSTLE_R); x <= sx + PB_RUSTLE_R; x += 2) {
      float r2 = (x + 1 - sx) * (x + 1 - sx) + (y + 1 - sy) * (y + 1 - sy);
      if (r2 > PB_RUSTLE_R * PB_RUSTLE_R) continue;
      float ex = (x + 1 - PB_GRASS_X) / (float)PB_GRASS_RX, ey = (y + 1 - PB_GRASS_Y) / (float)PB_GRASS_RY;
      if (ex * ex + ey * ey > 1) continue;
      float k = ((x + 1 - ax) * dx + (y + 1 - ay) * dy) / (dx * dx + dy * dy);  // (not in the gap)
      k = k < 0 ? 0 : k > 1 ? 1 : k;
      float qx = x + 1 - ax - k * dx, qy = y + 1 - ay - k * dy;
      if (qx * qx + qy * qy < 12 * 12) continue;
      uint16_t c = pbTuftAt(x + shake, y + (r2 < 30 ? 2 : 0), false, B, L, D);
      if (r2 < 56 && c != L) c = S;  // its shadow under the blades
      pbPx2(x, y, c);
    }
  for (int k = -1; k <= 1; k++) {  // blades pushed up over it, swaying as it goes
    int sway = moving ? ((int)(t * 10) + k) % 2 ? 2 : -2 : 0;
    int x = SNAP2((int)sx + k * 6 + sway), y = SNAP2((int)sy - 6 - (k ? 0 : 4));
    scRect(x - 2, y - 10, x + 3, y + 1, d);
    scRect(x + sway, y - 12, x + sway + 1, y - 11, d);
    scRect(x, y - 10, x + 1, y - 1, l);
  }
  if (!moving) return;
  for (int k = 0; k < 3; k++) {  // bits of leaf jumping up and falling back
    float life = fmodf(t * 1.7f + k * 0.37f, 1.0f);
    int x = SNAP2((int)(sx + (k - 1) * (4 + 10 * life))), y = SNAP2((int)(sy - 12 - 22 * sinf(life * 3.14159f)));
    scRect(x - 4, y - 2, x + 3, y + 3, d);
    scRect(x - 2, y, x + 1, y + 1, l);
  }
}
// the arrows up the ramp (Fedor: "maybe have some arrows or something going upward up the ramp"), on its climb: arrow k
// (0 the lowest) unlit, or lit, as they chase up it one after another (pinDraw())
#define PB_RAMP_ARROWS 5
static void pbRampArrow(int k, bool lit) {
  const PinTrack &tr = pinTrack();
  const int climb = PB_RAMP_SEGS - PB_RAMP_TURN_SEGS;
  float x, y, dx, dy, s = tr.s[climb] * (0.14f + 0.17f * k);
  pinTrackAt(s, x, y, dx, dy);
  float nx = -dy, ny = dx, tx = x + dx * 5, ty = y + dy * 5;
  float lx = x - dx * 4 + nx * 8, ly = y - dy * 4 + ny * 8, rx = x - dx * 4 - nx * 8, ry = y - dy * 4 - ny * 8;
  if (lit) {
    pbCapsule(lx, ly, tx, ty, 3, 3, 0x1B1B25);
    pbCapsule(rx, ry, tx, ty, 3, 3, 0x1B1B25);
  }
  Rgb c = lit ? 0xFFE27A : (pbDay ? 0xD89A5A : 0x6A7AC8);
  pbCapsule(lx, ly, tx, ty, 2, 2, c);
  pbCapsule(rx, ry, tx, ty, 2, 2, c);
}
// what's raised over the floor: the ramp (a see-through plastic lane between two walls, rising, so its shadow falls
// further off toward its top, and round over into the wall) and the wire rail it becomes, two wires on ties down
// the rim over the orbit, to a cup where it lets the ball down into the left inlane. Both along pinball.h's track
static void pbRaised(bool rampLit = false) {
  const PinTrack &tr = pinTrack();
  const int N = PB_RAMP_SEGS, W = PB_TRACK_N - 1;
  const float *px = tr.x, *py = tr.y;
  for (int i = 0; i < N; i++) {  // the ramp's shadow, further off the higher it is
    float t = i / (float)N, ox = 4 + 9 * t, oy = 6 + 12 * t;
    pbCapsule(px[i] + ox, py[i] + oy, px[i + 1] + ox, py[i + 1] + oy, 17, 17, pbFloorDark(), 110);
  }
  for (int i = N; i < W; i++)  // the wire's shadow
    pbCapsule(px[i] + 10, py[i] + 14, px[i + 1] + 10, py[i + 1] + 14, 9, 9, pbFloorDark(), 70);
  for (int i = 0; i < N; i++)  // the ramp's see-through floor, with arrows up it (unlit: pbRampArrow())
    pbCapsule(px[i], py[i], px[i + 1], py[i + 1], 14, 14, rampLit ? 0xFFF3B0 : (pbDay ? 0xF8ECC0 : 0xA8C4F8), 105);
  for (int k = 0; k < PB_RAMP_ARROWS; k++) pbRampArrow(k, false);
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
  // the wire rail: two wires on ties, from the ramp's end down the rim
  for (int i = N; i <= W; i += 3) {  // the ties
    int j = i < W ? i : W - 1;
    float dx2 = px[j + 1] - px[j], dy2 = py[j + 1] - py[j], L2 = hypotf(dx2, dy2), nx = -dy2 / L2 * 10, ny = dx2 / L2 * 10;
    pbCapsule(px[i] - nx, py[i] - ny, px[i] + nx, py[i] + ny, 3, 3, 0x1B1B25);
    pbCapsule(px[i] - nx, py[i] - ny, px[i] + nx, py[i] + ny, 1, 1, 0x9AA0B4);
  }
  for (int w = -1; w <= 1; w += 2)
    for (int pass = 0; pass < 2; pass++)
      for (int i = N; i < W; i++) {
        float dx2 = px[i + 1] - px[i], dy2 = py[i + 1] - py[i], L2 = hypotf(dx2, dy2), nx = -dy2 / L2 * 9 * w, ny = dx2 / L2 * 9 * w;
        if (pass == 0) {
          pbCapsule(px[i] + nx, py[i] + ny, px[i + 1] + nx, py[i + 1] + ny, 3, 3, 0x1B1B25);
        } else {
          pbCapsule(px[i] + nx, py[i] + ny, px[i + 1] + nx, py[i + 1] + ny, 2, 2, 0xD8DCE8);
          pbCapsule(px[i] + nx, py[i] + ny - 1, px[i + 1] + nx, py[i + 1] + ny - 1, 1, 1, 0xFFFFFF);
        }
      }
  float ex = px[W], ey = py[W];  // where it lets the ball down: a little cup at its end
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
// ===================================================================== the battle in the ring
static void fxBurst(uint8_t ty, int cx, int cy, float k);  // (the battle's, further on in the sketch)
// the wild one in the ring (Fedor: "The battles need to happen inside the pinball game"): its jump in from the grass,
// it standing there with its HP over it and the time left a gold ring round it, a hit's white flash and your move's
// burst on it (moveType its type), then knocked out (a white flash fading) or off back into the grass
static void pbWild(const PinState &s, uint8_t moveType) {
  if (!s.btlOn || !s.wildMax) return;
  const uint8_t *t = thumbs.getForm(s.wild, false);
  int fx = PB_RING_X, fy = PB_RING_Y;
  if (s.inT > 0) {  // the jump: from the grass to the ring, in an arc
    float k = 1 - s.inT / PB_BTL_IN_S;
    fx = (int)(PB_GRASS_X + (PB_RING_X - PB_GRASS_X) * k);
    fy = (int)(PB_GRASS_Y + (PB_RING_Y - PB_GRASS_Y) * k - 90 * sinf(k * 3.14159f));
  } else if (s.outT > 0 && !s.won) {  // gone: back off to the grass, the same way
    float k = s.outT / PB_BTL_OUT_S;
    k = k > 1 ? 1 : k;
    fx = (int)(PB_RING_X + (PB_GRASS_X - PB_RING_X) * k);
    fy = (int)(PB_RING_Y + (PB_GRASS_Y - PB_RING_Y) * k - 90 * sinf(k * 3.14159f));
  }
  if (s.inT <= 0 && s.outT <= 0) {  // the time left
    for (int a = 0; a < 360; a += 4) {
      if (a > s.btlLeft / PB_BTL_S * 360) break;
      float r = pbRad(a - 90);
      fsSq(PB_RING_X + (int)(44 * cosf(r)), PB_RING_Y - 22 + (int)(44 * sinf(r)), 4, 0xF2C230);
    }
  }
  if (s.outT > 0 && s.won) {  // knocked out: a white flash, fading
    int a = (int)(220 * (1 - s.outT / PB_BTL_OUT_S));
    if (a > 0) fsThumb(t, fx, fy, 2, false, 0xFFFFFF, a);
    return;
  }
  pbEllipse(fx + 2, fy + 2, 22, 7, pbFloorDark(), 140);  // its shadow
  fsThumb(t, fx, fy, 2);
  if (s.hitT > PB_BTL_HIT_S * 0.6f) fsThumb(t, fx, fy, 2, false, 0xFFFFFF, 170);  // hit: a white flash
  if (s.hitT > 0) fxBurst(moveType, fx, fy - 26, 1 - s.hitT / PB_BTL_HIT_S);
  if (s.inT <= 0 && s.outT <= 0) {  // its HP, over it
    int bx = fx - 30, by = fy - 72;
    gfx->fillRoundRect(bx - 2, by - 2, 64, 10, 4, sc565(0x1B1B25));
    gfx->fillRoundRect(bx, by, 60, 6, 3, sc565(0x4A4660));
    float pct = s.wildHp / (float)s.wildMax;
    int w = (int)(60 * pct);
    if (w > 1) gfx->fillRoundRect(bx, by, w, 6, 3, sc565(pct > 0.5f ? 0x58B868 : pct > 0.2f ? 0xF2C230 : 0xE8503A));
  }
}

// ===================================================================== the game, drawn from pinball.h's state
// What never changes (the floor, the orbit and its lights unlit, the guides, slingshots, Diglett's hole, the bumpers
// unlit, the ring, the grass) is painted once a look into a picture of the floor, and the ramp and the
// wire over that into a second. Each frame starts from the second and draws what changes over it (lit things,
// Diglett, the flippers, the ball, the score); a ball on the floor under the ramp or the wire gets them put back over
// it, wherever the two pictures differ. Two pictures of the screen (434 KB each); without the room, it's all painted
// every frame
static uint16_t *pbPicFloor = nullptr, *pbPic = nullptr;
static int32_t pbPicKey = -1;  // the look they were painted in
static void pbPaintStatic() {
  pbTable();
  pbOrbit();
  for (int k = 0; k < 6; k++) pbBerry(k, false);
  float wx = CX + PB_R * cosf(PB_GUIDE_A), wy = CY + PB_R * sinf(PB_GUIDE_A);
  pbRail(wx, wy, PB_GUIDE_END[0], PB_GUIDE_END[1]);
  pbRail(465 - wx, wy, 465 - PB_GUIDE_END[0], PB_GUIDE_END[1]);
  pbFillets();
  pbSlings();
  pbDiglett(false);
  for (int k = 0; k < 3; k++) pbBumper(k, false);
  pbGrass(false);
}
// the raised parts put back over (x0, y0)..(x1, y1): wherever the picture with them differs from the floor's
static void pbRaisedOver(int x0, int y0, int x1, int y1) {
  if (!pbPic || !pbPicFloor || pbPicKey < 0) return;
  x0 = x0 < 0 ? 0 : x0, y0 = y0 < 0 ? 0 : y0, x1 = x1 > V2_W - 1 ? V2_W - 1 : x1, y1 = y1 > V2_W - 1 ? V2_W - 1 : y1;
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  for (int y = y0; y <= y1; y++) {
    const int r = y * V2_W;
    for (int x = x0; x <= x1; x++)
      if (pbPic[r + x] != pbPicFloor[r + x]) fb[r + x] = pbPic[r + x];
  }
}
// a score as the game shows it
static void pinScoreText(uint32_t v, char *b, int n) { snprintf(b, n, "%lu", (unsigned long)v); }
// the keys over the flippers on a computer (Left and Right flip, Space both), in place of the touch hint
static void pbKeysHint(int y) {
  setSize(2);
  const char *flip = T(S_PIN_FLIP), *both = T(S_PIN_BOTH), *space = T(S_PIN_SPACE);
  int w = 30 + 6 + 30 + 10 + textW(flip, 2), x = CX - w / 2;
  x += pbKey(x, y, -1) + 6;
  x += pbKey(x, y, 1) + 10;
  fsSay(flip, x + textW(flip, 2) / 2, y + 6, 0xFFFFFF);
  int w2 = textW(space, 2) + 16 + 10 + textW(both, 2), x2 = CX - w2 / 2;
  x2 += pbKey(x2, y + 38, 0, space) + 10;
  fsSay(both, x2 + textW(both, 2) / 2, y + 44, 0xFFFFFF);
}
// the table, s at this moment (keys: a computer's, the hint shows its keys)
static void pinDraw(const PinState &s, bool keys) {
  pbDay = s.day;
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  scScreen();
  int32_t key = s.day ? 1 : 0;
  if (!pbPic) pbPic = (uint16_t *)ps_malloc(V2_W * V2_W * sizeof(uint16_t));
  if (!pbPicFloor) pbPicFloor = (uint16_t *)ps_malloc(V2_W * V2_W * sizeof(uint16_t));
  if (pbPic && pbPicFloor) {
    if (pbPicKey != key) {
      pbPaintStatic();
      memcpy(pbPicFloor, fb, V2_W * V2_W * sizeof(uint16_t));
      pbRaised(false);
      memcpy(pbPic, fb, V2_W * V2_W * sizeof(uint16_t));
      pbPicKey = key;
    } else {
      memcpy(fb, pbPic, V2_W * V2_W * sizeof(uint16_t));
    }
  } else {
    pbPaintStatic();
    pbRaised(false);
  }
  // what changes, on the floor: berries lit (blinking just after all six), bumpers, Diglett, the grass, the wild one
  for (int k = 0; k < 6; k++)
    if (((s.berryLit >> k) & 1) || (s.berryFlash > 0 && (int)(s.berryFlash * 6) % 2)) {
      float a = pbRad(PB_BERRY_A[k]);
      int x = (int)(CX + PB_WIRE_R * cosf(a)), y = (int)(CY + PB_WIRE_R * sinf(a));
      pbBerry(k, true);
      pbRaisedOver(x - 26, y - 26, x + 26, y + 26);
    }
  for (int k = 0; k < 3; k++)
    if (s.bumpFlash[k] > 0 || ((s.bumpLit >> k) & 1)) pbBumper(k, true);
  // Diglett, or Dugtrio (up, or just knocked dizzy), or it evolving
  bool dugOn = s.dug || (s.lastDug && s.digHitT > 0);
  if (s.digUp && s.evoT > 0) pbDigEvolve(1 - s.evoT / PB_EVO_S);
  else if (s.digUp || s.digHitT > 0)
    pbDiglett(true, !s.digUp, s.digUp ? s.digT / (s.dug ? PB_DUG_S : PB_DIG_S) : 0, dugOn ? 51 : 50, s.digUp ? s.dugHeads : 0);
  if (s.grassT > 0 || s.phase == PIN_CATCH) pbGrass(true);
  else if (!s.btlOn) {  // something in there: rustling through the grass, or eyes blinking in the cave
    float t = millis() / 1000.0f;
    if (pbDay) pbGrassRustle(t);
    else if (fmodf(t, 3.7f) < 0.18f) pbCaveEyes(false);
  } else if (!pbDay) pbCaveEyes(false);  // (out in the ring: nobody in there)
  pbWild(s, MOVE_TBL[s.move].type);
  pbFlipperAt(-1, s.flipA[0]);
  pbFlipperAt(1, s.flipA[1]);
  // the ball: rolled as it goes; up the ramp nearer (bigger) and its shadow further off; on the floor under the ramp or
  // the wire, them back over it; in the grass, the blades in front of it
  bool showBall = s.phase == PIN_PLAY || s.phase == PIN_CATCH || (s.phase == PIN_LOST && s.t < 0.3f);
  // the arrows up the ramp, lit one after another up it (two beats dark between runs): under a ball on the ramp, over
  // one on the floor
  int arrow = (int)(s.playT * 5) % (PB_RAMP_ARROWS + 2);
  bool arrowUnder = showBall && s.onTrack;
  if (arrowUnder && arrow < PB_RAMP_ARROWS) pbRampArrow(arrow, true);
  if (showBall) {
    int x = (int)s.x, y = (int)s.y;
    if (s.onTrack) {
      const PinTrack &tr = pinTrack();
      float up = s.s < tr.rampLen ? s.s / tr.rampLen : 1;
      pbBall(x, y, s.vx, s.vy, s.roll, PB_BALL_R + (int)(2 * up), 4 + (int)(10 * up));
    } else {
      pbBall(x, y, s.vx, s.vy, s.roll);
      if (s.phase == PIN_CATCH) pbGrassFront(y - 2, true);
      else pbRaisedOver(x - 16, y - 16, x + 22, y + 24);
    }
  }
  if (!arrowUnder && arrow < PB_RAMP_ARROWS) pbRampArrow(arrow, true);
  // what it says
  char b[40];
  if (s.phase == PIN_LOST) {  // the bottom glowing red; the next ball's number
    for (int yy = 300; yy < V2_W; yy += 4)
      for (int xx = 0; xx < V2_W; xx += 4) {
        float d = 233 - hypotf(xx + 2 - CX, yy + 2 - CY);
        if (d < 0 || d > 30) continue;
        fsBlend(xx, yy, xx + 3, yy + 3, 0xE8503A, (int)(170 * (1 - d / 30)));
      }
    if (s.ballsLeft && s.t > 0.4f) {
      snprintf(b, sizeof(b), T(S_PIN_BALL_FMT), (unsigned)(s.ball + 1));
      fsBoard(b, 214);
    }
  }
  if (s.phase == PIN_CATCH) fsBang(PB_GRASS_X - 4, PB_GRASS_Y - 54);
  if (s.btlOn && s.wildMax) {  // the battle's lines, under the ring: who came out, each hit's move, how it ended
    b[0] = 0;
    if (s.outT > 0 && s.won) snprintf(b, sizeof(b), "%s", T(S_BTL_WON));
    else if (s.outT > 0) snprintf(b, sizeof(b), T(S_FLED_FMT), dexName(s.wild));
    else if (s.hitT > 0) snprintf(b, sizeof(b), "%s: %s!", v2MyName(pet.speciesId), MOVE_TBL[s.move].name);
    else if (s.inT > 0 || s.btlLeft > PB_BTL_S - 1.2f) snprintf(b, sizeof(b), T(S_BTL_WILD), dexName(s.wild));
    if (b[0]) {
      char c[40];
      fsSay(v2Caps(b, c, sizeof(c)), CX, 334, s.outT > 0 && s.won ? 0xFFE27A : 0xFFFFFF);
    }
  }
  if (s.onTrack && s.rampTop) fsSay(T(S_PIN_RAMP), 184, 216, 0xFFE27A, 3);
  if (s.digUp && s.evoT > 0) {  // Diglett evolving, then Dugtrio: hit it
    char c[40];
    fsBoard(v2Caps(T(S_EVOLVING), c, sizeof(c)), 206);
  } else if (s.digUp && s.digT > (s.dug ? PB_DUG_S : PB_DIG_S) - 1.6f) {
    snprintf(b, sizeof(b), T(S_PIN_DIG_FMT), dexName(s.dug ? 51 : 50));
    char c[40];
    fsBoard(v2Caps(b, c, sizeof(c)), 206);
  }
  if (s.berryFlash > 0) {
    snprintf(b, sizeof(b), T(S_PIN_BERRIES_FMT), (unsigned)s.mult);
    fsSay(b, CX, 214, 0xFFE27A, 4);
  }
  if (s.popT > 0 && s.popPts) {  // the latest points, rising where they were made
    snprintf(b, sizeof(b), "+%u", (unsigned)s.popPts);
    int px = (int)s.popX, py = (int)(s.popY - (1 - s.popT) * 24);
    px = px < 60 ? 60 : px > 406 ? 406 : px;
    py = py < 110 ? 110 : py;  // (under the score and the balls)
    fsSay(b, px, py, 0xFFE27A, 3);
  }
  if (s.phase == PIN_PLAY && s.ball == 1 && s.playT < PB_HINT_S) {
    if (keys) pbKeysHint(318);
    else fsSay(T(S_PIN_HINT), CX, 344, 0xFFFFFF);
  }
  char sc[16], mult[8];
  pinScoreText(s.score, sc, sizeof(sc));
  snprintf(mult, sizeof(mult), "x%u", (unsigned)s.mult);
  pbHud(sc, s.phase == PIN_END ? 0 : s.ballsLeft + (s.phase == PIN_LOST ? 0 : 1), s.mult > 1 ? mult : nullptr);
}

