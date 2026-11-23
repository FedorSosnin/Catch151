// V2: the radial menu (Fedor). scene_v2.h includes this at its end.
//
// A hold on your Pokemon (V2_RADIAL_HOLD_MS, the finger still) opens a ring round the screen in four: Settings
// above, Medals to the right, Stats below and the Pokedex to the left, where the home's swipes put them. Its middle
// is open, so the finger that opened it covers none of it. Slide onto one and lift to open it; lift in the middle
// and the ring stays for a tap (in the middle or outside, it closes; left alone it stays: Fedor, "if I hold the
// Pokemon to pull up the menu, it eventually goes away after a couple seconds"; it went at 8 s). Held on still in the
// middle, the release question comes at 3 s, as before. The ring is see-through, as the icons' band is (Fedor),
// so while it's open the home leaves out what it would show through: the header, the band, the icons, the pill.
//
// Where there's sound (not the Knomi: HAS_MUSIC) the ring is in five, a fifth of a turn each: Stats on top, Medals up
// right, Settings down right, Sound down left (a speaker: the Sound page, sound_v2.h; Fedor: "lets add a new button to
// the menu ... Lets call it Sound with a speaker icon"), the Pokedex up left (Fedor: "lets move Stats to the top, and
// move settings down next to sound").

#define V2_RAD_IN 86    // the ring, from the screen's centre
#define V2_RAD_OUT 204
#define V2_RAD_GAP 7    // between two sections, along the diagonals: |dx| and |dy| this close
#if HAS_MUSIC
#define V2_RAD_N 5      // ...in five (the gaps 5 px either side of the lines between them, their soft edge 3 more)
#else
#define V2_RAD_N 4
#endif

// its icons, as the home's: 18 x 18, drawn at 4x (Fedor: "make the four icons ... larger"; they were 3x, which on
// the Knomi spreads each pixel over a pixel and a half)
static const char *const SPR_V2_RAD_GEAR[18] = {
  "........kk........",
  ".......kssk.......",
  "....k..kssk..k....",
  "...kskksssskksk...",
  "..kssssssssssssk..",
  "...kssssssssssk...",
  "...ksssskkssssk...",
  ".kkssssk..ksssSkk.",
  "ksssssk....ksSSSSk",
  "ksssssk....kSSSSSk",
  ".kkssssk..kSSSSkk.",
  "...ksssskkSSSSk...",
  "...ksssssSSSSSk...",
  "..ksssssSSSSSSSk..",
  "...kskkSSSSkkSk...",
  "....k..kSSk..k....",
  ".......kSSk.......",
  "........kk........",
};
static const char *const SPR_V2_RAD_MEDAL[18] = {
  "..................",
  "..kkkk......kkkk..",
  "..kbbBk....krrRk..",
  "...kbbBk..krrRk...",
  "....kbbBkkrrRk....",
  ".....kbBkkrRk.....",
  "......kkkkkk......",
  ".....kwwffffk.....",
  "....kwffffffYk....",
  "...kwfffOOfffYk...",
  "...kffffOOfffYk...",
  "...kfOOOOOOOOYk...",
  "...kffOOOOOOfYk...",
  "...kfffOOOOffYk...",
  "....kfOOffOOYk....",
  ".....kYffffYk.....",
  "......kkkkkk......",
  "..................",
};
static const char *const SPR_V2_RAD_STATS[18] = {
  "..................",
  "..kkkkkkkkkkkkkk..",
  ".kwwwwwwwwwwwwwwk.",
  ".kwwwwwwwwwkkkwwk.",
  ".kwwwwwwwwwkggkwk.",
  ".kwwwwwkkkwkggkwk.",
  ".kwwwwwkfkwkgGkwk.",
  ".kwwwwwkfkwkgGkwk.",
  ".kwkkkwkfkwkgGkwk.",
  ".kwkrkwkfYwkgGkwk.",
  ".kwkrkwkfYwkgGkwk.",
  ".kwkrRwkfYwkgGkwk.",
  ".kwkrRwkfYwkgGkwk.",
  ".kwkkkkkkkkkkkkwk.",
  ".kwwwwwwwwwwwwwwk.",
  ".kSSSSSSSSSSSSSSk.",
  "..kkkkkkkkkkkkkk..",
  "..................",
};
static const char *const SPR_V2_RAD_DEX[18] = {
  "..................",
  "..kkkkkkkkkkkkkk..",
  ".kRrrrrrrrrrrrrRk.",
  ".krkkkkrrrrrrrrrk.",
  ".krkbbkrrkrkrrrrk.",
  ".krkbBkrrkykkgkrk.",
  ".krkkkkrrrrrrrrrk.",
  ".krrrrrrrrrrrrrrk.",
  ".krkkkkkkkkkkkkrk.",
  ".krkwwwwwwwwwwkrk.",
  ".krkwggwwwwwwwkrk.",
  ".krkwwwwwwwwwwkrk.",
  ".krkwwwwwgggwwkrk.",
  ".krkkkkkkkkkkkkrk.",
  ".krrrrrrrrrrrrrrk.",
  ".kRRRRRRRRRRRRRRk.",
  "..kkkkkkkkkkkkkk..",
  "..................",
};

