// Buddy protocol and rules (buddy.h).
#include "framework.h"
#include "shim/Arduino.h"
#include "../src/buddy.h"
#include "../src/battle.h"

static BuddyMsg hello(uint32_t from, int16_t dex) {
  BuddyMsg m;
  buddyInit(m, BM_HELLO, from, 0, 0);
  m.dex = dex;
  m.level = 20;
  strcpy(m.nick, "SPARKY");
  return m;
}

static bool parseCopy(const BuddyMsg &in, BuddyMsg &out) {
  return buddyParse((const uint8_t *)&in, sizeof(in), out);
}

TEST(buddy, format_does_not_change_by_accident) { CHECK_EQ((int)sizeof(BuddyMsg), 65); }

TEST(buddy, accepts_a_valid_hello) {
  BuddyMsg out;
  CHECK(parseCopy(hello(5, 25), out));
  CHECK_EQ(out.dex, (int16_t)25);
  CHECK(strcmp(out.nick, "SPARKY") == 0);
}

TEST(buddy, rejects_nonsense) {
  BuddyMsg out, m = hello(5, 25);
  CHECK(!buddyParse((const uint8_t *)&m, sizeof(m) - 1, out));
  m = hello(5, 25); m.magic[0] = 'X';
  CHECK(!parseCopy(m, out));
  m = hello(5, 25); m.ver = BUDDY_VER + 1;
  CHECK(!parseCopy(m, out));
  m = hello(5, 25); m.type = BM_END;
  CHECK(!parseCopy(m, out));
  m = hello(5, 0);
  CHECK(!parseCopy(m, out));
  m = hello(5, 152);
  CHECK(!parseCopy(m, out));
  m = hello(5, 25); m.level = 0;
  CHECK(!parseCopy(m, out));
  m = hello(0, 25);
  CHECK(!parseCopy(m, out));

  buddyInit(m, BM_FIGHTER, 5, 6, 1);
  m.dex = 25; m.level = 20; m.hp = 50; m.atk = 40; m.def = 0; m.spe = 40;
  CHECK_MSG(!parseCopy(m, out), "defense 0 would divide by 0");
  m.def = 40; m.atk = 60000;
  CHECK(!parseCopy(m, out));
  buddyInit(m, BM_MOVE, 5, 6, 1);
  m.move = 2;
  CHECK(!parseCopy(m, out));
  buddyInit(m, BM_INVITE, 5, 6, 1);
  m.kind = 9;
  CHECK(!parseCopy(m, out));
}

TEST(buddy, odd_nickname_does_not_reach_the_screen) {
  BuddyMsg out, m = hello(5, 25);
  memset(m.nick, 'A', sizeof(m.nick));  // no terminator
  CHECK(parseCopy(m, out));
  CHECK_EQ((int)strlen(out.nick), (int)sizeof(out.nick) - 1);
  m = hello(5, 25);
  m.nick[1] = (char)0xC3;
  CHECK(parseCopy(m, out));
  CHECK_EQ((int)strlen(out.nick), 0);
}

TEST(buddy, arrives_stays_and_leaves) {
  BuddyPeer p;
  BuddyMsg m = hello(5, 25);
  CHECK(buddyPeerHello(p, m, 1000));
  CHECK(!buddyPeerHello(p, m, 2000));
  CHECK(!buddyPeerGone(p, 2000 + BUDDY_TIMEOUT_MS));
  CHECK(buddyPeerGone(p, 2001 + BUDDY_TIMEOUT_MS));
  BuddyMsg other = hello(9, 4);
  CHECK(!buddyPeerHello(p, other, 3000));
  CHECK_EQ(p.id, (uint32_t)5);
}

