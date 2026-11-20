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
| `src/audio.cpp` | The buzzer beeps the effects. Sound through an added MAX98357A once `WS21_AUDIO_I2S` is on. |
| `src/ota_update.cpp` | Wi-Fi updates are off. The update site carries the Knomi's firmware, which would not drive this screen. |
| `src/autorot.h` | The quarter turn stays 0: the picture turns in `ws21_hal.h` instead (45° plus quarter turns). |
| `src/knomi_rim.h` | Its row tables can be sized for the 466 canvas. The Knomi's build is unchanged. |
| `tools/build_ws21.sh` | Compile only. Output in `build-ws21/` (git ignores it). |
| `tools/ws21_flash.sh` | Builds, packs the sprites and flashes a board. The port must be given. |

What the code uses:

- **Panel:** esp_lcd's RGB panel, driven directly (`Ws21Panel`). Two frame buffers in PSRAM: the next frame goes
  into the one not on show, which takes over at the start of the next refresh, so nothing tears. Bounce buffers of
  20 lines in internal RAM feed the panel. 12 MHz pixel clock (44 Hz). Data changes on the **falling** edge of the
  pixel clock: on the rising edge, as in Waveshare's demo, this panel showed streaks along the rows.
- **Panel setup:** Waveshare's own command list, copied from their demo. Arduino_GFX has one for a similar 2.1" panel
  (`TL021WVC02`), but its gamma, VCOM and inversion settings differ. It goes over a 9-bit software SPI on GPIO
  1 and 2, with the CS and the panel's reset on the expander (`Ws21InitBus`).
- **Canvas:** `Ws21Canvas` makes the panel's rows from the 466 canvas a band of 8 at a time in internal RAM: in the
  middle, turned, and with the rim. Each band goes to the frame buffer by DMA while the next one is made, all on the
  game's core. The CPU writing PSRAM through the cache was the slowest part of a frame.
- **Straight:** the picture sits as the panel does (`WS21_ROT_DEG` 0), so the USB-C ports point out at 45°: the case
  handles that. Turned 45°, one port pointed straight down, but a frame took 164 ms against the 100 the game is made
  for, and everything played slower than on the Knomi; Fedor chose speed. Any angle still works (`ROTDEG`): each
  panel pixel then blends the four canvas pixels round its point (`WS21_SMOOTH`); without the blend, pixel art
  comes out dotted and its dithering in zigzags.
- **Auto-rotate:** in quarter turns, as the board is held. Each copies whole pixels, with no blending: upside down
  the same way as straight (120 ms a frame), sideways a canvas column per panel row (140 ms).
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

### What bring-up found (2026-11-20)

- **Streaks along the rows:** the pixel clock's edge. The panel wants data on the falling edge (`LCD_PCLK_NEG`).
- **The picture jumping:** reading sprites with `esp_partition_read()` stops the cache, and with it the panel's
  refill. Sprites are read through a flash map instead (`flashpak.h`).
- **Tearing:** one frame buffer. Now two, swapped between refreshes.
- **Speed:** almost everything here waits on PSRAM. The game draws a frame in about 75 ms. Copying it to the panel
  by CPU took 68 ms (138 ms turned 45° and blended): the CPU reads each frame-buffer line in before writing it.
  Straight, the canvas now comes in by DMA and goes out by DMA, a band at a time through internal RAM, and the CPU
  only shades the rim: 33 ms. A whole frame is about 110 ms (205 ms turned 45° at first, 164 by DMA). On both cores
  the turned copy took 65 ms, but the glitch lines came back (`SPLIT`). The refill, on the game's core, still takes
  about a quarter of that core.
- **Taps:** the game read the touch once a frame. A tap that came and went within a 200 ms frame was lost. A touch
  task now queues every press and lift.
- **The rule:** only one core may read PSRAM hard at a time, and it must be the core the refill runs on. The refill
  copies each band of the frame out of PSRAM in time for the panel; when the other core reads PSRAM at the same
  moment, it falls behind and lines show along the panel's rows (at 45° to the picture). Both times the refill or
  part of the frame copy went to the other core, the lines came back.
