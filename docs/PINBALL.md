# Pinball: the plan

The seventh cartridge (Fedor, 2026-11-15: "let's branch off to make a pinball mini game"). The Games menu already
has its cartridge, on its second page (four cartridges a page now); tapping it says no until the game is in.

This is a proposal to decide on before building. Nothing below is settled yet.

## The table: the round screen is the table

- **The rim is the wall.** The ball rolls inside the Knomi's circle, so the table is round, not a tall rectangle
  squeezed onto a round screen. The circle's own edge bounces it back, and at the bottom a gap between the two flippers
  is the drain.
- **Seen from above**, in the games' pixel style: a night-blue playfield on a chrome rim (or a meadow on a wooden one).
- **The ball** is a Poke Ball.
- It's an original layout, not a copy of any existing pinball game's table or modes.

### On the floor

- **The orbit:** a lane round the top, inside the rim. A ball shot up either side runs round the top and comes down
  the other side (+50). Its six **berry lights** sit in it; light them all for **x2** on everything after.
- **Three bumpers** in the upper middle kick the ball away and light up (+10).
- **Your own Pokemon** on a pad in the middle: hit it 3 times for a bonus (+100 a hit).
- **The cave**, a rock on the right with a wild Pokemon's eyes in its dark mouth, turned to the left flipper. Shoot the
  ball in and it's held there for **a battle** (below), then shot back out toward the left flipper.
- **Slingshots** over the flippers, and the guides down to them.

### Raised: over and under (Fedor: "ramps and areas the ball can go under and over")

- **The ramp** rises from its mouth left of the middle (a lit arrow shows the shot, from the right flipper), up the
  left, and **over the orbit lane**. There it becomes a **wire rail** running down the left rim, over the lane, and
  lets the ball down into the left inlane, onto the left flipper: a ramp shot comes back to you (+250).
- **Under it:** a ball going round the orbit passes **under the wire rail** along the left (in its shade, the wires
  across it). On the ramp the ball is drawn bigger and its shadow falls further off, so it's plain which level it's on.

### The cave's battle (Fedor: "a spot the ball can go into ... where a mini battle happens and the ball gets spit back out")

- In goes the ball; **BATTLE!**, and a round window opens over the dimmed table: the cave inside, a wild Pokemon of the
  zone you're in, and yours facing it.
- **Tap (or press any key) to attack**, as fast as you like. A gold ring round the window is the time left (a few
  seconds). Knock it out in time: **WIN! +500**; if not, no bonus.
- Either way, out comes the ball, shot from the cave's mouth toward the left flipper, and play goes on. The flippers
  rest during the battle, so nothing is lost while you're tapping.
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

- **3 balls.** Bumpers 10, the orbit 50, your Pokemon 100, the ramp 250, a battle won 500, and the x2 on top.
- **Records and rewards** work as in the other games: a best score with NEW RECORD!, joy and energy as Berry Drop
  gives them, and Repel seconds for the time actually played.
- **Play Again!** and the X at the end, as in every game.

## The mockups (2026-11-15, Fedor: "Mockups first"; the ramp, wire rail, orbit and cave added after his first look)

Drawn in the real game by `src/pinball_v2.h`, on a computer's console: `PINMOCK n` (1 to 17, 0 off).

1. The start: the ball rolling in along the orbit, nothing lit yet, TAP LEFT OR RIGHT over the flippers.
2. The how-to card (the Knomi's words). 17: the same on a computer, saying the keys.
3. A flip: a finger on the left half (that half's rim glows), the ball off the left flipper toward the ramp.
4. Over: the ball up the ramp, over the orbit lane, bigger and its shadow further off: RAMP! +250.
5. Under: the ball round the orbit under the wire rail, lighting a berry: ORBIT +50.
6. Into the cave: its mouth glowing, BATTLE!
7. The battle: a wild Zubat, yours facing it, TAP TO ATTACK!, the time ring two thirds full. 8: won, +500.
9. Spat back out: the ball shot from the cave toward the left flipper, +500 over it.
10. A bumper, +10. 11. Your Pokemon hit, +100. 12. Every berry lit: BERRIES x2!
13. A lost ball: down the drain, the bottom glowing red, BALL 2.
14. The end, as every game's: the score, NEW RECORD!, the joy, PLAY AGAIN!, the Repel row and the X.
15. The other look: a meadow on a wooden rim, by day (the same moment as 4).
16. On a computer: the keys over the flippers (Left and Right flip, Space both) in place of the touch hint.

## To decide (Fedor)

1. Is the table right now (the ramp and wire rail, the orbit, the cave, the bumpers, your Pokemon), or more to add?
2. Which look: the night-blue table on a chrome rim, or the meadow on a wooden rim (15)?
3. Does a battle won in the cave count for anything outside Pinball, or only points?

(Decided: mockups first, then a playable version; Left and Right arrows flip on computers.)
