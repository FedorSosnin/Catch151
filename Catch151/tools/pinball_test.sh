#!/bin/zsh
# Pinball's test copy of Catch 151 for Mac, apart from the real app: its own save folder, its own buddy port (it never
# visits the real pets), and another bundle id. It opens on Pinball's card with a Squirtle. The test state isn't saved
# (NOSAVE): each launch starts the same.
#   tools/pinball_test.sh          Pinball
#   tools/pinball_test.sh night    the same at night
# Hold Left or Right (or click and hold either half) to raise that flipper, Space for both. Into the tall grass and a
# wild one jumps into the ring in the middle: hit it with the ball. The Terminal window takes console commands too:
#   PINBOT     Pinball plays by itself (again: stops)
#   PINGRASS   the ball into the grass, for a battle
#   PINBALL    a new game of Pinball
#   GAMES      the Games menu
# Close it with its menu (the Poke Ball) or Cmd-Q.
set -e
cd "${0:A:h}/.."
SRC=build/mac/PokeMacGochi.app
APP=build/mac/PinballTest.app
if [[ ! -d $SRC || -n "$(find src host mac Catch151.ino -newer $SRC/Contents/MacOS/PokeMacGochi -print -quit)" ]]; then
  mac/build_app.sh
fi
if [[ ! -d $APP || $SRC/Contents/MacOS/PokeMacGochi -nt $APP/Contents/MacOS/PokeMacGochi ]]; then
  rm -rf $APP && cp -R $SRC $APP
  /usr/libexec/PlistBuddy -c 'Set :CFBundleIdentifier tv.dsrp.pokemacgochi.pinballtest' $APP/Contents/Info.plist
  /usr/libexec/PlistBuddy -c 'Set :CFBundleName Pinball Test' $APP/Contents/Info.plist
  codesign --force --deep --sign - $APP 2>/dev/null
fi
SAVE="$HOME/Library/Application Support/PokeMacGochi-PinballTest"
mkdir -p "$SAVE"
seed() {
  echo NOSAVE
  echo "TOWERPET 7 25 30 10 30"  # a Squirtle, level 25
  echo "NEEDS 90 90 90 90"
  [[ "$1" == night ]] && echo "TIME $(( $(date +%s) / 86400 * 86400 + 22 * 3600 ))"
  echo PINBALL
}
{ sleep 2; seed "$1"; cat; } |
  POKEMACGOCHI_SAVE_DIR="$SAVE" POKEMACGOCHI_BUDDY_PORT=47391 $APP/Contents/MacOS/PokeMacGochi --console
