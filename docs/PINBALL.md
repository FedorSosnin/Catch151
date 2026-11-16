# Pinball

The seventh cartridge (Fedor, 2026-11-15: "let's branch off to make a pinball mini game"), on the Games menu's second
page (four cartridges a page now). Built 2026-11-16 (Fedor: "Build it"), on the `pinball` branch: playable on the
Knomi, the Mac and Windows apps and the web.

- **The rules and the physics** are `src/pinball.h` (no drawing), checked by `test/test_pinball.cpp`: the serve,
  gravity and the rim, the flippers' throw, the drain and the three balls, the bumpers and Diglett, the berries'
  multiplier, the ramp and the wire rail, the grass and its battle, your Pokemon as a target, and 30 whole games
  played by a simple bot, the ball never leaving the table.
- **The drawing** is `src/pinball_v2.h`: the live game (`pinDraw()`, `pbBattleDraw()`), and the mockups it grew from
  (`PINMOCK n` on a computer's console).
- **In the sketch:** `startPinball()`, `pinEvents()` (the sounds), `pinInput()` (the flippers from a finger, the
  arrow keys or the button), `pinBattleSetup()` and `pinMove()` (the battle), `renderPinball()`. The console's
  `PINBALL` starts it and `PINBOT` plays it by itself.
- **To try it on a Mac:** `tools/pinball_test.sh` (or double-click `tools/Pinball Test.command`): a test copy with its
  own save and buddy port, never the real app or its save, opening on Pinball's card.

## As built

- **Pace:** gravity 300 px/s^2 and a top speed of 560 px/s, so the ball reads at the Knomi's 10 frames a second; the
  physics runs 240 steps a second, the flippers swinging in those steps, so the ball never passes through one.
- **The table changed from the mockups in two places,** both for the ball's sake: the slingshots sit 22 px higher,
  so the inlanes under them are wide enough for the ball to roll down to the flippers (it got stuck between them), and
  the orbit's inner rail is shorter (200 to 340 degrees round, r 180), so its mouths open down each side.
- **A stuck ball** (nearly still for 2.5 s anywhere but held up on a flipper) gets a nudge.
- **The battle:** a wild Pokemon picked as a wild battle picks one, at about your level; your Pokemon's two moves do
  their real damage (gen 1's formula, type matchups and all); a move lands every 0.35 s at most; 6 seconds to knock it
  out. Nothing outside Pinball changes (the Pokedex, the battle record): only points.
- **The songs** are, for now, the pier's calm one at night and the Field zone's by day, the wild battle's in a
  battle. Two songs of its own would come next.
- **Rewards:** as the other games: the best score is kept (`pinHi`), and a point of exercise for every 400.

## Still to tune once it's played

- The orbit is a skill shot: up either side along the rim. The bot never makes one; a person aiming should.
- How long a game lasts: 30 bot games ran 10 to 100 seconds.
- The battle's 6 seconds and the wild one's HP.

## The design

### The table: the round screen is the table

- **The rim is the wall.** The ball rolls inside the Knomi's circle, so the table is round, not a tall rectangle
  squeezed onto a round screen. The circle's own edge bounces it back, and at the bottom a gap between the two flippers
  is the drain.
- **Two looks, one each game at random, each with its own song** (Fedor: "I like both looks. Maybe its random which
  you get with music to match"): a **night table**, night-blue with stars on a chrome rim, and a **meadow table**, grass
  and flowers on a wooden rim. The battle window takes the same look (a starry clearing, or a sunny one). Two new songs
  for the game's own synthesizer, as Reel Fishing has its two: a mellow night tune and a bright meadow one.
- **The ball** is a Poke Ball that **rolls**: as it moves, its red half, band and button turn over the way a real ball
  would (Fedor: "make sure the ball spins as it moves"). Each pixel of it is a point on a sphere, turned by how far it
  has rolled, so it rolls true in any direction, and it's lit from one side so it reads as round.
- It's an original layout, not a copy of any existing pinball game's table or modes.

### On the floor

- **The orbit:** a lane round the top, inside the rim, with a red rubber post at each end. A ball shot up either side
  runs round the top and comes down the other side (+50). Its six **berry lights** sit in it; light them all for
  **x2** on everything after.
- **Three bumpers** in the upper middle, painted as a **Poke Ball, a Great Ball and an Ultra Ball**. Each kicks the
  ball away and lights up (+10).
- **Diglett** (Fedor: "hitting all three of those pins in the middles makes some mini event happen? Like diglet pops up
  there and you have some set time to hit him with the ball to beat him and get points"): there's a dirt hole between
  the bumpers. Light all three bumpers and **Diglett pops out of it**, throwing up dirt, with a gold ring round it
  for the time left (a few seconds). Hit it with the ball before the ring runs out: knocked dizzy, **+1000**, and back
  down its hole. Too slow and it ducks back down. Then the bumpers start over.
- **Your own Pokemon** on a pad in the middle: hit it 3 times for a bonus (+100 a hit).
- **The tall grass**, on the right, where wild Pokemon wait (it took the rock cave's place: Fedor, "minus the cave",
  then "If we use the grass"). It's drawn in the Game Boy games' spirit (Fedor: "grass that resembles grass from
  Pokemon Blue"): an original 16 px tile, 8 x 8 Knomi pixels with two staggered tufts, each a dark edge round light
  blades on a mid green, in a night and a meadow colouring (two tufts a tile, not four: Fedor, "Maybe its the density
  of the grass thats a bit much"). The patch has a dark edge, and tufts poke up along its top for
  height.
- **The way into the grass** (Fedor, of the stone hole: "That entrance there still doesnt look good"): a gap where the
  grass parts on its lower-left side, turned to the left flipper. The floor shows through it, darkening deeper in, the
  grass's dark edge runs down both its sides with tufts leaning over, and a lit arrow on the floor points up it. A
  ball that goes in disappears into the blades (the grass in front draws over it), the grass rustles (every other row
  of tufts shifts), and a "!" pops up over the patch, as when a trainer spots you.
- **Slingshots** over the flippers, and the guides down to them.

### Raised: over and under (Fedor: "ramps and areas the ball can go under and over")

- **The ramp** rises from its mouth left of the middle (a lit arrow shows the shot, from the right flipper), up the
  left, and **over the orbit lane**. There it becomes a **wire rail** running down the left rim, over the lane, and
  lets the ball down into the left inlane, onto the left flipper: a ramp shot comes back to you (+250).
- **Under it:** a ball going round the orbit passes **under the wire rail** along the left (in its shade, the wires
  across it). On the ramp the ball is drawn bigger and its shadow falls further off, so it's plain which level it's on.

### The battle (Fedor: "The mini battle is cool too", then "the battle scene should be a bit stronger")

- In goes the ball, and **the game's own battle** takes the whole screen: the wild battle's scene with its two bases,
  a wild Pokemon of the zone you're in **standing in tall grass** on its base (the grass in front of its feet), yours
  from behind on its base, and both HP boxes, all as in a real battle (Fedor: "I like the island your opponent floats
  on"). No grass along the bottom (Fedor: "I would remove the gras frome the bottomof the battle screen"). First it
  says **Wild ODDISH!**
- **Tapping does your attack** (Fedor: "Tapping to attack should do your attack skill"). The buttons are your
  Pokemon's own two moves, as in a battle (Squirtle's BITE and WATER GUN, say), in their types' colours, with
  **TAP A MOVE!** under them. Tap the left or right half (or press Left or Right on a computer) to use that move, as
  often as you can. Each lands as in a battle: yours lunges, the move's effect hits, a line says
  **SQUIRTLE: WATER GUN!**, and its HP drops; type matchups count.
- A **gold ring round the rim** is the time left (a few seconds). Knock it out in time: **ODDISH fainted...**,
  **WIN! +500**; if not, no bonus.
- Either way, the ball flies back out of the grass, down the gap toward the left flipper, and play goes on. The
  flippers rest during the battle, so nothing is lost while you're tapping.
- Open: should a win also count for something outside Pinball (seen in the Pokedex, say), or stay just points?

## Controls

- **Knomi:** tap and hold the left or right half of the screen to raise that flipper (that half's rim glows while
  it's held). The button raises both.
- **Computers and the web** (Fedor: "the arrow keys can be used for the flippers"): **Left and Right arrows** raise each
  flipper, **Space** both; a click on either half works as a tap. The hint over the flippers shows the keys instead of
  TAP LEFT OR RIGHT, and the how-to card says them. A controller uses its shoulder buttons (and the d-pad's left and
  right).
- **The launch:** the ball rolls in along the orbit at the top, so there's no plunger to learn.
- **Later, maybe:** a nudge, by tilting the Knomi or pressing Up.

## Pace: made for 10 frames a second

The Knomi draws about 10 frames a second. A fast ball would jump across the screen between frames, so the table plays
like a gentle "moon" table: low gravity and a top speed the eye can follow at 10 frames. The physics runs in many
small steps between frames, so the ball never passes through a flipper. Computers draw every frame, so the same
gentle table looks smooth there too.

## A game

- **3 balls.** Bumpers 10, the orbit 50, your Pokemon 100, the ramp 250, a battle won 500, Diglett 1000, and the x2
  on top.
- **Records and rewards** work as in the other games: a best score with NEW RECORD!, joy and energy as Berry Drop
  gives them, and Repel seconds for the time actually played.
- **Play Again!** and the X at the end, as in every game.

## The mockups (2026-11-15; fourth pass after Fedor's third look)

Drawn in the real game by `src/pinball_v2.h`, on a computer's console: `PINMOCK n` (1 to 26, 0 off).

1. The night table at the start: the ball rolling in along the orbit, TAP LEFT OR RIGHT over the flippers. 2: the
   meadow table, the same moment.
3. The how-to card (the Knomi's words). 21: the same on a computer, saying the keys.
4. A flip: a finger on the left half (that half's rim glows), the ball off the left flipper toward the ramp.
5. Over: the ball up the ramp, over the orbit lane, bigger and its shadow further off: RAMP! +250.
6. Under: the ball round the orbit under the wire rail, lighting a berry: ORBIT +50.
7. A bumper (the Great Ball), +10.
8. All three bumpers lit: Diglett pops out of its hole, its time ring round it: HIT DIGLETT! 9: hit, dizzy, +1000.
10. Into the tall grass: the ball gone into the blades, the grass rustling, a "!" over it.
11. The battle: Wild ODDISH!, standing in tall grass on its base, yours from behind, both HP boxes.
12. Tap a move: your Pokemon's two moves as the buttons, TAP A MOVE!, the time ring round the rim.
13. Your move landing: SQUIRTLE: WATER GUN!, its splash on Oddish, Oddish's HP down.
14. Won: Oddish gone, ODDISH fainted..., WIN! +500.
15. Spat back out of the grass toward the left flipper, +500.
16. Your Pokemon hit, +100. 17. Every berry lit: BERRIES x2!
18. A lost ball: down the drain, the bottom glowing red, BALL 2.
19. The end, as every game's: the score, NEW RECORD!, the joy, PLAY AGAIN!, the Repel row and the X.
20. On a computer: the keys over the flippers (Left and Right flip, Space both) in place of the touch hint.
22. Moving: the ball rolling round the orbit. 23: the ball close up, rolling right, down and left.
24 to 26: the meadow look of 5, 8 and 10.

## To decide (Fedor)

1. Does a battle won count for anything outside Pinball (seen in the Pokedex, say), or only points?
2. Anything more on the table, or ready to build?

(Decided: mockups first; both looks, one at random each game with its own song; Left and Right arrows flip on
computers; the ball rolls; Diglett for all three bumpers; the tall grass for the battles, its way in a gap where the
grass parts; the battle in the game's own battle scene, a tap using your Pokemon's move.)
