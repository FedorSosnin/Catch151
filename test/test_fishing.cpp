// Reel Fishing's rules (fishing.h): the water, casting, the bite, each weight's pace, the weigh-in, the score.
#include "framework.h"
#include "shim/Arduino.h"
#include "../src/fishing.h"
#include "../src/zones.h"

static const float DT = 1.0f / 60;

// steps until the phase is p (or a limit), returning every event seen
static uint32_t runUntil(FishState &s, FishPhase p, float limit = 30) {
  uint32_t ev = 0;
  for (float t = 0; t < limit && s.phase != p; t += DT) ev |= fishStep(s, DT);
  return ev;
}

// reels at taps a second (no thrashes unless they're allowed) until it's landed or lost
static FishPhase reelAt(FishState &s, float rate, bool thrashes = false) {
  float next = 0;
  for (float t = 0; t < 60 && s.phase == FP_REEL; t += DT) {
    if (!thrashes) s.thrashIn = 1e9f, s.warnLeft = s.thrashLeft = 0;
    if (t >= next) {
      fishTap(s);
      next += 1.0f / rate;
    }
    if (s.phase == FP_REEL) fishStep(s, DT);
  }
  return s.phase;
}

// a hooked fish of class cls, struck cleanly
static void hookOne(FishState &s, uint8_t cls, uint32_t seed = 7) {
  fishStart(s, seed);
  int i = cls == FC_GIANT ? FISH_GIANT : cls == FC_SMALL ? 0 : cls == FC_MID ? 3 : 5;
  if (cls == FC_GIANT) fishSpawn(s, FISH_GIANT);
  s.b[i].cls = cls;
  fishCastAt(s, s.b[i].x);
  runUntil(s, FP_WAIT);
  s.target = (int8_t)i;
  s.biteAt = s.t;
  s.nibbles = 0;
  runUntil(s, FP_BITE);
  CHECK(fishTap(s) & FE_HOOK);
}

TEST(fishing, the_water_holds_three_small_two_middling_and_a_big_one) {
  for (uint32_t seed = 1; seed < 200; seed++) {
    FishState s;
    fishStart(s, seed);
    int n[FC_N] = {};
    for (int i = 0; i < FISH_BLOBS; i++) {
      if (!s.b[i].on) continue;
      const FishKind &k = FISH_KINDS[s.b[i].kind];
      CHECK_EQ((int)k.cls, (int)s.b[i].cls);
      CHECK(s.b[i].kg10 >= k.kg10 * 80 / 100 && s.b[i].kg10 <= k.kg10 * 120 / 100 + 1);  // 0.8 to 1.2 of its kind
      n[s.b[i].cls]++;
    }
    CHECK_EQ(n[FC_SMALL], 3);
    CHECK_EQ(n[FC_MID], 2);
    CHECK_EQ(n[FC_BIG] + n[FC_HEAVY], 1);
    CHECK_EQ(n[FC_GIANT], 0);  // the giant comes later
  }
}

TEST(fishing, only_the_beachs_water_pokemon_and_the_giants_only_in_the_deep) {
  for (int k = 0; k < FISH_KIND_N; k++) {
    CHECK_EQ((int)zoneOf(FISH_KINDS[k].dex), (int)Z_BEACH);
    CHECK(FISH_KINDS[k].dex != 7 && FISH_KINDS[k].dex != 8 && FISH_KINDS[k].dex != 9);  // the starters stay special
    if (FISH_KINDS[k].cls == FC_GIANT) CHECK_EQ((int)FISH_KINDS[k].water, 4);
  }
  CHECK(FISH_SWIM[FC_GIANT][0] >= FISH_DEEP_X);
  CHECK(FISH_SWIM[FC_HEAVY][0] >= FISH_SHALLOW_X);
}

TEST(fishing, a_heavier_one_of_a_kind_has_a_bigger_blob) {
  FishBlob a = {}, b = {};
  a.cls = b.cls = FC_MID;
  a.r = 0.1f, b.r = 0.9f;
  CHECK(fishBlobSize(b) > fishBlobSize(a));
  a.cls = FC_GIANT;
  CHECK(fishBlobSize(a) > fishBlobSize(b));
}

