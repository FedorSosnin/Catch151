# Waveshare ESP32-S3-Touch-LCD-2.1

A port of Catch 151 to Waveshare's 2.1" round 480 × 480 board, for Fedor's own handheld: DS Lite speakers, an
1100 mAh cell and a case of his own. Ten boards are ordered: the **flat** touch panel version (SKU 28169), not the
2.5D curved one (the "2.1B", SKU 30697). They haven't arrived yet.

**Status (2026-11-15):** a compile-only target, `BOARD_WS21`. It builds with `tools/build_ws21.sh`. Nothing has
run on the board yet. Every guess in the code is marked `TODO(bring-up)`.

**Read this first:** the board charges its battery at about **2 A**. That is too much for most 1100 mAh cells. See
[Battery](#battery) before you plug one in.

## Sources

Everything here comes from Waveshare's own pages. Pins were read from their demo code and checked against the
schematic.

- Wiki: <https://www.waveshare.com/wiki/ESP32-S3-Touch-LCD-2.1>, and the newer docs site
  <https://docs.waveshare.com/ESP32-S3-Touch-LCD-2.1> (its FAQ lists the I2C addresses)
- Product page: <https://www.waveshare.com/esp32-s3-touch-lcd-2.1.htm> (the flat and 2.5D versions, side by side)
- Schematic: <https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-2.1/ESP32-S3-Touch-LCD-2.1_schematic_diagram.pdf>
- Demo code: <https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-2.1/ESP32-S3-Touch-LCD-2.1-Demo.zip>. Pins from
  `Arduino/examples/LVGL_Arduino/*.h` and `ESP-IDF/ESP32-S3-Touch-LCD-2.1-Test/main/*/`. Waveshare has no GitHub
  repo for this board.
- Flat version drawing and STEP model: <https://files.waveshare.com/wiki/ESP32-S3-Touch-LCD-2.1/ESP32-S3-Touch-LCD-2.1_structure.zip>.
  Import the STEP into CAD for the case.
- Charger: ETA6098 datasheet, <https://www.eta-semi.com/wp-content/uploads/2022/03/ETA6098_V1.1.pdf>
- Amplifier: MAX98357A datasheet (Adafruit's copy), <https://cdn-shop.adafruit.com/product-files/3006/MAX98357A-MAX98357B.pdf>,
  and Adafruit's guide, <https://learn.adafruit.com/adafruit-max98357-i2s-class-d-mono-amp/pinouts>

The flat and 2.5D versions share one schematic, one demo and one board. Only the cover glass differs.

## The board

| Part | What it is |
|---|---|
| MCU | ESP32-S3R8 chip (not a module): 2 cores at 240 MHz, Wi-Fi and BLE 5, **8 MB octal PSRAM** inside |
| Flash | 16 MB (W25Q128J) |
| Screen | 2.1" round IPS, 480 × 480, ST7701S, 185 cd/m². 16-bit RGB565 parallel (the panel is RGB666; its lowest red and blue bits aren't wired) |
| Touch | CST820, I2C 0x15. One finger |
| IMU | QMI8658, 6 axes, I2C 0x6B |
| Clock | PCF85063, I2C 0x51, with a socket for a rechargeable backup cell |
| IO expander | TCA9554PWR, I2C 0x20. All eight lines used on the board; none brought out |
| microSD | SDMMC 1-bit |
| Charger | ETA6098, a switching Li-ion charger: 4.2 V, **about 2 A** as fitted |
| Battery level | Voltage divider (200k over 100k, so 1/3) into an ADC pin. No fuel gauge, no PMU |
| 3.3 V | ME6217C33M5G LDO, 800 mA |
| USB | **Two USB-C ports.** "UART": a CH343P USB-serial chip to UART0, with auto-reset for flashing. "USB": the S3's own USB (GPIO 19, 20). Either one charges |
| Buzzer | A small buzzer, switched on and off by the expander. One fixed pitch |
| Buttons | BOOT (GPIO 0), RESET, and an ON/OFF slide switch that cuts the battery |
| Audio | **None** besides the buzzer: no amplifier, no speaker header, no I2S pins |
| Antenna | A chip antenna on the board. An IPEX socket for an outside one (moving a resistor) |

## Pins

| GPIO | Use |
|---|---|
| 0 | BOOT button (also on the 12-pin connector). The game's button, as on the Knomi |
| 1 | Panel setup SPI data, and the microSD's CMD |
| 2 | Panel setup SPI clock, and the microSD's CLK |
| 3, 8, 17, 18, 46 | Red, high five bits |
| 4 | Battery voltage (ADC1 channel 3), through the 1/3 divider |
| 5, 45, 48, 47, 21 | Blue, high five bits |
| 6 | Backlight: PWM, high = on (a MOSFET switches the LEDs) |
| 7, 15 | I2C SCL, SDA (expander, touch, IMU, clock; also on two connectors). Can't be used for anything else |
| 9..14 | Green G5..G0 |
| 16 | Touch interrupt |
| 19, 20 | The "USB" port's D− and D+, also on the 12-pin connector. Spare if that port isn't used |
| 26..37 | Flash and octal PSRAM. Never touch |
| 38, 39, 40, 41 | HSYNC, VSYNC, DE, PCLK |
| 42 | microSD D0 |
| 43, 44 | UART0 TX, RX. To the CH343 while the "UART" port has power; to the UART connectors when it doesn't (an FSUSB42 switch picks) |

| Expander line | Use |
|---|---|
| EXIO1 | Panel reset |
| EXIO2 | Touch reset |
| EXIO3 | Panel setup SPI CS |
| EXIO4 | microSD D3/CS |
| EXIO5, EXIO6 | IMU interrupts 2 and 1 |
| EXIO7 | Clock interrupt |
| EXIO8 | Buzzer |

The expander's own interrupt isn't wired to the ESP32. Waveshare's demo sets all eight lines as outputs; so does ours.

Panel timings, from the demo: pixel clock 18 MHz; HSYNC pulse 8, back porch 10, front porch 50; VSYNC pulse 3,
back porch 8, front porch 8. That is about 66 frames a second. The panel's setup is 9-bit 3-wire SPI.

**No GPIO is completely free.** The spares:

- GPIO 19 and 20, if the "USB" port isn't used (flash and talk through the "UART" port instead).
- GPIO 1, 2 and 42, and expander line EXIO4, if no microSD card is used. The game doesn't need one: the sprites are
  in flash. GPIO 1 and 2 are only used once, to set up the panel at boot. These are only reachable at the card slot.
- GPIO 43 and 44 only while the "UART" port is unplugged. Not useful.
- GPIO 0 is the BOOT button. Leave it.

### Connectors

| Connector | Pins |
|---|---|
| 12-pin, SH1.0 (a cable comes with the board) | 1 GND, 2 USB 5 V, 3 GPIO 19, 4 GPIO 20, 5 GND, 6 3V3, 7 SCL, 8 SDA, 9 TX (43), 10 RX (44), 11 NC (GND on the schematic), 12 GPIO 0 |
| I2C, SH1.0 4-pin (cable included) | SCL, SDA, 3V3, GND |
| UART, SH1.0 4-pin (cable included) | RX, TX, 3V3, GND. Only with the "UART" port unplugged |
| Battery, MX1.25 (PH1.25) 2-pin | 1 BAT+, 2 GND |
| Clock backup cell, SH1.0 2-pin | 1 +, 2 GND. Fed from 3V3 through a diode, with no resistor |

## The screen: 466 on 480

The game draws on a 466 × 466 canvas. Every position in the code is in those numbers. The V2 UI also keeps to a
2 px grid (`SNAP2`): on the Knomi each panel pixel averages a 2 × 2 block, so anything drawn on even pixels stays
sharp.

There are three ways to show 466 on 480:

| Way | What you see | Cost |
|---|---|---|
| **Centred, 1:1** (what the code does) | Every canvas pixel is one panel pixel. A 7 px black ring all round, about 0.8 mm. | A straight copy per row. |
| Scaled up 3% | Fills the glass. 14 of every 466 rows and columns are doubled (or blurred). Pixel art and the 2 px grid turn uneven. | A lookup per pixel, every frame. |
| A 480 canvas | Sharp and full. | Every layout number in the game changes. Not worth it. |

The ring is hard to see:

- The Knomi's soft rim (`knomi_rim.h`) already darkens the canvas toward its edge. Here it is made for the canvas's
  own circle, 1:1, so the dark edge runs straight into the black ring.
- The flat panel's visible area is 53.88 mm across. At 480 px that is 0.112 mm a pixel (about 226 ppi), so the ring
  is 0.79 mm wide. A bezel that covers 1 mm of the glass edge hides it completely.

The Knomi shows the same canvas at 240 px. This board shows it at full size: twice the detail, and nothing in the
game changes.

## How the target is built

`BOARD_WS21` sits on top of `BOARD_KNOMI_V2`, the same way the computers' build does (`BOARD_HOST`). To the game it
is a Knomi with other hardware underneath. So it gets the Knomi's Games menu, battle zones, BOOT button, Wi-Fi
setup, time zones and sprites in flash, with no new game code.

```bash
bash tools/build_ws21.sh     # compile only: build-ws21/Catch151.ino.bin
```

| File | What changes for `BOARD_WS21` |
|---|---|
| `src/ws21_hal.h` | New. The panel and its setup, the canvas, the touch and the accelerometer, with the Knomi HAL's class names. |
| `src/ws21_exio.h` | New. The TCA9554 expander: one copy of its eight output lines. |
| `src/pin_config.h` | A WS21 block, before the Knomi's. |
| `Catch151.ino` | Includes `ws21_hal.h`, builds the panel objects, and the touch address. |
| `src/rtcbat.cpp` | The PCF85063 keeps the time across restarts. The battery is read from the ADC. |
| `src/audio.cpp` | Sound through an added PAM8302A (`WS21_AUDIO_PDM`) or MAX98357A (`WS21_AUDIO_I2S`); else silent, or the buzzer (`WS21_BUZZER`). |
| `src/ota_update.cpp` | Wi-Fi updates are off. The update site carries the Knomi's firmware, which would not drive this screen. |
| `src/autorot.h` | The quarter turn stays 0: the picture turns in `ws21_hal.h` instead (45° plus quarter turns). |
| `src/knomi_rim.h` | Its row tables can be sized for the 466 canvas. The Knomi's build is unchanged. |
| `tools/build_ws21.sh` | Compile only. Output in `build-ws21/` (git ignores it). |
| `tools/ws21_flash.sh` | Builds, packs the sprites and flashes a board. The port must be given. |

What the code uses:

- **Panel:** esp_lcd's RGB panel, driven directly (`Ws21Panel`), with no frame buffer of the driver's. Three
  frames of 466 × 466 in PSRAM: one on show, one waiting for the next refresh, one being drawn, so nothing tears and
  the game never waits for a refresh. Two bounce buffers of 16 lines in internal RAM feed the panel. Each time one
  has gone out, `ws21Fill()` refills it from the frame on show: a plain copy of each row's part inside the circle,
  with the 7 px border and the corners black. The rows come from internal RAM, where the DMA brought them while the
  band before went out (`Ws21Prefetch`); if they aren't there, the CPU reads them from PSRAM.
  8 MHz pixel clock, 29 Hz. Data changes on the **falling** edge of the pixel clock: on the rising edge, as in
  Waveshare's demo, this panel showed streaks along the rows.
- **Panel setup:** Waveshare's own command list, copied from their demo. Arduino_GFX has one for a similar 2.1" panel
  (`TL021WVC02`), but its gamma, VCOM and inversion settings differ. It goes over a 9-bit software SPI on GPIO
  1 and 2, with the CS and the panel's reset on the expander (`Ws21InitBus`).
- **Drawing:** the game draws straight into the frame to be shown (`Ws21Canvas`, "direct"): every screen paints all
  of itself each time it's drawn, so there is nothing to copy. Then the Knomi's soft rim is shaded into the frame's
  edge (`shadeRim()`), and the frame is shown. Upside down (auto-rotate), the refill turns the frame over as it
  copies the rows, so that costs nothing. Sideways, the game draws on a canvas instead, and each frame is made from it
  by the CPU (below). See "Faster frames" for how it got here.
- **Straight:** the picture sits as the panel does (`WS21_ROT_DEG` 0), so the USB-C ports point out at 45°: the case
  handles that. Turned 45°, one port pointed straight down, but a frame took 164 ms against the 100 the game is made
  for, and everything played slower than on the Knomi; Fedor chose speed. Any angle still works (`ROTDEG`): each
  panel pixel then blends the four canvas pixels round its point (`WS21_SMOOTH`); without the blend, pixel art
  comes out dotted and its dithering in zigzags.
- **Auto-rotate:** in quarter turns, as the board is held. It holds still in every game, Pic Snap and BerryMatch
  included (2026-11-23). Each copies whole pixels, with no blending: upside down the same way as straight (120 ms a
  frame), sideways a canvas column per panel row (140 ms). Turned, the game draws on the canvas and each frame is made
  from it (2026-11-20 numbers, before "Faster frames").
- **Dimming:** only while the pet sleeps, as on computers (Fedor). The Knomi still dims when left alone.
- **Touch:** the CST820 read like the Knomi's CST816S (same registers), mapped back into the canvas and turned with
  the picture. A small task on the other core reads it every 8 ms while a finger is down and queues each press and
  lift, so a quick tap isn't lost while a frame is drawn. The chip gets the Knomi's setup: no gestures (a double-tap
  check holds taps back), INT on touch, change and release, no auto-reset, no auto-sleep. Waveshare's demo only turns
  auto-sleep off; with just that, taps went sluggish a few seconds after each start (Fedor).
- **IMU:** SensorLib's `SensorQMI8658`, accelerometer only, ±2 g at 62.5 Hz. Its axes are a quarter turn from the
  panel's (`WS21_ACC_MAP`), found from Fedor's video.
- **Clock:** the Knomi's clock (Wi-Fi time, time zones), plus SensorLib's `SensorPCF85063` to keep UTC across restarts.
  The chip's time counts only once the game has set it (a mark in its RAM byte) and its clock hasn't stopped since.
  A fresh board's chip ran on a leftover factory date (January 2026): taken as the time, the first save got it, and
  the months until network time counted as time away (a Lv.100 Pikachu on its first day). Until it's set, the game
  starts from its last saved time, as the Knomi does.
- **Battery:** `analogReadMilliVolts()` on GPIO 4, × 3 / 0.9945 (Waveshare's measured correction), and the LiPo curve
  in `batcurve.h`.
- **Serial:** UART0 through the CH343, as on the Knomi (`CDCOnBoot=default`). Use the "UART" port.

The sprites go in flash, as on the Knomi: same partition table (`app3M_fat9M_16MB`), same `tools/pack_flash.py`,
written at 0x610000. The microSD slot isn't needed.

Not done yet, on purpose:

- **Light sleep.** The board can't tell whether USB is in, so the code acts as if it always is: no light sleep, and
  no low-battery shutdown. The RGB panel also needs care before light sleep, because it stops scanning out.
- **Wi-Fi updates.** They need their own channel (`web/ws21/`, a manifest of its own) before they can be turned on.

### Faster frames (2026-11-23)

Fedor: "Its a large screen but feels so much slower than the Knomi." Pinball showed it most: 99 ms a frame (10 a
second, for a game paced at 25), and Pinball caps a step at 100 ms, so near that the ball also ran slow.

Where a frame went (Pinball, `PROF` and `FPS`): 36 ms putting back the table under what moves (a 434 KB copy, PSRAM
to PSRAM), 36 ms copying the canvas out to the panel's frame buffer, about 27 ms drawing the rest. Halving the
refresh rate saved only 6%; reading the canvas by DMA, or on both cores, saved 3 to 5 ms. PSRAM is the limit, so
the fix was copying less:

- **No frame buffer of 480 × 480.** The refill builds each panel row from a 466 frame itself (above), so frames
  are the canvas's size, and the game can draw into them.
- **Direct drawing.** The game draws into the frame to be shown: the copy out is gone (`DIRECT 0` puts it back,
  for comparing). Three frames in turn, so the one drawn into is never on show or waiting.
- **The rim once a frame, in the frame.** Shaded in the refill it cost 664 us a band, 70% of the game's core. In
  the frame, from a quarter table in internal RAM, it's about 9 ms. Where it darkens by less than 8/256 (at most one
  step of the panel's colours) it's left out: a ring 29 px wide instead of 45.
- **Pinball's table by DMA** (`restoreFrom()`): 18 to 21 ms instead of 36. Most of that is the DMA driver writing
  back the cache over the source, which it does for every copy from PSRAM.
- **The cache.** PSRAM is only reached through the 32 KB cache, and the DMA and the refill go round it. Writing back
  by address walks every line of the range (17 ms a frame), so the whole cache is written back at once
  (`ws21Writeback()`, 0.4 ms), and old lines are dropped by address, which is quick. The whole-cache write-back has
  no lock of its own: with the refill dropping lines at the same moment, the chip stopped ("Dcache sync parameter
  configuration error"). It now runs with interrupts held off. That's 0.4 ms, well inside a 20-line band (0.9 ms).
  Bands of 30 lines were tried too: no fewer late refills, and internal RAM fell to 34 KB while TLS connected.

Result, on the bench board: Pinball 67 ms a frame (15 a second, the ball at full speed); the home screen 82 ms
(115 before).

- **The refill reading ahead by DMA** (`Ws21Prefetch`). The CPU's copy of a band out of PSRAM took about 350 us
  of every 900, a third of the game's core. Now each refill has the DMA bring the next band into internal RAM, and
  the next refill only copies it across: 77 us a band, about 10% of the core. About 96% of bands come that way;
  the rest (the frame changed in between, or the DMA was late) are read by the CPU as before. Its own channel pair,
  driven directly: `esp_async_memcpy` writes back the cache over the source by address before every copy. It needs
  a memory-to-memory trigger taken as `esp_async_memcpy` takes one (without it the DMA never moved), and no owner
  checks on its descriptors (the transmit ones are never handed back, and a refused mount logged from the refill's
  interrupt until the watchdog stopped the chip). The bounce buffers went to 16 lines to make room for its 15 KB.
- **But PSRAM is the limit, not the CPU.** Freeing the core barely moved the frame times at 44 Hz: the home screen
  79 ms, Pinball 67 (its table's DMA copy then holds up the read-ahead: 38% of bands fell back to the CPU). At
  29 Hz (`PCLK 8`) there's room: Pinball 49 ms (20 a second) against 55 with the CPU's refill. Fedor saw no
  flicker at 29 Hz, so that's the pixel clock now: 8 MHz.
- **Upside down** (auto-rotate at 180°) used to draw on the canvas and have the CPU turn each frame: 21 ms more a
  frame, on every screen. Now the refill reads each band's rows from the bottom and copies them right to left
  (`ws21PanelRows()`, `flip`), and the game draws straight into the frame as upright. The circle and the rim are
  the same either way up.
- **Pic Snap** took 210 ms a frame while a photo's print was up: the photo doubled over the whole screen and dimmed
  pixel by pixel, every frame (91 ms), and the next place made ready a step a frame (about 85 ms a step). The print
  screen, and IT GOT AWAY!, are now drawn once and kept (`psStill`, 434 KB of PSRAM if there's 1 MB more to spare),
  then put back by DMA each frame: about 22 ms. A frame there is about 115 ms, so the next place is ready sooner.
- **DMA channels:** the read-ahead takes one more pair (now the panel, the frame copy, the read-ahead, the sound,
  and one left for TLS). HTTPS with it isn't tested yet. `WS21_REFILL_DMA 0` builds it out.

### What bring-up found (2026-11-20)

- **Streaks along the rows:** the pixel clock's edge. The panel wants data on the falling edge (`LCD_PCLK_NEG`).
- **The picture jumping:** reading sprites with `esp_partition_read()` stops the cache, and with it the panel's
  refill. Sprites are read through a flash map instead (`flashpak.h`).
- **Tearing:** one frame buffer. Now two, swapped between refreshes.
- **Speed:** almost everything here waits on PSRAM. The game draws a frame in about 75 ms. Copying it to the panel
  by CPU took 68 ms (138 ms turned 45° and blended): the CPU reads each frame-buffer line in before writing it.
  Straight, the CPU reads the canvas and the DMA writes the frame buffer, a band at a time through internal RAM:
  38 ms. A whole frame is about 115 ms (205 ms turned 45° at first, 164 by DMA). Reading the canvas by DMA too
  saved 9 ms, and on both cores the turned copy took 65 ms, but both brought the glitch lines back (`DMAIN`,
  `SPLIT`): anything else reading PSRAM hard while the refill runs holds it up. The refill, on the game's core, still takes
  about a quarter of that core.
- **Taps:** the game read the touch once a frame. A tap that came and went within a 200 ms frame was lost. A touch
  task now queues every press and lift.
- **The rule:** only one core may read PSRAM hard at a time, and it must be the core the refill runs on. The refill
  copies each band of the frame out of PSRAM in time for the panel; when the other core reads PSRAM at the same
  moment, it falls behind and lines show along the panel's rows (at 45° to the picture). Both times the refill or
  part of the frame copy went to the other core, the lines came back.
- **The buzzer:** it can beep each effect's rhythm, a beep a note, and click for taps (`WS21_BUZZER`). It's off:
  one high pitch, switched hard on and off, sounds harsh (Fedor). Without an amplifier the game is silent, as on
  the Knomi.
- **Overnight, the picture 20 lines up, with streaks (2026-11-21):** a fault in the screen driver (IDF 5.5.4).
  The driver refills its two bounce buffers in turn and picks which one by counting "buffer sent" signals. A flash
  write (each save) holds up interrupts. If that lasts more than one buffer (0.9 ms), two signals arrive as one and
  the count is off by one. From then on every refill goes into the buffer being sent: the picture is a buffer
  late, and streaks show where a refill loses the race. Nothing reset the count, so it stayed until another save
  happened to put it back. Espressif fixed it later (the count goes to 0 when the DMA restarts at each refresh).
  `Ws21Panel::frameStart()` does the same from the refresh's callback. Tested on the bench board: with the fix
  off, stalls left every refresh wrong (137 of 137); with it on, each fault was gone by the next refresh, after
  stalls and after 400 real flash writes alike. A save can still show streaks for a refresh or two (the screen
  can't be refilled while the flash is written), but they no longer stay.

Console commands for testing (115200 baud on the "UART" port; none of them is saved unless it says so):

| Command | What it does |
|---|---|
| `FPS` | Frames drawn since the last `FPS`, their time, and the flush's parts: the overlay, the CPU's part of a copy (and the cache's write-back), waiting for the DMA, the rim and handing over, and Pinball's table by DMA. |
| `PROF` | A game's frame in parts, ms each, and PSRAM free. Pinball: 0 the physics, 1 the table back, 2 what's lit, 3 the grass, 4 the wild one, 5 the flippers, 6 the ball, 7 the words, 8 the card over it, 9 the flush. Pic Snap: 0 the game's step, 2 the place, 3 its life, 4 the Pokémon (blurred), 5 the viewfinder or the print, 6 the rest, 7 the next place made ready, 8 the card, 9 the flush. |
| `PLAY g` / `QUIT` | After `NOSAVE`: game g (0 Tilt Catch to 8 Pic Snap) under its card, as from the Games menu; out of it again. A swipe sends the card away. |
| `DIRECT 0` / `DIRECT 1` | Drawing on the canvas and copying it out, as before, or straight into the frame (the default). |
| `PERF` | The frame copy's cost, 10 in a row. |
| `ROTDEG n` | Turns the picture n degrees clockwise. |
| `SMOOTH 0` / `SMOOTH 1` / `SMOOTH 2` | Turned: nearest pixel, blended, or blended only across the 2 px grid. |
| `PCLK n` | The pixel clock in MHz. |
| `PCLKEDGE 0` / `PCLKEDGE 1` | Data on the rising or falling edge. |
| `REFILLCORE 0` / `REFILLCORE 1` | The core the panel's refill runs on. Saved; used from the next start. |
| `PDMCHECK` | With `WS21_AUDIO_PDM`: GPIO 19 read back, to see the sound stream runs. |
| `AUDIO` | With an amplifier: the mixing's share of core 0, its longest block, dropouts, the amp and the track. |
| `AUDIOBENCH n` | 2 s of music track n mixed by a player of its own and timed (the game waits meanwhile). |
| `MUSIC n` / `MUSIC` | Track n whatever the screen, on this board even while the pet sleeps; `MUSIC` alone gives it back. |
| `PDMMODE n` | The sound stream's shape: 1 Espressif's DAC mode (the default), 0 the codec mode first built, to compare by ear. |
| `PDMGAIN n` | The mix's level on its way out, in tenths (20, x2, the default). |
| `TOUCHLOG 1` | Each press and lift, with how long the chip and the game took to see it. |
| `SPLIT 0` / `SPLIT 1` | Turned: the frame made on the game's core alone, or on both. |
| `LCD` | Refreshes since the last `LCD`, the longest gap between two, refreshes with a lost refill signal (`eofmiss`), refreshes refilled out of step (`eofodd`), the refill's time a band (`fill`: count, average, longest, in us), and how many bands came read ahead by the DMA, by the CPU, and with the DMA late. |
| `EOFFIX 0` / `EOFFIX 1` | The fix for lost refill signals off or on (on at start). |
| `STALL us n` | Holds up the interrupts n times for us microseconds, as a flash write does. |
| `NVSTEST n` | n real flash writes, to a scratch area (`ws21t`), not the save. |
| `HOLD n` | The game stops for n seconds; only the panel's refill runs. |
| `WIFIOFF` | Wi-Fi off until the next start. |
| `SHOT` | The panel's 480 × 480 as shown, RGB565 (the rows made as the refill makes them). |
| `BAT` | The cell now: mV, the smoothed mV, the percentage, whether it charges (`chg`), the lift taken off while charging, the reset reason (`rst`) and the time since the start. |
| `BATLOG` | The last 2 minutes, twice a second: mV and `chg`. It lasts through a restart that isn't a power cut, marked by a 0. The board also prints `bat: charging` when it changes, and `bat: start` with the reset reason. |

## Sound

The game's effects are square-wave notes from 196 to 3136 Hz (`sfx_notes.h`), Game Boy style. With an amplifier
wired (`WS21_AUDIO_PDM`, on since 2026-11-22), this board also plays the music and the Pokémon's cries (`HAS_MUSIC`),
as the computers do. `audio.cpp` mixes everything at 16 kHz in a task on core 0: effects and cries at the Sound
level, the music at the Music level, ducked while an effect plays (the same code as `host/host_audio.cpp`). The
hold menu has a fifth item, Sound, as on the computers (`sound_v2.h`): a slider for the music, one for the effects and
cries, and Mute. The task owns the amplifier's on/off line. The amp is on only
while something sounds, and 150 ms after: the unsmoothed stream hisses even when it carries silence (Fedor: "when
music is off, there is still static"). An effect waits the amp's 50 ms wake. The DMA holds 4 blocks of 8 ms, so a tap sounds within about 40 ms. The
console's `AUDIO` shows how much of core 0 the mixing takes and whether the stream ever ran dry. Measured on the bench
board (2026-11-22): 3% of core 0 with no music, about 40% with a battle track or Pinball's playing (longest block 9 of 8
ms, covered by the 32 ms in the DMA: no dropouts), the game's frames 123 ms instead of 119, and the panel's refill
clean (`LCD`: no eofmiss, eofodd or late frames). Each of the S3's five DMA channel pairs counts: the sound's output
took the last one TLS needed, so the frame copy now takes the SPLIT and DMAIN ones only when those are switched on.
Wiring, with a diagram: `~/Projects/waveshare-case/wiring/AMP-WIRING.md`.

### The buzzer can't replace a speaker

The buzzer hangs off expander line EXIO8, through a transistor. No ESP32 pin reaches it, so PWM can't drive it.
Waveshare's own code only turns it on and off, and it sounds at one fixed pitch when on. `WS21_BUZZER` beeps each
effect's rhythm on it, one beep a note, and a tap is a click; it's off, as it sounds harsh. It can't play notes,
jingles or music. For real sound you need a speaker and an amplifier.

### DS Lite speakers

Nobody publishes their ratings. Measure them before you buy an amplifier:

- Resistance: a multimeter across each speaker. An 8 Ω speaker reads about 6 to 7 Ω; a 4 Ω one about 3 Ω.
- Size: diameter and depth, for the case.
- Power: tiny. Treat them as 0.25 W or less until you know better.

The DS Lite has two, one each side. For this game, mono is plenty: wire both **in series** on one amplifier. That
halves the current, protects them, and both sides still sound.

### The amplifier: MAX98357A

The MAX98357A is a small I2S amplifier on a breakout board (Adafruit 3006, or the common generic ones):

- It takes digital audio straight from the ESP32 on three lines: bit clock (BCLK), word clock (LRC) and data (DIN).
  No codec and no master clock.
- It runs straight from 2.5 to 5.5 V, so from 3.3 V or a single Li-ion cell.
- Into 8 Ω it gives about 0.8 W at 3.3 V. That is more than tiny speakers can take, but the game's effects play at
  about 15% of full scale, so about 2% of full power.
- Gain is set by one pin: 9 dB if left open (start there), 3 dB with 100k to VDD for quieter.
- Its SD pin turns it on and off: above 1.4 V it plays the left channel, below 0.16 V it shuts down (0.6 µA).
  The game sends the same sound on both channels. On, doing nothing, it draws 2.4 mA.

The code for it is in place: `audio.cpp` plays the effects through it, with no codec to set up. Uncomment
`WS21_AUDIO_I2S` in `pin_config.h` once it's wired.

### Wiring: a PAM8302A on the 12-pin plug (Fedor's choice)

Parts: a PAM8302A board (the purple "2.5W Class D Amp" ones) and one or two 16 mm 1 W 8 Ω speakers. No resistor
or capacitor (Fedor, 2026-11-22: "I would rather not use any to keep this simple"). The board's own 12-pin cable
carries everything, five wires straight onto the amp (the case's `wiring/AMP-WIRING.md` has the steps and a diagram):

| 12-pin plug | Board signal | PAM8302A |
|---|---|---|
| 3 | GPIO 19: the sound as a 1-bit stream | A+ |
| 5 | GND | A−: the sound's own ground, so the amp's supply current on pin 1 doesn't add to it |
| 4 | GPIO 20 | SD: high plays, low shuts it down (the game turns it on only while something sounds) |
| 6 | 3V3 | Vin |
| 1 | GND | Gnd |
| | | Speaker + and −: one speaker, or two in series (16 Ω, a little quieter) |

- Count the pins from pin 1 on the board's silkscreen, not by wire colour.
- Straight in is safe: on Adafruit's board (and its copies) the Vol trimmer is a 10 kΩ divider from A+ to A−, then
  1 µF and 100 Ω into the chip. GPIO 19 drives 10 kΩ, 0.33 mA at most.
- Nothing smooths the stream before the amplifier, so it gets the megahertz edges (Espressif's docs call for a
  low-pass filter on this kind of output). Heard on 2026-11-22:
  - In the codec mode first built (`I2S_PDM_TX_SLOT_DEFAULT_CONFIG`, stereo): "a TON of static".
  - In Espressif's DAC mode (`I2S_PDM_TX_CLK_DAC_DEFAULT_CONFIG` and `I2S_PDM_TX_SLOT_DAC_DEFAULT_CONFIG`, mono: the
    master clock 160 MHz / 2.003 instead of / 3.255, so less jitter, and twice the level), with the mix at x2: "better",
    but static still with no music, as the amp stayed on between sounds.
  - With the amp on only while something sounds: "On the lowest setting, it seems good!"
  - Cleaner still, if it's ever needed: a 1 kΩ in the pin 3 wire with 10 nF from A+ to A− at the amp.
- Start with the board's Vol trimmer turned most of the way down, then turn it up.
- The "USB" port then only charges: GPIO 19 and 20 are its data lines. Flash and use the console through "UART".
- `WS21_AUDIO_PDM` is on in `pin_config.h` (2026-11-22). The hatch jingle plays at start-up, then the music.
  `PDMCHECK` on the console reads GPIO 19 back: a running stream flips all the time.

### Wiring: a MAX98357A from the microSD slot

The game doesn't use the microSD card, and the card slot carries three spare GPIOs and a spare expander line.
A microSD breakout ("sniffer") or a microSD extension ribbon plugged into the slot brings them out, with no
soldering on the board:

| microSD pin | Board signal | MAX98357A |
|---|---|---|
| CLK | GPIO 2 | BCLK |
| CMD | GPIO 1 | LRC |
| DAT0 | GPIO 42 | DIN |
| DAT3 | EXIO4 | SD (on/off: the game turns it off when idle or asleep) |
| VDD | 3V3 | Vin |
| VSS | GND | GND |
| | | Speaker + and −: the two DS Lite speakers in series |

`WS21_AUDIO_I2S` in `pin_config.h` is set up for this. It keeps both USB ports and the 12-pin connector free.

- GPIO 1 and 2 also go to the panel's setup lines. The panel only listens while its CS is low, and that's only at
  boot. Sound starts after the panel is set up.
- The card's lines have 10k pull-ups. They do no harm: each line is driven. DAT3 high means the amplifier plays the
  left channel, which carries the game's sound.

### Other wirings

- **12-pin connector plus one solder joint.** BCLK on GPIO 19 (pin 3), LRC on GPIO 20 (pin 4), and DIN on GPIO 42,
  soldered at the card slot's DAT0. Uses the included SH1.0 cable. You lose the "USB" port (use the "UART" one), and
  the amplifier has no on/off line: it stays on (2.4 mA).

## Battery

- **Connector:** MX1.25 (PH1.25) 2-pin, pin 1 BAT+, pin 2 GND. Check the polarity of every cell's plug against
  the board before plugging in. Cheap cells often come wired the other way round.
- **Use a protected cell** (one with its own protection board). The firmware doesn't shut down on low battery yet.
- **Charging is about 2 A.** The ETA6098's current is set by R7, 82 kΩ, and ETA's datasheet gives 2 A for 82 kΩ
  (and 1.2 A for 150 kΩ). An 1100 mAh LiPo usually allows 1C (1.1 A) at most; 0.5C is gentler. Two ways out:
  - Swap R7 for a larger resistor. The datasheet gives no formula, only those two points and the ISET pin's
    0.8 V. So the chip charges at a multiple of the current through R7, and the current is about 165 to 180
    divided by R7 in kΩ, in amps:

    | R7 | Charge current | For an 1100 mAh cell |
    |---|---|---|
    | 82 kΩ (as shipped) | 2 A | Too much |
    | 180 kΩ | about 0.9 to 1.0 A | At the 1C limit |
    | 220 kΩ | about 0.75 to 0.8 A | Fine, a full charge in 1.5 to 2 hours |
    | **330 kΩ** | **about 0.5 to 0.55 A** | **Recommended: gentle, a full charge in about 2.5 hours** |

    Swap it on one board first and measure the current with a USB meter, then do the rest. The pre-charge
    (200 mA) and the cut-off (130 mA) are fixed in the chip; both suit 1100 mAh.
  - Or pick a cell rated for 2 A charging.
  - Either way, check with a USB meter on the first board before trusting it. Charging at 2 A, the board draws about
    1.6 to 1.9 A from 5 V, more than a computer's USB port gives.
- **The slide switch** (ON/OFF) cuts the cell from the board through a MOSFET. The charger sits before it, so the
  cell still charges with the switch off. There is no soft power button and no way for the firmware to turn the
  power off.
- **The level:** the cell's voltage through the 1/3 divider on GPIO 4, turned into a percentage by the LiPo curve.
  Nothing on the board says whether USB is in or the cell is charging, so the code treats the board as always on
  USB for now (no light sleep, no low-battery shutdown).
- **Charging:** told from the cell's voltage instead. When the charger starts or stops, the voltage jumps. On the
  bench board (a 2000 mAh cell, charged at about 2 A) it rose about 100 mV within half a second of plugging in, and
  fell about 115 mV on unplugging. Its own loads move it 10 mV or less. The firmware reads it twice a second. A
  rise of 40 mV or more means plugged in, a fall as big means unplugged (`BatPlugDetect` in `src/batcurve.h`).
  - After 30 minutes at the top (4.15 V or more), it counts as charged and the bolt goes.
  - A board started while plugged in (after a flash, an update or a crash) sees no jump. It goes by the slow climb
    instead: while charging, the voltage rose about 6 mV a minute (3725, 3729, 3733, 3741 mV, one minute apart).
    Four one-minute averages, each at least 2 mV over the one before and 10 mV in all, mean charging. So the bolt
    comes about 4 minutes after a start. Near the top, where the voltage holds still, it doesn't come.
  - With R7 swapped for a gentler charge (above), the jump is smaller. At about 0.5 A it may be near the 40 mV limit:
    check with `BATLOG` while plugging in.
  - Charging lifts the voltage by that same jump, so the percentage would leap too (8% on the cell, 37% plugged in,
    on the bench board). While charging, the jump is taken off (100 mV after a start found by the climb). At the top
    it fades out over the 30 minutes, so the number ends at 100%. As on the Knomi, the number only goes down on the
    cell and only up while charging.
  - The guess only drives the icon and its number. Sleep and shutdown still treat the board as on USB.
  - A wire would make it certain: USB 5 V (12-pin, pin 2) through a divider into GPIO 20 (19 now carries the
    sound) would tell the firmware when USB is in.
- **On screen:** the battery takes the clock's place at the top of the home screen: an icon and the percentage.
  The icon turns amber under 40% and red under 15%. While charging it turns green with a bolt (see Charging,
  above). With no cell, the clock comes back. The time is still on the
  clock page. To see it on a computer: `POKEMACGOCHI_FAKE_BAT=85` (or `15c` for 15% and charging) on a test copy.
- **The power LED** is always on while powered (about 1 mA). Remove it, or cover it, if battery life matters.
- **The clock's backup cell:** with no cell on J4, the clock forgets the time whenever the slide switch is off. With
  a **rechargeable** coin cell (an ML1220 with SH1.0 leads, for example), it keeps time. Never fit a non-rechargeable
  one: the board charges it through a diode, with no resistor.

## Case design notes

From the flat version's drawing (the dimension drawing and STEP in the structure zip). All in mm.

**Overall:** the cover glass is Ø75.00 ± 0.15, and the board sits behind it. 9.50 from the front of the glass to
the tallest part on the back.

| Measurement | Value |
|---|---|
| Cover glass | Ø75.00 ± 0.15, 0.70 ± 0.15 thick |
| Visible area | Ø53.88 ± 0.15 (the product photo says 53.28) |
| Glass front to LCD back | 3.40 |
| Glass front to PCB back | 5.50 |
| Parts on the back | up to 4.0 tall |
| PCB | 66.00 across its left and right flats; its top flat 33.9 above the screen's centre; 64.25 from the top flat to the bottom edge |
| Mounting | 4 × M2 threaded standoffs on the back, on a Ø65.00 circle |
| Upper holes | ±29.50 from the centre line, 13.64 above the centre (20.36 below the top flat) |
| Lower holes | ±18.00 from the centre line, 27.06 below the centre (61.06 below the top flat) |

The standoffs' height isn't given. Measure it on the first board.

**Where things are**, as drawn from the back (seen from the front, left and right swap):

- Left edge: BOOT at 28.20 below the top flat, RESET at 35.80, the slide switch at 45.73 (ON toward the top).
- Right edge: the microSD slot's opening between 4.19 and 14.21 below the top flat (the upper corner, at an angle),
  the I2C connector at 27.07, the UART connector at 35.98. The buzzer is on this side, above the "UART" port.
- Bottom: the "USB" USB-C at lower left, the "UART" USB-C at lower right, both turned about 45° outward. The panel's
  ribbon folds over the bottom middle.
- Upper left: the 12-pin connector, at an angle. Near BOOT: the battery connector. Right of the centre: the clock
  cell's connector. Top: the chip antenna and the IPEX socket. Lower left, by the switch: the two LEDs.

**Design notes:**

- **Display opening:** at least Ø54.2 (the visible area plus its tolerance). The glass has a black border about
  10.5 wide outside that. Let the bezel overlap the glass edge by 1 or more, with a thin gasket, and hold the board
  by its four M2 standoffs. Don't clamp the glass.
- **Speaker grilles:** DS Lite speakers are small and need a sealed space behind them to sound full. Put them low on
  the front, either side of the screen, or on the bottom edge. Keep them and their magnets away from the top, where
  the antenna is.
- **Battery space:** behind the board, past the 4.0 of parts, with 1 of foam. Keep it off the antenna at the top,
  and away from the charger, which gets warm at 2 A. A typical 1100 mAh pouch is about 50 × 34 × 5; measure yours.
  The total depth is then about 9.5 + 1 + 5 + the back wall, so about 18 to 20.
- **USB-C:** both ports point out at 45°. The "UART" one is the one to reach: it flashes, gives the console and
  charges. The "USB" one can stay covered. Either cut an angled opening, or run a short USB-C extension to a
  straight socket on the bottom.
- **Buttons:**
  - BOOT is the game's button: a tap brings a wild battle, a 1 to 3 second hold turns the screen off or on. Give it
    a real cap or plunger that presses the side button.
  - RESET: recess it behind a pinhole.
  - The slide switch needs a slot, or a lever that reaches it.
- **Amplifier:** the MAX98357A breakout is about 18 × 19. Leave room by the microSD slot for the breakout or ribbon,
  plus its wires.
- **Clock cell:** an ML1220 is 12.5 across and 2 thick.
- **The buzzer:** give it a small hole if it will be used for alerts.

## Bring-up plan

Start with one board on the bench: no case, no battery, on the "UART" USB-C port. Keep the other nine sealed until
it works.

1. **Out of the box.** Plug it in and watch Waveshare's factory demo. That proves the board.
2. **Save its factory firmware** (16 MB):
   `esptool --port PORT read-flash 0 0x1000000 ws21-factory.bin`. PORT is the CH343's: `/dev/cu.usbmodem…`, or
   `/dev/cu.wchusbserial…` with WCH's driver.
3. **Prove the toolchain.** Build Waveshare's own Arduino demo here, flash it, and check it runs.
4. **Serial first.** Build ours and flash it with `tools/ws21_flash.sh <port>` (bootloader, partitions, boot_app0,
   firmware, then the sprites at 0x610000), on the CH343's port (`/dev/cu.usbmodem…`). Open the serial monitor at
   115200. Expect `Catch 151 fw v…`. Type `I2C`: it should list 0x15, 0x20, 0x51 and 0x6B (and Waveshare's FAQ also
   lists a 0x7E).
5. **Screen.** Does a picture show, the right way round, in the right colours? If not, check in this order: the
   setup commands (`WS21_INIT_OPS`), the pixel clock (try 16 MHz, as the Arduino demo), the porches, the PCLK edge,
   the colour order. Check that the 7 px ring is even on all four sides.
6. **Tearing.** Scroll a menu and run Berry Drop. If lines tear across, add the second frame buffer.
7. **Touch.** Tap near the four edges and the middle; the press preview should sit under the finger. If not, fix the
   axes in `KnomiTouch::getPoint()` in `ws21_hal.h`. If taps get missed, try the CST816S's other settings
   (0xEC, 0xFA) as the Knomi uses.
8. **Accelerometer.** Type `ACC` held upright, then on each side, and set `WS21_ACC_MAP` in `pin_config.h` so +X is
   right and +Y down. Then test tilt in Berry Drop and the screen turning.
9. **Backlight.** Let it dim and switch off, then wake it. No flicker at any level (try 5 kHz if it flickers).
10. **Clock.** Set Wi-Fi, let it take network time, restart without Wi-Fi: the time should still be right.
11. **Battery.** Fix the 2 A charging first (above). Then plug in a protected cell, compare the `HEALTH` line's mV
    with a multimeter, and adjust `BAT_ADC_RATIO`. The home screen's percentage should then match a full cell
    (100%) and a nearly flat one.
12. **Sound.** Wire the MAX98357A, uncomment `WS21_AUDIO_I2S`, start with the gain pin open.
13. **Soak.** 24 hours on battery, watching the `HEALTH` lines. Then measure the current at idle, dimmed and off.
14. **Then the case.**

The live save's rules still apply: a board under test uses its own save, never a copy of the real one.

## Open questions for Fedor

1. **The screen:** centred 1:1 with a 0.8 mm black ring (what the code does, and what I'd keep), or scaled up 3% to
   fill the glass?
2. **Sound hardware:** a MAX98357A breakout wired through the microSD slot (recommended: no soldering on the board),
   or through the 12-pin connector plus one solder joint, or a PAM8302A on one pin? And a microSD sniffer or
   extension ribbon for the slot, if you take the first.
3. **The DS Lite speakers:** what do they measure (ohms, size)? Both in series, or just one?
4. **The battery:** which cell exactly (size, protected, its maximum charge current)? Will you change R7 to bring
   charging down from 2 A, or pick a cell that takes 2 A?
5. **The case:** its shape and size, which way is up (the screen's rotation follows it), and which USB-C port and
   buttons it exposes. Should the screen still turn by itself when the handheld is turned, as the Knomi's does?
6. **The clock cell:** fit a rechargeable ML1220 on J4, so the time survives the power switch?
7. **Buttons:** is BOOT enough, as on the Knomi? GPIO 19 and 20 could take two more buttons if the sound goes
   through the card slot.
8. **The buzzer:** should it beep for alerts when the sound is off or no speaker is fitted?
9. **Updates:** Wi-Fi updates for these boards later, from their own channel on the site?
