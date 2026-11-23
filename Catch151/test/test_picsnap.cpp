// Pic Snap's rules (picsnap.h): the ten places and who turns up in each, a roll's order, the lens's sweeps and the
// camera's drift, the score of a photo, a tap at each moment, one that gets away, and whole rolls by a bot that waits
// for a sharp, centred moment and by a player who taps at random.
#include "framework.h"
#include "shim/Arduino.h"
#include "../src/zones.h"
#include "../src/picsnap.h"

static const float DT = 1.0f / 25;  // the arcade rate (25 frames a second)

// a roll ready at its first place, the Pokemon there (PSP_AIM)
static PsState aiming(uint32_t seed = 7) {
  PsState s;
  psStart(s, seed);
  psReady(s);
  for (int i = 0; i < 100 && s.phase != PSP_AIM; i++) psStep(s, DT);
  return s;
}
// steps on until phase (or the roll's end), every event seen
static uint32_t until(PsState &s, uint8_t phase, float maxS = 30) {
  uint32_t ev = 0;
  for (float t = 0; t < maxS && s.phase != phase && s.phase != PSP_END; t += DT) {
    ev |= psStep(s, DT);
    if (s.phase == PSP_LOAD) ev |= psReady(s);  // (the drawing makes each place ready at once here)
  }
  return ev;
}

TEST(picsnap, ten_places_two_in_each_zone_each_with_its_own_zones_pokemon) {
  int perZone[ZONE_COUNT] = {};
  for (int p = 0; p < PS_SCENES; p++) {
    const PsSceneDef &d = PS_SCENE[p];
    CHECK(d.zone < ZONE_COUNT);
    perZone[d.zone]++;
    CHECK_RANGE(d.depth, 30, 70);  // never at the lens's ends: a sweep goes past it both ways
    CHECK_RANGE(d.how, PSH_POP, PSH_FADE);
    CHECK(hypotf(d.feetX - 233.0f, d.feetY - 233.0f) < 120);  // where it stands is well inside the round screen
    int common = 0;
    for (int i = 0; i < PS_CAST; i++) {
      if (!d.cast[i]) continue;
      CHECK_RANGE(d.cast[i], 1, DEX_COUNT);
      CHECK_MSG(zoneOf(d.cast[i]) == d.zone, "a place's Pokemon live in its zone");
      common += i < PS_CAST_COMMON;
    }
    CHECK_EQ(common, PS_CAST_COMMON);
  }
  for (int z = 0; z < ZONE_COUNT; z++) CHECK_EQ(perZone[z], 2);
}

TEST(picsnap, a_roll_is_every_place_once_in_a_new_order_each_game) {
  PsState a, b;
  psStart(a, 1);
  psStart(b, 2);
  bool same = true;
  for (int i = 0; i < PS_SHOTS; i++) {
    int n = 0;
    for (int j = 0; j < PS_SHOTS; j++) n += a.order[j] == i;
    CHECK_EQ(n, 1);
    same &= a.order[i] == b.order[i];
  }
  CHECK(!same);
  CHECK(a.phase == PSP_LOAD && a.shot == 0 && a.score == 0);
  CHECK(psSnap(a) == 0);  // nothing to snap while the place is made ready
  CHECK(psReady(a) == PE_IN);
  CHECK(a.phase == PSP_IN);
}

TEST(picsnap, who_turns_up_is_mostly_a_common_one_now_and_then_a_rare_one_and_seldom_shiny) {
  PsState s;
  psStart(s, 3);
  int rare = 0, shiny = 0, n = 20000;
  for (int i = 0; i < n; i++) {
    uint8_t place = (uint8_t)(i % PS_SCENES);
    int16_t dex = psPickDex(s, place);
    bool found = false;
    for (int k = 0; k < PS_CAST; k++)
      if (PS_SCENE[place].cast[k] == dex) {
        found = true;
        rare += k >= PS_CAST_COMMON;
        break;
      }
    CHECK(found);
    s.order[0] = place;
    psBeginPlace(s);
    shiny += s.shiny;
  }
  CHECK_RANGE(rare * 100 / n, 2, 8);    // (6 to 1 a common one's weight)
  CHECK_RANGE(shiny * 1000 / n, 5, 16);  // about 1 in 100
}

