// V2: PokeFish MOCKUPS (not the game yet). A fishing minigame for the Games menu, played off the end of a pier at the
// Beach. The Knomi has no speaker and no vibration, so every moment of a cast reads on the screen alone (the float,
// rings, splashes, a "!", the rim's glow, the line's colour, the rod's bend); the computers and the web add sound, and
// phones and controllers a buzz, on top. These painters only draw still screens for the console's "FISHMOCK n"
// (1..13), so the look can be judged in the real game before any of it is built. scene_v2.h includes this after
// tower_v2.h. The plan is docs/FISHING.md.

static uint8_t fsMock = 0;  // the mockup on screen (0: none)
void fishMockSet(uint8_t n) { fsMock = n; }

// ===================================================================== the scene
// Seen from the side, cut away at the waterline: the sky down to the surface, the sea under it getting deeper to the
// right, the sand sloping away under the pier. Your Pokemon sits at the pier's end on the left with its rod; the float
// rides the surface on the right, its hook hanging under it where the fish come.
#define FS_SURF 208      // the water's surface
#define FS_DECK 184      // the pier's deck: your Pokemon's feet
#define FS_PIER_X1 168   // the pier's end
#define FS_PET_X 104
#define FS_FLOAT_X 304   // where the float sits in most of the mockups
static const Rgb FS_WATER[4] = { 0x62B6E2, 0x4C9CD6, 0x3B82C6, 0x2D66AE };
static const Rgb FS_WATER_N[4] = { 0x24466E, 0x1E3C62, 0x183256, 0x12284A };  // at night
static bool fsNight = false;

static int fsSandY(int x) { return x < 140 ? 356 + x / 10 : 370 + (x - 140) * 62 / 100; }

// a cell (or any box) pulled a256/256 toward col
static void fsBlend(int x0, int y0, int x1, int y1, Rgb col, int a256) {
  uint16_t c = sc565(col);
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > V2_W - 1) x1 = V2_W - 1;
  if (y1 > V2_W - 1) y1 = V2_W - 1;
  for (int y = y0; y <= y1; y++) {
    uint16_t *q = scFb + y * V2_W;
    for (int x = x0; x <= x1; x++) q[x] = v2Blend(q[x], c, a256);
  }
}
static void fsSq(int x, int y, int w, Rgb col) {  // a w x w square centred on (x, y), on the 2 px grid
  x = SNAP2(x - w / 2), y = SNAP2(y - w / 2);
  scRect(x, y, x + w - 1, y + w - 1, col);
}

static const char *const FS_WEED[8] = { "..g.", ".gG.", ".g..", "..g.", ".Gg.", ".g..", "..g.", ".gG." };
static void fsPaintScene(int night) {
  const ScPal &p = night ? SC_NIGHT : SC_DAY;
  scGradeTod = 0;
  Rgb sky[3] = { p.top, scMix(p.top, p.bot, .5), p.bot };
  const int16_t se[4] = { 0, 100, 160, FS_SURF };
  scBands(sky, se, 3, 0, V2_W);
  scSun(p, night, 388, 92);
  scCloud(p, 20, 132, 3);
  scCloud(p, 236, 150, 2);
  // a far islet on the horizon, faded toward the sky: Knomi Island seen from its pier
  Rgb isle = scMix(night ? 0x1E2E40 : 0x86C58E, p.bot, .45), isleLit = scMix(isle, 0xFFFFFF, night ? .05 : .2);
  scOval(scE(324, FS_SURF - 26, 452, FS_SURF + 40), isle, isleLit);
  ScKey palm[5];
  scPalmKeys(palm, p);
  for (auto &k : palm) k.c = scMix(k.c, p.bot, .45);
  scSprite(SPR_V2_PALM, 12, 380, FS_SURF - 26 - 30, 3, palm, 5);
  // the sea: bands getting deeper, the light coming down through it in shafts
  const Rgb *W = night ? FS_WATER_N : FS_WATER;
  const int16_t we[5] = { FS_SURF, 264, 326, 394, V2_W };
  scBands(W, we, 4, 0, V2_W);
  for (int y = FS_SURF + 4; y < 380; y += 4)
    for (int x = 0; x < V2_W; x += 4) {
      int u = scMod(x - (y - FS_SURF) * 6 / 10, 112);
      if (u >= 20) continue;
      if (y > 300 && (scFl4(x) + scFl4(y)) % 2) continue;
      fsBlend(x, y, x + 3, y + 3, 0xFFFFFF, (night ? 14 : 34) * (380 - y) / 172);
    }
  // the surface: a light line with crests stepping up from it
  Rgb surf = night ? 0x4A6C98 : 0xBFE6F5, crest = night ? 0x6A8CB8 : 0xFFFFFF;
  scRect(0, FS_SURF, V2_W, FS_SURF + 3, surf);
  for (int x = 4; x < V2_W; x += 32) {
    scRect(x, FS_SURF - 4, x + 11, FS_SURF - 1, surf);
    scRect(x + 4, FS_SURF - 4, x + 7, FS_SURF - 1, crest);
  }
  // the sand under the pier, falling away into the deep
  Rgb sand = night ? 0x4A4A5A : 0xE2C88A, sandLit = night ? 0x5A5A6A : 0xF2DCA4, sandSh = night ? 0x3A3A4A : 0xC4A66C;
  for (int x = 0; x < V2_W; x += 4) {
    int top = scFl4(fsSandY(x)) * 4;
    for (int y = top; y < V2_W; y += 4) {
      Rgb c = y == top ? sandLit : ((x / 4 * 7 + y / 4 * 3) % 11 == 0 ? sandSh : sand);
      scCell(x, y, scMix(c, W[3], .18));
    }
  }
  ScKey weed[2] = { { 'g', (Rgb)(night ? 0x2A5A3A : 0x58B868) }, { 'G', (Rgb)(night ? 0x1E4A2E : 0x3C8A4C) } };
  scSprite(FS_WEED, 8, 196, fsSandY(196) - 30, 4, weed, 2);
  scSprite(FS_WEED, 8, 220, fsSandY(220) - 22, 3, weed, 2);
  scSprite(FS_WEED, 8, 60, fsSandY(60) - 26, 3, weed, 2);
  const ScKey shell[3] = { { 'p', (Rgb)(night ? 0x7A5A6A : 0xF08AA4) }, { 'w', (Rgb)(night ? 0x8A8A9A : 0xFFFFFF) }, { 'o', (Rgb)(night ? 0x7A5A4A : 0xF5863D) } };
  scSprite(SPR_V2_STARFISH, 4, 112, fsSandY(112) + 8, 3, shell, 3);
  ScKey rock[5];
  scTreeKeys(rock, p, night ? 0x4A4A5A : 0x9A94B0, night ? 0x2A2A3A : 0x5A5470);
  scSprite(SPR_V2_ROCK, 5, 236, fsSandY(236) - 8, 3, rock, 5);
  // the pier: posts down to the sand (the water over them below the surface), then the deck
  Rgb post = night ? 0x3A2A1E : 0x7E5530, postLit = night ? 0x4A3626 : 0x9A6F43;
  for (int px : { 16, 72, 128 }) {
    for (int y = FS_DECK + 16; y < fsSandY(px + 6) + 4; y += 4) {
      Rgb a = scMix(post, W[0], y > FS_SURF ? .45 : 0), b = scMix(postLit, W[0], y > FS_SURF ? .45 : 0);
      scRect(px, y, px + 3, y + 3, b);
      scRect(px + 4, y, px + 11, y + 3, a);
    }
  }
  Rgb top = night ? 0x6A5038 : 0xD8A468, face = night ? 0x4A3626 : 0xB07A45, seam = night ? 0x2A1E14 : 0x6A4428;
  scRect(0, FS_DECK, FS_PIER_X1, FS_DECK + 3, top);
  scRect(0, FS_DECK + 4, FS_PIER_X1, FS_DECK + 15, face);
  for (int x = 20; x < FS_PIER_X1; x += 24) scRect(x, FS_DECK + 4, x + 1, FS_DECK + 15, seam);
  scRect(0, FS_DECK + 16, FS_PIER_X1, FS_DECK + 19, seam);
  scRect(FS_PIER_X1 - 12, FS_DECK - 12, FS_PIER_X1 - 1, FS_DECK - 1, face);  // the mooring post at its end
  scRect(FS_PIER_X1 - 12, FS_DECK - 12, FS_PIER_X1 - 1, FS_DECK - 9, top);
}
static void fsScene() {
  v2Cached(0x3F400000 + fsNight, fsPaintScene, fsNight);
  scScreen();
}

