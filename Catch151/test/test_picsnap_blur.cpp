// Pic Snap's blur and its dither's pick (picsnap_blur.h), held to the first ones bit for bit: the first ones are here
// as they were (picsnap_v2.h until 2026-11-23), and both are run on the same pictures, every size and blur the game
// uses and some it never will.
#include "framework.h"
#include <stdlib.h>
#include <vector>
#include "../src/picsnap_blur.h"

namespace {

uint32_t rng = 1;
uint32_t rnd() {
  rng ^= rng << 13, rng ^= rng >> 17, rng ^= rng << 5;
  return rng;
}

// ----- the first blur, as it was -----
void oldBoxRow(uint8_t *p, int w, int r, uint8_t *tmp) {
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
void oldBoxCols(uint8_t *p, int w, int h, int r, uint8_t *ring, uint16_t *sum) {
  int n = 2 * r + 1, keep = r + 1;
  uint32_t inv = 65536u / n;
  for (int x = 0; x < w; x++) sum[x] = 0;
  for (int k = -r; k <= r; k++) {
    const uint8_t *row = p + (k < 0 ? 0 : k >= h ? h - 1 : k) * w;
    for (int x = 0; x < w; x++) sum[x] += row[x];
  }
  for (int y = 0; y < h; y++) {
    uint8_t *row = p + y * w;
    memcpy(ring + (y % keep) * w, row, w);
    const uint8_t *add = p + (y + r + 1 >= h ? h - 1 : y + r + 1) * w;
    int s = y - r;
    const uint8_t *sub = s < 0 ? ring : ring + (s % keep) * w;
    if (s < 0) sub = ring + 0;
    for (int x = 0; x < w; x++) {
      row[x] = (uint8_t)((sum[x] * inv + 32768) >> 16);
      sum[x] += add[x] - sub[x];
    }
  }
}
void oldBlur(uint8_t *p, int w, int h, int r) {
  if (r <= 0) return;
  std::vector<uint8_t> ring((size_t)(r + 1) * w), tmp(w);
  std::vector<uint16_t> sums(w);
  for (int pass = 0; pass < 2; pass++)
    for (int y = 0; y < h; y++) oldBoxRow(p + y * w, w, r, tmp.data());
  for (int pass = 0; pass < 2; pass++) oldBoxCols(p, w, h, r, ring.data(), sums.data());
}

// ----- the first pick, as it was (its palette passed in) -----
uint8_t oldPick(const uint8_t (*palC)[3], int r, int g, int b, int cx, int cy, int n) {
  int i1 = 0, i2 = 0;
  long d1 = 1L << 30, d2 = 1L << 30;
  for (int i = 0; i < n; i++) {
    int dr = r - palC[i][0], dg = g - palC[i][1], db = b - palC[i][2];
    long d = 2L * dr * dr + 4L * dg * dg + 3L * db * db;
    if (d < d1) d2 = d1, i2 = i1, d1 = d, i1 = i;
    else if (d < d2) d2 = d, i2 = i;
  }
  if (n < 2 || d1 == 0) return (uint8_t)i1;
  int ar = palC[i2][0] - palC[i1][0], ag = palC[i2][1] - palC[i1][1], ab = palC[i2][2] - palC[i1][2];
  long den = 2L * ar * ar + 4L * ag * ag + 3L * ab * ab;
  if (den <= 0) return (uint8_t)i1;
  float t = (2.0f * (r - palC[i1][0]) * ar + 4.0f * (g - palC[i1][1]) * ag + 3.0f * (b - palC[i1][2]) * ab) / den;
  static const float BAYER[2][2] = { { 0.125f, 0.625f }, { 0.875f, 0.375f } };
  return (uint8_t)(t > BAYER[cy & 1][cx & 1] ? i2 : i1);
}
uint8_t newPick(PsPickCache &c, const uint8_t (*palC)[3], int r, int g, int b, int cx, int cy, int n) {
  PsPick p = psPickCached(c, palC, n, r, g, b);
  return (p.mask >> ((cy & 1) * 2 + (cx & 1)) & 1) ? p.i2 : p.i1;
}

// ----- the first colour sums, as they were (picsnap_v2.h's psCh(), psSetCh() and the lens step's loops) -----
uint8_t oldCh(uint16_t c, int ch) {
  if (ch == 0) { int r = c >> 11; return (uint8_t)(r << 3 | r >> 2); }
  if (ch == 1) { int g = (c >> 5) & 63; return (uint8_t)(g << 2 | g >> 4); }
  int b = c & 31;
  return (uint8_t)(b << 3 | b >> 2);
}
uint16_t oldSetCh(uint16_t c, int ch, int v) {
  v = v < 0 ? 0 : v > 255 ? 255 : v;
  if (ch == 0) return (uint16_t)((c & 0x07FF) | (v >> 3) << 11);
  if (ch == 1) return (uint16_t)((c & 0xF81F) | (v >> 2) << 5);
  return (uint16_t)((c & 0xFFE0) | v >> 3);
}

// pictures to blur: noise, hard shapes (a place's grass, a tombstone's edge), a smooth ramp, all one colour
void fill(std::vector<uint8_t> &p, int w, int h, int kind) {
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      uint8_t v;
      switch (kind) {
        case 0: v = (uint8_t)rnd(); break;
        case 1: v = ((x / 7 + y / 5) & 1) ? 255 : (x * 3 + y) % 40; break;
        case 2: v = (uint8_t)((x * 255) / (w > 1 ? w - 1 : 1) / 2 + (y * 255) / (h > 1 ? h - 1 : 1) / 2); break;
        default: v = 173; break;
      }
      p[(size_t)y * w + x] = v;
    }
}

}  // namespace

