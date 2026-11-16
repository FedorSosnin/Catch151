// The buddy relay: lets Catch 151 in a browser (which can't send the Knomi's local Wi-Fi
// messages) meet Knomis, Macs and other browsers on the same home network.
//
// It only passes along the game's own buddy messages (65 bytes, "PKG1", see buddy.h) between
// devices that share an internet address, as devices on one home Wi-Fi do. Battles, trades and
// every check on the messages stay on the devices, exactly as on the Wi-Fi. It stores one thing:
// the heaviest fish anyone has landed (below).
//
//   wss://<relay>/ws?v4=<the device's public IPv4, when it connects over IPv6>
//
// A computer or phone usually connects over IPv6, where every device has its own address, while a
// Knomi uses the home's shared IPv4: devices look their IPv4 up and send it, and are grouped by it.
// Text frames from the relay tell each device how many others share its group: {"n": 2}. A device
// that is alone sends nothing, which keeps the relay within Cloudflare's free plan.
//
//   https://<relay>/stats  ->  {"online": 12}
//
// How many devices are connected right now, all of them together (Knomis on Wi-Fi, the Mac and
// Windows apps, open browser tabs). Only the number: nothing about who or where.
//
// And the heaviest fish anyone has landed in Reel Fishing: {"online": 12, "fish": {...}, "biggest":
// "621.7 lb Gyarados"}. Each device tells the relay its own heaviest (a BM_FISH message, only to the
// relay) when it connects and when it lands a heavier one; the relay keeps the heaviest of all.
//
// And the world board (board.js): each game's best ten, with the gamer tag each came with and the day
// ({"board": {"games": [...], "fish": [...]}} in /stats), from BM_SCORE messages (and BM_FISH for the
// fish), and as a picture for the README:
//
//   https://<relay>/board.svg
//
// It keeps the scores, the tags, the days and a hash of each device's id (one entry a device), nothing
// about who or where.

import { BM_SCORE, GAMES, scoreReport, boardAdd, boardSvg, tagOf, idHash, tagShown } from './board.js';

const MSG_SIZE = 65;
const MAX_PER_GROUP = 16;  // a home has a few devices: more is someone filling a group on purpose
const RATE_PER_S = 8;      // the game sends a few messages a second at most, during a battle
const BURST = 16;
const STATS_CACHE_S = 60;  // the count is kept a minute: looking often doesn't wake the lobby each time
const BOARD_CACHE_S = 300; // the board's picture: five minutes (GitHub keeps its own copy a while too)

// Reel Fishing (src/fishing.h): the Beach's water Pokemon and their Pokedex weights in tenths of a kg. A fish
// weighs 0.8 to 1.2 times its kind's, so a report over 1.2 times can't be real and is ignored
const BM_FISH = 17;  // src/buddy.h
const FISH = {
  90: ['Shellder', 40], 98: ['Krabby', 65], 116: ['Horsea', 80], 129: ['Magikarp', 100], 60: ['Poliwag', 124],
  54: ['Psyduck', 196], 79: ['Slowpoke', 360], 118: ['Goldeen', 150], 61: ['Poliwhirl', 200], 117: ['Seadra', 250],
  120: ['Staryu', 345], 119: ['Seaking', 390], 72: ['Tentacool', 455], 73: ['Tentacruel', 550], 99: ['Kingler', 600],
  55: ['Golduck', 766], 80: ['Slowbro', 785], 121: ['Starmie', 800], 86: ['Seel', 900], 87: ['Dewgong', 1200],
  91: ['Cloyster', 1325], 131: ['Lapras', 2200], 130: ['Gyarados', 2350],
};

// a BM_FISH message's fish ({ kg10, dex, shiny }), or null if it isn't one that can be caught
export function fishReport(data) {
  if (!(data instanceof ArrayBuffer) || data.byteLength !== MSG_SIZE) return null;
  const v = new DataView(data);
  if (v.getUint8(5) !== BM_FISH) return null;
  const dex = v.getInt16(22, true), shiny = v.getUint8(25) === 1, kg10 = v.getUint16(26, true);
  const kind = FISH[dex];
  if (!kind || kg10 < 1 || kg10 * 5 > kind[1] * 6) return null;
  return { kg10, dex, shiny };
}

// as the game shows it: tenths of a pound, rounded (fishLb10() in src/fishing.h)
const lb = (kg10) => Math.floor((kg10 * 22046 + 5000) / 10000) / 10;
export function fishText(f) {
  if (!f || !FISH[f.dex]) return 'none yet';
  const lb10 = Math.floor((f.kg10 * 22046 + 5000) / 10000);
  return `${Math.floor(lb10 / 10)}.${lb10 % 10} lb ${f.shiny ? 'shiny ' : ''}${FISH[f.dex][0]}`;
}

