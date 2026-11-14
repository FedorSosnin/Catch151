# Reel Fishing: the plan (DRAFT, round 2, mockups only, nothing built)

A sixth game on the Games Game Boy. Your Pokemon fishes off the end of a pier at the Beach. You get 5 casts. Pick a
fish, cast by it, tap when the float goes under, then reel it in by tapping at its pace. Each one is weighed and let
go. The score is the total weight.

The mockups are drawn in the game itself (`src/fishing_v2.h`, console `FISHMOCK 1..16`, the Berry Garden mockups' way).
The pictures are in the session notes (`fishing/`): `fishing-1-cast-to-bite.png`, `fishing-2-reel-to-weigh-in.png`,
`fishing-3-got-away-to-end.png` and the cue sheet `fishing-4-cues.png`. Round 1's are in `fishing/round1/`.

## What changed in round 2 (Fedor)

- **The name is Reel Fishing** (no more "Poke" in names; on its cartridge, REEL over FISHING). The other games
  lost theirs too: Rock Hopper, Dungeon Crawler, Volley Ball.
- **Blobs, not fish shapes**, under the water.
- **Tap to reel**, not hold. Too fast snaps the line; too slow and it gets away.
- **Each weight has its own pace**: light ones want quick taps, heavy ones slow and steady.
- **It mustn't look like catching.** So it's a weigh-in: every fish goes on the scale and back in the water, and the
  score is total weight against goal lines, like a fishing contest.
- **More skill, less luck** (below).

## It isn't catching

- No Poke Balls anywhere, no Pokedex, never "caught" or "NEW!". The casts along the top are numbered 1 to 5.
- Every fish is weighed on a scale at the pier (mockup 11), its weight goes on your total, and it hops back in: BACK
  IN IT GOES!
- The score is kilograms, as in a fishing contest. The how-to card says "Each one is weighed, then let go."
- The Pokemon's names still show (STARYU!); they're just never kept.

## Skill over luck

1. **You pick the fish.** Every blob in the water is one fish with a set weight, and its size shows how heavy it is.
   The blob nearest the float takes the bait. What bites is your choice, not a dice roll.
2. **Aim counts.** Land the float right by a blob and it bites soon. Land it between blobs and you wait, and a small
   one may get there first. Bubbles over a blob mean it's about to bite.
3. **Heavier is harder at every step.** The heavy ones sit out in the deep (a longer cast), give a shorter strike
   window, want a slower and steadier pace, and have a narrower green band.
4. **The fight is a rhythm you can learn.** Each weight has its own pace (table below), and the hooked blob swells on
   the beat to show it. Thrashes are always warned (the blob flashes, bubbles burst) and come in the same pattern for
   the same kind, so practice pays.
5. **A clean strike pays.** Tap in the first third of the window and the fight starts with the notch in the middle of
   the green; a late tap starts it near an edge.
6. **Everyone gets the same water.** At any time there are 3 small, 2 middling and 1 big blob in sight, and when one
   is landed another of its size swims in. One giant (Gyarados or Lapras) turns up in every game, out in the deep,
   during cast 3, 4 or 5, and stays until it's hooked or the game ends. Landing it is the hardest thing in the game.
7. **What's left to luck doesn't score.** How long the bite takes and how many nibbles come first stay random: that's
   the suspense, and it never changes the weight.
8. **Later, maybe: the day's water.** One seed for everyone each day, so buddies fish the same water and can compare.

## The cues: the Knomi can't make a sound or buzz

Every moment is built in three layers:

1. **The screen (every device).** Each moment has its own look, and that alone is enough to play well. This is all
   the Knomi gets.
2. **+ Sound (Mac, Windows, web).** A sound for each moment. Once a fish is hooked, the fight music plays at its pace,
   so you can tap along.
3. **+ Buzz (the web on Android phones, Windows with a controller).** It comes through the hook that already buzzes
   for every sound effect (`Module.haptic` and its `BUZZ` table in `webgame/shell.html`, `kBuzz` in
   `windows/main.cpp`), so each new effect only needs a row in those two tables. iPhones can't buzz from a web page.

Rules that keep it fair:
- **The eye sets the timing, never the ear.** Sound and buzz fire with the picture, never earlier.
- **A nibble and a bite look different, not just bigger.** In a nibble the float stays up. In a bite it vanishes,
  with the "!", the rim glowing and everything else at once.
- **The rim is the Knomi's best signal.** It catches the corner of your eye. The Knomi darkens its outer ~15 px
  (`knomi_rim.h`), so the glow runs from about 18 px in to 40 px in. Red means too fast, blue too slow.
- **Colour plus shape plus words.** Every warning also moves (the notch, the line's sag or snap) and says what to do
  (SLOWER!, FASTER!), so it reads for colour-blind players.
- **Knomi only: the backlight** flashes to full at the bite and pulses when the line's about to snap
  (`panel->setBrightness()`, with a short "flash until" in `updateBrightness()`).
- **The screen stays awake** through a cast.

| Moment | Screen (all; the Knomi's only cue) | + Sound | + Buzz |
|---|---|---|---|
| Cast | The rod swings, the float arcs to where you tapped, a splash ring | A whoosh, then a plip | A 10 ms tick |
| Waiting | The float bobs, a soft ring every ~2.5 s, the blobs drift | Just the calm music | None |
| **Nibble** (a fake-out, 0 to 3 a cast) | The float dips a little for 0.15 s, one small ring, two ticks beside it | A soft tik | A tiny tick |
| **Bite** | The float pulled right under, a splash, two rings, the "!", the rim glows, the line snaps straight, the rod dips, a short shake. A white ring closes in on the float: the time left. Knomi: a backlight flash | A plunk and a rising two-note alarm; the music dips | One strong buzz |
| Hooked | The line goes taut; the LINE arc appears with its green band | A rising zing; the fight music starts at the fish's pace | 30 ms |
| **Reeling** | Each tap turns the reel (white marks round it), brings the blob a step closer and pushes the notch up the LINE arc; it sinks back between taps. The blob swells on the beat | A reel click on every tap, higher as the line tightens | None |
| Thrash warning (0.4 s before) | The blob flashes, bubbles burst, the line twitches | A creak | A double pulse |
| Thrash | Splashes over it, motion marks, the notch kicks up | A warble | A rattle |
| **Too fast** (notch in the red) | The line red and straight, the rod bent double, the rim glows red and pulses, SLOWER!, a small shake. Knomi: the backlight pulses | A warning beep that speeds up | A buzz with each pulse |
| **Too slow** (notch in the blue) | The line sags in a pale curve, the rod straightens, the rim glows a cold blue, the blob drifts off, FASTER! | The reel whirring as line runs out, falling | A slow double tap |
| Landed | It leaps out with drops and sparkles; its name on the board | A fanfare | A happy pattern |
| **Weigh-in** | On the scale: the needle swings, the weight goes on the total, BACK IN IT GOES! and a splash | Tick-tick-ding, then its own cry (`cry.h`) as it swims off | A short tick |
| Snap or slipped off | The line breaks (or goes limp), the blob shoots off, IT GOT AWAY! on a red board | A snap, then the aww of a wrong answer | One long buzz |
| Too soon, too slow to strike | TOO SOON! or TOO SLOW! over the water; the float bobs back up | A bloop going down | A short double |
| The giant | Gold rays, a gold board, THE BIG ONE! | The medal fanfare | A longer happy pattern |
| Night | The same; the float has a lamp that glows even under the water | The same | The same |

## The game

1. **The card**, then READY, GO! (as every game).
2. **Pick and cast.** Tap the water by the blob you want. Near the pier is shallow (small ones), the middle is open
   water, the dark blue on the right is the deep (the heavy ones).
3. **Wait** 2 to 8 seconds (less if you landed it close, or there are bubbles). Up to 3 nibbles come first.
4. **The bite.** Tap within the strike window: 0.9 s for small ones, 0.75 s middling, 0.6 s for the heavy ones.
   - Tap on a nibble or before the bite: TOO SOON, the fish swims off and the cast is over.
   - Miss the window: TOO SLOW, the float bobs back up and the cast is over.
5. **Reel.** Tap, tap, tap, at the fish's pace. Each tap pulls the line tighter (the notch goes up) and brings the
   fish a step closer; between taps the line eases (the notch sinks).
   - Notch up into the red: the line snaps.
   - Notch down in the blue for more than 1.5 s: it slips off and gets away.
   - Thrashes kick the notch up, so ease off through them.
6. **Landed**, then **weighed**, then back in the water.
7. After 5 casts, **the result**: the total, what each weighed, the goals and the record.

A game takes about 40 to 90 seconds, like the others.

### The pace for each weight (Fedor: "different weight fish require different speed tapping")

Each tap adds a pull, bigger for a heavier fish, and the line eases at a rate, slower for a heavier fish. The steady
pace that holds the notch in the middle is the ease rate over the pull, so it falls as the weight goes up. First
numbers, for the bots to tune:

| Weight | e.g. | Pace | Green band | Good taps to land it |
|---|---|---|---|---|
| Small, under 15 kg | Shellder, Krabby, Horsea, Magikarp, Poliwag | about 4 taps a second | wide (20 to 80%) | 10 |
| Middling, 15 to 50 kg | Goldeen, Psyduck, Poliwhirl, Seadra, Staryu, Slowpoke, Seaking, Tentacool | about 3 | 25 to 78% | 14 |
| Big, 50 to 100 kg | Tentacruel, Kingler, Golduck, Slowbro, Starmie, Seel | about 2 | 30 to 75% | 18 |
| Heavy, 100 to 150 kg | Dewgong, Cloyster | about 1.5 | 36 to 72% | 22 |
| Giant, 200 kg and up | Lapras, Gyarados | about 1 | 42 to 70% | 26 |

Four taps a second is about as fast as a child taps comfortably, so nothing asks for more. The Knomi's touch screen
has to register that rate (a test below).

## Controls

| | Knomi | Mac and Windows | Web on a phone | Web on a computer |
|---|---|---|---|---|
| Cast | Tap the water, or tilt to move the aim ring and press the button | Click the water, or aim with the arrow keys and press Space | Tap the water (tilting aims too, if tilt is on) | As on a Mac |
| Strike | Tap anywhere or press the button | Click or Space | Tap | Click or Space |
| Reel | Tap anywhere, or the button, at the pace | Click or Space, at the pace | Tap | Click or Space |
| Leave | The usual swipe up | The same | The same | The same |

Three things to change for this:
- **The Knomi's button.** Today a tap starts a wild battle from home and does nothing in a game, and a release after
  0.8 to 3 s turns the screen off (`handleKnomiButton()`). While Reel Fishing is open, each press is a tap for the
  game, and a long press does nothing (no screen-off). The 10 s erase question stays.
- **Space and key repeat.** Holding Space must not machine-gun the reel: only a new press counts.
- **Windows controllers** only tilt today. Their A button should be the Knomi's button, so a controller can play, and
  it rumbles with the buzzes.

## The fish and their weights

All from the Beach zone (`zones.h`). Weights are the Pokedex's (check them when it's built). Each fish weighs its
kind's weight times 0.8 to 1.2, and its blob is that much bigger or smaller, so a big Magikarp blob is a heavier
Magikarp.

| Water | Fish (about kg) |
|---|---|
| Shallows, by the pier | Shellder 4, Krabby 6.5, Horsea 8, Magikarp 10, Poliwag 12.4, Psyduck 19.6, Slowpoke 36 |
| Open water | Magikarp 10, Goldeen 15, Poliwhirl 20, Seadra 25, Staryu 34.5, Seaking 39, Tentacool 45.5, Tentacruel 55, Kingler 60 |
| The deep | Golduck 76.6, Slowbro 78.5, Starmie 80, Seel 90, Dewgong 120, Cloyster 132.5; the giant: Lapras 220 or Gyarados 235 |

- Not in the water: the Squirtle line (starters stay special), Jynx, Vaporeon and Articuno (the Beach's legend).
- A shiny one now and then (1 in 100, as in the wild) sparkles and weighs the same; it's just for fun.
- **Where:** a pier at the Beach, a scene of its own, seen from the side, cut away at the waterline. Later, maybe:
  a pier on the Beach's coast on the Travel map that opens it too.

## Score, goals and rewards

- **The score is the total weight** of what you landed in 5 casts, to a tenth of a kg.
- **Goal lines** (Fedor: "get over a set amount of weight"): bronze 50 kg, silver 150 kg, gold 300 kg, shown as a bar
  with three medals on the result (mockup 14). As a rough guide: five small ones make bronze, a few middling and big
  ones make silver, and gold takes the giant or several heavy ones.
- **Records:** the heaviest total (`fishHi`, in tenths of a kg) with NEW RECORD!, and maybe the heaviest single fish.
- **Reward:** `pet.fishResult(kg10)`, the exercise reward Rock Hopper and PikaSurf give. Bronze or less: the plain
  joy line; silver or gold: GREAT JOY. No food.
- It counts toward the Repel's 10 minutes of games, as every game does.

## The how-to card

- Touch: "Cast by a big shadow: big ones weigh more! When the float goes under, tap. Keep tapping to reel it in, not
  too fast, not too slow. Each one is weighed, then let go." (mockup 2: 8 lines, the card's limit in English; a
  language that runs longer drops the last sentence)
- Keys: "Click by a big shadow: big ones weigh more! When the float goes under, click or press Space. Keep tapping to
  reel it in, not too fast, not too slow."
- New strings, in all 8 languages: the name, the two cards, TAP THE WATER, TAP!, QUICK TAPS!, SLOW AND STEADY, SLOWER!,
  FASTER!, TOO SOON!, TOO SLOW!, IT GOT AWAY!, WEIGH-IN, BACK IN IT GOES!, THE BIG ONE!, kg. `test_tools.py` checks
  the cards fit.

## Music and effects

- **Two new tracks** in `tools/music/songs.py`, all original, appended after `MUS_ZONE_FIELD` so the zones' numbering
  stays:
  - `MUS_FISH`, at the pier: easy and lazy, about 88 bpm in 6/8, a plucked bass, a flute tune, a soft shaker like
    water on the posts.
  - `MUS_FISH_FIGHT`, once it's hooked: a short driving loop whose tempo follows the fish's pace (about 60 bpm a tap a
    second, so 240 bpm for a small one, 60 for a giant, its beat on every tap). The music engine needs a tempo override
    for this. Back to the calm one after the weigh-in.
- **Effects** in `sfx_notes.h`, in the game's square-wave style: CAST, NIBBLE, BITE, HOOK, REEL (the click), WARN (the
  creak), THRASH, STRAIN, SLACK (the whirr), SNAP, LANDED, SCALE, MISS. The splash and the snap would sound better with
  the noise `cry.h` already makes. The mixer already lowers the music to 45% under an effect, so the bite cuts through.
- The fish says its own cry as it swims off (`voiceSay(dex, VOICE_WIN)`).
- Each new effect gets a row in the web's `BUZZ` and Windows' `kBuzz`, in `audio.h`'s order.

## Where the code goes

- `src/fishing.h`: the rules with no hardware, like `surf.h` and `dungeon.h`: the water's blobs, the weights table
  (`FISH_KG10`), the strike windows, the pace model, the goals. Unit-tested.
- `src/fishing_v2.h`: the drawing. The mockups' pieces (pier, float, rod, line, rings, rim glow, blobs, the LINE arc,
  the slots, the scale) become the game's. The scene is cached in the one scene buffer, as the other games' are.
