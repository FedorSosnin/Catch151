#pragma once
// V2: the leaderboard on the devices (2.2.15): what each game's best three read as on the Games menu's leaderboard
// pages (leaderboard_v2.h draws them; buddy.h boardFrom() reads them from the relay). The rules without the drawing,
// tested in test/test_leaderboard.cpp.
#include <stdint.h>
#include <stdio.h>
#include "fishing.h"  // fishLb10()
#include "i18n.h"

// the games as the Games menu numbers them (Catch151.ino gameLabel()): those with a board keep a score; Who's That? (1)
// doesn't. Reel Fishing's (5) is its biggest fish
#define LB_GAMES 8
inline bool lbHasBoard(int g) { return g >= 0 && g < LB_GAMES && g != 1; }

// how the language marks thousands: 43,825 (English, Japanese, Korean), 43 825 (French), 43.825 (the others)
inline char lbThousandsSep(Lang l) { return l == LANG_EN || l == LANG_JA || l == LANG_KO ? ',' : l == LANG_FR ? ' ' : '.'; }
inline void lbThousands(uint32_t v, char *out, int n, Lang l) {
  char d[12];
  int k = snprintf(d, sizeof(d), "%lu", (unsigned long)v), o = 0;
  for (int i = 0; i < k && o < n - 1; i++) {
    if (i && (k - i) % 3 == 0 && o < n - 2) out[o++] = lbThousandsSep(l);
    out[o++] = d[i];
  }
  out[o] = 0;
}

// a game's score as its page shows it: Berry Drop's, Pinball's and BerryMatch's a number; Rock Hopper's and PikaSurf's
// seconds as m:ss; Dungeon Crawler's floor; Reel Fishing's weight (tenths of a kg) in pounds, as the game weighs them
inline void lbScoreText(int g, uint32_t v, char *out, int n, Lang l) {
  if (g == 2 || g == 4) snprintf(out, n, "%lu:%02lu", (unsigned long)(v / 60), (unsigned long)(v % 60));
  else if (g == 3) snprintf(out, n, T(S_LB_FLOOR_FMT), (unsigned)v);
  else if (g == 5) {
    uint32_t lb10 = fishLb10(v);
    snprintf(out, n, T(S_FISH_LB_FMT), (unsigned)(lb10 / 10), (unsigned)(lb10 % 10));
  } else lbThousands(v, out, n, l);
}

// what the best is (the line over it): the longest run or ride, the deepest floor, the biggest fish, or the high score
inline StrId lbWhat(int g) {
  return g == 2 ? S_LB_LONGEST_RUN : g == 4 ? S_LB_LONGEST_RIDE : g == 3 ? S_LB_DEEPEST : g == 5 ? S_LB_BIGGEST_FISH : S_LB_HI_SCORE;
}

// a day as the language writes it: 11/19 (English, Japanese, Korean: the month first), 19.11. (German), 19/11 (the others)
inline void lbDayText(uint8_t mon, uint8_t day, char *out, int n, Lang l) {
  if (l == LANG_EN || l == LANG_JA || l == LANG_KO) snprintf(out, n, "%u/%u", mon, day);
  else if (l == LANG_DE) snprintf(out, n, "%u.%u.", day, mon);
  else snprintf(out, n, "%u/%u", day, mon);
}