// ===================================================================== the pieces
// a thumbnail standing with its feet at (cx, feet), s px a pixel; flip: facing the other way; sil/silA: a see-through
// shadow of it (a256), as a fish under the water
static int16_t fsClip[4] = { 0, 0, V2_W, V2_W };  // what fsThumb may draw in (x0, y0, x1, y1): a slot keeps a big one in
static void fsThumb(const uint8_t *b, int cx, int feet, int s, bool flip = false, Rgb sil = SC_NONE, int silA = 256) {
  if (!b) return;
  int w = b[0], h = b[1], n = b[2];
  const uint8_t *pal = b + 3, *d = pal + n * 2;
  int l = w, r = -1, bot = -1;
  for (int i = 0; i < w * h; i++)
    if (d[i] != 0xFF) {
      if (i % w < l) l = i % w;
      if (i % w > r) r = i % w;
      bot = i / w;
    }
  if (bot < 0) return;
  int x0 = SNAP2(cx - (r - l + 1) * s / 2), y0 = SNAP2(feet - (bot + 1) * s);
  for (int y = 0; y < h; y++)
    for (int x = l; x <= r; x++) {
      uint8_t k = d[y * w + x];
      if (k == 0xFF) continue;
      int px = x0 + (flip ? r - x : x - l) * s, py = y0 + y * s;
      if (px < fsClip[0] || py < fsClip[1] || px + s > fsClip[2] || py + s > fsClip[3]) continue;
      if (sil != SC_NONE) fsBlend(px, py, px + s - 1, py + s - 1, sil, silA);
      else gfx->fillRect(px, py, s, s, (uint16_t)(pal[k * 2] | (pal[k * 2 + 1] << 8)));
    }
}
// the thumbnail's height at s (for placing what's over its head)
static int fsThumbH(const uint8_t *b, int s) {
  if (!b) return 40 * s;
  int w = b[0], h = b[1], n = b[2], top = h, bot = -1;
  const uint8_t *d = b + 3 + n * 2;
  for (int i = 0; i < w * h; i++)
    if (d[i] != 0xFF) {
      if (i / w < top) top = i / w;
      bot = i / w;
    }
  return bot < 0 ? 0 : (bot - top + 1) * s;
}