- `PokeKnomiGochi.ino`: `GAMES_COUNT` 6, the name and card arrays, start, close and render, `introBegin()`, the sixth
  cartridge (3 + 3 on the Game Boy, mockup 1) and its label picture; the button while Reel Fishing is open.
- `pet.h`, `pet.cpp`: `fishHi` and `fishResult()`.
- `i18n`: the strings. `audio.h`, `sfx_notes.h`, `shell.html`, `windows/main.cpp`: the effects and buzzes.
  `music.h`, `songs.py`, `music_v2.h` and the synth's tempo override: the tracks.

## Test plan

- **Rules** (`test/test_fishing.cpp`):
  - a tap on a nibble ends the cast as too soon; a tap in the window hooks it; after it, too slow;
  - a clean strike starts the notch in the middle of the green, a late one near an edge;
  - for each weight, tapping at its pace holds the notch in the green; tapping much faster snaps it; much slower
    lets it slip after 1.5 s;
  - a thrash during fast tapping snaps it, and easing off through it saves it;
  - the blob nearest the float bites; weights stay within 0.8 to 1.2 of the kind's;
  - the water always holds 3 small, 2 middling and 1 big, and one giant shows up in casts 3 to 5 of every game;
  - 5 casts and it ends; the goals and the record; only Beach Pokemon in the water.
