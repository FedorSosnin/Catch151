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
