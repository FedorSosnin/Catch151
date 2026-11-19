// The leaderboard's rules (src/board.js): run with  node --test relay/test/*.test.mjs
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { readFileSync } from 'node:fs';
import { STRONG, MILD, tagRude, tagShown, tagOf, scoreReport, boardAdd, recordAdd, boardSvg, idHash, BM_SCORE, TOP, topReply, RELAY_ID, SEASON, seasonOf, GAMES } from '../src/board.js';
import { CART } from '../src/board_art.js';
import { boardPublic, fishReport, FISH, liveSince } from '../src/index.js';

const HERE = new URL('.', import.meta.url).pathname;

// a 65-byte buddy message as the game builds it (src/buddy.h)
function msg(type, { from = 1234, session = 0, kind = 0, dex = 0, shiny = 0, hp = 0, nick = '', season = SEASON } = {}) {
  const b = new ArrayBuffer(65), v = new DataView(b);
  'PKG1'.split('').forEach((c, i) => v.setUint8(i, c.charCodeAt(0)));
  v.setUint8(5, type);
  v.setUint32(6, from, true);
  v.setUint32(14, session, true);
  v.setUint8(18, kind);
  v.setUint8(20, season);  // the board's mark (src/buddy.h BOARD_SEASON)
  v.setInt16(22, dex, true);
  v.setUint8(25, shiny);
  v.setUint16(26, hp, true);
  [...nick].forEach((c, i) => v.setUint8(34 + i, c.charCodeAt(0)));
  return b;
}

test('the rude-word lists are the game\'s (src/gamertag.h)', () => {
  const h = readFileSync(HERE + '../../src/gamertag.h', 'utf8');
  const list = (name) => [...h.split(name)[1].split('};')[0].matchAll(/"([A-Z]+)"/g)].map((m) => m[1]);
  assert.deepEqual(STRONG, list('GAMER_TAG_STRONG[] = {'));
  assert.deepEqual(MILD, list('GAMER_TAG_MILD[] = {'));
});

test('rude tags are caught through their stand-ins, and show as ****', () => {
  for (const t of ['FUCK', 'fuck', 'F.CK', 'F*CK', 'SH1T', '5H1T', 'N4ZI', '$EX', 'XFUK', 'ASS', 'A55', 'a.s.s']) assert.ok(tagRude(t), t);
  for (const t of ['PIKA', 'FED', 'ASH', 'MO', 'BASS', 'PASS', 'KAI', 'ZOE', 'RED', 'GG', 'TITO', 'SAM', '1234', 'LOL', 'MEW!']) assert.ok(!tagRude(t), t);
  assert.equal(tagShown('SH1T'), '****');
  assert.equal(tagShown(''), '----');
  assert.equal(tagShown('PIKA'), 'PIKA');
});

test('a tag is read from the message: printable, four at most, trimmed', () => {
  assert.equal(tagOf(new Uint8Array([80, 73, 75, 65, 67, 72, 85, 0, 0, 0, 0, 0])), 'PIKA');
  assert.equal(tagOf(new Uint8Array([32, 65, 1, 66, 0])), 'AB');
  assert.equal(tagOf(new Uint8Array(12)), '');
});

test('a score message is read, and one past what a game can score is ignored', () => {
  assert.deepEqual(scoreReport(msg(BM_SCORE, { kind: 6, session: 48250, nick: 'FED' })), { game: 6, score: 48250, tag: 'FED', id: idHash(1234) });
  assert.equal(scoreReport(msg(BM_SCORE, { kind: 0, session: 256 })), null);   // Berry Drop's score is a byte
  assert.equal(scoreReport(msg(BM_SCORE, { kind: 1, session: 9 })), null);     // Who's That? isn't on the board
  assert.equal(scoreReport(msg(BM_SCORE, { kind: 3, session: 0 })), null);
  assert.equal(scoreReport(msg(17, { kind: 6, session: 5 })), null);           // not a score
  assert.equal(scoreReport(msg(BM_SCORE, { kind: 6, session: 5, season: 0 })), null);  // unmarked: 2.2.9's all-time records
  assert.equal(scoreReport(new ArrayBuffer(64)), null);
  assert.deepEqual(scoreReport(msg(BM_SCORE, { kind: 7, session: 4660, nick: 'ZOE' })), { game: 7, score: 4660, tag: 'ZOE', id: idHash(1234) });  // BerryMatch
  assert.equal(scoreReport(msg(BM_SCORE, { kind: 7, session: 1000000 })), null);  // far past any game of 20 moves
  assert.equal(scoreReport(msg(BM_SCORE, { kind: 8, session: 50 })), null);       // no such game
});

