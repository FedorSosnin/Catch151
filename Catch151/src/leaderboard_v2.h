// V2: the leaderboard on the Game Boy (2.2.15; Fedor, 2026-11-19: "add our leaderboard in a section to the left of the
// games ... make each game and current record look like old school games on that green Gameboy screen"; of three
// mockups, "B is cool! Let's have them appear in order they appear as cartridges", the first with a crown). A page a
// game to the left of the Games menu's cartridges (menus_v2.h: v2GamesPage -1 Berry Drop's, on to -8 BerryMatch's), the
// Game Boy's screen drawn in four shades of its own green as the old games were: a game's title screen, its name big,
// its cartridge's picture at 2x in a box as Pokemon Red and Blue drew theirs, the best and what it is, who set it and
// when (a crown before the tag), the second and third, and TAP TO PLAY, which starts it, as the old title screens'
// PUSH START (only that line: a swipe anywhere else turns the page). The best three come from the relay (BM_BOARD: buddy.h boardFrom(), Catch151.ino boardAsk()), asked for
// when the menu opens. Who's That? keeps no score: its page says so. On the Games' first page LEADERBOARDS runs down
// the screen's left edge behind the cartridges (Fedor: "the text 'leaderboards' with an arrow ... in that same green
// color, text turned sideways top to bottom. So you see the carts but know that left is leaderboards").

// the screen's four shades: 0 the screen itself (v2PaintGb()'s), 3 the darkest
static const Rgb LB_G[4] = { 0xA6BA3C, 0x7A9430, 0x466424, 0x1C3412 };
static uint16_t lbG(int i) { return sc565(LB_G[i]); }

static void lbText(const char *s, int x, int y, int size, int shade) {
  gfx->setTextColor(lbG(shade));
  setSize(size);
  setCur(x, y);
  printT(s);
}
// (measured at its size: Japanese and Korean are measured in the font as it's set)
static void lbTextR(const char *s, int xr, int y, int size, int shade) {
  setSize(size);
  lbText(s, xr - textW(s, size), y, size, shade);
}
static void lbTextC(const char *s, int y, int size, int shade) {
  setSize(size);
  lbText(s, SNAP2(CX - textW(s, size) / 2), y, size, shade);
}
// in capitals (the Latin letters), centred, in the language's font
static void lbCapsC(const char *s, int y, int size, int shade) {
  char caps[64];
  lbTextC(v2Caps(s, caps, sizeof(caps)), y, size, shade);
}