TEST(picsnap, the_lens_sweeps_through_the_pokemon_again_and_again_and_now_and_then_rests_on_it) {
  int locks = 0, rests = 0;
  for (int seed = 1; seed <= 60; seed++) {
    PsState s = aiming(seed);
    float d = psDepth(s), was = s.f - d;
    int crossings = 0;
    float near = 0;  // time within 0.02 of it, in a row
    bool rested = false;
    for (float t = 0; s.phase == PSP_AIM; t += DT) {
      uint32_t ev = psStep(s, DT);
      locks += (ev & PE_LOCK) != 0;
      CHECK_RANGE(s.f, 0.0f, 1.0f);
      float e = s.f - d;
      if ((e > 0) != (was > 0)) crossings++;
      was = e;
      near = fabsf(e) < 0.02f ? near + DT : 0;
      rested |= near >= 0.3f;
    }
    rests += rested;
    CHECK(crossings >= 3);  // while it's there, every sweep across it: chances enough
  }
  CHECK(locks >= 20);  // a sweep in about three stops on it...
  CHECK(rests >= 30);  // ...for a moment a patient player can wait for
}

TEST(picsnap, the_camera_drifts_within_its_room_and_wider_later_in_the_roll) {
  // picsnap_v2.h's room each side: PS_PAD (32) of the Knomi's pixels, 64 of the canvas's, past the most the camera
  // aims off the place's middle (PS_AIM_MAX, 20)
  CHECK(20 + PS_DRIFT_X_LAST <= 64 && 20 + PS_DRIFT_Y_LAST <= 64);
  PsState s = aiming(5);
  float mx = 0, my = 0, ax0 = s.ampX;
  for (float t = 0; t < 30; t += DT) {
    s.st += DT;
    psDrift(s);
    CHECK(fabsf(s.wx) <= s.ampX + 0.01f && fabsf(s.wy) <= s.ampY + 0.01f);
    mx = fmaxf(mx, fabsf(s.wx)), my = fmaxf(my, fabsf(s.wy));
  }
  CHECK(mx > s.ampX * 0.6f && my > s.ampY * 0.6f);  // it does move, most of the way
  s.shot = PS_SHOTS - 1;
  psBeginPlace(s);
  CHECK(s.ampX > ax0 && s.stay < PS_STAY_S);
}

TEST(picsnap, a_photo_scores_up_to_fifty_for_focus_and_fifty_for_the_middle) {
  CHECK_EQ(psFocusPts(0), (uint8_t)PS_PTS_FOCUS);
  CHECK_EQ(psFocusPts(PS_F_FULL), (uint8_t)PS_PTS_FOCUS);
  CHECK_EQ(psFocusPts(PS_F_ZERO), (uint8_t)0);
  CHECK_EQ(psFocusPts(0.9f), (uint8_t)0);
  CHECK_EQ(psCentrePts(0), (uint8_t)PS_PTS_CENTRE);
  CHECK_EQ(psCentrePts(PS_C_ZERO + 1), (uint8_t)0);
  for (float e = 0; e < 0.4f; e += 0.01f) CHECK(psFocusPts(e) >= psFocusPts(e + 0.01f));
  for (float c = 0; c < 100; c += 1) CHECK(psCentrePts(c) >= psCentrePts(c + 1));
  // the blur the drawing gives it: none while it scores all its focus points, more the further off
  CHECK_EQ(psMonBlur(0), 0);
  CHECK_EQ(psMonBlur(PS_F_FULL), 0);
  CHECK(psMonBlur(PS_F_FULL + 0.01f) >= 1);
  for (float e = 0; e < 1; e += 0.01f) CHECK(psMonBlur(e) <= psMonBlur(e + 0.01f) && psMonBlur(e) <= PS_BLUR_MON_MAX);
  CHECK_EQ(psLensStep(0), 0);
  CHECK_EQ(psLensStep(1), PS_BLUR_MAX);
  CHECK_EQ(psLensStep(-1), 0);
  CHECK_EQ(psLensStep(0.5f), PS_BLUR_MAX / 2);
}

