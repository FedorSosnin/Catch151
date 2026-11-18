#pragma once
#include <math.h>
#include <stdint.h>

// Press preview, like the Wi-Fi keyboard: a finger held still on a control (PRESS_HOLD_MS) gets it outlined and a
// large copy floating beside the finger (which covers the real one); from then on sliding to a neighbour moves it,
// and the lift chooses. A control the finger lands on acts at once instead (handleTouch()'s tapDownTry()), so the
// preview is for a finger that slid onto one, and for the buddies. Made for big fingers on a 1.28" screen. Only
// geometry here (tested on the PC).
#define PRESS_GAP 56         // a fingertip covers ~50 logical px: the copy goes beyond it
#define PRESS_EDGE 20        // margin from the round edge
#define PRESS_FLICK_MS 350   // faster than this and longer than PRESS_FLICK_PX: still a swipe
#define PRESS_FLICK_PX 80
#define PRESS_SOFT_MS 150    // a soft target (a buddy) previews only once the finger has rested this long...
#define PRESS_SOFT_PX 30     // ...within this of where it landed; farther and the touch is a swipe
#define PRESS_HOLD_MS 450    // the preview: a finger still on a control this long (the radial menu's hold)...
#define PRESS_HOLD_PX 14     // ...moving no more than this (a slide across controls shows nothing)

// the control under the finger
struct PressTarget {
  int16_t x, y, w, h;       // the drawn control, to outline it
  const char *label;        // the large copy: text...
  const char *const *icon;  // ...or a 16x16 icon
  bool soft = false;        // a big target lying where swipes start (the buddies): gives way to them
};

struct PressBox {
  int16_t x, y, w, h;
};

// where the w x h copy goes for a finger at (fx, fy): above it, below when there is no room
// at the top, and always inside the round screen (center c, radius r)
inline PressBox pressPopupBox(int16_t fx, int16_t fy, int16_t w, int16_t h, int16_t c = 233, int16_t r = 231) {
  const int16_t top = c - r + PRESS_EDGE, bottom = c + r - PRESS_EDGE;
  int16_t y = fy - PRESS_GAP - h;
  if (y < top) y = fy + PRESS_GAP;  // finger near the top: below it
  if (y + h > bottom) y = bottom - h;
  if (y < top) y = top;
  // the corners must stay inside the circle: the row farthest from the center limits it
  int32_t dy = y + h - c > c - y ? y + h - c : c - y;
  int32_t lim = (int32_t)r - PRESS_EDGE;
  int16_t half = dy < lim ? (int16_t)sqrtf((float)(lim * lim - dy * dy)) : 0;
  int16_t x = fx - w / 2;
  if (w > 2 * half) x = c - w / 2;  // wider than the chord: centered is the best it can do
  else if (x < c - half) x = c - half;
  else if (x + w > c + half) x = c + half - w;
  return { x, y, w, h };
}

// a soft target, for a finger dx, dy from where it landed after dt ms: true once it has moved off
// (the touch is a swipe from then on), and *show says whether to preview it yet
inline bool pressSoftMoved(int dx, int dy) {
  return dx > PRESS_SOFT_PX || dx < -PRESS_SOFT_PX || dy > PRESS_SOFT_PX || dy < -PRESS_SOFT_PX;
}
inline bool pressSoftShows(int dx, int dy, uint32_t dt) { return dt >= PRESS_SOFT_MS && !pressSoftMoved(dx, dy); }

// the finger has moved off the spot where it came onto a control: its hold starts over
inline bool pressHoldMoved(int dx, int dy) {
  return dx > PRESS_HOLD_PX || dx < -PRESS_HOLD_PX || dy > PRESS_HOLD_PX || dy < -PRESS_HOLD_PX;
}
// a quick, long flick is still a swipe (pages, swipe up to exit) even if it ends on a control
inline bool pressIsFlick(int dx, int dy, uint32_t dt) {
  return dt < PRESS_FLICK_MS && (int32_t)dx * dx + (int32_t)dy * dy > PRESS_FLICK_PX * PRESS_FLICK_PX;
}