test('the board\'s games are the ones the game sends (scoreReportAll() in Catch151.ino), each with its cartridge', () => {
  const ino = readFileSync(HERE + '../../Catch151.ino', 'utf8');
  const match = +ino.match(/#define GAME_MATCH (\d+)/)[1];
  const sent = ino.match(/static const uint8_t G\[\] = \{([^}]*)\}/)[1].split(',').map((s) => s.trim() === 'GAME_MATCH' ? match : +s);
  assert.deepEqual(Object.keys(GAMES).map(Number), sent);
  assert.equal(GAMES[match].name, 'BerryMatch');
  for (const g of [...sent, 5]) assert.ok(CART[g]?.startsWith('data:image/png;base64,'), `game ${g}'s cartridge`);
});

test('a fish message carries the tag; the fish itself is checked as before', () => {
  assert.deepEqual(fishReport(msg(17, { dex: 130, hp: 2500, nick: 'ZOE' })), { kg10: 2500, dex: 130, shiny: false });
  assert.equal(fishReport(msg(17, { dex: 130, hp: 2900 })), null);  // heavier than a Gyarados can be
});

test('a top ten: one entry a device, its new tag whatever the score, a higher score only, the first to a tie first', () => {
  const list = [];
  assert.ok(boardAdd(list, { id: 'a', tag: 'AAA', score: 50, day: '2026-11-15' }));
  assert.ok(boardAdd(list, { id: 'b', tag: 'BBB', score: 70, day: '2026-11-16' }));
  assert.deepEqual(list.map((e) => e.id), ['b', 'a']);
  assert.ok(!boardAdd(list, { id: 'a', tag: 'AAA', score: 40, day: '2026-11-16' }));  // lower, same tag: nothing
  assert.ok(boardAdd(list, { id: 'a', tag: 'NEW', score: 40, day: '2026-11-16' }));   // a new tag: taken
  assert.equal(list[1].tag, 'NEW');
  assert.equal(list[1].score, 50);
  assert.ok(boardAdd(list, { id: 'a', tag: 'NEW', score: 90, day: '2026-11-17' }));   // higher: first now
  assert.deepEqual(list.map((e) => [e.id, e.score, e.day]), [['a', 90, '2026-11-17'], ['b', 70, '2026-11-16']]);
  boardAdd(list, { id: 'c', tag: 'CCC', score: 70, day: '2026-11-17' });             // a tie: after the first to it
  assert.deepEqual(list.map((e) => e.id), ['a', 'b', 'c']);
  for (let i = 0; i < 20; i++) boardAdd(list, { id: 'x' + i, tag: '', score: 100 + i, day: '2026-11-18' });
  assert.equal(list.length, TOP);
  assert.equal(list[0].score, 119);
  assert.ok(!boardAdd(list, { id: 'y', tag: '', score: 1, day: '2026-11-18' }));     // too low for the ten
});

test('the board as /stats shows it hides ids and rude tags, and the picture is drawn', () => {
  const scores = { 6: [{ id: 'a', tag: 'SH1T', score: 48250, day: '2026-11-16' }], 2: [{ id: 'b', tag: 'KAI', score: 222, day: '2026-11-15' }] };
  const fishTop = [{ id: 'c', tag: 'ZOE', score: 2500, dex: 130, shiny: false, day: '2026-11-16' }];
  const b = boardPublic(scores, fishTop);
  const pin = b.games.find((g) => g.game === 6);
  assert.deepEqual(pin.top, [{ tag: '****', score: 48250, shown: '48,250', day: '2026-11-16' }]);
  assert.equal(b.games.find((g) => g.game === 2).top[0].shown, '3:42');
  assert.deepEqual(b.fish, [{ tag: 'ZOE', lb: 551.2, kind: 'Gyarados', shiny: false, day: '2026-11-16' }]);
  assert.ok(!JSON.stringify(b).includes('"id"'));
  const svg = boardSvg(b);
  assert.ok(svg.startsWith('<svg') && svg.endsWith('</svg>'));
  assert.ok(svg.includes('<image') && svg.includes('id="c87"'));  // a W, drawn
  assert.ok(!/playing/i.test(svg) && !svg.includes('<circle'));  // no count of players: the page has the one
  assert.equal(svg.match(/<image /g).length, 7);  // seven games, each with its cartridge
  assert.ok(svg.includes(`href="${CART[7]}"`));   // BerryMatch's among them
  assert.ok(svg.includes('height="770"'));        // the page's <img> is sized so (web/leaderboard/index.html)
});

test('BerryMatch\'s row: none yet, then its best with thousands commas, as Pinball\'s', () => {
  const none = boardPublic({}, []);
  assert.deepEqual(none.games.find((g) => g.game === 7), { game: 7, name: 'BerryMatch', top: [] });
  const b = boardPublic({ 7: [{ id: 'a', tag: 'ZOE', score: 12480, day: '2026-11-19' }] }, []);
  assert.deepEqual(b.games.find((g) => g.game === 7).top, [{ tag: 'ZOE', score: 12480, shown: '12,480', day: '2026-11-19' }]);
  const gold = (svg) => svg.split('<g fill="#f0c04a"').length - 1;  // a best, drawn in gold
  assert.equal(gold(boardSvg(none)), 0);
  assert.equal(gold(boardSvg(b)), 1);
  const page = readFileSync(HERE + '../../web/leaderboard/index.html', 'utf8');
  assert.match(page, /width="760" height="770"/);
});

