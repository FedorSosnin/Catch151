#!/bin/bash
# Compiles the firmware for the Waveshare ESP32-S3-Touch-LCD-2.1 (480x480 round): compile only, nothing is
# flashed and nothing goes to web/. See docs/WAVESHARE_21.md. The project folder must be named PokeKnomiGochi.
# Usage: bash tools/build_ws21.sh
#
# BOARD_WS21 builds on BOARD_KNOMI_V2, as the computers' build does: to the game it is a Knomi with other hardware
# underneath (src/ws21_hal.h). The sprites live in flash as on the Knomi (same partition table, tools/pack_flash.py).
# CDCOnBoot=default: as on the Knomi, the USB-C goes through a USB-serial chip (a CH343P) to UART0.
set -e
cd "$(dirname "$0")/.."
if pgrep -x arduino-cli >/dev/null; then
  echo "another arduino-cli is running: wait for it to finish" >&2
  exit 1
fi
FQBN="esp32:esp32:esp32s3:CDCOnBoot=default,FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB"
B=build-ws21

arduino-cli compile --fqbn "$FQBN" --output-dir "$B" \
  --build-property "compiler.cpp.extra_flags=-DBOARD_WS21 -DBOARD_KNOMI_V2" \
  --build-property "compiler.c.extra_flags=-DBOARD_WS21 -DBOARD_KNOMI_V2" .
ls -l "$B/PokeKnomiGochi.ino.bin"
