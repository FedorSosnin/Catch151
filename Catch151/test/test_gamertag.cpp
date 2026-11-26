// The gamer tag's checks (gamertag.h): the rude-word list, its stand-ins, and how a tag is kept.
#include "framework.h"
#include "../src/gamertag.h"

TEST(gamertag, rude_words_are_caught_through_their_stand_ins) {
  CHECK(gamerTagRude("FUCK"));
  CHECK(gamerTagRude("fuck"));
  CHECK(gamerTagRude("F.CK"));      // a mark between: FCK
  CHECK(gamerTagRude("F*CK"));
  CHECK(gamerTagRude("SH1T"));      // 1 for I
  CHECK(gamerTagRude("5H1T"));
  CHECK(gamerTagRude("N4ZI"));
  CHECK(gamerTagRude("$EX"));
  CHECK(gamerTagRude("XFUK"));      // a strong word inside a longer tag
  CHECK(gamerTagRude("ASS"));       // a mild one as the whole tag
  CHECK(gamerTagRude("A55"));
  CHECK(gamerTagRude("a.s.s"));
}

TEST(gamertag, ordinary_tags_pass) {
  const char *ok[] = { "PIKA", "FED", "ASH", "MO", "BASS", "PASS", "KAI", "ZOE", "RED", "GG", "TITO", "SAM", "1234", "LOL", "MEW!" };
  for (const char *t : ok) CHECK_MSG(!gamerTagRude(t), t);
}

TEST(gamertag, a_tag_is_kept_trimmed_to_four_printable_characters) {
  char t[GAMER_TAG_LEN + 1];
  CHECK(gamerTagClean("PIKACHU", t) && !strcmp(t, "PIKA"));
  CHECK(gamerTagClean(" AB ", t) && !strcmp(t, "AB"));
  CHECK(gamerTagClean("A B", t) && !strcmp(t, "A B"));
  CHECK(gamerTagClean("a\x01" "b", t) && !strcmp(t, "ab"));  // nothing unprintable
  CHECK(!gamerTagClean("    ", t) && !t[0]);
  CHECK(!gamerTagClean("", t));
}

// the tag a player starts with (gamerTagStarter()): two letters, two digits, never I, O, 0 or 1, never rude, and a
// different one for most players
TEST(gamertag, a_starting_tag_is_two_letters_and_two_digits_never_rude) {
  char t[GAMER_TAG_LEN + 1];
  int rudeSeen = 0;
  static char seen[4096][5];
  uint32_t r = 12345;
  for (int i = 0; i < 200000; i++) {
    r = r * 2891336453u + 1;
    gamerTagStarter(r, t);
    CHECK(strlen(t) == 4);
    CHECK(t[0] >= 'A' && t[0] <= 'Z' && t[1] >= 'A' && t[1] <= 'Z' && t[2] >= '2' && t[2] <= '9' && t[3] >= '2' && t[3] <= '9');
    CHECK(t[0] != 'I' && t[0] != 'O' && t[1] != 'I' && t[1] != 'O');
    CHECK_MSG(!gamerTagRude(t), t);
    if (i < 4096) memcpy(seen[i], t, 5);
  }
  // the ones the list would catch are made again: FA9x (FAG), KY5x (KYS), SH7x (SHT)
  for (uint32_t v = 0; v < 24 * 24 * 64; v++) {
    char raw[5] = { "ABCDEFGHJKLMNPQRSTUVWXYZ"[v % 24], "ABCDEFGHJKLMNPQRSTUVWXYZ"[v / 24 % 24], "23456789"[v / 576 % 8],
                    "23456789"[v / 4608 % 8], 0 };
    if (gamerTagRude(raw)) {
      rudeSeen++;
      gamerTagStarter(v, t);
      CHECK_MSG(!gamerTagRude(t) && strcmp(t, raw), raw);
    }
  }
  CHECK(rudeSeen > 0);
  int same = 0;  // 4,096 players: a few share one, most don't
  for (int i = 0; i < 4096; i++)
    for (int j = i + 1; j < 4096; j++) same += !strcmp(seen[i], seen[j]);
  CHECK(same < 400);
}
