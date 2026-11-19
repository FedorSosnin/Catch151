// V2: Reel Fishing: the game's drawing (fsGame, from fishing.h's state) and its mockups (FISHMOCK n). A fishing minigame for the Games menu, played off the end of a pier at the
// Beach. The Knomi has no speaker and no vibration, so every moment of a cast reads on the screen alone (the float,
// rings, splashes, a "!", the rim's glow, the line's colour, the rod's bend); the computers and the web add sound, and
// phones and controllers a buzz, on top. These painters only draw still screens for the console's "FISHMOCK n"
// (1..19), so the look can be judged in the real game before any of it is built. scene_v2.h includes this after
// tower_v2.h. The plan is docs/FISHING.md.

static uint8_t fsMock = 0;  // the mockup on screen (0: none)
void fishMockSet(uint8_t n) { fsMock = n; }

// ===================================================================== the scene
// Seen from the side, cut away at the waterline: the sky down to the surface, the sea under it getting deeper to the
// right, the sand sloping away under the pier. The rod comes in from the screen's left edge, held by you just out of
// sight (Fedor: no Pokemon on the pier, since a Pidgey or a Magikarp can't hold a rod, and this way it's you fishing);
// the float rides the surface on the right, its hook hanging under it where the fish come.
#define FS_SURF 208      // the water's surface
#define FS_DECK 184      // the pier's deck
#define FS_PIER_X1 168   // the pier's end
#define FS_FLOAT_X 304   // where the float sits in most of the mockups
static const Rgb FS_WATER[4] = { 0x62B6E2, 0x4C9CD6, 0x3B82C6, 0x2D66AE };
static const Rgb FS_WATER_N[4] = { 0x2C5482, 0x254A76, 0x1E3F68, 0x18345A };  // at night (light enough to see the fish in)
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
  scSun(p, night, 360, 112);  // inside the LINE arc's ring: at 388, 92 its label sat on the moon (Fedor, 2026-11-18)
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
// shadow of it (a256)
static int16_t fsClip[4] = { 0, 0, V2_W, V2_W };  // what fsThumb may draw in (x0, y0, x1, y1): a slot keeps a big one in
static int16_t fsClipC[3] = { 0, 0, 0 };          // and within this circle (x, y, r; r 0: none)
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
      if (fsClipC[2]) {
        int ex = 2 * px + s - 2 * fsClipC[0], ey = 2 * py + s - 2 * fsClipC[1];  // (twice its middle's offset)
        if (ex * ex + ey * ey > 4 * fsClipC[2] * fsClipC[2]) continue;
      }
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

// something under the water: a dark blob, no shape to give it away (Fedor: "just blobs"), s its size (3 small,
// 4 middling, 6 big); its edge on a checker, soft
static void fsBlob(int cx, int cy, int s, int a256 = 150) {
  Rgb col = fsNight ? 0x050A18 : 0x0E2A5C;
  float rx = 5.0f * s, ry = 3.0f * s;
  for (int y = scFl4(cy - (int)ry) * 4; y < cy + ry; y += 4)
    for (int x = scFl4(cx - (int)rx) * 4; x < cx + rx; x += 4) {
      float dx = (x + 2 - cx) / rx, dy = (y + 2 - cy) / ry, d = dx * dx + dy * dy;
      if (d > 1 || (d > 0.6f && (scFl4(x) + scFl4(y)) % 2)) continue;
      fsBlend(x, y, x + 3, y + 3, col, a256);
    }
}

static int fsReelX, fsReelY;  // where the reel was drawn
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
  fsReelX = rx, fsReelY = ry;
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
// the line's colour by how hard it's pulled: pale when it's going slack (too slow), white, yellow as it tightens,
// red about to snap (too fast)
#define FS_SLACK 20  // the LINE arc's zones: under this, too slow; over FS_SNAP, too fast
#define FS_SNAP 80
static Rgb fsLineCol(int pct) { return pct < FS_SLACK ? 0xA8BCD4 : pct < 62 ? 0xF4F4F8 : pct < FS_SNAP ? 0xFFE27A : 0xE8503A; }

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
// The screen's edge glowing (gold at the bite, red with the line too tight, blue too slack): col laid over the scene,
// a256 of the way at the rim and fading to nothing w px in. It was 4 px cells, a checker over its outer half and a
// straight fade that stopped in an edge, which looked coarse in a big window (Fedor, 2026-11-15: "look low quality
// when large on the mac"). Now each pixel fades on its own (a Knomi pixel on the Knomi: one blend for its four, fewer
// than the cells cost), easing out so the glow has no inner edge. On a computer and the web the colour is also
// dithered (4 x 4 ordered) as it's rounded to the screen's 16 bits, so a large window shows a smooth fade, not bands.
// Each pixel's distance from the rim, in quarter px, is worked out once.
#define FS_RIM_MAX 40  // the widest glow
struct FsRimPx { uint32_t i; uint8_t d4; };  // where in the frame, and how far in from the rim (quarter px)
static FsRimPx *fsRim = nullptr;
static int fsRimN = 0;
#ifdef BOARD_HOST
#define FS_RIM_STEP 1
#else
#define FS_RIM_STEP 2  // the Knomi's pixels
#endif
static void fsRimBuild() {
  if (fsRim) return;
  for (int pass = 0, n = 0; pass < 2; pass++) {
    for (int y = 0; y < V2_W; y += FS_RIM_STEP)
      for (int x = 0; x < V2_W; x += FS_RIM_STEP) {
        float d = 233 - hypotf(x + FS_RIM_STEP * 0.5f - CX, y + FS_RIM_STEP * 0.5f - CY);
        if (d < 0 || d > FS_RIM_MAX) continue;
        if (pass) fsRim[n] = { (uint32_t)(y * V2_W + x), (uint8_t)(d * 4) };
        n++;
      }
    if (!pass) {
      fsRim = (FsRimPx *)ps_malloc((size_t)n * sizeof(FsRimPx));
      if (!fsRim) return;
      fsRimN = n;
      n = 0;
    }
  }
}
static void fsRimGlow(Rgb col, int a256, int w) {
  fsRimBuild();
  if (!fsRim || w <= 0) return;
  if (w > FS_RIM_MAX) w = FS_RIM_MAX;
  const int w4 = w * 4;
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
#ifdef BOARD_HOST
  static const uint8_t BAYER[4][4] = { { 0, 8, 2, 10 }, { 12, 4, 14, 6 }, { 3, 11, 1, 9 }, { 15, 7, 13, 5 } };
  const int cr = (col >> 16) & 255, cg = (col >> 8) & 255, cb = col & 255;
#else
  const uint16_t c = sc565(col);
#endif
  uint8_t fade[FS_RIM_MAX * 4];  // how much at each quarter px in: a256 at the rim, eased out (smoothstep) to 0 at w
  for (int d = 0; d < w4; d++) {
    int t = 255 - d * 255 / w4;
    int f = a256 * (t * t / 255 * (765 - 2 * t) / 255) / 255;
    fade[d] = (uint8_t)(f > 255 ? 255 : f);
  }
  for (int k = 0; k < fsRimN; k++) {
    const FsRimPx &p = fsRim[k];
    if (p.d4 >= w4) continue;
    int a = fade[p.d4];
    if (!a) continue;
    uint16_t *q = fb + p.i;
#ifdef BOARD_HOST
    uint16_t v = *q;
    int r = (v >> 11) & 31, g = (v >> 5) & 63, b = v & 31;
    r = (r << 3) | (r >> 2), g = (g << 2) | (g >> 4), b = (b << 3) | (b >> 2);
    r += (cr - r) * a >> 8, g += (cg - g) * a >> 8, b += (cb - b) * a >> 8;
    int x = (int)(p.i % V2_W), y = (int)(p.i / V2_W), dz = BAYER[y & 3][x & 3];
    r = (r + (dz >> 1)) >> 3, g = (g + (dz >> 2)) >> 2, b = (b + (dz >> 1)) >> 3;
    *q = (uint16_t)((r > 31 ? 31 : r) << 11 | (g > 63 ? 63 : g) << 5 | (b > 31 ? 31 : b));
#else
    uint16_t v = v2Blend(q[0], c, a);
    q[0] = q[1] = q[V2_W] = q[V2_W + 1] = v;
#endif
  }
}
// how tight the line is: an arc on the right rim (where the need bars sit at home) in three zones, blue at the
// bottom (too slow: it'll slip off), green in the middle (keep it there), red at the top (too fast: it'll snap). It
// fills from the bottom, each zone in its own colour, and a white notch marks where it is now. The mockups draw the
// game's own (fsArcDraw, below)
static void fsArcDraw(float pct, int lo, int hi, bool beat);
static void fsLineArc(int pct, int lo = FS_SLACK, int hi = FS_SNAP) { fsArcDraw(pct, lo, hi, false); }

// what each one in the mockups weighed, in tenths of a kg (the game would weigh each fish: its kind's Pokedex weight,
// bigger or smaller by its blob's size)
static int fsKg10(int16_t dex) { return dex == 129 ? 112 : dex == 118 ? 146 : dex == 120 ? 361 : dex == 130 ? 2418 : 0; }
static void fsKgText(char *b, int n, int kg10) {  // shown in pounds, as the game shows it (fishLb10())
  const int lb10 = (int)fishLb10(kg10);
  snprintf(b, n, "%d.%d lb", lb10 / 10, lb10 % 10);
}

