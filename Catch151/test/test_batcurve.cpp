#include "framework.h"
#include "../src/batcurve.h"

TEST(batcurve, extremes) {
  CHECK_EQ(batPercentFromMv(4300), 100);
  CHECK_EQ(batPercentFromMv(4200), 100);
  CHECK_EQ(batPercentFromMv(BAT_CUTOFF_MV), 0);
  CHECK_EQ(batPercentFromMv(3000), 0);
  CHECK_EQ(batPercentFromMv(3840), 50);
}

TEST(batcurve, monotonic_and_covers_full_range) {
  int prev = 100;
  for (int mv = 4200; mv >= BAT_CUTOFF_MV; mv--) {
    int p = batPercentFromMv(mv);
    CHECK(p <= prev);
    CHECK(p >= 0 && p <= 100);
    prev = p;
  }
  CHECK_EQ(batPercentFromMv(3660), 8);  // between (3610, 5) and (3690, 10)
}

// readings every 2 s from t0 at the same voltage; returns whether a shutdown is needed at the end
static bool lowFor(BatMonitor &m, int mv, uint32_t t0, uint32_t ms) {
  bool r = false;
  for (uint32_t t = t0; t <= t0 + ms; t += 2000) { m.sample(mv, t); r = m.shutdownNeeded(t); }
  return r;
}

TEST(batcurve, shutdown_requires_sustained_low_voltage) {
  BatMonitor m;
  CHECK(!lowFor(m, 3250, 100000, 20000));  // 20 s is not enough
  CHECK(lowFor(m, 3250, 120000, 12000));   // 30+ s in a row is
}

TEST(batcurve, load_dip_does_not_shut_down) {
  BatMonitor m;
  for (uint32_t t = 100000; t < 130000; t += 2000) m.sample(3700, t);
  // a long load (sound, radio) sinks the voltage below the limit for 20 s and then it recovers
  CHECK(!lowFor(m, 3200, 130000, 20000));
  m.sample(3700, 152000); m.sample(3700, 154000); m.sample(3700, 156000); m.sample(3700, 158000);
  CHECK(!m.shutdownNeeded(160000));
}

TEST(batcurve, no_shutdown_right_after_boot) {
  BatMonitor m;
  for (uint32_t t = 0; t <= 20000; t += 2000) m.sample(3200, t);
  CHECK(!m.shutdownNeeded(20000));
  CHECK(!m.shutdownNeeded(BAT_BOOT_GRACE_MS - 1));
  for (uint32_t t = 22000; t <= 60000; t += 2000) m.sample(3200, t);
  CHECK(m.shutdownNeeded(60000));
}

TEST(batcurve, failed_reading_resets_the_count) {
  BatMonitor m;
  lowFor(m, 3250, 100000, 24000);
  CHECK_EQ(m.sample(0, 126000), -1);
  CHECK(!m.shutdownNeeded(128000));
  CHECK(!lowFor(m, 3250, 128000, 20000));
}

TEST(batcurve, percentage_only_drops_while_discharging) {
  BatMonitor m;
  m.seed(60);                              // the PMU said 60 when unplugged
  CHECK_EQ(m.sample(4100, 1000), 60);      // voltage inflated by charging does not raise it
  int a = 60;
  for (uint32_t t = 3000; t < 23000; t += 2000) a = m.sample(3700, t);  // the voltage really drops
  CHECK(a < 60 && a > 10);
  int b = m.sample(4000, 5000);            // a spike does not raise it either
  CHECK_EQ(b, a);
  CHECK_EQ(m.sample(3840, 7000) <= a, true);
}

TEST(batcurve, without_seed_starts_on_the_curve) {
  BatMonitor m;
  CHECK_EQ(m.sample(3840, 1000), 50);
  m.reset();
  CHECK_EQ(m.pct, -1);
}

