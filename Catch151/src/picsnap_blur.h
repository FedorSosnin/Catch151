// Pic Snap's blur and its dither's pick (picsnap_v2.h uses them), on their own so the tests can hold them to the first
// ones, bit for bit (test/test_picsnap_blur.cpp). No Arduino here: plain bytes in, plain bytes out.
//
// Why they're built this way (measured on the XL, 2026-11-23): a place took 4.4 s to make ready on every board, 3.8 s
// of it the blur. The first blur worked on whole 88 KB planes in PSRAM, a byte at a time, each pixel through it about
// ten times; the pick searched the whole palette for each of the dither's cells.
#pragma once
#include <stdint.h>
#include <string.h>

// ----- the blur -----
// One channel (w x h, r px): a box twice across and twice down, close to a Gaussian; each box clamped at the edges
// (the edge pixel goes on past it) and rounded to a byte, (sum * inv + 32768) >> 16 with inv = 65536 / (2r + 1).
// Streamed: each row read once (src(y, row)), both boxes across it, then the two boxes down one after the other, each
// a running sum per column over a ring of its last rows; each row written once, as soon as it's done (sink(y, row)).
// All of it in mem (psBlurMem() bytes: the boards' internal RAM, quick a byte at a time where PSRAM isn't). A row is
// written only after it's been read, never read again: src and sink can be the same plane.

// the rows a ring keeps: from y - r to y + r + 1
static inline int psBlurRing(int r) { return 2 * r + 2; }
// the room a blur up to w wide and r px needs
static inline size_t psBlurMem(int w, int r) {
  return (size_t)w * 2 * sizeof(uint16_t) + (size_t)w * 2 + (size_t)w * 2 * psBlurRing(r);
}

// one box across a row (in to out, w px)
static inline void psBoxRowTo(const uint8_t *in, uint8_t *out, int w, int r) {
  const int n = 2 * r + 1;
  const uint32_t inv = 65536u / n;
  int sum = 0;
  for (int k = -r; k <= r; k++) sum += in[k < 0 ? 0 : k >= w ? w - 1 : k];
  int x = 0;
  const int mid0 = r, mid1 = w - r - 1;  // x in mid0..mid1-1: neither end of the box past the row's
  for (; x < w && x < mid0; x++) {
    out[x] = (uint8_t)((sum * inv + 32768) >> 16);
    int add = x + r + 1, sub = x - r;
    sum += in[add >= w ? w - 1 : add] - in[sub < 0 ? 0 : sub];
  }
  for (; x < mid1; x++) {
    out[x] = (uint8_t)((sum * inv + 32768) >> 16);
    sum += in[x + r + 1] - in[x - r];
  }
  for (; x < w; x++) {
    out[x] = (uint8_t)((sum * inv + 32768) >> 16);
    int add = x + r + 1, sub = x - r;
    sum += in[add >= w ? w - 1 : add] - in[sub < 0 ? 0 : sub];
  }
}

// src(y, row): fill row (w bytes) with row y; sink(y, row): row y blurred (row is mem's, gone after the call)
template <class Src, class Sink>
static void psBlurStream(int w, int h, int r, uint8_t *mem, Src src, Sink sink) {
  if (w <= 0 || h <= 0) return;
  if (r <= 0) {  // no blur: each row through as it is
    for (int y = 0; y < h; y++) {
      src(y, mem);
      sink(y, mem);
    }
    return;
  }
  const int R = psBlurRing(r), n = 2 * r + 1;
  const uint32_t inv = 65536u / n;
  uint16_t *s1 = (uint16_t *)mem, *s2 = s1 + w;  // the running sums down the columns, the first box's and the second's
  uint8_t *ta = (uint8_t *)(s2 + w), *tb = ta + w;
  uint8_t *ringH = tb + w, *ringV = ringH + (size_t)R * w;  // the rows across (both boxes), the first box down's
  int hDone = 0, vDone = 0;                                 // how many of each are made
  auto cl = [h](int j) { return j < 0 ? 0 : j >= h ? h - 1 : j; };
  auto rowH = [&](int j) { return ringH + (size_t)(j % R) * w; };
  auto rowV = [&](int j) { return ringV + (size_t)(j % R) * w; };
  auto needH = [&](int j) {  // the rows across made up to j
    for (; hDone <= j; hDone++) {
      src(hDone, ta);
      psBoxRowTo(ta, tb, w, r);
      psBoxRowTo(tb, rowH(hDone), w, r);
    }
  };
  auto needV = [&](int j) {  // the first box down made up to row j
    for (; vDone <= j; vDone++) {
      const int y = vDone;
      if (y == 0) {
        needH(cl(r));
        memset(s1, 0, (size_t)w * sizeof(uint16_t));
        for (int k = -r; k <= r; k++) {
          const uint8_t *row = rowH(cl(k));
          for (int x = 0; x < w; x++) s1[x] += row[x];
        }
      }
      uint8_t *o = rowV(y);
      for (int x = 0; x < w; x++) o[x] = (uint8_t)((s1[x] * inv + 32768) >> 16);
      if (y + 1 < h) {
        needH(cl(y + r + 1));
        const uint8_t *add = rowH(cl(y + r + 1)), *sub = rowH(cl(y - r));
        for (int x = 0; x < w; x++) s1[x] = (uint16_t)(s1[x] + add[x] - sub[x]);
      }
    }
  };
  for (int y = 0; y < h; y++) {  // the second box down, each row out as it's done
    if (y == 0) {
      needV(cl(r));
      memset(s2, 0, (size_t)w * sizeof(uint16_t));
      for (int k = -r; k <= r; k++) {
        const uint8_t *row = rowV(cl(k));
        for (int x = 0; x < w; x++) s2[x] += row[x];
      }
    }
    for (int x = 0; x < w; x++) ta[x] = (uint8_t)((s2[x] * inv + 32768) >> 16);
    sink(y, ta);
    if (y + 1 < h) {
      needV(cl(y + r + 1));
      const uint8_t *add = rowV(cl(y + r + 1)), *sub = rowV(cl(y - r));
      for (int x = 0; x < w; x++) s2[x] = (uint16_t)(s2[x] + add[x] - sub[x]);
    }
  }
}