// a fish's shadow under the water, its head toward dir (-1 left, 1 right), s px a pixel (3 small, 4 middling, 6 big)
static const char *const FS_FISH[7] = {
  "....xxxxx.....",
  "..xxxxxxxxx..x",
  ".xxxxxxxxxxxxx",
  "xxxxxxxxxxxxx.",
  ".xxxxxxxxxxxxx",
  "..xxxxxxxxx..x",
  "....xxxxx.....",
};
static void fsShadow(int cx, int cy, int s, int dir, int a256 = 150) {
  Rgb col = fsNight ? 0x050A18 : 0x0E2A5C;
  int x0 = SNAP2(cx - 7 * s), y0 = SNAP2(cy - 7 * s / 2);
  for (int r = 0; r < 7; r++)
    for (int c = 0; c < 14; c++) {
      if (FS_FISH[r][dir < 0 ? c : 13 - c] != 'x') continue;
      fsBlend(x0 + c * s, y0 + r * s, x0 + c * s + s - 1, y0 + r * s + s - 1, col, a256);
    }
}

// the rod: a cork grip, a dark shaft thinning to its tip, the reel under it; bend pulls its middle toward (bx, by)
static void fsRod(int hx, int hy, int tx, int ty, int bx = 0, int by = 0) {
  int gx = hx - 14, gy = hy + 12;  // its butt, behind the hand
  float mx = (gx + tx) / 2.0f + bx, my = (gy + ty) / 2.0f + by;
  const int N = 90;
  for (int pass = 0; pass < 2; pass++)
    for (int i = 0; i <= N; i++) {
      float t = i / (float)N, u = 1 - t;
      float x = u * u * gx + 2 * u * t * mx + t * t * tx, y = u * u * gy + 2 * u * t * my + t * t * ty;
      int w = t < 0.3f ? 6 : t < 0.75f ? 4 : 2;
      if (pass == 0) fsSq((int)x, (int)y, w + 4, 0x1B1B25);
      else fsSq((int)x, (int)y, w, t < 0.28f ? 0xD8A468 : t < 0.75f ? 0x5A3A22 : 0x8A5A30);
    }
  float t = 0.2f, u = 1 - t;  // the reel
  int rx = (int)(u * u * gx + 2 * u * t * mx + t * t * tx), ry = (int)(u * u * gy + 2 * u * t * my + t * t * ty) + 10;
  rx = SNAP2(rx), ry = SNAP2(ry);
  gfx->fillCircle(rx, ry, 8, sc565(0x1B1B25));
  gfx->fillCircle(rx, ry, 6, sc565(0xAAB0BC));
  gfx->fillCircle(rx, ry, 2, sc565(0x787E8C));
}
// the line, 2 px, from (x0, y0) to (x1, y1), sagging by sag at its middle (0: taut)
static void fsLine(int x0, int y0, int x1, int y1, int sag, Rgb col) {
  float mx = (x0 + x1) / 2.0f, my = (y0 + y1) / 2.0f + sag * 2;
  int n = (int)(hypotf(x1 - x0, y1 - y0) + abs(sag)) + 2;
  for (int i = 0; i <= n; i++) {
    float t = i / (float)n, u = 1 - t;
    fsSq((int)(u * u * x0 + 2 * u * t * mx + t * t * x1), (int)(u * u * y0 + 2 * u * t * my + t * t * y1), 2, col);
  }
}
// the line's colour by how hard it's pulled: white slack, yellow taut, orange, red about to snap
static Rgb fsLineCol(int pct) { return pct < 40 ? 0xF4F4F8 : pct < 70 ? 0xFFE27A : pct < 85 ? 0xF5A040 : 0xE8503A; }