test('the first of a game is told so: a BM_SCORE from the relay to it, as src/buddy.h reads one', () => {
  const v = new DataView(topReply(1234, 6, 48250));
  assert.equal(v.byteLength, 65);
  assert.equal(String.fromCharCode(v.getUint8(0), v.getUint8(1), v.getUint8(2), v.getUint8(3)), 'PKG1');
  assert.equal(v.getUint8(5), BM_SCORE);
  assert.equal(v.getUint32(6, true), RELAY_ID);
  assert.equal(v.getUint32(10, true), 1234);
  assert.equal(v.getUint32(14, true), 48250);
  assert.equal(v.getUint8(18), 6);
  const h = readFileSync(HERE + '../../src/buddy.h', 'utf8');
  assert.match(h, /#define BUDDY_RELAY_ID 0xFFFFFFFFu/);
});

test('the board started over: only a fish or score with the mark counts, and the mark is the game\'s', () => {
  assert.equal(seasonOf(msg(17, { dex: 130, hp: 2500 })), SEASON);
  assert.equal(seasonOf(msg(17, { dex: 130, hp: 2500, season: 0 })), 0);
  const h = readFileSync(HERE + '../../src/buddy.h', 'utf8');
  assert.match(h, new RegExp('#define BOARD_SEASON ' + SEASON + '\\b'));
});

test('the fish are the game\'s: each kind, its name and its weight (src/fishing.h FISH_KINDS)', () => {
  const h = readFileSync(HERE + '../../src/fishing.h', 'utf8');
  const kinds = [...h.matchAll(/\{ (\d+), (\d+), FC_\w+, \d+ \},\s*\/\/ (\w+)/g)].map(([, d, kg, name]) => [+d, name, +kg]);
  assert.equal(kinds.length, 23);
  assert.deepEqual(Object.fromEntries(kinds.map(([d, name, kg]) => [d, [name, kg]])), FISH);
});

test('each kind of fish\'s record: the heavier takes it, a tie stays with the first, its holder\'s new tag comes along', () => {
  const r = {};
  assert.ok(recordAdd(r, 129, { id: 'a', tag: 'ZOE', score: 90, day: '2026-11-17' }));
  assert.ok(!recordAdd(r, 129, { id: 'b', tag: 'KAI', score: 90, day: '2026-11-18' }));  // a tie
  assert.ok(!recordAdd(r, 129, { id: 'b', tag: 'KAI', score: 80, day: '2026-11-18' }));
  assert.ok(!recordAdd(r, 129, { id: 'a', tag: 'ZOE', score: 70, day: '2026-11-18' }));
  assert.ok(recordAdd(r, 129, { id: 'a', tag: 'ZO3', score: 70, day: '2026-11-18' }));  // a new tag, the record as it was
  assert.deepEqual(r[129], { id: 'a', tag: 'ZO3', score: 90, day: '2026-11-17' });
  assert.ok(recordAdd(r, 129, { id: 'b', tag: 'KAI', score: 110, day: '2026-11-18' }));
  assert.equal(r[129].tag, 'KAI');
  assert.ok(recordAdd(r, 130, { id: 'a', tag: 'ZOE', score: 2000, shiny: true, day: '2026-11-18' }));
  const b = boardPublic({}, [], r);
  assert.equal(b.kinds.length, 23);
  assert.deepEqual(b.kinds.map((k) => k.dex), Object.keys(FISH).map(Number));  // every kind, in Pokedex order
  assert.deepEqual(b.kinds.find((k) => k.dex === 129), { dex: 129, kind: 'Magikarp', lb: 24.3, shiny: undefined, tag: 'KAI', day: '2026-11-18' });
  assert.deepEqual(b.kinds.find((k) => k.dex === 90), { dex: 90, kind: 'Shellder', lb: null });  // none landed yet
  assert.equal(b.kinds.find((k) => k.dex === 130).shiny, true);
  assert.ok(!JSON.stringify(b).includes('"id"'));
});

test('the count of players: only connections heard from in the last two minutes (a ping is every 45 s)', () => {
  const now = 1_800_000_000_000;
  assert.ok(liveSince(now - 45_000, 0, now));            // pinged a moment ago
  assert.ok(liveSince(0, now - 5_000, now));             // just connected, or just sent something
  assert.ok(!liveSince(now - 180_000, now - 300_000, now));  // Wi-Fi gone, the connection not closed yet
  assert.ok(!liveSince(0, 0, now));
});
