#!/bin/zsh
# Reel Fishing's test copy of Catch 151 for Mac, apart from the real app: its own save folder, its own buddy port (it
# never visits the real pets), and another bundle id. It opens on the Games menu with a Squirtle, Reel Fishing the
# sixth cartridge. The test state isn't saved (NOSAVE): each launch starts the same.
#   tools/fishing_test.sh          the Games menu
#   tools/fishing_test.sh night    the same at night (the float's lamp)
# Click the water to cast (or aim with the arrow keys and press Space), click or press Space when the float goes
# under, then click or press Space to the beat to reel it in. The Terminal window takes console commands too:
#   FISHBOT    Reel Fishing plays by itself (again: stops)
#   GAMES      the Games menu again
# Close it with its menu (the Poke Ball) or Cmd-Q.
set -e
cd "${0:A:h}/.."
SRC=build/mac/PokeMacGochi.app
APP=build/mac/FishingTest.app
if [[ ! -d $SRC || -n "$(find src host mac PokeKnomiGochi.ino -newer $SRC/Contents/MacOS/PokeMacGochi -print -quit)" ]]; then
  mac/build_app.sh
fi
if [[ ! -d $APP || $SRC/Contents/MacOS/PokeMacGochi -nt $APP/Contents/MacOS/PokeMacGochi ]]; then
  rm -rf $APP && cp -R $SRC $APP
  /usr/libexec/PlistBuddy -c 'Set :CFBundleIdentifier tv.dsrp.pokemacgochi.fishingtest' $APP/Contents/Info.plist
  /usr/libexec/PlistBuddy -c 'Set :CFBundleName Reel Fishing Test' $APP/Contents/Info.plist
  codesign --force --deep --sign - $APP 2>/dev/null
fi
SAVE="$HOME/Library/Application Support/PokeMacGochi-FishingTest"
mkdir -p "$SAVE"
seed() {
  echo NOSAVE
  echo "TOWERPET 7 25 30 10 30"  # a Squirtle, level 25
  echo "NEEDS 90 90 90 90"
  [[ "$1" == night ]] && echo "TIME $(( $(date +%s) / 86400 * 86400 + 22 * 3600 ))"
  echo GAMES
}
{ sleep 2; seed "$1"; cat; } |
  POKEMACGOCHI_SAVE_DIR="$SAVE" POKEMACGOCHI_BUDDY_PORT=47391 $APP/Contents/MacOS/PokeMacGochi --console
