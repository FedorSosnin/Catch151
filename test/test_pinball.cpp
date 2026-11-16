// Pinball's rules and physics (pinball.h): the serve, gravity and the rim, the flippers, the drain and the three
// balls, the bumpers, Diglett and Dugtrio, the rounded corners, the berries' multiplier, the ramp's turn and the wire
// rail, the grass and the battle in the ring, and whole games played by a simple bot, the ball never leaving the table.
#include "framework.h"
#include "shim/Arduino.h"
#include "../src/pinball.h"

static const float DT = 1.0f / 60;

// a ball put down at (x, y) with speed (vx, vy), in play
static void putBall(PinState &s, float x, float y, float vx, float vy) {
  s.phase = PIN_PLAY;
  s.t = 0;
  s.onTrack = false;
  s.x = x, s.y = y, s.vx = vx, s.vy = vy;
}
// steps for secs, every event seen
static uint32_t run(PinState &s, float secs) {
  uint32_t ev = 0;
  for (float t = 0; t < secs; t += DT) ev |= pinStep(s, DT);
  return ev;
}

TEST(pinball, a_game_starts_with_three_balls_one_rolling_in_along_the_orbit) {
  PinState s;
  pinStart(s, 7);
  CHECK(s.phase == PIN_PLAY);
  CHECK_EQ(s.ballsLeft, (uint8_t)2);
  CHECK_EQ(s.ball, (uint8_t)1);
  CHECK_EQ(s.mult, (uint8_t)1);
  CHECK_EQ(s.score, (uint32_t)0);
  float d = hypotf(s.x - PB_CX, s.y - PB_CY);
  CHECK(d > PB_ORBIT_R0 && d < PB_R);  // in the orbit lane
  CHECK(s.flipA[0] == PB_FLIP_REST && s.flipA[1] == PB_FLIP_REST);
  bool day = false, night = false;  // both looks come up, at random
  for (uint32_t seed = 1; seed < 40; seed++) {
    pinStart(s, seed);
    (s.day ? day : night) = true;
  }
  CHECK(day && night);
}

TEST(pinball, gravity_pulls_it_down_and_the_rim_keeps_it_in) {
  PinState s;
  pinStart(s, 3);
  putBall(s, 233, 140, 0, 0);
  s.bumpLit = 0;
  for (int i = 0; i < 6; i++) pinStep(s, DT);
  CHECK(s.vy > 20 && s.y > 140);
  putBall(s, 233, 233, 520, -300);  // thrown hard at the wall, again and again: it stays on the table
  for (int i = 0; i < 600 && s.phase == PIN_PLAY; i++) {
    pinStep(s, DT);
    CHECK(hypotf(s.x - PB_CX, s.y - PB_CY) <= PB_R + 0.5f || s.y > PB_FLIP_Y);
  }
}

TEST(pinball, a_flipper_swung_up_throws_the_ball_up_the_table) {
  PinState s;
  pinStart(s, 5);
  // the ball resting on the left flipper, near its tip
  float a = PB_FLIP_REST, tx = PB_FLIP_LX + 52 * cosf(a), ty = PB_FLIP_Y + 52 * sinf(a);
  putBall(s, tx, ty - 18, 0, 0);
  run(s, 0.4f);  // it settles on the flipper
  CHECK(s.phase == PIN_PLAY);
  CHECK_EQ(pinHold(s, 0, true), (uint32_t)PE_FLIP);
  CHECK_EQ(pinHold(s, 0, true), (uint32_t)0);  // held: only once
  float best = 0;
  for (int i = 0; i < 20; i++) {
    pinStep(s, DT);
    best = s.vy < best ? s.vy : best;
  }
  CHECK_MSG(best < -300, "thrown up fast");
  CHECK(s.flipA[0] == PB_FLIP_UP);
  pinHold(s, 0, false);
  run(s, 0.3f);
  CHECK(s.flipA[0] == PB_FLIP_REST);
}

