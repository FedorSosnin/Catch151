# Pinball: the plan

The seventh cartridge (Fedor, 2026-11-15: "let's branch off to make a pinball mini game"). The Games menu already
has its cartridge, on its second page (four cartridges a page now); tapping it says no until the game is in.

This is a proposal to decide on before building. Nothing below is settled yet.

## The table: the round screen is the table

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
- **The battle spot**, turned to the left flipper. The rock cave is gone (Fedor: "minus the cave"). Two ideas for what
  replaces it:
  - **A: a patch of tall grass** on the right (the pick). It's where wild Pokemon come from in the games, with a
    hole of stones in its front and two eyes glowing in it.
  - **B: a tunnel into the right wall**, a stone arch round a dark mouth.
- **Slingshots** over the flippers, and the guides down to them.

### Raised: over and under (Fedor: "ramps and areas the ball can go under and over")

- **The ramp** rises from its mouth left of the middle (a lit arrow shows the shot, from the right flipper), up the
  left, and **over the orbit lane**. There it becomes a **wire rail** running down the left rim, over the lane, and
  lets the ball down into the left inlane, onto the left flipper: a ramp shot comes back to you (+250).
- **Under it:** a ball going round the orbit passes **under the wire rail** along the left (in its shade, the wires
  across it). On the ramp the ball is drawn bigger and its shadow falls further off, so it's plain which level it's on.

### The battle (Fedor: "The mini battle is cool too")

- In goes the ball; **BATTLE!**, and a round window opens over the dimmed table: a clearing in the tall grass, a wild
  Pokemon of the zone you're in, and yours facing it.
- **Tap (or press any key) to attack**, as fast as you like. A gold ring round the window is the time left (a few
  seconds). Knock it out in time: **WIN! +500**; if not, no bonus.
- Either way, out comes the ball, shot from the hole toward the left flipper, and play goes on. The flippers rest
  during the battle, so nothing is lost while you're tapping.
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

## The mockups (2026-11-15; third pass after Fedor's second look)

Drawn in the real game by `src/pinball_v2.h`, on a computer's console: `PINMOCK n` (1 to 25, 0 off).

1. The night table at the start: the ball rolling in along the orbit, TAP LEFT OR RIGHT over the flippers. 2: the
   meadow table, the same moment.
3. The how-to card (the Knomi's words). 20: the same on a computer, saying the keys.
4. A flip: a finger on the left half (that half's rim glows), the ball off the left flipper toward the ramp.
5. Over: the ball up the ramp, over the orbit lane, bigger and its shadow further off: RAMP! +250.
6. Under: the ball round the orbit under the wire rail, lighting a berry: ORBIT +50.
7. A bumper (the Great Ball), +10.
8. All three bumpers lit: Diglett pops out of its hole, its time ring round it: HIT DIGLETT! 9: hit, dizzy, +1000.
10. Into the tall grass: the grass rustling, its hole glowing, BATTLE! 11: the other idea, a tunnel in the right wall.
12. The battle in a clearing in the grass: a wild Oddish, yours facing it, TAP TO ATTACK! 13: won, +500.
14. Spat back out of the grass toward the left flipper, +500.
15. Your Pokemon hit, +100. 16. Every berry lit: BERRIES x2!
17. A lost ball: down the drain, the bottom glowing red, BALL 2.
18. The end, as every game's: the score, NEW RECORD!, the joy, PLAY AGAIN!, the Repel row and the X.
19. On a computer: the keys over the flippers (Left and Right flip, Space both) in place of the touch hint.
21. Moving: the ball rolling round the orbit. 22: the ball close up, rolling right, down and left.
23 to 25: the meadow look of 5, 8 and 12.

## To decide (Fedor)

1. The battle spot: the tall grass (A) or a tunnel in the wall (B)?
2. Does a battle won count for anything outside Pinball (seen in the Pokedex, say), or only points?
3. Anything more on the table, or ready to build?

(Decided: mockups first; both looks, one at random each game with its own song; Left and Right arrows flip on
computers; the ball rolls; Diglett for all three bumpers.)
