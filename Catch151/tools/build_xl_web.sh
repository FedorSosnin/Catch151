#!/bin/bash
# Regenerates web/knomi/xl/: Catch 151 XL's (the Waveshare 2.1's) Wi-Fi update channel, the firmware, the sprites and
# update.json. pages.yml copies web/knomi/ to the site's root, so XLs read https://fedorsosnin.github.io/Catch151/xl/
# (OTA_BASE_URL_XL in src/ota.h). Not a page: the XL is held back until its own release (no installer page, no links
# to it); XL boards just read it. update.json says "board": "xl", and an XL takes nothing that doesn't.
# Run it with every release, after tools/build_knomi_web.sh (the same version, from Catch151.ino).
# Usage: bash tools/build_xl_web.sh   (from the Catch151 folder, the sketch)
set -e
cd "$(dirname "$0")/.."
bash tools/build_ws21.sh
B=build-ws21
OUT=web/knomi/xl/firmware
python3 -I tools/pack_flash.py "$B/sprites.tpkz"

mkdir -p "$OUT"
cp "$B/Catch151.ino.bin" "$OUT/firmware.bin"
cp "$B/sprites.tpkz" "$OUT/sprites.bin"
du -ch "$OUT"/*.bin | tail -1

VER=$(sed -n 's/^#define FW_VERSION "\(.*\)"/\1/p' Catch151.ino)
SIZE=$(wc -c < "$OUT/sprites.bin" | tr -d ' ')
SHA=$(shasum -a 256 "$OUT/sprites.bin" | cut -d' ' -f1)
printf '{\n  "version": "%s",\n  "board": "xl",\n  "sprites_size": %s,\n  "sprites_sha256": "%s"\n}\n' "$VER" "$SIZE" "$SHA" \
  > web/knomi/xl/update.json
echo "xl/update.json: v$VER"