TEST(pinball, the_flipper_never_jumps_past_the_ball_even_at_ten_frames_a_second) {
  PinState s;
  pinStart(s, 5);
  float a = PB_FLIP_REST, tx = PB_FLIP_LX + 40 * cosf(a), ty = PB_FLIP_Y + 40 * sinf(a);
  putBall(s, tx, ty - 17, 0, 0);
  pinHold(s, 0, true);
  pinStep(s, 0.1f);  // one Knomi frame: the flipper all the way up
  pinStep(s, 0.1f);
  CHECK(s.y < ty - 17);  // the ball went up with it, not through it
}

TEST(pinball, a_ball_down_the_drain_is_lost_and_the_next_comes_in) {
  PinState s;
  pinStart(s, 9);
  putBall(s, 233, 400, 0, 120);
  uint32_t ev = run(s, 1.0f);
  CHECK(ev & PE_LOST);
  CHECK(s.phase == PIN_LOST);
  ev = run(s, PB_LOST_S + 0.1f);
  CHECK(ev & PE_BALL);
  CHECK(s.phase == PIN_PLAY);
  CHECK_EQ(s.ball, (uint8_t)2);
  CHECK_EQ(s.ballsLeft, (uint8_t)1);
  for (int b = 0; b < 2; b++) {  // the last two lost: the end
    putBall(s, 233, 400, 0, 120);
    ev = run(s, 1.0f + PB_LOST_S + 0.1f);
  }
  CHECK(ev & PE_END);
  CHECK(s.phase == PIN_END);
}

TEST(pinball, bumpers_kick_and_score_and_all_three_bring_up_diglett) {
  PinState s;
  pinStart(s, 11);
  uint32_t ev = 0;
  for (int k = 0; k < 3; k++) {
    putBall(s, PB_BUMP[k][0] - 40, PB_BUMP[k][1], 250, 0);
    uint32_t score = s.score, e = 0;
    for (int i = 0; i < 30 && !(e & PE_BUMP); i++) e |= pinStep(s, DT);
    ev |= e;
    CHECK(e & PE_BUMP);
    CHECK(s.score >= score + PB_PTS_BUMP);
    CHECK(s.vx < 0);  // kicked back
  }
  CHECK(ev & PE_DIG_UP);
  CHECK(s.digUp);
  CHECK_EQ(s.bumpLit, (uint8_t)0);
  uint32_t score = s.score;  // hit it with the ball
  putBall(s, PB_DIG_X - 40, PB_DIG_Y - 18, 260, 0);
  ev = run(s, 0.3f);
  CHECK(ev & PE_DIG_HIT);
  CHECK(!s.digUp);
  CHECK(s.score >= score + PB_PTS_DIG);
  // too slow: it ducks back down by itself
  s.digUp = true, s.digT = 0.2f;
  putBall(s, 60, 300, 0, 0);
  ev = run(s, 0.5f);
  CHECK(ev & PE_DIG_GONE);
  CHECK(!s.digUp);
}

