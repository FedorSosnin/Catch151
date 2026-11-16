# Pinball

The seventh cartridge (Fedor, 2026-11-15: "let's branch off to make a pinball mini game"), on the Games menu's second
page (four cartridges a page now). Built 2026-11-16 (Fedor: "Build it") and released the same day in 2.2.7: playable on the
Knomi, the Mac and Windows apps and the web. Reworked the same day after Fedor played the Mac test copy: the battle
moved onto the table, a smooth ramp, no trail behind the ball, a song of its own, and a smaller patch of grass. Then
(after 2.2.7): a bigger ball, the corners where the wall meets the guides rounded off, Diglett evolving into Dugtrio,
and a cave for the grass on the night table.

- **The rules and the physics** are `src/pinball.h` (no drawing), checked by `test/test_pinball.cpp`: the serve,
  gravity and the rim, the flippers' throw, the drain and the three balls, the bumpers, Diglett and Dugtrio, the
  rounded corners keeping a ball's speed, the berries' multiplier, the ramp's smooth turn and the wire rail, the grass and the battle in the ring, and 30 whole games played
  by a simple bot, the ball never leaving the table.
- **The drawing** is `src/pinball_v2.h` (`pinDraw()`). The mockups it grew from (`PINMOCK n`) are gone from the code;
  they're in the history, at commit 9eacb87.