#ifdef BOARD_HOST
// how much of a pixel a shape covers, from how far inside its edge the pixel's middle is (px; < 0 outside)
static inline int fsCover(float in) { return in >= 0.5f ? 256 : in <= -0.5f ? 0 : (int)((in + 0.5f) * 256); }
#endif
// one cast's place along the top (Fedor: round, "the same dark transparent color" as the home screen's icon band, no
// white, "No need for numbers ... Just the spaces and pokemon when you catch them"): a see-through holder in the band's
// ink (v2BandInk(): dark by day, white at night), its 3 px edge fainter, and in it the Pokemon once one's landed; this
// cast's edge is yellow. One that got away leaves its place empty. On the Knomi on its 2 px grid, a Knomi pixel a cell;
// on a computer and the web a pixel at a time with soft edges, as the band is
// st: 0 to come, 1 this one, -1 got away, else the dex landed
#define FS_SLOT 40
#define FS_SLOT_R 19     // the holder's radius, its edge 3 px more
// a round see-through holder in the band's ink, R px, its 3 px edge fainter or (ring) yellow
static void fsDisc(int cx, int cy, int R, bool ringed) {
  const uint16_t ink = sc565(fsNight ? 0xFFFFFF : 0x1B1B25), ring = sc565(0xF8C030);
  const int ra = fsNight ? 26 : V2_RIM_A, ta = fsNight ? 31 : V2_TRACK_A, E = R + 3;
  const int st = ringed ? 1 : 0;
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
#ifdef BOARD_HOST
  for (int y = cy - E - 1; y <= cy + E + 1; y++)
    for (int x = cx - E - 1; x <= cx + E + 1; x++) {
      float d = hypotf(x + 0.5f - cx, y + 0.5f - cy);
      int ce = fsCover(E - d), ci = fsCover(R - d);
      if (!ce) continue;
      uint16_t &p = fb[y * V2_W + x];
      if (st == 1) p = v2Blend(p, ring, ce - ci);  // this cast: its edge yellow
      else p = v2Blend(p, ink, ra * (ce - ci) >> 8);
      if (ci) p = v2Blend(p, ink, (ra + ta) * ci >> 8);
    }
#else
  for (int y = (cy - E) & ~1; y <= cy + E; y += 2)
    for (int x = (cx - E) & ~1; x <= cx + E; x += 2) {
      int dx = x + 1 - cx, dy = y + 1 - cy, d2 = dx * dx + dy * dy;
      if (d2 > E * E) continue;
      uint16_t c, *q = fb + y * V2_W + x;
      if (d2 > R * R) c = st == 1 ? ring : v2Blend(q[0], ink, ra);
      else c = v2Blend(q[0], ink, ra + ta);
      q[0] = q[1] = q[V2_W] = q[V2_W + 1] = c;
    }
#endif
}
static void fsSlot(int cx, int cy, int16_t st, bool shiny) {
  const int R = FS_SLOT_R;
  fsDisc(cx, cy, R, st == 1);
  if (st <= 1) return;
  const uint8_t *t = thumbs.getForm(st, shiny);
  int h = fsThumbH(t, 1);
  fsClipC[0] = cx, fsClipC[1] = cy, fsClipC[2] = R;
  fsThumb(t, cx, cy + (h < 2 * R - 4 ? h / 2 : R - 3), 1);
  fsClipC[2] = 0;
}
// the casts along the top (fsSlot()), and under them the weight so far
static void fsSlots(const int16_t *st, bool total = true) {
  int x0 = CX - (5 * FS_SLOT + 4 * 6) / 2, y = 40, kg = 0;
  for (int i = 0; i < 5; i++) {
    fsSlot(x0 + i * (FS_SLOT + 6) + FS_SLOT / 2, y + FS_SLOT / 2, st[i], false);
    if (st[i] > 1) kg += fsKg10(st[i]);
  }
  if (!total || !kg) return;
  char b[16];
  fsKgText(b, sizeof(b), kg);
  setSize(2);
  int w = textW(b, 2) + 20;
  gfx->fillRoundRect(CX - w / 2 - 3, y + FS_SLOT + 6, w + 6, 26, 8, sc565(0x1B1B25));
  gfx->fillRoundRect(CX - w / 2, y + FS_SLOT + 9, w, 20, 6, sc565(0x3A6FA0));
  gfx->setTextColor(UI_WHITE);
  setCur(CX - textW(b, 2) / 2, y + FS_SLOT + 12);
  printT(b);
}

// the rod, held by you: its butt just off the screen's left edge, the grip and the reel just inside it, up over the
// empty pier; tip: where the rod's tip is, (bx, by) how far it's bent
#define FS_HAND_X 22     // the hand (fsRod puts the butt behind and below it, under the rim)
#define FS_HAND_Y 160
// The bend: each moment's (bx, by) read as how hard the line pulls (bx + by; under 0, springing back), the rod bowing
// up off the straight line from the hand to the tip by that much, so its tip curves down toward the fish, as a loaded
// rod does (Fedor: pulling up, it bent the wrong way, its tip pointing up: the push went straight down, which sagged
// this long rod's middle)
static void fsAngler(int tx, int ty, int bx = 0, int by = 0) {
  float gx = FS_HAND_X - 14, gy = FS_HAND_Y + 12, dx = tx - gx, dy = ty - gy, L = hypotf(dx, dy);  // (fsRod()'s butt)
  float k = (bx + by) * 0.8f, nx = dy / L, ny = -dx / L;  // px; the line's upper side
  fsRod(FS_HAND_X, FS_HAND_Y, tx, ty, (int)(nx * k), (int)(ny * k));
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
// ===================================================================== the mock screens
static const int16_t FS_ST_1[5] = { 1, 0, 0, 0, 0 };
static const int16_t FS_ST_2[5] = { 129, 1, 0, 0, 0 };
static const int16_t FS_ST_3[5] = { 129, 118, 1, 0, 0 };
static const int16_t FS_ST_4[5] = { 129, 118, -1, 1, 0 };
static const int16_t FS_ST_5[5] = { 129, 118, -1, 120, 1 };
static const int16_t FS_ST_END[5] = { 129, 118, -1, 120, 130 };

// 1: the Games menu with a sixth cartridge, Reel Fishing (REEL / FISHING), in Magikarp's orange: the sea, a float, a Magikarp leaping
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
  v2Cart(XY[5][0], XY[5][1], fsLbl, "REEL FISHING", 0xF07A3A);
  v2CloseX();
}

// the water as you'll fish it, before anything's on it
static void fsBack() { fsScene(); }

// 2: its how-to card over the pier
static void fsMockHow() {
  fsBack();
  fsAngler(FS_TIP_X, FS_TIP_Y);
  fsLine(FS_TIP_X, FS_TIP_Y, FS_TIP_X + 4, FS_TIP_Y + 60, 0, 0xF4F4F8);
  v2IntroCard(CX, CY, true, "REEL FISHING",
              "Cast by a big shadow: big ones weigh more! When the float goes under, tap. Keep tapping to reel it "
              "in, not too fast, not too slow. Each one is weighed, then let go.");
}

// 3: casting: pick a fish and tap the water by it; the float flies there and a ring marks the spot. Blobs under the
// water, the big (heavy) one far out where it's deep, and bubbles where one's about to bite
static void fsMockCast() {
  fsBack();
  fsBlob(212, 262, 3);
  fsBlob(296, 330, 4);
  fsBlob(404, 318, 6, 130);
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
  fsBlob(380, 286, 4);
  fsBlob(170, 300, 3, 110);
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
  fsBlob(FS_FLOAT_X + 30, FS_SURF + 66, 4);
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
  fsBlob(FS_FLOAT_X + 8, FS_SURF + 70, 4, 190);
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

// a hooked fish under the water, fighting at (cx, cy): its blob, the float dragged down by it, bubbles, splashes over
static void fsHooked(int cx, int cy, int s, int thrash) {
  fsBlob(cx, cy, s, 200);
  fsFloat(cx - 5 * s - 14, cy - 6);
  fsBubbles(cx - 3 * s, cy - 18, 3);
  if (thrash) {
    fsSplash(cx, FS_SURF - 4, 20 + 10 * thrash, 10 + 12 * thrash, 11 + thrash);
    fsRing(cx, 26 + 12 * thrash, 0xFFFFFF, 2);
    for (int k = 0; k < 3; k++) {  // motion marks either side of it
      scRect(cx + 5 * s + 8, cy - 10 + k * 10, cx + 5 * s + 19 + thrash * 4, cy - 7 + k * 10, 0xFFFFFF);
      scRect(cx - 5 * s - 36 - thrash * 4, cy - 10 + k * 10, cx - 5 * s - 25, cy - 7 + k * 10, 0xFFFFFF);
    }
  }
}

// the reel turning: three little white marks round it (each tap turns it a notch)
static void fsSpin() {
  static const int8_t M[3][2] = { { 0, -14 }, { 13, 6 }, { -13, 6 } };
  for (auto &m : M) {
    fsSq(fsReelX + m[0], fsReelY + m[1], 4, 0xFFFFFF);
    fsSq(fsReelX + m[0] * 5 / 4 + (m[1] < 0 ? 4 : m[0] > 0 ? -2 : 2), fsReelY + m[1] * 5 / 4 + (m[1] < 0 ? 0 : 3), 2, 0xFFFFFF);
  }
}
// where a finger tapped: a ring spreading from it
static void fsTapMark(int cx, int cy) {
  fsCircle(cx, cy, 16, 0x1B1B25, 6);
  fsCircle(cx, cy, 16, 0xFFFFFF, 2);
  fsCircle(cx, cy, 6, 0xFFFFFF, 4);
}

// 7: reeling it in: tap, tap, tap (anywhere, or the button). Each tap turns the reel a notch, brings it a step closer
// and tightens the line; the line slackens between taps. Kept in the green, it comes in. Each weight has its own pace
// (Fedor): a small one barely pulls and slips off quickly, so it wants quick taps
static void fsMockReel() {
  fsBack();
  int fx = 314, fy = 296, s = 3;
  fsAngler(FS_TIP_X + 4, FS_TIP_Y + 24, 18, 12);
  fsSpin();
  fsLine(FS_TIP_X + 4, FS_TIP_Y + 24, fx - 5 * s - 12, fy - 4, 0, fsLineCol(50));
  fsHooked(fx, fy, s, 1);
  fsLineArc(50);
  fsSlots(FS_ST_4);
  fsTapMark(150, 336);
  fsSay("QUICK TAPS!", CX - 10, 396, 0xFFFFFF);
}

// 16: a giant on the line: slow and steady. Every tap pulls hard and the line takes longer to ease, so the taps come
// slowly, and its green is narrower. (Shown beside 7: a different weight, a different pace)
static void fsMockReelBig() {
  fsBack();
  int fx = 336, fy = 318, s = 7;
  fsAngler(FS_TIP_X - 2, FS_TIP_Y + 48, 30, 28);
  fsSpin();
  fsLine(FS_TIP_X - 2, FS_TIP_Y + 48, fx - 5 * s - 12, fy - 4, 0, fsLineCol(60));
  fsHooked(fx, fy, s, 1);
  fsLineArc(58, 42, 70);
  fsSlots(FS_ST_5);
  fsTapMark(150, 336);
  fsSay("SLOW AND STEADY", CX - 10, 396, 0xFFFFFF);
}

// 8: too fast: tapping hard while it thrashes. The line red, the rod bent double, the notch in the red, the rim
// glowing red (it pulses), SLOWER!
static void fsMockStrain() {
  fsBack();
  fsRimGlow(0xE8503A, 230, 40);
  int fx = 296, fy = 290;
  fsAngler(FS_TIP_X - 6, FS_TIP_Y + 56, 36, 30);
  fsSpin();
  fsLine(FS_TIP_X - 6, FS_TIP_Y + 56, fx - 32, fy - 4, 0, fsLineCol(92));
  for (int k = 0; k < 3; k++) {  // sparks off the rod's tip
    static const int8_t SP[3][2] = { { -14, -12 }, { 10, -16 }, { 16, 2 } };
    fsSq(FS_TIP_X - 6 + SP[k][0], FS_TIP_Y + 56 + SP[k][1], 4, 0xFFE27A);
  }
  fsHooked(fx, fy, 4, 2);
  fsLineArc(92);
  fsSlots(FS_ST_4);
  fsSay("SLOWER!", CX - 10, 386, 0xFF8A7A, 4);
}

// 9: too slow: no taps for a while. The line sags in a pale curve, the rod straightens, the notch sinks into the
// blue, the rim glows a cold blue, and the blob drifts off with the float: FASTER! (a little longer and it's gone)
static void fsMockSlack() {
  fsBack();
  fsRimGlow(0x7CC4EA, 190, 34);
  int fx = 372, fy = 312;
  fsBlob(fx, fy, 4, 170);
  for (int k = 0; k < 3; k++) scRect(fx - 50 - k * 22, fy - 2 + (k % 2) * 6, fx - 41 - k * 22, fy + 1 + (k % 2) * 6, 0xD8F0FF);  // drifting
  fsFloat(fx - 34, FS_SURF + 2);
  fsAngler(FS_TIP_X, FS_TIP_Y - 4);
  fsLine(FS_TIP_X, FS_TIP_Y - 4, fx - 34, FS_SURF - 16, 34, fsLineCol(10));
  fsLineArc(10);
  fsSlots(FS_ST_4);
  fsSay("FASTER!", CX - 10, 386, 0x9FD6F0, 4);
}

// 10: landed! It leaps out onto the pier's side of the screen, drops flying, and its name goes up on the board
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
  fsBoard("STARYU!", 44);  // (the casts come back along the top once it's been weighed)
}

