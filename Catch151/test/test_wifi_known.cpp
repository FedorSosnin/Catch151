// The Wi-Fi networks a Knomi remembers (wifi_known.h): the most recent first, up to five, and the loudest heard
// is the one it joins.
#include "framework.h"
#include "../src/wifi_known.h"

TEST(wifi_known, new_networks_go_to_the_front) {
  WifiKnown l[WIFI_KNOWN_MAX] = {};
  int n = 0;
  n = wifiKnownAdd(l, n, "SOS", "home-pass");
  n = wifiKnownAdd(l, n, "DISRUPTIVE INC", "work-pass");
  CHECK_EQ(n, 2);
  CHECK(!strcmp(l[0].ssid, "DISRUPTIVE INC"));
  CHECK(!strcmp(l[0].pass, "work-pass"));
  CHECK(!strcmp(l[1].ssid, "SOS"));
  CHECK(!strcmp(l[1].pass, "home-pass"));
}

TEST(wifi_known, joining_a_known_one_moves_it_up_with_its_new_password) {
  WifiKnown l[WIFI_KNOWN_MAX] = {};
  int n = 0;
  n = wifiKnownAdd(l, n, "A", "aaaaaaaa");
  n = wifiKnownAdd(l, n, "B", "bbbbbbbb");
  n = wifiKnownAdd(l, n, "C", "cccccccc");
  n = wifiKnownAdd(l, n, "A", "a-new-one");
  CHECK_EQ(n, 3);
  CHECK(!strcmp(l[0].ssid, "A"));
  CHECK(!strcmp(l[0].pass, "a-new-one"));
  CHECK(!strcmp(l[1].ssid, "C"));
  CHECK(!strcmp(l[2].ssid, "B"));
}

TEST(wifi_known, moving_one_up_from_inside_the_list) {  // (rtcbat.cpp passes the list's own strings)
  WifiKnown l[WIFI_KNOWN_MAX] = {};
  int n = 0;
  n = wifiKnownAdd(l, n, "SOS", "home-pass");
  n = wifiKnownAdd(l, n, "DISRUPTIVE INC", "work-pass");
  n = wifiKnownAdd(l, n, l[1].ssid, l[1].pass);
  CHECK_EQ(n, 2);
  CHECK(!strcmp(l[0].ssid, "SOS"));
  CHECK(!strcmp(l[0].pass, "home-pass"));
  CHECK(!strcmp(l[1].ssid, "DISRUPTIVE INC"));
  CHECK(!strcmp(l[1].pass, "work-pass"));
}

TEST(wifi_known, a_sixth_pushes_the_oldest_out) {
  WifiKnown l[WIFI_KNOWN_MAX] = {};
  int n = 0;
  const char *names[6] = { "one", "two", "three", "four", "five", "six" };
  for (auto s : names) n = wifiKnownAdd(l, n, s, "password");
  CHECK_EQ(n, WIFI_KNOWN_MAX);
  CHECK(!strcmp(l[0].ssid, "six"));
  CHECK(!strcmp(l[4].ssid, "two"));
  CHECK_EQ(wifiKnownFind(l, n, "one"), -1);
}

TEST(wifi_known, open_networks_and_bad_names) {
  WifiKnown l[WIFI_KNOWN_MAX] = {};
  int n = wifiKnownAdd(l, 0, "Cafe", "");
  CHECK_EQ(n, 1);
  CHECK(!strcmp(l[0].pass, ""));
  CHECK_EQ(wifiKnownAdd(l, n, "", "password"), 1);                                   // no name
  CHECK_EQ(wifiKnownAdd(l, n, "a name of more than thirty-two letters", "pw"), 1);  // too long
}

TEST(wifi_known, it_joins_the_loudest_known_network_it_hears) {
  WifiKnown l[WIFI_KNOWN_MAX] = {};
  int n = 0;
  n = wifiKnownAdd(l, n, "SOS", "home-pass");
  n = wifiKnownAdd(l, n, "DISRUPTIVE INC", "work-pass");
  // at home: SOS and the neighbours, not the office
  const char *home[3] = { "Neighbor", "SOS", "Office-Tv" };
  int homeRssi[3] = { -40, -55, -70 };
  CHECK_EQ(wifiKnownPick(l, n, home, homeRssi, 3, nullptr), wifiKnownFind(l, n, "SOS"));
  // both in range: the louder
  const char *both[2] = { "SOS", "DISRUPTIVE INC" };
  int bothRssi[2] = { -75, -47 };
  CHECK_EQ(wifiKnownPick(l, n, both, bothRssi, 2, nullptr), wifiKnownFind(l, n, "DISRUPTIVE INC"));
}

TEST(wifi_known, after_a_failure_it_tries_another_it_hears) {
  WifiKnown l[WIFI_KNOWN_MAX] = {};
  int n = 0;
  n = wifiKnownAdd(l, n, "SOS", "home-pass");
  n = wifiKnownAdd(l, n, "DISRUPTIVE INC", "work-pass");
  const char *both[2] = { "SOS", "DISRUPTIVE INC" };
  int bothRssi[2] = { -75, -47 };
  CHECK_EQ(wifiKnownPick(l, n, both, bothRssi, 2, "DISRUPTIVE INC"), wifiKnownFind(l, n, "SOS"));
  // the only one it hears failed: it tries it again
  const char *one[1] = { "SOS" };
  int oneRssi[1] = { -60 };
  CHECK_EQ(wifiKnownPick(l, n, one, oneRssi, 1, "SOS"), wifiKnownFind(l, n, "SOS"));
}

TEST(wifi_known, hearing_none_it_tries_the_most_recent) {
  WifiKnown l[WIFI_KNOWN_MAX] = {};
  CHECK_EQ(wifiKnownPick(l, 0, nullptr, nullptr, 0, nullptr), -1);  // knows none
  int n = wifiKnownAdd(l, 0, "Hidden", "password");
  n = wifiKnownAdd(l, n, "Work", "password");
  const char *heard[1] = { "Somewhere else" };
  int rssi[1] = { -50 };
  CHECK_EQ(wifiKnownPick(l, n, heard, rssi, 1, nullptr), 0);
}
