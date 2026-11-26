// V2: the leaderboard on the Game Boy (2.2.15; Fedor, 2026-11-19: "add our leaderboard in a section to the left of the
// games ... make each game and current record look like old school games on that green Gameboy screen"; of three
// mockups, "B is cool! Let's have them appear in order they appear as cartridges", the first with a crown). Each game's
// own page, opened by a tap on its cartridge (Fedor, 2026-11-22: "That way each game has its own page"; menus_v2.h:
// v2GamesPage -1 Berry Drop's, on to -9 Pic Snap's; its side buttons go round the games, its X back to them), the
// Game Boy's screen drawn in four shades of its own green as the old games were: a game's title screen, its name big,
// its cartridge's picture at 2x in a box as Pokemon Red and Blue drew theirs, the best and what it is, who set it and
// when (a crown before the tag), the second and third, and TAP TO PLAY, which starts it, as the old title screens'
// PUSH START: so does a tap anywhere on the screen (2.2.16; Fedor: "Not just the tiny button at the bottom. Its too
// hard to press on the knomi"), while a swipe there still turns the page. The best three come from the relay (BM_BOARD: buddy.h boardFrom(), Catch151.ino boardAsk()), asked for
// when the menu opens. Who's That? keeps no score: its page says so. (Until 2026-11-22 the pages ran to the left of the
// cartridges, and LEADERBOARDS ran down the first page's left edge, pointing there.)

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

// a score in bold: each letter drawn twice, a font pixel apart (2 px: whole on the Knomi), a font pixel more between
// letters so they don't run together. Right-aligned to xr as lbTextR() puts the plain ones (the last letter's ink ends
// 2 px before it)
#define LB_BOLD_ADV 14
static int lbBoldW(const char *s) { return gCjkFont ? textW(s, 2) + 2 : (int)strlen(s) * LB_BOLD_ADV; }
static void lbBoldR(const char *s, int xr, int y, int shade) {
  setSize(2);
  int x = xr - lbBoldW(s);
  if (gCjkFont) {  // (Dungeon Crawler's floor in Japanese or Korean: the language's letters, a pixel thicker each pass)
    lbText(s, x, y, 2, shade);
    lbText(s, x + 2, y, 2, shade);
    return;
  }
  gfx->setTextColor(lbG(shade));
  for (const char *p = s; *p; p++, x += LB_BOLD_ADV)
    for (int d = 0; d <= 2; d += 2) {
      setCur(x + d, y);
      printT(*p);
    }
}

// the rows: the best three alike (Fedor, 2026-11-25: "make all three scores similar in size. With the top one still
// with a crown and bold score"), the first's day (Reel Fishing: its fish) on a line of its own under its tag
#define LB_WHAT_Y 224  // what the best is (HI-SCORE...)
#define LB_ROW1_Y 244
#define LB_WHEN_Y 262  // the first's day, or its fish
#define LB_DOTS_Y 282
#define LB_ROW2_Y 290  // the second; the third 20 px under it
#define LB_RANK_X 112  // the second's and third's number (the crown's middle on it)
#define LB_TAG_X 142
#define LB_SCORE_XR 354

