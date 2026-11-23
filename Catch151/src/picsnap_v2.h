// V2: Pic Snap, drawn: the game from picsnap.h's state (psDraw()). Through the camera: a place on Knomi Island seen
// close up, a wild Pokemon in it, the viewfinder's marks over it; the photo, its print and score; the roll at the end.
// scene_v2.h includes this after berrymatch_v2.h; it paints with scene_v2.h's painters (scRect, scSprite, scHills...)
// and fishing_v2.h's (fsSay, fsBlend, fsSq).
//
// How the focus is drawn, within the Knomi's time and memory (a frame there takes 75-120 ms, all of it PSRAM):
//  * Each place is three planes: the far things (the sky, the hills, the trees behind), the near things at the
//    picture's edge (big blades of grass, a starfish, a tombstone), and the Pokemon with what's at its feet.
//  * The far and the near planes are painted once a place, at the Knomi's own size (233 x 233, each pixel the
//    canvas's 2 x 2 block, which is all the Knomi shows) with PS_PAD pixels more round them for the camera to drift
//    over, then blurred and put together at the lens's PS_LENS_STEPS steps: at 0 the near ones sharp and the far
//    ones PS_BLUR_MAX px soft, at 1 the other way round. A frame copies the step nearest the lens, doubled.
//  * The Pokemon is blurred each frame, by how far the lens is from it (psMonBlur()), in a little picture of its
//    own just big enough for it, and laid over that.
// The making ready is done a few steps a frame (psPrepWork()), while the last photo's print is up.

#define PS_K 233                     // the Knomi's pixels across the canvas
#define PS_PAD 32                    // the camera's room each side (the Knomi's px): PS_AIM_MAX + the widest drift
#define PS_BW (PS_K + 2 * PS_PAD)    // a plane's width and height (the Knomi's px)
#define PS_M (2 * PS_PAD)            // ...the margin round the screen it covers (the canvas's px)
#define PS_AIM_MAX 20                // the camera aims at the Pokemon, at most this far off the place's middle (px)
#define PS_STRIP 32                  // the planes are painted at the canvas's size this many rows at a time
#define PS_SUB_W 200                 // the Pokemon's little picture, at most (the Knomi's px)
#define PS_SUB_H 224
#define PS_OCC_W 128                 // what's at its feet, at most (the Knomi's px)
#define PS_OCC_H 72
#define PS_THUMB 46                  // a photo in the roll at the end (the Knomi's px)
#define PS_CREAM 0xF4ECD6            // a print's border

// ----- the buffers: made when the game opens, gone when it closes -----
static uint16_t *psLay[PS_LENS_STEPS];  // the place at each lens step, far and near together (PS_BW x PS_BW)
static int psLays = 0;                  // how many there's room for (fewer: the steps between share one)
static uint16_t *psFar = nullptr;       // the far plane, sharp
static uint16_t *psNear = nullptr;      // the near plane, sharp, where psNearA says it's there
static uint8_t *psNearA = nullptr;
static uint8_t *psWork[2] = { nullptr, nullptr };  // one channel being blurred, and the near plane's cover
static uint8_t *psRing = nullptr;       // a blur's last rows (psBlur())
static uint16_t *psStripBuf = nullptr;  // a strip of the canvas-sized painting
// A still screen kept whole: the print (the photo dimmed over the screen, the print, its word, score and meters), or
// IT GOT AWAY! Drawn once; each frame after, it's put back and only the line under it drawn. Redrawn each frame, the
// whole screen dimmed pixel by pixel, it took 91 ms a frame on the Waveshare 2.1 (2026-11-23), and the next place's
// making ready got that much less of each second. Only if there's room once the lens steps have theirs (none: drawn
// each frame, as before)
static uint16_t *psStill = nullptr;
static int32_t psStillKey = -1;
static uint32_t psPhotoN = 0;  // photos kept (psKeepPhoto(), IT GOT AWAY!'s too): a still is the photo's
static uint16_t *psPhoto = nullptr;     // the photo just taken (PS_K x PS_K)
static uint16_t *psThumbs = nullptr;    // the roll: each photo small (PS_SHOTS x PS_THUMB x PS_THUMB)
static uint8_t *psSub = nullptr;        // the Pokemon's little picture: four planes, its colours times its cover
static uint16_t *psOcc = nullptr;       // what's at its feet (PS_OCC_W x PS_OCC_H, V2_STRIP_KEY where there's nothing)
static PmdMon psMon;

static void psFreeAll() {
  for (int i = 0; i < PS_LENS_STEPS; i++) {
    free(psLay[i]);
    psLay[i] = nullptr;
  }
  psLays = 0;
  free(psFar), free(psNear), free(psNearA), free(psWork[0]), free(psWork[1]), free(psRing), free(psStripBuf);
  free(psPhoto), free(psThumbs), free(psSub), free(psOcc), free(psStill);
  psStill = nullptr;
  psStillKey = -1;
  psFar = psNear = nullptr;
  psNearA = psWork[0] = psWork[1] = psRing = nullptr;
  psStripBuf = psPhoto = psThumbs = psOcc = nullptr;
  psSub = nullptr;
  psMon.unload();
}
static bool psAllocAll() {
  const size_t P = (size_t)PS_BW * PS_BW;
  psFar = (uint16_t *)ps_malloc(P * 2);
  psNear = (uint16_t *)ps_malloc(P * 2);
  psNearA = (uint8_t *)ps_malloc(P);
  psWork[0] = (uint8_t *)ps_malloc(P);
  psWork[1] = (uint8_t *)ps_malloc(P);
  psRing = (uint8_t *)ps_malloc((size_t)(2 * PS_BLUR_MON_MAX + 2) * PS_BW);
  psStripBuf = (uint16_t *)ps_malloc((size_t)PS_STRIP * 2 * PS_BW * 2);
  psPhoto = (uint16_t *)ps_malloc((size_t)PS_K * PS_K * 2);
  psThumbs = (uint16_t *)ps_malloc((size_t)PS_SHOTS * PS_THUMB * PS_THUMB * 2);
  psSub = (uint8_t *)ps_malloc((size_t)4 * PS_SUB_W * PS_SUB_H);
  psOcc = (uint16_t *)ps_malloc((size_t)PS_OCC_W * PS_OCC_H * 2);
  if (!psFar || !psNear || !psNearA || !psWork[0] || !psWork[1] || !psRing || !psStripBuf || !psPhoto || !psThumbs || !psSub ||
      !psOcc) {
    psFreeAll();
    return false;
  }
  for (psLays = 0; psLays < PS_LENS_STEPS; psLays++)  // as many lens steps as there's room for (all of them, mostly)
    if (!(psLay[psLays] = (uint16_t *)ps_malloc(P * 2))) break;
  if (!psLays) {
    psFreeAll();
    return false;
  }
#ifdef ESP_PLATFORM
  const bool room = heap_caps_get_free_size(MALLOC_CAP_SPIRAM) > (size_t)V2_W * V2_W * 2 + 1024 * 1024;  // (1 MB left)
#else
  const bool room = true;  // (a computer)
#endif
  if (room) {
#ifdef BOARD_WS21
    psStill = ws21FrameAlloc();  // (on whole lines, as the frames: it goes back by DMA, restoreFrom())
#else
    psStill = (uint16_t *)ps_malloc((size_t)V2_W * V2_W * 2);
#endif
  }
  return true;
}
// the still screen kept for key, put back (false: not kept)
static bool psStillBack(int32_t key) {
  if (!psStill || psStillKey != key) return false;
#ifdef BOARD_WS21
  gfx->restoreFrom(psStill);
#else
  memcpy(gfx->getFramebuffer(), psStill, (size_t)V2_W * V2_W * 2);
#endif
  return true;
}
// the screen as drawn so far, kept as key's still
static void psStillKeep(int32_t key) {
  if (!psStill) return;
  memcpy(psStill, gfx->getFramebuffer(), (size_t)V2_W * V2_W * 2);
#ifdef BOARD_WS21
  ws21Writeback();  // (out to PSRAM, where the DMA reads it)
#endif
  psStillKey = key;
}
// the picture made for a lens step (with fewer than all of them, the nearest there is)
static int psLaySlot(int step) { return psLays >= PS_LENS_STEPS ? step : (step * (psLays - 1) + PS_BLUR_MAX / 2) / PS_BLUR_MAX; }

// ----- colours, a channel at a time (8 bits, from and back to 565) -----
static inline uint8_t psCh(uint16_t c, int ch) {
  if (ch == 0) { int r = c >> 11; return (uint8_t)(r << 3 | r >> 2); }
  if (ch == 1) { int g = (c >> 5) & 63; return (uint8_t)(g << 2 | g >> 4); }
  int b = c & 31;
  return (uint8_t)(b << 3 | b >> 2);
}
static inline uint16_t psSetCh(uint16_t c, int ch, int v) {
  v = v < 0 ? 0 : v > 255 ? 255 : v;
  if (ch == 0) return (uint16_t)((c & 0x07FF) | (v >> 3) << 11);
  if (ch == 1) return (uint16_t)((c & 0xF81F) | (v >> 2) << 5);
  return (uint16_t)((c & 0xFFE0) | v >> 3);
}

// A blur of one channel (w x h, r px): a box twice across and twice down, close to a Gaussian, each pass the same
// cost whatever r is (a running sum). Down the rows a running sum per column, row after row (the Knomi's PSRAM wants
// its reads in order), the rows it still needs kept in psRing
static void psBoxRow(uint8_t *p, int w, int r, uint8_t *tmp) {
  memcpy(tmp, p, w);
  int n = 2 * r + 1, sum = 0;
  uint32_t inv = 65536u / n;
  for (int k = -r; k <= r; k++) sum += tmp[k < 0 ? 0 : k >= w ? w - 1 : k];
  for (int x = 0; x < w; x++) {
    p[x] = (uint8_t)((sum * inv + 32768) >> 16);
    int add = x + r + 1, sub = x - r;
    sum += tmp[add >= w ? w - 1 : add] - tmp[sub < 0 ? 0 : sub];
  }
}
static void psBoxCols(uint8_t *p, int w, int h, int r, uint8_t *ring, uint16_t *sum) {
  int n = 2 * r + 1, keep = r + 1;
  uint32_t inv = 65536u / n;
  for (int x = 0; x < w; x++) sum[x] = 0;
  for (int k = -r; k <= r; k++) {
    const uint8_t *row = p + (k < 0 ? 0 : k >= h ? h - 1 : k) * w;
    for (int x = 0; x < w; x++) sum[x] += row[x];
  }
  for (int y = 0; y < h; y++) {
    uint8_t *row = p + y * w;
    memcpy(ring + (y % keep) * w, row, w);  // this row as it was, for the rows below that still count it
    const uint8_t *add = p + (y + r + 1 >= h ? h - 1 : y + r + 1) * w;
    int s = y - r;
    const uint8_t *sub = s < 0 ? ring : ring + (s % keep) * w;  // (row 0 as it was sits at the ring's start until y = r)
    if (s < 0) sub = ring + 0;
    for (int x = 0; x < w; x++) {
      row[x] = (uint8_t)((sum[x] * inv + 32768) >> 16);
      sum[x] += add[x] - sub[x];
    }
  }
}
static uint16_t psSums[PS_BW > PS_SUB_W ? PS_BW : PS_SUB_W];
static uint8_t psTmpRow[PS_BW > PS_SUB_W ? PS_BW : PS_SUB_W];
static void psBlur(uint8_t *p, int w, int h, int r) {
  if (r <= 0) return;
  for (int pass = 0; pass < 2; pass++)
    for (int y = 0; y < h; y++) psBoxRow(p + y * w, w, r, psTmpRow);
  for (int pass = 0; pass < 2; pass++) psBoxCols(p, w, h, r, psRing, psSums);
}