// Fedor's Waveshare 2.1 on 2026-11-23, twice a second (console BATLOG): on USB, unplugged at reading 135, plugged
// back in at 206 (a 2000 mAh cell at about 9%, charging at about 2 A)
static const int16_t PLUG_LOG[240] = {
  3692, 3695, 3692, 3692, 3689, 3689, 3689, 3695, 3689, 3689, 3692, 3695, 3692, 3692, 3689, 3689,
  3692, 3689, 3692, 3689, 3689, 3689, 3692, 3695, 3689, 3689, 3689, 3692, 3689, 3692, 3695, 3695,
  3692, 3692, 3686, 3698, 3695, 3695, 3689, 3695, 3692, 3695, 3698, 3695, 3695, 3695, 3692, 3695,
  3695, 3692, 3695, 3698, 3695, 3695, 3692, 3692, 3695, 3692, 3698, 3692, 3692, 3689, 3692, 3695,
  3698, 3695, 3695, 3695, 3698, 3695, 3698, 3695, 3698, 3689, 3692, 3695, 3695, 3698, 3692, 3695,
  3692, 3695, 3698, 3692, 3695, 3701, 3689, 3692, 3695, 3695, 3698, 3692, 3689, 3692, 3692, 3686,
  3692, 3689, 3695, 3698, 3695, 3692, 3698, 3683, 3698, 3689, 3692, 3692, 3701, 3692, 3704, 3698,
  3695, 3698, 3695, 3707, 3695, 3698, 3695, 3698, 3692, 3698, 3698, 3698, 3695, 3695, 3698, 3695,
  3698, 3701, 3695, 3698, 3695, 3695, 3683, 3592, 3580, 3583, 3577, 3583, 3583, 3571, 3577, 3577,
  3577, 3574, 3577, 3574, 3577, 3574, 3577, 3577, 3574, 3571, 3574, 3571, 3574, 3574, 3580, 3574,
  3571, 3574, 3571, 3571, 3568, 3571, 3571, 3574, 3571, 3574, 3571, 3571, 3571, 3571, 3568, 3565,
  3568, 3568, 3568, 3571, 3571, 3568, 3568, 3568, 3565, 3568, 3568, 3568, 3568, 3568, 3565, 3574,
  3568, 3568, 3568, 3568, 3568, 3562, 3568, 3571, 3568, 3571, 3568, 3574, 3565, 3589, 3680, 3680,
  3683, 3689, 3686, 3686, 3686, 3692, 3686, 3686, 3689, 3683, 3686, 3689, 3689, 3686, 3692, 3689,
  3695, 3689, 3689, 3689, 3686, 3689, 3686, 3686, 3689, 3692, 3689, 3689, 3689, 3689, 3689, 3686
};

TEST(batcurve, plug_steps_in_the_real_log) {
  BatPlugDetect d;
  d.charging = true;  // as if the plugging in had been seen
  uint32_t t = 0;
  for (int i = 0; i < 240; i++, t += 500) {
    bool c = d.sample(PLUG_LOG[i], t);
    if (i < 135) CHECK(c);                // on USB
    if (i >= 140 && i < 206) CHECK(!c);   // unplugged: seen within 2.5 s
    if (i >= 211) CHECK(c);               // plugged back in: seen within 2.5 s
  }
}

TEST(batcurve, plug_ignores_noise_and_small_load_steps) {
  BatPlugDetect d;
  uint32_t t = 0;
  for (int i = 0; i < 400; i++, t += 500) {
    int mv = 3800 + ((i * 7) % 25) - 12;  // +-12 mV of noise
    if (i >= 100 && i < 200) mv -= 30;    // a load (the amplifier, Wi-Fi) taking 30 mV for 50 s
    if (i >= 300) mv += 25;
    CHECK(!d.sample(mv, t));
  }
  d.charging = true;
  for (int i = 0; i < 100; i++, t += 500) CHECK(d.sample(3800 - (i >= 50 ? 30 : 0), t));
}

TEST(batcurve, plug_counts_as_charged_after_a_while_at_the_top) {
  BatPlugDetect d;
  uint32_t t = 0;
  for (int i = 0; i < 20; i++, t += 500) d.sample(4100, t);
  for (int i = 0; i < 20; i++, t += 500) d.sample(4200, t);  // plugged in near the top
  CHECK(d.charging);
  uint32_t t0 = t;
  for (; t - t0 < BAT_FULL_MS - 10000; t += 500) d.sample(4200, t);
  CHECK(d.charging);
  for (; t - t0 < BAT_FULL_MS + 10000; t += 500) d.sample(4200, t);
  CHECK(!d.charging);
  for (int i = 0; i < 20; i++, t += 500) d.sample(4180, t);  // unplugged: a small fall, nothing to change
  CHECK(!d.charging);
}

TEST(batcurve, plug_no_cell) {
  BatPlugDetect d;
  d.charging = true;
  CHECK(!d.sample(0, 0));
  CHECK_EQ(d.n, 0);
}

