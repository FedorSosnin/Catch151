// The world board (Fedor, 2026-11-16: "a section in the Repo showing live players, biggest fish, longest run, highest
// pinball score, etc." with each best's "score, date, and tag"). The relay keeps each game's top ten: the score, the
// gamer tag it came with and the day, one entry a device (by a hash of its id: nothing about who or where). The README
// shows it as a picture (/board.svg) and the website's scores page reads it from /stats.
import { FONT, CART, CART_W, CART_H } from './board_art.js';

export const BM_SCORE = 18;  // src/buddy.h
export const TOP = 10;       // kept a game
// the games on the board, as the Games menu numbers them (src/menus_v2.h V2_CART_GAME), with the most a real one can
// score (the game's own limits: Berry Drop's is a byte, Dungeon Crawler's floor too, times in seconds), and how a
// score is shown. Reel Fishing's place is its biggest fish (BM_FISH, index.js)
export const GAMES = {
  0: { name: 'Berry Drop', max: 255, show: (v) => num(v) },
  2: { name: 'Rock Hopper', max: 65535, show: (v) => mmss(v), sub: 'LONGEST RUN' },
  3: { name: 'Dungeon Crawler', max: 255, show: (v) => `FLOOR ${v}` },
  4: { name: 'PikaSurf', max: 65535, show: (v) => mmss(v), sub: 'LONGEST RIDE' },
  6: { name: 'Pinball', max: 9999999, show: (v) => num(v) },
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
  if (v.getUint8(5) !== BM_SCORE) return null;
  const game = v.getUint8(18), score = v.getUint32(14, true), g = GAMES[game];
  if (!g || score < 1 || score > g.max) return null;
  return { game, score, tag: tagOf(new Uint8Array(data, 34, 12)), id: idHash(v.getUint32(6, true)) };
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

// the board as SVG: stats is /stats's { online, board } (board: { games: [{ game, top }], fish: [...] })
export function boardSvg(online, board) {
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
  for (const g of [0, 2, 3, 4, FISH_GAME, 6]) {
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
  const tw = textW('WORLD BOARD', 4) + 44;
  out.push(`<rect x="32" y="30" width="${tw + 8}" height="52" fill="#3a2414"/><rect x="36" y="34" width="${tw}" height="44" fill="#8a5a30"/><rect x="40" y="36" width="${tw - 8}" height="3" fill="#b07a45"/>`);
  text(58, 42, 'WORLD BOARD', 4, CREAM);
  const live = `${online} PLAYING NOW`, lx = W - 40 - textW(live, 2);
  out.push(`<circle cx="${lx - 19}" cy="54" r="7" fill="#58b868"/>`);
  text(lx, 47, live, 2, '#a0dcaa');
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
  const foot = 'LIVE FROM THE GAMES / TOP 10 OF EACH: FEDORSOSNIN.GITHUB.IO/CATCH151/SCORES';
  text((W - textW(foot, 1)) / 2, H - 32, foot, 1, MUTED);
  const defs = [...used].map((c) => `<path id="c${c}" d="${glyph(c)}"/>`).join('');
  return `<svg xmlns="http://www.w3.org/2000/svg" width="${W}" height="${H}" viewBox="0 0 ${W} ${H}" shape-rendering="crispEdges">` +
    `<title>${esc(`Catch 151 world board: ${online} playing now`)}</title><defs>${defs}</defs>${out.join('')}</svg>`;
}
