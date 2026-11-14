#!/usr/bin/env python3
"""PARKED (Fedor, 2026-11-13: skip medals for now, since the other minigames have none). Reel Fishing's medals,
MOCKUPS: three trophies made the trophy room's way (tools/trophies.py), and its golden
Pikachu's way in particular: each piece shaded round in gold, the lines between pieces cut in deep gold, ink only round
the outside, a glint. Small for the room (2 px a pixel), big for a card (3 px).

  FULL NET     a gold landing net on a stand, a golden fish seen through its mesh (all 5 landed in one game)
  GOLD ANGLER  a gold scale, its needle past the top (300 kg in one game)
  THE BIG ONE  a golden Lapras on a sea-blue cushion (a giant landed; Fedor: "a golden Lapras in the same way we have a
               golden pikachu")

  python3 tools/fishing_trophies.py         # render build/fishing_trophies.png: each won and not, small and big
  python3 tools/fishing_trophies.py emit    # ...and write src/fishing_trophies_mock.h for the FISHMOCK screens
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import trophies as T  # noqa: E402
from trophies import Cv, C, K, GOLD, WOOD, RED, BLUE, R, mask, part, pillow, glint  # noqa: E402

FACE = R('#ffffff', '#fbf8ee', '#ece4d0', '#c8bea4', '#9a907a')
CUT = C('#5a3206')                # a hollow cut in the gold: eyes
CORD = C('#f4ecd8', 215)          # the net's cord: a little see-through, so it takes no outline of its own
SPARK = C('#ffffff', 255, True)   # a sparkle, only on a medal won


class Shapes:
    """the Pikachu's helpers: shapes in design units from an origin, f pixels a unit, on a W x H canvas"""

    def __init__(s, W, H, f, ox=0.0, oy=0.0):
        s.W, s.H, s.f, s.ox, s.oy = W, H, f, ox, oy

    def m(s, fn):
        return mask(s.W, s.H, lambda u, v: fn(u - s.ox, v - s.oy), s.f)

    def ell(s, ex, ey, rx, ry):
        return s.m(lambda u, v: ((u - ex) / rx) ** 2 + ((v - ey) / ry) ** 2 <= 1)

    def seg(s, x0, y0, x1, y1, wd):
        def fn(u, v):
            dx, dy = x1 - x0, y1 - y0
            t = ((u - x0) * dx + (v - y0) * dy) / (dx * dx + dy * dy)
            return -.05 <= t <= 1.05 and abs((u - x0) * dy - (v - y0) * dx) / math.hypot(dx, dy) <= wd / 2
        return s.m(fn)

    def tri(s, a, b, c):
        def fn(u, v):
            def side(p, q):
                return (q[0] - p[0]) * (v - p[1]) - (q[1] - p[1]) * (u - p[0])
            s1, s2, s3 = side(a, b), side(b, c), side(c, a)
            return (s1 >= 0 and s2 >= 0 and s3 >= 0) or (s1 <= 0 and s2 <= 0 and s3 <= 0)
        return s.m(fn)

    def box(s, x0, y0, x1, y1, r=0.0):
        def fn(u, v):
            if not (x0 <= u <= x1 and y0 <= v <= y1):
                return False
            cx, cy = min(max(u, x0 + r), x1 - r), min(max(v, y0 + r), y1 - r)
            return math.hypot(u - cx, v - cy) <= r + 1e-6
        return s.m(fn)

    def piece(s, m, rp=GOLD, shift=0, sq=0.92, hi=True, line=None):
        """a piece shaded round, its edge in line (deep gold: a line cut between pieces), or None"""
        c = Cv(s.W, s.H)
        pillow(c, m, rp, shift=shift, hi=hi, sq=sq)
        return c.outline(line) if line else c


def wood_base(S, x0, x1, y0, y1):
    """dark wood with a gold band along its top, as under the golden Pikachu"""
    f = S.f

    def col(x, y):
        u, v = (x + .5) / f, (y + .5) / f
        if v < y0 + 1.0:
            return GOLD[1] if v < y0 + .5 or not f > 1.2 else GOLD[2]
        return WOOD[3] if u > x1 - 3.5 or v > y1 - .8 else WOOD[2]
    return part(S.W, S.H, S.box(x0, y0, x1, y1, .4), col)


