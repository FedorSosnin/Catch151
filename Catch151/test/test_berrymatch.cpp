// BerryMatch's rules (berrymatch.h): the round board and its fit on the screen, a fresh deal, finding lines, a swap
// that catches and one that swaps back, gravity and the refill, the balls (made and gone off), no swap left leading
// to a shuffle, taps while the board moves, the keys, and whole games played by a bot.
#include "framework.h"
#include "shim/Arduino.h"
#include "../src/berrymatch.h"

static const float DT = 1.0f / 30;

// the board from rows of digits (0: nothing, 1..5 kinds, 8 Great, 9 Ultra, M Master); a corner is '.'
static void setBoard(BmState &s, const char *const rows[BM_N]) {
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++) {
      char c = rows[y][x];
      s.b[y][x] = c == '.' ? 0 : c == 'M' ? BM_MASTER : (uint8_t)(c - '0');
    }
}
// the board with no swap that catches and no line (each kind two places on from the one to its left, three in a column)
static void setStuck(BmState &s) {
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++) s.b[y][x] = bmOn(x, y) ? (uint8_t)(1 + (x + 2 * y) % BM_KINDS) : 0;
}
static bool full(const BmState &s) {
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++)
      if (bmOn(x, y) != (s.b[y][x] != 0)) return false;
  return true;
}
// steps until the board is ready (or the game's end), every event seen
static uint32_t settle(BmState &s, float maxS = 30) {
  uint32_t ev = 0;
  for (float t = 0; t < maxS; t += DT) {
    ev |= bmStep(s, DT);
    if (s.phase == BMP_PLAY || s.phase == BMP_END) break;
  }
  return ev;
}
static uint32_t swap(BmState &s, int x0, int y0, int x1, int y1) {
  uint32_t ev = bmTap(s, x0, y0);
  ev |= bmTap(s, x1, y1);
  return ev | settle(s);
}
// a fresh state with a board, ready to play
static BmState board(const char *const rows[BM_N], uint32_t seed = 11) {
  BmState s;
  bmStart(s, seed);
  setBoard(s, rows);
  return s;
}

TEST(berrymatch, the_board_is_round_and_every_square_is_on_the_screen_and_big_enough_for_a_finger) {
  int on = 0;
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++) {
      if (!bmOn(x, y)) continue;
      on++;
      for (int c = 0; c < 4; c++) {  // every corner of the square inside the round screen, clear of its rim
        float px = BM_X0 + (x + (c & 1)) * BM_CELL, py = BM_Y0 + (y + (c >> 1)) * BM_CELL;
        CHECK(hypotf(px - 233, py - 233) <= 226);
      }
    }
  CHECK_EQ(on, BM_N * BM_N - 4);  // only the four corners are off
  CHECK(!bmOn(0, 0) && !bmOn(BM_N - 1, 0) && !bmOn(0, BM_N - 1) && !bmOn(BM_N - 1, BM_N - 1));
  CHECK(BM_CELL >= 56);  // (the zone squares that worked on the Knomi are 78, the Search bar 56 tall)
  CHECK(BM_CELL >= 18 * 3);  // a piece is an 18 x 18 icon at 3x
  int x, y;
  CHECK(bmCellAt(BM_X0 + BM_CELL + 5, BM_Y0 + 5, x, y) && x == 1 && y == 0);
  CHECK(!bmCellAt(BM_X0 + 5, BM_Y0 + 5, x, y));  // a corner
  CHECK(!bmCellAt(10, 233, x, y));
}

TEST(berrymatch, a_new_board_is_full_with_no_line_and_a_swap_that_works) {
  for (uint32_t seed = 1; seed < 300; seed++) {
    BmState s;
    bmStart(s, seed);
    CHECK(full(s));
    CHECK(!bmAnyMatch(s));
    CHECK(bmHasMove(s));
    CHECK(s.hint[0] >= 0 && bmSwapWorks(s, s.hint[0], s.hint[1], s.hint[2], s.hint[3]));  // a hint ready
    CHECK_EQ(s.moves, (uint8_t)BM_MOVES);
    CHECK_EQ(s.score, (uint32_t)0);
    for (int y = 0; y < BM_N; y++)
      for (int x = 0; x < BM_N; x++) CHECK(!bmOn(x, y) || bmIsKind(s.b[y][x]));  // no ball to start with
  }
  BmState a, b;  // the seed decides it
  bmStart(a, 42);
  bmStart(b, 42);
  CHECK(memcmp(a.b, b.b, sizeof(a.b)) == 0);
}