#if V2_RAD_N == 5
// a speaker in the gear's silver, its sound in white
static const char *const SPR_V2_RAD_SOUND[18] = {
  "..................",
  "........kk........",
  ".......ksk........",
  "......kssk...w....",
  ".....ksssk....w...",
  "....kssssk.....w..",
  "kkkkkssssk.w...w..",
  "kwwssssssk..w..w..",
  "kssssssssk..w..w..",
  "kSSSSSSSSk..w..w..",
  "kSSSSSSSSk..w..w..",
  "kkkkkSSSSk.w...w..",
  "....kSSSSk.....w..",
  ".....kSSSk....w...",
  "......kSSk...w....",
  ".......kSk........",
  "........kk........",
  "..................",
};
static void v2SoundOpen();  // (sound_v2.h)
#endif

static bool v2RadialSticky = false;  // let go in the middle: open for a tap
static uint32_t v2RadialT = 0;       // when it opened, or was let go in the middle

static void v2RadialStart() {
  v2RadialOpen = true;
  v2RadialSticky = false;
  v2RadialHover = -1;
  v2RadialT = millis();
  sfxPlay(SFX_TAP);
}
static void v2RadialClose() {
  v2RadialOpen = v2RadialSticky = false;
  v2RadialHover = -1;
}
// the section of a point (fx, fy) from the centre: in four, 0 up, 1 right, 2 down, 3 left; in five, 0 up, then round
// clockwise (1 up right, 2 down right, 3 down left, 4 up left)
static int v2RadialSection(int fx, int fy) {
#if V2_RAD_N == 5
  float a = atan2f((float)fy, (float)fx) * 57.29578f + 126.0f;  // 0 on the line left of the top one
  if (a < 0) a += 360.0f;
  int s = (int)(a / 72.0f);
  return s > 4 ? 4 : s;
#else
  return abs(fy) > abs(fx) ? (fy < 0 ? 0 : 2) : (fx > 0 ? 1 : 3);
#endif
}
#if V2_RAD_N == 5
// in five: how far (fx, fy) is from the nearest line between two sections (they run out from the centre at -126,
// -54, 18, 90 and 162 degrees)
static float v2RadialLineDist(int fx, int fy) {
  static const float LC[5] = { -0.5878f, 0.5878f, 0.9511f, 0.0f, -0.9511f };
  static const float LS[5] = { -0.8090f, -0.8090f, 0.3090f, 1.0f, 0.3090f };
  float best = 1e9f;
  for (int i = 0; i < 5; i++) {
    if (fx * LC[i] + fy * LS[i] <= 0) continue;  // (the line's own side of the centre)
    float d = fabsf(fx * LS[i] - fy * LC[i]);
    if (d < best) best = d;
  }
  return best;
}
#endif
// the section at (x, y); -1 in the middle or outside (a little past its edge counts)
static int v2RadialAt(int16_t x, int16_t y) {
  int fx = x - CX, fy = y - CY, r2 = fx * fx + fy * fy;
  if (r2 < V2_RAD_IN * V2_RAD_IN || r2 > (V2_RAD_OUT + 16) * (V2_RAD_OUT + 16)) return -1;
  return v2RadialSection(fx, fy);
}
// what each opens: the page its swipe on home opened (V2 has none there: this ring is the way to them)
static void v2RadialPick(int s) {
#if V2_RAD_N == 5
  static const int8_t PICK[5] = { 2, 1, 0, 4, 3 };  // round from the top: Stats, Medals, Settings, Sound, the Pokedex
  s = PICK[s];                                       // (as the four's numbers below; 4 Sound)
  if (s == 4) {  // Sound (not the Knomi)
    v2SoundOpen();
    return;
  }
#endif
  if (s == 0) {  // Settings: the swipe down's
    openClock();
  } else if (s == 1) {  // Medals: the swipe left's
    cardOpen = cardAch = true;
    cardPage = 0;
    achDetail = -1;
    achGroup = -1;  // the trophy room
  } else if (s == 2) {  // Stats: the swipe up's card
    cardOpen = true;
    cardAch = false;
    cardPage = 0;
  } else {  // the Pokedex: the swipe right's
    galleryOpen = true;
    galleryPage = 0;
    galleryDetail = 0;
    galleryDirty = true;
  }
}
// a finger lifting while it's open: on a section, its page; in the middle after the hold that opened it, it stays
// for a tap; anywhere else, it closes
static void v2RadialLift(int16_t x, int16_t y) {
  int s = v2RadialAt(x, y), fx = x - CX, fy = y - CY;
  if (s >= 0) {
    sfxPlay(SFX_TAP);
    v2RadialClose();
    v2RadialPick(s);
  } else if (!v2RadialSticky && fx * fx + fy * fy < V2_RAD_IN * V2_RAD_IN) {
    v2RadialSticky = true;
    v2RadialHover = -1;
    v2RadialT = millis();
  } else {
    v2RadialClose();
  }
}
// an icon at 4x centred at (cx, cy), and its label (in capitals) centred under it or over it (labelY): white on a
// dark 2 px shadow, as the pages' numbers, so it reads on any scene through the ring. lx0..lx1: where the label must
// stay (the ring in five's lower two: inside the ring, so it moves in from under its icon if it's long)
static void v2RadialItem(const char *const *icon, int cx, int cy, const char *label, int labelY, int lx0 = 0,
                         int lx1 = V2_W) {
  drawMap(icon, HOME_ICON_N, cx - 36, cy - 36, 4, false);
  char b[40];
  v2Caps(label, b, sizeof(b));
  setSize(2);
  int x = centerX(b, 2) + (cx - CX), w = textW(b, 2);
  if (x + w > lx1) x = lx1 - w;
  if (x < lx0) x = lx0;
  gfx->setTextColor(C565(0x1b, 0x1b, 0x25));
  setCur(x + 2, labelY + 2);
  printT(b);
  gfx->setTextColor(UI_WHITE);
  setCur(x, labelY);
  printT(b);
}
// The ring's cells, worked out once since it never moves (V2Cells): their kind is the section * 2, + 1 on its soft edge
static V2Cells gRadCells;
static void v2RadialCells() {
  const int ri = V2_RAD_IN, ro = V2_RAD_OUT, rr = ro + 3;
  v2CellsBuild(gRadCells, (CX - rr) & ~1, (CY - rr) & ~1, CX + rr, CY + rr, [&](int x, int y) {
    int fx = x + 1 - CX, fy = y + 1 - CY, r2 = fx * fx + fy * fy;
    if (r2 < (ri - 3) * (ri - 3) || r2 > rr * rr) return -1;
#if V2_RAD_N == 5
    float d = v2RadialLineDist(fx, fy);
    if (d < 5.0f) return -1;  // the gaps between them
    bool rim = r2 < ri * ri || r2 > ro * ro || d < 8.0f;
#else
    int d = abs(abs(fx) - abs(fy));
    if (d < V2_RAD_GAP) return -1;  // the gaps between them
    bool rim = r2 < ri * ri || r2 > ro * ro || d < V2_RAD_GAP + 4;
#endif
    return v2RadialSection(fx, fy) * 2 + rim;
  });
}
// The ring over the dimmed home, see-through as the icons' band is (Fedor: "the same color as the area behind the
// main screen icons"), with its fainter edge; the section under the finger green in the panels' dark rim. Its icons
// and words, the Pokemon seen through the middle. One pass: every pixel dimmed (v2DimFrame(140)) and, on the ring,
// the band's ink over that (the edge's, then inside the inside's), the blends as tables (V2Lut)
static void v2RadialDraw() {
  if (!gRadCells.row) v2RadialCells();
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  const uint16_t green = C565(0x58, 0xb8, 0x68);
  uint16_t col;
  int ra, ta;
  v2BandInk(col, ra, ta);
  V2Lut dim, edge, in;
  v2LutStart(dim);
  v2LutBlend(dim, sc565(V2_DIM_TINT), 140);
  edge = dim;
  v2LutBlend(edge, col, ra);
  in = edge;
  v2LutBlend(in, col, ta);
  for (int y = 0; y < V2_W; y++) {
    uint16_t *row = fb + y * V2_W;
    int x = 0, k = y >> 1;
    if (gRadCells.row)
      for (int i = gRadCells.row[k]; i < gRadCells.row[k + 1]; i++) {
        const V2CellRun &u = gRadCells.run[i];
        int n = 2 * u.cells;
        v2LutRun(row + x, u.x0 - x, dim);
        if (u.kind >> 1 == v2RadialHover) {
          uint16_t c = (u.kind & 1) ? V2_RIM : green;
          for (int j = 0; j < n; j++) row[u.x0 + j] = c;
        } else {
          v2LutRun(row + u.x0, n, (u.kind & 1) ? edge : in);
        }
        x = u.x0 + n;
      }
    v2LutRun(row + x, V2_W - x, dim);
  }
#if V2_RAD_N == 5  // (each icon and its word inside its section and the ring: worked out on their corners. Down
                   // there, the ring's edge at the words' foot, y 386, is 135 px either side of the middle line)
  v2RadialItem(SPR_V2_RAD_STATS, CX, 72, T(S_RAD_STATS), 114);
  v2RadialItem(SPR_V2_RAD_MEDAL, 376, 187, T(S_RAD_MEDALS), 229);
  v2RadialItem(SPR_V2_RAD_GEAR, 330, 330, T(S_RAD_SETTINGS), 372, CX + 7, CX + 133);
  v2RadialItem(SPR_V2_RAD_SOUND, 136, 330, T(S_SOUND), 372, CX - 133, CX - 7);
  v2RadialItem(SPR_V2_RAD_DEX, 90, 187, T(S_DEX_TITLE), 229);
#else
  v2RadialItem(SPR_V2_RAD_GEAR, CX, 72, T(S_SETTINGS), 114);  // (72 px icons: their corners inside the ring's edge)
  v2RadialItem(SPR_V2_RAD_MEDAL, 378, 206, T(S_RAD_MEDALS), 250);
  v2RadialItem(SPR_V2_RAD_STATS, CX, 390, T(S_RAD_STATS), 334);
  v2RadialItem(SPR_V2_RAD_DEX, 88, 206, T(S_DEX_TITLE), 250);
#endif
}