// ----- how out of focus looks: the blur in the place's own colours, dithered in the checker the scenes use between
// two colours, in their 4 px cells (Fedor picked it from four mockups, 2026-11-22: "I want the art style to match the
// rest of the game"; a smooth blur, the blur in cells, and blocks were the others) -----
static int8_t psPrepFor = -1;  // the place being made ready (or ready)
// the place's colours (its far and near planes, sharp), for the dither
#define PS_PAL_MAX 96
static uint16_t psPal[PS_PAL_MAX];
static uint8_t psPalC[PS_PAL_MAX][3];
static int psPalN = 0, psPalPlace = 0;
static void psPalAdd(uint16_t c) {
  for (int i = psPalN - 1; i >= 0; i--)
    if (psPal[i] == c) return;
  if (psPalN >= PS_PAL_MAX) return;
  psPal[psPalN] = c;
  for (int ch = 0; ch < 3; ch++) psPalC[psPalN][ch] = psCh(c, ch);
  psPalN++;
}
static void psPalCollect() {
  psPalN = 0;
  uint16_t last = 0xFFFF ^ 1;
  for (int i = 0; i < PS_BW * PS_BW; i++) {
    if (psFar[i] != last) psPalAdd(last = psFar[i]);
    if (psNearA[i] && psNear[i] != last) psPalAdd(last = psNear[i]);
  }
  psPalPlace = psPalN;
  Serial.printf("picsnap: place %d, %d colours\n", psPrepFor, psPalN);
}
// the colour a 2 x 2 cell at (cx, cy) gets for (r, g, b): of the two nearest of the palette's first n, the one the
// checker gives it, by how far it lies from the nearest toward the other
static uint16_t psDitherPick(int r, int g, int b, int cx, int cy, int n) {
  int i1 = 0, i2 = 0;
  long d1 = 1L << 30, d2 = 1L << 30;
  for (int i = 0; i < n; i++) {
    int dr = r - psPalC[i][0], dg = g - psPalC[i][1], db = b - psPalC[i][2];
    long d = 2L * dr * dr + 4L * dg * dg + 3L * db * db;
    if (d < d1) d2 = d1, i2 = i1, d1 = d, i1 = i;
    else if (d < d2) d2 = d, i2 = i;
  }
  if (n < 2 || d1 == 0) return psPal[i1];
  int ar = psPalC[i2][0] - psPalC[i1][0], ag = psPalC[i2][1] - psPalC[i1][1], ab = psPalC[i2][2] - psPalC[i1][2];
  long den = 2L * ar * ar + 4L * ag * ag + 3L * ab * ab;
  if (den <= 0) return psPal[i1];
  float t = (2.0f * (r - psPalC[i1][0]) * ar + 4.0f * (g - psPalC[i1][1]) * ag + 3.0f * (b - psPalC[i1][2]) * ab) / den;
  static const float BAYER[2][2] = { { 0.125f, 0.625f }, { 0.875f, 0.375f } };
  return t > BAYER[cy & 1][cx & 1] ? psPal[i2] : psPal[i1];
}
// a picture (w x h, stride st) in 2 x 2 cells from (ox, oy): each its average, then the palette's, checkered
static void psDitherCells(uint16_t *p, int w, int h, int st, int ox, int oy, int palN) {
  for (int y = (oy & 1); y + 1 < h; y += 2)
    for (int x = (ox & 1); x + 1 < w; x += 2) {
      uint16_t *q = p + y * st + x;
      int c[3];
      for (int ch = 0; ch < 3; ch++) c[ch] = (psCh(q[0], ch) + psCh(q[1], ch) + psCh(q[st], ch) + psCh(q[st + 1], ch) + 2) / 4;
      q[0] = q[1] = q[st] = q[st + 1] = psDitherPick(c[0], c[1], c[2], (x - ox) >> 1, (y - oy) >> 1, palN);
    }
}

// ----- painting a place -----
// The painters draw in the canvas's px, the camera at rest, over the screen and PS_M more each side: the strip being
// painted is psStripBuf (scene_v2.h's scFb, scDX, scDY point a painter at it)
static void psOval(const ScEll &e, Rgb col, Rgb lit = SC_NONE, Rgb under = SC_NONE) {  // scOval, past the screen too
  int y0 = scFl4(e.y0) * 4, x0 = scFl4(e.x0) * 4;
  int lo = scFl4(-scDX) * 4, hiX = scW - scDX, top = -scDY, bot = scH - scDY;
  for (int cy = y0 < top ? scFl4(top) * 4 : y0; cy < e.y1 + 4 && cy < bot; cy += 4)
    for (int cx = x0 < lo ? lo : x0; cx < e.x1 + 4 && cx < hiX; cx += 4) {
      if (!scInEll(cx + 2, cy + 2, e)) continue;
      Rgb c = col;
      if (lit != SC_NONE && !scInEll(cx + 2, cy - 2, e)) c = lit;
      else if (under != SC_NONE && !scInEll(cx + 2, cy + 10, e)) c = under;
      scCell(cx, cy, c);
    }
}
static void psBands(const Rgb *cols, const int16_t *edges, int n) { scBands(cols, edges, n, -PS_M, V2_W + PS_M); }
static void psSky3(Rgb top, Rgb bot, int16_t a, int16_t b, int16_t c) {
  Rgb col[3] = { top, scMix(top, bot, .5), bot };
  const int16_t e[4] = { (int16_t)-PS_M, a, b, c };
  psBands(col, e, 3);
}
// a sprite of the scene's, scale s, its colours as keys
static void psSpr(const char *const *rows, int nr, int x, int y, int s, const ScKey *k, int nk) { scSprite(rows, nr, x, y, s, k, nk); }
// tall grass, s px a cell, in two greens
static void psTallGrass(int x, int y, int s, Rgb dark, Rgb mid) {
  const ScKey k[2] = { { 'G', dark }, { 'g', mid } };
  psSpr(SPR_V2_TALLGRASS, 5, x, y, s, k, 2);
}
// a tuft of grass blades standing on y, from x0 to x1: each blade a column of 4 px cells, narrowing to its tip, its
// height h give or take, leaning a little; the ones behind darker
static void psTuft(int x0, int x1, int y, int h, Rgb lit, Rgb dark, int seed) {
  for (int row = 0; row < 2; row++)
    for (int x = x0 + row * 6, i = seed; x < x1; x += 12, i++) {
      int bh = h * (60 + (i * 37) % 40) / 100, lean = (i % 3) - 1;
      Rgb c = row ? lit : dark;
      for (int k = 0; k < bh; k += 4) {
        int w = k < bh * 2 / 3 ? 8 : 4, dx = lean * (k * k) / (bh * 8);
        scRect(x + dx, y - k - 4, x + dx + w - 1, y - k - 1, c);
      }
    }
}
// a big flower: five petals round a middle, s px a petal
static void psFlower(int cx, int cy, int s, Rgb petal, Rgb mid) {
  scRect(cx - s / 2, cy - s - s / 2, cx + s / 2 - 1, cy - s / 2 - 1, petal);
  scRect(cx - s - s / 2, cy - s / 2, cx - s / 2 - 1, cy + s / 2 - 1, petal);
  scRect(cx + s / 2, cy - s / 2, cx + s + s / 2 - 1, cy + s / 2 - 1, petal);
  scRect(cx - s, cy + s / 2, cx - 1, cy + s + s / 2 - 1, petal);
  scRect(cx, cy + s / 2, cx + s - 1, cy + s + s / 2 - 1, petal);
  scRect(cx - s / 2, cy - s / 2, cx + s / 2 - 1, cy + s / 2 - 1, mid);
}

// PS_GRASS: down in the Forest's tall grass, the pines far off in the sun
static void psGrass(int part) {
  ScPal p = scForestPal();
  if (part == 0) {
    psSky3(p.top, p.bot, 90, 150, 230);
    scSun(p, false, 372, 76);
    scCloud(p, 70, 60, 5);
    for (int x = -PS_M - 20, i = 0; x < V2_W + PS_M; x += 40, i++) scRole(SPR_V2_PINE, 12, x, 150 + (i * 37 % 26), 6, p, p.far);
    psOval(scE(-300, 214, 300, 700), p.far, scMix(p.far, 0xFFFFFF, p.lit));
    psOval(scE(160, 204, 800, 700), p.far, scMix(p.far, 0xFFFFFF, p.lit));
    scRole(SPR_V2_PINE, 12, -40, 136, 10, p, p.mid);
    scRole(SPR_V2_PINE, 12, 392, 128, 10, p, p.mid);
    psOval(scE(-500, 262, 966, 1200), p.mid, scMix(p.mid, 0xFFFFFF, p.lit));
    psOval(scE(-700, 296, 1166, 1400), p.near, scMix(p.near, 0xFFFFFF, p.lit));
    static const int16_t tufts[6][2] = { { 60, 270 }, { 140, 300 }, { 320, 296 }, { 400, 268 }, { 10, 320 }, { 450, 318 } };
    for (auto &t : tufts) psTallGrass(t[0], t[1], 4, p.dark, scMix(p.near, p.dark, .35));
    static const int16_t pinks[4][2] = { { 104, 320 }, { 364, 328 }, { 180, 344 }, { 292, 352 } };
    for (auto &f : pinks) scCell(f[0], f[1], SC_FOREST_PINK);
    psOval(scE(-700, 340, 1166, 1400), scMix(p.near, p.dark, .2));
  } else if (part == 1) {  // near: huge blades along the bottom, a flower at the left
    psTallGrass(-90, 360, 22, p.dark, scMix(p.near, p.dark, .35));
    psTallGrass(250, 376, 24, p.dark, scMix(p.near, p.dark, .35));
    psTallGrass(60, 430, 20, scMix(p.dark, 0x000000, .2), p.dark);
    psFlower(48, 330, 22, SC_FOREST_PINK, 0xFFE48A);
  } else {  // at its feet: a tuft of grass
    psTuft(-84, 84, 12, 48, scMix(p.near, 0xFFFFFF, .1), scMix(p.near, p.dark, .4), 3);
    scRect(-80, 8, 79, 15, scMix(p.near, p.dark, .4));
  }
}

