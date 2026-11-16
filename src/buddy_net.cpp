#include "buddy_net.h"

#ifdef BOARD_KNOMI_V2
#include <WiFi.h>
#include <WiFiUdp.h>
#include <NetworkClientSecure.h>
#include <esp_mac.h>
#include <esp_random.h>
#include "ota_roots.h"

static WiFiUDP udp;
static bool listening = false;
static uint32_t peerId = 0;
static IPAddress peerIp;

uint32_t buddyNetId() {
  static uint32_t id = 0;
  if (!id) {
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    id = ((uint32_t)mac[2] << 24 | (uint32_t)mac[3] << 16 | (uint32_t)mac[4] << 8 | mac[5]) | 1;
  }
  return id;
}

// ---- the relay: the same messages over a WebSocket, so browsers on the same home network
// (which can't use the Wi-Fi messages) meet this Knomi too. A message may arrive both ways; the
// game already copes with repeats, since the Wi-Fi ones are resent until answered. ----
#define RELAY_FIRST_MS 20000UL   // after boot: Wi-Fi and the clock first
#define RELAY_RETRY_MS 10000UL   // after a failure, doubling up to RELAY_RETRY_MAX
#define RELAY_RETRY_MAX 300000UL
static NetworkClientSecure *rc = nullptr;
static bool relayOn = false;
static int relayOthers = 0;  // the relay's {"n": ...}: alone, nothing is sent
static uint32_t relayT = 0, relayWait = RELAY_FIRST_MS, relayPingT = 0;
static bool relayHeld = false;  // buddyNetRelayHold: closed for a Wi-Fi update
static bool relayFresh = false;  // newly connected: buddyNetRelayFresh()

static void relayClose(bool failed) {
  if (rc) {
    rc->stop();
    delete rc;
    rc = nullptr;
  }
  if (failed) relayWait = relayWait < RELAY_RETRY_MS ? RELAY_RETRY_MS : (relayWait * 2 > RELAY_RETRY_MAX ? RELAY_RETRY_MAX : relayWait * 2);
  relayOn = false;
  relayOthers = 0;
  relayT = millis();
}

// one WebSocket frame from us: always masked, as the standard asks of clients; short ones only
static bool wsSend(uint8_t opcode, const uint8_t *data, size_t len) {
  if (!rc || len > 125) return false;
  uint8_t f[6 + 125];
  uint32_t m = esp_random();
  f[0] = 0x80 | opcode;
  f[1] = 0x80 | len;
  memcpy(f + 2, &m, 4);
  for (size_t i = 0; i < len; i++) f[6 + i] = data[i] ^ f[2 + (i & 3)];
  return rc->write(f, 6 + len) == 6 + len;
}

// Connects and upgrades to a WebSocket: true once it's upgraded. The TLS handshake takes a second or two, and when the
// relay can't be reached, up to its timeouts: so it runs on a task of its own, on the other core (relayConnect()),
// and the game no longer waits for it. It held the loop all that time, the screen frozen: 32 s once, on a Wi-Fi that
// came and went.
static bool relayHandshake(NetworkClientSecure *c) {
  c->setCACert(OTA_ROOTS);
  c->setHandshakeTimeout(8);
  if (!c->connect(BUDDY_RELAY_HOST, 443)) {
    Serial.println("buddy: relay unreachable");
    return false;
  }
  uint8_t raw[16];
  esp_fill_random(raw, sizeof(raw));
  static const char *B64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  char key[25];
  for (int i = 0, o = 0; i < 16; i += 3) {
    uint32_t v = raw[i] << 16 | (i + 1 < 16 ? raw[i + 1] << 8 : 0) | (i + 2 < 16 ? raw[i + 2] : 0);
    key[o++] = B64[(v >> 18) & 63];
    key[o++] = B64[(v >> 12) & 63];
    key[o++] = i + 1 < 16 ? B64[(v >> 6) & 63] : '=';
    key[o++] = i + 2 < 16 ? B64[v & 63] : '=';
    key[o] = 0;
  }
  c->printf("GET " BUDDY_RELAY_PATH " HTTP/1.1\r\nHost: " BUDDY_RELAY_HOST "\r\nUpgrade: websocket\r\n"
            "Connection: Upgrade\r\nSec-WebSocket-Key: %s\r\nSec-WebSocket-Version: 13\r\n"
            "User-Agent: Catch151/2\r\n\r\n", key);  // /2: steps aside for Wi-Fi updates (1.31 on)
  c->setTimeout(4000);
  String status = c->readStringUntil('\n');
  if (!status.startsWith("HTTP/1.1 101")) {
    Serial.printf("buddy: relay refused (%s)\n", status.c_str());
    return false;
  }
  while (c->connected()) {  // the rest of the headers, up to the blank line
    String h = c->readStringUntil('\n');
    if (h.length() <= 1) break;
  }
  c->setTimeout(1000);
  return true;
}