TEST(berrymatch, lines_of_three_or_more_are_found_in_rows_and_columns_and_nothing_else) {
  static const char *const R[BM_N] = {
    ".1231.",
    "222413",
    "314524",
    "413152",
    "524231",
    ".3514.",
  };
  BmState s = board(R);
  BmRun r[30];
  int n = bmRuns(s, r);
  CHECK_EQ(n, 1);
  CHECK(r[0].across && r[0].x == 0 && r[0].y == 1 && r[0].len == 3 && r[0].kind == 2);
  CHECK(bmMatchAt(s, 0, 1) && bmMatchAt(s, 2, 1) && !bmMatchAt(s, 3, 1));
  static const char *const C[BM_N] = {
    ".5231.",
    "314153",
    "314524",
    "413152",
    "524231",
    ".3514.",
  };
  s = board(C);
  n = bmRuns(s, r);
  CHECK_EQ(n, 1);
  CHECK(!r[0].across && r[0].x == 1 && r[0].y == 1 && r[0].len == 3 && r[0].kind == 1);
  setStuck(s);
  CHECK_EQ(bmRuns(s, r), 0);
  CHECK(!bmAnyMatch(s));
}

TEST(berrymatch, a_swap_that_lines_up_three_catches_them_scores_and_costs_a_move) {
  static const char *const R[BM_N] = {
    ".1231.",
    "224413",  // the 4 at (2, 1) swapped with the 2 below it: 2 2 2 along row 1
    "312524",
    "413152",
    "524231",
    ".3514.",
  };
  BmState s = board(R);
  CHECK(!bmAnyMatch(s));
  CHECK(bmSwapWorks(s, 2, 1, 2, 2));
  uint32_t ev = bmTap(s, 2, 1);
  CHECK(ev & BE_PICK);
  CHECK(s.selX == 2 && s.selY == 1);
  ev = bmTap(s, 2, 2);
  CHECK(ev & BE_SWAP);
  CHECK_EQ((int)s.phase, (int)BMP_SWAP);
  ev = settle(s);
  CHECK(ev & BE_POP);
  CHECK(!(ev & BE_BACK));
  CHECK(s.phase == BMP_PLAY);
  CHECK_EQ(s.moves, (uint8_t)(BM_MOVES - 1));
  CHECK(s.score >= 3 * BM_PTS);
  CHECK(full(s));          // refilled
  CHECK(!bmAnyMatch(s));   // and settled: whatever lined up on landing went too
  CHECK(bmHasMove(s));
}

TEST(berrymatch, a_swap_that_lines_up_nothing_swaps_back_and_costs_nothing) {
  BmState s;
  bmStart(s, 8);
  setStuck(s);  // (no swap lines anything up on it)
  uint8_t before[BM_N][BM_N];
  memcpy(before, s.b, sizeof(before));
  CHECK(!bmSwapWorks(s, 1, 1, 2, 1));
  uint32_t ev = swap(s, 1, 1, 2, 1);
  CHECK(ev & BE_SWAP);
  CHECK(ev & BE_BACK);
  CHECK(!(ev & BE_POP));
  CHECK(s.phase == BMP_PLAY);
  CHECK(memcmp(before, s.b, sizeof(before)) == 0);  // as it was
  CHECK_EQ(s.moves, (uint8_t)BM_MOVES);
  CHECK_EQ(s.score, (uint32_t)0);
}

TEST(berrymatch, picking_tapping_it_again_a_square_farther_off_and_a_corner) {
  BmState s;
  bmStart(s, 5);
  CHECK(bmTap(s, 2, 2) & BE_PICK);
  CHECK(bmTap(s, 2, 2) & BE_UNPICK);  // the same again: put down
  CHECK(s.selX < 0);
  bmTap(s, 2, 2);
  CHECK(bmTap(s, 4, 4) & BE_PICK);  // not beside it: that one picked instead, nothing swapped
  CHECK(s.selX == 4 && s.selY == 4 && s.phase == BMP_PLAY);
  CHECK(bmTap(s, 3, 3) & BE_PICK);  // corner to corner isn't beside it either
  CHECK(s.phase == BMP_PLAY);
  CHECK(bmTap(s, 0, 0) & BE_UNPICK);  // a corner that's off: put down
  CHECK(s.selX < 0);
  CHECK(!bmNext(0, 0, 1, 0) && bmNext(1, 0, 2, 0) && !bmNext(1, 1, 2, 2));
}