// each Diglett beaten digs a hole beside its own; with both dug, the next one up evolves into Dugtrio: no hit counts
// while it evolves, then three heads to hit; knocked out, the holes fill in and it's Diglett again
static uint32_t hitDig(PinState &s) {
  putBall(s, PB_DIG_X - 50, PB_DIG_Y - 18, 300, 0);
  s.digCool = 0;
  return run(s, 0.3f);
}
TEST(pinball, two_digletts_beaten_and_the_next_evolves_into_dugtrio) {
  PinState s;
  pinStart(s, 21);
  for (int k = 0; k < PB_DIG_EVOLVE; k++) {
    uint32_t ev = pinDigUp(s);
    CHECK(ev & PE_DIG_UP);
    CHECK(!(ev & PE_DIG_EVOLVE));
    CHECK(!s.dug);
    CHECK(hitDig(s) & PE_DIG_HIT);
    CHECK_EQ(s.digBeat, (uint8_t)(k + 1));  // a hole dug beside its own
  }
  uint32_t ev = pinDigUp(s);  // the third: it evolves
  CHECK(ev & PE_DIG_EVOLVE);
  CHECK(s.dug && s.digUp && s.evoT > 0);
  CHECK_EQ(s.dugHeads, (uint8_t)3);
  uint32_t score = s.score;
  ev = hitDig(s);  // evolving: the ball bounces off it, no hit
  CHECK(!(ev & (PE_DUG_HEAD | PE_DIG_HIT)));
  CHECK_EQ(s.dugHeads, (uint8_t)3);
  CHECK(s.digT == PB_DUG_S);  // (its time not running yet)
  ev = run(s, PB_EVO_S);  // evolved
  CHECK(ev & PE_DIG_EVOLVED);
  CHECK(s.evoT <= 0);
  score = s.score;
  CHECK(hitDig(s) & PE_DUG_HEAD);
  CHECK(hitDig(s) & PE_DUG_HEAD);
  CHECK_EQ(s.dugHeads, (uint8_t)1);
  CHECK(s.digUp);
  ev = hitDig(s);  // the third head: knocked out
  CHECK(ev & PE_DIG_HIT);
  CHECK(!s.digUp && !s.dug);
  CHECK_EQ(s.digBeat, (uint8_t)0);  // the holes fill in
  CHECK(s.score >= score + 3 * PB_PTS_DIG + PB_PTS_DUG);
  CHECK(!(pinDigUp(s) & PE_DIG_EVOLVE));  // Diglett again
  CHECK(!s.dug);
}
TEST(pinball, dugtrio_that_got_away_comes_back_without_evolving_again) {
  PinState s;
  pinStart(s, 22);
  s.digBeat = PB_DIG_EVOLVE;
  CHECK(pinDigUp(s) & PE_DIG_EVOLVE);
  uint32_t ev = 0;
  for (float t = 0; t < PB_EVO_S + PB_DUG_S + 0.5f; t += 0.25f) {  // (the ball kept up, out of its way)
    putBall(s, 60, 300, 0, 0);
    ev |= run(s, 0.25f);
  }
  CHECK(ev & PE_DIG_GONE);
  CHECK(!s.digUp && s.dug);
  ev = pinDigUp(s);
  CHECK(!(ev & PE_DIG_EVOLVE));
  CHECK(s.dug && s.evoT <= 0);
  CHECK_EQ(s.dugHeads, (uint8_t)3);
}

// the corners where the wall meets the guides are rounded: a ball rolling down the wall onto a guide keeps its speed
// (with a sharp corner it lost a quarter to three quarters of it)
TEST(pinball, a_ball_rolling_down_the_wall_keeps_its_speed_onto_the_guide) {
  for (int side = 0; side < 2; side++) {
    PinState s;
    pinStart(s, 5);
    float a = (side ? 5 : 175) * 0.01745329f, r = PB_R - PB_BALL_R, v0 = 200;
    putBall(s, PB_CX + r * cosf(a), PB_CY + r * sinf(a), 0, 0);
    float tx = -sinf(a), ty = cosf(a);  // along the wall, down
    if (ty < 0) tx = -tx, ty = -ty;
    s.vx = tx * v0, s.vy = ty * v0;
    float y0 = s.y;
    const PinFillet &f = pinFillet(side);
    float past = f.fx + f.a2x + (side ? -15 : 15);
    for (int i = 0; i < 240 && (side ? s.x > past : s.x < past); i++) pinStep(s, DT);
    float v = hypotf(s.vx, s.vy), free = sqrtf(v0 * v0 + 2 * PB_G * (s.y - y0));
    CHECK_MSG(v > 0.9f * free, "it keeps its speed round the corner");
    CHECK(side ? s.vx < 0 : s.vx > 0);  // on along the guide toward the flipper
  }
}

TEST(pinball, all_six_berries_lit_double_the_points) {
  PinState s;
  pinStart(s, 13);
  for (int k = 0; k < 6; k++) {
    float a = PB_BERRY_A[k] * 0.01745329f;
    putBall(s, PB_CX + PB_WIRE_R * cosf(a), PB_CY + PB_WIRE_R * sinf(a), 0, 0);
    pinStep(s, DT);
  }
  CHECK_EQ(s.mult, (uint8_t)2);
  CHECK_EQ(s.berryLit, (uint8_t)0);
  uint32_t score = s.score;
  pinAdd(s, PB_PTS_BUMP, 0, 0);
  CHECK_EQ(s.score, score + 2 * PB_PTS_BUMP);
}