// PS_WAVES: out on the Beach's sea, the swell in front
static void psWaves(int part) {
  ScPal p = scBeachPal();
  const Rgb sea = SC_SEA, deep = scMix(SC_SEA, 0x1F4A78, .35), hi = SC_SEA_HI;
  if (part == 0) {
    psSky3(p.top, p.bot, 70, 130, 186);
    scSun(p, false, 108, 92);
    scCloud(p, 250, 54, 6);
    scCloud(p, 400, 124, 4);
    Rgb isle = scMix(0x86C58E, p.bot, .4);  // Knomi Island far off, faded toward the sky
    psOval(scE(300, 168, 560, 240), isle, scMix(isle, 0xFFFFFF, .2));
    ScKey palm[5];
    scPalmKeys(palm, p);
    for (auto &k : palm) k.c = scMix(k.c, p.bot, .4);
    psSpr(SPR_V2_PALM, 12, 380, 120, 4, palm, 5);
    const Rgb bands[4] = { scMix(sea, 0xFFFFFF, .2), sea, scMix(sea, deep, .5), deep };
    const int16_t e[5] = { 186, 230, 300, 380, (int16_t)(V2_W + PS_M) };
    psBands(bands, e, 4);
    scRect(-PS_M, 186, V2_W + PS_M, 189, scMix(sea, 0xFFFFFF, .5));  // the horizon
    for (int i = 0; i < 40; i++) {  // glints, longer and wider as they come near
      int y = 196 + (i * 53 % 280), x = -PS_M + (i * 137 % (V2_W + 2 * PS_M)), w = 8 + y / 20;
      scRect(x & ~3, y & ~3, (x & ~3) + w, (y & ~3) + 3 + (y > 330 ? 4 : 0), y < 260 ? hi : scMix(hi, sea, .35));
    }
  } else if (part == 1) {  // near: a swell's crest across the bottom, its foam
    psOval(scE(-200, 392, 700, 760), deep, scMix(deep, hi, .4));
    psOval(scE(-300, 420, 300, 700), scMix(deep, 0x123A60, .3), hi);
    for (int x = -PS_M; x < V2_W + PS_M; x += 48) scRect(x, 404 + (x * 7 % 12), x + 28, 411 + (x * 7 % 12), hi);
  } else {  // at its feet: the water's ring round it
    scRect(-60, -2, 59, 3, hi);
    scRect(-44, 6, 43, 9, scMix(hi, sea, .4));
    scRect(-76, 14, -52, 17, scMix(hi, sea, .5));
    scRect(52, 14, 76, 17, scMix(hi, sea, .5));
  }
}

// PS_GRAVES: the Mansion's graveyard under the moon, tombstones close by
static void psGraves(int part) {
  ScPal p = scMansionPal();
  if (part == 0) {
    psSky3(p.top, p.bot, 110, 190, 260);
    for (auto &st : SC_STARS) scCell(st[0] / 4 * 4, st[1] / 4 * 4, 0xFFFFFF);
    scDisc(356, 92, 40, p.sun, 0, 0);  // the moon, big and close
    scDisc(356, 92, 34, scMix(p.top, p.bot, .3), 20, -14);
    scCloud(p, 40, 120, 6);
    psOval(scE(-200, 236, 680, 700), p.far, scMix(p.far, 0xFFFFFF, p.lit));
    scMansion(233 - 58 * 2, 236 - 34 * 4 + 12);
    const ScKey bare[1] = { { '1', p.dark } };
    psSpr(SPR_V2_BARE, 11, -10, 140, 10, bare, 1);
    psOval(scE(-500, 300, 966, 1200), p.mid, scMix(p.mid, 0xFFFFFF, p.lit));
    const ScKey stone[2] = { { 'o', 0x0D0B20 }, { 's', 0x5A5F7C } };
    psSpr(SPR_V2_TOMB, 7, 120, 290, 5, stone, 2);
    psSpr(SPR_V2_CROSS, 7, 318, 286, 5, stone, 2);
    psOval(scE(-700, 344, 1166, 1400), p.near, scMix(p.near, 0xFFFFFF, p.lit));
    static const int16_t g[3][2] = { { 60, 340 }, { 380, 344 }, { 230, 360 } };
    for (auto &t : g) psTallGrass(t[0], t[1], 4, p.dark, scMix(p.near, p.dark, .35));
  } else if (part == 1) {  // near: a tombstone at the left and a cross at the right, big
    const ScKey stone[2] = { { 'o', 0x262240 }, { 's', 0x7A7FA0 } };
    psSpr(SPR_V2_TOMB, 7, -30, 290, 22, stone, 2);
    psSpr(SPR_V2_CROSS, 7, 352, 318, 20, stone, 2);
  }
}

// PS_PINES: under the Forest's pines, red mushrooms close by; it peeks over a fallen log
static void psLog(int x0, int x1, int y0, int y1) {  // a log lying across, its cut end at the right
  const Rgb bark = 0x7E5530, lit = 0xB07A45, dark = 0x5A3A20, ring = 0xF8D990, ringD = 0xE0B860, ink = 0x3A2414;
  int ry = (y1 - y0) / 2;
  psOval(scE(x0, y0, x0 + 2 * ry, y1), ink);  // its far end, round
  scRect(x0 + ry, y0, x1 - 14, y1, ink);
  psOval(scE(x0 + 4, y0 + 4, x0 + 2 * ry - 4, y1 - 4), bark);
  scRect(x0 + ry, y0 + 4, x1 - 14, y1 - 4, bark);
  scRect(x0 + ry, y0 + 4, x1 - 14, y0 + 11, lit);  // the light along its top, the shade under it
  scRect(x0 + ry, y1 - 11, x1 - 14, y1 - 4, dark);
  for (int x = x0 + 32; x < x1 - 48; x += 40) scRect(x, y0 + 16 + (x * 7 & 7), x + 19, y0 + 19 + (x * 7 & 7), dark);
  scRect(x0 + 60, y0 - 8, x0 + 67, y0 + 3, 0x58B868), scRect(x0 + 68, y0 - 4, x0 + 75, y0 + 3, 0x3C8A4C);  // a sprout
  int cx = x1 - 14, cy = (y0 + y1) / 2;
  psOval(scE(cx - 14, cy - ry, cx + 14, cy + ry), ink);
  psOval(scE(cx - 10, cy - ry + 4, cx + 10, cy + ry - 4), ring);
  psOval(scE(cx - 4, cy - 6, cx + 4, cy + 6), ringD);
}
static void psPines(int part) {
  ScPal p = scForestPal();
  const ScKey mush[2] = { { 'r', 0xE8503A }, { 'w', 0xFFF3DC } };
  if (part == 0) {
    psSky3(p.top, p.bot, 50, 100, 160);
    for (int x = -PS_M - 10, i = 0; x < V2_W + PS_M; x += 34, i++) scRole(SPR_V2_PINE, 12, x, 104 + (i * 29 % 22), 5, p, p.far);
    psOval(scE(-300, 166, 800, 700), p.far, scMix(p.far, 0xFFFFFF, p.lit));
    scRole(SPR_V2_PINE, 12, -96, 28, 16, p, p.mid);  // two big pines, the near side of the clearing
    scRole(SPR_V2_PINE, 12, 380, 12, 16, p, p.mid);
    psOval(scE(-600, 244, 1066, 1400), p.mid, scMix(p.mid, 0xFFFFFF, p.lit));
    psOval(scE(-700, 296, 1166, 1400), p.near, scMix(p.near, 0xFFFFFF, p.lit));
    static const int16_t m[4][2] = { { 70, 262 }, { 388, 270 }, { 140, 330 }, { 330, 336 } };
    for (auto &q : m) psSpr(SPR_V2_MUSH, 3, q[0], q[1], 4, mush, 2);
    static const int16_t t[4][2] = { { 20, 300 }, { 420, 306 }, { 96, 356 }, { 360, 362 } };
    for (auto &q : t) psTallGrass(q[0], q[1], 4, p.dark, scMix(p.near, p.dark, .35));
  } else if (part == 1) {  // near: big mushrooms at the bottom, a pine's bough over the top left
    psSpr(SPR_V2_MUSH, 3, -40, 374, 22, mush, 2);
    psSpr(SPR_V2_MUSH, 3, 344, 396, 20, mush, 2);
    psSpr(SPR_V2_MUSH, 3, 100, 440, 14, mush, 2);
    scRole(SPR_V2_PINE, 12, -170, -150, 22, p, p.dark);
  } else {  // at its feet: a fallen log
    psLog(-112, 112, -24, 24);
  }
}

// PS_SHORE: the Beach's sand at the water's edge, a starfish and a shell close by; it comes up from behind a mound
static void psShore(int part) {
  ScPal p = scBeachPal();
  const Rgb sea = SC_SEA, hi = SC_SEA_HI;
  const ScKey shell[3] = { { 'p', 0xF08AA4 }, { 'w', 0xFFF3DC }, { 'o', 0xF5863D } };
  if (part == 0) {
    psSky3(p.top, p.bot, 70, 130, 186);
    scSun(p, false, 110, 74);
    scCloud(p, 250, 60, 5);
    const Rgb bands[3] = { scMix(sea, 0xFFFFFF, .25), sea, scMix(sea, 0x1F4A78, .2) };
    const int16_t e[4] = { 186, 206, 230, 262 };
    psBands(bands, e, 3);
    scRect(-PS_M, 186, V2_W + PS_M, 189, scMix(sea, 0xFFFFFF, .5));
    for (int i = 0; i < 16; i++) {
      int y = 196 + (i * 23 % 48), x = -PS_M + (i * 151 % (V2_W + 2 * PS_M));
      scRect(x & ~3, y & ~3, (x & ~3) + 12 + y / 30, (y & ~3) + 3, hi);
    }
    ScKey palm[5];
    scPalmKeys(palm, p);
    psSpr(SPR_V2_PALM, 12, 352, 86, 11, palm, 5);
    psOval(scE(260, 214, 800, 600), p.mid, scMix(p.mid, 0xFFFFFF, p.lit));
    psOval(scE(-600, 240, 1066, 1400), scMix(p.near, sea, .25));  // the wet sand
    for (int x = -PS_M; x < V2_W + PS_M; x += 40) scRect(x, 244 + (x * 3 & 7), x + 24, 247 + (x * 3 & 7), hi);  // foam
    psOval(scE(-700, 262, 1166, 1400), p.near, scMix(p.near, 0xFFFFFF, .3));
    const ScKey crab[3] = { { 'r', 0xE86F5F }, { 'w', 0xFFFFFF }, { 'k', 0x1B1B25 } };
    psSpr(SPR_V2_CRAB, 6, 60, 290, 3, crab, 3);
    psSpr(SPR_V2_SHELL, 3, 388, 300, 3, shell, 3);
    psSpr(SPR_V2_STARFISH, 4, 150, 322, 3, shell, 3);
  } else if (part == 1) {  // near: a big starfish and a shell on a dune's edge
    psOval(scE(-300, 424, 760, 800), scMix(p.near, p.dark, .15), scMix(p.near, 0xFFFFFF, .3));
    psSpr(SPR_V2_STARFISH, 4, -36, 346, 24, shell, 3);
    psSpr(SPR_V2_SHELL, 3, 356, 372, 22, shell, 3);
  } else {  // at its feet: a mound of sand
    psOval(scE(-100, -14, 100, 44), p.near, scMix(p.near, 0xFFFFFF, .35));
    scRect(-40, 12, -33, 15, p.dark), scRect(24, 20, 31, 23, p.dark), scRect(56, 8, 59, 11, 0xF08AA4);
  }
}