// The task: a connection made each time relayConnect() asks, handed back in rcMade with relayDone 1 (or relayDone -1,
// nothing made). Meanwhile the loop leaves the relay alone (relayBusy).
static NetworkClientSecure *rcMade = nullptr;
static int relayDone = 0;
static bool relayBusy = false;
static SemaphoreHandle_t relayGo = nullptr;
static void relayMake() {  // (on the task, or in the loop if there's no task)
  NetworkClientSecure *c = new NetworkClientSecure;
  bool ok = relayHandshake(c);
  if (!ok) {
    c->stop();
    delete c;
    c = nullptr;
  }
  rcMade = c;
  __atomic_store_n(&relayDone, ok ? 1 : -1, __ATOMIC_RELEASE);
}
static void relayTask(void *) {
  for (;;) {
    xSemaphoreTake(relayGo, portMAX_DELAY);
    relayMake();
  }
}
static void relayConnect() {
  static bool tried = false, task = false;
  if (!tried) {
    tried = true;
    relayGo = xSemaphoreCreateBinary();
    // (8 KB of stack, as the loop's task had for it)
    task = relayGo && xTaskCreatePinnedToCore(relayTask, "relay", 8192, nullptr, 1, nullptr, 0) == pdPASS;
  }
  relayBusy = true;
  relayDone = 0;
  if (task) xSemaphoreGive(relayGo);
  else relayMake();  // no room for the task: in the loop, as before
}
// In the loop: a connection made meanwhile becomes the relay's, or is closed if the relay isn't wanted now (off the
// Wi-Fi, or held for an update); a failure makes the pause before the next try longer
static void relayCollect(bool wanted) {
  int done = __atomic_load_n(&relayDone, __ATOMIC_ACQUIRE);
  if (!relayBusy || !done) return;
  NetworkClientSecure *c = rcMade;
  rcMade = nullptr;
  relayBusy = false;
  relayDone = 0;
  if (done > 0 && wanted) {
    rc = c;
    relayOn = true;
    relayWait = RELAY_RETRY_MS;
    relayPingT = millis();
    relayFresh = true;
    Serial.println("buddy: relay connected");
    return;
  }
  if (c) {
    c->stop();
    delete c;
  }
  relayClose(done < 0);
}

static void relayTick(bool wifi) {
  uint32_t now = millis();
  relayCollect(wifi && !relayHeld);
  if (!wifi || relayHeld) {
    if (rc) relayClose(false);
    return;
  }
  if (!rc && !relayBusy && now - relayT >= relayWait) relayConnect();
  if (relayOn && now - relayPingT >= BUDDY_RELAY_PING_MS) {
    relayPingT = now;
    if (!wsSend(0x1, (const uint8_t *)"ping", 4)) relayClose(true);
  }
  if (rc && !rc->connected()) relayClose(true);
}