- **The buzzer:** each effect beeps its rhythm, a beep a note (`audio.cpp`). A tap is a 4 ms click.

Console commands for testing (115200 baud on the "UART" port; none of them is saved unless it says so):

| Command | What it does |
|---|---|
| `FPS` | Frames drawn since the last `FPS`, their time, and the frame copy's parts. |
| `PERF` | The frame copy's cost, 10 in a row. |
| `ROTDEG n` | Turns the picture n degrees clockwise. |
| `SMOOTH 0` / `SMOOTH 1` / `SMOOTH 2` | Turned: nearest pixel, blended, or blended only across the 2 px grid. |
| `PCLK n` | The pixel clock in MHz. |
| `PCLKEDGE 0` / `PCLKEDGE 1` | Data on the rising or falling edge. |
| `REFILLCORE 0` / `REFILLCORE 1` | The core the panel's refill runs on. Saved; used from the next start. |
| `TOUCHLOG 1` | Each press and lift, with how long the chip and the game took to see it. |
| `SPLIT 0` / `SPLIT 1` | The frame copy on the game's core alone, or on both (faster, but the lines come back). |
| `LCD` | Refreshes since the last `LCD`, and the longest gap between two. |
| `HOLD n` | The game stops for n seconds; only the panel's refill runs. |
| `WIFIOFF` | Wi-Fi off until the next start. |
| `SHOT` | The whole 480 × 480 frame buffer, as RGB565. |

## Sound

The game's effects are square-wave notes from 196 to 3136 Hz (`sfx_notes.h`), Game Boy style. Music and the
Pokémon's cries only play on computers today (`HAS_MUSIC`). They could come to this board later; the Knomi has no
speaker, so nobody has needed them on a board yet.

### The buzzer can't replace a speaker

The buzzer hangs off expander line EXIO8, through a transistor. No ESP32 pin reaches it, so PWM can't drive it.
Waveshare's own code only turns it on and off, and it sounds at one fixed pitch when on. Without a speaker the game
beeps each effect's rhythm on it, one beep a note, and a tap is a click. It can't play notes, jingles or music. For
real sound you need a speaker and an amplifier.

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

### Wiring: all from the microSD slot (recommended)

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

This is what `pin_config.h` is set up for. It keeps both USB ports and the 12-pin connector free.

- GPIO 1 and 2 also go to the panel's setup lines. The panel only listens while its CS is low, and that's only at
  boot. Sound starts after the panel is set up.
- The card's lines have 10k pull-ups. They do no harm: each line is driven. DAT3 high means the amplifier plays the
  left channel, which carries the game's sound.

### Other wirings

- **12-pin connector plus one solder joint.** BCLK on GPIO 19 (pin 3), LRC on GPIO 20 (pin 4), and DIN on GPIO 42,
  soldered at the card slot's DAT0. Uses the included SH1.0 cable. You lose the "USB" port (use the "UART" one), and
  the amplifier has no on/off line: it stays on (2.4 mA).
- **An analog amplifier on one pin**, such as a PAM8302A (Adafruit 2130). GPIO 19 makes a 1-bit signal (the S3's I2S
  in PDM mode), a resistor and capacitor smooth it, and the PAM8302A drives the speakers. GPIO 20 can be its
  shutdown pin. All from the 12-pin connector, and the board has a volume trimmer. The sound is a little rougher,
  and the code needs a small change (PDM instead of standard I2S).

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
  Nothing on the board says whether USB is in or the cell is charging. So the percentage reads high while charging,
  and the code treats the board as always on USB for now (no light sleep, no low-battery shutdown).
  - A fix that needs a wire: USB 5 V (12-pin, pin 2) through a divider into GPIO 19 or 20 would tell the firmware
    when USB is in.
- **On screen:** the battery takes the clock's place at the top of the home screen: an icon and the percentage.
  The icon turns amber under 40% and red under 15%. While charging it turns green with a bolt, but only once the
  board can tell it is charging (the wire above). With no cell, the clock comes back. The time is still on the
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