TEST(picsnap, a_tap_takes_the_photo_as_it_is_then_its_print_then_the_next_place) {
  PsState s = aiming(9);
  s.f = psDepth(s), s.wx = s.wy = 0, s.ox = s.oy = 0;  // sharp and in the middle
  uint32_t ev = psSnap(s);
  CHECK(ev & PE_SNAP);
  CHECK(ev & PE_PERFECT);
  CHECK(s.phase == PSP_SNAP && s.got[0] && s.pts[0] == 100 && s.score == 100 && s.dexOf[0] == s.dex);
  CHECK(psSnap(s) == 0);  // one photo a place
  float f = s.f, wx = s.wx;
  psStep(s, DT);
  CHECK(s.f == f && s.wx == wx);  // the picture held: the lens and the camera wait
  until(s, PSP_PRINT);
  CHECK(s.phase == PSP_PRINT);
  psSnap(s);  // a tap too soon on the print: nothing
  CHECK(s.phase == PSP_PRINT);
  for (float t = 0; t < PS_PRINT_MIN_S + 0.05f; t += DT) psStep(s, DT);
  psSnap(s);  // then it sends it off
  ev = psStep(s, DT);
  CHECK(ev & PE_LOAD);
  CHECK(s.phase == PSP_LOAD && s.shot == 1);
  // off focus and off the middle: less on both
  psReady(s);
  until(s, PSP_AIM);
  s.f = psDepth(s) + 0.15f, s.wx = 16, s.wy = 0, s.ox = s.oy = 0;
  ev = psSnap(s);
  CHECK(!(ev & (PE_GOOD | PE_PERFECT)));
  CHECK_RANGE(s.focusPts, 10, 25);
  CHECK_RANGE(s.centrePts, 20, 35);
  CHECK_EQ(s.score, (uint16_t)(100 + s.focusPts + s.centrePts));
}

TEST(picsnap, it_shows_up_and_leaves_its_own_way_and_half_out_of_sight_is_half_a_photo) {
  for (uint8_t how : { (uint8_t)PSH_POP, (uint8_t)PSH_FLY, (uint8_t)PSH_FADE }) {
    int place = 0;
    while (PS_SCENE[place].how != how) place++;
    PsState s;
    psStart(s, 4);
    s.order[0] = (uint8_t)place;
    psBeginPlace(s);
    psReady(s);
    psStep(s, 0.01f);
    if (how == PSH_POP) CHECK(s.oy > 60);       // still down behind what's at its feet
    if (how == PSH_FLY) CHECK(s.ox > 200);      // still off in the sky
    if (how == PSH_FADE) CHECK(s.vis < 0.1f);   // still in the dark
    until(s, PSP_AIM);
    CHECK(s.vis == 1 && fabsf(s.ox) <= 13 && fabsf(s.oy) <= 5);  // there: at its spot, weaving or drifting a little
  }
  PsState s = aiming(12);
  s.phase = PSP_IN, s.t = PS_IN_S / 2;
  psMonMove(s);
  s.f = psDepth(s), s.wx = s.wy = 0;
  psSnap(s);
  CHECK(s.centrePts < PS_PTS_CENTRE - 10);  // snapped as it was coming: off the middle, or not all of it there
}

TEST(picsnap, while_its_there_it_stands_walks_hops_and_does_its_own_moves_within_its_reach) {
  int seen[4] = {};
  for (int place = 0; place < PS_SCENES; place++)
    for (int seed = 1; seed <= 20; seed++) {
      PsState s;
      psStart(s, seed);
      s.order[0] = (uint8_t)place;
      psBeginPlace(s);
      psReady(s);
      until(s, PSP_AIM);
      while (s.phase == PSP_AIM) {
        psStep(s, DT);
        seen[s.act]++;
        if (s.act == PSA_WALK || s.act == PSA_HOP) CHECK(PS_SCENE[place].how == PSH_POP && place != PS_WAVES);
        CHECK(fabsf(s.ox) <= PS_WALK_MAX + 9 && s.oy >= -PS_HOP_H - 6 && s.oy <= 6);
      }
    }
  for (int a = 0; a < 4; a++) CHECK(seen[a] > 0);
  CHECK(seen[PSA_IDLE] > seen[PSA_WALK]);  // mostly it stands
}

