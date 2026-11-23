// The leaderboard (Fedor, 2026-11-16, first called the world board, then High Scores, then "Use Leaderboard": "a section in the Repo showing live players, biggest fish, longest run, highest
// pinball score, etc." with each best's "score, date, and tag"). The relay keeps each game's top ten: the score, the
// gamer tag it came with and the day, one entry a device (by a hash of its id: nothing about who or where). The README
// shows it as a picture (/board.svg) and the website's scores page reads it from /stats.
import { FONT, CART, CART_W, CART_H } from './board_art.js';

export const BM_SCORE = 18;  // src/buddy.h
// The board started over on 2026-11-16 (Fedor: some of the first scores "were made during testing when the games were
// still evolving ... we should put them all to zeros until someone plays the game and gets their score put there"):
// only a score or a fish sent with this mark (the message's turn byte, offset 20: src/buddy.h BOARD_SEASON) counts, a
// best made since. 2.2.9 sent its all-time records unmarked: those are ignored
export const SEASON = 1;
export const seasonOf = (data) => new Uint8Array(data)[20];
export const RELAY_ID = 0xffffffff;  // the relay as a message's sender (src/buddy.h BUDDY_RELAY_ID)
export const TOP = 10;       // kept a game
// the games on the board, as the Games menu numbers them (src/menus_v2.h V2_CART_GAME), with the most a real one can
// score (the game's own limits: Berry Drop's is a byte, Dungeon Crawler's floor too, times in seconds; BerryMatch has
// none, its 20 moves scoring 750 to 4,700 a game, 20,000 a rare one: six figures is far past that; Pic Snap's roll of
// ten photos is 100 points each at most), and how a score is shown. Reel Fishing's place is its biggest fish
// (BM_FISH, index.js)
export const GAMES = {
  0: { name: 'Berry Drop', max: 255, show: (v) => num(v) },
  2: { name: 'Rock Hopper', max: 65535, show: (v) => mmss(v), sub: 'LONGEST RUN' },
  3: { name: 'Dungeon Crawler', max: 255, show: (v) => `FLOOR ${v}` },
  4: { name: 'PikaSurf', max: 65535, show: (v) => mmss(v), sub: 'LONGEST RIDE' },
  6: { name: 'Pinball', max: 9999999, show: (v) => num(v) },
  7: { name: 'BerryMatch', max: 999999, show: (v) => num(v) },
  8: { name: 'Pic Snap', max: 1000, show: (v) => num(v) },
};
const FISH_GAME = 5;
const num = (v) => String(v).replace(/\B(?=(\d{3})+(?!\d))/g, ',');
const mmss = (v) => `${Math.floor(v / 60)}:${String(v % 60).padStart(2, '0')}`;

// ----- the gamer tag (src/gamertag.h): the same list and the same reading of it, so what the game refuses is what
// the board hides; a tag on it shows as **** (Fedor: "a list of typical cursewords that would force a name to show
// up as ****"). The list can grow here without an update to the game
export const STRONG = [
  'FUCK', 'FUK', 'FUC', 'FCK', 'FUQ', 'SHIT', 'SHT', 'CUNT', 'COCK', 'DICK', 'DIK', 'TWAT', 'JIZ', 'CUM', 'NIG', 'NGR',
  'FAG', 'KKK', 'NAZI', 'RAPE', 'PORN', 'SLUT', 'WHOR', 'KYS', 'STFU', 'GTFO', 'TITS', 'BICH', 'BTCH', 'WANK', 'CLIT',
  'DYKE', 'KIKE', 'SPIC', 'GOOK', 'COON', 'CHNK', 'SEX', 'XXX', 'MILF', 'DILF', 'ANAL', 'ANUS', 'BOOB', 'ARSE', 'ASSH',
  'PISS', 'NUDE', 'PUSY', 'PSSY', 'HTLR',
];
export const MILD = ['ASS', 'TIT', 'HOE', 'WTF', 'PAKI', 'HOMO', 'SUX', 'PEE', 'POO'];
const STAND_IN = { 0: 'O', 1: 'I', '!': 'I', '|': 'I', 3: 'E', 4: 'A', '@': 'A', 5: 'S', $: 'S', 7: 'T', 8: 'B', 9: 'G' };
export function tagLetters(tag) {
  let l = '';
  for (const ch of tag.toUpperCase()) {
    const c = STAND_IN[ch] || ch;
    if (c >= 'A' && c <= 'Z') l += c;
  }
  return l.slice(0, 8);
}
export function tagRude(tag) {
  const l = tagLetters(tag);
  return STRONG.some((w) => l.includes(w)) || MILD.includes(l);
}
// a message's tag (the nick field: offset 34, 12 bytes): printable characters, four at most, trimmed
export function tagOf(bytes) {
  let t = '';
  for (let i = 0; i < 12 && bytes[i]; i++) if (bytes[i] >= 32 && bytes[i] < 127 && t.length < 4) t += String.fromCharCode(bytes[i]);
  return t.trim();
}
// how a tag shows: ----, its own, or **** for a rude one
export const tagShown = (t) => (!t ? '----' : tagRude(t) ? '****' : t);