// PS_CRYSTAL: the Cave's crystals glowing in the dark; it comes up from behind a little cluster of them
static void psCrystalSpr(int x, int y, int s) { psSpr(SPR_V2_CRYSTAL, 6, x, y, s, SC_CRYSTAL_KEYS, 2); }
static void psCaveBack(const ScPal &p, int cx, int cy, int rx, int ry, Rgb hollow) {  // the dark, a lighter hollow in it
  scRect(-PS_M, -PS_M, V2_W + PS_M, V2_W + PS_M, p.top);
  for (int y = -PS_M; y < V2_W + PS_M; y += 4)
    for (int x = -PS_M; x < V2_W + PS_M; x += 4) {
      float a = (x + 2 - cx) / (float)rx, b = (y + 2 - cy) / (float)ry, r = a * a + b * b;
      if (r < 1) scCell(x, y, (r > .45f || (r > .35f && (x / 4 + y / 4) % 2)) ? p.bot : hollow);
    }
}
static void psStalactites(const int16_t (*d)[3], int n, Rgb col, Rgb lit, int y0) {  // x, width, length each
  for (int k = 0; k < n; k++)
    for (int cy = 0; cy < d[k][2]; cy += 4) {
      float half = d[k][1] / 2.0f * (1 - (float)cy / d[k][2]);
      for (int cx = (int)(d[k][0] - half) / 4 * 4; cx <= (int)(d[k][0] + half); cx += 4)
        scCell(cx, y0 + cy, cx < d[k][0] - half + 4 ? lit : col);
    }
}
static void psCrystal(int part) {
  ScPal p = scCavePal();
  if (part == 0) {
    psCaveBack(p, 233, 190, 230, 170, 0x3A3450);
    static const int16_t drip[7][3] = { { -20, 50, 110 }, { 60, 30, 70 }, { 150, 26, 50 }, { 290, 30, 60 }, { 370, 40, 90 }, { 450, 50, 120 }, { 520, 40, 80 } };
    psStalactites(drip, 7, 0x494258, 0x5A5270, -PS_M);
    psOval(scE(-500, 250, 966, 1200), p.mid, scMix(p.mid, 0xFFFFFF, p.lit));
    static const int16_t back[5][3] = { { 40, 220, 4 }, { 74, 236, 3 }, { 356, 214, 4 }, { 392, 232, 3 }, { 300, 250, 2 } };
    for (auto &c : back) psCrystalSpr(c[0], c[1], c[2]);
    psOval(scE(-600, 300, 1066, 1200), p.near, scMix(p.near, 0xFFFFFF, p.lit));
    static const int16_t glint[8][2] = { { 120, 120 }, { 340, 110 }, { 200, 170 }, { 270, 150 }, { 60, 160 }, { 420, 180 }, { 160, 330 }, { 320, 350 } };
    for (int k = 0; k < 8; k++) scCell(glint[k][0], glint[k][1], k % 2 ? 0x8FF0E0 : 0x3FB8A8);
  } else if (part == 1) {  // near: big crystals at the sides, a stalactite at the top
    psCrystalSpr(-44, 318, 20);
    psCrystalSpr(386, 296, 18);
    psCrystalSpr(330, 404, 12);
    static const int16_t drip[2][3] = { { 40, 120, 150 }, { 470, 90, 110 } };
    psStalactites(drip, 2, 0x2E2840, 0x3E3656, -PS_M);
  } else {  // at its feet: a cluster of little crystals on a rock
    psOval(scE(-104, 4, 104, 44), p.near, scMix(p.near, 0xFFFFFF, .2));
    psCrystalSpr(-92, -26, 6), psCrystalSpr(-56, -12, 5), psCrystalSpr(36, -18, 6), psCrystalSpr(70, -4, 5);
  }
}

// PS_LEDGE: a ledge of rock in the Cave, daylight far up through its mouth; it comes up from behind a rock
static void psLedge(int part) {
  ScPal p = scCavePal();
  if (part == 0) {
    scRect(-PS_M, -PS_M, V2_W + PS_M, V2_W + PS_M, 0x1A1624);
    for (int y = -PS_M; y < 200; y += 4)  // the mouth, up on the left: the day outside
      for (int x = -PS_M; x < 260; x += 4) {
        float a = (x + 2 - 60) / 150.0f, b = (y + 2 - 40) / 110.0f, r = a * a + b * b;
        if (r < 1) scCell(x, y, r < .5f ? 0xBFE6F5 : r < .7f || (r < .8f && (x / 4 + y / 4) % 2) ? 0x7FA8C8 : 0x3A3450);
      }
    psOval(scE(-300, 120, 300, 420), p.far, scMix(p.far, 0xFFFFFF, p.lit));
    psOval(scE(200, 90, 800, 440), p.top, scMix(p.top, 0xFFFFFF, .08));
    psOval(scE(-500, 268, 900, 440), p.mid, scMix(p.mid, 0xFFFFFF, .2));  // the ledge
    psOval(scE(-500, 330, 900, 1200), p.dark, scMix(p.dark, 0xFFFFFF, .06));  // under it, the drop
    static const int16_t glint[6][2] = { { 300, 160 }, { 380, 200 }, { 420, 130 }, { 120, 360 }, { 330, 380 }, { 260, 230 } };
    for (int k = 0; k < 6; k++) scCell(glint[k][0], glint[k][1], k % 2 ? 0x8FF0E0 : 0x3FB8A8);
  } else if (part == 1) {  // near: stalactites over the top right, a boulder at the bottom left
    static const int16_t drip[3][3] = { { 300, 90, 120 }, { 400, 120, 170 }, { 500, 90, 110 } };
    psStalactites(drip, 3, 0x2E2840, 0x3E3656, -PS_M);
    scRole(SPR_V2_ROCK, 5, -90, 372, 22, p, p.near);
  } else {  // at its feet: a rock
    scRole(SPR_V2_ROCK, 5, -48, -20, 12, p, p.near);
  }
}

// PS_WINDOW: under the Mansion's lit window, at night; it peeks over the balcony's stone rail
static void psWindowPane(int x0, int y0, int x1, int y1) {  // a window: its frame, four lit panes, a warm middle
  scRect(x0, y0, x1, y1, 0x0D0B20);
  scRect(x0 + 8, y0 + 8, x1 - 8, y1 - 8, 0xF0C04A);
  psOval(scE((x0 + x1) / 2 - (x1 - x0) / 4, (y0 + y1) / 2 - (y1 - y0) / 4, (x0 + x1) / 2 + (x1 - x0) / 4, (y0 + y1) / 2 + (y1 - y0) / 4), 0xFFF0B0);
  scRect((x0 + x1) / 2 - 4, y0, (x0 + x1) / 2 + 3, y1, 0x0D0B20);
  scRect(x0, (y0 + y1) / 2 - 4, x1, (y0 + y1) / 2 + 3, 0x0D0B20);
}
static void psWindow(int part) {
  if (part == 0) {
    scRect(-PS_M, -PS_M, V2_W + PS_M, V2_W + PS_M, 0x342A55);  // the wall, in blocks of stone
    for (int y = -PS_M, r = 0; y < 320; y += 28, r++) {
      scRect(-PS_M, y, V2_W + PS_M, y + 3, 0x2C2448);
      for (int x = -PS_M + (r % 2) * 32; x < V2_W + PS_M; x += 64) scRect(x, y, x + 3, y + 27, 0x2C2448);
    }
    psWindowPane(-60, 40, 60, 220);
    psWindowPane(141, 70, 325, 290);
    scRect(125, 286, 341, 301, 0x4E4280);  // its sill
    scRect(125, 286, 341, 289, 0x7A6AA8);
    psOval(scE(-500, 300, 966, 1200), 0x4E4280, 0x7A6AA8);  // the balcony's floor
    for (int x = -PS_M; x < V2_W + PS_M; x += 48) scRect(x, 330 + (x * 5 & 15), x + 27, 333 + (x * 5 & 15), 0x3D3262);
  } else if (part == 1) {  // near: bare branches over the top
    const ScKey bare[1] = { { '1', 0x0D0B20 } };
    psSpr(SPR_V2_BARE, 11, -100, -80, 16, bare, 1);
    psSpr(SPR_V2_BARE, 11, 330, -110, 14, bare, 1);
  } else {  // at its feet: the balcony's rail, between two posts
    const Rgb stone = 0x4E4280, lit = 0x7A6AA8, dark = 0x2C2448;
    scRect(-128, -12, 127, 3, lit);
    scRect(-128, -4, 127, 7, stone);
    for (int x = -96; x <= 84; x += 30) {
      scRect(x, 8, x + 11, 59, stone);
      scRect(x, 8, x + 3, 59, lit);
    }
    scRect(-128, 56, 127, 71, dark);
    scRect(-128, -20, -105, 71, stone), scRect(-128, -20, -105, -17, lit);
    scRect(104, -20, 127, 71, stone), scRect(104, -20, 127, -17, lit);
  }
}

// PS_SKY: in the air over the Field, its fence close by below
static void psSky(int part) {
  ScPal p = scFieldPal();
  const Rgb post = 0x9A6F43, postLit = 0xB38A58, rail = 0xB38A58;
  if (part == 0) {
    psSky3(p.top, p.bot, 130, 220, 310);
    scSun(p, false, 380, 84);
    scCloud(p, 30, 80, 6);
    scCloud(p, 290, 170, 4);
    scCloud(p, 130, 230, 3);
    psOval(scE(-300, 300, 300, 800), p.far, scMix(p.far, 0xFFFFFF, p.lit));
    psOval(scE(200, 290, 800, 800), p.far, scMix(p.far, 0xFFFFFF, p.lit));
    scHouse(20, 270, false);
    for (int k = 0; k < 3; k++) scRole(SPR_V2_TREE, 10, 330 + k * 40, 262 + (k % 2) * 8, 3, p, p.mid);
    psOval(scE(-500, 330, 966, 1200), p.mid, scMix(p.mid, 0xFFFFFF, p.lit));
    scFence(372, -PS_M, V2_W + PS_M, post, rail, 28);
    psOval(scE(-700, 380, 1166, 1400), p.near, scMix(p.near, 0xFFFFFF, p.lit));
  } else if (part == 1) {  // near: the fence's posts and rails, big, across the bottom
    for (int x : { -40, 392 }) {
      scRect(x, 300, x + 63, V2_W + PS_M, post);
      scRect(x, 300, x + 63, 311, postLit);
      scRect(x, 300, x + 11, V2_W + PS_M, postLit);
    }
    scRect(-PS_M, 380, V2_W + PS_M, 411, rail);
    scRect(-PS_M, 380, V2_W + PS_M, 387, scMix(rail, 0xFFFFFF, .3));
    scRect(-PS_M, 456, V2_W + PS_M, 487, rail);
    psTallGrass(80, 470, 16, p.dark, scMix(p.near, p.dark, .35));
  }
}

