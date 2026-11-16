# Pinball: the plan

The seventh cartridge (Fedor, 2026-11-15: "let's branch off to make a pinball mini game"). The Games menu already
has its cartridge, on its second page (four cartridges a page now); tapping it says no until the game is in.

This is a proposal to decide on before building. Nothing below is settled yet.

## The table: the round screen is the table

- **The rim is the wall.** The ball rolls inside the Knomi's circle, so the table is round, not a tall rectangle
  squeezed onto a round screen. The circle's own edge bounces it back, and at the bottom a gap between the two flippers
  is the drain.
- **Seen from above**, in the games' pixel style: a night-blue playfield with lane lights, like the cartridge's label.
- **In the upper half:** three round bumpers that kick the ball away and light up. **In the middle:** your own Pokemon
  as a target; hit it 3 times for a bonus and it cheers. **Round the sides:** a row of berry lights to roll over; light
  them all for a multiplier.
- **The ball** is a Poke Ball.
- It's an original layout, not a copy of any existing pinball game's table or modes.

## Controls

- **Knomi:** tap and hold the left or right half of the screen to raise that flipper. The button raises both.
- **Computers and the web:** Left and Right arrows (or Z and /) for each flipper, Space for both, and clicks on either
  half work as taps do. A controller uses its shoulder buttons.
- **The launch:** the ball rolls in from the top, so there's no plunger to learn.
- **Later, maybe:** a nudge, by tilting the Knomi or pressing Up.

## Pace: made for 10 frames a second

The Knomi draws about 10 frames a second. A fast ball would jump across the screen between frames, so the table plays
like a gentle "moon" table: low gravity and a top speed the eye can follow at 10 frames. The physics runs in many
small steps between frames, so the ball never passes through a flipper. Computers draw every frame, so the same
gentle table looks smooth there too.

## A game

- **3 balls.** Bumpers 10 points, berry lights 25, your Pokemon 100, and the multiplier on top.
- **Records and rewards** work as in the other games: a best score with NEW RECORD!, joy and energy as Berry Drop
  gives them, and Repel seconds for the time actually played.
- **Play Again!** and the X at the end, as in every game.

## The mockups (2026-11-15, Fedor: "Mockups first")

Drawn in the real game by `src/pinball_v2.h`, on a computer's console: `PINMOCK n` (1 to 9, 0 off).

1. The start: the ball rolls in at the top, nothing lit yet, and TAP LEFT OR RIGHT over the flippers.
2. Its how-to card over the table.
3. A flip: a finger on the left half (that half's rim glows), the left flipper up, the ball flying off it.
4. A bumper: the ball off the right one, which lights up and kicks it, +10.
5. Your Pokemon hit: it lights up, +100, the second of three toward its bonus.
6. Every berry lit: BERRIES x2!, and x2 by the score from then on.
7. A lost ball: down the drain between the flippers, the bottom glowing red, BALL 2.
8. The end, as every game's: the score, NEW RECORD!, the joy, PLAY AGAIN!, the Repel row and the X.
9. The other look: a meadow on a wooden rim, by day (the same moment as 4).

## To decide (Fedor)

1. Is the table as above (round, bumpers, your Pokemon as the target), or something else?
2. Are the controls right (tap a half, hold to keep that flipper up)?
3. Do you want mockups of the table first (as Reel Fishing had), or a playable first version to try on the Knomi and
   a computer?