// TAP TO PLAY: a button as wide as the rows (wider if its words need it), dark, its words the screen's colour; its box
// for a tap a finger's width round it (Fedor, 2026-11-25: the scores in a row each "give us more space for a bigger
// 'tap to play' area"; your own row under the three was tried and dropped for it: "keep the tap to play larger")
#define LB_PLAY_Y 334  // the button's top
#define LB_PLAY_H 44
#define LB_PLAY_W 276  // (the dots' width, 96..372)
static const char *lbPlayWords(char *buf, int n) { return v2Caps(T(playKeysUi ? S_LB_PLAY_KEYS : S_LB_PLAY), buf, n); }
static void lbPlayBox(int &x0, int &w) {
  char b[48];
  setSize(2);
  w = 18 + textW(lbPlayWords(b, sizeof(b)), 2) + 32;
  if (w < LB_PLAY_W) w = LB_PLAY_W;
  x0 = SNAP2(CX - w / 2);
}
static void lbPlayButton() {
  int x0, w;
  lbPlayBox(x0, w);
  const int y0 = LB_PLAY_Y, h = LB_PLAY_H;
  V2_HOVER(x0 - 2, y0 - 2, w + 6, h + 6);  // under the mouse: lifted (hover_v2.h)
  gfx->fillRect(x0 + 4, y0 + h, w - 4, 2, lbG(1));  // its shadow, below and to the right
  gfx->fillRect(x0 + w, y0 + 4, 2, h - 2, lbG(1));
  gfx->fillRect(x0 + 2, y0, w - 4, h, lbG(3));  // the button, its corners a pixel in
  gfx->fillRect(x0, y0 + 2, w, h - 4, lbG(3));
  gfx->fillRect(x0 + 4, y0 + 4, w - 8, 2, lbG(1));  // a line round inside it, as a Game Boy box has
  gfx->fillRect(x0 + 4, y0 + h - 6, w - 8, 2, lbG(1));
  gfx->fillRect(x0 + 4, y0 + 6, 2, h - 12, lbG(1));
  gfx->fillRect(x0 + w - 6, y0 + 6, 2, h - 12, lbG(1));
  char b[48];
  const char *words = lbPlayWords(b, sizeof(b));
  setSize(2);
  int tw = 18 + textW(words, 2) - 2;  // (the words' ink: the blank column after the last letter left out)
  int px = SNAP2(CX - tw / 2), py = SNAP2(y0 + (h - 14) / 2);
  lbPointer(px, py, 14, 0);
  lbText(words, px + 18, py, 2, 0);
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
    lbCapsC(T(lbWhat(g)), LB_WHAT_Y, 2, 2);
    char rec[40], line[48];
    if (!gBoardHave) {  // nothing from the relay yet: on its way, or no connection
      if (buddyNetUp()) {
        lbTextC("-----", 250, 4, 1);
      } else {
        lbCapsC(T(S_LB_OFFLINE), 266, 2, 3);
      }
    } else if (!b.n) {
      lbCapsC(T(S_LB_NONE), 266, 2, 3);
    } else {
      // the three in rows, each its place (the first a crown), its tag, its score at the right (the first's in bold:
      // plain if bold would reach its tag). Scores in the classic font, unless they're in the language's letters
      // (Dungeon Crawler's floor in Japanese)
      for (int i = 0; i < 3; i++) {
        int y = i ? LB_ROW2_Y + (i - 1) * 20 : LB_ROW1_Y;
        cjk = classicFontBegin();
        if (i) {
          char n[4];
          snprintf(n, sizeof(n), "%d", i + 1);
          lbText(n, LB_RANK_X, y, 2, 2);
        } else {
          lbCrown(SNAP2(LB_RANK_X + 5 - LB_CROWN_W / 2), y);
        }
        lbTag(i < b.n ? b.tag[i] : "----", LB_TAG_X, y, i < b.n ? 3 : 1);
        setSize(2);
        int tagEnd = LB_TAG_X + textW(i < b.n ? b.tag[i] : "----", 2) + 4;
        if (i >= b.n) lbTextR("-----", LB_SCORE_XR, y, 2, 1);
        classicFontEnd(cjk);
        if (i < b.n) {
          lbScoreText(g, b.score[i], rec, sizeof(rec), gLang);
          v2Caps(rec, line, sizeof(line));
          cjk = v2CartFont(line);
          if (!i && LB_SCORE_XR - lbBoldW(line) >= tagEnd + 12) lbBoldR(line, LB_SCORE_XR, y, 3);
          else lbTextR(line, LB_SCORE_XR, y, 2, 3);
          classicFontEnd(cjk);
        }
        if (!i) {  // the first's day (Reel Fishing: its fish), under its tag, lighter
          if (g == 5) {
            char fish[40];
            snprintf(fish, sizeof(fish), b.fishShiny ? T(S_LB_SHINY_FMT) : "%s", dexName(b.fishDex));
            v2Caps(fish, rec, sizeof(rec));
          } else {
            lbDayText(b.mon[0], b.day[0], rec, sizeof(rec), gLang);
          }
          bool whenCjk = g != 5 && classicFontBegin();
          lbText(rec, LB_TAG_X, LB_WHEN_Y, 2, 2);
          if (g != 5) classicFontEnd(whenCjk);
          lbDots(96, 372, LB_DOTS_Y, 1);
        }
      }
    }
  }
  lbPlayButton();
  v2GamesArrows();  // (both: round the games)
  v2PageOf(V2_LB_PAGES, c, 400);
  v2CloseX();
}

// what the Games menu shows, all but its power light (Catch151.ino screenKey(): drawn again only when this changes):
// its page; the board as the relay sent it, whether it's come, and the connection (its pages' NOT CONNECTED and -----);
// the gamer tag on its label (grey while it's the one given); the language and the play line's words; whether the
// cartridges' pictures can be made yet; and the minute (the pictures follow the hour's light, a page asks for the board
// again each minute)
uint32_t v2GamesScreenKey() {
  uint32_t h = 2166136261u;
  auto mix = [&h](uint32_t v) { h = (h ^ v) * 16777619u; };
  mix((uint32_t)(v2GamesPage + 64));
  mix(gBoardHave), mix(buddyNetUp()), mix(gLang), mix(playKeysUi), mix(pet.tagGiven), mix(thumbs.loaded);
  for (const char *p = pet.gamerTag; *p; p++) mix((uint8_t)*p);
  const uint8_t *b = (const uint8_t *)gBoard;
  for (size_t i = 0; i < sizeof(gBoard); i++) mix(b[i]);
  mix(millis() / 60000UL);
  return h;
}

// the game a tap on a leaderboard page's play line plays, or -1. Only there does the touch itself play it: a control
// acts the moment it's touched and a swipe that starts on one presses it (Catch151.ino tapDownTry()). The rest of the
// screen plays it on the lift, if the finger didn't move (gamesMenuTap()), so a swipe there turns the page
static bool lbOnPlay(int16_t x, int16_t y) {
  int px, pw;
  lbPlayBox(px, pw);
  return x >= px - 12 && x < px + pw + 12 && y >= LB_PLAY_Y - 8 && y < LB_PLAY_Y + LB_PLAY_H + 10;
}
static int v2LbPlayAt(int16_t x, int16_t y) {
  if (v2GamesPage >= 0 || !lbOnPlay(x, y)) return -1;
  int c = -1 - v2GamesPage;
  return c >= 0 && c < V2_LB_PAGES ? V2_CART_GAME[c] : -1;
}
