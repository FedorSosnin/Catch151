// The soundtrack page's player (web/music/): the game's own music player (music_synth.h) and songs (music_data.h)
// built alone into a small WebAssembly module, no other code, that the page's audio worklet runs. So the page plays
// exactly what the game plays, live, looping the way the game loops, and no recording is kept anywhere.
// tools/build_music_page.sh builds it into web/music/music.wasm.
//
// Four players play the same song, each keeping one voice (the two pulses, the bass, the drums), so the page can show
// what each voice does; together they are the song (each voice adds into the mix on its own: music_synth.h sample()).
#include "music_synth.h"

#define MW_BLOCK 128  // an audio worklet's block

static MusicPlayer gVoice[4];
static float gOut[4][MW_BLOCK];

extern "C" {

// the songs: how many, and each one's name, tempo, intro and loop (in ticks, 24 a beat), 0-based
__attribute__((used)) int mwCount() { return MUS_COUNT - 1; }
__attribute__((used)) const char *mwName(int i) { return MUSIC_SONGS[i].name; }
__attribute__((used)) float mwBpm(int i) { return MUSIC_SONGS[i].bpm; }
__attribute__((used)) int mwLoops(int i) { return MUSIC_SONGS[i].loops; }
__attribute__((used)) int mwIntroTicks(int i) { return (int)MUSIC_SONGS[i].introTicks; }
__attribute__((used)) int mwLoopTicks(int i) { return (int)MUSIC_SONGS[i].loopTicks; }

__attribute__((used)) void mwBegin(float rate) {
  for (int c = 0; c < 4; c++) {
    gVoice[c] = MusicPlayer();
    gVoice[c].begin(rate);
    gVoice[c].mute = (uint8_t)(0x0F & ~(1 << c));
  }
}

// the song to play (0-based; -1 stops): it fades the one playing out first, as the game does between screens
__attribute__((used)) void mwPlay(int i) {
  for (int c = 0; c < 4; c++) gVoice[c].want(i < 0 ? MUS_NONE : (uint8_t)(i + 1));
}

// the song playing now (0-based, -1: none): changes once the last one has faded out
__attribute__((used)) int mwPlaying() { return (int)gVoice[0].playing() - 1; }

// the next n samples (n <= MW_BLOCK) of each voice, at mwVoice(0..3)
__attribute__((used)) void mwRender(int n) {
  if (n > MW_BLOCK) n = MW_BLOCK;
  for (int c = 0; c < 4; c++) {
    memset(gOut[c], 0, sizeof(gOut[c]));
    gVoice[c].render(gOut[c], n, 1.0f);
  }
}
__attribute__((used)) float *mwVoice(int c) { return gOut[c & 3]; }

}