// a device's id as kept: a hash of it (FNV-1a), only to keep one entry a device
export function idHash(from) {
  let h = 0x811c9dc5;
  for (let i = 0; i < 4; i++) h = Math.imul(h ^ ((from >>> (8 * i)) & 255), 16777619) >>> 0;
  return h.toString(16);
}

// a BM_SCORE message ({ game, score, tag, id }), or null if it isn't one, or the score couldn't be real
export function scoreReport(data, MSG_SIZE = 65) {
  if (!(data instanceof ArrayBuffer) || data.byteLength !== MSG_SIZE) return null;
  const v = new DataView(data);
  if (v.getUint8(5) !== BM_SCORE || v.getUint8(20) !== SEASON) return null;
  const game = v.getUint8(18), score = v.getUint32(14, true), g = GAMES[game];
  if (!g || score < 1 || score > g.max) return null;
  return { game, score, tag: tagOf(new Uint8Array(data, 34, 12)), id: idHash(v.getUint32(6, true)) };
}

// the relay's answer to a device whose best is the first of its game's ten (Fedor: "a badge/achievement for holding a
// global high score"): a BM_SCORE from RELAY_ID to it, the game and the score as it sent them. The game gives the
// golden cartridge for it, once
export function topReply(to, game, score) {
  const b = new ArrayBuffer(65), v = new DataView(b);
  [0x50, 0x4b, 0x47, 0x31].forEach((c, i) => v.setUint8(i, c));  // "PKG1"
  v.setUint8(4, 1);  // BUDDY_VER
  v.setUint8(5, BM_SCORE);
  v.setUint32(6, RELAY_ID, true);
  v.setUint32(10, to, true);
  v.setUint32(14, score, true);
  v.setUint8(18, game);
  return b;
}

// ----- the board on the devices (2.2.15): the Games menu's leaderboard pages show each game's best three. A device asks
// (a BM_BOARD from it, to the relay alone: kind = one game, or BOARD_ALL) when its Games menu opens; the relay answers
// each game in a BM_BOARD of its own (src/buddy.h boardFrom() reads it)
export const BM_BOARD = 19;  // src/buddy.h
export const BOARD_ALL = 0xff;
export const BOARD_TOP = 3;   // the best three a game
export const BOARD_GAMES = [0, 2, 3, 4, FISH_GAME, 6, 7, 8];  // the games with a board, as the Games menu numbers them
// a device asking for the board ({ from, kind }), or null if it isn't one
export function boardAsk(data, MSG_SIZE = 65) {
  if (!(data instanceof ArrayBuffer) || data.byteLength !== MSG_SIZE) return null;
  const v = new DataView(data);
  if (v.getUint8(5) !== BM_BOARD) return null;
  const from = v.getUint32(6, true), kind = v.getUint8(18);
  if (!from || from === RELAY_ID || (kind !== BOARD_ALL && !BOARD_GAMES.includes(kind))) return null;
  return { from, kind };
}
// the answer for one game: BM_BOARD from RELAY_ID to the device, its best three ({ tag, score, day, dex?, shiny? }, best
// first) as the board shows them. session = the first's score, offsets 26 and 30 the second's and third's (32 bits
// each), move = how many, 34 the first two tags, 46 the third's, 50.. each one's month and day; Reel Fishing's first
// fish at 22 (its kind) and 25 (shiny)
export function boardReply(to, game, top) {
  const b = new ArrayBuffer(65), v = new DataView(b);
  [0x50, 0x4b, 0x47, 0x31].forEach((c, i) => v.setUint8(i, c));  // "PKG1"
  v.setUint8(4, 1);  // BUDDY_VER
  v.setUint8(5, BM_BOARD);
  v.setUint32(6, RELAY_ID, true);
  v.setUint32(10, to, true);
  v.setUint8(18, game);
  v.setUint8(19, 1);  // ack: a reply
  v.setUint8(20, SEASON);
  const list = top.slice(0, BOARD_TOP);
  v.setUint8(21, list.length);
  const at = [14, 26, 30], tagAt = [34, 38, 46];
  list.forEach((e, i) => {
    v.setUint32(at[i], e.score, true);
    const t = tagShown(e.tag).padEnd(4, ' ').slice(0, 4);
    for (let k = 0; k < 4; k++) v.setUint8(tagAt[i] + k, t.charCodeAt(k) & 0x7f);
    const [, mm, dd] = String(e.day).split('-');
    v.setUint8(50 + 2 * i, +mm || 1);
    v.setUint8(51 + 2 * i, +dd || 1);
  });
  if (game === FISH_GAME && list.length) {
    v.setInt16(22, list[0].dex, true);
    v.setUint8(25, list[0].shiny ? 1 : 0);
  }
  return b;
}

