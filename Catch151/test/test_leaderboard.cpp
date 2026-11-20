// The leaderboard's pages: how each game's best reads (leaderboard.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "../src/leaderboard.h"

static const char *score(int g, uint32_t v, Lang l = LANG_EN) {
  static char b[40];
  Lang was = gLang;
  gLang = l;
  lbScoreText(g, v, b, sizeof(b), l);
  gLang = was;
  return b;
}

TEST(leaderboard, the_games_with_a_board) {
  CHECK(lbHasBoard(0) && lbHasBoard(7) && lbHasBoard(5));
  CHECK(!lbHasBoard(1));  // Who's That? keeps no score
  CHECK(!lbHasBoard(-1) && !lbHasBoard(8));
}

TEST(leaderboard, scores_read_as_each_game_shows_them) {
  CHECK(!strcmp(score(0, 50), "50"));
  CHECK(!strcmp(score(6, 43825), "43,825"));
  CHECK(!strcmp(score(6, 1234567), "1,234,567"));
  CHECK(!strcmp(score(7, 999), "999"));
  CHECK(!strcmp(score(7, 1000), "1,000"));
  CHECK(!strcmp(score(2, 60), "1:00"));    // Rock Hopper's seconds
  CHECK(!strcmp(score(4, 31), "0:31"));    // PikaSurf's
  CHECK(!strcmp(score(4, 605), "10:05"));
  CHECK(!strcmp(score(3, 9), "FLOOR 9"));  // Dungeon Crawler's floor
  CHECK(!strcmp(score(5, 2485), "547.8 lb"));  // Reel Fishing: tenths of a kg, weighed in pounds as the game does
}

TEST(leaderboard, thousands_as_the_language_marks_them) {
  CHECK(!strcmp(score(6, 43825, LANG_DE), "43.825"));
  CHECK(!strcmp(score(6, 43825, LANG_FR), "43 825"));
  CHECK(!strcmp(score(6, 43825, LANG_JA), "43,825"));
  CHECK(!strcmp(score(3, 9, LANG_ES), "PISO 9"));
}

TEST(leaderboard, what_each_best_is) {
  CHECK_EQ((int)lbWhat(0), (int)S_LB_HI_SCORE);
  CHECK_EQ((int)lbWhat(2), (int)S_LB_LONGEST_RUN);
  CHECK_EQ((int)lbWhat(3), (int)S_LB_DEEPEST);
  CHECK_EQ((int)lbWhat(4), (int)S_LB_LONGEST_RIDE);
  CHECK_EQ((int)lbWhat(5), (int)S_LB_BIGGEST_FISH);
}

TEST(leaderboard, days_as_the_language_writes_them) {
  char b[16];
  lbDayText(11, 19, b, sizeof(b), LANG_EN);
  CHECK(!strcmp(b, "11/19"));
  lbDayText(11, 19, b, sizeof(b), LANG_ES);
  CHECK(!strcmp(b, "19/11"));
  lbDayText(1, 2, b, sizeof(b), LANG_DE);
  CHECK(!strcmp(b, "2.1."));
  lbDayText(11, 19, b, sizeof(b), LANG_KO);
  CHECK(!strcmp(b, "11/19"));
}