// the float, red over white, its middle at (cx, cy); what's under the surface tinted by the water
static const char *const FS_BOBBER[10] = {
  "...k...",
  "...k...",
  "..kkk..",
  ".krrrk.",
  "krwrrrk",
  "krrrrrk",
  "kkkkkkk",
  "kwwwwwk",
  ".kwwwk.",
  "..kkk..",
};
static void fsFloat(int cx, int cy, bool lamp = false) {
  const ScKey k[3] = { { 'k', 0x1B1B25 }, { 'r', 0xE8503A }, { 'w', 0xFFFFFF } };
  int x0 = SNAP2(cx - 10), y0 = SNAP2(cy - 18);
  scSprite(FS_BOBBER, 10, x0, y0, 3, k, 3);
  if (lamp) {  // at night its tip is a little lamp
    twGlow(cx, y0 + 2, 26, 0xFFE27A, 150);
    scRect(x0 + 8, y0 - 4, x0 + 13, y0 + 1, 0xFFF3B0);
  }
  if (y0 + 30 > FS_SURF + 4) fsBlend(x0, FS_SURF + 4 > y0 ? FS_SURF + 4 : y0, x0 + 21, y0 + 29, fsNight ? FS_WATER_N[0] : FS_WATER[0], 110);
}
// the hook's line hanging from the float, the hook and its bait
static void fsHook(int cx, int top, int bot) {
  for (int y = top; y < bot; y += 2) fsSq(cx, y, 2, fsNight ? 0x8090A8 : 0xD8ECF8);
  scRect(SNAP2(cx) - 2, bot, SNAP2(cx) + 1, bot + 7, 0xAAB0BC);
  scRect(SNAP2(cx) - 6, bot + 6, SNAP2(cx) - 1, bot + 9, 0xAAB0BC);
  scRect(SNAP2(cx) - 8, bot + 2, SNAP2(cx) - 5, bot + 7, 0xAAB0BC);
  gfx->fillCircle(SNAP2(cx) + 2, bot + 4, 4, sc565(0xF08AA4));  // the bait
}
// a ring spreading on the surface round (cx, FS_SURF), seen from the side: a flat oval
static void fsRing(int cx, int rx, Rgb col, int w = 2) {
  int ry = rx / 5 + 2;
  for (int i = 0; i < 360; i += 2) {
    float a = i * 0.01745329f;
    fsSq(cx + (int)(rx * cosf(a)), FS_SURF + 2 + (int)(ry * sinf(a)), w, col);
  }
}
// drops thrown up from (cx, cy): light, no outline (the garden's rule for effects)
static void fsSplash(int cx, int cy, int spread, int n, uint32_t seed) {
  uint32_t r = seed;
  for (int i = 0; i < n; i++) {
    r = r * 1664525u + 1013904223u;
    int dx = (int)((r >> 8) % (2 * spread + 1)) - spread;
    r = r * 1664525u + 1013904223u;
    int h = (int)((r >> 8) % (spread + 1));
    int y = cy - h + abs(dx) / 3;
    scRect(SNAP2(cx + dx), SNAP2(y), SNAP2(cx + dx) + 3, SNAP2(y) + 3, i % 3 ? 0xFFFFFF : 0xBFE6F5);
  }
}
// a circle of 2 px dots (a ring round something), w thick
static void fsCircle(int cx, int cy, int r, Rgb col, int w = 2) {
  int n = (int)(6.2832f * r / 2) + 8;
  for (int i = 0; i < n; i++) {
    float a = i * 6.2832f / n;
    fsSq(cx + (int)(r * cosf(a)), cy + (int)(r * sinf(a)), w, col);
  }
}
// the "!" over the float: a white bubble with a red mark, as a Pokemon trainer's when they spot you
static void fsBang(int cx, int cy) {
  const uint16_t ink = sc565(0x1B1B25);
  gfx->fillTriangle(cx - 10, cy + 14, cx + 6, cy + 16, cx - 14, cy + 36, ink);
  gfx->fillCircle(cx, cy, 25, ink);
  gfx->fillCircle(cx, cy, 21, UI_WHITE);
  gfx->fillTriangle(cx - 8, cy + 14, cx + 3, cy + 15, cx - 11, cy + 30, UI_WHITE);
  scRect(cx - 4, cy - 15, cx + 3, cy + 5, 0xE8503A);
  scRect(cx - 4, cy + 9, cx + 3, cy + 16, 0xE8503A);
}
// the screen's edge glowing: the 4 px cells within w of the rim pulled toward col, stronger toward the edge
static void fsRimGlow(Rgb col, int a256, int w) {
  for (int y = 0; y < V2_W; y += 4)
    for (int x = 0; x < V2_W; x += 4) {
      float d = 233 - hypotf(x + 2 - CX, y + 2 - CY);
      if (d < 0 || d > w) continue;
      if (d > w * 0.5f && (scFl4(x) + scFl4(y)) % 2) continue;
      fsBlend(x, y, x + 3, y + 3, col, (int)(a256 * (1 - d / w)));
    }
}
// how hard the line is pulled: an arc on the right rim (where the need bars sit at home), filling from the bottom,
// green to yellow to red; the top fifth is the danger, red even when empty
static void fsTension(int pct) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  const float a0 = 50, a1 = -50;  // the bottom end, the top end (degrees, 0 = right)
  Rgb fill = pct < 50 ? 0x58B868 : pct < 80 ? 0xF8C030 : 0xE8503A;
  for (int y = 0; y < V2_W; y += 2)
    for (int x = CX; x < V2_W; x += 2) {
      float dx = x + 1 - CX, dy = y + 1 - CY, r = hypotf(dx, dy), a = atan2f(dy, dx) * 57.29578f;
      if (r < V2_BAR_RIN - 3 || r > V2_BAR_ROUT + 3 || a > a0 + 2 || a < a1 - 2) continue;
      Rgb c;
      if (r < V2_BAR_RIN || r > V2_BAR_ROUT || a > a0 || a < a1) c = 0x1B1B25;
      else {
        float f = (a0 - a) / (a0 - a1) * 100;
        c = f <= pct ? fill : f >= 80 ? 0x7A3A3A : 0x3A4A5A;
        if (f <= pct && r > V2_BAR_ROUT - 4) c = scMix(c, 0xFFFFFF, .35);
      }
      fsBlend(x, y, x + 1, y + 1, c, 256);
    }
  v2BarLabel(fb, "LINE", 0, UI_WHITE);
}

// the casts so far, along the top: a Pokemon where one was caught, a red X where it got away, a yellow edge on this
// one, a faint Poke Ball on those to come. st: 0 to come, 1 this one, -1 got away, else the dex caught
#define FS_SLOT 40
static void fsSlots(const int16_t *st) {
  int x0 = CX - (5 * FS_SLOT + 4 * 6) / 2, y = 40;
  for (int i = 0; i < 5; i++) {
    int x = x0 + i * (FS_SLOT + 6);
    uint16_t edge = st[i] == 1 ? sc565(0xF8C030) : sc565(0x3A4A5A);
    gfx->fillRoundRect(x - 3, y - 3, FS_SLOT + 6, FS_SLOT + 6, 9, sc565(0x1B1B25));
    gfx->fillRoundRect(x - 1, y - 1, FS_SLOT + 2, FS_SLOT + 2, 8, edge);
    gfx->fillRoundRect(x + 2, y + 2, FS_SLOT - 4, FS_SLOT - 4, 6, st[i] == -1 ? sc565(0xC8CCD8) : UI_WHITE);
    if (st[i] == 0 || st[i] == 1) {
      drawMap(SPR_HOME_BALL, HOME_ICON_N, x + 11, y + 11, 1, false);
      if (st[i] == 0) fsBlend(x + 4, y + 4, x + FS_SLOT - 5, y + FS_SLOT - 5, 0xFFFFFF, 150);
    } else if (st[i] == -1) {
      for (int k = 0; k < 14; k += 2) {
        fsSq(x + 13 + k, y + 13 + k, 4, 0xE8503A);
        fsSq(x + 27 - k, y + 13 + k, 4, 0xE8503A);
      }
    } else {
      const uint8_t *t = thumbs.getForm(st[i], false);
      int h = fsThumbH(t, 1);
      fsClip[0] = x + 2, fsClip[1] = y + 2, fsClip[2] = x + FS_SLOT - 2, fsClip[3] = y + FS_SLOT - 2;
      fsThumb(t, x + FS_SLOT / 2, y + FS_SLOT / 2 + (h < FS_SLOT - 4 ? h / 2 : FS_SLOT / 2 - 3), 1);
      fsClip[0] = fsClip[1] = 0, fsClip[2] = fsClip[3] = V2_W;
    }
  }
}