// the board as /stats shows it: each game's best ten and the fish's, the tags as they show (---- or ****), no ids
export function boardPublic(scores, fishTop) {
  return {
    games: Object.keys(GAMES).map((g) => ({
      game: +g, name: GAMES[g].name,
      top: (scores[g] || []).map((e) => ({ tag: tagShown(e.tag), score: e.score, shown: GAMES[g].show(e.score), day: e.day })),
    })),
    fish: fishTop.map((e) => ({ tag: tagShown(e.tag), lb: lb(e.score), kind: FISH[e.dex][0], shiny: e.shiny, day: e.day })),
  };
}

export default {
  async fetch(request, env) {
    const url = new URL(request.url);
    if (url.pathname === '/ws') {
      if (request.headers.get('Upgrade') !== 'websocket') return new Response('expected a websocket', { status: 426 });
      const keys = groupKeys(request.headers.get('CF-Connecting-IP') || '', url.searchParams.get('v4'));
      if (!keys.length) return new Response('no address', { status: 400 });
      const lobby = env.LOBBY.get(env.LOBBY.idFromName('lobby'));
      const fwd = new Request(request);
      fwd.headers.set('X-Groups', keys.join(','));
      return lobby.fetch(fwd);
    }
    if (url.pathname === '/stats' || url.pathname === '/board.svg') {
      const svg = url.pathname === '/board.svg';
      const cache = caches.default;
      const key = new Request(url.origin + url.pathname);
      let res = await cache.match(key);
      if (!res) {
        const lobby = env.LOBBY.get(env.LOBBY.idFromName('lobby'));
        const { online, fish, board } = await (await lobby.fetch(new Request(url.origin + '/stats'))).json();
        const big = fish ? { lb: lb(fish.kg10), kind: FISH[fish.dex][0], shiny: fish.shiny, day: fish.day } : null;
        res = new Response(svg ? boardSvg(online, board) : JSON.stringify({ online, fish: big, biggest: fishText(fish), board }), {
          headers: {
            'content-type': svg ? 'image/svg+xml' : 'application/json',
            'cache-control': `public, max-age=${svg ? BOARD_CACHE_S : STATS_CACHE_S}`,
            'access-control-allow-origin': '*',  // a dashboard on another site can read it (the scores page does)
          },
        });
        await cache.put(key, res.clone());
      }
      return res;
    }
    return new Response('Catch 151 buddy relay. See https://github.com/FedorSosnin/Catch151\n', {
      headers: { 'content-type': 'text/plain' },
    });
  },
};

const V4 = /^(25[0-5]|2[0-4]\d|1?\d?\d)(\.(25[0-5]|2[0-4]\d|1?\d?\d)){3}$/;

// the groups a device belongs to: its IPv4, or its IPv6 /64 (one home network) plus the IPv4 it
// reports. A reported IPv4 is only taken from IPv6 connections, which can't show it themselves.
export function groupKeys(ip, claimed) {
  if (V4.test(ip)) return ['4:' + ip];
  if (!ip.includes(':')) return [];
  const keys = ['6:' + expand6(ip).slice(0, 4).join(':')];
  if (claimed && V4.test(claimed)) keys.push('4:' + claimed);
  return keys;
}

function expand6(ip) {
  const [head, tail = ''] = ip.split('::');
  const a = head ? head.split(':') : [];
  const b = tail ? tail.split(':') : [];
  const fill = ip.includes('::') ? Array(8 - a.length - b.length).fill('0') : [];
  return [...a, ...fill, ...b].map((h) => h.toLowerCase().replace(/^0+(?=.)/, ''));
}

export function validMessage(data) {
  if (!(data instanceof ArrayBuffer) || data.byteLength !== MSG_SIZE) return false;
  const b = new Uint8Array(data);
  return b[0] === 0x50 && b[1] === 0x4b && b[2] === 0x47 && b[3] === 0x31;  // "PKG1"
}

// One object holds every connection (with WebSocket hibernation, idle ones cost nothing). Each
// socket is tagged with its groups, so finding who shares a group is a lookup, not a scan.
export class Lobby {
  constructor(ctx) {
    this.ctx = ctx;
    // devices keep their connection open with a "ping" now and then; answered here without waking
    // the object, so keeping a quiet connection open costs nothing
    ctx.setWebSocketAutoResponse(new WebSocketRequestResponsePair('ping', 'pong'));
  }