// the weigh-in: a red platform scale with a round dial, the Pokemon sitting on it, kg turning the needle
static void fsScale(int cx, int top, int kg10, const uint8_t *who) {
  const uint16_t ink = sc565(0x1B1B25);
  gfx->fillRoundRect(cx - 58, top + 8, 116, 70, 10, ink);  // the body
  gfx->fillRoundRect(cx - 55, top + 11, 110, 64, 8, sc565(0xE8503A));
  gfx->fillRect(cx - 49, top + 13, 98, 4, sc565(0xF58C78));
  int dy = top + 46;
  gfx->fillCircle(cx, dy, 25, ink);  // the dial
  gfx->fillCircle(cx, dy, 22, UI_WHITE);
  for (int k = 0; k < 10; k++) {
    float a = k * 0.6283f - 1.5708f;
    fsSq(cx + (int)(17 * cosf(a)), dy + (int)(17 * sinf(a)), k % 5 ? 2 : 4, 0x1B1B25);
  }
  float a = (kg10 % 1000) / 1000.0f * 6.2832f - 1.5708f;  // a turn per 100 kg
  for (int r = 0; r < 15; r += 2) fsSq(cx + (int)(r * cosf(a)), dy + (int)(r * sinf(a)), 4, 0xE8503A);
  gfx->fillCircle(cx, dy, 4, ink);
  gfx->fillRoundRect(cx - 80, top - 4, 160, 16, 5, ink);  // the platform it sits on
  gfx->fillRoundRect(cx - 77, top - 1, 154, 10, 4, sc565(0xAAB0BC));
  gfx->fillRect(cx - 73, top - 1, 146, 3, sc565(0xE4E6EC));
  fsThumb(who, cx, top, 3);
}

// 11: the weigh-in, the moment that makes it fishing and not catching: it sits on the scale, the needle swings to
// its weight, the weight goes on your total, and it hops back into the water
static void fsMockWeigh() {
  fsBack();
  fsAngler(FS_TIP_X, FS_TIP_Y);
  v2DimFrame(115);
  fsBoard("WEIGH-IN", 70);
  v2Panel(108, 112, 358, 360, 12);
  fsScale(CX, 236, 361, thumbs.getForm(120, false));
  setSize(4);
  gfx->setTextColor(UI_INK);
  setCur(centerX("36.1 kg", 4), 322);
  printT("36.1 kg");
  fsSay("BACK IN IT GOES!", CX, 376, 0xFFFFFF);
}

// ---- the weigh-in, three ways to try (Fedor: "The scale just doesnt match and its not a great looking screen. Gotta
// be another way to do it"). Each keeps the scene, with no white card over it: FISHMOCK 20, 21 and 22 ----
// 20 (A): it hangs from the rod's tip under a brass spring scale, the way anglers weigh one, the pointer at its weight
static void fsHangScale(int cx, int top, int h, float frac) {
  scRect(cx - 10, top, cx + 9, top + h - 1, 0x1B1B25);  // the tube
  scRect(cx - 8, top + 2, cx + 7, top + h - 3, 0xD8A848);
  scRect(cx - 8, top + 2, cx - 5, top + h - 3, 0xF2D68A);
  scRect(cx + 4, top + 2, cx + 7, top + h - 3, 0xA87A2C);
  scRect(cx - 4, top + 6, cx + 3, top + h - 7, 0xF4ECD0);  // its window, marked
  for (int y = top + 8; y < top + h - 8; y += 6) scRect(cx - 4, y, cx - 1, y + 1, 0x1B1B25);
  int py = top + 8 + (int)((h - 18) * frac) / 2 * 2;
  scRect(cx - 6, py, cx + 5, py + 1, 0xE8503A);  // the pointer
  fsCircle(cx, top - 6, 6, 0x1B1B25, 2);  // the ring it hangs by
  scRect(cx - 2, top + h, cx + 1, top + h + 9, 0xAAB0BC);  // the hook
  scRect(cx - 8, top + h + 8, cx + 1, top + h + 11, 0xAAB0BC);
  scRect(cx - 8, top + h + 2, cx - 5, top + h + 11, 0xAAB0BC);
}
static void fsMockWeighHang() {
  fsBack();
  const uint8_t *t = thumbs.getForm(120, false);  // Staryu, 36.1 kg
  const int cx = 244, top = 98, h = 48;
  fsAngler(cx, 76, 6, 6);
  fsLine(cx, 76, cx, top - 12, 0, 0xF4F4F8);
  fsHangScale(cx, top, h, 0.72f);
  fsThumb(t, cx - 6, top + h + 14 + fsThumbH(t, 2), 2);
  fsBoard("STARYU!", 44);
  fsSay("36.1 kg", CX, 252, 0xFFFFFF, 4);
  fsSay("BACK IN IT GOES!", CX, 376, 0xFFFFFF);
}
// 22 (C): no scale at all: its picture, big in a round see-through holder like the casts', its name on the sign over
// it and its weight in a dark pill under it, as the total is
static void fsMockWeighPhoto() {
  fsBack();
  const uint8_t *t = thumbs.getForm(120, false);
  fsAngler(FS_TIP_X, FS_TIP_Y);
  const int cx = CX, cy = 178, R = 70;
  fsDisc(cx, cy, R, false);
  fsThumb(t, cx, cy + fsThumbH(t, 3) / 2, 3);
  for (int i = 0; i < 4; i++) {  // sparkles round it
    static const int8_t SP[4][2] = { { -86, -52 }, { 80, -66 }, { 88, 30 }, { -78, 44 } };
    int gx = SNAP2(cx + SP[i][0]), gy = SNAP2(cy + SP[i][1]);
    scGridRect(gx, gy - 6, gx + 3, gy + 9, 0xFFFFFF);
    scGridRect(gx - 6, gy, gx + 9, gy + 3, 0xFFFFFF);
  }
  fsBoard("STARYU!", 44);
  const char *w = "36.1 kg";
  setSize(3);
  int tw = textW(w, 3), pw = tw + 32;
  gfx->fillRoundRect(cx - pw / 2 - 3, 262, pw + 6, 40, 12, sc565(0x1B1B25));
  gfx->fillRoundRect(cx - pw / 2, 265, pw, 34, 10, sc565(0x3A6FA0));
  gfx->setTextColor(UI_WHITE);
  setCur(cx - tw / 2, 272);
  printT(w);
  fsSay("BACK IN IT GOES!", CX, 376, 0xFFFFFF);
}

// 12: got away: the line snapped and hangs from the rod, the blob shooting off with a trail of bubbles
static void fsMockAway() {
  fsBack();
  fsBlob(414, 318, 4, 120);
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

// 13: the big one: Gyarados, the giant that turns up once a game, from the deep. Gold rays and a gold board
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
  fsSay("THE BIG ONE!", CX - 20, 392, 0xFFE27A, 3);
}