// adds an entry ({ id, score, tag, day, ... }) to a top ten, best first: a device's own entry takes its new tag
// whatever the score, and the new score and day only when it's higher. Returns whether anything changed
export function boardAdd(list, e) {
  const i = list.findIndex((o) => o.id === e.id);
  if (i >= 0) {
    const o = list[i];
    const changed = e.score > o.score || o.tag !== e.tag;
    if (e.score > o.score) Object.assign(o, e);
    else o.tag = e.tag;
    if (changed) list.sort(byBest);
    return changed;
  }
  if (list.length >= TOP && e.score <= list[list.length - 1].score) return false;
  list.push(e);
  list.sort(byBest);
  list.length = Math.min(list.length, TOP);
  return true;
}
// a kind of fish's record ({ id, tag, score, ... }) in records, by its dex (Fedor: "a record weight for each type of
// fish caught. That would give incentive to try to catch all fish"): the heavier one takes it (a tie: the first to it
// keeps it), and its holder's new tag comes with any report from it. Returns whether anything changed
export function recordAdd(records, key, e) {
  const o = records[key];
  if (!o || e.score > o.score) {
    records[key] = e;
    return true;
  }
  if (o.id !== e.id || o.tag === e.tag) return false;
  o.tag = e.tag;
  return true;
}
const byBest = (a, b) => b.score - a.score || (a.day < b.day ? -1 : a.day > b.day ? 1 : 0);  // a tie: the first to it