// a cartridge's picture (v2Lbl()) at 2x in the four shades, kept until another is wanted (and again each minute: the
// pictures follow the hour's light). The shades split the picture's own light and dark: its darkest 16 in 100 the
// darkest (no lighter than what's dark: Who's That?'s silhouette, not the darker rays round it), and so on
#define LB_PIC_W (2 * V2_LBL_W)
#define LB_PIC_H (2 * V2_LBL_H)
static uint16_t *lbPic = nullptr;
static int32_t lbPicKey = -1;
static void lbGreenPic(int lbl, int x, int y) {
  const uint16_t *a = v2Lbl(lbl);
  if (!a) return;
  if (!lbPic) lbPic = (uint16_t *)ps_malloc(LB_PIC_W * LB_PIC_H * sizeof(uint16_t));
  if (!lbPic) return;
  int32_t key = lbl | (int32_t)(millis() / 60000UL % 100000UL) << 8;
  if (key != lbPicKey) {
    lbPicKey = key;
    static uint8_t L[V2_LBL_W * V2_LBL_H];
    int hist[256] = {};
    for (int i = 0; i < V2_LBL_W * V2_LBL_H; i++) {
      uint16_t c = a[i];
      int r = (c >> 11) << 3, g = ((c >> 5) & 63) << 2, b = (c & 31) << 3;
      L[i] = (uint8_t)((r * 77 + g * 150 + b * 29) >> 8);
      hist[L[i]]++;
    }
    static const int CUT[3] = { 16, 42, 70 };  // in 100: the darkest shade's share, then the next two's with it
    int th[3], n = 0, k = 0;
    for (int v = 0; v < 256 && k < 3; v++) {
      n += hist[v];
      while (k < 3 && n * 100 >= CUT[k] * V2_LBL_W * V2_LBL_H) th[k++] = v;
    }
    while (k < 3) th[k++] = 255;
    if (th[0] > 90) th[0] = 90;
    for (int i = 1; i < 3; i++)
      if (th[i] <= th[i - 1]) th[i] = th[i - 1] + 1;
    for (int j = 0; j < V2_LBL_H; j++)
      for (int i = 0; i < V2_LBL_W; i++) {
        int v = L[j * V2_LBL_W + i];
        uint16_t c = lbG(v <= th[0] ? 3 : v <= th[1] ? 2 : v <= th[2] ? 1 : 0);
        uint16_t *q = lbPic + 2 * j * LB_PIC_W + 2 * i;
        q[0] = q[1] = q[LB_PIC_W] = q[LB_PIC_W + 1] = c;
      }
  }
  gfx->draw16bitRGBBitmap(x, y, lbPic, LB_PIC_W, LB_PIC_H);
}
// a box as Pokemon Red and Blue drew theirs: a dark line, a light one, a dark one, then the screen
static void lbFrame(int x, int y, int w, int h) {
  gfx->fillRect(x, y, w, h, lbG(3));
  gfx->fillRect(x + 2, y + 2, w - 4, h - 4, lbG(0));
  gfx->fillRect(x + 4, y + 4, w - 8, h - 8, lbG(3));
  gfx->fillRect(x + 6, y + 6, w - 12, h - 12, lbG(0));
}
// a row of dots, 2 x 2 every 6 px
static void lbDots(int x0, int x1, int y, int shade) {
  for (int x = x0; x < x1; x += 6) gfx->fillRect(x, y, 2, 2, lbG(shade));
}
// the menus' pointer: a solid triangle, h px tall, pointing right (dir 1) or left (-1) from x
static void lbPointer(int x, int y, int h, int shade, int dir = 1) {
  for (int j = 0; j < h / 2; j += 2) {
    int w = 2 + j, x0 = dir > 0 ? x : x - w;
    gfx->fillRect(x0, y + j, w, 2, lbG(shade));
    gfx->fillRect(x0, y + h - 2 - j, w, 2, lbG(shade));
  }
}
// the first's crown, in 2 px cells (# the darkest, o the screen showing through): 22 x 14, as tall as the letters
static const char *const LB_CROWN[7] = {
  "#....#....#",
  "##..###..##",
  "###.###.###",
  "###########",
  "#o###o###o#",
  "###########",
  ".#########.",
};
#define LB_CROWN_W 22
static void lbCrown(int x, int y) {
  for (int j = 0; j < 7; j++)
    for (int i = 0; LB_CROWN[j][i]; i++)
      if (LB_CROWN[j][i] != '.') gfx->fillRect(x + i * 2, y + j * 2, 2, 2, lbG(LB_CROWN[j][i] == '#' ? 3 : 0));
}
// your tag (the gamer tag you have now) lit: dark, with the screen's colour in it
static bool lbMine(const char *tag) { return tag[0] && pet.gamerTag[0] && !strcmp(tag, pet.gamerTag); }
static void lbTag(const char *tag, int x, int y, int shade) {
  if (lbMine(tag)) {
    gfx->fillRect(x - 4, y - 2, textW(tag, 2) + 8, 18, lbG(3));
    lbText(tag, x, y, 2, 0);
  } else {
    lbText(tag, x, y, 2, shade);
  }
}

// the play line: its pointer and words, centred at y 352; its box for a tap (a finger's width round it)
#define LB_PLAY_Y 352
static const char *lbPlayWords(char *buf, int n) { return v2Caps(T(playKeysUi ? S_LB_PLAY_KEYS : S_LB_PLAY), buf, n); }
static void lbPlayBox(int &x0, int &w) {
  char b[48];
  setSize(2);
  w = 18 + textW(lbPlayWords(b, sizeof(b)), 2);
  x0 = SNAP2(CX - w / 2);
}
// the picture's box
#define LB_PIC_X (CX - V2_LBL_W)
#define LB_PIC_Y 124