TEST(buddy, greetings_carry_its_stats) {
  BuddyPeer p;
  BuddyMsg m = hello(5, 25);
  m.hp = 61; m.atk = 55; m.def = 40; m.spe = 90;
  buddyPeerHello(p, m, 1000);
  CHECK_EQ(p.hp, (uint16_t)61);
  CHECK_EQ(p.spe, (uint16_t)90);
  BuddyPeer old;  // an older version sends zeros: unknown
  buddyPeerHello(old, hello(6, 25), 1000);
  CHECK_EQ(old.atk, (uint16_t)0);
  BuddyPeer odd;  // nonsense counts as unknown too
  BuddyMsg n = hello(7, 25);
  n.hp = 61; n.atk = BUDDY_MAX_STAT + 1;
  buddyPeerHello(odd, n, 1000);
  CHECK_EQ(odd.hp, (uint16_t)0);
}

TEST(buddy, the_buddys_pokedex) {
  uint8_t reg[19] = {};
  reg[0] = 1;
  reg[18] = 1 << 6;  // #151
  CHECK(buddyRegHas(reg, 1));
  CHECK(buddyRegHas(reg, 151));
  CHECK(!buddyRegHas(reg, 2));
  CHECK(!buddyRegHas(reg, 0));
  CHECK(!buddyRegHas(reg, 152));
}

// both boards must see exactly the same battle
TEST(buddy, both_boards_see_the_same_battle) {
  for (uint32_t seed = 1; seed <= 300; seed++) {
    BtlFighter a = btlMakeWild(1 + seed % 151, 30, 90), b = btlMakeWild(1 + (seed * 7) % 151, 30, 90);
    if (seed % 3 == 0) b.spe = a.spe;  // speed ties
    BtlMoves am = btlMovesFor(a.dex, a.level, b.dex), bm = btlMovesFor(b.dex, b.level, a.dex);
    BtlFighter a1 = a, b1 = b, a2 = a, b2 = b;  // what each board sees
    uint32_t r1 = seed, r2 = seed, idA = 100, idB = 200;
    for (int turn = 0; turn < 300 && a1.hp && b1.hp; turn++) {
      uint8_t aPick = turn % am.count, bPick = turn % bm.count;
      bool f1 = pvpMineFirst(a1.spe, b1.spe, idA, idB, r1);  // board A: "mine" is a
      bool f2 = pvpMineFirst(b2.spe, a2.spe, idB, idA, r2);  // board B: "mine" is b
      CHECK_MSG(f1 != f2, "exactly one attacks first on both");
      for (int step = 0; step < 2 && a1.hp && b1.hp; step++) {
        bool aHits = (step == 0) == f1;
        uint8_t ra = pvpRoll(r1), rb = pvpRoll(r2);
        CHECK_EQ(ra, rb);
        const BtlMove &m = MOVE_TBL[aHits ? am.ids[aPick] : bm.ids[bPick]];
        BtlFighter &at = aHits ? a1 : b1, &df = aHits ? b1 : a1;
        uint16_t d = btlDamage(at.level, m, at.atk, df.def, DEX_TBL[at.dex].type, DEX_TBL[df.dex].type, ra);
        df.hp = d >= df.hp ? 0 : df.hp - d;
        BtlFighter &at2 = aHits ? a2 : b2, &df2 = aHits ? b2 : a2;
        uint16_t d2 = btlDamage(at2.level, m, at2.atk, df2.def, DEX_TBL[at2.dex].type, DEX_TBL[df2.dex].type, rb);
        df2.hp = d2 >= df2.hp ? 0 : df2.hp - d2;
      }
      CHECK_EQ(a1.hp, a2.hp);
      CHECK_EQ(b1.hp, b2.hp);
    }
  }
}

