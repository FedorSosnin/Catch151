#!/usr/bin/env python3
"""Reel Fishing's medals, MOCKUPS: three trophies drawn the trophy room's way (tools/trophies.py: the game's outline,
five shades lit from the top left, a white glint), small for the room (2 px a pixel) and big for a card (3 px).
They're things, not characters: a wicker creel, a gold scale, a gold fish mounted on a plaque.

  python3 tools/fishing_trophies.py         # render build/fishing_trophies.png: each won and not, small and big
  python3 tools/fishing_trophies.py emit    # ...and write src/fishing_trophies_mock.h for the FISHMOCK screens
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import trophies as T  # noqa: E402
from trophies import Cv, C, K, WH, GOLD, WOOD, RED, CREAM, R, part, pillow  # noqa: E402

WICKER = R('#fff4d0', '#ecd08a', '#cfa860', '#a07c3c', '#6e5226')
LEATHER = R('#c89070', '#9a6040', '#764428', '#542e1a', '#381c10')
FACE = R('#ffffff', '#fbf8ee', '#ece4d0', '#c8bea4', '#9a907a')
SPARK = C('#ffffff', 255, True)  # a sparkle: only on a medal won


def grid(W, H, f, fn):
    """the pixels whose centres, in design units (f pixels a unit), fn holds for"""
    return {(x, y) for y in range(H) for x in range(W) if fn((x + .5) / f, (y + .5) / f)}


def shade_box(m, rp, x0, x1, y0, y1, lit_top=True):
    """a flat box's shade: lit on its top and left, darker to its right and bottom"""
    def f(x, y):
        u = (x - x0) / max(1, x1 - x0)
        v = (y - y0) / max(1, y1 - y0)
        lv = 1 if (u < .22 or (lit_top and v < .14)) else 2 if u < .7 else 3
        if v > .86:
            lv = min(4, lv + 1)
        return rp[lv]
    return f


# ---------------------------------------------------------------- 0: Full Net, a wicker creel (all 5 landed in a game)
def creel(big):
    f = 2 if big else 1
    W, H = 22 * f, 21 * f
    cv = Cv(W, H)
    # the strap, over the top from one side to the other
    strap = grid(W, H, f, lambda x, y: y < 8.2 and abs(math.hypot(x - 11, (y - 9) * 1.15) - 8.2) < .62)
    cv.over(part(W, H, strap, lambda x, y: LEATHER[1] if y < 3 * f else LEATHER[2]))
    # the body: a box narrowing a little to its foot, woven in two tones, a darker band across it
    body = grid(W, H, f, lambda x, y: 9.5 <= y < 20 and 2.5 + (y - 9.5) * .14 <= x < 19.5 - (y - 9.5) * .14)

    def weave(x, y):
        u, v = x / f, y / f
        lv = 1 + (int(u) + int(v)) % 2
        if u > 14:
            lv += 1
        if 14 <= v < 15.2:
            lv = 3 if u < 14 else 4
        if v >= 19:
            lv = min(4, lv + 1)
        return WICKER[lv]
    cv.over(part(W, H, body, weave))
    # the lid: a rounded slab on top
    lid = grid(W, H, f, lambda x, y: 7 <= y < 10 and 1.6 <= x < 20.4 and not (y < 7.8 and (x < 2.6 or x > 19.4)))
    cv.over(part(W, H, lid, lambda x, y: WICKER[1] if y < 8 * f else WICKER[2] if x < 15 * f else WICKER[3]))
    # its gold catch, and a fin's tip poking out from under the lid: something's in there
    catch = grid(W, H, f, lambda x, y: 9.6 <= x < 12.4 and 8.6 <= y < 11.6)
    cv.over(part(W, H, catch, lambda x, y: GOLD[1] if y < 9.8 * f else GOLD[2]))
    tail = grid(W, H, f, lambda x, y: 16 <= x < 19.5 and 4.6 <= y < 7 and (x - 16) > (7 - y) * .9 - .4)
    cv.over(part(W, H, tail, lambda x, y: GOLD[1] if x < 17.5 * f else GOLD[2]))
    for x, y in ((3 * f, 8 * f), (10 * f, 9 * f)):
        cv.put(x, y, WH)
    if big:
        cv.put(17 * f, 5 * f, WH)
    for x, y in ((1, 3), (W - 2, 12)):
        cv.put(x, y, SPARK)
    return cv