// PS_FLOWERS: the Field's flowers at sunset; it comes up out of the grass
static void psFlowers(int part) {
  ScPal p = scFieldPal();
  const ScPal &d = SC_DUSK_SKY;
  if (part == 0) {
    psSky3(d.top, d.bot, 100, 180, 260);
    scSun(d, false, 320, 236);
    scCloud(d, 40, 90, 5);
    scCloud(d, 300, 140, 3);
    scGradeTod = 1;  // the land in the evening's light
    psOval(scE(-300, 236, 340, 800), p.far, scMix(p.far, 0xFFFFFF, p.lit));
    psOval(scE(180, 248, 800, 800), p.far, scMix(p.far, 0xFFFFFF, p.lit));
    for (int k = 0; k < 4; k++) scRole(SPR_V2_TREE, 10, -20 + k * 44, 214 + (k % 2) * 10, 3, p, p.mid);
    psOval(scE(-600, 274, 1066, 1400), p.mid, scMix(p.mid, 0xFFFFFF, p.lit));
    psOval(scE(-700, 310, 1166, 1400), p.near, scMix(p.near, 0xFFFFFF, p.lit));
    for (int i = 0; i < 24; i++) scCell(-40 + (i * 97 % 540), 290 + (i * 41 % 120), p.fl[i % 3]);
    scGradeTod = 0;
  } else if (part == 1) {  // near: big flowers, their stems, at the sides
    scGradeTod = 1;
    scRect(30, 410, 41, V2_W + PS_M, p.dark), scRect(390, 430, 401, V2_W + PS_M, p.dark), scRect(338, 480, 347, V2_W + PS_M, p.dark);
    psFlower(36, 384, 22, 0xF08AA4, 0xFFE48A);
    psFlower(396, 404, 20, 0xFFD95E, 0xE8503A);
    psFlower(342, 470, 14, 0xFFF3DC, 0xFFD95E);
    psTallGrass(-60, 450, 14, p.dark, scMix(p.near, p.dark, .35));
    scGradeTod = 0;
  } else {  // at its feet: grass and little flowers
    scGradeTod = 1;
    psTuft(-84, 84, 12, 44, scMix(p.near, 0xFFFFFF, .1), scMix(p.near, p.dark, .4), 5);
    scRect(-80, 8, 79, 15, scMix(p.near, p.dark, .4));
    psFlower(-60, -22, 4, p.fl[2], p.fl[1]), psFlower(52, -14, 4, p.fl[0], p.fl[1]), psFlower(-8, -30, 4, p.fl[1], 0xE8503A);
    scGradeTod = 0;
  }
}

// a place's far (0), near (1) or at-its-feet (2) plane: at-its-feet is drawn round (0, 0), the Pokemon's feet
static bool psHasFeet(uint8_t place) { return place != PS_GRAVES && place != PS_SKY; }
static void psPaint(uint8_t place, int part) {
  scGradeTod = 0;
  switch (place) {
    case PS_GRASS: psGrass(part); break;
    case PS_PINES: psPines(part); break;
    case PS_SHORE: psShore(part); break;
    case PS_WAVES: psWaves(part); break;
    case PS_CRYSTAL: psCrystal(part); break;
    case PS_LEDGE: psLedge(part); break;
    case PS_GRAVES: psGraves(part); break;
    case PS_WINDOW: psWindow(part); break;
    case PS_SKY: psSky(part); break;
    default: psFlowers(part); break;
  }
  scGradeTod = 0;
}

// ----- making a place ready, a few steps a frame -----
// 0: the sprite; 1..: the far plane's strips, then the near plane's; then each lens step's picture, a channel a step
// (the far plane blurred into it, then the near plane's cover and colours laid over)
#define PS_STRIPS ((2 * PS_BW + PS_STRIP - 1) / PS_STRIP)
static int psPrepAt = 0;       // its next step
static bool psPrepDone = false;
static int16_t psMonDex = 0;   // the sprite loaded
static bool psMonShiny = false;
static uint8_t psMonS = 4;     // its size (the canvas's px a sprite pixel: 2, 4 or 6)
static int16_t psMonUp = 40, psMonOffX = 0;  // its middle: how far over its feet, and across from where it stands
static int16_t psAimX = 0, psAimY = 0;       // the camera's aim, off the place's middle (px): at the Pokemon
static int16_t psResX = 0, psResY = 0;       // ...what's left over (PS_AIM_MAX), the Pokemon's middle off the screen's at rest

// a strip of the canvas-sized painting, rows y0.. of the world, into one of the Knomi-sized planes
static void psPaintStrip(uint8_t place, int part, int y0) {
  const int W = 2 * PS_BW;
  scFb = psStripBuf;
  scW = W;
  scH = PS_STRIP;
  scDX = PS_M;
  scDY = -y0;
  scKeyed = part != 0;
  if (part != 0)
    for (int i = 0; i < W * PS_STRIP; i++) psStripBuf[i] = V2_STRIP_KEY;
  psPaint(place, part);
  scScreen();
  // each of the Knomi's pixels from its 2 x 2 block's top left (the painters keep to the 2 px grid)
  for (int r = 0; r < PS_STRIP; r += 2) {
    int y = (y0 + PS_M + r) / 2;
    if (y >= PS_BW) break;
    const uint16_t *src = psStripBuf + r * W;
    if (part == 0) {
      uint16_t *d = psFar + y * PS_BW;
      for (int x = 0; x < PS_BW; x++) d[x] = src[2 * x];
    } else {
      uint16_t *d = psNear + y * PS_BW;
      uint8_t *a = psNearA + y * PS_BW;
      for (int x = 0; x < PS_BW; x++) {
        uint16_t c = src[2 * x];
        a[x] = c == V2_STRIP_KEY ? 0 : 255;
        d[x] = c == V2_STRIP_KEY ? 0 : c;
      }
    }
  }
}

// what's at its feet: painted round (0, 0) into psOcc, the Knomi's size (psOccX0..: where its top left is, from the feet)
static int16_t psOccX0 = -PS_OCC_W / 2, psOccY0 = -PS_OCC_H / 2;
static bool psOccOn = false;
static void psPaintOcc(uint8_t place) {
  psOccOn = psHasFeet(place);
  if (!psOccOn) return;
  // painted at the canvas's size a strip at a time (psStripBuf is wide enough), then halved
  const int W = 2 * PS_OCC_W;
  for (int y0 = 0; y0 < 2 * PS_OCC_H; y0 += PS_STRIP) {
    scFb = psStripBuf;
    scW = W;
    scH = PS_STRIP;
    scDX = W / 2;
    scDY = PS_OCC_H - y0;
    scKeyed = true;
    for (int i = 0; i < W * PS_STRIP; i++) psStripBuf[i] = V2_STRIP_KEY;
    psPaint(place, 2);
    scScreen();
    for (int r = 0; r < PS_STRIP && y0 + r < 2 * PS_OCC_H; r += 2)
      for (int x = 0; x < PS_OCC_W; x++) psOcc[(y0 + r) / 2 * PS_OCC_W + x] = psStripBuf[r * W + 2 * x];
  }
  psOccX0 = -PS_OCC_W / 2, psOccY0 = -PS_OCC_H / 2;
}

// the Pokemon's sprite, and how big it's drawn: the biggest of 6, 4 and 2 px a pixel it fits at (PS_SUB_*), its
// middle close to where the camera can aim
static void psLoadMon(const PsState &s) {
  if (psMonDex != s.dex || psMonShiny != s.shiny || !psMon.loaded) {
    psMon.load((uint8_t)s.dex, s.shiny);
    psMonDex = s.dex;
    psMonShiny = s.shiny;
  }
  int top = 0, bottom = 31, left = 0, right = 31;
  if (psMon.loaded) pmdBoxAll(psMon, PMD_IDLE, top, bottom, left, right);
  int h = bottom - top + 1, w = right - left + 1;
  const PmdAct &a = psMon.acts[PMD_IDLE];
  int aw = psMon.loaded ? a.w : 32, base = psMon.loaded && a.base ? a.base : bottom + 1;
  psMonS = 2;
  for (int sc = 6; sc >= 4; sc -= 2)
    if (h * sc <= 150 && w * sc <= 180 && aw * sc / 2 + 2 * PS_BLUR_MON_MAX + 4 <= PS_SUB_W && (bottom + 1) * sc / 2 <= PS_SUB_H - 24) {
      psMonS = (uint8_t)sc;
      break;
    }
  psMonUp = (int16_t)((base * 2 - top - bottom - 1) * psMonS / 2);
  psMonOffX = (int16_t)((left + right + 1 - aw) * psMonS / 2);
  const PsSceneDef &d = psScene(s);
  int ax = d.feetX + psMonOffX - CX, ay = d.feetY - psMonUp - CX;
  psAimX = (int16_t)(ax < -PS_AIM_MAX ? -PS_AIM_MAX : ax > PS_AIM_MAX ? PS_AIM_MAX : ax);
  psAimY = (int16_t)(ay < -PS_AIM_MAX ? -PS_AIM_MAX : ay > PS_AIM_MAX ? PS_AIM_MAX : ay);
  psResX = (int16_t)(ax - psAimX), psResY = (int16_t)(ay - psAimY);
}

// one step of making place ready; true once it's done
static bool psPrepStep(uint8_t place) {
  const int P = PS_BW * PS_BW;
  int k = psPrepAt++;
  if (k == 0) {
    psPaintOcc(place);
    return false;
  }
  k--;
  if (k < PS_STRIPS) { psPaintStrip(place, 0, -PS_M + k * PS_STRIP); return false; }
  k -= PS_STRIPS;
  if (k < PS_STRIPS) { psPaintStrip(place, 1, -PS_M + k * PS_STRIP); return false; }
  k -= PS_STRIPS;
  // the lens steps: for each, 3 far channels, the near cover, 3 near channels, then the dither
  int slot = k / 8, c = k % 8;
  if (slot >= psLays) return true;
  if (slot == 0 && c == 0) psPalCollect();
  int step = psLays >= PS_LENS_STEPS ? slot : slot * PS_BLUR_MAX / (psLays > 1 ? psLays - 1 : 1);
  uint16_t *out = psLay[slot];
  int rFar = PS_BLUR_MAX - step, rNear = step;
  if (c < 3) {
    for (int i = 0; i < P; i++) psWork[0][i] = psCh(psFar[i], c);
    psBlur(psWork[0], PS_BW, PS_BW, rFar);
    if (c == 0)
      for (int i = 0; i < P; i++) out[i] = psSetCh(0, 0, psWork[0][i]);
    else
      for (int i = 0; i < P; i++) out[i] = psSetCh(out[i], c, psWork[0][i]);
  } else if (c == 3) {
    memcpy(psWork[1], psNearA, P);
    psBlur(psWork[1], PS_BW, PS_BW, rNear);
  } else if (c < 7) {
    int ch = c - 4;
    for (int i = 0; i < P; i++) psWork[0][i] = (uint8_t)(psCh(psNear[i], ch) * psNearA[i] / 255);  // its colour times its cover
    psBlur(psWork[0], PS_BW, PS_BW, rNear);
    for (int i = 0; i < P; i++) {
      int a = psWork[1][i];
      if (!a) continue;
      out[i] = psSetCh(out[i], ch, psWork[0][i] + psCh(out[i], ch) * (255 - a) / 255);
    }
  } else {  // the dither, in the scenes' cells (the world's: PS_PAD is even)
    psDitherCells(out, PS_BW, PS_BW, PS_BW, 0, 0, psPalN);
  }
  return slot == psLays - 1 && c == 7;
}
// make place ready for up to ms (all of it: ms 0)
static void psPrepWork(uint8_t place, uint32_t ms) {
  if (psPrepFor != (int8_t)place) {
    psPrepFor = (int8_t)place;
    psPrepAt = 0;
    psPrepDone = false;
  }
  uint32_t t0 = millis();
  while (!psPrepDone) {
    psPrepDone = psPrepStep(place);
    if (ms && millis() - t0 >= ms) break;
  }
}

