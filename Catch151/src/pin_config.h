#pragma once

#if defined(BOARD_WS21)

// Waveshare ESP32-S3-Touch-LCD-2.1 (ESP32-S3R8: 8 MB octal PSRAM, 16 MB flash). Pins from Waveshare's own demo
// (ESP32-S3-Touch-LCD-2.1-Demo: Arduino/examples/LVGL_Arduino and ESP-IDF/.../main), in docs/WAVESHARE_21.md.
// BOARD_KNOMI_V2 is also set: the game sees a Knomi (ws21_hal.h).

// The game still draws on its 466x466 canvas; it sits in the middle of the 480x480 panel, 1:1
#define LCD_WIDTH 466
#define LCD_HEIGHT 466
#define PANEL_SIZE 480

// ST7701S, 16-bit RGB565 (the panel is set to 18-bit RGB666, its low bits not wired)
#define LCD_HSYNC 38
#define LCD_VSYNC 39
#define LCD_DE 40
#define LCD_PCLK 41
#define LCD_B0 5
#define LCD_B1 45
#define LCD_B2 48
#define LCD_B3 47
#define LCD_B4 21
#define LCD_G0 14
#define LCD_G1 13
#define LCD_G2 12
#define LCD_G3 11
#define LCD_G4 10
#define LCD_G5 9
#define LCD_R0 46
#define LCD_R1 3
#define LCD_R2 8
#define LCD_R3 18
#define LCD_R4 17
// timings, as Waveshare's demo: 18 MHz, about 66 frames a second
#define LCD_PCLK_HZ 12000000  // 44 refreshes a second (Waveshare's demos: 16 and 18): plenty for a game drawing ~7 frames a
                               // second, and each refresh's copy out of PSRAM takes the CPU less often. The console's
                               // PCLK tries others
#define LCD_PCLK_NEG 1  // data out on the falling edge, so the panel reads it steady on the rising one. With 0 (as
                        // Waveshare's demos) thin streaks ran along the rows at any pixel clock (2026-11-20)
#define LCD_HSYNC_POL 1  // (1: idle high, esp_lcd's default, which the demo keeps)
#define LCD_HSYNC_PW 8
#define LCD_HSYNC_BP 10
#define LCD_HSYNC_FP 50
#define LCD_VSYNC_POL 1
#define LCD_VSYNC_PW 3
#define LCD_VSYNC_BP 8
#define LCD_VSYNC_FP 8
#define LCD_BOUNCE_PX (16 * PANEL_SIZE)  // bounce buffers: 16 lines each in internal RAM (2 x 15 KB; the demo has 10),
                                         // so a short stall (a flash read, the cache written back: ws21Writeback(),
                                         // 0.4 ms) doesn't make a refill late (a band goes out in 0.73 ms). 480 / 16 =
                                         // 30 a refresh. The refill's DMA brings the next band into 15 KB more
                                         // (Ws21Prefetch). (30 lines took 19 KB more, and internal RAM fell to 34 KB
                                         // while TLS connected)

// the panel's setup commands: 3-wire SPI (9-bit) on GPIO 1 and 2, its CS on the expander. GPIO 1 and 2 are also
// the microSD's CMD and CLK; the game doesn't use the card
#define LCD_SPI_MOSI 1
#define LCD_SPI_SCK 2

// backlight: PWM on GPIO 6, high = on (the demo: 20 kHz)
#define LCD_BL 6
#define LCD_BL_PWM_HZ 20000

// I2C: the expander, touch, IMU and RTC
#define IIC_SDA 15
#define IIC_SCL 7

// TCA9554PWR IO expander. Its lines, by bit (Waveshare's EXIO1..EXIO8 are bits 0..7)
#define EXIO_ADDR 0x20
#define EXIO_LCD_RST 0  // EXIO1: the panel's reset
#define EXIO_TP_RST 1   // EXIO2: the touch's reset
#define EXIO_LCD_CS 2   // EXIO3: the panel's SPI CS
#define EXIO_SD_CS 3    // EXIO4: the microSD's D3/CS
// (EXIO5 and 6: the IMU's interrupts, EXIO7: the clock's; unused, and outputs like the rest, as in Waveshare's demo)
#define EXIO_BUZZER 7   // EXIO8: the buzzer (on/off only)

// CST820 touch
#define TOUCH_I2C_ADDR 0x15
#define TP_INT 16
#define TP_RESET -1  // (on the expander: EXIO_TP_RST)

// QMI8658 accelerometer and gyro. TODO(bring-up): its axes against the panel's; this keeps them as they come.
#define IMU_I2C_ADDR 0x6B
#define WS21_SMOOTH 1    // turned (not by quarter turns), the picture blended between pixels, not the nearest pixel
#define WS21_ROT_DEG 0   // the picture turned clockwise on the panel, in degrees (ws21_hal.h). 45 puts a USB-C port
                         // straight down, but a frame then took 164 ms against the 100 the game is made for, and
                         // everything played slower than on the Knomi: Fedor chose 0, "Knomi speed" (2026-11-20)
// The QMI8658's axes are a quarter turn round from the screen's: the sensor's +Y is the screen's +X, its +X the
// screen's -Y. Found from Fedor's video (2026-11-20): held with the cable to the left it read up as (0.58, -0.70);
// with the sensor taken as it is, and then mirrored, the picture settled the wrong way round each time.
#define WS21_ACC_MAP(a, g) ((g)[0] = (a)[1], (g)[1] = -(a)[0], (g)[2] = (a)[2])

// PCF85063 clock: 0x51, SensorLib's default