// your Pokemon at the pier's end and its rod; tip: where the rod's tip is, (bx, by) how far it's bent
static void fsAngler(int tx, int ty, int bx = 0, int by = 0) {
  fsThumb(thumbs.getForm(pet.speciesId, false), FS_PET_X, FS_DECK + 2, 3);
  fsRod(FS_PET_X + 26, FS_DECK - 30, tx, ty, bx, by);
}
#define FS_TIP_X 252  // the rod's tip, at rest
#define FS_TIP_Y 88

// a word over the water with a dark edge (twRimText), at size 2 or bigger
static void fsSay(const char *t, int cx, int y, Rgb col, int size = 2) {
  setSize(size);
  int x = cx - textW(t, size) / 2;
  gfx->setTextColor(sc565(0x1B1B25));
  for (int k = 0; k < 8; k++) {
    static const int8_t D[8][2] = { { -2, 0 }, { 2, 0 }, { 0, -2 }, { 0, 2 }, { -2, -2 }, { 2, -2 }, { -2, 2 }, { 2, 2 } };
    setCur(x + D[k][0] * size / 2, y + D[k][1] * size / 2);
    printT(t);
  }
  gfx->setTextColor(sc565(col));
  setCur(x, y);
  printT(t);
}
// bubbles rising from (cx, cy)
static void fsBubbles(int cx, int cy, int n) {
  for (int i = 0; i < n; i++) {
    int x = cx + ((i * 7) % 3 - 1) * 6, y = cy - i * 14, r = 2 + (i % 3);
    fsCircle(x, y, r + 1, fsNight ? 0x6A8CB8 : 0xD8F0FF, 2);
  }
}
// rays from (cx, cy) over the whole screen: every other one of n wedges pulled toward col, fading with distance
static void fsRays(int cx, int cy, int n, Rgb col, int a256) {
  const float wedge = 6.2831853f / n;
  for (int y = 0; y < V2_W; y += 4)
    for (int x = 0; x < V2_W; x += 4) {
      float dx = x + 2 - cx, dy = y + 2 - cy;
      if ((int)floorf((atan2f(dy, dx) + 3.1415927f) / wedge) % 2) continue;
      int a = (int)(a256 * (1 - hypotf(dx, dy) / 260));
      if (a > 0) fsBlend(x, y, x + 3, y + 3, col, a);
    }
}
// a sign with its wood in two given colours (v2SignIn), its text in caps
static void fsBoard(const char *s, int y, uint16_t wood = C565(0x8a, 0x5a, 0x30), uint16_t lit = C565(0xb0, 0x7a, 0x45)) {
  char b[40];
  v2SignIn(v2Caps(s, b, sizeof(b)), y, wood, lit);
}
// a chip with its points: "+3"
static void fsPoints(const char *s, int cx, int y, Rgb col) {
  setSize(3);
  int w = textW(s, 3) + 20;
  gfx->fillRoundRect(cx - w / 2 - 3, y - 3, w + 6, 36, 10, sc565(0x1B1B25));
  gfx->fillRoundRect(cx - w / 2, y, w, 30, 8, sc565(col));
  gfx->setTextColor(UI_WHITE);
  setCur(cx - textW(s, 3) / 2, y + 4);
  printT(s);
}

// ===================================================================== the mock screens
static const int16_t FS_ST_1[5] = { 1, 0, 0, 0, 0 };
static const int16_t FS_ST_2[5] = { 129, 1, 0, 0, 0 };
static const int16_t FS_ST_3[5] = { 129, 118, 1, 0, 0 };
static const int16_t FS_ST_4[5] = { 129, 118, -1, 1, 0 };
static const int16_t FS_ST_5[5] = { 129, 118, -1, 120, 1 };
static const int16_t FS_ST_END[5] = { 129, 118, -1, 120, 130 };

// 1: the Games menu with a sixth cartridge, PokeFish, in Magikarp's orange: the sea, a float, a Magikarp leaping
static uint16_t fsLbl[V2_LBL_W * V2_LBL_H];
static void fsMockGames() {
  v2PaintGameLabels();
  v2LblSky(0x7FBDE6, 0xD8EEE8, 22, 0x4F93C4, 0x3A78A8);
  for (int k = 0; k < 4; k++) gfx->fillRect(6 + k * 20, 26 + (k % 2) * 8, 10, 2, sc565(0x9FD6F0));
  gfx->fillRect(14, 30, 4, 3, sc565(0xE8503A));  // the float
  gfx->fillRect(14, 33, 4, 2, UI_WHITE);
  for (int x = 16; x < 40; x += 2) gfx->fillRect(x, 6 + (x - 16) / 2, 1, 1, UI_WHITE);  // the line
  v2LblThumb(129, 54, 30, false);
  for (int k = 0; k < 4; k++) gfx->fillRect(40 + k * 6, 34 - (k % 2) * 3, 2, 2, UI_WHITE);  // drops
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  for (int j = 0; j < V2_LBL_H; j++) memcpy(fsLbl + j * V2_LBL_W, fb + j * V2_W, V2_LBL_W * sizeof(uint16_t));
  v2GbBack(T(S_GAMES));
  static const int16_t XY[6][2] = { { 56, 96 }, { 180, 96 }, { 304, 96 }, { 56, 238 }, { 180, 238 }, { 304, 238 } };
  for (int k = 0; k < 5; k++) v2Cart(XY[k][0], XY[k][1], v2Lbl(k), gameLabel(V2_CART_GAME[k]), V2_CART_COL[k]);
  v2Cart(XY[5][0], XY[5][1], fsLbl, "POKEFISH", 0xF07A3A);
  v2CloseX();
}