def sparkles(cv, f, pts, big):
    r = 2 if big else 1
    for u, v in pts:
        x, y = int(u * f), int(v * f)
        for d in range(-r, r + 1):
            cv.put(x + d, y, SPARK)
            cv.put(x, y + d, SPARK)


# ---------------------------------------------------------------- 0: FULL NET, a gold landing net with a fish in it
# Seen from a little above: the hoop an oval, the mesh bag hanging under it with a golden fish in it, the handle into
# the base below the bag
def net(big):
    f = 1.65 if big else 1.0
    W, H = (38, 48) if big else (23, 29)
    S = Shapes(W, H, f)
    cv = Cv(W, H)
    cv.over(wood_base(S, 5.0, 18.0, 25.4, 28.4))
    cv.over(part(W, H, S.seg(11.5, 15.0, 11.5, 25.6, 1.9), lambda x, y: WOOD[1] if (x + .5) / f < 11.4 else WOOD[3]))
    cv.over(S.piece(S.box(10.2, 21.4, 12.8, 23.2, .5), sq=.8).outline())  # a gold ferrule on the handle
    hx, hy, rx, ry = 11.5, 5.4, 9.2, 3.2
    bag = S.m(lambda u, v: v >= hy and ((u - hx) / (rx - .9)) ** 2 + ((v - hy) / 12.6) ** 2 <= 1)
    for x, y in bag:  # the bag: the wall seen through it, a little darker
        cv.put(x, y, C('#2a1a10', 70))
    # the fish in it, curled up gold, its tail over the rim's edge
    fish = S.ell(10.4, 10.6, 4.6, 2.8) | S.tri((14.0, 10.4), (17.6, 7.6), (17.4, 13.4))
    cv.over(S.piece(fish, line=GOLD[4]).outline(C('#7e4a08')))
    for x, y in S.ell(7.2, 10.0, .55, .6):
        cv.put(x, y, CUT)
    # the mesh: cords on the diagonals over the bag, and its edge solid
    step = 2.6
    edge = {(x, y) for x, y in bag if any((x + dx, y + dy) not in bag for dx, dy in ((1, 0), (-1, 0), (0, 1)))}
    for x, y in bag:
        u, v = (x + .5) / f, (y + .5) / f
        if (x, y) in edge:
            cv.put(x, y, C('#f4ecd8'))
        elif (u + v * 1.3) % step < .5 or (u - v * 1.3) % step < .5:
            cv.put(x, y, CORD)
    # the hoop: a gold oval, its far side thinner, lit along its top
    ring = S.ell(hx, hy, rx, ry) - S.ell(hx, hy + .35, rx - 1.1, ry - .9)
    cv.over(S.piece(ring, sq=.98).outline())
    glint(cv, [(int(4.6 * f), int(3.6 * f))] + ([(int(4.6 * f) + 1, int(3.6 * f))] if big else []))
    sparkles(cv, f, [(1.6, 11.6), (21.4, 15.0)], big)
    return cv


# ---------------------------------------------------------------- 1: GOLD ANGLER, a gold scale past the gold line
def scale(big):
    f = 1.65 if big else 1.0
    W, H = (38, 42) if big else (23, 26)
    S = Shapes(W, H, f)
    cv = Cv(W, H)
    feet = S.box(4.6, 21.6, 7.8, 24.6, .4) | S.box(15.2, 21.6, 18.4, 24.6, .4)
    cv.over(part(W, H, feet, GOLD[3]))
    cv.over(S.piece(S.box(2.6, 9.0, 20.4, 22.4, 2.4), sq=.85).outline())  # the body
    cv.over(S.piece(S.box(9.8, 6.4, 13.2, 9.4, .3), shift=1, sq=.7).outline())  # its neck
    cv.over(S.piece(S.box(1.0, 4.2, 22.0, 6.9, 1.2), sq=.9).outline())  # the platform
    # the dial: a bezel cut into the gold, a cream face, ticks, the needle past the top
    cx, cy = 11.5, 15.6
    bez = S.ell(cx, cy, 5.2, 5.2)
    face = S.ell(cx, cy, 4.2, 4.2)
    for x, y in bez - face:
        u, v = (x + .5) / f, (y + .5) / f
        cv.put(x, y, GOLD[4] if (u - cx) + (v - cy) > 0 else GOLD[1])
    for x, y in face:
        u, v = (x + .5) / f, (y + .5) / f
        d = (u - cx) * .7 + (v - cy) * .7
        cv.put(x, y, FACE[0] if d < -2.2 else FACE[2] if d > 2.0 else FACE[1])
    for k in range(4):  # ticks at the quarters, the top one the gold line
        a = k * math.pi / 2 - math.pi / 2
        for rr in ((3.0, 3.6) if big else (3.3,)):
            cv.put(int((cx + rr * math.cos(a)) * f), int((cy + rr * math.sin(a)) * f), C('#c07a10') if k == 0 else K)
    a = -math.pi / 2 + .7  # past the top: the gold line
    for i in range(int(3.4 * f * 2)):
        t = i / (3.4 * f * 2) * 3.4
        cv.put(int((cx + t * math.cos(a)) * f), int((cy + t * math.sin(a)) * f), RED[2])
        if big:
            cv.put(int((cx + t * math.cos(a)) * f) + 1, int((cy + t * math.sin(a)) * f), RED[3])
    for x, y in S.ell(cx, cy, .7, .7):
        cv.put(x, y, K)
    glint(cv, [(int(4.4 * f), int(10.6 * f)), (int(3.0 * f), int(4.9 * f))] + ([(int(4.4 * f) + 1, int(10.6 * f))] if big else []))
    sparkles(cv, f, [(1.2, 1.4), (21.6, 10.6)], big)
    return cv


