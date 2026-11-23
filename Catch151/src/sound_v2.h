// V2: the Sound page (not the Knomi, which has no speaker: HAS_MUSIC). The ring's speaker opens it (radial_v2.h).
// Fedor: "The sound options need to be simple and easy to use. One slider for music, and one for other sounds. Along
// with a mute button that turns both off quickly." The settings page has no sound rows here any more (Fedor: "this
// screen is a mess"). scene_v2.h includes this after about_v2.h, whose sliders it uses.
//
// It's the settings screen's (clockOpen) with soundOpen set, so closing it, a swipe up and the screen going off work
// as there. Opening Settings clears soundOpen.
#if HAS_MUSIC
#define V2_SOUND_PANEL_Y0 100
#define V2_SOUND_MUS_Y 124    // the Music row's top (V2_SET_ROW_H high)
#define V2_SOUND_FX_Y 184     // the Effects row's: the effects and the Pokemon's cries
#define V2_SOUND_PANEL_Y1 236
#define V2_MUTE_X0 83         // Mute: its box, which is also its tap
#define V2_MUTE_Y0 264
#define V2_MUTE_X1 383
#define V2_MUTE_Y1 314

// the speaker with a red cross, on Mute (its sound back, the ring's own icon, on Unmute)
static const char *const SPR_V2_MUTED[18] = {
  "..................",
  "........kk........",
  ".......ksk........",
  "......kssk........",
  ".....ksssk........",
  "....kssssk........",
  "kkkkkssssk.rr..rr.",
  "kwwssssssk..rrrr..",
  "kssssssssk...rr...",
  "kSSSSSSSSk...rr...",
  "kSSSSSSSSk..rrrr..",
  "kkkkkSSSSk.rr..rr.",
  "....kSSSSk........",
  ".....kSSSk........",
  "......kSSk........",
  ".......kSk........",
  "........kk........",
  "..................",
};

static bool v2MuteSnd = true, v2MuteMus = true;  // which were on when Mute was tapped: Unmute brings back those

static bool v2SoundMuted() { return !audioEnabled() && !musicEnabled(); }

static void v2SoundOpen() {
  openClock();
  soundOpen = true;
}

static void v2SoundPage() {
  v2MenuBackdrop();
  v2Sign(T(S_SOUND));
  v2Panel(72, V2_SOUND_PANEL_Y0, 394, V2_SOUND_PANEL_Y1, 10);
  v2SetLabel(T(S_MUSIC), V2_SOUND_MUS_Y);
  v2Slider(V2_SOUND_MUS_Y, musicEnabled() ? musicVolume() : 0);
  v2SetLabel(T(S_SND_EFFECTS), V2_SOUND_FX_Y);
  v2Slider(V2_SOUND_FX_Y, audioEnabled() ? soundVolume() : 0);
  // Mute, blue with the crossed speaker; once muted, Unmute, green with the speaker's sound back
  {
    bool muted = v2SoundMuted();
    const char *lab = muted ? T(S_UNMUTE) : T(S_MUTE);
    V2_HOVER(V2_MUTE_X0, V2_MUTE_Y0, V2_MUTE_X1 - V2_MUTE_X0, V2_MUTE_Y1 - V2_MUTE_Y0);
    v2Button(V2_MUTE_X0, V2_MUTE_Y0, V2_MUTE_X1 - V2_MUTE_X0, V2_MUTE_Y1 - V2_MUTE_Y0,
             muted ? C565(0x58, 0xb8, 0x68) : C565(0x4f, 0x93, 0xc4));
    setSize(2);
    int tw = textW(lab, 2), w = 36 + 10 + tw, x = CX - w / 2, cy = (V2_MUTE_Y0 + V2_MUTE_Y1) / 2;
    drawMap(muted ? SPR_V2_RAD_SOUND : SPR_V2_MUTED, HOME_ICON_N, x, cy - 18, 2, false);
    gfx->setTextColor(UI_WHITE);
    setCur(x + 46, cy - 7);
    printT(lab);
  }
  v2CloseX();
}

// a tap on a row: a little more round it than the settings page's, so it's easy to hit
static bool v2SoundRowHit(int16_t x, int16_t y, int y0) {
  return x >= V2_SET_ROW_X0 && x <= V2_SET_ROW_X1 + 10 && y >= y0 - 10 && y <= y0 + V2_SET_ROW_H + 10;
}
static bool v2SoundMuteHit(int16_t x, int16_t y) {
  return x >= V2_MUTE_X0 - 4 && x <= V2_MUTE_X1 + 4 && y >= V2_MUTE_Y0 - 4 && y <= V2_MUTE_Y1 + 4;
}

static void v2SoundTap(int16_t x, int16_t y) {
  if (closeHit(x, y)) {
    sfxPlay(SFX_TAP);
    clockOpen = soundOpen = false;
    return;
  }
  if (v2SoundRowHit(x, y, V2_SOUND_MUS_Y)) {  // (a notch: 0 off; another, that level, and on)
    int k = v2SliderAt(x);
    if (k == 0) musicSetEnabled(false);
    else if (k > 0) musicSetVolume((uint8_t)k);
    if (k >= 0) sfxPlay(SFX_TAP);
    return;
  }
  if (v2SoundRowHit(x, y, V2_SOUND_FX_Y)) {  // (the tap sounds at the level it sets)
    int k = v2SliderAt(x);
    if (k == 0) audioSetEnabled(false);
    else if (k > 0) soundSetVolume((uint8_t)k);
    if (k >= 0 && audioEnabled()) sfxPlay(SFX_TAP);
    return;
  }
  if (v2SoundMuteHit(x, y)) {
    if (v2SoundMuted()) {  // Unmute: what was on before, each at its own level
      musicSetEnabled(v2MuteMus);
      audioSetEnabled(v2MuteSnd);
      v2MuteSnd = v2MuteMus = true;
      sfxPlay(SFX_TAP);
    } else {  // Mute: both off at once
      v2MuteSnd = audioEnabled();
      v2MuteMus = musicEnabled();
      audioSetEnabled(false);
      musicSetEnabled(false);
    }
  }
}

// the press preview's (press.h)
static bool v2SoundPress(int16_t x, int16_t y, PressTarget &t) {
  if (v2SoundRowHit(x, y, V2_SOUND_MUS_Y)) return v2SliderPress(x, V2_SOUND_MUS_Y, t);
  if (v2SoundRowHit(x, y, V2_SOUND_FX_Y)) return v2SliderPress(x, V2_SOUND_FX_Y, t);
  if (v2SoundMuteHit(x, y))
    return v2PressSet(t, V2_MUTE_X0, V2_MUTE_Y0, V2_MUTE_X1 - V2_MUTE_X0, V2_MUTE_Y1 - V2_MUTE_Y0,
                      v2SoundMuted() ? T(S_UNMUTE) : T(S_MUTE));
  return closeHit(x, y) && v2PressNav(x, y, t, 0, 0, 0, true, V2_CLOSE_Y);  // its X
}
#endif  // HAS_MUSIC