  // the heaviest fish reported, kept in the object's storage (read once, then from memory)
  async biggest() {
    if (this.fish === undefined) this.fish = (await this.ctx.storage.get('fish')) || null;
    return this.fish;
  }

  // the world board: each game's top ten ({ game: [entry] }) and the fish's, from storage once. The heaviest fish
  // kept before there was a board starts the fish's (with no tag)
  async board() {
    if (this.scores === undefined) {
      this.scores = (await this.ctx.storage.get('scores')) || {};
      this.fishTop = (await this.ctx.storage.get('fishTop')) || [];
      const f = await this.biggest();
      if (!this.fishTop.length && f) this.fishTop.push({ id: '', tag: '', score: f.kg10, dex: f.dex, shiny: f.shiny, day: f.day });
    }
    return { scores: this.scores, fishTop: this.fishTop };
  }

  async fetch(request) {
    if (new URL(request.url).pathname === '/stats') {  // each socket once, whatever its groups
      const { scores, fishTop } = await this.board();
      return Response.json({ online: this.ctx.getWebSockets().length, fish: await this.biggest(), board: boardPublic(scores, fishTop) });
    }
    const groups = (request.headers.get('X-Groups') || '').split(',').filter(Boolean);
    for (const g of groups)
      if (this.ctx.getWebSockets(g).length >= MAX_PER_GROUP) return new Response('group full', { status: 429 });
    const pair = new WebSocketPair();
    const [client, server] = Object.values(pair);
    this.ctx.acceptWebSocket(server, groups);
    server.serializeAttachment({ groups, tokens: BURST, at: Date.now() });
    this.announce(groups);
    return new Response(null, { status: 101, webSocket: client });
  }

  // everyone sharing a group with ws (itself and one that is leaving excluded), each once
  others(ws, groups, gone) {
    const seen = new Set();
    for (const g of groups) for (const o of this.ctx.getWebSockets(g)) if (o !== ws && o !== gone) seen.add(o);
    return seen;
  }

  // tells every socket in these groups how many others it can reach now
  announce(groups, gone) {
    const touched = new Set();
    for (const g of groups) for (const s of this.ctx.getWebSockets(g)) if (s !== gone) touched.add(s);
    for (const s of touched) {
      const a = s.deserializeAttachment();
      try { s.send(JSON.stringify({ n: this.others(s, a.groups, gone).size })); } catch (e) {}
    }
  }

  async webSocketMessage(ws, data) {
    if (!validMessage(data)) return;  // text and anything else: ignored
    const a = ws.deserializeAttachment();
    const now = Date.now();
    a.tokens = Math.min(BURST, a.tokens + ((now - a.at) / 1000) * RATE_PER_S);
    a.at = now;
    if (a.tokens < 1) return;  // too fast: dropped, like a lost Wi-Fi packet
    a.tokens -= 1;
    ws.serializeAttachment(a);
    const type = new Uint8Array(data)[5];
    if (type === BM_FISH) {  // a device's heaviest fish: the board's, and kept if it's the heaviest yet; never passed on
      const f = fishReport(data);
      if (!f) return;
      const day = new Date().toISOString().slice(0, 10);
      const { fishTop } = await this.board();
      const id = idHash(new DataView(data).getUint32(6, true)), tag = tagOf(new Uint8Array(data, 34, 12));
      if (boardAdd(fishTop, { id, tag, score: f.kg10, dex: f.dex, shiny: f.shiny, day })) await this.ctx.storage.put('fishTop', fishTop);
      const best = await this.biggest();
      if (!best || f.kg10 > best.kg10) {
        this.fish = { ...f, day };
        await this.ctx.storage.put('fish', this.fish);
      }
      return;
    }
    if (type === BM_SCORE) {  // a game's best, with the gamer tag: the board's; never passed on
      const s = scoreReport(data, MSG_SIZE);
      if (!s) return;
      const { scores } = await this.board();
      const list = scores[s.game] || (scores[s.game] = []);
      if (boardAdd(list, { id: s.id, tag: s.tag, score: s.score, day: new Date().toISOString().slice(0, 10) }))
        await this.ctx.storage.put('scores', scores);
      return;
    }
    for (const o of this.others(ws, a.groups)) {
      try { o.send(data); } catch (e) {}
    }
  }

  async webSocketClose(ws) {
    const a = ws.deserializeAttachment();
    try { ws.close(); } catch (e) {}
    if (a) this.announce(a.groups, ws);
  }

  async webSocketError(ws) {
    await this.webSocketClose(ws);
  }
}