TEST(fishing, a_cast_lands_where_you_tap_and_the_nearest_fish_comes) {
  FishState s;
  fishStart(s, 11);
  CHECK(fishCastAt(s, 9999) & FE_CAST);
  CHECK_EQ((int)s.floatX, FISH_CAST_X1);  // kept on the water
  CHECK(runUntil(s, FP_WAIT) & FE_SPLASH);
  float best = 1e9f;
  for (int i = 0; i < FISH_BLOBS; i++)
    if (s.b[i].on) best = fminf(best, fabsf(s.b[i].x - s.floatX));
  CHECK(s.target >= 0);
  CHECK(fabsf(fabsf(s.b[s.target].x - s.floatX) - best) < 2);
  CHECK_EQ(fishCastAt(s, 300), 0u);  // one cast at a time
}

TEST(fishing, too_soon_scares_that_fish_off_and_the_cast_goes_on) {
  FishState s;
  fishStart(s, 21);
  fishCastAt(s, 300);
  runUntil(s, FP_WAIT);
  int first = s.target;
  CHECK(first >= 0);
  CHECK(fishTap(s) & FE_TOO_SOON);
  CHECK_EQ((int)s.phase, (int)FP_WAIT);  // still fishing
  CHECK(s.b[first].gone > 0);             // that one swims off
  CHECK(s.target != first);
  CHECK_EQ((int)s.cast, 0);
  CHECK(s.soonT > 0);
}

TEST(fishing, nibbles_come_before_the_bite_and_a_strike_in_the_window_hooks_it) {
  for (uint32_t seed = 1; seed < 60; seed++) {
    FishState s;
    fishStart(s, seed);
    fishCastAt(s, 320);
    runUntil(s, FP_WAIT);
    uint32_t ev = runUntil(s, FP_BITE);
    if (s.nibbles) CHECK(ev & FE_NIBBLE);
    CHECK(ev & FE_BITE);
    CHECK_EQ((int)s.nibbled, (int)s.nibbles);
    CHECK(fishTap(s) & FE_HOOK);
    CHECK_EQ((int)s.phase, (int)FP_REEL);
    CHECK(s.clean);  // struck at once: the notch starts mid-green
    const FishClassDef &c = FISH_CLASS[s.b[s.target].cls];
    CHECK(s.T > c.lo && s.T < c.hi);
  }
}

TEST(fishing, too_slow_to_strike_ends_the_cast) {
  FishState s;
  fishStart(s, 5);
  fishCastAt(s, 300);
  runUntil(s, FP_BITE);
  uint32_t ev = runUntil(s, FP_AWAY, 3);
  CHECK(ev & FE_TOO_SLOW);
  CHECK_EQ((int)s.why, (int)FW_SLOW);
  CHECK_EQ((int)s.slot[0], -1);
  CHECK(runUntil(s, FP_AIM, 3) & FE_NEXT);
  CHECK_EQ((int)s.cast, 1);
}

TEST(fishing, a_late_strike_starts_near_the_slack_edge) {
  FishState s;
  fishStart(s, 9);
  fishCastAt(s, 300);
  runUntil(s, FP_BITE);
  float w = FISH_CLASS[s.b[s.target].cls].window;
  for (float t = 0; t < w * 0.6f; t += DT) fishStep(s, DT);
  fishTap(s);
  CHECK(!s.clean);
  CHECK(s.T < FISH_CLASS[s.b[s.target].cls].lo + 6);
}

TEST(fishing, each_weight_lands_at_its_own_pace) {
  for (uint8_t cls = 0; cls < FC_N; cls++) {
    FishState s;
    hookOne(s, cls);
    CHECK_EQ((int)reelAt(s, FISH_CLASS[cls].pace), (int)FP_LANDED);
    CHECK(s.slot[0] > 0);
    CHECK_EQ(s.total, (uint32_t)s.slotKg10[0]);
  }
}

TEST(fishing, too_fast_snaps_the_line_and_too_slow_lets_it_slip) {
  for (uint8_t cls = 0; cls < FC_N; cls++) {
    FishState s;
    hookOne(s, cls);
    CHECK_EQ((int)reelAt(s, FISH_CLASS[cls].pace * 2.6f), (int)FP_AWAY);
    CHECK_EQ((int)s.why, (int)FW_SNAP);
    CHECK_EQ((int)s.slot[0], -1);
    hookOne(s, cls);
    CHECK_EQ((int)reelAt(s, FISH_CLASS[cls].pace * 0.3f), (int)FP_AWAY);
    CHECK_EQ((int)s.why, (int)FW_SLIP);
  }
}