TEST(picsnap_blur, the_streamed_blur_is_the_first_one_bit_for_bit_at_every_size_and_radius) {
  // the places' planes (297 x 297, up to 6 px), the Pokemon's little picture (up to 200 x 224, up to 8 px), and
  // pictures smaller than the blur itself
  const int sizes[][2] = { { 297, 297 }, { 200, 224 }, { 63, 41 }, { 17, 17 }, { 5, 40 }, { 40, 5 },
                           { 3, 3 },     { 1, 9 },     { 9, 1 },   { 1, 1 },   { 2, 30 } };
  std::vector<uint8_t> mem(psBlurMem(297, 8));
  int checked = 0;
  for (auto &s : sizes) {
    const int w = s[0], h = s[1];
    for (int r = 1; r <= 8; r++)
      for (int kind = 0; kind < 4; kind++) {
        if (w * h > 20000 && kind == 3) continue;
        std::vector<uint8_t> a((size_t)w * h), b;
        fill(a, w, h, kind);
        b = a;
        oldBlur(a.data(), w, h, r);
        psBlurPlane(b.data(), w, h, r, mem.data());
        CHECK_MSG(a == b, "the blur differs from the first one");
        checked++;
      }
  }
  CHECK(checked > 300);
}

TEST(picsnap_blur, read_from_one_place_and_written_to_another_as_the_place_is_made) {
  // as picsnap_v2.h makes a lens step: a channel read out of a 565 plane, blurred, its result laid into another
  const int W = 297;
  std::vector<uint16_t> src((size_t)W * W), outOld((size_t)W * W, 0), outNew((size_t)W * W, 0);
  for (auto &v : src) v = (uint16_t)rnd();
  std::vector<uint8_t> mem(psBlurMem(W, 6));
  for (int r = 0; r <= 6; r++) {
    std::vector<uint8_t> work((size_t)W * W);
    for (size_t i = 0; i < work.size(); i++) work[i] = (uint8_t)(src[i] >> 5);
    oldBlur(work.data(), W, W, r);
    for (size_t i = 0; i < work.size(); i++) outOld[i] = (uint16_t)(work[i] + r);
    psBlurStream(
        W, W, r, mem.data(),
        [&](int y, uint8_t *row) {
          for (int x = 0; x < W; x++) row[x] = (uint8_t)(src[(size_t)y * W + x] >> 5);
        },
        [&](int y, const uint8_t *row) {
          for (int x = 0; x < W; x++) outNew[(size_t)y * W + x] = (uint16_t)(row[x] + r);
        });
    CHECK_MSG(outOld == outNew, "a lens step's channel differs");
  }
}