// The same board started on USB (just flashed), every 2 s for 4 minutes: charging, so no step to see
static const int16_t USB_CLIMB[] = {
  3716, 3719, 3722, 3719, 3722, 3716, 3725, 3728, 3719, 3722, 3728, 3725, 3728, 3731, 3731, 3725, 3722, 3722, 
  3725, 3725, 3725, 3725, 3725, 3728, 3731, 3725, 3725, 3728, 3740, 3734, 3731, 3725, 3725, 3725, 3719, 3725, 
  3719, 3734, 3734, 3728, 3734, 3728, 3731, 3728, 3731, 3722, 3731, 3728, 3731, 3731, 3731, 3728, 3728, 3734, 
  3731, 3734, 3731, 3734, 3734, 3728, 3737, 3728, 3734, 3734, 3728, 3731, 3731, 3731, 3728, 3731, 3734, 3734, 
  3728, 3731, 3734, 3725, 3731, 3740, 3737, 3728, 3731, 3731, 3737, 3743, 3731, 3731, 3737, 3734, 3740, 3740, 
  3731, 3737, 3734, 3743, 3731, 3737, 3743, 3743, 3740, 3728, 3743, 3740, 3731, 3743, 3743, 3749, 3746, 3749, 
  3740, 3743, 3749, 3746, 3734, 3746, 3743, 3734, 3743, 3743, 3743, 3749, 3749, 3746, 3749, 3737, 3746, 3740, 
  3743, 3731
};

TEST(batcurve, plug_climb_after_a_start_on_usb) {
  BatPlugDetect d;
  uint32_t t = 0;
  int n = sizeof(USB_CLIMB) / sizeof(USB_CLIMB[0]);
  for (int i = 0; i < n; i++)
    for (int k = 0; k < 4; k++, t += 500) {  // its readings were 2 s apart: four of ours each
      d.sample(USB_CLIMB[i], t);
      if (t < 3UL * 60 * 1000) CHECK(!d.charging);  // fewer than four minutes: not yet
    }
  CHECK(d.charging);  // after four
}

TEST(batcurve, plug_no_climb_on_the_cell_alone) {
  BatPlugDetect d;
  uint32_t t = 0;
  for (int i = 0; i < 20 * 120; i++, t += 500) {
    int mv = 3800 - i / 80 + ((i * 7) % 25) - 12;  // falling 1.5 mV a minute, with noise
    if (i >= 5 * 120) mv += 9;                     // a load stops: up once,
    if (i >= 5 * 120) mv += (i - 5 * 120) / 120;   // then a slow recovery of 1 mV a minute
    CHECK(!d.sample(mv, t));
  }
}

TEST(batcurve, plug_percentage_holds_across_plugging) {
  BatPlugDetect d;
  d.charging = true;  // as if the plugging in had been seen, with no lift known
  uint32_t t = 0;
  int unplugged = -1, last = -1;
  for (int i = 0; i < 240; i++, t += 500) {
    d.sample(PLUG_LOG[i], t);
    int p = d.percent(PLUG_LOG[i], t);
    if (i == 200) unplugged = p;  // on the cell: about 3%
    if (i >= 211) {
      CHECK(p >= unplugged - 1 && p <= unplugged + 2);  // plugged back in: no jump (it was 3% to 10% before)
      CHECK(p >= last);                                 // and only up while charging
    }
    last = p;
  }
  CHECK(d.lift >= 80 && d.lift <= 130);
}

TEST(batcurve, plug_percentage_only_down_on_the_cell) {
  BatPlugDetect d;
  uint32_t t = 0;
  int last = 101;
  for (int i = 0; i < 400; i++, t += 500) {
    int mv = 3850 - i / 40 + (i >= 200 && i < 260 ? 12 : 0);  // a load stopping for half a minute
    d.sample(mv, t);
    int p = d.percent(mv, t);
    CHECK(p <= last);
    last = p;
  }
}

TEST(batcurve, plug_percentage_reaches_100_at_the_top) {
  BatPlugDetect d;
  uint32_t t = 0;
  for (int i = 0; i < 20; i++, t += 500) d.sample(4090, t);
  for (int i = 0; i < 20; i++, t += 500) d.sample(4200, t);  // plugged in: a 110 mV lift
  CHECK(d.charging);
  CHECK_EQ(d.lift, 110);
  int first = d.percent(4200, t), last = first;
  CHECK(first < 95);
  uint32_t t0 = t;
  for (; t - t0 < BAT_FULL_MS - 60000; t += 500) {
    d.sample(4200, t);
    int p = d.percent(4200, t);
    CHECK(p >= last);
    last = p;
  }
  CHECK(last >= 98);
  for (; t - t0 < BAT_FULL_MS + 60000; t += 500) d.sample(4200, t);
  CHECK(!d.charging);
  CHECK_EQ(d.percent(4200, t), 100);
}