// ----- the picture: /board.svg, in the game's own 5x7 font, each game's cartridge beside it. GitHub shows it in the
// README as an image (an <img> can't load anything, so the cartridges are in it)
const BG = '#1b2130', RIM = '#3a4a5a', LINE = '#2c3648', ALT = '#1f2637';
const CREAM = '#fff3dc', GOLD = '#f0c04a', MUTED = '#8a96a8', WHITE = '#ecf0f8', INK = '#222230';
const MONTH = ['JAN', 'FEB', 'MAR', 'APR', 'MAY', 'JUN', 'JUL', 'AUG', 'SEP', 'OCT', 'NOV', 'DEC'];
const esc = (s) => s.replace(/[&<>"]/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;' })[c]);

// each character as one path in font pixels (5 x 8), used where it's needed
function glyph(code) {
  let d = '';
  for (let x = 0; x < 5; x++) {
    const col = FONT[(code - 32) * 5 + x] || 0;
    for (let y = 0; y < 8; y++) {
      if (!(col >> y & 1)) continue;
      let h = 1;
      while (y + h < 8 && col >> (y + h) & 1) h++;
      d += `M${x} ${y}h1v${h}h-1z`;
      y += h - 1;
    }
  }
  return d;
}
export const textW = (s, k) => s.length * 6 * k - k;

// the board as SVG, from /stats's board ({ games: [{ game, top }], fish: [...] }). No count of players: kept five
// minutes, it disagreed with the leaderboard page's own, kept one (Fedor, 2026-11-18: "We dont need two, we need
// one correct one"); the page and the README's badge show /stats's
export function boardSvg(board) {
  const used = new Set();
  const out = [];
  const text = (x, y, s, k, fill) => {
    s = String(s).toUpperCase();
    let t = '';
    for (let i = 0; i < s.length; i++) {
      const c = s.charCodeAt(i);
      if (c <= 32 || c > 126) continue;
      used.add(c);
      t += `<use href="#c${c}" x="${(x / k + i * 6).toFixed(2)}" y="${(y / k).toFixed(2)}"/>`;
    }
    if (t) out.push(`<g fill="${fill}" transform="scale(${k})">${t}</g>`);
  };
  const rows = [];
  for (const g of [0, 2, 3, 4, FISH_GAME, 6, 7, 8]) {
    if (g === FISH_GAME) {
      const f = board.fish[0];
      rows.push({ g, name: 'Reel Fishing', best: f ? `${f.lb.toFixed(1)} LB` : null, sub: f ? `${f.shiny ? 'SHINY ' : ''}${f.kind}, BIGGEST FISH` : null, e: f });
    } else {
      const e = (board.games.find((o) => o.game === g) || { top: [] }).top[0];
      rows.push({ g, name: GAMES[g].name, best: e ? GAMES[g].show(e.score) : null, sub: e ? GAMES[g].sub : null, e });
    }
  }
  const W = 760, ROW = 84, TOPY = 132, H = TOPY + rows.length * ROW + 50;
  out.push(`<rect width="${W}" height="${H}" rx="22" fill="${RIM}"/><rect x="5" y="5" width="${W - 10}" height="${H - 10}" rx="18" fill="${BG}"/>`);
  out.push(`<rect x="12" y="12" width="${W - 24}" height="${H - 24}" rx="14" fill="none" stroke="${LINE}" stroke-width="2"/>`);
  // the title on a wooden board, as the game's screens have theirs
  const tw = textW('LEADERBOARD', 4) + 44;
  out.push(`<rect x="32" y="30" width="${tw + 8}" height="52" fill="#3a2414"/><rect x="36" y="34" width="${tw}" height="44" fill="#8a5a30"/><rect x="40" y="36" width="${tw - 8}" height="3" fill="#b07a45"/>`);
  text(58, 42, 'LEADERBOARD', 4, CREAM);
  for (const [x, s] of [[116, 'GAME'], [380, 'BEST'], [566, 'TAG'], [640, 'DATE']]) text(x, 104, s, 2, MUTED);
  out.push(`<rect x="28" y="124" width="${W - 56}" height="2" fill="${LINE}"/>`);
  rows.forEach((r, i) => {
    const y = TOPY + i * ROW;
    if (i % 2) out.push(`<rect x="20" y="${y - 2}" width="${W - 40}" height="${ROW - 4}" fill="${ALT}"/>`);
    if (CART[r.g]) out.push(`<image x="40" y="${y + 4}" width="${CART_W}" height="${CART_H}" href="${CART[r.g]}"/>`);
    text(116, y + 26, r.name, 2, WHITE);
    if (!r.e) {
      text(380, y + 26, 'NONE YET', 2, MUTED);
      return;
    }
    text(380, y + (r.sub ? 12 : 20), r.best, 3, GOLD);
    if (r.sub) text(380, y + 44, r.sub, r.sub.length > 14 ? 1 : 2, MUTED);
    const tag = tagShown(r.e.tag), w = textW(tag, 2);
    out.push(`<rect x="564" y="${y + 16}" width="${w + 20}" height="30" rx="6" fill="${RIM}"/><rect x="566" y="${y + 18}" width="${w + 16}" height="26" rx="5" fill="#f4ecd8"/>`);
    text(574, y + 24, tag, 2, INK);
    const [yy, mm, dd] = r.e.day.split('-');
    text(640, y + 24, `${MONTH[+mm - 1]} ${+dd}`, 2, WHITE);
    text(640, y + 44, yy, 2, MUTED);
  });
  const foot = 'LIVE FROM THE GAMES / TOP 10 OF EACH AND EVERY FISH\'S RECORD: FEDORSOSNIN.GITHUB.IO/CATCH151/LEADERBOARD';
  text((W - textW(foot, 1)) / 2, H - 32, foot, 1, MUTED);
  const defs = [...used].map((c) => `<path id="c${c}" d="${glyph(c)}"/>`).join('');
  return `<svg xmlns="http://www.w3.org/2000/svg" width="${W}" height="${H}" viewBox="0 0 ${W} ${H}" shape-rendering="crispEdges">` +
    `<title>Catch 151 leaderboard</title><defs>${defs}</defs>${out.join('')}</svg>`;
}
