#!/bin/zsh
# Remakes relay/src/board_art.js (the leaderboard's font and cartridges): pictures of the Games menu's three pages from a
# headless test copy (its own save and buddy port, no relay), cropped by tools/relay_board_art.py. Run it when a
# cartridge's art changes, then deploy the relay.
#   tools/relay_board_art.sh
set -e
cd "${0:A:h}/.."
make -C host -j8 >/dev/null
clang++ -std=c++17 -O2 -I host tools/host_shot.cpp build/host/libpokehost.a -framework CoreFoundation -o build/host_shot
T=$(mktemp -d)
mkdir -p $T/save
cat > $T/script.txt <<EOF
SER NOSAVE
SER TOWERPET 7 5 30 10 30
WAIT 2500
SER GAMES
WAIT 1500
SHOT $T/p1.ppm
TAP 432 228
WAIT 1500
SHOT $T/p2.ppm
TAP 432 228
WAIT 1500
SHOT $T/p3.ppm
EOF
RES=build/mac/PokeMacGochi.app/Contents/Resources
[[ -d $RES ]] || arch -arm64 zsh mac/build_app.sh
POKEMACGOCHI_BUDDY_PORT=47391 POKEMACGOCHI_SAVE_DIR=$T/save POKEMACGOCHI_RELAY=0 build/host_shot $RES $T/save $T/script.txt >/dev/null
python3 -I tools/relay_board_art.py $T/p1.ppm $T/p2.ppm $T/p3.ppm
rm -rf $T