// a plane blurred where it lies
static inline void psBlurPlane(uint8_t *p, int w, int h, int r, uint8_t *mem) {
  if (r <= 0) return;
  psBlurStream(
      w, h, r, mem, [&](int y, uint8_t *row) { memcpy(row, p + (size_t)y * w, w); },
      [&](int y, const uint8_t *row) { memcpy(p + (size_t)y * w, row, w); });
}

// ----- the dither's pick -----
// Of the palette's first n colours (palC: each one's 8-bit r, g, b), the two nearest to (r, g, b), and how far it lies
// from the nearest toward the other: which of the two each of the checker's four cells gets. Bit (cy & 1) * 2 + (cx & 1)
// of mask set: the other (i2), clear: the nearest (i1).
struct PsPick {
  uint8_t i1, i2, mask;
};
static const float PS_BAYER[2][2] = { { 0.125f, 0.625f }, { 0.875f, 0.375f } };
static inline PsPick psPickWork(const uint8_t (*palC)[3], int n, int r, int g, int b) {
  int i1 = 0, i2 = 0;
  long d1 = 1L << 30, d2 = 1L << 30;
  for (int i = 0; i < n; i++) {
    int dr = r - palC[i][0], dg = g - palC[i][1], db = b - palC[i][2];
    long d = 2L * dr * dr + 4L * dg * dg + 3L * db * db;
    if (d < d1) d2 = d1, i2 = i1, d1 = d, i1 = i;
    else if (d < d2) d2 = d, i2 = i;
  }
  PsPick p = { (uint8_t)i1, (uint8_t)i1, 0 };
  if (n < 2 || d1 == 0) return p;
  int ar = palC[i2][0] - palC[i1][0], ag = palC[i2][1] - palC[i1][1], ab = palC[i2][2] - palC[i1][2];
  long den = 2L * ar * ar + 4L * ag * ag + 3L * ab * ab;
  if (den <= 0) return p;
  float t = (2.0f * (r - palC[i1][0]) * ar + 4.0f * (g - palC[i1][1]) * ag + 3.0f * (b - palC[i1][2]) * ab) / den;
  p.i2 = (uint8_t)i2;
  for (int k = 0; k < 4; k++)
    if (t > PS_BAYER[k >> 1][k & 1]) p.mask |= (uint8_t)(1 << k);
  return p;
}

// The same, each colour's answer kept: a blurred place has the same few thousand colours over and over. Kept for one
// palette: psPickForget() when it changes.
#define PS_PICK_BITS 10
#define PS_PICK_N (1 << PS_PICK_BITS)
struct PsPickCache {
  uint32_t key[PS_PICK_N];  // the colour (r << 16 | g << 8 | b) and the palette's turn (<< 24); 0: nothing kept
  PsPick val[PS_PICK_N];
  uint8_t turn;  // the palette's (1..255)
};
static inline void psPickForget(PsPickCache &c) {
  if (++c.turn == 0) {  // round again: everything kept goes
    memset(c.key, 0, sizeof(c.key));
    c.turn = 1;
  }
}
static inline PsPick psPickCached(PsPickCache &c, const uint8_t (*palC)[3], int n, int r, int g, int b) {
  uint32_t rgb = (uint32_t)r << 16 | (uint32_t)g << 8 | (uint32_t)b, key = rgb | (uint32_t)c.turn << 24;
  uint32_t i = (rgb * 2654435761u) >> (32 - PS_PICK_BITS);
  if (c.key[i] == key) return c.val[i];
  PsPick p = psPickWork(palC, n, r, g, b);
  c.key[i] = key;
  c.val[i] = p;
  return p;
}