TEST(picsnap_blur, the_quick_colour_loops_are_the_first_ones_bit_for_bit) {
  for (int x = 0; x <= 255 * 255; x++)  // (all it is asked: a channel times a cover)
    if (psDiv255(x) != x / 255) {
      CHECK_MSG(false, "psDiv255 differs from / 255");
      return;
    }
  const int W = 297;
  std::vector<uint16_t> c(W), n(W), o(W), oNew(W);
  std::vector<uint8_t> a(W), cv(W), row(W), rowNew(W);
  for (int pass = 0; pass < 200; pass++) {
    for (int x = 0; x < W; x++) {
      c[x] = (uint16_t)rnd(), n[x] = (uint16_t)rnd(), o[x] = (uint16_t)rnd(), row[x] = (uint8_t)rnd();
      a[x] = pass % 3 == 0 ? (uint8_t)(rnd() & 1 ? 255 : 0) : (uint8_t)rnd();
      cv[x] = pass % 4 == 0 ? 0 : (uint8_t)rnd();
    }
    for (int ch = 0; ch < 3; ch++) {
      psRowCh(c.data(), rowNew.data(), W, ch);  // a channel out
      for (int x = 0; x < W; x++) CHECK_EQ(rowNew[x], oldCh(c[x], ch));
      oNew = o;  // a channel in
      psRowSetCh(oNew.data(), row.data(), W, ch);
      for (int x = 0; x < W; x++) CHECK_EQ(oNew[x], ch == 0 ? oldSetCh(0, 0, row[x]) : oldSetCh(o[x], ch, row[x]));
      psRowNear(n.data(), a.data(), rowNew.data(), W, ch);  // a near channel times its cover
      for (int x = 0; x < W; x++) CHECK_EQ(rowNew[x], (uint8_t)(oldCh(n[x], ch) * a[x] / 255));
      oNew = o;  // ...laid over by its cover
      psRowOver(oNew.data(), row.data(), cv.data(), W, ch);
      for (int x = 0; x < W; x++)
        CHECK_EQ(oNew[x], cv[x] ? oldSetCh(o[x], ch, row[x] + oldCh(o[x], ch) * (255 - cv[x]) / 255) : o[x]);
    }
  }
}

TEST(picsnap_blur, the_kept_pick_is_the_first_one_for_every_colour_and_cell) {
  static PsPickCache c;
  memset(&c, 0, sizeof(c));
  psPickForget(c);
  uint8_t palC[96][3];
  int checked = 0;
  for (int pal = 0; pal < 40; pal++) {
    // a palette: a place's (up to 96), sometimes with the same colour twice or the same distance to two of them
    int n = pal < 3 ? pal : 2 + (int)(rnd() % 95);
    for (int i = 0; i < n; i++)
      for (int ch = 0; ch < 3; ch++) palC[i][ch] = (uint8_t)(rnd() % (pal % 4 == 0 ? 4 : 256) * (pal % 4 == 0 ? 85 : 1));
    if (n > 4) memcpy(palC[n - 1], palC[1], 3);
    psPickForget(c);
    int r = 0, g = 0, b = 0;
    for (int k = 0; k < 20000; k++) {
      if (k % 4 == 1) {
        // the cell before's colour again (the most often of all)
      } else if (k % 3 == 0 && n > 0) {  // the palette's own colours and near them
        int i = (int)(rnd() % n);
        r = palC[i][0] + (int)(rnd() % 5) - 2, g = palC[i][1] + (int)(rnd() % 5) - 2, b = palC[i][2] + (int)(rnd() % 5) - 2;
        r = r < 0 ? 0 : r > 255 ? 255 : r, g = g < 0 ? 0 : g > 255 ? 255 : g, b = b < 0 ? 0 : b > 255 ? 255 : b;
      } else {
        r = (int)(rnd() % 256), g = (int)(rnd() % 256), b = (int)(rnd() % 256);
      }
      int cx = (int)(rnd() % 150), cy = (int)(rnd() % 150);
      if (oldPick(palC, r, g, b, cx, cy, n) != newPick(c, palC, r, g, b, cx, cy, n)) {
        CHECK_MSG(false, "the kept pick differs from the first one");
        return;
      }
      checked++;
    }
  }
  CHECK(checked > 500000);
}

TEST(picsnap_blur, a_new_palette_forgets_what_was_kept_even_round_the_turns) {
  static PsPickCache c;
  memset(&c, 0, sizeof(c));
  psPickForget(c);
  uint8_t a[2][3] = { { 0, 0, 0 }, { 255, 255, 255 } }, b[2][3] = { { 255, 0, 0 }, { 0, 0, 255 } };
  uint8_t first = newPick(c, a, 10, 10, 10, 0, 0, 2);
  CHECK_EQ(first, 0);
  for (int k = 0; k < 600; k++) {  // more than 255 palettes (the turn goes round): never one kept from another
    psPickForget(c);
    const uint8_t (*pal)[3] = k & 1 ? a : b;
    for (int v = 0; v < 256; v += 15)
      for (int cell = 0; cell < 4; cell++)
        if (oldPick(pal, v, 0, 255 - v, cell & 1, cell >> 1, 2) != newPick(c, pal, v, 0, 255 - v, cell & 1, cell >> 1, 2)) {
          CHECK_MSG(false, "a pick kept from another palette");
          return;
        }
  }
  CHECK(c.turn != 0);
}