TEST(pinball, the_ramp_turns_over_the_top_smoothly) {
  const PinTrack &t = pinTrack();
  float worst = 0;  // the sharpest turn between two pieces of it, ramp and wire (the old one turned back on itself)
  for (int i = 1; i < PB_TRACK_N - 1; i++) {
    float ax = t.x[i] - t.x[i - 1], ay = t.y[i] - t.y[i - 1], bx = t.x[i + 1] - t.x[i], by = t.y[i + 1] - t.y[i];
    float c = (ax * bx + ay * by) / (hypotf(ax, ay) * hypotf(bx, by));
    float deg = acosf(c > 1 ? 1 : c < -1 ? -1 : c) * 57.3f;
    worst = deg > worst ? deg : worst;
  }
  CHECK_MSG(worst < 35, "no sharp corner on it");
  for (int i = 0; i < PB_TRACK_N; i++) CHECK(hypotf(t.x[i] - PB_CX, t.y[i] - PB_CY) <= PB_R - PB_BALL_R);
}

TEST(pinball, a_shot_up_the_ramp_goes_over_and_down_the_wire_to_the_left_flipper) {
  PinState s;
  pinStart(s, 15);
  float ux = PB_RAMP_TOP[0] - PB_RAMP_MOUTH[0], uy = PB_RAMP_TOP[1] - PB_RAMP_MOUTH[1], L = hypotf(ux, uy);
  putBall(s, PB_RAMP_MOUTH[0] - ux / L * 10, PB_RAMP_MOUTH[1] - uy / L * 10, ux / L * 480, uy / L * 480);
  uint32_t ev = 0;
  bool onTrack = false;
  for (int i = 0; i < 240 && !(ev & PE_DROP); i++) {
    ev |= pinStep(s, DT);
    onTrack |= s.onTrack;
  }
  CHECK(onTrack);
  CHECK(ev & PE_RAMP_IN);
  CHECK(ev & PE_RAMP);
  CHECK(ev & PE_DROP);
  CHECK(s.score >= PB_PTS_RAMP);
  CHECK(s.x < 120 && s.y > 250);  // let down by the left rim
  ev = run(s, 2.5f);  // ...and it rolls down to the left flipper
  CHECK(s.phase == PIN_LOST || (s.x < 240 && s.y > 330));
  // too slow: it rolls back down and out, no points
  pinStart(s, 16);
  putBall(s, PB_RAMP_MOUTH[0] - ux / L * 10, PB_RAMP_MOUTH[1] - uy / L * 10, ux / L * 200, uy / L * 200);
  ev = run(s, 2.5f);
  CHECK(ev & PE_RAMP_IN);
  CHECK(!(ev & PE_RAMP));
  CHECK(!s.onTrack);
  CHECK_EQ(s.score, (uint32_t)0);
}

TEST(pinball, into_the_grass_a_wild_one_comes_out_to_battle_in_the_ring) {
  PinState s;
  pinStart(s, 17);
  float nx = PB_NOTCH[1][0] - PB_NOTCH[0][0], ny = PB_NOTCH[1][1] - PB_NOTCH[0][1], L = hypotf(nx, ny);
  putBall(s, PB_NOTCH[0][0] - nx / L * 24, PB_NOTCH[0][1] - ny / L * 24, nx / L * 250, ny / L * 250);
  uint32_t ev = run(s, 0.3f);
  CHECK(ev & PE_GRASS);
  CHECK(s.phase == PIN_CATCH);
  ev = run(s, PB_CATCH_S + 0.05f);
  CHECK(ev & PE_BATTLE);
  CHECK(s.phase == PIN_PLAY);  // the ball's back out, dropped onto the right flipper
  CHECK(s.vx < 0);
  pinBattleStart(s, 43, 30, 12, 0);
  CHECK(s.btlOn && !pinWildThere(s));  // jumping into the ring
  run(s, PB_BTL_IN_S + 0.05f);
  CHECK(pinWildThere(s));
  // three hits of the ball knock it out (12 a hit, 30 HP)
  uint32_t score = s.score;
  for (int k = 0; k < 3; k++) {
    putBall(s, PB_RING_X - 50, PB_RING_Y - 22, 300, 0);
    ev = run(s, 0.25f);
    CHECK(ev & PE_BTL_HIT);
  }
  CHECK(ev & PE_BTL_WON);
  CHECK(s.won && s.wildHp == 0);
  CHECK(s.score >= score + 3 * PB_PTS_BTL_HIT + PB_PTS_BATTLE);
  run(s, PB_BTL_OUT_S + 0.1f);
  CHECK(!s.btlOn);  // gone from the ring
  // not knocked out in time: it goes back, no points (a lost ball ends it too)
  pinBattleStart(s, 43, 500, 5, 0);
  s.inT = 0, s.btlLeft = 0.5f;  // (its last half second)
  putBall(s, 120, 120, 0, 0);
  score = s.score;
  ev = run(s, 0.8f);
  CHECK(ev & PE_BTL_GONE);
  CHECK(!s.won);
  pinBattleStart(s, 43, 500, 5, 0);
  putBall(s, 233, 400, 0, 120);
  ev = run(s, 1.0f);
  CHECK((ev & PE_LOST) && !s.btlOn);
  // while one's out, the grass doesn't take the ball
  pinBattleStart(s, 43, 500, 5, 0);
  putBall(s, PB_NOTCH[0][0] - nx / L * 24, PB_NOTCH[0][1] - ny / L * 24, nx / L * 250, ny / L * 250);
  ev = run(s, 0.3f);
  CHECK(!(ev & PE_GRASS));
}