// ----- a frame through the camera -----
// the camera's pan now (px): its aim at the Pokemon and its drift
static int psPanX(const PsState &s) { return psAimX + (int)lroundf(s.wx); }
static int psPanY(const PsState &s) { return psAimY + (int)lroundf(s.wy); }

// the place at the lens's step, the camera panned (px, the canvas's), each of its pixels a 2 x 2 block
static void psBlitPlace(int step, int panX, int panY) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  const uint16_t *L = psLay[psLaySlot(step)];
  int ox = PS_PAD + (panX >> 2) * 2, oy = PS_PAD + (panY >> 2) * 2;  // whole cells, so they stay on the screen's grid
                                                                     // (>> 2: down, for a negative pan too)
  ox = ox < 0 ? 0 : ox > 2 * PS_PAD ? 2 * PS_PAD : ox;
  oy = oy < 0 ? 0 : oy > 2 * PS_PAD ? 2 * PS_PAD : oy;
  for (int y = 0; y < PS_K; y++) {
    const uint16_t *src = L + (oy + y) * PS_BW + ox;
    uint32_t *d0 = (uint32_t *)(fb + 2 * y * V2_W), *d1 = (uint32_t *)(fb + (2 * y + 1) * V2_W);
    for (int x = 0; x < PS_K; x++) {
      uint32_t v = src[x] | (uint32_t)src[x] << 16;
      d0[x] = v;
      d1[x] = v;
    }
  }
}

// The Pokemon and what's at its feet: what's there stays at (fx, fy) on the screen (its feet at rest, the canvas's px),
// the Pokemon's feet at (mx, my), blurred r px, alpha of it showing (a ghost fading): put in psSub (the Knomi's px,
// colours times cover) just big enough, blurred, laid over. clipY: nothing of it under that row (it's coming up from
// behind what's at its feet, or under the water; -1: none)
static void psDrawMon(int fx, int fy, int mx, int my, int clipY, int r, float alpha, uint32_t now, uint8_t act = PMD_IDLE,
                      uint32_t actMs = 0, bool loop = true) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  if (!psMon.has(act)) act = PMD_IDLE, actMs = now, loop = true;
  const PmdAct &a = psMon.acts[act];
  bool mon = psMon.loaded && a.frames && alpha > 0.02f;
  bool clip = clipY >= 0;
  int k = psMonS / 2;  // the Knomi's px a sprite pixel
  int kfx = fx >> 1, kfy = fy >> 1, kmx = mx >> 1, kmy = my >> 1, kclip = clipY >> 1;
  // the box (the Knomi's px, on the screen): the sprite's and what's at its feet
  int bx0 = 9999, by0 = 9999, bx1 = -9999, by1 = -9999;
  uint8_t fi = 0;
  int sx0 = 0, sy0 = 0;
  if (mon) {
    fi = pmdFrameAt(a, actMs, loop);
    sx0 = kmx - a.w * k / 2;
    sy0 = kmy - pmdFrameRise(a, fi, psMonS) / 2;
    bx0 = sx0, by0 = sy0, bx1 = sx0 + a.w * k, by1 = sy0 + a.h * k;
    if (clip && by1 > kclip) by1 = kclip;
    if (by1 <= by0) mon = false, bx0 = by0 = 9999, bx1 = by1 = -9999;
  }
  if (psOccOn) {
    int ox0 = kfx + psOccX0, oy0 = kfy + psOccY0;
    bx0 = bx0 < ox0 ? bx0 : ox0, by0 = by0 < oy0 ? by0 : oy0;
    bx1 = bx1 > ox0 + PS_OCC_W ? bx1 : ox0 + PS_OCC_W, by1 = by1 > oy0 + PS_OCC_H ? by1 : oy0 + PS_OCC_H;
  }
  bx0 = bx0 < -PS_BLUR_MON_MAX - 1 ? -PS_BLUR_MON_MAX - 1 : bx0, by0 = by0 < -PS_BLUR_MON_MAX - 1 ? -PS_BLUR_MON_MAX - 1 : by0;
  bx1 = bx1 > PS_K + PS_BLUR_MON_MAX + 1 ? PS_K + PS_BLUR_MON_MAX + 1 : bx1, by1 = by1 > PS_K + PS_BLUR_MON_MAX + 1 ? PS_K + PS_BLUR_MON_MAX + 1 : by1;
  if (bx1 <= bx0 || by1 <= by0) return;
  bx0 -= r + 1, by0 -= r + 1, bx1 += r + 1, by1 += r + 1;
  if (bx1 - bx0 > PS_SUB_W) {  // (seldom: the sizes are picked to fit) the middle of it
    int c = (bx0 + bx1) / 2;
    bx0 = c - PS_SUB_W / 2, bx1 = bx0 + PS_SUB_W;
  }
  if (by1 - by0 > PS_SUB_H) by0 = by1 - PS_SUB_H;  // (...its foot, where it stands)
  int w = bx1 - bx0, h = by1 - by0, n = w * h;
  uint8_t *R = psSub, *G = R + n, *B = G + n, *A = B + n;
  memset(psSub, 0, 4 * n);
  if (mon) {  // the sprite, each of its pixels k x k
    const uint8_t *fr = a.data + (uint32_t)fi * a.w * a.h;
    for (int row = 0; row < a.h; row++)
      for (int col = 0; col < a.w; col++) {
        uint8_t ix = fr[row * a.w + col];
        if (ix == 0xFF) continue;
        uint16_t c = psMon.pal[ix];
        uint8_t cr = psCh(c, 0), cg = psCh(c, 1), cb = psCh(c, 2);
        for (int dy = 0; dy < k; dy++) {
          int y = sy0 + row * k + dy;
          if (clip && y >= kclip) break;
          if (y < by0 || y >= by1) continue;
          for (int dx = 0; dx < k; dx++) {
            int x = sx0 + col * k + dx;
            if (x < bx0 || x >= bx1) continue;
            int i = (y - by0) * w + (x - bx0);
            R[i] = cr, G[i] = cg, B[i] = cb, A[i] = 255;
          }
        }
      }
    if (alpha < 1) {  // fading: its cover and colours with it
      int v = (int)(alpha * 256);
      for (int i = 0; i < n; i++) R[i] = R[i] * v >> 8, G[i] = G[i] * v >> 8, B[i] = B[i] * v >> 8, A[i] = A[i] * v >> 8;
    }
  }
  if (psOccOn)  // what's at its feet, in front of it
    for (int y = 0; y < PS_OCC_H; y++)
      for (int x = 0; x < PS_OCC_W; x++) {
        uint16_t c = psOcc[y * PS_OCC_W + x];
        if (c == V2_STRIP_KEY) continue;
        int X = kfx + psOccX0 + x - bx0, Y = kfy + psOccY0 + y - by0;
        if (X < 0 || Y < 0 || X >= w || Y >= h) continue;
        int i = Y * w + X;
        R[i] = psCh(c, 0), G[i] = psCh(c, 1), B[i] = psCh(c, 2), A[i] = 255;
      }
  if (r > 0) {
    psBlur(R, w, h, r);
    psBlur(G, w, h, r);
    psBlur(B, w, h, r);
    psBlur(A, w, h, r);
  }
  for (int y = 0; y < h; y++) {
    int Y = by0 + y;
    if (Y < 0 || Y >= PS_K) continue;
    for (int x = 0; x < w; x++) {
      int X = bx0 + x, i = y * w + x, al = A[i];
      if (X < 0 || X >= PS_K || !al) continue;
      uint16_t *p = fb + 2 * Y * V2_W + 2 * X, d = *p, o;
      if (al >= 255) o = (uint16_t)((R[i] >> 3) << 11 | (G[i] >> 2) << 5 | B[i] >> 3);
      else {
        int inv = 255 - al;
        o = psSetCh(psSetCh(psSetCh(0, 0, R[i] + psCh(d, 0) * inv / 255), 1, G[i] + psCh(d, 1) * inv / 255), 2, B[i] + psCh(d, 2) * inv / 255);
      }
      p[0] = p[1] = p[V2_W] = p[V2_W + 1] = o;
    }
  }
  if (r > 0 || alpha < 1) {  // out of focus (or fading): dithered where it was laid, in the place's colours and its own
    psPalN = psPalPlace;
    for (int i = 0; i < psMon.palCount && i < 255; i++) psPalAdd(psMon.pal[i]);
    auto covered = [&](int X, int Y) {  // any of it in the screen's cell at (X, Y)
      for (int yy = 0; yy < 2; yy++)
        for (int xx = 0; xx < 2; xx++) {
          int x = X + xx - bx0, y = Y + yy - by0;
          if (x >= 0 && y >= 0 && x < w && y < h && A[y * w + x]) return true;
        }
      return false;
    };
    for (int Y = (by0 < 0 ? 0 : by0) & ~1; Y + 1 < PS_K && Y < by1; Y += 2)
      for (int X = (bx0 < 0 ? 0 : bx0) & ~1; X + 1 < PS_K && X < bx1; X += 2) {
        if (!covered(X, Y)) continue;
        int c[3];
        uint16_t *q = fb + 2 * Y * V2_W + 2 * X;
        for (int ch = 0; ch < 3; ch++) c[ch] = (psCh(q[0], ch) + psCh(q[2], ch) + psCh(q[2 * V2_W], ch) + psCh(q[2 * V2_W + 2], ch) + 2) / 4;
        uint16_t v = psDitherPick(c[0], c[1], c[2], X >> 1, Y >> 1, psPalN);
        for (int yy = 0; yy < 4; yy++)
          for (int xx = 0; xx < 4; xx++) q[yy * V2_W + xx] = v;
      }
    psPalN = psPalPlace;
  }
  if (mon && psMon.shiny && r < 3 && alpha > 0.6f) {
    int top = sy0 * 2, bot = my;
    drawSparkles(mx - a.w * k, top + (bot - top) / 4, mx + a.w * k, bot, now);
  }
}