TEST(fishing, a_heavier_fish_wants_a_slower_pace_and_has_less_green) {
  for (int c = 1; c < FC_N; c++) {
    CHECK(FISH_CLASS[c].pace < FISH_CLASS[c - 1].pace);
    CHECK(FISH_CLASS[c].hi - FISH_CLASS[c].lo < FISH_CLASS[c - 1].hi - FISH_CLASS[c - 1].lo);
    CHECK(FISH_CLASS[c].window <= FISH_CLASS[c - 1].window);
    CHECK(FISH_CLASS[c].taps > FISH_CLASS[c - 1].taps);
    CHECK(FISH_CLASS[c].pace * 60 >= 70 && FISH_CLASS[c].pace * 60 <= 200);  // the fight song's tempo stays musical
  }
  CHECK(FISH_CLASS[0].pace <= 3.3f);  // nothing faster than a child taps comfortably
}

// the fish pulls harder or softer on each beat (FISH_TUG): the notch sinks at that beat's speed, never past the limits,
// and it does vary; the beat itself stays steady
TEST(fishing, the_fish_pulls_unevenly_from_beat_to_beat) {
  FishState s;
  hookOne(s, FC_MID);
  float lo = 9, hi = 0, beatT = -1;
  int beats = 0;
  for (float t = 0; t < 20 && beats < 40; t += DT) {
    s.thrashIn = 1e9f, s.warnLeft = s.thrashLeft = 0;
    s.T = 70, s.slackT = 0;  // (held in the green, never landed)
    if (fishStep(s, DT) & FE_BEAT) {
      if (beatT >= 0) CHECK(fabsf(t - beatT - 1.0f / FISH_CLASS[FC_MID].pace) < 2 * DT);  // a steady beat
      beatT = t;
      beats++;
      CHECK(s.tug >= 1 - FISH_TUG - 1e-4f && s.tug <= 1 + FISH_TUG + 1e-4f);
      lo = s.tug < lo ? s.tug : lo, hi = s.tug > hi ? s.tug : hi;
      float T0 = s.T;
      fishStep(s, DT);
      CHECK(fabsf(T0 - s.T - fishEase(FC_MID) * s.tug * DT) < 1e-3f);  // the notch sinks at this beat's pull
      s.T = 70;
    }
  }
  CHECK(beats >= 40);
  CHECK_MSG(hi - lo > FISH_TUG, "it varies");
}

TEST(fishing, a_thrash_is_warned_and_easing_off_through_it_saves_the_line) {
  FishState s;
  hookOne(s, FC_BIG);
  s.thrashIn = 0.01f;
  CHECK(fishStep(s, DT) & FE_WARN);
  uint32_t ev = 0;
  for (float t = 0; t < 0.5f; t += DT) ev |= fishStep(s, DT);
  CHECK(ev & FE_THRASH);
  float T0 = s.T;
  for (float t = 0; t < 0.3f; t += DT) fishStep(s, DT);  // no taps: the thrash alone eases more slowly
  CHECK(s.T < T0);
  CHECK(T0 - s.T < fishEase(FC_BIG) * 0.3f);
}

TEST(fishing, the_beat_follows_the_fishs_pace) {
  for (uint8_t cls = 0; cls < FC_N; cls++) {
    FishState s;
    hookOne(s, cls);
    int beats = 0;
    for (float t = 0; t < 2.0f && s.phase == FP_REEL; t += DT) {
      s.T = (FISH_CLASS[cls].lo + FISH_CLASS[cls].hi) / 2.0f;  // held mid-green, so only time passes
      s.thrashIn = 1e9f;
      if (fishStep(s, DT) & FE_BEAT) beats++;
    }
    CHECK(abs(beats - (int)lroundf(FISH_CLASS[cls].pace * 2)) <= 1);
  }
}

TEST(fishing, the_weigh_in_moves_on_and_a_tap_skips_it) {
  FishState s;
  hookOne(s, FC_SMALL);
  reelAt(s, FISH_CLASS[FC_SMALL].pace);
  CHECK(runUntil(s, FP_WEIGH) & FE_WEIGH);
  CHECK_EQ(fishTap(s), 0u);  // not at once: a tap still reeling lands on nothing
  for (float t = 0; t < FISH_WEIGH_TAP_S; t += DT) fishStep(s, DT);
  CHECK(fishTap(s) & FE_NEXT);
  CHECK_EQ((int)s.cast, 1);
  CHECK_EQ((int)s.phase, (int)FP_AIM);
}