// battery: through a 3:1 divider into GPIO 4 (ADC1 channel 3). The demo also divides by 0.9945, a measured correction
#define BAT_ADC_PIN 4
#define BAT_ADC_RATIO (3.0f / 0.9945f)

// microSD (not used: the sprites are in flash, as on the Knomi, and its lines can carry the amplifier's, below).
// SDMMC 1-bit: CLK 2, CMD 1, D0 42, D3 on the expander
#define SDMMC_CLK 2
#define SDMMC_CMD 1
#define SDMMC_DATA 42

// BOOT button
#define KNOMI_BTN_PIN 0

// Sound. With no amplifier the game is silent, as on the Knomi. WS21_BUZZER beeps the effects on the board's buzzer
// instead, but it's one high pitch, on or off (Fedor: "Its not a good sound. Very high pitch and abrupt"), so it's off.
// With an amplifier added by hand, uncomment its line (docs/WAVESHARE_21.md, "Sound"):
// #define WS21_BUZZER
// - WS21_AUDIO_PDM: a PAM8302A on the 12-pin connector, Fedor's choice (2026-11-20). GPIO 19 sends the sound as a
//   1-bit stream (the I2S in PDM mode, with no clock line) straight into the amplifier's A+ (no resistor or capacitor,
//   Fedor 2026-11-22), and GPIO 20 is the amplifier's SD (high plays). The "USB" port then only charges.
// - WS21_AUDIO_I2S: a MAX98357A on the microSD slot's lines (the game uses no card; GPIO 1 and 2 only set up the
//   panel at boot).
// On (2026-11-22, Fedor: "let's add music and sound through that amp"): the PAM8302A on the 12-pin connector. Effects,
// cries and music, mixed in audio.cpp.
#define WS21_AUDIO_PDM
// #define WS21_AUDIO_I2S
#if defined(WS21_AUDIO_PDM)
#define PDM_DO_IO 19               // 12-pin connector pin 3 -> A+ (pin 5, GND -> A-)
#define PA 20                      // pin 4 -> SD: high plays, low shuts the amplifier down
#else
#define I2S_BCK_IO 2               // microSD CLK -> MAX98357A BCLK
#define I2S_WS_IO 1                // microSD CMD -> LRC
#define I2S_DO_IO 42               // microSD DAT0 -> DIN
#define WS21_AMP_EXIO EXIO_SD_CS   // microSD DAT3 (EXIO4) -> SD: high plays (left channel), low shuts it down
#endif

#elif defined(BOARD_KNOMI_V2)

// BIGTREETECH Knomi V2 (ESP32-S3R8, 16 MB of flash). Pins from BTT's official firmware
// (firmware branch, src/pinout_knomi_v2.h). No SD, audio, RTC or battery.

// The game still draws on a 466x466 logical canvas; ScaledCanvas scales it down to the panel.
#define LCD_WIDTH 466
#define LCD_HEIGHT 466
#define PANEL_SIZE 240
// ...each 2 x 2 block of it one of the panel's pixels: the canvas's 233 x 233 shown one to one, centred, a dark edge
// round it 3 px wide (4 on the right and below). Stretched over all 240, 7 columns and 7 rows were shown twice and a
// line on one of them came out twice as thick (Fedor: "some of the text in the how to play has thick lines")
#define PANEL_PIC (LCD_WIDTH / 2)
#define PANEL_OFF ((PANEL_SIZE - PANEL_PIC) / 2)

// GC9A01 240x240 SPI display, single-wire AW9346 backlight
#define LCD_MOSI 14
#define LCD_SCLK 18
#define LCD_CS 20
#define LCD_DC 19
#define LCD_RESET 21
#define LCD_BL 12

// CST816S touch over I2C
#define IIC_SDA 2
#define IIC_SCL 1
#define TP_INT 17
#define TP_RESET 16

// Tilt: accelerometer axis parallel to the screen width, and its sign
#define KNOMI_TILT_AXIS 0
#define KNOMI_TILT_SIGN -1.0f  // measured on the board: tilting left gives positive X

// BOOT button
#define KNOMI_BTN_PIN 0

#else  // Waveshare ESP32-S3-Touch-AMOLED-1.75

// Official Waveshare ESP32-S3-Touch-AMOLED-1.75 pins
// Fuente: github.com/waveshareteam/ESP32-S3-Touch-AMOLED-1.75 (libraries/Mylibrary/pin_config.h)

#define XPOWERS_CHIP_AXP2101

// AMOLED 466x466 display, CO5300 driver over QSPI
#define LCD_SDIO0 4
#define LCD_SDIO1 5
#define LCD_SDIO2 6
#define LCD_SDIO3 7
#define LCD_SCLK 38
#define LCD_CS 12
#define LCD_RESET 39
#define LCD_WIDTH 466
#define LCD_HEIGHT 466

// Capacitive touch CST9217 over I2C
#define IIC_SDA 15
#define IIC_SCL 14
#define TP_INT 11
#define TP_RESET 40

// Audio ES8311. NOTE: the real MCLK is GPIO42 (verified on the board with the
// PlaneRadar2.0 project); the 16 that was listed was wrong. Even so the codec is
// configured with a clock derived from BCLK, so MCLK hardly matters.
#define I2S_MCK_IO 42
#define I2S_BCK_IO 9
#define I2S_DI_IO 10
#define I2S_WS_IO 45
#define I2S_DO_IO 8
#define PA 46

// TF slot (not used yet)
#define SDMMC_CLK 2
#define SDMMC_CMD 1
#define SDMMC_DATA 3
#define SDMMC_CS 41

#endif  // BOARD_KNOMI_V2