// the viewfinder's marks: a corner at each side, the focus point in the middle, the roll's count and score
static void psMark(int x, int y, int dx, int dy, int len, Rgb col) {  // a corner, its 4 x 4 corner at (x, y), arms toward dx, dy
  int xa = dx > 0 ? x : x - len + 4, ya = dy > 0 ? y : y - len + 4;
  fsBlend(xa - 2, y - 2, xa + len + 1, y + 5, 0x1B1B25, 140);  // a dark edge, see-through
  fsBlend(x - 2, ya - 2, x + 5, ya + len + 1, 0x1B1B25, 140);
  scRect(xa, y, xa + len - 1, y + 3, col);
  scRect(x, ya, x + 3, ya + len - 1, col);
}
static void psViewfinder(const PsState &s) {
  scScreen();
  const Rgb w = 0xF4F4F0;
  const int e = 128, L = 30;
  psMark(CX - e, CX - e, 1, 1, L, w);
  psMark(CX + e - 4, CX - e, -1, 1, L, w);
  psMark(CX - e, CX + e - 4, 1, -1, L, w);
  psMark(CX + e - 4, CX + e - 4, -1, -1, L, w);
  const int f = 26, l = 12;  // the focus point
  psMark(CX - f, CX - f, 1, 1, l, w);
  psMark(CX + f - 4, CX - f, -1, 1, l, w);
  psMark(CX - f, CX + f - 4, 1, -1, l, w);
  psMark(CX + f - 4, CX + f - 4, -1, -1, l, w);
  char b[16];
  snprintf(b, sizeof(b), "%u/%u", (unsigned)(s.shot + 1), (unsigned)PS_SHOTS);
  fsSay(b, CX, 40, 0xFFFFFF);
  snprintf(b, sizeof(b), "%u", (unsigned)s.score);
  fsSay(b, CX, 416, 0xF2C230);
}

// the photo: the screen as it is (the place and the Pokemon, before the marks), each 2 x 2 block's top left
static void psKeepPhoto(const PsState &s) {
  psPhotoN++;
  const uint16_t *fb = (const uint16_t *)gfx->getFramebuffer();
  for (int y = 0; y < PS_K; y++)
    for (int x = 0; x < PS_K; x++) psPhoto[y * PS_K + x] = fb[2 * y * V2_W + 2 * x];
  uint16_t *t = psThumbs + (size_t)s.shot * PS_THUMB * PS_THUMB;
  for (int y = 0; y < PS_THUMB; y++)
    for (int x = 0; x < PS_THUMB; x++) t[y * PS_THUMB + x] = psPhoto[(y * PS_K / PS_THUMB) * PS_K + x * PS_K / PS_THUMB];
}
// the photo over the whole screen, dimmed (a256 toward the night tint)
static void psPhotoFull(int dim) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  for (int y = 0; y < PS_K; y++) {
    uint32_t *d0 = (uint32_t *)(fb + 2 * y * V2_W), *d1 = (uint32_t *)(fb + (2 * y + 1) * V2_W);
    const uint16_t *src = psPhoto + y * PS_K;
    for (int x = 0; x < PS_K; x++) {
      uint32_t v = src[x] | (uint32_t)src[x] << 16;
      d0[x] = d1[x] = v;
    }
  }
  if (dim) v2DimFrame(dim);
}
// the photo at half the screen's size (each of its pixels one of the canvas's), its top left at (x, y), a border
static void psPrint(int x, int y) {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  const int b = 8;
  fsBlend(x - b + 6, y - b + 8, x + PS_K + b + 5, y + PS_K + b + 7, 0x0D0B20, 110);  // its shadow
  scRect(x - b, y - b, x + PS_K + b - 1, y + PS_K + b - 1, PS_CREAM);
  for (int r = 0; r < PS_K; r++) memcpy(fb + (y + r) * V2_W + x, psPhoto + r * PS_K, PS_K * 2);
}
// a score's meter: its word, a bar of pts out of max, the number
static void psMeter(const char *word, int y, uint8_t pts, uint8_t max) {
  const int x0 = 172, w = 112;
  setSize(2);
  int tw = textW(word, 2);
  fsSay(word, x0 - 12 - tw / 2, y, 0xFFFFFF);
  scRect(x0 - 2, y - 2, x0 + w + 1, y + 13, 0x1B1B25);
  scRect(x0, y, x0 + w - 1, y + 11, 0x3A3450);
  int f = w * pts / max;
  if (f > 0) scRect(x0, y, x0 + f - 1, y + 11, pts * 5 >= max * 4 ? 0x9AF07A : pts * 2 >= max ? 0xF2C230 : 0xE8503A);
  char b[8];
  snprintf(b, sizeof(b), "%u", (unsigned)pts);
  fsSay(b, x0 + w + 24, y, 0xFFFFFF);
}

// between two photos, at the foot of the print or of IT GOT AWAY!: the seconds till the next place, then GET READY! as it
// comes (Fedor: "I just want people to be ready for the next one since there is a pause inbetween them"; of three
// mockups, the countdown). dur: how long that screen stays. None after the last photo: the roll is next
static void psNextLine(const PsState &s, float t, float dur) {
  bool loading = s.phase == PSP_LOAD;  // (the next place being made ready: the shot is already the next one's)
  if (!loading && s.shot >= PS_SHOTS - 1) return;
  int left = loading ? 0 : (int)ceilf(dur - t);
  char b[48];
  if (left > 0) snprintf(b, sizeof(b), T(S_PS_NEXT_FMT), (unsigned)left);
  else snprintf(b, sizeof(b), "%s", T(S_PS_READY));
  fsSay(b, CX, 414, 0xF2C230);
}

// the word on a photo, by its points: the best, or what let it down
static StrId psVerdict(uint8_t pts, uint8_t focus, uint8_t centre) {
  if (pts >= PS_PERFECT) return S_PS_PERFECT;
  if (pts >= PS_GOOD) return S_PS_GREAT;
  if (pts >= 55) return S_PS_NICE;
  return focus * PS_PTS_CENTRE < centre * PS_PTS_FOCUS ? S_PS_BLURRY : S_PS_OFF;
}
#define PS_PRINT_X 116  // the print's top left: the photo at half the screen's size, a little over the middle
#define PS_PRINT_Y 64
// the print of the photo just taken (psPhoto): growing out of the whole screen for the first moments (k 0..1), its
// word over it, its score and how it was got under it
static void psDrawPrint(const PsState &s, float t) {
  float k = t < 0.25f ? psEase(t / 0.25f) : 1;
  const int32_t key = (int32_t)(psPhotoN << 1);
  if (k >= 1 && psStillBack(key)) {  // (kept: put back, only the line under it drawn)
    psNextLine(s, t, PS_PRINT_S);
    return;
  }
  psPhotoFull((int)(130 * k));
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  if (k < 1) {  // shrinking toward its place
    int size = (int)(2 * PS_K + (PS_K - 2 * PS_K) * k) & ~1, x0 = (int)(PS_PRINT_X * k) & ~1, y0 = (int)(PS_PRINT_Y * k) & ~1;
    fsBlend(x0 - 8, y0 - 8, x0 + size + 7, y0 + size + 7, PS_CREAM, (int)(256 * k));
    for (int y = 0; y < size; y++) {
      if (y0 + y >= V2_W) break;
      const uint16_t *src = psPhoto + (y * PS_K / size) * PS_K;
      uint16_t *d = fb + (y0 + y) * V2_W + x0;
      for (int x = 0; x < size && x0 + x < V2_W; x++) d[x] = src[x * PS_K / size];
    }
    return;
  }
  psPrint(PS_PRINT_X, PS_PRINT_Y);
  uint8_t pts = s.focusPts + s.centrePts;
  StrId v = psVerdict(pts, s.focusPts, s.centrePts);
  fsSay(T(v), CX, 26, pts >= PS_GOOD ? 0xF2C230 : 0xFFFFFF, 2);
  char b[16];
  snprintf(b, sizeof(b), "+%u", (unsigned)pts);
  fsSay(b, CX, 318, pts >= PS_GOOD ? 0xF2C230 : 0xFFFFFF, 4);
  psMeter(T(S_PS_FOCUS), 362, s.focusPts, PS_PTS_FOCUS);
  psMeter(T(S_PS_CENTRE), 386, s.centrePts, PS_PTS_CENTRE);
  psStillKeep(key);
  psNextLine(s, t, PS_PRINT_S);
}
// the roll at the end: each photo small, its points under it, in rows of 3, 4 and 3
static void psDrawRoll(const PsState &s) {
  scScreen();
  scRect(0, 0, V2_W - 1, V2_W - 1, 0x1E1838);
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  static const uint8_t ROW[3] = { 3, 4, 3 };
  const int cell = 2 * PS_THUMB, gap = 14;
  int i = 0;
  for (int r = 0; r < 3; r++) {
    int y = CX - (3 * cell + 2 * gap + 28) / 2 + r * (cell + gap + 10);
    int x = CX - (ROW[r] * cell + (ROW[r] - 1) * gap) / 2;
    for (int c = 0; c < ROW[r]; c++, i++, x += cell + gap) {
      scRect(x - 4, y - 4, x + cell + 3, y + cell + 3, PS_CREAM);
      if (s.got[i]) {
        const uint16_t *t = psThumbs + (size_t)i * PS_THUMB * PS_THUMB;
        for (int yy = 0; yy < cell; yy++)
          for (int xx = 0; xx < cell; xx++) fb[(y + yy) * V2_W + x + xx] = t[(yy / 2) * PS_THUMB + xx / 2];
      } else {
        scRect(x, y, x + cell - 1, y + cell - 1, 0x3A3450);
      }
      char b[8];
      snprintf(b, sizeof(b), "%u", (unsigned)s.pts[i]);
      fsSay(b, x + cell / 2, y + cell - 12, s.pts[i] >= PS_GOOD ? 0xF2C230 : 0xFFFFFF);
    }
  }
}
// the first photo's hint, under the marks: tap anywhere (or, on a computer, click or Space)
static void psHint(bool keys) { fsSay(T(keys ? S_PS_TAP_KEYS : S_PS_TAP), CX, 384, 0xFFFFFF); }