// a bot that plays a whole game: casts by the biggest fish, strikes at once and reels by eye, as a player does:
// a tap when the notch sinks into the green's lower half (so it eases off through a thrash by itself). Sloppy: late
// to strike, and tapping on a rhythm of its own, off the pace by up to that share, not watching the notch
static FishState playGame(uint32_t seed, float sloppy, uint32_t *evs = nullptr) {
  FishState s;
  fishStart(s, seed);
  uint32_t r = seed * 2654435761u, ev = 0;
  float next = 0, now = 0, lateBy = 0;
  for (int guard = 0; guard < 60 * 600 && s.phase != FP_END; guard++, now += DT) {
    r = r * 1664525u + 1013904223u;
    float jitter = ((r >> 8) % 1000 / 1000.0f - 0.5f) * 2 * sloppy;
    if (s.phase == FP_AIM) {
      int best = -1;
      for (int i = 0; i < FISH_BLOBS; i++)
        if (s.b[i].on && s.b[i].gone <= 0 && (best < 0 || s.b[i].kg10 > s.b[best].kg10)) best = i;
      ev |= fishCastAt(s, best >= 0 ? s.b[best].x : 300);
    } else if (s.phase == FP_BITE) {
      if (lateBy <= 0) lateBy = 0.05f + sloppy * 0.5f * ((r >> 9) % 1000 / 1000.0f);
      if (s.t >= lateBy) ev |= fishTap(s), lateBy = 0;
    } else if (s.phase == FP_REEL) {
      const FishClassDef &c = FISH_CLASS[s.b[s.target].cls];
      if (sloppy < 0.01f) {
        if (s.T <= (c.lo + c.hi - c.pull) * 0.5f && now >= next) {
          ev |= fishTap(s);
          next = now + 0.15f;  // (a finger can't tap faster than this)
        }
      } else if (now >= next) {
        ev |= fishTap(s);
        next = now + 1.0f / (c.pace * (1 + jitter));
      }
    } else if (s.phase == FP_WEIGH && s.t > FISH_WEIGH_TAP_S) {
      ev |= fishTap(s);
    }
    ev |= fishStep(s, DT);
  }
  if (evs) *evs = ev;
  return s;
}

TEST(fishing, a_whole_game_is_five_casts_and_the_score_is_their_weight) {
  for (uint32_t seed = 1; seed <= 40; seed++) {
    uint32_t ev;
    FishState s = playGame(seed, 0, &ev);
    CHECK_EQ((int)s.phase, (int)FP_END);
    CHECK(ev & FE_END);
    uint32_t sum = 0;
    for (int i = 0; i < FISH_CASTS; i++) {
      CHECK(s.slot[i] != 0);
      if (s.slot[i] > 0) sum += s.slotKg10[i];
    }
    CHECK_EQ(s.total, sum);
  }
}

TEST(fishing, a_giant_comes_in_every_game) {
  for (uint32_t seed = 1; seed <= 40; seed++) {
    uint32_t ev;
    FishState s = playGame(seed, 0, &ev);
    CHECK(ev & FE_GIANT);
    CHECK(s.giantCast >= 2 && s.giantCast <= 4);
  }
}

TEST(fishing, skill_wins_a_careful_player_beats_a_sloppy_one) {
  int good = 0, sloppy = 0, gold = 0;
  uint32_t goodKg = 0, sloppyKg = 0;
  for (uint32_t seed = 1; seed <= 60; seed++) {
    FishState a = playGame(seed, 0), b = playGame(seed, 0.6f);
    for (int i = 0; i < FISH_CASTS; i++) good += a.slot[i] > 0, sloppy += b.slot[i] > 0;
    goodKg += a.total, sloppyKg += b.total;
    gold += fishGoal(a.total) == 3;
  }
  CHECK(good > sloppy);
  CHECK(goodKg > sloppyKg);
  CHECK(good >= 60 * FISH_CASTS * 85 / 100);  // the careful bot lands nearly all
  CHECK(gold >= 30);                          // and makes gold in most games (it goes for the giant)
}

TEST(fishing, the_goal_lines) {
  CHECK_EQ((int)fishGoal(0), 0);
  CHECK_EQ((int)fishGoal(499), 0);
  CHECK_EQ((int)fishGoal(500), 1);
  CHECK_EQ((int)fishGoal(1500), 2);
  CHECK_EQ((int)fishGoal(2999), 2);
  CHECK_EQ((int)fishGoal(3000), 3);
}
