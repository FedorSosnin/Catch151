#!/bin/zsh
# Builds the soundtrack page's player, web/music/music.wasm: the game's music player and songs (host/music_web.cpp,
# src/music_synth.h, src/music_data.h) alone, as a small WebAssembly module the page's audio worklet runs. Run it after
# changing the songs (tools/music/songs.py), so the page plays what the game plays. Needs Emscripten (as
# webgame/build.sh). Also copies the logo and icons the page shows (tools/assets/brand, drawn by brand_assets.py).
# The page itself (web/music/index.html, music-worklet.js) and its pictures (web/music/img/, screens of the game) are
# kept as they are. GitHub Pages publishes web/music/ at https://fedorsosnin.github.io/Catch151/music/.
set -e
cd "${0:A:h}/.."
OUT=web/music
mkdir -p $OUT/brand
em++ -std=c++17 -O2 -DBOARD_HOST -DBOARD_KNOMI_V2 -I host -I host/shim -I . -I src \
  --no-entry -sSTANDALONE_WASM -sINITIAL_MEMORY=1MB -sSTACK_SIZE=65536 -sALLOW_MEMORY_GROWTH=0 \
  -sEXPORTED_FUNCTIONS=_mwCount,_mwName,_mwBpm,_mwLoops,_mwIntroTicks,_mwLoopTicks,_mwBegin,_mwPlay,_mwPlaying,_mwRender,_mwVoice \
  host/music_web.cpp -o $OUT/music.wasm
for f in catch151-soundtrack.svg catch151-icon-16.png catch151-icon-32.png catch151-icon-180.png; do
  cp tools/assets/brand/$f $OUT/brand/
done
echo "OK -> $OUT/music.wasm ($(( $(stat -f %z $OUT/music.wasm) / 1024 )) KB)"