TEST(picsnap, not_snapped_in_time_it_gets_away_for_nothing_and_the_roll_goes_on) {
  PsState s = aiming(21);
  uint32_t ev = until(s, PSP_AWAY);
  CHECK(ev & PE_AWAY);
  CHECK(s.phase == PSP_AWAY && !s.got[0] && s.pts[0] == 0 && s.score == 0);
  CHECK(psSnap(s) == 0);  // gone: a tap does nothing
  ev = 0;
  for (float t = 0; t < PS_AWAY_S + 0.1f; t += DT) ev |= psStep(s, DT);
  CHECK(ev & PE_LOAD);
  CHECK(s.shot == 1);
}

// a whole roll: bot snaps when psBotWants() says (sharp and near the middle, or its last moment); otherwise at a
// moment picked at random while it's there. Its seconds come back in secs
static uint32_t playRoll(PsState &s, uint32_t seed, bool bot, float *secs) {
  psStart(s, seed);
  uint32_t ev = 0, r = seed * 2654435761u | 1;
  float at = -1, t = 0;
  while (s.phase != PSP_END && t < 400) {
    if (s.phase == PSP_LOAD) {
      ev |= psReady(s);
      r ^= r << 13, r ^= r >> 17, r ^= r << 5;
      at = (r % 1000) / 1000.0f * s.stay;
    }
    bool tap = bot ? psBotWants(s) : (s.phase == PSP_AIM && s.t >= at) || (s.phase == PSP_PRINT && s.t >= PS_PRINT_MIN_S + 0.4f);
    if (tap) ev |= psSnap(s);
    ev |= psStep(s, DT);
    t += DT;
  }
  *secs = t;
  return ev;
}

TEST(picsnap, whole_rolls_end_after_ten_photos_and_waiting_for_the_moment_beats_tapping_at_random) {
  const int rolls = 300;
  uint32_t botSum = 0, rndSum = 0, botLo = 9999, botHi = 0, rndHi = 0, seen = 0;
  float longest = 0, shortest = 1e9f;
  int perfect = 0;
  for (int g = 0; g < rolls; g++) {
    PsState s;
    float secs;
    uint32_t ev = playRoll(s, 500 + g, true, &secs);
    seen |= ev;
    CHECK(ev & PE_END);
    CHECK(s.phase == PSP_END && s.shot == PS_SHOTS - 1);
    uint16_t sum = 0;
    for (int i = 0; i < PS_SHOTS; i++) {
      CHECK(s.got[i]);  // the bot never lets one get away
      CHECK(s.dexOf[i] > 0);
      sum += s.pts[i];
      perfect += s.pts[i] >= PS_PERFECT;
    }
    CHECK_EQ(sum, s.score);
    CHECK(s.score <= PS_SHOTS * 100);
    CHECK(psSnap(s) == 0 && psStep(s, DT) == 0);  // over: nothing more
    botSum += s.score;
    botLo = s.score < botLo ? s.score : botLo, botHi = s.score > botHi ? s.score : botHi;
    longest = fmaxf(longest, secs), shortest = fminf(shortest, secs);
    PsState r;
    playRoll(r, 900 + g, false, &secs);
    CHECK(r.phase == PSP_END);
    rndSum += r.score;
    rndHi = r.score > rndHi ? r.score : rndHi;
  }
  CHECK(seen & PE_PERFECT);
  CHECK(seen & PE_LOCK);
  printf("    (bot: %d rolls, %u..%u, mean %u, %d perfect photos; tapping at random: mean %u, best %u; a roll %.0f..%.0f s)\n",
         rolls, botLo, botHi, botSum / rolls, perfect, rndSum / rolls, rndHi, shortest, longest);
  CHECK(botSum / rolls >= 750);  // waiting for the moment pays (a patient bot: about 790)...
  CHECK(rndSum / rolls * 100 <= botSum / rolls * 55);  // ...a tap at any old moment scores about half (about 400)
  CHECK(rndHi < botLo + 60);  // and a lucky random roll hardly ever beats a patient one
  CHECK(shortest >= 30 && longest <= 110);  // a roll takes a minute or so
}
