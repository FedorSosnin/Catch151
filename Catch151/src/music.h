#pragma once
// V2: background music. Each pass of the loop the game says which track its screen wants (music_v2.h picks it), and
// music_synth.h plays it under the effects, faded from one to the next. On a computer, the web, and the Waveshare 2.1
// with an amplifier added (audio.cpp mixes it there as host_audio.cpp does). The Knomi has no speaker, so it has no
// music code and no song in its firmware (HAS_MUSIC 0). The songs are written in tools/music/songs.py, which makes
// music_data.h.
#include <stdint.h>
#ifdef BOARD_WS21
#include "pin_config.h"  // (WS21_AUDIO_PDM / WS21_AUDIO_I2S: an amplifier is wired)
#endif

enum MusicTrack : uint8_t {
  MUS_NONE,
  MUS_HELLO,   // choosing a starter or the next Pokemon
  MUS_HOME,    // the home by day
  MUS_NIGHT,   // ...in the evening and at night, the pet awake
  MUS_MENU,    // the card, the Pokedex, the trophies, settings, buddies, the games menu
  MUS_TRAVEL,  // Knomi Island (in a zone, the zone's own: MUS_ZONE_FOREST...)
  MUS_TOWER,   // Knomi Tower, outside and between its floors
  MUS_FOREST,  // a battle in each zone, in zones.h's order (MUS_FOREST + zone)
  MUS_BEACH,
  MUS_CAVE,
  MUS_MANSION,
  MUS_FIELD,
  MUS_LEGEND,       // a legendary one: the zones' and the tower's Legend
  MUS_TOWER_FIGHT,  // a tower floor's guardian
  MUS_BUDDY,        // a battle with a buddy
  MUS_VICTORY,      // a battle won, a catch: once, then quiet until the screen changes
  MUS_BERRY,        // the minigames
  MUS_RUN,
  MUS_SURF,
  MUS_DUNGEON,
  MUS_QUIZ,     // Who's That? and Showdown
  MUS_VOLLEY,
  MUS_EVOLVE,   // while it evolves
  MUS_GOODBYE,  // a goodbye
  MUS_ZONE_FOREST,  // Travel: in a zone, its own (zones.h's order: MUS_ZONE_FOREST + zone)
  MUS_ZONE_BEACH,
  MUS_ZONE_CAVE,
  MUS_ZONE_MANSION,
  MUS_ZONE_FIELD,
  MUS_FISH,        // Reel Fishing: at the pier, waiting for a bite
  MUS_FISH_FIGHT,  // ...and a fish on the line, played at its pace (musicTempo)
  MUS_PINBALL,     // Pinball
  MUS_BERRYMATCH,  // BerryMatch
  MUS_COUNT
};

enum : uint8_t { VOICE_PET, VOICE_WIN };  // a Pokemon's cry (cry.h): petted, or after a win

#if defined(BOARD_HOST) || (defined(BOARD_WS21) && (defined(WS21_AUDIO_PDM) || defined(WS21_AUDIO_I2S)))
#define HAS_MUSIC 1
void musicWant(uint8_t track);  // the track this screen wants: what plays follows it (host_audio.cpp)
bool musicEnabled();            // the Music switch, saved as mus, on by default (apart from SOUND, the effects')
void musicSetEnabled(bool on);
// the levels on the settings page: 1 low, 2 medium, 3 high (the default, how they always played). SOUND's is all of it,
// the music included (saved as sndv); the music's is its share under that (musv). Setting one switches it on
uint8_t soundVolume();
void soundSetVolume(uint8_t level);
uint8_t musicVolume();
void musicSetVolume(uint8_t level);
// a Pokemon's cry (cry.h): with the effects, at SOUND's level, never while the pet sleeps
void voiceSay(int16_t dex, uint8_t mood);
// Reel Fishing: the fight song (MUS_FISH_FIGHT) at the hooked fish's pace, its tempo times f (1: as written); the
// pitch stays. Pinball's (MUS_PINBALL) the same way, a little faster while a wild one is in the ring. The other songs
// are never touched
void musicTempo(float f);
#else
#define HAS_MUSIC 0
#endif