// the water as you'll fish it, before anything's on it
static void fsBack() { fsScene(); }

// 2: its how-to card over the pier
static void fsMockHow() {
  fsBack();
  fsAngler(FS_TIP_X, FS_TIP_Y);
  fsLine(FS_TIP_X, FS_TIP_Y, FS_TIP_X + 4, FS_TIP_Y + 60, 0, 0xF4F4F8);
  v2IntroCard(CX, CY, true, "POKEFISH",
              "Tap the water to cast. Wait for the float to go under, then tap! Hold to reel in, and let go when the "
              "line turns red. 5 casts.");
}

// 3: casting: tap the water and the float flies there; a ring marks the spot. Shadows under the water, a big one
// far out where it's deep, and bubbles where a fish is waiting
static void fsMockCast() {
  fsBack();
  fsShadow(212, 262, 3, -1);
  fsShadow(296, 330, 4, 1);
  fsShadow(404, 318, 6, -1, 130);
  fsBubbles(410, FS_SURF + 52, 4);
  scRect(402, FS_SURF - 8, 405, FS_SURF - 5, 0xFFFFFF);  // glints where the bubbles break: a fish is there
  scRect(418, FS_SURF - 12, 421, FS_SURF - 9, 0xFFFFFF);
  scRect(410, FS_SURF - 16, 413, FS_SURF - 13, 0xFFFFFF);
  int tx = 316;  // where you tapped
  fsRing(tx, 30, 0xFFFFFF, 2);
  fsRing(tx, 18, 0xFFFFFF, 2);
  for (int k = 0; k < 3; k++) scRect(tx - 2, FS_SURF - 44 + k * 8, tx + 1, FS_SURF - 40 + k * 8, 0xFFFFFF);  // an arrow down
  scRect(tx - 6, FS_SURF - 20, tx + 5, FS_SURF - 17, 0xFFFFFF);
  scRect(tx - 2, FS_SURF - 16, tx + 1, FS_SURF - 13, 0xFFFFFF);
  fsAngler(FS_TIP_X + 36, FS_TIP_Y + 44, 10, -10);  // swung forward
  // the float on its way, the line trailing it, its path dotted ahead
  int fx = 290, fy = 122;
  fsLine(FS_TIP_X + 36, FS_TIP_Y + 44, fx, fy - 14, -6, 0xF4F4F8);
  for (int i = 1; i < 6; i++) {
    float t = i / 6.0f;
    int x = fx + (int)((tx - fx) * t), y = fy + (int)((FS_SURF - 10 - fy) * t * t) - (int)(18 * t * (1 - t));
    fsSq(x, y, 4, 0xFFFFFF);
  }
  fsFloat(fx, fy);
  fsSlots(FS_ST_1);
  fsSay("TAP THE WATER", CX, 396, 0xFFFFFF);
}

// 4: waiting: the float bobs on its ring, a fish comes round toward the hook
static void fsMockWait() {
  fsBack();
  fsShadow(380, 286, 4, -1);
  fsShadow(170, 300, 3, 1, 110);
  fsAngler(FS_TIP_X, FS_TIP_Y);
  fsLine(FS_TIP_X, FS_TIP_Y, FS_FLOAT_X, FS_SURF - 18, 14, 0xF4F4F8);
  fsHook(FS_FLOAT_X, FS_SURF + 12, FS_SURF + 52);
  fsRing(FS_FLOAT_X, 22, fsNight ? 0x6A8CB8 : 0xE6F6FF, 2);
  fsFloat(FS_FLOAT_X, FS_SURF, fsNight);
  fsSlots(FS_ST_2);
}

// 5: a nibble (a fake-out): the float dips a little, one small ring, the fish nosing the bait. Not yet!
static void fsMockNibble() {
  fsBack();
  fsShadow(FS_FLOAT_X + 30, FS_SURF + 66, 4, -1);
  fsAngler(FS_TIP_X, FS_TIP_Y);
  fsLine(FS_TIP_X, FS_TIP_Y, FS_FLOAT_X, FS_SURF - 12, 10, 0xF4F4F8);
  fsHook(FS_FLOAT_X, FS_SURF + 18, FS_SURF + 56);
  fsRing(FS_FLOAT_X, 16, 0xFFFFFF, 2);
  fsFloat(FS_FLOAT_X, FS_SURF + 6, fsNight);
  for (int k = 0; k < 2; k++) {  // two little ticks either side: it twitched
    scRect(FS_FLOAT_X - 24, FS_SURF - 22 + k * 8, FS_FLOAT_X - 19, FS_SURF - 19 + k * 8, 0xFFFFFF);
    scRect(FS_FLOAT_X + 20, FS_SURF - 22 + k * 8, FS_FLOAT_X + 25, FS_SURF - 19 + k * 8, 0xFFFFFF);
  }
  fsSlots(FS_ST_2);
}