# ---------------------------------------------------------------- 1: Gold Angler, a gold scale (the 300 kg line)
def scale(big):
    f = 2 if big else 1
    W, H = 22 * f, 21 * f
    cv = Cv(W, H)
    feet = grid(W, H, f, lambda x, y: 18.5 <= y < 20.5 and (4 <= x < 7 or 15 <= x < 18))
    cv.over(part(W, H, feet, GOLD[3]))
    body = grid(W, H, f, lambda x, y: 8 <= y < 19 and 3 <= x < 19 and not ((y < 8.9 or y > 18.1) and (x < 3.9 or x > 18.1)))
    pillow(cv, body, GOLD, sq=.8)
    cv.over(part(W, H, body, lambda x, y: cv.get(x, y)))
    # the dial: a cream face, ticks round it, the needle past the top (the gold line)
    cx, cy, r = 11 * f, 13.6 * f, 3.9 * f
    for x in range(W):
        for y in range(H):
            d = math.hypot(x + .5 - cx, y + .5 - cy)
            if d < r:
                cv.put(x, y, FACE[1] if d < r * .55 or (x < cx and y < cy) else FACE[2])
            elif d < r + (1.2 if big else .8):
                cv.put(x, y, K)
    for k in range(8):
        a = k * math.pi / 4 - math.pi / 2
        cv.put(int(cx + (r - (1.6 if big else 1)) * math.cos(a)), int(cy + (r - (1.6 if big else 1)) * math.sin(a)), K)
    a = -math.pi / 2 + 0.75
    for i in range(int(r * .85) + 1):
        cv.put(int(cx + i * math.cos(a)), int(cy + i * math.sin(a)), RED[2])
    cv.put(int(cx), int(cy), K)
    # the post and the platform it carries
    post = grid(W, H, f, lambda x, y: 9.6 <= x < 12.4 and 5.6 <= y < 8.2)
    cv.over(part(W, H, post, GOLD[3]))
    plat = grid(W, H, f, lambda x, y: 3.6 <= y < 6 and 1 <= x < 21)
    cv.over(part(W, H, plat, lambda x, y: GOLD[0] if y < 4.4 * f and x % (5 * f) == 2 * f else GOLD[1] if y < 4.6 * f else GOLD[2] if x < 16 * f else GOLD[3]))
    for x, y in ((4 * f, 9 * f + 1), (2 * f, 4 * f)):
        cv.put(x, y, WH)
    for x, y in ((0, 1), (W - 2, 9 * f)):
        cv.put(x, y, SPARK)
    return cv