- **In the sketch:** `startPinball()`, `pinEvents()` (the sounds), `pinInput()` (the flippers from a finger, the
  arrow keys or the button), `pinBattleSetup()` (who comes out of the grass, and what each hit does),
  `renderPinball()`. The console's `PINBALL` starts it, `PINBOT` plays it by itself, `PINGRASS` drops the ball
  into the grass for a battle, `PINDIG` brings Diglett up and `PINDUG` brings it up evolving into Dugtrio (all but
  the first a test copy's only).
- **To try it on a Mac:** `tools/pinball_test.sh` (or double-click `tools/Pinball Test.command`): a test copy with its
  own save and buddy port, never the real app or its save, opening on Pinball's card.

## As built

- **Pace:** gravity 300 px/s^2 and a top speed of 560 px/s, so the ball reads at the Knomi's 10 frames a second; the
  physics runs 240 steps a second, the flippers swinging in those steps, so the ball never passes through one.
- **The ball** is 13 px round (it was 10: Fedor, shown 10, 11 and 12, "Lets do with 13px. That looks the best").
- **The table changed from the mockups in two places,** both for the ball's sake: the slingshots sit 28 px higher
  (22, then 6 more for the 13 px ball), so the inlanes under them are wide enough for the ball to roll down to the
  flippers (it got stuck between them), and the orbit's inner rail is shorter (200 to 340 degrees round, r 180), so
  its mouths open down each side.
- **The corners where the wall meets the guides are rounded** (Fedor: "round off the edges ... So the ball can get
  some speed if its rolling around the parimeter"): a curve 100 px round from the wall onto each guide, drawn as the
  rim carried round it. A ball rolling down the wall onto a guide keeps 95% of its speed; with the sharp corner it kept
  a quarter to three quarters.
- **A stuck ball** (nearly still for 2.5 s anywhere but held up on a flipper) gets a nudge.
- **The ramp** (Fedor: "The ramp's sharp angle makes it feel like there is a physics issue"; then, of four shapes
  drawn in the game, he chose the one up into the top corner, "but maybe have some arrows or something going upward up
  the ramp. And slightly away from that one pin"): straight up from its mouth into the top-left corner, clear of the
  Poke Ball bumper, then round over the top (a 20 px turn, not a corner) into the wall, where the wire rail takes
  over, one track from mouth to cup; no bend between two of its steps is sharper than 35 degrees. Five arrows on its
  floor light one after another up it. Its mouth catches a ball a little wider than before (22 px), so about as many
  shots go up it as before it climbed higher; a few more roll back down. (The hairpin it had for a while looked like a
  paperclip.)
- **No trail behind the ball** (Fedor: "I dont like the trail the ball leaves").
- **The battle, on the table** (Fedor: "The battles need to happen inside the pinball game"): see The battle below.
  Nothing outside Pinball changes (the Pokedex, the battle record): only points.
- **The song:** its own, **Pinball** (`tools/music/songs.py`), a little faster (x1.12) while a wild one is in the ring.
  It's on the soundtrack page with the other games' songs.
- **Rewards:** as the other games: the best score is kept (`pinHi`), and a point of exercise for every 400.

## Still to tune once it's played

- The orbit is a skill shot: up either side along the rim. The bot never makes one; a person aiming should.
- How long a game lasts: 30 bot games ran 10 to 100 seconds; with the 13 px ball and the rounded corners, 300 bot
  games average 106 seconds (33 before), the ball reaching the flippers rolling instead of dropping off a corner.
- The battle's 20 seconds, and two to five hits to knock the wild one out.
- Diglett's 12 seconds (it was 7: Fedor, "The diglet in the middle was too quick to leave").

## The design

### The table: the round screen is the table

- **The rim is the wall.** The ball rolls inside the Knomi's circle, so the table is round, not a tall rectangle
  squeezed onto a round screen. The circle's own edge bounces it back, and at the bottom a gap between the two flippers
  is the drain.
- **Two looks, one each game at random, each with its own song** (Fedor: "I like both looks. Maybe its random which
  you get with music to match"): a **night table**, night-blue with stars on a chrome rim, and a **meadow table**, grass
  and flowers on a wooden rim. (Built, they share one song, Pinball's own: Fedor, "We need to compose a custom tune
  for this".)
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
  for the time left (12 seconds). Hit it with the ball before the ring runs out: knocked dizzy, **+1000**, and back
  down its hole. Too slow and it ducks back down. Then the bumpers start over.
- **Dugtrio** (Fedor: "lets find a reason to evolve diglet to digtrio"): beat Diglett twice, and the next time the
  bumpers bring it up, it **evolves**, as a Pokemon does at home: a gold glow, and it blinks white between Diglett and Dugtrio,
  faster and faster (a dark rim round the white, so the shape reads), EVOLVING!, then HIT DUGTRIO!. It can't be hit
  while it evolves and its time doesn't run. Then it's up 20 seconds, three gold dots under it for its three heads:
  each hit knocks one down (+1000), and the third knocks it out (+2000 more): Diglett again, and the count starts
  over. If it gets away it comes back as Dugtrio, without evolving again. Two beaten, so the third is Dugtrio, three
  heads. (In 300 bot games it evolved in most of them.) At first each Diglett beaten dug a hole beside its own, to
  count toward it; they went (Fedor: "No need for the three holes. They look strange"). (Fedor asked whether the bumpers
  should bring out a random Pokemon instead, "Or should be stick to a theme?": Diglett stays, the bumpers' own event,
  and the random wild Pokemon are the grass's.)
- **The ring** in the middle, where your own Pokemon stood as a target (Fedor: "its hard to get over there because of
  the squirtle"): a ring painted on the floor, empty until a wild one jumps into it from the grass.
- **The tall grass**, by the right wall, where wild Pokemon wait (it took the rock cave's place: Fedor, "minus the cave",
  then "If we use the grass"). It's drawn in the Game Boy games' spirit (Fedor: "grass that resembles grass from
  Pokemon Blue"): an original 16 px tile, 8 x 8 Knomi pixels with two staggered tufts, each a dark edge round light
  blades on a mid green, in a night and a meadow colouring (two tufts a tile, not four: Fedor, "Maybe its the density
  of the grass thats a bit much"). The patch has a dark edge, and tufts poke up along its top for
  height. It's a small patch close to the wall, so a shot off the right flipper reaches the upper right past it
  (Fedor: "It was very hard to get to the upper right size of the screen since the island blocks it mostly").
- **At night, a cave** (Fedor: "On the darker version of the game, Lets change the grassy patch with the entrance to
  a cave entrance"): on the night table the grass is a moonlit mound of rock, as big and in the same place, the same
  to the ball. Boulders along its top, each dark on its far side; its mouth an arch standing on the floor where the
  grass's gap is, turned to the left flipper, rocks hanging from it, two eyes glinting in the dark, pebbles by the way
  in. A ball that goes in is gone into it, its mouth glowing gold, and the wild one jumps out of it into the ring.
  The meadow table keeps its grass. The eyes blink now and then, and are gone while the wild one's out in the ring.
- **Something in the grass** (Fedor: "make it look like there is a pokemon rustling through the grass there"):
  unseen, it wanders round the patch, two seconds on the move, then a moment still. Where it is, a shadow under the
  blades, the blades shaking and three pushed up over it swaying, and bits of leaf jumping up while it moves. The
  patch is still while the wild one's out in the ring.
- **Who comes out** (Fedor: "cave pokemon for the cave and forest/field pokemon for the grass patch"): picked as a
  zone's Search picks (`zones.h`), from the zones that fit: out of the grass the Forest's or the Field's, out of the
  cave the Cave's (Zubat, Geodude, Clefairy, Onix, Cubone and the rest), but never Diglett or Dugtrio, the bumpers'
  own. Each zone's legendary can come out, seldom, as on a Search.
- **The way into the grass** (Fedor, of the stone hole: "That entrance there still doesnt look good"): a gap where the
  grass parts on its lower-left side, turned to the left flipper. The floor shows through it, darkening deeper in, the
  grass's dark edge runs down both its sides with tufts leaning over, and a lit arrow on the floor points up it. A
  ball that goes in disappears into the blades (the grass in front draws over it), the grass rustles (every other row
  of tufts shifts), and a "!" pops up over the patch, as when a trainer spots you.
- **Slingshots** over the flippers, and the guides down to them.

### Raised: over and under (Fedor: "ramps and areas the ball can go under and over")

- **The ramp** rises from its mouth left of the middle (a lit arrow shows the shot, from the right flipper, and arrows
  chase up the ramp itself), up the left into the top corner, and **over the orbit lane**. There it becomes a **wire rail** running down the left rim, over the lane, and
  lets the ball down into the left inlane, onto the left flipper: a ramp shot comes back to you (+250).
- **Under it:** a ball going round the orbit passes **under the wire rail** along the left (in its shade, the wires
  across it). On the ramp the ball is drawn bigger and its shadow falls further off, so it's plain which level it's on.

### The battle, in the ring (Fedor: "The battles need to happen inside the pinball game. Not taken out")

- In goes the ball, a "!" over the grass, and a **wild Pokemon** (one that lives there, at about your level: see
  Who comes out, above) **jumps out of the grass into the ring** in the middle of the table, the ball rolling back out toward the
  right flipper. **Wild ODDISH!**
- It stands in the ring with its **HP over it** and a **gold ring round it** for the time left (20 seconds). The
  flippers keep working: it's the ball that fights.
- **Each hit of the ball on it is your Pokemon's move:** the stronger of its two, its damage worked out as in a battle
  (gen 1's formula, type matchups and all), but always two to five hits to knock it out. A hit flashes it white, the
  move's effect bursts on it, a line says **SQUIRTLE: WATER GUN!**, its HP drops, **+50**, and the ball is kicked
  away.
- Knocked out in time: it flashes and fades, **YOU WON!**, **+500**. If the time runs out: **ODDISH got away...** and
  it hops back into the grass.
- While one's in the ring the grass takes no ball. Losing the ball ends the battle.

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

- **3 balls.** Slingshots 5, bumpers 10, the orbit 50, a hit on the wild one 50, the ramp 250, a battle won 500,
  Diglett 1000, each of Dugtrio's heads 1000 and 2000 more for all three, and the x2 on top.
- **Records and rewards** work as in the other games: a best score with NEW RECORD!, joy and energy as Berry Drop
  gives them, and Repel seconds for the time actually played.
- **Play Again!** and the X at the end, as in every game.

## The mockups (2026-11-15)

Four passes of mockups, drawn in the real game (`PINMOCK n` on a computer's console), came before the build; they're
gone from the code now, and in the history at commit 9eacb87.

## To decide (Fedor)

1. Does a battle won count for anything outside Pinball (seen in the Pokedex, say), or only points?

(Decided: mockups first; both looks, one at random each game; Left and Right arrows flip on computers; the ball rolls;
Diglett for all three bumpers; the tall grass for the battles, its way in a gap where the grass parts. After the first
play: the battle on the table, in a ring where your Pokemon stood, each hit of the ball your Pokemon's move; a smooth
ramp; no trail; Pinball's own song; a smaller patch of grass; Diglett up longer. Then: the ramp up into the top corner,
clear of the bumper, with arrows; a game's end back on its own page of the Games menu. After 2.2.7: the ball 13 px;
the corners at the guides rounded; Diglett evolving into Dugtrio after two are beaten (without the holes counting
toward it); a cave for the grass on the night table; cave Pokemon out of the cave and forest and field ones out of
the grass, and something rustling through the grass.)