- **Bots** on the console, like RUNBOT (Rock Hopper) and DNBOT (Dungeon Crawler): FISHBOT taps at the right pace;
  a "kid" bot reacts in 400 to 700 ms and taps with a wobbly rhythm. Tune it so the perfect bot lands about 95% and
  makes gold most games, and the kid bot lands about 60% and makes silver about half the time.
- **Host tests**: the effects table, `BUZZ` and `kBuzz` have a row per effect; the track count; holding Space reels
  once.
- **Screens**: `tools/host_shot` scripts for every moment at day, dusk and night, against these mockups.
- **On the Knomi, by eye and finger**:
  - a nibble against a bite at arm's length;
  - the rim glow (red and blue) through the Knomi's dark rim; the backlight flash;
  - that the touch screen and the button both register 4 taps a second, and a long press never turns the screen off;
  - the frame rate while reeling (the water is drawn every frame; it should stay at 20 fps or more).
- **Muted on a Mac**: a whole game with the sound off has to feel just as fair.
- **Web**: an Android phone buzzes at the bite, the thrashes and the snap; an iPhone doesn't and plays the same
  otherwise. Windows with a controller: A strikes and reels, and it rumbles.
- **Languages**: `test_tools.py` (the cards fit) and `test_i18n_formats.py`.
- The usual: `test/run_tests.sh`, the Knomi compile and its size.
- **A playtest with your daughter**: is the bite easy to catch, are the paces comfortable, and does she ever think
  she's catching them?

## Questions for Fedor

1. **Scoring:** total weight against bronze, silver and gold lines (my pick, as mocked), or points by rarity?
2. **The goal lines:** are 50, 150 and 300 kg about right? Should they become medals in the trophy room?
3. **The giant:** one in every game (fair, as mocked), or rare and random (more of a surprise, more luck)?
4. **Showing the pace:** the blob swells on the beat (easier), or you only feel it from the notch (harder)?
5. **Too soon:** does tapping on a nibble lose the cast (the classic rule), or only scare that fish off?
6. **The Game Boy:** is 3 + 3 cartridges all right (mockup 1)?
7. **The Knomi's backlight flash** at the bite: yes?
