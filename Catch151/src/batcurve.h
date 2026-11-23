#pragma once
#include <stdint.h>

// Single-cell LiPo/Li-ion battery (charged to 4.2 V, as the Waveshare example configures): from
// resting voltage to percentage, and when to shut down so as not to discharge it deeply. Pure logic, with
// tests in test/test_batcurve.cpp.

#define BAT_CUTOFF_MV 3300     // below this (while discharging) the firmware saves and shuts down
#define BAT_CUTOFF_READS 3     // consecutive readings below (besides the time, see BatMonitor)
#define BAT_HW_CUTOFF_MV 3100  // backup in the PMU (AXP2101, 2600 to 3300 in steps of 100)

// typical LiPo cell discharge curve, (mV, %) from highest to lowest
struct BatPoint { int16_t mv; uint8_t pct; };
static const BatPoint BAT_CURVE[] = {
  { 4200, 100 }, { 4150, 95 }, { 4110, 90 }, { 4080, 85 }, { 4020, 80 }, { 3980, 75 }, { 3950, 70 },
  { 3910, 65 }, { 3870, 60 }, { 3850, 55 }, { 3840, 50 }, { 3820, 45 }, { 3800, 40 }, { 3790, 35 },
  { 3770, 30 }, { 3750, 25 }, { 3730, 20 }, { 3710, 15 }, { 3690, 10 }, { 3610, 5 }, { BAT_CUTOFF_MV, 0 },
};

// linear interpolation between points; 100 above 4.2 V and 0 at shutdown
inline int batPercentFromMv(int mv) {
  const int n = sizeof(BAT_CURVE) / sizeof(BAT_CURVE[0]);
  if (mv >= BAT_CURVE[0].mv) return 100;
  if (mv <= BAT_CURVE[n - 1].mv) return 0;
  for (int i = 1; i < n; i++) {
    if (mv >= BAT_CURVE[i].mv) {
      int dmv = BAT_CURVE[i - 1].mv - BAT_CURVE[i].mv, dp = BAT_CURVE[i - 1].pct - BAT_CURVE[i].pct;
      return BAT_CURVE[i].pct + (mv - BAT_CURVE[i].mv) * dp / dmv;
    }
  }
  return 0;
}

// Smooths the voltage (exponential moving average, 1/4 per reading), makes the displayed percentage only
// go down while discharging lasts (the charge spike right after unplugging, or a dip from load, neither
// raises it nor makes it dance) and decides shutdown: the smoothed voltage must be below the limit
// for BAT_CUTOFF_MS in a row (and at least BAT_CUTOFF_READS readings), not just a few readings.
#define BAT_CUTOFF_MS 30000UL
#define BAT_BOOT_GRACE_MS 30000UL  // no shutdown after boot: the first voltage doesn't say anything yet

struct BatMonitor {
  int smoothMv = 0;
  int pct = -1;            // last percentage shown (-1 = no reading yet)
  uint32_t lowSince = 0;
  uint8_t lowReads = 0;

  void reset() { smoothMv = 0; pct = -1; lowSince = 0; lowReads = 0; }
  // upper limit of the percentage when discharge starts (the one the PMU was showing)
  void seed(int p) { if (pct < 0 && p >= 0) pct = p; }
  // One reading while discharging (no USB or charging). Returns the percentage, -1 if there was no reading.
  int sample(int mv, uint32_t now) {
    if (mv <= 0) { lowReads = 0; lowSince = 0; return -1; }
    smoothMv = smoothMv ? smoothMv + (mv - smoothMv) / 4 : mv;
    if (smoothMv < BAT_CUTOFF_MV) {
      if (!lowReads) lowSince = now ? now : 1;
      if (lowReads < 255) lowReads++;
    } else {
      lowReads = 0;
      lowSince = 0;
    }
    int c = batPercentFromMv(smoothMv);
    if (pct < 0 || c < pct) pct = c;
    return pct;
  }
  bool shutdownNeeded(uint32_t now) const {
    return now >= BAT_BOOT_GRACE_MS && lowReads >= BAT_CUTOFF_READS && now - lowSince >= BAT_CUTOFF_MS;
  }
};

