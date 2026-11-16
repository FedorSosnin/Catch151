#!/bin/zsh
# The zoom keyboard's test copy of Catch 151 for Mac, apart from the real app: its own save folder, its own buddy port
# (it never visits the real pets), no relay, and another bundle id. It opens on the gamer tag's keyboard with a
# Squirtle.
# The test state isn't saved (NOSAVE): each launch starts the same.
#   tools/keyboard_test.sh
# Click a letter and the keyboard zooms in round it; click the big keys to type (it stays zoomed). The blue down arrow
# where the X was zooms back out. Shift, backspace, 123, SPACE and OK work at once.
# Hold the mouse down to see the key under it big; slide off before letting go to change your mind. The Terminal
# window takes console commands too:
#   GAMERTAG   the gamer tag's keyboard again (four at most; a rude one gets PICK ANOTHER)
#   KEYBOARD   the name's keyboard
#   GAMES      the Games menu, with the gamer tag's label on the Game Boy (a click there opens its keyboard)
#   TOPSCORE   as if the relay said one of your bests is the first: the High Score medal (the golden cartridge)
#   TROPHIES   the trophy room (the cartridge is on the trophies' shelf, with the cups)
# Close it with its menu (the Poke Ball) or Cmd-Q.
set -e
cd "${0:A:h}/.."
SRC=build/mac/PokeMacGochi.app
APP=build/mac/KeyboardTest.app
if [[ ! -d $SRC || -n "$(find src host mac Catch151.ino -newer $SRC/Contents/MacOS/PokeMacGochi -print -quit)" ]]; then
  arch -arm64 zsh mac/build_app.sh
fi
if [[ ! -d $APP || $SRC/Contents/MacOS/PokeMacGochi -nt $APP/Contents/MacOS/PokeMacGochi ]]; then
  rm -rf $APP && cp -R $SRC $APP
  /usr/libexec/PlistBuddy -c 'Set :CFBundleIdentifier tv.dsrp.pokemacgochi.keyboardtest' $APP/Contents/Info.plist
  /usr/libexec/PlistBuddy -c 'Set :CFBundleName Keyboard Test' $APP/Contents/Info.plist
  codesign --force --deep --sign - $APP 2>/dev/null
fi
SAVE="$HOME/Library/Application Support/PokeMacGochi-KeyboardTest"
mkdir -p "$SAVE"
seed() {
  echo NOSAVE
  echo "TOWERPET 7 12 30 10 30"  # a Squirtle, level 13
  echo "NEEDS 90 90 90 90"
  echo GAMERTAG
}
{ sleep 2; seed; cat; } |
  POKEMACGOCHI_SAVE_DIR="$SAVE" POKEMACGOCHI_BUDDY_PORT=47391 POKEMACGOCHI_RELAY=0 $APP/Contents/MacOS/PokeMacGochi --console