# ---------------------------------------------------------------- 2: THE BIG ONE, a golden Lapras
def lapras(big):
    f = 1.65 if big else 1.0
    W, H = (40, 46) if big else (24, 28)
    S = Shapes(W, H, f)
    cv = Cv(W, H)
    cv.over(wood_base(S, 1.4, 22.6, 23.4, 27.4))
    cush = S.ell(12.0, 22.6, 9.8, 1.9)  # a sea-blue velvet cushion
    c2 = Cv(W, H)
    pillow(c2, cush, BLUE, hi=False)
    cv.over(c2.outline())
    st = Cv(W, H)  # the statue, facing left: its lines cut in deep gold, ink only round the outside
    st.over(S.piece(S.tri((19.6, 19.4), (23.0, 17.0), (21.6, 21.0)), line=GOLD[4]))  # its tail
    st.over(S.piece(S.ell(13.4, 19.0, 8.0, 2.8), shift=1, line=GOLD[4]))  # the body under its shell
    st.over(S.piece(S.ell(14.2, 15.0, 7.4, 5.4), line=GOLD[4]))  # the shell
    for kx, ky in ((10.2, 10.6), (14.0, 9.7), (17.8, 10.6)):  # the shell's knobs
        st.over(S.piece(S.ell(kx, ky, 1.25, 1.15), sq=.8, line=GOLD[4]))
    st.over(S.piece(S.ell(6.8, 19.8, 3.4, 1.25), shift=1, line=GOLD[4]))  # its front flipper
    neck = S.seg(9.2, 16.8, 7.0, 11.8, 3.8) | S.seg(7.0, 12.4, 6.2, 8.2, 3.2)
    st.over(S.piece(neck, line=GOLD[4]))  # the long neck
    st.over(S.piece(S.tri((7.2, 5.0), (8.4, 6.6), (10.6, 4.2)), shift=1, line=GOLD[4]))  # its ear, swept back
    st.over(S.piece(S.ell(5.6, 6.4, 3.0, 2.5), line=GOLD[4]))  # the head, round
    st.over(S.piece(S.ell(3.2, 7.3, 1.6, 1.2), shift=1, line=GOLD[4]))  # its snout
    st.over(S.piece(S.tri((4.6, 4.4), (6.0, 4.1), (4.4, 2.0)) | S.ell(4.8, 2.2, .6, .6), line=GOLD[4]))  # its horn
    cv.over(st.outline())
    for x, y in S.ell(4.9, 6.1, .6, .85 if big else .6):  # its eye, cut in
        cv.put(x, y, CUT)
    if big:
        cv.put(int(4.7 * f), int(5.7 * f), GOLD[1])
        for i in range(3):  # its mouth
            cv.put(int((2.0 + i * .7) * f), int(8.0 * f), CUT)
    glint(cv, [(int(10.6 * f), int(12.0 * f)), (int(4.0 * f), int(4.4 * f))] + ([(int(10.6 * f) + 1, int(12.0 * f))] if big else []))
    sparkles(cv, f, [(1.4, 11.0), (21.6, 6.2), (19.4, 13.8)], big)
    return cv


ARTS = [net, scale, lapras]
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