TEST(berrymatch, gravity_brings_the_pieces_down_in_order_and_new_ones_fill_the_top) {
  BmState s;
  bmStart(s, 9);
  // column 2: knock out rows 1 and 3 (of 0..5); column 0 (rows 1..4 in play): its bottom one
  uint8_t col2[BM_N], col0[BM_N];
  for (int y = 0; y < BM_N; y++) col2[y] = s.b[y][2], col0[y] = s.b[y][0];
  s.b[1][2] = s.b[3][2] = 0;
  s.b[4][0] = 0;
  int most = bmFall(s);
  CHECK(full(s));
  CHECK_EQ(most, 2);
  // column 2: 5 and 4 stay, 2 falls one, 0 falls two, two new ones on top
  CHECK_EQ(s.b[5][2], col2[5]);
  CHECK_EQ(s.b[4][2], col2[4]);
  CHECK_EQ(s.b[3][2], col2[2]);
  CHECK_EQ(s.b[2][2], col2[0]);
  CHECK_EQ(s.drop[5][2], 0);
  CHECK_EQ(s.drop[3][2], 1);
  CHECK_EQ(s.drop[2][2], 2);
  CHECK_EQ(s.drop[1][2], 2);  // the new ones come from as far above as there were gaps
  CHECK_EQ(s.drop[0][2], 2);
  CHECK(bmIsKind(s.b[0][2]) && bmIsKind(s.b[1][2]));
  // column 0: its top square is row 1 (row 0 is a corner)
  CHECK_EQ(s.b[4][0], col0[3]);
  CHECK_EQ(s.b[2][0], col0[1]);
  CHECK(bmIsKind(s.b[1][0]) && s.drop[1][0] == 1);
  CHECK_EQ(s.b[0][0], 0);  // the corner stays empty
  CHECK_EQ(s.b[5][0], 0);
  // a column left alone doesn't move
  for (int y = 0; y < BM_N; y++) CHECK_EQ(s.drop[y][3], 0);
  // the fall's time: longer for farther, landed at its end
  CHECK(bmFallTime(4) > bmFallTime(1) && bmFallTime(1) > 0);
  CHECK(bmFallLeft(3, 0) == 3);
  CHECK(bmFallLeft(3, bmFallTime(3) + 0.001f) == 0);
}

TEST(berrymatch, what_lines_up_as_the_pieces_land_is_caught_too_as_a_chain_scoring_more) {
  BmState s;
  bmStart(s, 3);
  setStuck(s);
  // column 2: a line of 1s in rows 3..5 and a 4 above it, which lands in row 5 beside two 4s
  s.b[3][2] = s.b[4][2] = s.b[5][2] = 1;
  s.b[2][2] = 4;
  s.b[5][3] = s.b[5][4] = 4;
  BmRun r[30];
  CHECK_EQ(bmRuns(s, r), 1);
  s.ax = s.ay = s.bx = s.by = -1;
  uint32_t ev = bmResolve(s, nullptr, 0);
  CHECK(ev & BE_POP);
  CHECK(!(ev & BE_CHAIN));
  CHECK_EQ(s.chain, 1);
  CHECK_EQ(s.pts, (uint16_t)(3 * BM_PTS));
  ev = 0;
  for (int i = 0; i < 200 && !(s.phase == BMP_POP && s.chain == 2); i++) ev |= bmStep(s, DT);
  CHECK(ev & BE_LAND);
  CHECK(ev & BE_CHAIN);
  CHECK_EQ(s.chain, 2);
  CHECK(s.gone[5][2] == 4 && s.gone[5][3] == 4 && s.gone[5][4] == 4);
  int n = 0;
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++) n += s.gone[y][x] != 0;
  CHECK_EQ(s.pts, (uint16_t)(n * BM_PTS * 2));  // the second step: twice a piece
  settle(s);
  CHECK(s.bestChain >= 2);
  CHECK(s.phase == BMP_PLAY && full(s) && !bmAnyMatch(s));
}