// a leaderboard page: v2GamesPage -1 - c, cartridge c's game
static void v2LbPage() {
  if (millis() - gBoardAskT > 60000UL) boardAsk(false);  // each minute while it's on screen: others play too
  int c = -1 - v2GamesPage;
  if (c < 0 || c >= V2_LB_PAGES) c = 0;
  int g = V2_CART_GAME[c];
  v2PaintGameLabels();  // (at the canvas's top left: before the Game Boy covers it)
  v2GbBack(T(S_LEADERBOARD));
  v2TagChip();
  // the name, big, a shadow under it; in the classic font when it's in Latin letters, as on its cartridge
  char name[48], caps[48];
  snprintf(name, sizeof(name), "%s", gameLabel(g));
  for (char *p = name; *p; p++)
    if (*p == '\n') *p = ' ';
  bool cjk = v2CartFont(name);
  v2Caps(name, caps, sizeof(caps));
  setSize(3);
  int ns = gCjkFont ? 4 : textW(caps, 3) <= 340 ? 3 : 2;  // (Japanese and Korean at 4: twice their font's size)
  setSize(ns);
  int nx = SNAP2(CX - textW(caps, ns) / 2);
  lbText(caps, nx + 2, 94, ns, 1);
  lbText(caps, nx, 92, ns, 3);
  classicFontEnd(cjk);
  lbFrame(LB_PIC_X - 8, LB_PIC_Y - 8, LB_PIC_W + 16, LB_PIC_H + 16);
  lbGreenPic(V2_CART_LBL[c], LB_PIC_X, LB_PIC_Y);
  if (!lbHasBoard(g)) {  // Who's That?: no score kept
    lbCapsC(T(S_LB_FUN), 226, 2, 2);
    lbCapsC(T(S_LB_NAME_ALL), 248, gCjkFont ? 4 : 3, 3);
    lbCapsC(T(S_LB_NO_SCORES), 282, 2, 2);
  } else {
    const BoardTop &b = gBoard[g];
    lbCapsC(T(lbWhat(g)), 224, 2, 2);
    char rec[40], line[48];
    if (!gBoardHave) {  // nothing from the relay yet: on its way, or no connection
      if (buddyNetUp()) {
        lbTextC("-----", 242, 4, 1);
      } else {
        lbCapsC(T(S_LB_OFFLINE), 250, 2, 3);
      }
    } else if (!b.n) {
      lbCapsC(T(S_LB_NONE), 250, 2, 3);
    } else {
      // the best, big (in the classic font, unless it's in the language's letters: Dungeon Crawler's floor in Japanese);
      // then the crown, who set it, and when (Reel Fishing: the fish), centred together
      lbScoreText(g, b.score[0], rec, sizeof(rec), gLang);
      v2Caps(rec, line, sizeof(line));
      cjk = v2CartFont(line);
      lbTextC(line, 242, 4, 3);
      classicFontEnd(cjk);
      cjk = classicFontBegin();
      setSize(2);
      int tagW = textW(b.tag[0], 2);
      classicFontEnd(cjk);
      if (g == 5) {
        char fish[40];
        snprintf(fish, sizeof(fish), b.fishShiny ? T(S_LB_SHINY_FMT) : "%s", dexName(b.fishDex));
        v2Caps(fish, rec, sizeof(rec));
      } else {
        lbDayText(b.mon[0], b.day[0], rec, sizeof(rec), gLang);
      }
      bool dayCjk = g != 5 && classicFontBegin();
      setSize(2);
      int recW = textW(rec, 2);
      int x0 = SNAP2(CX - (LB_CROWN_W + 8 + tagW + 24 + recW) / 2);
      lbText(rec, x0 + LB_CROWN_W + 8 + tagW + 24, 278, 2, 3);
      if (g != 5) classicFontEnd(dayCjk);
      cjk = classicFontBegin();
      lbCrown(x0, 278);
      lbTag(b.tag[0], x0 + LB_CROWN_W + 8, 278, 3);
      classicFontEnd(cjk);
      // the second and third
      lbDots(96, 372, 300, 1);
      for (int i = 1; i < 3; i++) {
        int y = 308 + (i - 1) * 20;
        char n[4];
        snprintf(n, sizeof(n), "%d", i + 1);
        cjk = classicFontBegin();
        lbText(n, 112, y, 2, 2);
        lbTag(i < b.n ? b.tag[i] : "----", 142, y, i < b.n ? 3 : 1);
        if (i >= b.n) lbTextR("-----", 354, y, 2, 1);
        classicFontEnd(cjk);
        if (i < b.n) {
          lbScoreText(g, b.score[i], rec, sizeof(rec), gLang);
          v2Caps(rec, line, sizeof(line));
          cjk = v2CartFont(line);
          lbTextR(line, 354, y, 2, 3);
          classicFontEnd(cjk);
        }
      }
    }
  }
  int px, pw;
  lbPlayBox(px, pw);
  {
    V2_HOVER(px - 6, LB_PLAY_Y - 6, pw + 12, 26);  // under the mouse: lifted (hover_v2.h)
    char b[48];
    lbPointer(px, LB_PLAY_Y, 14, 3);
    lbText(lbPlayWords(b, sizeof(b)), px + 18, LB_PLAY_Y, 2, 3);
  }
  drawPageArrows(v2GamesAt(), V2_GAMES_ALL, V2_PAGE_ARROW_Y);
  v2PageOf(V2_LB_PAGES, c, 400);
  v2CloseX();
}

