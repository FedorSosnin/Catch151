# Pic Snap

The ninth cartridge, alone on the Games menu's third page (teal). A camera on Knomi Island: ten places, a wild
Pokémon in each, and you snap it when it's sharp and in the middle (Fedor, 2026-11-22). The player's rules are in
[HOW_IT_WORKS.md](HOW_IT_WORKS.md#pic-snap).

## Where it is

- **The rules** are `src/picsnap.h` (no drawing), checked by `test/test_picsnap.cpp`: the places and who turns up in
  each, a roll's order, the lens's sweeps, the camera's drift, the score, a tap at each moment, one that gets away,
  what it does while it's there, and 300 whole rolls by a patient bot and by a random tapper.
- **The drawing** is `src/picsnap_v2.h` (`psDraw()`): the ten places, the focus, the Pokémon, the viewfinder, the
  print and the roll at the end, and the cartridge's label (`psPaintLabel()`).
- **In the sketch:** `startSnap()`, `snapEvents()` (the sounds, the reward and record), `snapTouchDown()` (a photo as
  the finger goes down), `snapButton()` (the card's READY, GO!, then the shutter), `renderSnap()`.
- **Saved:** the record as `pshi`, the leaderboard's best as `lbps`. It's sent to the relay as game 8, which keeps it on
  the leaderboard as the other games' (`relay/src/board.js` `GAMES[8]`).

## The places

Two in each zone, every roll all ten in a new order. Each is three planes:

| Place | Far | Near (at the edge) | At its feet | How it shows up |
|---|---|---|---|---|
| Forest: tall grass | Pines in the sun | Huge blades, a pink flower | A tuft | Up out of the grass |
| Forest: pines | A clearing, big pines | Red mushrooms, a bough | A fallen log | Up from behind the log |
| Beach: shore | Sea, palm, wet sand | A starfish, a shell | A mound of sand | Up from behind the mound |
| Beach: waves | Sky, Knomi Island far off | A swell's crest | The water's ring | Up out of the sea, half under |
| Cave: crystals | The dark, stalactites | Big crystals | Little crystals | Up from behind them |
| Cave: ledge | The cave's mouth, the day outside | Stalactites, a boulder | A rock | Up from behind the rock |
| Mansion: graves | The moon, the mansion | A tombstone, a cross | (none) | Out of the dark (fades in) |
| Mansion: window | A lit window, stone wall | Bare branches | The balcony's rail | Up from behind the rail |
| Field: sky | Clouds, the house, the fence | Big fence posts | (none) | Flies in, hovers |
| Field: flowers | A sunset | Big flowers | Grass, little flowers | Up out of the grass |

The art is the scenes' own (`scene_v2.h` painters, `sprites_v2.h` sprites) at close-up sizes. Each place's cast is
six common Pokémon of its zone and two rare ones (`PS_SCENE`); a test checks each lives in that zone.

## The focus

- **The lens** (`psSweep()`): the sharp plane goes from near (0) to far (1). Each sweep crosses the Pokémon's depth and
  ends 0.16 to 0.5 past it. About 1 sweep in 3 stops on the Pokémon and rests 0.45 s. Sweeps take about 1.15 s at the
  first photo and 0.75 s at the last.
- **The look** (Fedor picked it from four mockups: smooth, 4 px cells, blocks, dithered): out of focus is a blur, then
  each 4 px cell takes the nearer of the place's own two colours by the checker the skies use (`psDitherPick()`).
- **How it's drawn in time:** the far and near planes are painted once a place at the Knomi's size (233 px, plus 32 a
  side for the drift), then blurred and dithered at 7 lens steps (the far plane 6 px soft at the near end, the near
  plane at the far end). A frame copies the step nearest the lens, doubled. The Pokémon is blurred and dithered each
  frame in a picture just big enough for it. The making ready runs a few steps a frame while the last print is up,
  and under the how-to card for the first place (it used to be made all at once as the game opened, which stopped the
  screen for about 4.5 s before the card came up).
- **The blur** (`src/picsnap_blur.h`) is a box twice across and twice down, streamed: each row is read out of PSRAM
  once, the four passes run on a few rows kept in internal RAM, and each row is written once. The first blur worked on
  whole 88 KB planes in PSRAM a byte at a time, each pixel through it about ten times: 80 ms a blur, 3.8 s of the
  4.4 s a place took on the XL (2026-11-23). The dither keeps each colour's pick (the two nearest of the palette and
  which the checker gives each cell), so the palette is searched for about 1 cell in 14. Both give exactly the
  pictures the first ones did: `test/test_picsnap_blur.cpp` holds them to the first ones, bit for bit.
- **Memory:** about 2.1 MB of PSRAM while it's open (1.2 MB for the 7 steps of 297 × 297, then the planes, the
  blurred cover, the photo, the roll and the Pokémon's picture), about 17 KB of internal RAM for the blur's rows and the
  kept picks (in PSRAM instead when less than 40 KB of internal RAM would be left), and the sprite. All of it is freed
  when it closes. With less room it makes fewer lens steps, down to one.

## The score

- Focus: 50 when the plane is within 0.04 of the Pokémon (no blur at all), none at 0.22.
- Centre: 50 within 4 px of the middle, none at 32 px. The camera drifts 32 px across at first (22 up and down), 44 by
  the last photo, and the Pokémon walks up to 28 px from its spot, so centring is timing too.
- Tuned with the bots (300 rolls each): a patient player about 770 a roll (570 to 870), tapping at any old moment
  about 380. At first (90 px and 0.30) a random tapper scored three quarters of the best.
- Rewards as the other games: a point for every 40.

## While it's there

Fedor: "The pokemon should be animated. So the scene looks alive." It breathes (its Idle), and now and then walks a few
steps (its Walk), hops, or nods, poses or takes a big breath, from the sprite pack. Swimmers bob and drift, flyers
weave, ghosts drift. Each place has small things moving too: butterflies, motes in the sun, foam, glints, drips, wisps,
moths, a far bird, petals (`psLife()`).

## Testing

- `PICSNAP` opens it under its card. On a test copy (`NOSAVE`): `PSBOT` snaps by itself; `PSPLACE n [dex]` puts place
  n (and that Pokémon) in this photo's place; `PSHOLD f wx wy` holds the lens at f and the drift at (wx, wy) (`PSHOLD
  -1` lets them go); `PSSNAP` takes a photo now.
- `test/test_picsnap.cpp` (rules), `host/test_host.cpp` (from the Games menu's third page to a roll's end and Play
  Again).

## Not done yet

- A song of its own (it plays Travel's).
- A check on the Knomi and the Waveshare: the time a frame takes and the PSRAM left.