// The Waveshare 2.1 can't tell whether USB is in: it has no PMU and no sense line (docs/WAVESHARE_21.md). But the
// cell's voltage jumps when the charger starts or stops. On Fedor's board (a 2000 mAh cell, charged at about 2 A) it
// rose about 100 mV within half a second of plugging in, and fell about 115 mV on unplugging. The board's own loads
// move it 10 mV or less (2026-11-23, test/test_batcurve.cpp has the log). So: a reading twice a second, and the
// middle of the last 2 seconds against the middle of 2 seconds a little before. A rise of BAT_STEP_MV or more is
// "plugged in", a fall as big "unplugged". Near the top the charger holds the cell at 4.2 V and the current fades,
// so there's no clear step at the end: after BAT_FULL_MS up there it counts as charged (the Knomi's PMU stops saying
// "charging" then too).
// A board started while plugged in sees no step. For that, the slow climb: on the bench board the one-minute
// averages went 3725, 3729, 3733, 3741 mV while charging at about 20%, and on the cell alone they only fall. A load
// that stops (the music, Wi-Fi) lifts them once and then they sit still. So four one-minute averages, each at least
// BAT_CLIMB_STEP above the one before and BAT_CLIMB_MV or more in all, also mean "charging". Not caught that way:
// a start near the top, where the charger holds the voltage still, and maybe the flattest middle of the curve.
#define BAT_STEP_MV 40                    // the 2 A step is 100; a 0.5 A charge (R7 swapped, see the doc) about 40
#define BAT_FULL_MV 4150                  // the top, where the charger holds it
#define BAT_FULL_MS (30UL * 60UL * 1000UL)
#define BAT_PLUG_WIN 12                   // readings kept (6 s): the oldest 4 against the newest 4
#define BAT_MIN_READS 120                 // a minute of readings
#define BAT_CLIMB_N 4                     // one-minute averages compared
#define BAT_CLIMB_STEP 2                  // mV, each over the one before
#define BAT_CLIMB_MV 10                   // mV, the last over the first
// The percentage, which comes from the voltage, would jump up by the same lift on plugging in (Fedor: 8% on the
// cell, 37% to 44% plugged in). So while charging, the lift is taken off: the step seen on plugging in, or
// BAT_LIFT_MV when it was found by the climb. At the top it fades over BAT_FULL_MS, as the charge current does.
// And as on the Knomi, the number only goes down on the cell and only up while charging, so a load coming and going
// doesn't make it dance.
#define BAT_LIFT_MV 100                   // the steps seen on the bench board: 91 to 115 mV

struct BatPlugDetect {
  int16_t mv[BAT_PLUG_WIN] = {};
  uint8_t n = 0;
  bool charging = false;
  uint32_t topSince = 0;
  int32_t minSum = 0;                     // this minute's readings
  uint8_t minN = 0, avgN = 0;
  int16_t avg[BAT_CLIMB_N] = {};          // the last minutes' averages, oldest first
  int16_t lift = 0;                       // while charging: how far the charger lifts the reading
  int8_t shown = -1;                      // the percentage last given (-1: none yet)

  void forgetClimb() { minSum = 0; minN = avgN = 0; }
  bool climbing() const {
    if (avgN < BAT_CLIMB_N) return false;
    for (int i = 1; i < BAT_CLIMB_N; i++)
      if (avg[i] - avg[i - 1] < BAT_CLIMB_STEP) return false;
    return avg[BAT_CLIMB_N - 1] - avg[0] >= BAT_CLIMB_MV;
  }

  static int mid4(const int16_t *v) {  // the middle two of four, averaged
    int a[4] = { v[0], v[1], v[2], v[3] };
    for (int i = 1; i < 4; i++)
      for (int j = i; j > 0 && a[j] < a[j - 1]; j--) { int t = a[j]; a[j] = a[j - 1]; a[j - 1] = t; }
    return (a[1] + a[2]) / 2;
  }
  // the percentage to show for this (smoothed) voltage
  int percent(int mvNow, uint32_t now) {
    int l = 0;
    if (charging) {
      l = lift;
      if (topSince) {
        uint32_t e = now - topSince;
        l = e >= BAT_FULL_MS ? 0 : (int)((int64_t)lift * (int64_t)(BAT_FULL_MS - e) / (int64_t)BAT_FULL_MS);
      }
    }
    int c = batPercentFromMv(mvNow - l);
    if (shown < 0 || (charging ? c > shown : c < shown)) shown = (int8_t)c;
    return shown;
  }
  // one reading, every 500 ms; returns whether it's charging
  bool sample(int mvNow, uint32_t now) {
    bool was = charging;
    sampleIn(mvNow, now);
    if (charging != was) shown = -1;  // the number starts again from the next reading, the other way round
    return charging;
  }
  void sampleIn(int mvNow, uint32_t now) {
    if (mvNow < 2500) { n = 0; charging = false; topSince = 0; forgetClimb(); return; }  // no cell
    minSum += mvNow;
    if (++minN == BAT_MIN_READS) {
      if (avgN == BAT_CLIMB_N) {
        for (int i = 1; i < BAT_CLIMB_N; i++) avg[i - 1] = avg[i];
        avgN--;
      }
      avg[avgN++] = (int16_t)(minSum / BAT_MIN_READS);
      minSum = 0;
      minN = 0;
      if (!charging && climbing()) { charging = true; topSince = 0; lift = BAT_LIFT_MV; }
    }
    if (n == BAT_PLUG_WIN) {
      for (int i = 1; i < BAT_PLUG_WIN; i++) mv[i - 1] = mv[i];
      n--;
    }
    mv[n++] = (int16_t)mvNow;
    if (n < 4) return;
    int recent = mid4(mv + n - 4);
    if (n == BAT_PLUG_WIN) {
      int before = mid4(mv);
      if (recent - before >= BAT_STEP_MV) {
        if (!charging) { charging = true; topSince = 0; lift = 0; }
        if (recent - before > lift) {  // it's seen half-way through the step: the lift grows for a second or two
          lift = (int16_t)(recent - before);
          shown = -1;
        }
        forgetClimb();  // (its minutes straddle the step)
      } else if (before - recent >= BAT_STEP_MV) {
        charging = false;
        topSince = 0;
        forgetClimb();
      }
    }
    if (charging && recent >= BAT_FULL_MV) {
      if (!topSince) topSince = now ? now : 1;
      else if (now - topSince >= BAT_FULL_MS) { charging = false; topSince = 0; forgetClimb(); }
    } else topSince = 0;
  }
};