// The game at this moment. capture: the photo was just taken, keep the picture as it is (before the marks go over it)
static bool psShowedPrint = false;  // what's up while the next place is made ready: the last print, or IT GOT AWAY!
// The little things that move in each place, so it looks alive: drawn at (x, y) in the place (the canvas's px, the
// camera at rest) in its 4 px cells, panned with it (px, py: the camera's pan, in whole cells)
static void psCellAt(int x, int y, int px, int py, Rgb c, int w = 4, int h = 4) {
  x = (x & ~3) - px, y = (y & ~3) - py;
  if (x > -w && y > -h && x < V2_W && y < V2_W) scRect(x, y, x + w - 1, y + h - 1, c);
}
static float psFrac(float v) { return v - floorf(v); }
static void psLife(uint8_t place, uint32_t now, int px, int py) {
  const float t = now / 1000.0f;
  switch (place) {
    case PS_GRASS:  // two butterflies flitting over the grass
    case PS_FLOWERS:
      for (int i = 0; i < 2; i++) {
        float u = psFrac(t * 0.045f + i * 0.5f);
        int x = -40 + (int)(u * 560), y = (place == PS_GRASS ? 210 : 236) + i * 30 + (int)(18 * sinf(t * 1.7f + i * 2));
        bool up = ((now / 120) + i) % 2;
        Rgb c = i ? 0xFFF3DC : 0xFFD95E;
        psCellAt(x - 4, y - (up ? 4 : 0), px, py, c);
        psCellAt(x + 4, y - (up ? 4 : 0), px, py, c);
        psCellAt(x, y, px, py, 0x3A2414);
      }
      if (place == PS_FLOWERS)  // petals on the evening's breeze
        for (int i = 0; i < 6; i++) {
          float u = psFrac(t * 0.07f + i * 0.17f);
          psCellAt(-56 + (int)(u * 590), 120 + i * 40 + (int)(u * 80) + (int)(10 * sinf(t * 2 + i)), px, py, i % 2 ? 0xF08AA4 : 0xFFF3DC);
        }
      break;
    case PS_PINES:  // motes drifting down through the sun under the trees
      for (int i = 0; i < 7; i++) {
        float u = psFrac(t * 0.06f + i * 0.143f);
        if ((now / 300 + i) % 4 == 0) continue;  // (catching the light now and then)
        psCellAt(40 + i * 60 + (int)(14 * sinf(t + i)), 40 + (int)(u * 300), px, py, 0xFFF3C8);
      }
      break;
    case PS_SHORE: {  // the foam running up the wet sand and back
      int y = 240 + (int)(6 * sinf(t * 0.9f));
      for (int x = -56; x < 522; x += 40) psCellAt(x + (int)(8 * sinf(t * 0.9f + x)), y + (x * 3 & 4), px, py, SC_SEA_HI, 24, 4);
      break;
    }
    case PS_WAVES:  // the sun glinting on the water
    case PS_CRYSTAL:  // the crystals and the dark glinting
      for (int i = 0; i < 12; i++) {
        uint32_t b = (now / 160 + i * 7) % 12;
        if (b > 2) continue;
        int x = -40 + (i * 97) % 540, y = (place == PS_WAVES ? 200 : 60) + (i * 61) % (place == PS_WAVES ? 240 : 300);
        Rgb c = place == PS_WAVES ? 0xFFFFFF : i % 2 ? 0x8FF0E0 : 0xFFFFFF;
        psCellAt(x, y, px, py, c);
        if (b == 1) psCellAt(x - 4, y, px, py, c), psCellAt(x + 4, y, px, py, c), psCellAt(x, y - 4, px, py, c), psCellAt(x, y + 4, px, py, c);
      }
      break;
    case PS_LEDGE:  // water dripping from the stalactites
      for (int i = 0; i < 3; i++) {
        float u = psFrac(t * 0.5f + i * 0.37f);
        static const int16_t X[3] = { 300, 400, 500 };
        psCellAt(X[i], 60 + (int)(u * u * 300), px, py, 0x7FA8C8, 4, 8);
      }
      break;
    case PS_GRAVES:  // wisps of light drifting among the stones
      for (int i = 0; i < 4; i++) {
        int x = 60 + i * 110 + (int)(30 * sinf(t * 0.6f + i * 1.7f)), y = 250 + (int)(26 * sinf(t * 0.9f + i));
        if ((now / 200 + i * 3) % 7 == 0) continue;
        psCellAt(x, y, px, py, i % 2 ? 0xB8F070 : 0x8FF0E0);
      }
      break;
    case PS_WINDOW:  // moths round the lit window
      for (int i = 0; i < 3; i++) {
        float a = t * (1.6f + i * 0.4f) + i * 2.1f;
        psCellAt(233 + (int)(90 * cosf(a)), 170 + (int)(70 * sinf(a * 1.3f)), px, py, 0xFFF3DC);
      }
      break;
    case PS_SKY: {  // a bird far off, flapping across
      const ScKey k[1] = { { 'k', 0x18181F } };
      float u = psFrac(t * 0.05f);
      int x = -56 + (int)(u * 590) - px, y = 150 + (int)(10 * sinf(t * 1.3f)) - py;
      scSprite((now / 200) % 2 ? SPR_V2_BIRD_UP : SPR_V2_BIRD_DOWN, 3, x & ~3, y & ~3, 4, k, 1);
      break;
    }
  }
}

#ifdef BOARD_WS21
#define PSP(k) ws21ProfMark(k)  // (the console's PROF: Pic Snap's frame in parts)
#else
#define PSP(k)
#endif
static void psDrawPlace(PsState &s, uint32_t now, bool mon = true) {
  const PsSceneDef &d = psScene(s);
  int px = (psPanX(s) >> 2) * 4, py = (psPanY(s) >> 2) * 4;  // (in whole cells, as psBlitPlace() pans the place)
  PSP(1);
  psBlitPlace(psLensStep(s.f), px, py);
  PSP(2);
  psLife(psPlace(s), now, px, py);
  PSP(3);
  int fx = d.feetX - px, fy = d.feetY - py;
  int sink = psPlace(s) == PS_WAVES ? psMonUp * 3 / 4 : 0;  // in the sea: up to its middle in the water
  bool pop = d.how == PSH_POP;
  // what it's doing: its walk (the way it goes), a hop, one of its own moves, or standing breathing
  uint8_t act = PMD_IDLE;
  uint32_t at = now;
  bool loop = true;
  if (s.phase == PSP_AIM || s.phase == PSP_OUT) {
    static const uint8_t LOOK[3] = { PMD_POSE, PMD_NOD, PMD_BREATH };
    if (s.act == PSA_WALK) act = s.walkTo > s.walkFrom ? PMD_WALKR : PMD_WALKL;
    else if (s.act == PSA_HOP) act = PMD_HOP, loop = false;
    else if (s.act == PSA_LOOK) act = LOOK[s.look % 3], loop = false;
    if (act != PMD_IDLE) at = (uint32_t)(s.actT * 1000);
    if (!loop && psMon.has(act) && at >= pmdActTotalMs(psMon.acts[act])) act = PMD_IDLE, at = now, loop = true;  // (done)
  }
  psDrawMon(fx, fy, fx + SNAP2((int)lroundf(s.ox)), fy + sink + SNAP2((int)lroundf(s.oy)), pop ? fy + 2 : -1,
            psMonBlur(psFocusErr(s)), !mon ? 0 : d.how == PSH_FADE ? s.vis : 1, now, act, at, loop);
  PSP(4);
}
static void psDraw(PsState &s, uint32_t now, bool capture, bool keys) {
  scScreen();
  if (s.phase == PSP_END) { psDrawRoll(s); return; }
  if (capture) {  // the photo: the place as it was this moment, kept before the flash goes over it
    psDrawPlace(s, now);
    psKeepPhoto(s);
  }
  if (s.phase == PSP_SNAP) {  // the picture held, a flash over it
    psPhotoFull(0);
    if (s.t < 0.25f) fsBlend(0, 0, V2_W - 1, V2_W - 1, 0xFFFFFF, (int)(240 * (1 - s.t / 0.25f)));
    psShowedPrint = true;
    return;
  }
  if (s.phase == PSP_PRINT || (s.phase == PSP_LOAD && psShowedPrint && s.shot > 0)) {
    PSP(1);
    psDrawPrint(s, s.phase == PSP_PRINT ? s.t : 9);
    PSP(5);
    return;
  }
  if (s.phase == PSP_AWAY || (s.phase == PSP_LOAD && s.shot > 0)) {  // the place as it was left, dimmed
    const int32_t key = (int32_t)(psPhotoN << 1 | 1);
    if (!psStillBack(key)) {
      psPhotoFull(110);
      fsSay(T(S_PS_AWAY), CX, 220, 0xFFFFFF, 3);
      psStillKeep(key);
    }
    psNextLine(s, s.t, PS_AWAY_S);
    psShowedPrint = false;
    return;
  }
  if (s.phase == PSP_LOAD) {  // the first place, under the how-to card: the place, nobody there yet
    if (psPrepDone && psPrepFor == (int8_t)psPlace(s)) {
      psDrawPlace(s, now, false);
      psViewfinder(s);
    } else {
      scRect(0, 0, V2_W - 1, V2_W - 1, 0x1E1838);
    }
    return;
  }
  psDrawPlace(s, now);
  if (s.phase == PSP_OUT) {  // keep the place as it's left, for IT GOT AWAY!
    psPhotoN++;
    const uint16_t *fb = (const uint16_t *)gfx->getFramebuffer();
    for (int y = 0; y < PS_K; y++)
      for (int x = 0; x < PS_K; x++) psPhoto[y * PS_K + x] = fb[2 * y * V2_W + 2 * x];
  }
  psViewfinder(s);
  if (s.shot == 0 && (s.phase == PSP_IN || (s.phase == PSP_AIM && s.t < 2.5f))) psHint(keys);
  PSP(5);
}

// The cartridge's label (menus_v2.h, 79 x 42 at the canvas's top left): Oddish in the Forest's tall grass, soft pines
// behind, the viewfinder's corners round it
static void psPaintLabel() {
  scScreen();
  ScPal p = scForestPal();
  v2LblSky(p.top, p.bot, 28, p.near, scMix(p.near, p.dark, .25));
  for (int x = -4, i = 0; x < V2_LBL_W; x += 12, i++) {  // pines far off, soft (the lens is on the Pokemon)
    int h = 10 + (i * 5) % 6;
    fsBlend(x + 2, 28 - h, x + 9, 27, p.far, 150);
    fsBlend(x + 4, 26 - h - 3, x + 7, 27, p.far, 150);
  }
  v2LblThumb(43, 40, 34, false);
  const ScKey k[2] = { { 'G', p.dark }, { 'g', scMix(p.near, p.dark, .35) } };
  scSprite(SPR_V2_TALLGRASS, 5, 28, 30, 2, k, 2);
  const Rgb w = 0xF4F4F0;
  const int x0 = 18, y0 = 4, x1 = 61, y1 = 38, L = 7;  // the corners
  scRect(x0, y0, x0 + L, y0 + 1, w), scRect(x0, y0, x0 + 1, y0 + L, w);
  scRect(x1 - L, y0, x1, y0 + 1, w), scRect(x1 - 1, y0, x1, y0 + L, w);
  scRect(x0, y1 - 1, x0 + L, y1, w), scRect(x0, y1 - L, x0 + 1, y1, w);
  scRect(x1 - L, y1 - 1, x1, y1, w), scRect(x1 - 1, y1 - L, x1, y1, w);
}
