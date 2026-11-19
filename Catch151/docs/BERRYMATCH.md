# BerryMatch

The eighth cartridge, on the Games menu's second page after Pinball: a match-3 in the style of the classic
gem-swapping games, built on the game's own art and look (Fedor, 2026-11-19). The player's rules are in
[HOW_IT_WORKS.md](HOW_IT_WORKS.md#berrymatch).

- **The rules** are `src/berrymatch.h` (no drawing), checked by `test/test_berrymatch.cpp`: the board's fit on the
  round screen, a fresh deal (no line, a move), finding lines, a swap that catches and one that swaps back, gravity and
  the refill, chains, the balls made and gone off (and caught by each other), no move left leading to a shuffle of the
  same pieces, taps while the board moves, the hint, the keys, and 200 whole games played by a bot that swaps the hint
  (the board always full and settled between moves; on the 5 x 5 board scores 990 to 4,640, about 2,260 on average,
  4 balls a game, a shuffle in about 1 game in 30).
- **The drawing** is `src/berrymatch_v2.h` (`bmDraw()`): a lawn seen from above with the board on it in the home's
  see-through dark tint, cached together for each light of the day (`v2Cached`). Fedor picked the lawn from four
  (`bmPaintBack()`): Berry Drop's clearing, the first, put the light sky under the board's top half and its tint came
  out grey-blue, hiding the Oran Berries and the Rare Candy; a wooden table and a berry patch were the others (in the
  history at 390d154, `BMSCENE n`). The pieces are the 18 x 18 food icons at 4x, 72 px, as the backpack draws them
  (`SPR_HOME_FOOD`, `SPR_BERRY_ORAN`, `SPR_BERRY_PINAP`, `SPR_CANDY`); the balls are
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

- **The board:** 5 x 5 squares of 76 px with the four corners off (21 squares), its top left on an even pixel (42):
  every square's corners lie within 226 px of the screen's middle (a test checks it). The pieces are the icons at 4x
  (72 px = 36 of the Knomi's pixels), the only size that's sharp on the Knomi and fits: there the panel shows each 2 x
  2 block as one pixel, so 18 px art is clean at 36, 72 or 108 px only. History: 6 x 6 at 60 px with the icons at 3x
  (54 px), then "just a hair larger" (Fedor, big fingers) 62 px squares and 58 px pieces, which on the Knomi came out
  uneven ("so low resolution compared to the berries ... on the main ui"); a near miss beside a picked piece still
  swaps since then (`bmTapAt()`, `bmTapNear()`). The 5 x 5 board was simulated first (it plays about the same) and
  tried on Fedor's Knomi as a preview from the branch `berrymatch-5x5`. The score sits over the board, the moves left
  under it.
- **Sharp on the Knomi:** its panel shows each 2 x 2 block from an even pixel as one pixel, so a piece's cells start on
  even pixels (`SNAP2(cx - n)`). The first 58 px build drew them one pixel off and every piece blurred (Fedor: "all
  the graphics seem blurry on the knomi"), and the board itself started on an odd pixel (47) until `BM_X0` rounded it
  down; a host test checks a Razz Berry's white lies in whole blocks.
- **Four kinds:** Razz, Oran, Pinap, Rare Candy. On 21 squares five kinds would need a shuffle in most games; with four
  a game still nearly always has a move. The plain Poke Ball, a fifth kind on the 6 x 6 board, went: it looked most
  like the Great, Ultra and Master Balls.
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