// 14: the end: the total weight big, the five with what each weighed, how far along the goals it got (bronze 50 kg,
// silver 150, gold 300) and the record. PLAY AGAIN! where every game has it
static void fsMockResult() {
  fsBack();
  fsAngler(FS_TIP_X, FS_TIP_Y);
  v2DimFrame(115);
  fsBoard("REEL FISHING", 70);
  v2Panel(96, 112, 370, V2_AGAIN_Y + V2_AGAIN_H + 16, 12);
  int total = 0;
  for (int i = 0; i < 5; i++) total += FS_ST_END[i] > 1 ? fsKg10(FS_ST_END[i]) : 0;
  char b[24];
  fsKgText(b, sizeof(b), total);
  setSize(4);
  gfx->setTextColor(UI_INK);
  setCur(centerX(b, 4), 120);
  printT(b);
  int y = 158, x0 = CX - (5 * 40 + 4 * 6) / 2;
  for (int i = 0; i < 5; i++) {  // the five casts, and what each weighed
    int x = x0 + i * 46, d = FS_ST_END[i];
    gfx->fillRoundRect(x, y, 40, 40, 8, sc565(d < 0 ? 0xE4E6EC : 0xEAF6FB));
    char w[8];
    if (d < 0) {
      for (int k = 0; k < 14; k += 2) {
        fsSq(x + 13 + k, y + 13 + k, 4, 0xE8503A);
        fsSq(x + 27 - k, y + 13 + k, 4, 0xE8503A);
      }
      snprintf(w, sizeof(w), "-");
    } else {
      const uint8_t *t = thumbs.getForm(d, false);
      int h = fsThumbH(t, 1);
      fsClip[0] = x + 2, fsClip[1] = y + 2, fsClip[2] = x + 38, fsClip[3] = y + 38;
      fsThumb(t, x + 20, y + 20 + (h < 36 ? h / 2 : 17), 1);
      fsClip[0] = fsClip[1] = 0, fsClip[2] = fsClip[3] = V2_W;
      snprintf(w, sizeof(w), "%d", (int)(fishLb10(fsKg10(d)) + 5) / 10);
    }
    setSize(2);
    gfx->setTextColor(V2_INK_MUTED);
    setCur(x + 20 - textW(w, 2) / 2 + 1, y + 43);
    printT(w);
  }
  // the goals: a bar to 330 kg with a medal at 50, 150 and 300, filled to the total
  const int bx0 = 122, bx1 = 344, by = 222, gk[3] = { 50, 150, 300 };
  const Rgb gc[3] = { 0xC8783A, 0xB4BAC8, 0xF8C030 };
  int fx = bx0 + (bx1 - bx0) * (total > 3300 ? 3300 : total) / 3300;
  gfx->fillRoundRect(bx0 - 2, by - 2, bx1 - bx0 + 4, 12, 5, sc565(0x1B1B25));
  gfx->fillRoundRect(bx0, by, bx1 - bx0, 8, 4, sc565(0xD8DCE4));
  gfx->fillRoundRect(bx0, by, fx - bx0, 8, 4, sc565(0x58B868));
  for (int k = 0; k < 3; k++) {
    int mx = bx0 + (bx1 - bx0) * gk[k] / 330;
    bool got = total >= gk[k] * 10;
    gfx->fillCircle(mx, by + 4, 9, sc565(0x1B1B25));
    gfx->fillCircle(mx, by + 4, 7, sc565(got ? gc[k] : 0xE4E6EC));
    if (got) gfx->fillRect(mx - 3, by + 1, 3, 3, UI_WHITE);
  }
  setSize(2);
  gfx->setTextColor(C565(0xc0, 0x84, 0x10));
  setCur(centerX("GOLD!  NEW RECORD!", 2), 240);
  printT("GOLD!  NEW RECORD!");
  v2AgainButton(V2_AGAIN_Y);
}

// 15: the bite at night: the same moment in the dark. The float's lamp, the rings and the "!" carry it
static void fsMockNight() {
  fsNight = true;
  fsMockBite();
  fsNight = false;
}

// ===================================================================== the game: drawn from fishing.h's state
// Every moment has its own look, so the Knomi (no speaker, no buzz) plays it as well as a computer: the float, rings,
// the "!", the rim glowing (red too tight, blue too slack), the line's colour and sag, the rod's bend, the LINE arc
// with the fish's own green band, and the hooked blob swelling on the fight's beat (the fight song plays that beat on a
// computer). The sketch keeps the state (fish) and the times things last happened (fishTapT, fishBiteT, fishSoonT).

// the LINE arc's cells, worked out once (their place along it and how far out), so a frame only colours them: on the
// Knomi's chip an atan2 a cell, every frame, would cost more than the rest of the frame
// It has round ends, as the need bars have (v2Seg(), v2In()), and its dark edge goes round them (Fedor: "rounded edges
// on the corners", for continuity)
struct FsArcCell { int16_t x, y; uint8_t f, k; };  // f: 0..200 along the arc (bottom to top); k: 0 bar, 1 its edge, 2 notch room, +4 lit
static FsArcCell *fsArc = nullptr;
static int fsArcN = 0;
#define FS_ARC_A0 52.0f   // its bottom end (degrees, 0 = right)
#define FS_ARC_A1 -34.0f  // its top end
static void fsArcBuild() {
  if (fsArc) return;
  // the bar, its dark edge 3 px round it and the notch's room 7 px round it: each a segment with round ends
  const V2Seg bar = v2Seg(V2_BAR_RIN, V2_BAR_ROUT, FS_ARC_A1, FS_ARC_A0),
              edge = v2Seg(V2_BAR_RIN - 3, V2_BAR_ROUT + 3, FS_ARC_A1, FS_ARC_A0),
              room = v2Seg(V2_BAR_RIN - 7, V2_BAR_ROUT + 7, FS_ARC_A1, FS_ARC_A0);
  int n = 0;
  for (int pass = 0; pass < 2; pass++) {
    // (on the 2 px grid, each cell a Knomi pixel: they started at CX, an odd x, so every cell fell across two of the
    // Knomi's pixels and the arc came out blurred, Fedor: "blurry")
    for (int y = 0; y < V2_W; y += 2)
      for (int x = CX & ~1; x < V2_W; x += 2) {
        float dx = x + 1 - CX, dy = y + 1 - CY, r2 = dx * dx + dy * dy, r = sqrtf(r2);
        if (r < V2_BAR_RIN - 7 || r > V2_BAR_ROUT + 7 || !v2In(room, dx, dy, r2)) continue;
        float a = atan2f(dy, dx) * 57.29578f, f = (FS_ARC_A0 - a) / (FS_ARC_A0 - FS_ARC_A1) * 200;  // (past an end: 0 or 200)
        uint8_t k = v2In(bar, dx, dy, r2) ? 0 : v2In(edge, dx, dy, r2) ? 1 : 2;
        if (k == 0 && r > V2_BAR_ROUT - 4) k |= 4;
        if (pass) fsArc[n] = { (int16_t)x, (int16_t)y, (uint8_t)(f < 0 ? 0 : f > 200 ? 200 : f), k };
        n++;
      }
    if (!pass) {
      fsArc = (FsArcCell *)ps_malloc(n * sizeof(FsArcCell));
      if (!fsArc) return;
      fsArcN = n;
      n = 0;
    }
  }
}
#ifdef BOARD_HOST
// On a computer and the web, where the canvas shows 2 to 4 times bigger, the arc is drawn a pixel at a time with soft
// edges, as the need bars are there (v2ArcCover()), so it's as clean as they are at every size (Fedor: "sharpen and
// clean up its lines on all sizes and builds"; its 2 px cells came out in steps). Its pixels are worked out once: how
// far along the arc each is (f, as the cells' 0..200; s, how much f a pixel is there) and how much of it the bar, its
// dark edge and the notch's room cover (0..256)
struct FsArcPx { int16_t x, y; float f, s, r; uint16_t bar, edge, room; };
static FsArcPx *fsArcPx = nullptr;
static int fsArcPxN = 0;
static void fsArcPxBuild() {
  if (fsArcPx) return;
  const float rm = (V2_BAR_RIN + V2_BAR_ROUT) / 2.0f, h = (V2_BAR_ROUT - V2_BAR_RIN) / 2.0f;
  const V2Arc bar = v2ArcOf(rm, h, FS_ARC_A1, FS_ARC_A0), edge = v2ArcOf(rm, h + 3, FS_ARC_A1, FS_ARC_A0),
              room = v2ArcOf(rm, h + 7, FS_ARC_A1, FS_ARC_A0);
  int n = 0;
  for (int pass = 0; pass < 2; pass++) {
    for (int y = 0; y < V2_W; y++)
      for (int x = CX; x < V2_W; x++) {
        float fx = x + 0.5f - CX, fy = y + 0.5f - CY;
        int cr = v2ArcCover(room, fx, fy);
        if (!cr) continue;
        if (pass) {
          float r = hypotf(fx, fy), a = atan2f(fy, fx) * 57.29578f, f = (FS_ARC_A0 - a) / (FS_ARC_A0 - FS_ARC_A1) * 200;
          fsArcPx[n] = { (int16_t)x, (int16_t)y, f < 0 ? 0 : f > 200 ? 200 : f,
                         200 / ((FS_ARC_A0 - FS_ARC_A1) * 0.01745329f * r),  // (f a pixel along the arc, there)
                         r, (uint16_t)v2ArcCover(bar, fx, fy), (uint16_t)v2ArcCover(edge, fx, fy), (uint16_t)cr };
        }
        n++;
      }
    if (!pass) {
      fsArcPx = (FsArcPx *)ps_malloc(n * sizeof(FsArcPx));
      if (!fsArcPx) return;
      fsArcPxN = n;
      n = 0;
    }
  }
}
#endif
// the arc now: tension pct in the zones lo..hi, the green brighter on the beat
static void fsArcDraw(float pct, int lo, int hi, bool beat) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  const uint16_t ink = sc565(0x1B1B25), white = UI_WHITE;
  uint16_t dim[3] = { sc565(0x2E4A72), sc565(beat ? 0x3E7A4A : 0x2E5A3A), sc565(0x7A3A3A) };
  uint16_t lit[3] = { sc565(0x7CC4EA), sc565(beat ? 0x8AE08A : 0x58B868), sc565(0xE8503A) };
  uint16_t litT[3] = { sc565(scMix(0x7CC4EA, 0xFFFFFF, .35)), sc565(scMix(beat ? 0x8AE08A : 0x58B868, 0xFFFFFF, .35)),
                       sc565(scMix(0xE8503A, 0xFFFFFF, .35)) };
  float p2 = pct * 2;