TEST(berrymatch, four_in_a_line_leave_a_great_ball_where_the_swap_put_its_piece) {
  BmState s;
  bmStart(s, 3);
  setStuck(s);
  s.b[3][1] = s.b[3][2] = s.b[3][3] = s.b[3][4] = 4;  // (row 3 is 2 _ _ _ _ 2 round it)
  int8_t at[1][2] = { { 3, 3 } };
  uint32_t ev = 0;
  int n = bmCatch(s, nullptr, 0, at, 1, &ev);
  CHECK_EQ(n, 4);
  CHECK(ev & BE_BALL);
  CHECK_EQ(s.b[3][3], (uint8_t)BM_GREAT);  // the piece the swap put there turns into the ball
  CHECK_EQ(s.made[3][3], (uint8_t)BM_GREAT);
  CHECK(!s.b[3][1] && !s.b[3][2] && !s.b[3][4]);
  CHECK(s.gone[3][3] == 4);
  // no swap in it (a chain): the line's middle
  setStuck(s);
  for (int y = 1; y <= 4; y++) s.b[y][2] = 4;
  bmCatch(s, nullptr, 0, nullptr, 0);
  CHECK_EQ(s.b[2][2], (uint8_t)BM_GREAT);
}

TEST(berrymatch, a_row_and_a_column_crossing_leave_an_ultra_ball_and_five_in_a_line_a_master_ball) {
  BmState s;
  bmStart(s, 3);
  setStuck(s);
  s.b[2][1] = s.b[2][2] = s.b[2][3] = 2;  // an L: row 2 from x 1, column 3 from row 2 down
  s.b[3][3] = s.b[4][3] = 2;
  uint32_t ev = 0;
  int n = bmCatch(s, nullptr, 0, nullptr, 0, &ev);
  CHECK_EQ(n, 5);
  CHECK_EQ(s.b[2][3], (uint8_t)BM_ULTRA);  // where they cross
  int balls = 0;
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++) balls += bmIsBall(s.b[y][x]);
  CHECK_EQ(balls, 1);  // one ball for the group, not one a line
  setStuck(s);
  for (int x = 0; x < 5; x++) s.b[2][x] = 3;
  n = bmCatch(s, nullptr, 0, nullptr, 0);
  CHECK_EQ(n, 5);
  CHECK_EQ(s.b[2][2], (uint8_t)BM_MASTER);
}

TEST(berrymatch, each_ball_catches_its_own_shape_when_swapped_and_a_ball_caught_goes_off_too) {
  BmState s;
  bmStart(s, 3);
  // a Great Ball: its row and its column
  setStuck(s);
  s.b[2][2] = BM_GREAT;
  int8_t fire[1][2] = { { 2, 2 } };
  uint32_t ev = 0;
  int n = bmCatch(s, fire, 1, nullptr, 0, &ev);
  CHECK(ev & BE_FIRE);
  CHECK_EQ(n, BM_N + BM_N - 1);  // row 2 and column 2 are all in play
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++) CHECK((s.gone[y][x] != 0) == (bmOn(x, y) && (x == 2 || y == 2)));
  CHECK_EQ(s.fxN, 1);
  // an Ultra Ball: everything two steps from it
  setStuck(s);
  s.b[2][2] = BM_ULTRA;
  n = bmCatch(s, fire, 1, nullptr, 0);
  CHECK_EQ(n, 13);
  CHECK(s.gone[0][2] && s.gone[4][2] && s.gone[2][0] && s.gone[3][3] && !s.gone[4][4] && !s.gone[0][1]);
  // a Master Ball: the whole board
  setStuck(s);
  s.b[2][2] = BM_MASTER;
  ev = 0;
  n = bmCatch(s, fire, 1, nullptr, 0, &ev);
  CHECK_EQ(n, BM_N * BM_N - 4);
  CHECK(ev & BE_MASTER);
  // a Great Ball caught in an Ultra Ball's blast goes off as well: its row and column too
  setStuck(s);
  s.b[2][2] = BM_ULTRA;
  s.b[2][4] = BM_GREAT;  // two steps away: caught
  ev = 0;
  n = bmCatch(s, fire, 1, nullptr, 0, &ev);
  CHECK_EQ(s.fxN, 2);
  CHECK(s.gone[5][4] && s.gone[0][4]);  // the Great Ball's column, out of the Ultra Ball's reach
  CHECK(n > 13);
}