# ---------------------------------------------------------------- 2: The Big One, a gold fish on a plaque (a giant)
def mount(big):
    f = 2 if big else 1
    W, H = 26 * f, 21 * f
    cv = Cv(W, H)
    # the plaque: a board with rounded corners, a bevel lit on its top and left, a few grain lines, a brass plate
    def inboard(x, y, g=0.0):
        x0, x1, y0, y1, rr = 1 + g, 25 - g, 2.5 + g, 20 - g, 2.2
        if not (x0 <= x < x1 and y0 <= y < y1):
            return False
        cx, cy = min(max(x, x0 + rr), x1 - rr), min(max(y, y0 + rr), y1 - rr)
        return math.hypot(x - cx, y - cy) <= rr
    plq = grid(W, H, f, inboard)
    inner = grid(W, H, f, lambda x, y: inboard(x, y, 1.2))

    def wood(x, y):
        u, v = x / f, y / f
        if (x, y) not in inner:
            return WOOD[1] if (u + v < 13 or v < 3.8) and not (u > 23.6 or v > 18.6) else WOOD[4]
        return WOOD[3] if (abs(v - 7.5) < .5 / f and 3 < u < 11) or (abs(v - 14.5) < .5 / f and 15 < u < 23) else WOOD[2]
    cv.over(part(W, H, plq, wood))
    plate = grid(W, H, f, lambda x, y: 9.5 <= x < 16.5 and 16.6 <= y < 18.6)
    cv.over(part(W, H, plate, lambda x, y: GOLD[1] if y < 17.4 * f else GOLD[2]))
    # the fish leaping across it: a round body, a crown of fin on top, a fanned tail, a whisker; all gold
    fish = grid(W, H, f, lambda x, y: ((x - 12) / 7.2) ** 2 + ((y - 9.6 - .05 * (x - 12) ** 2 / 3) / 3.9) ** 2 <= 1)
    tail = grid(W, H, f, lambda x, y: 17.8 <= x < 23.5 and abs(y - 7.6 + (x - 17.8) * .25) < (x - 17.8) * .62 + .6 and not (x > 21.2 and abs(y - 7.6 + (x - 17.8) * .25) < 1.1))
    fin = grid(W, H, f, lambda x, y: 8 <= x < 15 and 4 <= y < 6.6 and (y - 4) > abs(((x - 8) % 2.33) - 1.16) * 1.5 - .2)
    cv.over(part(W, H, fin | tail, lambda x, y: GOLD[2] if y < 6 * f else GOLD[3]))
    body = Cv(W, H)
    pillow(body, fish, GOLD, sq=.9)
    cv.over(body.outline())
    # its eye, its mouth, a whisker down from it
    ex, ey = int(7.2 * f), int(8.4 * f)
    cv.put(ex, ey, K)
    if big:
        cv.put(ex + 1, ey, K)
        cv.put(ex, ey + 1, K)
        cv.put(ex + 1, ey + 1, K)
        cv.put(ex, ey, WH)
    for i in range(int(2.4 * f)):
        cv.put(int(5.4 * f) - i // 2, int(11.2 * f) + i, GOLD[3])
    for i in range(int(1.6 * f)):
        cv.put(int(5.2 * f) + i, int(10.4 * f), K)
    for x, y in ((9 * f, 7 * f), (10 * f, 7 * f)):
        cv.put(x, y, WH)
    for x, y in ((0, 0), (W - 1, 18 * f)):
        cv.put(x, y, SPARK)
    return cv


ARTS = [creel, scale, mount]
NAMES = ['FULL NET', 'GOLD ANGLER', 'THE BIG ONE']


def art(i, big):
    return ARTS[i](big).trim()


def sheet(path):
    cell = 180
    IW, IH = cell * 3, 330
    img = bytearray(bytes(T.hexrgb('#8a5a3a')) * (IW * IH))
    for i in range(3):
        x0 = i * cell
        sm, bg = art(i, False), art(i, True)
        T.paint(img, IW, sm, x0 + 14, 10, 2, True)
        T.paint(img, IW, sm, x0 + 14 + sm.w * 2 + 10, 10, 2, False)
        T.paint(img, IW, bg, x0 + (cell - bg.w * 3) // 2, 70 + (250 - bg.h * 3), 3, True)
    ppm = path[:-4] + '.ppm'
    with open(ppm, 'wb') as fh:
        fh.write(b'P6 %d %d 255\n' % (IW, IH) + bytes(img))
    import subprocess
    subprocess.run(['magick', ppm, path], check=True)
    os.remove(ppm)
    print('saved', path)


def emit(path='src/fishing_trophies_mock.h'):
    pal, idx = [None], {}

    def pi(c):
        if c not in idx:
            idx[c] = len(pal)
            pal.append(c)
        return idx[c]
    pi(K)
    data = []
    for big in (False, True):
        for i in range(3):
            cv = art(i, big)
            data.append((i, big, cv.w, cv.h, [0 if c is None else pi(c) for row in cv.p for c in row]))
    out = ['#pragma once\n// GENERATED by tools/fishing_trophies.py (MOCKUPS: Reel Fishing\'s three medals). Run:\n'
           '//   python3 tools/fishing_trophies.py emit\n// As trophies_v2.h: each pixel an index in FT_PAL (0 nothing, 1 the outline); '
           'FT_PAL_A 255 solid; FT_PAL_WON drawn only once won.\n\n']
    out.append('static const uint32_t FT_PAL[%d] = {\n' % len(pal))
    for i in range(0, len(pal), 8):
        out.append('  ' + ', '.join('0x000000' if c is None else '0x' + c[0][1:].upper() for c in pal[i:i + 8]) + ',\n')
    out.append('};\nstatic const uint8_t FT_PAL_A[%d] = { %s };\n' % (len(pal), ', '.join('0' if c is None else str(c[1]) for c in pal)))
    out.append('static const uint8_t FT_PAL_WON[%d] = { %s };\n\n' % (len(pal), ', '.join('0' if c is None else str(int(c[2])) for c in pal)))
    for i, big, w, h, px in data:
        out.append('static const uint8_t FT_PX_%s%d[%d] = {\n' % ('B' if big else 'S', i, w * h))
        for j in range(0, len(px), w):
            out.append('  ' + ','.join(str(v) for v in px[j:j + w]) + ',\n')
        out.append('};\n')
    for big in (False, True):
        out.append('static const TrArt FT_ART_%s[3] = {\n' % ('BIG' if big else 'SMALL'))
        for i, b, w, h, px in data:
            if b == big:
                out.append('  { %d, %d, FT_PX_%s%d },\n' % (w, h, 'B' if big else 'S', i))
        out.append('};\n')
    open(path, 'w').write(''.join(out))
    print('saved %s: %d colours' % (path, len(pal)))


if __name__ == '__main__':
    os.makedirs('build', exist_ok=True)
    sheet('build/fishing_trophies.png')
    if 'emit' in sys.argv:
        emit()