// the game a tap on a leaderboard page's play line plays, or -1. Only there: a control acts the moment it's touched and
// a swipe that starts on one presses it (Catch151.ino tapDownTry()), so the rest of the page is for swiping
static bool lbOnPlay(int16_t x, int16_t y) {
  int px, pw;
  lbPlayBox(px, pw);
  return x >= px - 16 && x < px + pw + 16 && y >= LB_PLAY_Y - 10 && y < LB_PLAY_Y + 26;
}
static int v2LbPlayAt(int16_t x, int16_t y) {
  if (v2GamesPage >= 0 || !lbOnPlay(x, y)) return -1;
  int c = -1 - v2GamesPage;
  return c >= 0 && c < V2_LB_PAGES ? V2_CART_GAME[c] : -1;
}

// LEADERBOARDS turned a quarter to the right (to read from the top down) down the Games' first page's left edge,
// between the page arrow and the cartridges, in the screen's green, a pointer at each end towards the leaderboard.
// Made once a language: written in the canvas's top left (what's there put back), read back turned
#define LB_SIDE_MAXW 240
#define LB_SIDE_H 16
static uint8_t lbSideInk[LB_SIDE_MAXW * LB_SIDE_H];
static int16_t lbSideW = 0;
static int8_t lbSideLang = -1;
static void v2LbHint() {
  uint16_t *fb = (uint16_t *)gfx->getFramebuffer();
  if (lbSideLang != (int8_t)gLang) {
    lbSideLang = (int8_t)gLang;
    char caps[48];
    v2Caps(T(S_LEADERBOARDS), caps, sizeof(caps));
    setSize(2);
    int w = textW(caps, 2) + 2;
    if (w > LB_SIDE_MAXW) w = LB_SIDE_MAXW;
    lbSideW = (int16_t)w;
    static uint16_t keep[LB_SIDE_MAXW * LB_SIDE_H];
    for (int j = 0; j < LB_SIDE_H; j++) memcpy(keep + j * w, fb + j * V2_W, w * 2);
    gfx->fillRect(0, 0, w, LB_SIDE_H, 0x0000);
    gfx->setTextColor(0xFFFF);
    setCur(0, 0);
    printT(caps);
    for (int j = 0; j < LB_SIDE_H; j++)
      for (int i = 0; i < w; i++) lbSideInk[j * w + i] = fb[j * V2_W + i] == 0xFFFF;
    for (int j = 0; j < LB_SIDE_H; j++) memcpy(fb + j * V2_W, keep + j * w, w * 2);
  }
  const int w = lbSideW, x = 70, y = SNAP2(V2_PAGE_ARROW_Y - w / 2);
  const uint16_t col = lbG(2);
  for (int j = 0; j < LB_SIDE_H; j++)
    for (int i = 0; i < w; i++)
      if (lbSideInk[j * w + i]) fb[(y + i) * V2_W + x + (LB_SIDE_H - 1 - j)] = col;
  lbPointer(86, y - 24, 14, 2, -1);
  lbPointer(86, y + w + 10, 14, 2, -1);
}