TEST(berrymatch, a_ball_swapped_with_any_piece_beside_it_is_a_move_and_goes_off_where_it_lands) {
  BmState s;
  bmStart(s, 3);
  setStuck(s);
  CHECK(!bmHasMove(s));
  s.b[1][1] = BM_GREAT;
  CHECK(bmHasMove(s));  // with a ball on it there's always a move
  CHECK(bmSwapWorks(s, 1, 1, 2, 1));
  uint32_t ev = bmTap(s, 1, 1);
  ev |= bmTap(s, 2, 1);
  for (int i = 0; i < 30 && s.phase != BMP_POP; i++) ev |= bmStep(s, DT);
  CHECK(ev & BE_FIRE);
  CHECK(s.gone[1][0] && s.gone[1][5] && s.gone[0][2] && s.gone[5][2]);  // row 1, column 2 (where it went)
  CHECK_EQ(s.gone[0][1], 0);  // (column 1, row 0: neither)
  ev |= settle(s);
  CHECK(full(s));
  CHECK_EQ(s.moves, (uint8_t)(BM_MOVES - 1));
}

TEST(berrymatch, no_swap_left_shuffles_the_same_pieces_into_a_board_with_one) {
  BmState s;
  bmStart(s, 21);
  setStuck(s);
  CHECK(!bmHasMove(s) && !bmAnyMatch(s));
  int count[16] = {};
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++) count[s.b[y][x]]++;
  s.chain = 1;
  s.phase = BMP_FALL;  // as at the end of a fall with nothing more to catch
  uint32_t ev = bmResolve(s, nullptr, 0);
  CHECK(ev & BE_SHUFFLE);
  CHECK(s.phase == BMP_SHUFFLE);
  CHECK(bmHasMove(s));
  CHECK(!bmAnyMatch(s));
  int after[16] = {};
  for (int y = 0; y < BM_N; y++)
    for (int x = 0; x < BM_N; x++) after[s.b[y][x]]++;
  CHECK(memcmp(count, after, sizeof(count)) == 0);  // the same pieces, moved round
  CHECK(bmTap(s, 2, 2) & BE_PICK);  // a piece can be picked while it shuffles...
  ev = settle(s);
  CHECK(s.phase == BMP_PLAY);  // ...and then it's ready again
  CHECK_EQ(s.moves, (uint8_t)BM_MOVES);  // a shuffle costs no move
}

TEST(berrymatch, a_swap_asked_for_while_the_board_moves_is_played_once_it_is_ready) {
  for (uint32_t seed = 1; seed < 40; seed++) {
    BmState s;
    bmStart(s, seed);
    bmFindHint(s);
    int8_t m[4];
    memcpy(m, s.hint, 4);
    bmTap(s, m[0], m[1]);
    bmTap(s, m[2], m[3]);
    // the board moves: pick a piece and ask for a swap with the one beside it
    for (int i = 0; i < 8; i++) bmStep(s, DT);
    if (s.phase == BMP_PLAY) continue;
    bmTap(s, 2, 2);
    CHECK(s.selX == 2 && s.selY == 2);
    bmTap(s, 3, 2);
    CHECK(s.queueX == 3 && s.queueY == 2);
    int moves = s.moves;
    uint32_t ev = 0;
    for (int i = 0; i < 600 && !(ev & BE_SWAP) && s.phase != BMP_END; i++) ev |= bmStep(s, DT);
    if (s.phase == BMP_END) continue;
    CHECK(ev & BE_SWAP);  // the swap went once the board was ready
    settle(s);
    CHECK(s.moves == moves || s.moves == moves - 1);  // (it may not have worked: then it swapped back)
    return;
  }
  CHECK(false);  // no seed tried it
}

