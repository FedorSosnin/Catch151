#pragma once
#include <stdint.h>
#include <string.h>
// The gamer tag (Fedor: "lets call it a gamer tag and I think 4 gives lots of freedom"): up to four characters typed
// on the keyboard, shown with your best scores on the world board (relay/src/index.js draws it). Asked for after your
// first scored game, changed from its label on the Games menu.
//
// Words nobody should see on the board (Fedor: "a list of typical cursewords that would force a name to show up as
// ****"): the relay keeps the same list and shows a tag on it as ****, so the list there can grow without an update.
// Here it only asks for another before it's sent. Checked on the letters alone, read through the usual stand-ins
// (0 for O, 1 for I, 3 for E, 4 for A, 5 for S, 7 for T, 8 for B, 9 for G, $ for S, @ for A, ! and | for I), the
// marks between them dropped (F.CK, F*CK): the strong words anywhere in a tag, the milder ones only as the whole tag
// (ASS is rude, BASS and PASS aren't).
#define GAMER_TAG_LEN 4

// the strong ones: rude wherever they are in a tag
static const char *const GAMER_TAG_STRONG[] = {
  "FUCK", "FUK", "FUC", "FCK", "FUQ", "SHIT", "SHT", "CUNT", "COCK", "DICK", "DIK", "TWAT", "JIZ", "CUM", "NIG", "NGR",
  "FAG", "KKK", "NAZI", "RAPE", "PORN", "SLUT", "WHOR", "KYS", "STFU", "GTFO", "TITS", "BICH", "BTCH", "WANK", "CLIT",
  "DYKE", "KIKE", "SPIC", "GOOK", "COON", "CHNK", "SEX", "XXX", "MILF", "DILF", "ANAL", "ANUS", "BOOB", "ARSE", "ASSH",
  "PISS", "NUDE", "PUSY", "PSSY", "HTLR",
};
// the milder ones: rude only as the whole tag
static const char *const GAMER_TAG_MILD[] = { "ASS", "TIT", "HOE", "WTF", "PAKI", "HOMO", "SUX", "PEE", "POO" };

// the letters of a tag as the list reads them (at most 8 of them)
inline void gamerTagLetters(const char *tag, char *out) {
  int n = 0;
  for (const char *p = tag; *p && n < 8; p++) {
    char c = *p;
    if (c >= 'a' && c <= 'z') c -= 32;
    switch (c) {
      case '0': c = 'O'; break;
      case '1': case '!': case '|': c = 'I'; break;
      case '3': c = 'E'; break;
      case '4': case '@': c = 'A'; break;
      case '5': case '$': c = 'S'; break;
      case '7': c = 'T'; break;
      case '8': c = 'B'; break;
      case '9': c = 'G'; break;
      default: break;
    }
    if (c >= 'A' && c <= 'Z') out[n++] = c;
  }
  out[n] = 0;
}
// true for a tag nobody should see on the board
inline bool gamerTagRude(const char *tag) {
  char l[9];
  gamerTagLetters(tag, l);
  for (const char *w : GAMER_TAG_STRONG)
    if (strstr(l, w)) return true;
  for (const char *w : GAMER_TAG_MILD)
    if (!strcmp(l, w)) return true;
  return false;
}
// a tag as it's kept: printable characters only (the keyboard's), at most four, no spaces at its ends; false if
// nothing is left
inline bool gamerTagClean(const char *in, char *out) {
  int n = 0;
  for (const char *p = in; *p && n < GAMER_TAG_LEN; p++)
    if (*p >= 32 && *p < 127 && (n || *p != ' ')) out[n++] = *p;
  while (n && out[n - 1] == ' ') n--;
  out[n] = 0;
  return n > 0;
}