TEST(buddy, several_buddies_get_their_own_slots) {
  BuddyPeer ps[BUDDY_MAX];
  BuddyMsg m;
  bool arrived = false;
  for (uint32_t id = 11; id < 11 + BUDDY_MAX; id++) {
    buddyInit(m, BM_HELLO, id, 0, 0);
    m.dex = 25;
    m.level = 5;
    CHECK_EQ(buddyHello(ps, m, 1000, &arrived), (int)(id - 11));
    CHECK(arrived);
  }
  CHECK_EQ(buddyCount(ps), BUDDY_MAX);
  buddyInit(m, BM_HELLO, 99, 0, 0);  // a fifth: no room until one leaves
  m.dex = 1;
  m.level = 3;
  CHECK_EQ(buddyHello(ps, m, 1000, &arrived), -1);
  buddyInit(m, BM_HELLO, 13, 0, 0);  // one already here: same slot, not new
  m.dex = 26;
  m.level = 6;
  CHECK_EQ(buddyHello(ps, m, 2000, &arrived), 2);
  CHECK(!arrived);
  CHECK_EQ(ps[2].dex, (int16_t)26);
  ps[1].on = false;                  // one left: the fifth fits now
  buddyInit(m, BM_HELLO, 99, 0, 0);
  m.dex = 1;
  m.level = 3;
  CHECK_EQ(buddyHello(ps, m, 3000, &arrived), 1);
  CHECK_EQ(buddyFind(ps, 99), 1);
  CHECK_EQ(buddyFind(ps, 12), -1);
}

TEST(buddy, yours_stands_in_the_middle_and_bigger) {
  CHECK_EQ(partyScale(0, true), (uint8_t)5);  // alone: as always
  for (int n = 1; n <= BUDDY_MAX; n++) {
    CHECK(partyScale(n, true) > partyScale(n, false));  // never mistaken for a buddy
    for (int k = 1; k <= n; k++) {
      int x = partyBuddyX(n, k);
      CHECK(x - partyHalf(n) >= 13 && x + partyHalf(n) <= 453);   // inside the round screen at ground level
      CHECK(abs(x - PARTY_MINE_X) >= 90);                          // clear of yours in the middle
      if (k > 1) CHECK(x - partyBuddyX(n, k - 1) >= 2 * partyHalf(n));  // and of each other
    }
  }
  CHECK(partyBuddyX(2, 1) < PARTY_MINE_X && partyBuddyX(2, 2) > PARTY_MINE_X);  // one on each side
}

TEST(buddy, showdown_rounds_are_the_same_on_both_boards) {
  for (uint32_t session : { 1u, 0x12345u, 0xFFFFFFFFu }) {
    int16_t seen[SD_ROUNDS] = {};
    for (uint8_t r = 0; r < SD_ROUNDS; r++) {
      int16_t a1, a2, o1[3], o2[3];
      sdRoundOf(session, r, &a1, o1);
      sdRoundOf(session, r, &a2, o2);  // the other board, the same numbers
      CHECK_EQ(a1, a2);
      CHECK(memcmp(o1, o2, sizeof(o1)) == 0);
      CHECK(buddyDexOk(a1));
      int n = (o1[0] == a1) + (o1[1] == a1) + (o1[2] == a1);
      CHECK_EQ(n, 1);  // the answer once, the other two different from it and each other
      CHECK(o1[0] != o1[1] && o1[1] != o1[2] && o1[0] != o1[2]);
      seen[r] = a1;
    }
    CHECK(seen[0] != seen[1] || seen[1] != seen[2]);  // the rounds aren't all the same Pokemon
  }
}

TEST(buddy, showdown_referee) {
  CHECK_EQ(sdWinner(true, 900, true, 1200), (uint8_t)SD_HOST);
  CHECK_EQ(sdWinner(true, 1300, true, 1200), (uint8_t)SD_GUEST);  // faster on its own screen wins
  CHECK_EQ(sdWinner(false, 500, true, 4000), (uint8_t)SD_GUEST);  // a wrong tap never wins
  CHECK_EQ(sdWinner(false, 0, false, 0), (uint8_t)SD_NOBODY);
}

TEST(buddy, showdown_messages_are_checked) {
  BuddyMsg m;
  uint8_t buf[sizeof(BuddyMsg)];
  buddyInit(m, BM_ANSWER, 5, 6, 7);
  m.turn = 2;
  m.move = 1;
  memcpy(buf, &m, sizeof(m));
  CHECK(buddyParse(buf, sizeof(buf), m));
  m.turn = SD_ROUNDS;  // no such round
  memcpy(buf, &m, sizeof(m));
  CHECK(!buddyParse(buf, sizeof(buf), m));
  buddyInit(m, BM_INVITE, 5, 6, 7);
  m.kind = BK_SHOWDOWN;
  memcpy(buf, &m, sizeof(m));
  CHECK(buddyParse(buf, sizeof(buf), m));
}