// 6: the bite. Everything at once, so it can't be missed on a screen with no sound: the float pulled right under,
// a splash, two rings, the "!", the rim lit, the line snapping straight and the rod dipping; the white ring closing in
// on the float is the time left to strike
static void fsMockBite() {
  fsBack();
  fsRimGlow(fsNight ? 0xFFE27A : 0xFFF3B0, 220, 34);
  fsShadow(FS_FLOAT_X + 8, FS_SURF + 70, 4, -1, 190);
  fsAngler(FS_TIP_X + 6, FS_TIP_Y + 18, 8, 10);
  fsLine(FS_TIP_X + 6, FS_TIP_Y + 18, FS_FLOAT_X, FS_SURF + 30, 0, 0xFFE27A);
  fsFloat(FS_FLOAT_X, FS_SURF + 34, fsNight);  // under (at night its lamp still glows down there)
  fsRing(FS_FLOAT_X, 46, 0xFFFFFF, 4);
  fsRing(FS_FLOAT_X, 28, 0xFFFFFF, 2);
  fsSplash(FS_FLOAT_X, FS_SURF - 6, 30, 22, 7);
  fsCircle(FS_FLOAT_X, FS_SURF + 6, 58, 0x1B1B25, 6);  // the strike window, closing
  fsCircle(FS_FLOAT_X, FS_SURF + 6, 58, 0xFFFFFF, 2);
  fsBang(FS_FLOAT_X - 4, FS_SURF - 92);
  fsSlots(FS_ST_3);
  fsSay("TAP!", CX - 6, 396, 0xFFE27A, 4);
}

// a hooked fish under the water, fighting at (cx, cy): its shadow, the float dragged down by it, bubbles, splashes over
static void fsHooked(int cx, int cy, int s, int thrash) {
  fsShadow(cx, cy, s, 1, 200);
  fsFloat(cx - 7 * s - 14, cy - 6);
  fsBubbles(cx - 4 * s, cy - 18, 3);
  if (thrash) {
    fsSplash(cx, FS_SURF - 4, 20 + 10 * thrash, 10 + 12 * thrash, 11 + thrash);
    fsRing(cx, 26 + 12 * thrash, 0xFFFFFF, 2);
    for (int k = 0; k < 3; k++) {  // motion marks either side of it
      scRect(cx + 7 * s + 8, cy - 10 + k * 10, cx + 7 * s + 19 + thrash * 4, cy - 7 + k * 10, 0xFFFFFF);
      scRect(cx - 7 * s - 20 - thrash * 4, cy - 10 + k * 10, cx - 7 * s - 9, cy - 7 + k * 10, 0xFFFFFF);
    }
  }
}

// 7: reeling it in: hold anywhere (or the button). The line is taut and yellow, the rod bends, the arc on the rim
// shows how hard it's pulling
static void fsMockReel() {
  fsBack();
  int fx = 314, fy = 300;
  fsAngler(FS_TIP_X + 4, FS_TIP_Y + 30, 22, 16);
  fsLine(FS_TIP_X + 4, FS_TIP_Y + 30, fx - 30, fy - 4, 0, fsLineCol(55));
  fsHooked(fx, fy, 4, 1);
  fsTension(55);
  fsSlots(FS_ST_4);
  fsSay("HOLD TO REEL", CX - 10, 396, 0xFFFFFF);
}

// 8: about to snap: it's thrashing and you're still reeling. The line red, the rod bent double, the arc in the red,
// the rim glowing red (it pulses), LET GO!
static void fsMockStrain() {
  fsBack();
  fsRimGlow(0xE8503A, 230, 40);
  int fx = 296, fy = 290;
  fsAngler(FS_TIP_X - 6, FS_TIP_Y + 56, 36, 30);
  fsLine(FS_TIP_X - 6, FS_TIP_Y + 56, fx - 30, fy - 4, 0, fsLineCol(92));
  for (int k = 0; k < 3; k++) {  // sparks off the rod's tip
    static const int8_t SP[3][2] = { { -14, -12 }, { 10, -16 }, { 16, 2 } };
    fsSq(FS_TIP_X - 6 + SP[k][0], FS_TIP_Y + 56 + SP[k][1], 4, 0xFFE27A);
  }
  fsHooked(fx, fy, 4, 2);
  fsTension(92);
  fsSlots(FS_ST_4);
  fsSay("LET GO!", CX - 10, 386, 0xFF8A7A, 4);
}

// 9: caught! It leaps out onto the pier's side of the screen, drops flying, and its name goes up on the board
static void fsMockCaught() {
  fsBack();
  const uint8_t *t = thumbs.getForm(120, false);  // Staryu
  int cx = 316, feet = 196;
  twGlow(cx, feet - 34, 70, 0xFFF3B0, 170);
  fsAngler(236, 142, -6, -10);
  fsLine(236, 142, cx - 14, feet - 30, 4, 0xF4F4F8);
  fsThumb(t, cx, feet, 3);
  fsSplash(cx, FS_SURF + 4, 40, 26, 21);
  fsRing(cx, 50, 0xFFFFFF, 2);
  for (int i = 0; i < 4; i++) {  // sparkles
    static const int8_t SP[4][2] = { { -62, -70 }, { 58, -86 }, { 66, -24 }, { -50, -10 } };
    int gx = cx + SP[i][0], gy = feet - 40 + SP[i][1];
    scGridRect(gx, gy - 8, gx + 4, gy + 12, 0xFFFFFF);
    scGridRect(gx - 8, gy, gx + 12, gy + 4, 0xFFFFFF);
  }
  fsBoard("STARYU!", 44);  // (the casts come back along the top once it's landed)
  fsPoints("+1", CX, 392, 0x58B868);
}