#ifdef BOARD_HOST
  // the same shapes as the Knomi's cells below, their sizes in pixels (a cell's f is 1.65 px along the arc): the dark
  // edge and the bar in its zones' colours with dark seams between them, then the notch, dark round it, white in it
  fsArcPxBuild();
  if (!fsArcPx) return;
  for (int i = 0; i < fsArcPxN; i++) {
    const FsArcPx &c = fsArcPx[i];
    const float px = 1 / c.s, g = fabsf(c.f - p2) * px;  // px: pixels to one f here; g: pixels from the notch's middle
    uint16_t &q = fb[c.y * V2_W + c.x];
    uint16_t p = v2Blend(q, ink, c.edge);
    if (c.bar) {
      int z = c.f < 2 * lo ? 0 : c.f < 2 * hi ? 1 : 2;
      uint16_t col = c.f <= p2 ? (c.r > V2_BAR_ROUT - 4 ? litT[z] : lit[z]) : dim[z];
      int seam = fsCover(1.0f * px - fabsf(c.f - 2 * lo) * px), seam2 = fsCover(1.0f * px - fabsf(c.f - 2 * hi) * px);
      col = v2Blend(col, ink, seam > seam2 ? seam : seam2);
      p = v2Blend(p, col, c.bar);
    }
    p = v2Blend(p, ink, fsCover(4.4f * px - g) * c.room >> 8);
    int w = fsCover(2.0f * px - g) * c.edge >> 8, w2 = fsCover(1.0f * px - g) * c.room >> 8;
    q = v2Blend(p, white, w > w2 ? w : w2);
  }
  setSize(2);
  v2BarLabel(fb, T(S_FISH_LINE), -45, UI_WHITE);
  return;
#endif
  fsArcBuild();
  if (!fsArc) return;
  for (int i = 0; i < fsArcN; i++) {
    const FsArcCell &c = fsArc[i];
    float m = fabsf(c.f - p2);
    uint8_t k = c.k & 3;
    uint16_t col;
    if (m <= 2.0f && k != 2) col = white;               // the notch, a little proud of the bar
    else if (m <= 2.0f && k == 2 && m <= 1.0f) col = white;
    else if (m <= 4.4f) col = ink;
    else if (k == 2) continue;
    else if (k == 1) col = ink;
    else if (abs(c.f - 2 * lo) <= 1 || abs(c.f - 2 * hi) <= 1) col = ink;  // where the zones meet
    else {
      int z = c.f < 2 * lo ? 0 : c.f < 2 * hi ? 1 : 2;
      col = c.f <= p2 ? ((c.k & 4) ? litT[z] : lit[z]) : dim[z];
    }
    uint16_t *q = fb + c.y * V2_W + c.x;
    q[0] = q[1] = q[V2_W] = q[V2_W + 1] = col;
  }
  setSize(2);
  v2BarLabel(fb, T(S_FISH_LINE), -45, UI_WHITE);
}

// The weigh-in's gauge (Fedor chose it, the mockups' B: "a cool animation of it getting to the weight of the pokemon",
// on the left rim, since the LINE bar is on the right): the LINE bar mirrored, its round ends and dark edge, filling
// gold from the bottom, and the same needle as its notch where the weight lands: 0 at the bottom, the heaviest fish
// there can be at the top (Fedor), spaced out at the light end (by the square root) so a 5 kg fish still lifts it, with
// a faint mark every 100 lb (it reads in pounds, LB past its top); lit for a moment when it gets there. Its pixels are the LINE bar's, across the middle
// (x to 465 - x; a Knomi cell, 464 - x)
static float fsGaugeAt(float kg10) { return sqrtf(kg10 / (float)fishHeaviestKg10()); }  // 0..1 up the gauge
static void fsGaugeDraw(float pos, bool lit) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  const uint16_t ink = sc565(0x1B1B25), white = UI_WHITE, gold = sc565(lit ? 0xFFE27A : 0xF2C230),
                 goldT = sc565(scMix(lit ? 0xFFE27A : 0xF2C230, 0xFFFFFF, .4)), dim = sc565(0x6A5A2A);
  const float p2 = pos * 200;
  float mark[6];  // the 100 lb marks, as f (the heaviest fish is a little over 600 lb)
  for (int k = 0; k < 6; k++) mark[k] = fsGaugeAt((k + 1) * 1000.0f / FISH_LB_PER_KG) * 200;
#ifdef BOARD_HOST
  fsArcPxBuild();
  if (!fsArcPx) return;
  for (int i = 0; i < fsArcPxN; i++) {
    const FsArcPx &c = fsArcPx[i];
    const float px = 1 / c.s, g = fabsf(c.f - p2) * px;  // pixels to one f here; pixels from the needle
    uint16_t &q = fb[c.y * V2_W + (V2_W - 1 - c.x)];
    uint16_t p = v2Blend(q, ink, c.edge);
    if (c.bar) {
      uint16_t col = c.f <= p2 ? (c.r > V2_BAR_ROUT - 4 ? goldT : gold) : dim;
      for (float m : mark)
        if (m < 198) col = v2Blend(col, ink, fsCover(0.7f - fabsf(c.f - m) * px) * 2 / 3);
      p = v2Blend(p, col, c.bar);
    }
    p = v2Blend(p, ink, fsCover(4.4f * px - g) * c.room >> 8);  // the needle, as the LINE bar's notch
    int w = fsCover(2.0f * px - g) * c.edge >> 8, w2 = fsCover(1.0f * px - g) * c.room >> 8;
    q = v2Blend(p, white, w > w2 ? w : w2);
  }
#else
  fsArcBuild();
  if (!fsArc) return;
  for (int i = 0; i < fsArcN; i++) {
    const FsArcCell &c = fsArc[i];
    float m = fabsf(c.f - p2);
    uint8_t k = c.k & 3;
    uint16_t col;
    if (m <= 2.0f && (k != 2 || m <= 1.0f)) col = white;  // the needle
    else if (m <= 4.4f) col = ink;
    else if (k == 2) continue;
    else if (k == 1) col = ink;
    else {
      bool marked = false;
      for (float mk : mark) marked |= mk < 198 && fabsf(c.f - mk) < 0.6f;
      col = marked ? ink : c.f <= p2 ? ((c.k & 4) ? goldT : gold) : dim;
    }
    uint16_t *q = fb + c.y * V2_W + (V2_W - 2 - c.x);
    q[0] = q[1] = q[V2_W] = q[V2_W + 1] = col;
  }
#endif
  setSize(2);
  v2BarLabel(fb, "LB", 225, UI_WHITE);  // past its top end, as LINE's is
}
// The weigh-in, t s in: the fish in the air in its glow; the gauge filling (FS_FILL_S, easing in) to its weight, the
// needle and the number counting up together; then a line saying how big it is for one of its kind (kindKg10 its
// Pokedex weight: the fish come 0.8 to 1.2 times that). No scale and no card over the scene
#define FS_FILL_S 1.2f
static void fsWeighDraw(int16_t dex, bool shiny, uint16_t kg10, uint16_t kindKg10, float t) {
  const uint8_t *th = thumbs.getForm(dex, shiny);
  const int cx = CX + 16;
  float frac = kindKg10 ? (kg10 / (float)kindKg10 - 0.8f) / 0.4f : 0.5f, p = t / FS_FILL_S;
  frac = frac < 0 ? 0 : frac > 1 ? 1 : frac;
  if (p > 1) p = 1;
  const float e = 1 - (1 - p) * (1 - p) * (1 - p);
  twGlow(cx, 168, 74, 0xFFF3B0, 150);
  fsThumb(th, cx, 168 + fsThumbH(th, 3) / 2, 3);
  fsGaugeDraw(fsGaugeAt(kg10 * e), p >= 1 && t - FS_FILL_S < 0.3f);
  fsBoard(T(S_FISH_WEIGH), 44);
  char w[24];
  uint32_t shown = fishLb10((uint32_t)(kg10 * e + 0.5f));
  snprintf(w, sizeof(w), T(S_FISH_LB_FMT), (unsigned)(shown / 10), (unsigned)(shown % 10));
  fsSay(w, cx, 244, 0xFFFFFF, 4);
  if (p >= 1) {
    char nm[40], line[80];
    v2Caps(dexName(dex), nm, sizeof(nm));
    snprintf(line, sizeof(line), T(frac < 0.34f ? S_FISH_SMALL_FMT : frac < 0.67f ? S_FISH_FINE_FMT : S_FISH_LARGE_FMT), nm);
    fsSay(line, cx, 296, 0xFFE27A);
    if (fishBigNew) fsSay(T(S_FISH_BIGGEST), cx, 330, 0x9CF0A0);  // heavier than any landed before
  }
  fsSay(T(S_FISH_BACK), CX, 376, 0xFFFFFF);
}
static void fsMockWeighGauge() {  // (the game's own, over and over: Staryu, 36.1 kg of a kind's 34.5)
  fsBack();
  fsAngler(FS_TIP_X, FS_TIP_Y);
  fsWeighDraw(120, false, 361, 345, (millis() % 2800) / 1000.0f);
}
// a blob of any size (s px a unit, as fsBlob). At night the moon catches its top edge, so its size still shows
static void fsBlobF(float cx, float cy, float s, int a256) {
  Rgb col = fsNight ? 0x050A18 : 0x0E2A5C;
  float rx = 5.0f * s, ry = 3.0f * s;
  for (int y = scFl4((int)(cy - ry)) * 4; y < cy + ry; y += 4)
    for (int x = scFl4((int)(cx - rx)) * 4; x < cx + rx; x += 4) {
      float dx = (x + 2 - cx) / rx, dy = (y + 2 - cy) / ry, d = dx * dx + dy * dy;
      if (d > 1 || (d > 0.6f && (scFl4(x) + scFl4(y)) % 2)) continue;
      float uy = (y - 2 - cy) / ry;
      if (fsNight && dx * dx + uy * uy > 1 && dy < 0) fsBlend(x, y, x + 3, y + 3, 0x8AA8D0, 150);  // moonlit
      else fsBlend(x, y, x + 3, y + 3, col, a256);
    }
}

// the casts along the top, from the game: what each landed, this one edged, one that got away empty (fsSlot())
static void fsSlotsLive() {
  int16_t st[FISH_CASTS];
  for (int i = 0; i < FISH_CASTS; i++) st[i] = i < fish.cast ? fish.slot[i] : i == fish.cast ? (fish.slot[i] ? fish.slot[i] : 1) : 0;
  int x0 = CX - (5 * FS_SLOT + 4 * 6) / 2, y = 40;
  for (int i = 0; i < FISH_CASTS; i++) fsSlot(x0 + i * (FS_SLOT + 6) + FS_SLOT / 2, y + FS_SLOT / 2, st[i], fish.slotShiny[i]);
  if (!fish.total) return;
  char b[20];
  const uint32_t lb10 = fishLb10(fish.total);
  snprintf(b, sizeof(b), T(S_FISH_LB_FMT), (unsigned)(lb10 / 10), (unsigned)(lb10 % 10));
  setSize(2);
  int w = textW(b, 2) + 20;
  gfx->fillRoundRect(CX - w / 2 - 3, y + FS_SLOT + 6, w + 6, 26, 8, sc565(0x1B1B25));
  gfx->fillRoundRect(CX - w / 2, y + FS_SLOT + 9, w, 20, 6, sc565(0x3A6FA0));
  gfx->setTextColor(UI_WHITE);
  setCur(CX - textW(b, 2) / 2, y + FS_SLOT + 12);
  printT(b);
}