TEST(buddy, what_a_buddy_can_trade) {
  BuddyPeer b;
  b.dex = 25;  // raising a Pikachu
  b.reg[(25 - 1) >> 3] |= 1 << ((25 - 1) & 7);
  CHECK_EQ(buddyTradeable(b), 0);  // only its partner
  b.reg[0] |= 1;  // Bulbasaur too
  CHECK_EQ(buddyTradeable(b), 1);
}

TEST(buddy, a_trade_offer_carries_its_stars_in_level) {
  BuddyMsg m, out;
  buddyInit(m, BM_OFFER, 5, 6, 7);
  m.dex = 25;
  m.level = 3;
  CHECK(parseCopy(m, out));
  CHECK_EQ(out.level, (uint8_t)3);
  m.level = 0;  // an older version's offer: no stars, still fine
  CHECK(parseCopy(m, out));
  CHECK_EQ(out.level, (uint8_t)0);
  CHECK_EQ((int)sizeof(BuddyMsg), 65);
}

TEST(buddy, volley_messages_are_checked) {
  BuddyMsg out, m;
  buddyInit(m, BM_VBALL, 5, 6, 7);
  m.kind = BK_VOLLEY;
  m.hp = 200;
  m.atk = 2000;  // no curve
  CHECK(parseCopy(m, out));
  m.hp = 900;    // off the court
  CHECK(!parseCopy(m, out));
  m.hp = 200;
  m.atk = 9000;  // a curve nobody could put on it
  CHECK(!parseCopy(m, out));
  buddyInit(m, BM_VMISS, 5, 6, 7);
  m.kind = BK_VOLLEY;
  m.atk = 2;
  m.def = 1;
  CHECK(parseCopy(m, out));
  m.def = 40;    // not a real score
  CHECK(!parseCopy(m, out));
}

TEST(buddy, surf_messages_are_checked) {
  BuddyMsg out, m;
  buddyInit(m, BM_INVITE, 5, 6, 7);
  m.kind = BK_SURF;  // the newest game can be invited to
  CHECK(parseCopy(m, out));
  m.kind = BK_SURF + 1;
  CHECK(!parseCopy(m, out));
  buddyInit(m, BM_SURF, 5, 6, 7);
  m.kind = BK_SURF;
  m.hp = 233;    // in the middle of the water
  m.atk = 415;   // 41.5 s
  m.move = 1;    // crashed
  CHECK(parseCopy(m, out));
  CHECK_EQ(out.atk, (uint16_t)415);
  m.hp = 600;    // off the screen
  CHECK(!parseCopy(m, out));
  m.hp = 233;
  m.move = 2;    // neither surfing nor crashed
  CHECK(!parseCopy(m, out));
}

// the leaderboard (relay/src/board.js): a BM_SCORE goes to the relay alone, and only the relay's answer comes back in
TEST(buddy, a_score_is_taken_only_from_the_relay) {
  BuddyMsg m;
  buddyInit(m, BM_SCORE, 0x1234, 0x99, 48250);
  m.kind = 6;
  BuddyMsg out;
  CHECK(!buddyParse((const uint8_t *)&m, sizeof(m), out));
  m.from = BUDDY_RELAY_ID;
  CHECK(buddyParse((const uint8_t *)&m, sizeof(m), out));
  CHECK_EQ(out.session, (uint32_t)48250);
  CHECK_EQ((int)out.kind, 6);
}

// the leaderboard's best three, as the relay sends them (relay/src/board.js boardReply())
static BuddyMsg boardMsg(uint8_t game, uint8_t n) {
  BuddyMsg m;
  buddyInit(m, BM_BOARD, BUDDY_RELAY_ID, 0x99, 43825);
  m.kind = game;
  m.ack = 1;
  m.move = n;
  uint32_t s2 = 8075, s3 = 123456;
  m.hp = s2 & 0xFFFF, m.atk = s2 >> 16, m.def = s3 & 0xFFFF, m.spe = s3 >> 16;
  memcpy(m.nick, "PIKATES ", 8);
  memcpy(m.reg, "----", 4);
  const uint8_t md[6] = { 11, 17, 11, 20, 1, 2 };
  memcpy(m.reg + 4, md, 6);
  return m;
}