// 10: got away: the line snapped and hangs from the rod, the shadow shooting off with a trail of bubbles
static void fsMockAway() {
  fsBack();
  fsShadow(414, 318, 4, 1, 120);
  for (int k = 0; k < 4; k++) scRect(340 - k * 26, 314 + (k % 2) * 6, 351 - k * 26, 317 + (k % 2) * 6, 0xD8F0FF);  // its wake
  fsBubbles(330, 300, 3);
  fsAngler(236, 140, -6, -10);
  int x = 236, y = 140;  // the broken line hanging from the tip, its end curled
  for (int i = 0; i < 40; i++) fsSq(x + (int)(8 * sinf(i * 0.2f)) + i / 4, y + i * 2, 4, i % 2 ? 0x1B1B25 : 0xF4F4F8);
  for (int i = 0; i < 40; i++) fsSq(x + (int)(8 * sinf(i * 0.2f)) + i / 4, y + i * 2, 2, 0xFFFFFF);
  fsRing(FS_FLOAT_X, 34, 0xFFFFFF, 2);
  fsSplash(FS_FLOAT_X, FS_SURF - 4, 16, 8, 31);
  fsBoard("IT GOT AWAY!", 44, C565(0x8a, 0x2a, 0x30), C565(0xb8, 0x40, 0x44));
  fsSay("SNAP!", 300, 146, 0xFF8A7A, 3);
}

// 11: the big one: Gyarados, the rare one from the deep. Gold rays and a gold board
static void fsMockRare() {
  fsBack();
  const uint8_t *t = thumbs.getForm(130, false);
  int cx = 316, feet = 206;
  fsRays(cx, feet - 60, 18, 0xFFE27A, 110);
  twGlow(cx, feet - 60, 110, 0xFFF3B0, 190);
  fsAngler(228, 150, 6, 14);
  fsLine(228, 150, cx - 24, feet - 70, 0, 0xFFE27A);
  fsThumb(t, cx, feet, 3);
  fsSplash(cx, FS_SURF + 6, 60, 40, 41);
  fsRing(cx, 70, 0xFFFFFF, 4);
  v2SignIn("GYARADOS!", 44, C565(0xc0, 0x84, 0x10), C565(0xf0, 0xc0, 0x4a));
  fsPoints("+10", CX - 46, 392, 0xF8A020);
  setSize(2);  // and it's new to your Pokedex
  int w = textW("NEW!", 2) + 16, nx = CX + 50;
  gfx->fillRoundRect(nx - 3, 395, w + 6, 30, 9, sc565(0x1B1B25));
  gfx->fillRoundRect(nx, 398, w, 24, 7, sc565(0xE8503A));
  gfx->setTextColor(UI_WHITE);
  setCur(nx + 8, 403);
  printT("NEW!");
}

// 12: the end: v2GameResult's panel with the score and the record, and the five casts in a row under the score
static void fsMockResult() {
  fsBack();
  fsAngler(FS_TIP_X, FS_TIP_Y);
  v2DimFrame(115);
  fsBoard("POKEFISH", 70);
  v2Panel(96, 112, 370, V2_AGAIN_Y + V2_AGAIN_H + 16, 12);
  int y = 124;
  setSize(4);
  gfx->setTextColor(UI_INK);
  setCur(centerX("13", 4), y);  // Magikarp 1 + Goldeen 1 + Staryu 1 + Gyarados 10
  printT("13");
  y += 40;
  int x0 = CX - (5 * 40 + 4 * 6) / 2;
  for (int i = 0; i < 5; i++) {  // the five casts
    int x = x0 + i * 46, d = FS_ST_END[i];
    gfx->fillRoundRect(x, y, 40, 40, 8, sc565(d < 0 ? 0xE4E6EC : 0xEAF6FB));
    if (d < 0) {
      for (int k = 0; k < 14; k += 2) {
        fsSq(x + 13 + k, y + 13 + k, 4, 0xE8503A);
        fsSq(x + 27 - k, y + 13 + k, 4, 0xE8503A);
      }
      continue;
    }
    const uint8_t *t = thumbs.getForm(d, false);
    int h = fsThumbH(t, 1);
    fsClip[0] = x + 2, fsClip[1] = y + 2, fsClip[2] = x + 38, fsClip[3] = y + 38;
    fsThumb(t, x + 20, y + 20 + (h < 36 ? h / 2 : 17), 1);
    fsClip[0] = fsClip[1] = 0, fsClip[2] = fsClip[3] = V2_W;
    char p[4];
    snprintf(p, sizeof(p), "%d", d == 130 ? 10 : 1);
    setSize(1);
    gfx->setTextColor(sc565(d == 130 ? 0xC08410 : 0x3C8A4C));
    setCur(x + 40 - textW(p, 1) - 3, y + 31);
    printT(p);
  }
  y += 48;
  setSize(2);
  gfx->setTextColor(C565(0xc0, 0x84, 0x10));
  setCur(centerX("NEW RECORD!", 2), y);
  printT("NEW RECORD!");
  y += 22;
  gfx->setTextColor(C565(0x3c, 0x8a, 0x4c));
  setCur(centerX(T(S_GREAT_JOY), 2), y);
  printT(T(S_GREAT_JOY));
  v2AgainButton(V2_AGAIN_Y);
}

// 13: the bite at night: the same moment in the dark. The float's lamp, the rings and the "!" carry it
static void fsMockNight() {
  fsNight = true;
  fsMockBite();
  fsNight = false;
}

static void fishMockRender() {
  switch (fsMock) {
    case 1: fsMockGames(); break;
    case 2: fsMockHow(); break;
    case 3: fsMockCast(); break;
    case 4: fsMockWait(); break;
    case 5: fsMockNibble(); break;
    case 6: fsMockBite(); break;
    case 7: fsMockReel(); break;
    case 8: fsMockStrain(); break;
    case 9: fsMockCaught(); break;
    case 10: fsMockAway(); break;
    case 11: fsMockRare(); break;
    case 12: fsMockResult(); break;
    case 13: fsMockNight(); break;
  }
}
