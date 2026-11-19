# BerryMatch

The eighth cartridge, on the Games menu's second page after Pinball: a match-3 in the style of the classic
gem-swapping games, built on the game's own art and look (Fedor, 2026-11-19). The player's rules are in
[HOW_IT_WORKS.md](HOW_IT_WORKS.md#berrymatch).

- **The rules** are `src/berrymatch.h` (no drawing), checked by `test/test_berrymatch.cpp`: the board's fit on the
  round screen, a fresh deal (no line, a move), finding lines, a swap that catches and one that swaps back, gravity and
  the refill, chains, the balls made and gone off (and caught by each other), no move left leading to a shuffle of the
  same pieces, taps while the board moves, the hint, the keys, and 200 whole games played by a bot that swaps the hint
  (the board always full and settled between moves; scores 750 to 4,660, about 1,900 on average, 3 balls a game, a
  shuffle in about 1 game in 14).
- **The drawing** is `src/berrymatch_v2.h` (`bmDraw()`): a lawn seen from above with the board on it in the home's
  see-through dark tint, cached together for each light of the day (`v2Cached`). Fedor picked the lawn from four
  (`bmPaintBack()`): Berry Drop's clearing, the first, put the light sky under the board's top half and its tint came
  out grey-blue, hiding the Oran Berries and the Rare Candy; a wooden table and a berry patch were the others (in the
  history at 390d154, `BMSCENE n`). The pieces are the 18 x 18 food icons drawn 58 px (`SPR_HOME_FOOD`,
  `SPR_BERRY_ORAN`, `SPR_BERRY_PINAP`, `SPR_CANDY`, `SPR_HOME_BALL`); the balls are
  master_v2.h's Ultra and Master Balls and a Great Ball made from the same sprite (its cap blue, its marks red, as
  Pinball's bumper has them). The cartridge's label (`bmPaintLabel()`) is a little board with three Oran Berries lined
  up and Squirtle watching.
- **In the sketch:** `startMatch()`, `bmEvents()` (the sounds, the words over the board, the end's reward and record),
  `matchTouchDown()` (a square tapped as the finger goes down), `matchButton()` (the card's READY, GO!, and Space on a
  computer), `renderMatch()` (the arrow keys, the step, the drawing, the end). The console's `BERRYMATCH` starts it
  under its card; on a test copy (`NOSAVE`), `BMBOT` plays it by itself, `BMTAP x y` taps a square, `BMSET x y p` puts
  a piece there (1 to 5 the kinds, 8 Great, 9 Ultra, 10 Master), `BMSTUCK` leaves no move so it shuffles, and
  `BMMOVES n` sets the moves left.
- **Saved:** the record as `bmhi`, the leaderboard's best since it started over as `lbbm` (apart from the other games'
  `lbhi` blob, which older saves read whole). The game sends it to the relay as game 7 (`scoreReport()`), and it's on
  the leaderboard: the relay's `GAMES[7]` (`relay/src/board.js`, a score up to 999,999 shown with commas), the board
  picture's last row with the pink cartridge (`tools/relay_board_art.sh` crops it from the Games menu's second page),
  the leaderboard page's last card. It counts toward asking for the gamer tag (`Pet::hasScore()`). A relay from before
  this drops game 7, so the relay goes out before the game does.

## As built

- **The board:** 6 x 6 squares of 62 px with the four corners off: every square's corners lie within 226 px of the
  screen's middle (a test checks it), and the squares are the biggest that fit six across (the zone squares that work
  on the Knomi are 78 px, the Search bar 56 px tall). First built at 60 px with the icons at 3x (54 px); after Fedor
  tried it on his Knomi ("just a hair larger", big fingers) 62 px squares and 58 px pieces, drawn in 2 px cells,
  and a near miss beside a picked piece still swaps (`bmTapAt()`, `bmTapNear()`). A 5 x 5 board of 76 px squares with
  the icons at 4x and 4 kinds was simulated as the next step up (it plays about the same: shuffles 0.06 a game).
  The score sits over the board, the moves left under it.
- **Five kinds:** with four the board lines up too much by itself; with five a game still has a move nearly always
  (the shuffle is for the rest).
- **Hints:** after 10 s with nothing done (`BM_HINT_S`), three a game (`BM_HINTS`; Fedor: at 6 s with no limit they
  came too soon and too often).
- **Moves, not time:** 20 swaps a game, so nothing hurries a finger on the small screen. A swap that swaps back costs
  nothing.
- **Pace:** a swap slides in 0.16 s, the catch pops in 0.3 s, pieces fall from 5 squares a second speeding up by 60;
  the game runs at the arcade rate (25 frames a second on the Knomi) while it's open.
- **Sounds:** the effects there are: a tap to pick, the reel's click to swap, the deny buzz for a swap back, a hit for
  a catch, the level-up for a chain, the hook for a ball made, the cast for one going off, the evolution for a Master
  Ball, a shake for the shuffle. The song is Berry Drop's (`MUS_BERRY`); a song of its own would be a new one in
  `tools/music`.