// the water's fish: each blob, the gone ones swimming off with a wake, bubbles over the one about to bite
static void fsBlobsLive(uint32_t now) {
  for (int i = 0; i < FISH_BLOBS; i++) {
    const FishBlob &b = fish.b[i];
    if (!b.on) continue;
    if (b.gone > 0 && b.fleeV == 0) continue;
    bool hooked = i == fish.target && (fish.phase == FP_REEL || fish.phase == FP_LANDED || fish.phase == FP_WEIGH);
    if (hooked) continue;  // (drawn with the line)
    float sz = fishBlobSize(b), bob = sinf(now * 0.002f + i * 1.7f) * 2;
    int a = b.gone > 0 ? 120 : b.cls == FC_GIANT ? 165 : 140 + (int)(b.y - 250) / 6;
    fsBlobF(b.x, b.y + bob, sz, a);
    if (b.gone > 0)
      for (int k = 0; k < 3; k++) scRect((int)b.x - (int)(5 * sz) - 14 - k * 20, (int)b.y - 2 + (k % 2) * 6,
                                        (int)b.x - (int)(5 * sz) - 5 - k * 20, (int)b.y + 1 + (k % 2) * 6, 0xD8F0FF);  // its wake
    if (i == fish.eager && b.gone <= 0) {  // bubbles rising from it, glints where they break
      for (int k = 0; k < 3; k++) {
        int rise = (int)((now / 18 + k * 22) % (int)(b.y - FS_SURF - 6 > 20 ? b.y - FS_SURF - 6 : 20));
        fsCircle((int)b.x + (k - 1) * 6, (int)b.y - 6 - rise, 2 + k % 2, fsNight ? 0x6A8CB8 : 0xD8F0FF, 2);
      }
      if ((now / 300) % 2) scRect((int)b.x - 2, FS_SURF - 8, (int)b.x + 1, FS_SURF - 5, 0xFFFFFF);
    }
  }
}

// where the rod's tip goes for the moment, and how far it's bent
static void fsRodNow(int &tx, int &ty, int &bx, int &by) {
  tx = FS_TIP_X, ty = FS_TIP_Y, bx = by = 0;
  switch (fish.phase) {
    case FP_FLY: tx = FS_TIP_X + 36, ty = FS_TIP_Y + 44, bx = 10, by = -10; break;
    case FP_WAIT:
      if (fish.t - fish.lastNibble < 0.15f && fish.nibbled) ty += 6;  // a nibble tugs it
      break;
    case FP_BITE: tx = FS_TIP_X + 6, ty = FS_TIP_Y + 18, bx = 8, by = 10; break;
    case FP_REEL: {
      float T = fish.T;
      tx = FS_TIP_X + 4 - (int)(T * 0.1f), ty = FS_TIP_Y + 10 + (int)(T * 0.45f), bx = 10 + (int)(T * 0.26f), by = 8 + (int)(T * 0.22f);
      break;
    }
    case FP_LANDED: tx = 236, ty = 142, bx = -6, by = -10; break;
    case FP_AWAY: tx = FISH_PIER_X + 68, ty = 140, bx = -6, by = -10; break;
    default: break;
  }
}

static void fsGameAim(uint32_t now) {
  int x = (int)fish.aimX;  // the aim ring: where the button or Space casts (a tap casts where it goes down)
  int r = 24 + (int)((now / 120) % 4) * 2;
  fsRing(x, r, 0xFFFFFF, 2);
  scRect(x - 2, FS_SURF - 28, x + 1, FS_SURF - 13, 0xFFFFFF);
  scRect(x - 6, FS_SURF - 16, x + 5, FS_SURF - 13, 0xFFFFFF);
  scRect(x - 2, FS_SURF - 12, x + 1, FS_SURF - 9, 0xFFFFFF);
  fsAngler(FS_TIP_X, FS_TIP_Y);
  fsLine(FS_TIP_X, FS_TIP_Y, FS_TIP_X + 4, FS_TIP_Y + 36, 0, 0xF4F4F8);
  fsFloat(FS_TIP_X + 4, FS_TIP_Y + 50, fsNight);
  fsSay(T(S_FISH_TAP_WATER), CX, 396, 0xFFFFFF);
}

// the hooked fish on the line: its blob swelling on the beat, the float dragged under beside it, a thrash's splashes
static void fsGameReel(uint32_t now, int tx, int ty) {
  const FishBlob &b = fish.b[fish.target];
  const FishClassDef &c = FISH_CLASS[b.cls];
  float sz = fishBlobSize(b), sw = 1 - fish.beat, swell = 1 + 0.18f * sw * sw * sw * sw;
  bool warn = fish.warnLeft > 0, thrash = fish.thrashLeft > 0;
  float bx = b.x, by = b.y;
  int a = warn && (now / 80) % 2 ? 110 : 205;  // a thrash coming: it flashes
  fsBlobF(bx, by, sz * swell, a);
  if (warn) fsBubbles((int)bx, (int)by - 12, 3);
  int fx = (int)(bx - 5 * sz - 14), fy = (int)by - 6;
  uint8_t z = fish.zone;
  Rgb lc = z == 0 ? 0xA8BCD4 : z == 2 ? 0xE8503A : fish.T > c.hi - 8 ? 0xFFE27A : 0xF4F4F8;
  fsLine(tx, ty, fx + 2, fy, z == 0 ? 26 : 0, lc);
  fsFloat(fx, fy);
  if (thrash) {
    fsSplash((int)bx, FS_SURF - 4, 26, 16, now / 90);
    fsRing((int)bx, 32 + (int)((now / 60) % 3) * 4, 0xFFFFFF, 2);
    for (int k = 0; k < 3; k++) {
      int dx = (int)(5 * sz) + 8;
      scRect((int)bx + dx, (int)by - 10 + k * 10, (int)bx + dx + 14, (int)by - 7 + k * 10, 0xFFFFFF);
      scRect((int)bx - dx - 30, (int)by - 10 + k * 10, (int)bx - dx - 16, (int)by - 7 + k * 10, 0xFFFFFF);
    }
  }
  if (now - fishTapT < 160) fsSpin();  // each tap turns the reel
  if (z == 2) {  // too tight: sparks off the tip
    static const int8_t SP[3][2] = { { -14, -12 }, { 10, -16 }, { 16, 2 } };
    if ((now / 100) % 2)
      for (auto &q : SP) fsSq(tx + q[0], ty + q[1], 4, 0xFFE27A);
  }
}

// the end: the total big, the five with what each weighed, the goal bar (bronze, silver, gold) and the record
static void fsResultLive() {
  v2DimFrame(115);
  char b[48];
  fsBoard(T(S_GAME_FISH), 70);
  v2Panel(96, 112, 370, V2_AGAIN_Y + V2_AGAIN_H + 16, 12);
  const uint32_t lb10 = fishLb10(fish.total);
  snprintf(b, sizeof(b), T(S_FISH_LB_FMT), (unsigned)(lb10 / 10), (unsigned)(lb10 % 10));
  setSize(4);
  gfx->setTextColor(UI_INK);
  setCur(centerX(b, 4), 118);
  printT(b);
  int y = 156, x0 = CX - (5 * 40 + 4 * 6) / 2;
  for (int i = 0; i < FISH_CASTS; i++) {
    int x = x0 + i * 46, d = fish.slot[i];
    gfx->fillRoundRect(x, y, 40, 40, 8, sc565(d < 0 ? 0xE4E6EC : 0xEAF6FB));
    char w[8];
    if (d <= 0) {
      for (int k = 0; k < 14; k += 2) {
        fsSq(x + 13 + k, y + 13 + k, 4, 0xE8503A);
        fsSq(x + 27 - k, y + 13 + k, 4, 0xE8503A);
      }
      snprintf(w, sizeof(w), "-");
    } else {
      const uint8_t *t = thumbs.getForm(d, fish.slotShiny[i]);
      int h = fsThumbH(t, 1);
      fsClip[0] = x + 2, fsClip[1] = y + 2, fsClip[2] = x + 38, fsClip[3] = y + 38;
      fsThumb(t, x + 20, y + 20 + (h < 36 ? h / 2 : 17), 1);
      fsClip[0] = fsClip[1] = 0, fsClip[2] = fsClip[3] = V2_W;
      snprintf(w, sizeof(w), "%u", (unsigned)((fishLb10(fish.slotKg10[i]) + 5) / 10));
    }
    setSize(2);
    gfx->setTextColor(V2_INK_MUTED);
    setCur(x + 20 - textW(w, 2) / 2 + 1, y + 42);
    printT(w);
  }
  const int bx0 = 122, bx1 = 344, by = 222;  // the goals: a bar to 330 kg, a medal at each line
  const uint32_t gk[3] = { FISH_GOAL_BRONZE, FISH_GOAL_SILVER, FISH_GOAL_GOLD };
  const Rgb gc[3] = { 0xC8783A, 0xB4BAC8, 0xF8C030 };
  uint32_t tot = fish.total > 3300 ? 3300 : fish.total;
  int fx = bx0 + (int)((bx1 - bx0) * tot / 3300);
  gfx->fillRoundRect(bx0 - 2, by - 2, bx1 - bx0 + 4, 12, 5, sc565(0x1B1B25));
  gfx->fillRoundRect(bx0, by, bx1 - bx0, 8, 4, sc565(0xD8DCE4));
  if (fx > bx0) gfx->fillRoundRect(bx0, by, fx - bx0, 8, 4, sc565(0x58B868));
  for (int k = 0; k < 3; k++) {
    int mx = bx0 + (int)((bx1 - bx0) * gk[k] / 3300);
    bool got = fish.total >= gk[k];
    gfx->fillCircle(mx, by + 4, 9, sc565(0x1B1B25));
    gfx->fillCircle(mx, by + 4, 7, sc565(got ? gc[k] : 0xE4E6EC));
    if (got) gfx->fillRect(mx - 3, by + 1, 3, 3, UI_WHITE);
  }
  // under it: the goal reached and the record, as much of the two as fits on the line
  uint8_t g = fishGoal(fish.total);
  static const StrId GOAL[4] = { STR_COUNT, S_FISH_BRONZE, S_FISH_SILVER, S_FISH_GOLD };
  char rec[40];
  if (fishNewHi) snprintf(rec, sizeof(rec), "%s", T(S_NEW_RECORD));
  else {
    const uint32_t hi = fishLb10(pet.fishHi);
    snprintf(rec, sizeof(rec), T(S_FISH_REC_FMT), (unsigned)(hi / 10), (unsigned)(hi % 10));
  }
  const char *gw = g ? T(GOAL[g]) : "";
  setSize(2);
  int wg = g ? textW(gw, 2) : 0, wr = textW(rec, 2), gap = g ? 16 : 0;
  if (wg + gap + wr > 266) wr = 0, gap = 0;  // (no room: the medal word alone)
  int lx = CX - (wg + gap + wr) / 2;
  if (g) {
    gfx->setTextColor(sc565(g == 3 ? 0xC08410 : g == 2 ? 0x787E8C : 0xA85A28));
    setCur(lx, 238);
    printT(gw);
  }
  if (wr) {
    gfx->setTextColor(fishNewHi ? C565(0xc0, 0x84, 0x10) : V2_INK_MUTED);
    setCur(lx + wg + gap, 238);
    printT(rec);
  }
  v2AgainButton(V2_AGAIN_Y);
  v2RepelRowFor(V2_AGAIN_Y);
}

