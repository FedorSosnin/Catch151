# PokeFish: the plan (DRAFT for Fedor, mockups only, nothing built)

A sixth game on the Games Game Boy. Your Pokemon fishes off the end of a pier at the Beach. You get 5 casts: wait for
the float to go under, tap, and reel it in without snapping the line. What bites are the Beach's water Pokemon.

The mockups are drawn in the game itself (`src/fishing_v2.h`, console `FISHMOCK 1..13`, the Berry Garden mockups' way).
The pictures are in the session notes (`fishing/`): `fishing-1-cast-to-bite.png`, `fishing-2-fight-to-end.png` and
`fishing-3-cues.png`.

## The problem: the Knomi can't make a sound or buzz

A fishing game lives on the bite: feel the tug, strike now. The Knomi has no speaker and no vibration motor. So every
moment is built in three layers:

1. **The screen (every device).** Each moment has its own look, and that alone is enough to play well. This is all
   the Knomi gets.
2. **+ Sound (Mac, Windows, web).** A sound for each moment, and the music changes when a fish is hooked.
3. **+ Buzz (the web on Android phones, Windows with a controller).** It comes through the hook that already buzzes
   for every sound effect (`Module.haptic` with its `BUZZ` table in `webgame/shell.html`, and `kBuzz` in
   `windows/main.cpp`), so each new effect only needs a row in those two tables. iPhones can't buzz from a web page;
   they get the screen and the sound.

Rules that keep it fair:
- **The eye sets the timing, never the ear.** The strike window and the warning before a thrash are long enough to
  see. Sound and buzz fire at the same moment as the picture, never earlier, so a Knomi player (or a muted computer)
  is never behind.
- **A nibble and a bite look different, not just bigger.** In a nibble the float stays up: a small dip, one small
  ring, no "!" and no glow. In a bite it vanishes under, with the "!", the rim glowing and everything else at once.
  A child should tell them apart from across the room.
- **The rim is the Knomi's best signal.** A glow at the round edge catches the corner of your eye. The Knomi already
  darkens its outer ~15 px (`knomi_rim.h`), so the glow runs from about 18 px in to 40 px in.
- **Colour plus shape.** Red danger also pulses and says LET GO!, so it reads for colour-blind players.
- **Knomi only: the backlight.** At the bite the backlight flashes to full and back; while the line is about to snap
  it pulses. It's a physical flash that the computers can't do. (`panel->setBrightness()`, with a short "flash until"
  in `updateBrightness()`.)
- **The screen stays awake.** A cast counts as a touch, so the screen never dims while you wait.

## The cues, moment by moment

| Moment | Screen (all; the Knomi's only cue) | + Sound | + Buzz |
|---|---|---|---|
| Cast | The rod swings, the float arcs to where you tapped, a splash ring | A whoosh, then a plip | A 10 ms tick |
| Waiting | The float bobs 2 px, a soft ring every ~2.5 s, shadows circle the hook | Just the calm music | None |
| **Nibble** (a fake-out, 0 to 3 a cast) | The float dips 6 px for 0.15 s, one small ring, two ticks beside it, a fish's nose at the bait | A soft tik | A tiny 12 ms tick |
| **Bite** | The float pulled right under, a splash, two rings, the "!" bubble, the rim glows, the line snaps straight and yellow, the rod dips, a 0.2 s shake. A white ring closes in on the float: the time left. Knomi: a backlight flash | A plunk and a rising two-note alarm; the music dips | One strong 90 ms buzz |
| Hooked | The line stays taut; the fight begins | A rising zing; the fight music starts | 30 ms |
| Reeling | The line goes white, yellow, orange, red as it tightens; the rod bends more; the LINE arc on the right rim fills; the fish comes toward the pier | Reel clicks, faster as you reel, higher as it tightens | None |
| Thrash warning (0.4 s before) | Its shadow flashes, bubbles burst, the line twitches | A creak | A double pulse |
| Thrash | Splashes over it, motion marks, the line jerks | A warble | A rattle |
| **About to snap** (85% and up) | Line and arc red, the rim glows red and pulses 3 times a second, LET GO!, a small shake. Knomi: the backlight pulses | A warning beep that speeds up | A buzz with each pulse |
| Snap | The line breaks and curls, the shadow shoots off, IT GOT AWAY! on a red board | A snap, then the aww of a wrong answer | One long 200 ms buzz |
| Too soon, too slow | TOO SOON! or TOO SLOW! over the water; the float bobs back up | A bloop going down | A short double |
| Caught | It leaps out with drops and sparkles, its name on the board, +points, its slot fills | A fanfare, then its own cry (`cry.h`) | A happy pattern |
| Rare (Gyarados, Lapras) | Gold rays, a gold board, +10, NEW! if it's not in your Pokedex | The medal fanfare and its cry | A longer happy pattern |
| Night | The same; the float has a little lamp that glows even when pulled under | The same | The same |

## The game

1. **The card**, then READY, GO! (as every game).
2. **Cast.** Tap the water where you want the float. Near the pier is the shallows, the middle is open water, and the
   dark blue on the right is the deep. Shadows show what's around: small, middling, big (the big ones are deep).
   Bubbles breaking on the surface mean a fish is waiting there, and it bites sooner.
3. **Wait** 2 to 8 seconds (half that at bubbles). Up to 3 nibbles come first, 1 to 2.5 s apart.
4. **The bite.** Tap within the strike window: 0.9 s in the shallows, 0.75 s in open water, 0.6 s in the deep.
   - Tap on a nibble or before the bite: TOO SOON, the fish swims off and the cast is over.
   - Miss the window: TOO SLOW, the float bobs back up and the cast is over.
5. **The fight.** Hold to reel: the fish comes toward the pier and the line tightens. Let go: the line eases and the
   fish swims back out a little. Now and then it thrashes (a warning first). Reeling through a thrash sends the line
   into the red, and at 100% it snaps. Leaving it slack doesn't lose it, but it drifts away, and if it reaches the
   far edge it's gone. A first form takes ~3 s of reeling, the deep ones ~6 s, Gyarados ~9 s, with more and longer
   thrashes the bigger it is.
6. **Landed**: it leaps out, and its points go up.
7. After 5 casts, **the result**.

A game takes about 40 to 90 seconds, like the others.

## Controls

| | Knomi | Mac and Windows | Web on a phone | Web on a computer |
|---|---|---|---|---|
| Cast | Tap the water, or tilt to move the aim ring and press the button | Click the water, or aim with the arrow keys and press Space | Tap the water (tilting aims too, if tilt is on) | As on a Mac |
| Strike | Tap anywhere or press the button | Click or Space | Tap | Click or Space |
| Reel | Hold the screen or the button | Hold the mouse button or Space | Hold | Hold the mouse button or Space |
| Leave | The usual swipe up | The same | The same | The same |

Two things to change for this:
- **The Knomi's button.** Today a release after 0.8 to 3 s turns the screen off, and a 10 s hold asks to erase the
  save (`handleKnomiButton()`). While PokeFish is open the button belongs to the game: down reels and up stops, with no
  screen-off. The 10 s erase question stays.
- **Windows controllers** only tilt today (stick and D-pad). Their A button should be the Knomi's button, so a
  controller can strike and reel, and it rumbles with the buzzes.

## Which Pokemon, and where

All from the Beach zone (`zones.h`). Where you cast decides what's likely:

| Water | First forms (1 point) | Evolved (3 points) | Rare (10 points) |
|---|---|---|---|
| Shallows, by the pier | Magikarp (most), Poliwag, Krabby, Shellder, Psyduck, Slowpoke | none | none |
| Open water | Magikarp, Goldeen, Staryu, Horsea, Tentacool, Seel | Poliwhirl, Kingler, Golduck, Slowbro (now and then) | none |
| The deep | Magikarp | Tentacruel, Seaking, Starmie, Seadra, Cloyster, Dewgong | Gyarados (1 cast in 20 out there), Lapras (1 in 35) |

- Not fished: the Squirtle line (starters stay special), Jynx, Vaporeon and Articuno (the Beach's legend).
- A shiny is 1 in 100, as in the wild: it sparkles and counts double. A gold Magikarp!
- **Where:** a pier at the Beach, a scene of its own (seen from the side, cut away at the waterline). Later, maybe: a
  pier on the Beach's coast on the Travel map that opens it too.

## Scoring and rewards (as the other games)

- **Points:** 1 for a first form, 3 for an evolved one, 10 for Gyarados or Lapras; a shiny doubles it.
- **Record:** the best points, saved as `fishHi`, with NEW RECORD! as in the others.
- `pet.fishResult(points)`: the exercise reward Run and Surf give; GREAT JOY at 4 points or more, otherwise the plain
  joy line. No food: everything goes back in the water.
- It counts toward the Repel's 10 minutes of games, as every game does.
- Nothing landed: 0 and the plain joy line.
- **Catch and release by default.** Whether a new one can join the Pokedex is a question for Fedor (below).

## The how-to card

- Touch: "Tap the water to cast. Wait for the float to go under, then tap! Hold to reel in, and let go when the line
  turns red. 5 casts." (mockup 2: it fits in 6 lines)
- Keys: "Click the water or press Space to cast. When the float goes under, click! Hold to reel, and let go when the
  line turns red. 5 casts."
- New strings, in all 8 languages: the game's name, the two cards, TAP THE WATER, HOLD TO REEL, TAP!, LET GO!, TOO
  SOON!, TOO SLOW!, IT GOT AWAY!. `test_tools.py` checks every card fits.

## Music and effects

- **Two new tracks** in `tools/music/songs.py`, all original, appended after `MUS_ZONE_FIELD` so the zones' numbering
  stays:
  - `MUS_FISH`, at the pier: easy and lazy, about 88 bpm in 6/8, a plucked bass, a flute tune, a soft shaker like
    water on the posts.
  - `MUS_FISH_FIGHT`, once it's hooked: about 150 bpm, a driving bass and a short tune that repeats, back to the calm
    one after the catch. The change itself tells a computer player that it's on.
- **Effects** in `sfx_notes.h`, in the game's square-wave style: CAST, NIBBLE, BITE, HOOK, REEL (one click, played
  faster as you reel), WARN (the creak), THRASH, STRAIN, SNAP, LANDED, MISS. The splash and the snap would sound better
  with the noise that `cry.h` already makes, so the effects player may borrow it. The mixer already lowers the music to
  45% under an effect, which makes the bite cut through.
- The landed Pokemon says its own cry (`voiceSay(dex, VOICE_WIN)`).
- Each new effect gets a row in the web's `BUZZ` and Windows' `kBuzz`, in `audio.h`'s order.

## Where the code goes

- `src/fishing.h`: the rules with no hardware, like `surf.h` and `dungeon.h`: the cast's steps, the fish tables, the
  strike windows, the line's tension. Unit-tested.
- `src/fishing_v2.h`: the drawing. The mockups' pieces (pier, float, rod, line, rings, rim glow, the LINE arc, the
  slots) become the game's. The scene is cached in the one scene buffer, as the other games' are.
- `PokeKnomiGochi.ino`: `GAMES_COUNT` 6, the name and card arrays, start, close and render, `introBegin()`, the sixth
  cartridge (3 + 3 on the Game Boy, mockup 1) and its label picture.
- `pet.h`, `pet.cpp`: `fishHi` and `fishResult()`.
- `i18n`: the strings. `audio.h`, `sfx_notes.h`, `shell.html`, `windows/main.cpp`: the effects and buzzes.
  `music.h`, `songs.py`, `music_v2.h`: the two tracks.
- The Knomi: the button taken over while it's open, and the backlight flash.

## Test plan

- **Rules** (`test/test_fishing.cpp`): a tap on a nibble ends the cast as too soon; a tap in the window hooks it;
  after the window it's too slow; reeling through a thrash snaps at 100%; letting go lowers the tension; points by
  species, a shiny doubled; 5 casts and it ends; the tables hold only Beach Pokemon, and never Gyarados in the
  shallows; 10,000 seeded casts land on the planned odds.
- **Bots** on the console, like RUNBOT and DNBOT: FISHBOT plays perfectly; a "kid" bot reacts in 400 to 700 ms and
  holds through half the thrashes. Tune it so the perfect bot lands about 95% and the kid bot about 60%.
- **Host tests**: the effects table, `BUZZ` and `kBuzz` all have one row per effect; the track count.
- **Screens**: `tools/host_shot` scripts for every moment at day, dusk and night, against these mockups.
- **On the Knomi, by eye**: a nibble against a bite at arm's length; the rim glow through the Knomi's dark rim; the
  backlight flash; the frame rate during the fight (the water's drawn every frame; it should stay at 20 fps or more);
  that holding the button reels and never turns the screen off.
- **Muted on a Mac**: a whole game with the sound off has to feel just as fair.
- **Web**: an Android phone buzzes at the bite, the thrashes and the snap; an iPhone doesn't, and plays the same
  otherwise. Windows with a controller: A strikes and reels, and it rumbles.
- **Languages**: `test_tools.py` (the cards fit) and `test_i18n_formats.py`.
- The usual: `test/run_tests.sh`, the Knomi compile and its size.
- **A playtest with your daughter**: is the bite easy to catch, and is the strike window long enough for her?

## Questions for Fedor

1. **The name**: PokeFish (next to PokeRun and PokeDungeon), or Fishing?
2. **The Pokedex**: catch and release only (my pick for the first version), or may a new one be caught? For example,
   at the end a new one gets a Poke Ball throw (the Field's catch), at most once a game.
3. **Length**: 5 casts a game, or a 60-second timer (as many as you can)?
4. **Too soon**: does tapping on a nibble lose the cast (the classic rule, as mocked), or only scare that fish off
   and keep the cast going (gentler)?
5. **The Game Boy**: is 3 + 3 cartridges all right (mockup 1)?
6. **Tilt**: later, the big ones could run left and right while you lean the Knomi against them. Now, later, or not
   at all?
7. **The backlight flash** on the Knomi at the bite: yes?