TEST(pinball, a_stuck_ball_is_nudged_free) {
  PinState s;
  pinStart(s, 21);
  // balanced on the very top of the Ultra Ball bumper, still: after a while a nudge frees it
  putBall(s, PB_BUMP[2][0], PB_BUMP[2][1] - PB_BUMP_R - PB_BALL_R - 0.5f, 0, 0);
  uint32_t ev = run(s, 3.0f);
  CHECK(ev & PE_NUDGE);
}

// a simple player: flicks a flipper (up for a quarter second) when the ball is coming down near it
static void botFlip(PinState &s, float *held) {
  for (int k = 0; k < 2; k++) {
    float px = k ? 465 - PB_FLIP_LX : PB_FLIP_LX, mid = px + (k ? -35 : 35);
    float out = k ? px - s.x : s.x - px;  // how far out along the flipper the ball is (not in the crook at its pivot)
    bool near = s.phase == PIN_PLAY && !s.onTrack && out > 22 && out < 80 && s.y > 340 && s.y < PB_FLIP_Y + 30;
    (void)mid;
    if (held[k] > 0) held[k] += DT;
    if (held[k] > 0.25f) held[k] = -0.2f;  // let go, and a moment before the next flick
    else if (held[k] < 0) held[k] = held[k] + DT >= 0 ? 0 : held[k] + DT;
    else if (near) held[k] = pinRand(s) < 0.3f ? -0.4f : DT;  // (a third of the time it misses, as a person would)
    pinHold(s, k, held[k] > 0);
  }
}

TEST(pinball, whole_games_end_and_the_ball_never_leaves_the_table) {
  int ramps = 0, orbits = 0, bumps = 0, grass = 0, hits = 0;
  float longest = 0;
  for (uint32_t seed = 1; seed <= 30; seed++) {
    PinState s;
    pinStart(s, seed * 7919);
    float t = 0, held[2] = { 0, 0 };
    for (; t < 900 && s.phase != PIN_END; t += DT) {
      botFlip(s, held);
      uint32_t ev = pinStep(s, DT);
      if (ev & PE_BATTLE) pinBattleStart(s, 43, 40, 12, 0);
      ramps += (ev & PE_RAMP) != 0, orbits += (ev & PE_ORBIT) != 0, bumps += (ev & PE_BUMP) != 0, grass += (ev & PE_GRASS) != 0;
      hits += (ev & PE_BTL_HIT) != 0;
      if (s.phase == PIN_PLAY) {
        CHECK(s.x == s.x && s.y == s.y);  // no NaN
        CHECK(hypotf(s.x - PB_CX, s.y - PB_CY) <= PB_R + 1 || s.y > PB_FLIP_Y);
      }
    }
    CHECK_MSG(s.phase == PIN_END, "a game ends");
    longest = t > longest ? t : longest;
  }
  printf("    30 bot games: %d ramps, %d orbits, %d bumper hits, %d into the grass, %d hits in the ring, longest %.0f s\n", ramps,
         orbits, bumps, grass, hits, longest);
  CHECK(bumps > 0);
}
