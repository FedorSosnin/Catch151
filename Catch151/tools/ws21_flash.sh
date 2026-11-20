#!/bin/zsh
# Builds Catch 151 for the Waveshare ESP32-S3-Touch-LCD-2.1 and flashes it (firmware + sprites) over USB, through
# the board's "UART" USB-C port. Does not erase the NVS: a board keeps its save and Wi-Fi from one flash to the next.
#   tools/ws21_flash.sh <port>           build and flash, e.g. tools/ws21_flash.sh /dev/cu.usbmodem5C940812191
#   tools/ws21_flash.sh --build-only
# The port is never guessed: a Knomi on the same Mac must not get this firmware. On a Mac the board's CH343 shows up
# twice, as /dev/cu.usbmodem<serial>1 and /dev/cu.wchusbserial<serial>1 (the same chip); a Knomi only as
# /dev/cu.wchusbserial*. So give the usbmodem name: a wchusbserial one is refused, in case it is a Knomi.
set -e
cd "${0:A:h}/.."
if [[ "$1" != "--build-only" && ! -e "$1" ]]; then
  echo "usage: tools/ws21_flash.sh <port> | --build-only" >&2
  echo "ports now:" >&2
  ls /dev/cu.usbmodem*(N) /dev/cu.wchusbserial*(N) /dev/cu.usbserial*(N) >&2
  exit 1
fi
if [[ "$1" == /dev/cu.wchusbserial* ]]; then
  echo "$1 looks like a Knomi (wchusbserial): not flashing it" >&2
  exit 1
fi
bash tools/build_ws21.sh
OUT=build-ws21
python3 -I tools/pack_flash.py "$OUT/sprites.tpkz"
[[ "$1" == "--build-only" ]] && exit 0
BOOTAPP=$(ls ~/Library/Arduino15/packages/esp32/hardware/esp32/*/tools/partitions/boot_app0.bin | tail -1)
ESPTOOL=$(ls ~/Library/Arduino15/packages/esp32/tools/esptool_py/*/esptool | tail -1)
"$ESPTOOL" --port "$1" --baud 460800 write-flash \
  0x0 "$OUT/Catch151.ino.bootloader.bin" \
  0x8000 "$OUT/Catch151.ino.partitions.bin" \
  0xe000 "$BOOTAPP" \
  0x10000 "$OUT/Catch151.ino.bin" \
  0x610000 "$OUT/sprites.tpkz"