TEST(berrymatch, the_hint_is_a_swap_that_works_shown_after_a_while_idle_three_times_a_game_at_most) {
  for (uint32_t seed = 1; seed < 50; seed++) {
    BmState s;
    bmStart(s, seed);
    CHECK(s.hint[0] >= 0);
    CHECK(bmSwapWorks(s, s.hint[0], s.hint[1], s.hint[2], s.hint[3]));
    for (float t = 0; t < BM_HINT_S - 0.5f; t += DT) bmStep(s, DT);
    CHECK(!bmHintShows(s));  // not yet
    for (float t = 0; t < 0.6f; t += DT) bmStep(s, DT);
    CHECK(bmHintShows(s));
    bmTap(s, 2, 2);  // a tap: not idle any more
    CHECK(!bmHintShows(s));
    bmTap(s, 2, 2);  // (put down again)
  }
  BmState s;
  bmStart(s, 7);
  int shown = 0;
  for (int k = 0; k < BM_HINTS + 3; k++) {  // left alone again and again: the first BM_HINTS times only
    for (float t = 0; t < BM_HINT_S + 0.2f; t += DT) bmStep(s, DT);
    shown += bmHintShows(s);
    bmTap(s, 2, 2);
    bmTap(s, 2, 2);
  }
  CHECK_EQ(shown, BM_HINTS);
  CHECK_EQ(s.hintsLeft, 0);
}

TEST(berrymatch, the_keys_move_a_cursor_past_the_corners_space_picks_and_an_arrow_swaps) {
  BmState s;
  bmStart(s, 4);
  CHECK(!s.keys);
  s.curX = 1, s.curY = 1;
  bmKey(s, -1, 0);
  CHECK(s.keys);
  CHECK(s.curX == 0 && s.curY == 1);
  bmKey(s, 0, -1);  // (0, 0) is a corner that's off: the cursor stays
  CHECK(s.curX == 0 && s.curY == 1);
  s.curX = 1, s.curY = 0;
  bmKey(s, -1, 0);  // the same: nothing in play that way
  CHECK(s.curX == 1 && s.curY == 0);
  bmKey(s, 1, 0);
  CHECK(s.curX == 2 && s.curY == 0);
  // pick with Space, then an arrow swaps that way
  bmFindHint(s);
  s.curX = s.hint[0], s.curY = s.hint[1];
  CHECK(bmKeyPick(s) & BE_PICK);
  int dx = s.hint[2] - s.hint[0], dy = s.hint[3] - s.hint[1];
  CHECK(bmKey(s, dx, dy) & BE_SWAP);
  CHECK(s.curX == s.hint[2] && s.curY == s.hint[3]);
  uint32_t ev = settle(s);
  CHECK(ev & BE_POP);
  CHECK_EQ(s.moves, (uint8_t)(BM_MOVES - 1));
}

// a bot that plays a working swap each turn (the hint), as a player would
static uint32_t playGame(BmState &s, uint32_t seed, int *shuffles = nullptr) {
  bmStart(s, seed);
  uint32_t all = 0;
  int guard = 0;
  while (s.phase != BMP_END && guard++ < 200) {
    CHECK(s.phase == BMP_PLAY);
    CHECK(full(s));
    CHECK(!bmAnyMatch(s));
    CHECK(bmHasMove(s));
    if (s.hint[0] < 0) bmFindHint(s);
    uint32_t ev = swap(s, s.hint[0], s.hint[1], s.hint[2], s.hint[3]);
    CHECK(!(ev & BE_BACK));
    if (shuffles && (ev & BE_SHUFFLE)) (*shuffles)++;
    all |= ev;
  }
  return all;
}

TEST(berrymatch, a_whole_game_ends_after_its_moves_the_board_always_full_and_settled) {
  uint64_t total = 0;
  uint32_t lo = 0xFFFFFFFF, hi = 0;
  int shuffles = 0, balls = 0, games = 200;
  uint32_t seen = 0;
  for (int g = 0; g < games; g++) {
    BmState s;
    uint32_t ev = playGame(s, 1000 + g, &shuffles);
    seen |= ev;
    CHECK(ev & BE_END);
    CHECK(ev & BE_LAST);
    CHECK_EQ(s.moves, (uint8_t)0);
    CHECK(s.phase == BMP_END);
    CHECK(s.score >= BM_MOVES * 3 * BM_PTS);  // every move caught three at least
    CHECK(bmTap(s, 2, 2) == 0);  // over: taps do nothing
    total += s.score;
    if (s.score < lo) lo = s.score;
    if (s.score > hi) hi = s.score;
    balls += s.balls;
  }
  CHECK(seen & BE_CHAIN);
  CHECK(seen & BE_BALL);
  CHECK(seen & BE_FIRE);
  printf("    (bot: %d games, score %u..%u, mean %u; %d balls made, %d shuffles)\n", games, lo, hi,
         (unsigned)(total / games), balls, shuffles);
}