// the whole screen, a frame
static void fsGame(uint32_t now) {
  int hh = sceneHour();
  fsNight = hh < 6 || hh >= 20;
  fsScene();
  fsBlobsLive(now);
  int tx, ty, bx, by;
  fsRodNow(tx, ty, bx, by);
  FishPhase ph = fish.phase;
  bool hud = ph <= FP_REEL || ph == FP_AWAY;
  switch (ph) {
    case FP_AIM: fsGameAim(now); break;
    case FP_FLY: {
      float p = fish.t / FISH_FLY_S;
      p = p > 1 ? 1 : p;
      int fx = (int)(fish.fromX + (fish.floatX - fish.fromX) * p);
      int fy = (int)((FS_TIP_Y + 30) * (1 - p) + FS_SURF * p - 70 * sinf(3.14159f * p));
      fsAngler(tx, ty, bx, by);
      fsLine(tx, ty, fx, fy - 14, -6, 0xF4F4F8);
      fsFloat(fx, fy, fsNight);
      break;
    }
    case FP_WAIT: {
      int fx = (int)fish.floatX, fy = FS_SURF + ((now / 600) % 2 ? 0 : 2);
      bool nib = fish.nibbled && fish.t - fish.lastNibble < 0.15f;
      if (nib) fy += 6;
      if (fish.nibbled && fish.t - fish.lastNibble < 0.5f) {
        fsRing(fx, 12 + (int)((fish.t - fish.lastNibble) * 30), 0xFFFFFF, 2);
        for (int k = 0; k < 2; k++) {
          scRect(fx - 24, FS_SURF - 22 + k * 8, fx - 19, FS_SURF - 19 + k * 8, 0xFFFFFF);
          scRect(fx + 20, FS_SURF - 22 + k * 8, fx + 25, FS_SURF - 19 + k * 8, 0xFFFFFF);
        }
      } else if ((now % 2500) < 700) {
        fsRing(fx, 12 + (int)(now % 2500) / 40, fsNight ? 0x6A8CB8 : 0xE6F6FF, 2);  // a soft ring now and then
      }
      fsAngler(tx, ty, bx, by);
      fsLine(tx, ty, fx, FS_SURF - 18 + (fy - FS_SURF), 14, 0xF4F4F8);
      fsHook(fx, fy + 12, fy + 52);
      fsFloat(fx, fy, fsNight);
      break;
    }
    case FP_BITE: {
      int fx = (int)fish.floatX;
      float w = FISH_CLASS[fish.b[fish.target].cls].window, p = fish.t / w;
      fsRimGlow(fsNight ? 0xFFE27A : 0xFFF3B0, (int)(220 * (1 - p * 0.5f)), 34);
      fsAngler(tx, ty, bx, by);
      fsLine(tx, ty, fx, FS_SURF + 30, 0, 0xFFE27A);
      fsFloat(fx, FS_SURF + 34, fsNight);
      fsRing(fx, 28 + (int)(p * 30), 0xFFFFFF, 4);
      fsRing(fx, 16 + (int)(p * 20), 0xFFFFFF, 2);
      if (p < 0.5f) fsSplash(fx, FS_SURF - 6, 30, 22, 7);
      int r = 70 - (int)(52 * (p > 1 ? 1 : p));  // the strike window, closing
      fsCircle(fx, FS_SURF + 6, r, 0x1B1B25, 6);
      fsCircle(fx, FS_SURF + 6, r, 0xFFFFFF, 2);
      fsBang(fx - 4, FS_SURF - 92);
      fsSay(T(S_FISH_TAP), CX - 6, 396, 0xFFE27A, 4);
      break;
    }
    case FP_REEL: {
      const FishClassDef &c = FISH_CLASS[fish.b[fish.target].cls];
      if (fish.zone == 2) fsRimGlow(0xE8503A, 150 + (int)(80 * sinf(now * 0.019f)), 40);  // pulsing, 3 times a second
      else if (fish.zone == 0) fsRimGlow(0x7CC4EA, 170, 34);
      fsAngler(tx, ty, bx, by);
      fsGameReel(now, tx, ty);
      fsArcDraw(fish.T, c.lo, c.hi, fish.beat < 0.2f);
      if (fish.zone == 2) fsSay(T(S_FISH_SLOWER), CX - 10, 386, 0xFF8A7A, 4);
      else if (fish.zone == 0) fsSay(T(S_FISH_FASTER), CX - 10, 386, 0x9FD6F0, 4);
      else if (fish.t < 2.5f && fish.b[fish.target].cls == FC_SMALL) fsSay(T(S_FISH_QUICK), CX - 10, 396, 0xFFFFFF);
      else if (fish.t < 2.5f && fish.b[fish.target].cls >= FC_HEAVY) fsSay(T(S_FISH_STEADY), CX - 10, 396, 0xFFFFFF);
      if (fishTapX >= 0 && now - fishTapT < 200) fsTapMark(fishTapX, fishTapY);
      break;
    }
    case FP_LANDED: {
      const FishBlob &b = fish.b[fish.target];
      int16_t dex = fish.slot[fish.cast];
      bool giant = fish.target == FISH_GIANT;
      float p = fish.t / FISH_LEAP_S, up = sinf(3.14159f * (p > 1 ? 1 : p) * 0.5f);  // up, then held (FISH_LANDED_S)
      int cx = (int)b.x, feet = FS_SURF + 30 - (int)(up * (giant ? 116 : 104));
      if (giant) {
        fsRays(cx, feet - 60, 18, 0xFFE27A, 110);
        twGlow(cx, feet - 60, 110, 0xFFF3B0, 190);
      } else {
        twGlow(cx, feet - 34, 70, 0xFFF3B0, 170);
      }
      fsAngler(tx, ty, bx, by);
      fsLine(tx, ty, cx - 14, feet - 30, 4, giant ? 0xFFE27A : 0xF4F4F8);
      fsThumb(thumbs.getForm(dex, fish.slotShiny[fish.cast]), cx, feet, 3);
      if (p < 1.4f) {  // its splash, settling as it hangs there
        fsSplash(cx, FS_SURF + 4, giant ? 60 : 40, giant ? 40 : 26, 21);
        fsRing(cx, 50, 0xFFFFFF, 2);
      } else {
        fsRing(cx, 50 + (int)((p - 1.4f) * 30), fsNight ? 0x6A8CB8 : 0xE6F6FF, 2);
      }
      for (int i = 0; i < 4; i++) {
        static const int8_t SP[4][2] = { { -62, -70 }, { 58, -86 }, { 66, -24 }, { -50, -10 } };
        if ((now / 200 + i) % 4 == 0) continue;
        int gx = cx + SP[i][0], gy = feet - 40 + SP[i][1];
        scGridRect(gx, gy - 8, gx + 4, gy + 12, 0xFFFFFF);
        scGridRect(gx - 8, gy, gx + 12, gy + 4, 0xFFFFFF);
      }
      char nm[40];
      snprintf(nm, sizeof(nm), "%s!", dexName(dex));
      if (giant) {
        char cb[40];
        v2SignIn(v2Caps(nm, cb, sizeof(cb)), 44, C565(0xc0, 0x84, 0x10), C565(0xf0, 0xc0, 0x4a));
        fsSay(T(S_FISH_BIG), CX - 20, 392, 0xFFE27A, 3);
      } else {
        fsBoard(nm, 44);
      }
      break;
    }
    case FP_WEIGH: {  // (fsWeighDraw(): the gauge on the left rim filling to its weight)
      fsAngler(FS_TIP_X, FS_TIP_Y);
      fsWeighDraw(fish.slot[fish.cast], fish.slotShiny[fish.cast], fish.slotKg10[fish.cast],
                  FISH_KINDS[fish.b[fish.target].kind].kg10, fish.t);
      break;
    }
    case FP_AWAY: {
      if (fish.why == FW_SLOW) {
        int fx = (int)fish.floatX;
        fsAngler(FS_TIP_X, FS_TIP_Y);
        fsLine(FS_TIP_X, FS_TIP_Y, fx, FS_SURF - 18, 14, 0xF4F4F8);
        fsFloat(fx, FS_SURF + ((now / 300) % 2 ? 0 : 3), fsNight);
        fsRing(fx, 20 + (int)(fish.t * 20), 0xFFFFFF, 2);
        fsSay(T(S_FISH_TOO_SLOW), CX, 150, 0xFF8A7A, 3);
        break;
      }
      fsAngler(tx, ty, bx, by);
      for (int i = 0; i < 40; i++) {  // the line hanging from the tip: snapped and curling, or limp
        int lx = tx + (fish.why == FW_SNAP ? (int)(8 * sinf(i * 0.2f)) + i / 4 : i / 6), ly = ty + i * 2;
        fsSq(lx, ly, 4, i % 2 ? 0x1B1B25 : 0xF4F4F8);
      }
      for (int i = 0; i < 40; i++)
        fsSq(tx + (fish.why == FW_SNAP ? (int)(8 * sinf(i * 0.2f)) + i / 4 : i / 6), ty + i * 2, 2, 0xFFFFFF);
      fsBoard(T(S_FISH_AWAY), 44, C565(0x8a, 0x2a, 0x30), C565(0xb8, 0x40, 0x44));
      if (fish.why == FW_SNAP) fsSay("SNAP!", 300, 146, 0xFF8A7A, 3);
      hud = false;
      break;
    }
    case FP_END:
      fsAngler(FS_TIP_X, FS_TIP_Y);
      fsResultLive();
      return;
  }
  if (fish.soonT > 0 && ph == FP_WAIT) fsSay(T(S_FISH_TOO_SOON), CX, 150, 0xFF8A7A, 3);
  if (hud) fsSlotsLive();
  if (ph == FP_BITE && fish.t < 0.2f) {  // a short shake as it bites: the picture jolts side to side
    int dx = (now / 40) % 2 ? 4 : -4;
    uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
    for (int y = 0; y < V2_W; y++) {
      uint16_t *row = fb + y * V2_W;
      if (dx > 0) memmove(row + dx, row, (V2_W - dx) * 2);
      else memmove(row, row - dx, (V2_W + dx) * 2);
    }
  }
}

