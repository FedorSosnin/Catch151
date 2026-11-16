// V2: your tag, for the world high-score board. MOCKUPS only for now (TAGMOCK n on a computer's console), until
// Fedor picks. scene_v2.h includes this at its end.
//
// Fedor (2026-11-16): "after a player plays their first game and adds a score, they will be asked to make their tag.
// With a scrolling style letter/number/symbol picker. Then maybe in settings they can switch it or somewhere on the
// games menu". The board shows each game's best with its "score, date, and tag".
//   0: the picker after a first score: three reels, the middle one the one being turned
//   1: the same with four
//   2: the Games menu with your tag on the Game Boy, under its title: a tap there opens the picker
// (v2TagMock, the mockup shown or -1, is with the other screens' flags in the sketch: the console sets it)

// what a reel turns through, in order, round and round: the letters, the digits, then a heart, a smiley, a note and
// some marks the font draws, and a blank (a two-letter tag). (Not 13: the font's note there is a carriage return.)
static const char V2_TAG_CHARS[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\x03\x01\x0e!?&+-.*# ";
#define V2_TAG_NCHARS ((int)sizeof(V2_TAG_CHARS) - 1)
static int v2TagIndex(char c) {
  for (int i = 0; i < V2_TAG_NCHARS; i++)
    if (V2_TAG_CHARS[i] == c) return i;
  return 0;
}
static char v2TagAt(int i) { return V2_TAG_CHARS[((i % V2_TAG_NCHARS) + V2_TAG_NCHARS) % V2_TAG_NCHARS]; }

#define V2_TAG_CY 216      // the reels' middle: the character chosen sits there
#define V2_TAG_Y0 146      // the reels' window
#define V2_TAG_Y1 286
#define V2_TAG_STEP 50     // from one character to the next on a reel
#define V2_TAG_GOLD C565(0xf0, 0xc0, 0x4a)

static void v2TagGlyph(char c, int cx, int cy, int size, uint16_t col) {  // one character centred on (cx, cy)
  char s[2] = { c, 0 };
  gfx->setTextColor(col);
  setSize(size);
  setCur(SNAP2(cx - (5 * size) / 2), SNAP2(cy - (7 * size) / 2));
  printT(s);
}
static void v2TagArrow(int cx, int cy, bool up, uint16_t col) {  // a small solid triangle
  for (int r = 0; r < 8; r++) {
    int w = up ? r : 7 - r;
    gfx->fillRect(cx - w * 2 - 1, cy - 8 + r * 2, w * 4 + 2, 2, col);
  }
}
// one reel: c the character chosen, roll how far it's turned toward the next (-1..1, a swipe under way), hot the reel
// the arrow keys turn on a computer (a gold rim)
static void v2TagReel(int cx, int w, char c, float roll, bool hot) {
  int x0 = cx - w / 2;
  gfx->fillRoundRect(x0 - 3, V2_TAG_Y0 - 3, w + 6, V2_TAG_Y1 - V2_TAG_Y0 + 6, 10, hot ? V2_TAG_GOLD : V2_RIM);
  gfx->fillRoundRect(x0, V2_TAG_Y0, w, V2_TAG_Y1 - V2_TAG_Y0, 8, C565(0x1e, 0x25, 0x32));  // darker at its ends,
  gfx->fillRect(x0, V2_TAG_Y0 + 18, w, V2_TAG_Y1 - V2_TAG_Y0 - 36, C565(0x26, 0x2f, 0x3e));    // curving away, and
  gfx->fillRect(x0, V2_TAG_CY - 40, w, 80, C565(0x30, 0x3a, 0x4c));  // lit where it faces you, as a drum is
  int ci = v2TagIndex(c), off = (int)lroundf(roll * V2_TAG_STEP);
  // the slot the chosen one sits in: white, a dark rim
  gfx->fillRoundRect(x0 + 4, V2_TAG_CY - 26, w - 8, 52, 6, V2_RIM);
  gfx->fillRoundRect(x0 + 6, V2_TAG_CY - 24, w - 12, 48, 5, UI_WHITE);
  bool cjk = classicFontBegin();
  for (int k = -2; k <= 2; k++) {
    int y = V2_TAG_CY + k * V2_TAG_STEP + off;
    if (y < V2_TAG_Y0 || y > V2_TAG_Y1) continue;  // (one turning in or out: half of it, under the panel's white)
    bool mid = y > V2_TAG_CY - 24 && y < V2_TAG_CY + 24;
    char ch = v2TagAt(ci + k);
    if (ch == ' ') {  // the blank: a short line where a character would be
      gfx->fillRect(cx - 10, y + 10, 20, 4, mid ? UI_INK : C565(0x6a, 0x76, 0x88));
      continue;
    }
    v2TagGlyph(ch, cx, y, mid ? 6 : 4, mid ? UI_INK : C565(0x9a, 0xa8, 0xbc));
  }
  classicFontEnd(cjk);
  // what turned out of the window is under the panel again
  gfx->fillRect(x0 - 3, V2_TAG_Y0 - 19, w + 6, 16, UI_WHITE);
  gfx->fillRect(x0 - 3, V2_TAG_Y1 + 3, w + 6, 16, UI_WHITE);
  gfx->fillRoundRect(x0 - 3, V2_TAG_Y0 - 3, w + 6, 6, 3, hot ? V2_TAG_GOLD : V2_RIM);
  gfx->fillRoundRect(x0 - 3, V2_TAG_Y1 - 3, w + 6, 6, 3, hot ? V2_TAG_GOLD : V2_RIM);
  uint16_t a = hot ? V2_TAG_GOLD : C565(0x8a, 0x96, 0xa8);
  v2TagArrow(cx, V2_TAG_Y0 - 12, true, a);
  v2TagArrow(cx, V2_TAG_Y1 + 20, false, a);
}
// the picker: YOUR TAG on its board, a line on what it's for, the reels, OK, and the X for not now
static void v2TagPicker(const char *tag, int hot, float roll) {
  int n = (int)strlen(tag), w = n == 3 ? 70 : 58, gap = n == 3 ? 16 : 12, span = n * w + (n - 1) * gap;
  v2DimFrame(115);
  v2Sign("YOUR TAG", 60);
  v2Panel(70, 100, 396, 318, 12);
  gfx->setTextColor(V2_INK_MUTED);
  setSize(2);
  const char *why = "ON THE WORLD BOARD";
  setCur(centerX(why, 2), 112);
  printT(why);
  for (int i = 0; i < n; i++) v2TagReel(CX - span / 2 + w / 2 + i * (w + gap), w, tag[i], i == hot ? roll : 0, i == hot);
  v2LabelButton(150, 332, 316, 376, "OK", C565(0x58, 0xb8, 0x68));
  v2CloseX();
}
// the Games menu with your tag on the Game Boy, under GAMES: a label stuck on it, a pencil beside
static void v2TagGamesChip(const char *tag) {
  v2GamesTv();
  bool cjk = classicFontBegin();
  setSize(2);
  int tw = textW(tag, 2) - 2, w = tw + 40, x0 = SNAP2(CX - w / 2), y0 = 62;
  gfx->fillRoundRect(x0 - 2, y0 - 2, w + 4, 26, 6, V2_RIM);
  gfx->fillRoundRect(x0, y0, w, 22, 5, C565(0xf4, 0xec, 0xd8));
  gfx->setTextColor(UI_INK);
  setCur(x0 + 8, y0 + 4);
  printT(tag);
  int px = x0 + w - 20, py = y0 + 4;  // the pencil: a yellow body, a pink end, a dark tip, at 45 degrees
  for (int i = 0; i < 6; i++) gfx->fillRect(px + 10 - i * 2, py + i * 2, 4, 4, i < 1 ? C565(0xe8, 0x80, 0x90) : i < 5 ? C565(0xf0, 0xc0, 0x4a) : UI_INK);
  classicFontEnd(cjk);
}
static void v2TagMockDraw(int n) {
  if (n == 2) {
    v2TagGamesChip("FED");
    return;
  }
  v2MenuBackdrop();
  v2TagPicker(n == 1 ? "PIKA" : "FED", 1, 0);
}