static bool relayPoll(BuddyMsg &out) {
  for (int k = 0; k < 4 && relayOn && rc->available() >= 2; k++) {
    uint8_t h[2];
    rc->read(h, 2);
    uint8_t op = h[0] & 0x0f;
    size_t len = h[1] & 0x7f;
    if (len == 126) {
      uint8_t e[2];
      if (rc->readBytes(e, 2) != 2) { relayClose(true); return false; }
      len = (size_t)e[0] << 8 | e[1];
    }
    if (len > 512 || len == 127 || (h[1] & 0x80)) {  // nothing that big comes from the relay, and it never masks
      relayClose(true);
      return false;
    }
    uint8_t buf[513];  // + the end of a text frame
    if (rc->readBytes(buf, len) != len) {
      relayClose(true);
      return false;
    }
    if (op == 0x8) {  // the relay is closing
      relayClose(true);
      return false;
    }
    if (op == 0x9) {  // ping: answered with the same data
      wsSend(0xA, buf, len);
      continue;
    }
    if (op == 0x1) {  // {"n": others}; "pong" and anything else are ignored
      buf[len] = 0;
      const char *n = strstr((const char *)buf, "\"n\":");
      if (n) relayOthers = atoi(n + 4);
      continue;
    }
    if (op != 0x2 || !buddyParse(buf, len, out) || out.from == buddyNetId()) continue;
    if (out.to != 0 && out.to != buddyNetId()) continue;
    return true;
  }
  return false;
}

bool buddyNetUp() {
  bool up = WiFi.status() == WL_CONNECTED;
  if (up && !listening) listening = udp.begin(BUDDY_PORT);
  else if (!up && listening) {
    udp.stop();
    listening = false;
    peerIp = IPAddress();
  }
  relayTick(up);
  return up && listening;
}

bool buddyNetPoll(BuddyMsg &out) {
  if (!listening) return false;
  // doesn't get stuck on garbage: at most a few packets per call
  for (int i = 0; i < 4; i++) {
    int len = udp.parsePacket();
    if (len <= 0) break;
    uint8_t buf[sizeof(BuddyMsg) + 1];
    int got = udp.read(buf, sizeof(buf));
    if (got != len) continue;
    if (!buddyParse(buf, got, out) || out.from == buddyNetId()) continue;
    if (out.to != 0 && out.to != buddyNetId()) continue;
    if (out.from == peerId) peerIp = udp.remoteIP();
    return true;
  }
  return relayPoll(out);
}

void buddyNetSend(const BuddyMsg &m) {
  if (!listening) return;
  bool direct = m.to != 0 && m.to == peerId && peerIp != IPAddress();
  udp.beginPacket(direct ? peerIp : WiFi.broadcastIP(), BUDDY_PORT);
  udp.write((const uint8_t *)&m, sizeof(m));
  udp.endPacket();
  if (relayOn && relayOthers > 0 && !wsSend(0x2, (const uint8_t *)&m, sizeof(m))) relayClose(true);
}

void buddyNetReport(const BuddyMsg &m) {
  if (relayOn && !wsSend(0x2, (const uint8_t *)&m, sizeof(m))) relayClose(true);
}

bool buddyNetRelayFresh() {
  bool f = relayFresh && relayOn;
  relayFresh = false;
  return f;
}

void buddyNetSetPeer(uint32_t id) {
  if (id != peerId) peerIp = IPAddress();
  peerId = id;
}

void buddyNetRelayHold(bool hold) {
  if (hold == relayHeld) return;
  relayHeld = hold;
  if (hold) {
    // frees its TLS memory now, a connection being made too once it's done (the update makes one of its own; this
    // waits only before an update, which the screen dimmed or the update's own screen allows)
    while (relayBusy && !__atomic_load_n(&relayDone, __ATOMIC_ACQUIRE)) delay(20);
    relayCollect(false);
    if (rc) relayClose(false);
  } else {
    relayT = millis();
    relayWait = 2000;  // back soon after
  }
}

#else

uint32_t buddyNetId() { return 1; }
void buddyNetRelayHold(bool) {}
bool buddyNetUp() { return false; }
bool buddyNetPoll(BuddyMsg &) { return false; }
void buddyNetSend(const BuddyMsg &) {}
void buddyNetReport(const BuddyMsg &) {}
bool buddyNetRelayFresh() { return false; }
void buddyNetSetPeer(uint32_t) {}

#endif