// ===================================================================== its medals (FISHMOCK 17..19), PARKED
// Skipped for now (Fedor: the other minigames have no medals); kept here in case they come back. Three, the trainer's (kept across every Pokemon raised), as things in the trophy room in its style
// (tools/fishing_trophies.py writes their art): FULL NET, a gold landing net (all 5 landed in one game); GOLD ANGLER, a
// gold scale (the 300 kg gold line); THE BIG ONE, a golden Lapras (a giant landed)
#include "fishing_trophies_mock.h"
static const char *const FT_NAME[3] = { "FULL NET", "GOLD ANGLER", "THE BIG ONE" };
static const char *const FT_HOW[3] = { "Land all 5 casts in one game of Reel Fishing.",
                                       "Weigh in 300 kg or more in one game of Reel Fishing.",
                                       "Land a giant in Reel Fishing: Gyarados or Lapras." };
static const bool FT_WON[3] = { true, true, false };  // as the mockups show them: two won, the giant still to land
// trArt with Reel Fishing's palette: a thing standing with the middle of its foot at (x, base), s px a pixel
static void ftArt(const TrArt &a, int x, int base, int s, bool won, Rgb sil = TR_SIL, Rgb silD = TR_SIL_D) {
  int x0 = x - a.w * s / 2, y0 = base - a.h * s;
  for (int j = 0; j < a.h; j++)
    for (int i = 0; i < a.w; i++) {
      uint8_t k = a.px[j * a.w + i];
      if (!k) continue;
      int px = x0 + i * s, py = y0 + j * s;
      if (!won) {
        if (FT_PAL_WON[k]) continue;
        scRect(px, py, px + s - 1, py + s - 1, k == 1 ? silD : sil);
      } else {
        scRect(px, py, px + s - 1, py + s - 1, FT_PAL[k]);
      }
    }
}

// 17: the trophy room with Reel Fishing's three: the scale on its bracket left of the badges' case, the mounted fish on
// the wall right of it, the creel on the carpet by the board. A sixth group: FISHING
static void fsMockRoom() {
  uint32_t m0 = pet.medals, t0 = pet.trainerMedals;  // (a trainer some way along, for the picture)
  pet.medals |= MED_LV10 | MED_LV25 | MED_BERRY | MED_BOND;
  pet.trainerMedals |= MED_DEX10 | MED_DEX50 | MED_ZONE_BEACH | MED_ZONE_FOREST | MED_BUDDY;
  v2TrophyRoom();
  pet.medals = m0, pet.trainerMedals = t0;
  scScreen();
  scRR(66, 110, 126, 116, 0x5A3A22);  // the scale's bracket
  scRR(66, 110, 126, 112, TR_WALL_L);
  ftArt(FT_ART_SMALL[2], 380, 108, 2, FT_WON[2]);
  ftArt(FT_ART_SMALL[1], 96, 110, 2, FT_WON[1]);
  ftArt(FT_ART_SMALL[0], 98, 392, 2, FT_WON[0]);
  v2SignIn("MEDALS 11/29", 350, C565(0x6e, 0x1e, 0x2a), C565(0x8a, 0x2a, 0x38));
}

// 18: the FISHING group's shelf, as the other groups' pages: the three on one shelf, their names on plates (gold
// once won), the board with how many, the page buttons (page 6 of 6) and the X
static void fsPaintFishShelf(int) {
  scRR(0, 0, V2_W, 376, TR_WALL);
  for (int x = 24; x < V2_W; x += 60) {
    scRR(x, 0, x + 4, 376, TR_WALL_D);
    scRR(x + 4, 0, x + 8, 376, TR_WALL_L);
  }
  scRR(0, 372, V2_W, 380, 0x3A2414);
  scRR(0, 380, V2_W, V2_W, 0xA83C3C);
  for (int y = 388; y < V2_W; y += 16)
    for (int x = ((y / 16) % 2) * 16; x < V2_W; x += 32) scGridRect(x, y, x + 8, y + 8, 0x882C2C);
  scRR(0, 380, V2_W, 384, TR_GOLD_D);
  const int sy = 250, x0 = 52, x1 = 414;  // the shelf
  scRR(x0, sy, x1, sy + 8, 0x5A3A22);
  scRR(x0, sy, x1, sy + 2, TR_WALL_L);
  scRR(x0 + 8, sy + 8, x0 + 16, sy + 16, 0x3A2414);
  scRR(x1 - 16, sy + 8, x1 - 8, sy + 16, 0x3A2414);
}
static void fsMockShelf() {
  v2Cached(0x3E700000, fsPaintFishShelf, 0);
  scScreen();
  static const int16_t X[3] = { 118, 233, 348 };
  for (int k = 0; k < 3; k++) ftArt(FT_ART_BIG[k], X[k], 250, 2, FT_WON[k]);
  TrPlate pl[3];
  int pw = 0, ph = 0;
  for (int k = 0; k < 3; k++) {
    v2PlateFit(FT_NAME[k], 104, pl[k]);
    if (pl[k].w > pw) pw = pl[k].w;
    if (pl[k].h > ph) ph = pl[k].h;
  }
  for (int k = 0; k < 3; k++) v2NamePlate(pl[k], X[k], 272, pw, ph, FT_WON[k]);
  v2Sign("FISHING 2/3");
  drawPageArrows(5, 6, V2_PAGE_ARROW_Y);
  v2PageOf(6, 5, 390);
  v2CloseX();
}

// 19: a medal's card, as every medal has: THE BIG ONE, won: on the plinth under the spotlight, shining, its name on a
// gold board, how to win it and Earned!
static void fsMockCard() {
  const int m = 2;
  v2Cached(0x3F000000, v2PaintTrophyBack, 0);
  scScreen();
  scGradeTod = 0;
  scRR(173, 262, 293, 290, 0x5A3A22);  // the plinth
  scRR(173, 262, 293, 266, TR_WALL_L);
  const TrArt &a = FT_ART_BIG[m];
  ftArt(a, CX, 262, 3, true);
  int x0 = CX - a.w * 3 / 2, x1 = x0 + a.w * 3, y0 = 262 - a.h * 3;  // it shines: three sparkles round it
  const int16_t sp[3][3] = { { (int16_t)(x0 - 12), (int16_t)(y0 + 18), 3 }, { (int16_t)(x1 + 9), (int16_t)(y0 + 6), 3 },
                             { (int16_t)(x1 + 15), (int16_t)(y0 + a.h * 2), 2 } };
  for (auto &q : sp) {
    int x = q[0], y = q[1], r = q[2] * 3;
    scRR(x - 1, y - r - 1, x + 2, y + r + 2, 0xFFFFFF);
    scRR(x - r - 1, y - 1, x + r + 2, y + 2, 0xFFFFFF);
    scRR(x - 2, y - 2, x + 3, y + 3, 0xFFF6C8);
  }
  v2SignIn(FT_NAME[m], V2_BOARD_Y, C565(0xc0, 0x84, 0x10), C565(0xf0, 0xc0, 0x4a));
  v2Panel(72, 270, 394, 374, 10);
  int n = drawWrappedAt(FT_HOW[m], 2, 306, CX, 0, 18, false), y = 270 + (105 - (n * 18 + 8 + 16)) / 2;
  gfx->setTextColor(UI_INK);
  drawWrappedAt(FT_HOW[m], 2, 306, CX, y, 18, true);
  y += n * 18 + 8;
  gfx->setTextColor(C565(0x3c, 0x8a, 0x4c));
  setCur(centerX(T(S_MEDAL_GOT), 2), y);
  printT(T(S_MEDAL_GOT));
  drawPageArrows(2, 3, V2_PAGE_ARROW_Y);
  v2CloseX();
}

// the mockups are for a computer's console only: on a board, nothing calls them, so they (and the parked medals' art)
// aren't in its firmware
static void fishMockRender() {
#ifdef BOARD_HOST
  switch (fsMock) {
    case 1: fsMockGames(); break;
    case 2: fsMockHow(); break;
    case 3: fsMockCast(); break;
    case 4: fsMockWait(); break;
    case 5: fsMockNibble(); break;
    case 6: fsMockBite(); break;
    case 7: fsMockReel(); break;
    case 8: fsMockStrain(); break;
    case 9: fsMockSlack(); break;
    case 10: fsMockCaught(); break;
    case 11: fsMockWeigh(); break;
    case 12: fsMockAway(); break;
    case 13: fsMockRare(); break;
    case 14: fsMockResult(); break;
    case 15: fsMockNight(); break;
    case 16: fsMockReelBig(); break;
    case 17: fsMockRoom(); break;
    case 18: fsMockShelf(); break;
    case 19: fsMockCard(); break;
    case 20: fsMockWeighHang(); break;
    case 21: fsMockWeighGauge(); break;
    case 22: fsMockWeighPhoto(); break;
  }
#endif
}
