#pragma once
#include <stdint.h>
#include <string.h>

// The Wi-Fi networks a Knomi remembers (Fedor, 2026-11-18: a Knomi carried between home and work), the most
// recent first. Every network it joins goes to the front; a sixth pushes the oldest out. At each try it joins
// the one it hears loudest, or the most recent when it hears none (a hidden network, or a scan that missed
// it). Saved by rtcbat.cpp as one blob ("wifinets" "list"); the logic here is plain so the tests can run it.

#define WIFI_KNOWN_MAX 5

struct WifiKnown {
  char ssid[33];
  char pass[65];
};

// where ssid is in the list, or -1
inline int wifiKnownFind(const WifiKnown *list, int n, const char *ssid) {
  for (int i = 0; i < n; i++)
    if (!strcmp(list[i].ssid, ssid)) return i;
  return -1;
}

// puts ssid at the front with this password (a network it already knew moves up, its password replaced);
// returns the new count
inline int wifiKnownAdd(WifiKnown *list, int n, const char *ssid, const char *pass) {
  if (!ssid || !ssid[0] || strlen(ssid) > 32 || strlen(pass ? pass : "") > 64) return n;
  WifiKnown e;  // copied first: ssid and pass may point into the list itself
  memset(&e, 0, sizeof(e));
  strncpy(e.ssid, ssid, 32);
  strncpy(e.pass, pass ? pass : "", 64);
  int at = wifiKnownFind(list, n, e.ssid);
  if (at < 0) at = n < WIFI_KNOWN_MAX ? n++ : WIFI_KNOWN_MAX - 1;  // a new one: one more, or in the oldest's place
  for (int i = at; i > 0; i--) list[i] = list[i - 1];
  list[0] = e;
  return n;
}

// which to try: of the known networks among those heard (names and signal), the loudest, passing over
// `avoid` (the one that just failed) when another is heard; if none is heard, the most recent. -1 if it knows none
inline int wifiKnownPick(const WifiKnown *list, int n, const char *const *heard, const int *rssi, int heardN,
                         const char *avoid) {
  int best = -1, bestRssi = -1000, other = -1, otherRssi = -1000;
  for (int h = 0; h < heardN; h++) {
    int k = wifiKnownFind(list, n, heard[h]);
    if (k < 0) continue;
    if (rssi[h] > bestRssi) bestRssi = rssi[h], best = k;
    if ((!avoid || strcmp(list[k].ssid, avoid)) && rssi[h] > otherRssi) otherRssi = rssi[h], other = k;
  }
  if (other >= 0) return other;
  if (best >= 0) return best;
  return n > 0 ? 0 : -1;
}