TEST(buddy, the_board_is_taken_only_from_the_relay) {
  BuddyMsg m = boardMsg(6, 3), out;
  CHECK(buddyParse((const uint8_t *)&m, sizeof(m), out));
  m.from = 0x1234;  // a buddy can't send one
  CHECK(!buddyParse((const uint8_t *)&m, sizeof(m), out));
}

TEST(buddy, the_board_reads_three_scores_tags_and_days) {
  BuddyMsg m = boardMsg(6, 3), out;
  CHECK(buddyParse((const uint8_t *)&m, sizeof(m), out));
  BoardTop b;
  CHECK(boardFrom(out, b));
  CHECK_EQ((int)b.n, 3);
  CHECK_EQ(b.score[0], (uint32_t)43825);
  CHECK_EQ(b.score[1], (uint32_t)8075);
  CHECK_EQ(b.score[2], (uint32_t)123456);  // (past 16 bits: both halves)
  CHECK(!strcmp(b.tag[0], "PIKA") && !strcmp(b.tag[1], "TES") && !strcmp(b.tag[2], "----"));  // a short tag's spaces trimmed
  CHECK_EQ((int)b.mon[1], 11);
  CHECK_EQ((int)b.day[1], 20);
  CHECK_EQ((int)b.fishDex, 0);
}

TEST(buddy, the_board_keeps_only_what_it_was_sent) {
  BuddyMsg m = boardMsg(0, 1);
  BoardTop b;
  CHECK(boardFrom(m, b));
  CHECK_EQ((int)b.n, 1);
  CHECK(!b.tag[1][0] && !b.score[1]);
  m = boardMsg(3, 0);
  CHECK(boardFrom(m, b));
  CHECK_EQ((int)b.n, 0);  // nobody yet
}

TEST(buddy, the_board_names_reel_fishings_first_fish) {
  BuddyMsg m = boardMsg(5, 2);
  m.dex = 131, m.shiny = 1;
  BoardTop b;
  CHECK(boardFrom(m, b));
  CHECK_EQ((int)b.fishDex, 131);
  CHECK(b.fishShiny);
  m.dex = 0;
  CHECK(!boardFrom(m, b));  // a fish that can't be
}

TEST(buddy, the_board_refuses_what_cant_be) {
  BoardTop b, keep;
  memset(&keep, 0, sizeof(keep));
  keep.n = 2;
  b = keep;
  BuddyMsg m = boardMsg(1, 3);
  CHECK(!boardFrom(m, b));  // Who's That? keeps no score
  m = boardMsg(9, 3);
  CHECK(!boardFrom(m, b));  // no such game
  m = boardMsg(6, 4);
  CHECK(!boardFrom(m, b));  // more than three
  m = boardMsg(6, 3);
  m.reg[4] = 13;
  CHECK(!boardFrom(m, b));  // no 13th month
  m = boardMsg(6, 3);
  m.session = BOARD_MAX_SCORE + 1;
  CHECK(!boardFrom(m, b));
  m = boardMsg(6, 3);
  m.reg[1] = 7;
  CHECK(!boardFrom(m, b));  // a tag that won't print
  CHECK_EQ((int)b.n, 2);    // and what it had is kept
}

TEST(buddy, asking_for_the_board) {
  BuddyMsg m;
  boardAskMsg(m, 0x1234);
  CHECK_EQ((int)m.type, (int)BM_BOARD);
  CHECK_EQ((int)m.kind, BOARD_ALL);
  CHECK_EQ(m.to, (uint32_t)0);
  CHECK_EQ((int)m.turn, BOARD_SEASON);
  BuddyMsg out;
  CHECK(!buddyParse((const uint8_t *)&m, sizeof(m), out));  // a buddy that hears one passes it over
}
