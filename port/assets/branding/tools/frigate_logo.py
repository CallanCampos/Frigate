"""Writes Frigate's logo set: the mark, the horizontal logo for light and for dark pages, and the two app icons.

The mark is the Plasma Frigate, the flying ship of Pokémon Black 2 and White 2 that the port is named after, drawn as
flat shapes of our own in profile on a 96-unit grid, sailing to the right: one main mast carrying a single tall stack
of square sails, three tiers of four panels with their feet bellied by the wind, and a pennant at its truck; two
swept jibs nested over a long ram that runs on from the deck; a tall sail on a short mast at the very stern; a domed
round tower on the raised stern deck, which steps down to the main deck in a curve; a swept fin under the bow; and an
outrigger pontoon slung below the keel on raked struts, with sky between, a thruster cone parted from its tail. The rig
and the fittings are one colour, the hull, ram and tower another, and the hull's port strake is cut out of it, so the
strake takes the colour of whatever the ship sits on. On a dark hull (light pages, White 2's icon) the six light runs
between the strake's five ports are cut, giving a chequered side; on a light hull (dark pages, Black 2's icon) the
five ports themselves are cut. The wordmark FRIGATE is drawn as paths; the forward ends of its bars belly like a sail.

At 16 and 32 pixels a scaled drawing blurs its sails into one smudge and loses its ram and the sky under the keel, so
the icons' small images are drawn for their size, pixel by pixel (SHIP_16 and SHIP_32 below): the same ship with
whole-pixel sails, hull and pontoon, the sails' panels and the strake's ports where the pixels allow, on the same
plate, with a stronger edge on White 2's light plate.

It writes the twelve SVGs of port/assets/branding, whose README says what each is for: the mark (mark.svg,
mark-dark.svg), the logo of the landing page and the READMEs (logo.svg, logo-dark.svg), the app icons that the macOS app
and the window on Linux and Windows show, through their PNG masters (icon-black2.svg, icon-white2.svg), the macOS
icon's small images (icon-black2-16.svg, icon-black2-32.svg and White 2's), and the window's 32 px icon, whose plate
fills its square (window-black2.svg, window-white2.svg). The SVGs are committed as it writes them, and the test
assets_branding (port/tests/assets/test_branding.py) checks that they still are, and that the small PNGs hold the
sprites' pixels; render.py runs it and renders the PNGs.

usage: python3 port/assets/branding/tools/frigate_logo.py OUTDIR
"""
import os
import re
import sys

# Palette
INK = "#10151C"  # hull and wordmark on light pages
SNOW = "#F1F5F9"  # hull and wordmark on dark pages
STEEL = "#4A7FB0"  # the rig and fittings (masts, sails, pennant, dome, fin, pontoon, struts) on light pages
ICE = "#9DC0E0"  # the rig and fittings on dark pages

LIGHT = dict(rig=STEEL, hull=INK, word=INK, strake="runs")
DARK = dict(rig=ICE, hull=SNOW, word=SNOW, strake="ports")


def n(v):
    s = f"{v:.2f}".rstrip("0").rstrip(".")
    return "0" if s in ("-0", "") else s


def p(x, y):
    return f"{n(x)} {n(y)}"


def poly(*pts):
    """A closed path through the points, with H and V where a side is level or upright."""
    d = f"M{p(*pts[0])}"
    x0, y0 = pts[0]
    for x, y in pts[1:]:
        d += f"H{n(x)}" if n(y) == n(y0) else f"V{n(y)}" if n(x) == n(x0) else f"L{p(x, y)}"
        x0, y0 = x, y
    return d + "Z"


def quad(a, k, b, t):
    """The point at t on the quadratic curve from a through control k to b."""
    return tuple((1 - t) ** 2 * u + 2 * (1 - t) * t * v + t * t * w for u, v, w in zip(a, k, b))


def quad_ctrl(a, k, b, t0, t1):
    """The control point of the quadratic curve's piece from t0 to t1."""
    return tuple((1 - t0) * (1 - t1) * u + ((1 - t0) * t1 + t0 * (1 - t1)) * v + t0 * t1 * w
                 for u, v, w in zip(a, k, b))


# ---------------------------------------------------------------------------------------------------------------
# The mark, on a 96 grid, y down, the bow to the right. The ship spans x 4.6 to 91.2 and y 9 to 87, centred on
# (48, 48). The level edges that matter at small sizes (the poop, the main deck, the strake, the keel, the heads of the
# sails and the pontoon's underside) lie on the 3-unit grid of a 32 px render.
DECK, KEEL = 63, 78  # the main deck and the keel: a hull 15 deep
POOP = 60  # the raised stern deck
STERN, STERN_FOOT = 7.5, 12  # the stern's corner at the poop, and where its rake meets the keel
BREAK = 26  # the poop's forward edge, from which it steps down to the main deck in a curve 4 long
STEM, RAM_TIP, RAM_ROOT = 72, (91.2, 61.5), 65.6  # the stem head on the deck, the ram's point, its root's underside
BOW_KEEL = 52  # where the bow's sweep up into the ram leaves the keel
STRAKE, STRAKE_FROM, STRAKE_TO, PORTS, PORT = (66, 69), 15, 63.5, 5, 3.5  # top and bottom, ends, ports, port width

# The tower on the poop: a shoulder, a drum on it and a round dome as wide as the drum.
TOWER_X = 18.2
SHOULDER, DRUM, DOME_R = (10.4, 2), (8.4, 3), 4.2  # (width, height) of each step, and the dome's radius

# The main mast and its sails: three tiers of four panels, as (foot, head, foot width, head width) with y at the
# corners; each foot bellies 1.3 below its corners at the middle, and open seams 1.2 wide part the panels.
MAIN, MAST_W, TRUCK = 42.5, 1.7, 9
TIERS = ((60, 48, 27, 26), (44, 33, 25.6, 24.6), (29, 18, 24.2, 22.6))
BELLY, SEAM = 1.3, 1.2
PENNANT = (13, 3, 6)  # length past the mast, the drop of its tip, and its depth at the mast

# The stern sail: its luff on the short mast, the head at the mast's top, its foot level and its leech bowed a little.
MIZZEN, MIZZEN_TOP, STERN_SAIL_FOOT, CLEW, LEECH_BOW = 10.5, 30, 52, 4.6, 1.2

# The jibs, nested blades swept back from points over the ram, their heads at half the mast's height: the upper one
# the larger, its point nearly at the ram's tip; the lower one's luff runs parallel to the upper one's foot, JIB_GAP
# below it, and its point stays clear of the ram.
JIB_HI = dict(head=(63.5, 38), clew=(65, 53), tack=(89, 58.5), sag=3.4)
JIB_LO = dict(clew=(67.6, 59.8), point_x=84.5, sag=1.1)
JIB_GAP = 2.6

# The pontoon below the keel, with sky between: an ogive nose forward, a thruster cone aft parted from its tail.
PONTOON = (82, 87)  # top and bottom
PONTOON_X = (25.2, 29.3, 30, 57.5, 66.5)  # cone tip, cone base, tail, the nose's shoulder and its tip
STRUTS, STRUT_W, STRUT_RAKE = (36, 45.5, 55), 2.4, 4.6  # roots on the keel, width, how far aft each foot lands

# The bow fin, swept forward and down from under the beak: its root inside the hull, its point, and where its
# trailing edge returns into the hull.
FIN = (0.28, (78.5, 77.5), 0.64, (69, 76.4))  # where its edges leave the bow, its point, the trailing edge's control


def cubic(a, b, c, d, t):
    """The point at t on the cubic curve a-b-c-d."""
    s = 1 - t
    return tuple(s ** 3 * u + 3 * s * s * t * v + 3 * s * t * t * w + t ** 3 * z for u, v, w, z in zip(a, b, c, d))


def bow_curve():
    """The bow's underside: from the ram's underside at the stem, one curve down and aft to the keel."""
    return (STEM, RAM_ROOT), (68.4, 66.4), (63, KEEL), (BOW_KEEL, KEEL)


def tier(foot, head, wf, wh):
    """One tier of the main sails as four panels with open seams between them. The tier's foot is one quadratic
    curve bellied below its corners; each panel's foot is its share of that curve."""
    a, k, b = (MAIN - wf / 2, foot), (MAIN, foot + 2 * BELLY), (MAIN + wf / 2, foot)
    d = ""
    for i in range(4):
        f0 = i / 4 + (SEAM / 2 / wf if i else 0)
        f1 = (i + 1) / 4 - (SEAM / 2 / wf if i < 3 else 0)
        h0 = MAIN - wh / 2 + wh * i / 4 + (SEAM / 2 if i else 0)
        h1 = MAIN - wh / 2 + wh * (i + 1) / 4 - (SEAM / 2 if i < 3 else 0)
        fx, fy = quad(a, k, b, f1)
        side = f"V{n(fy)}" if n(fx) == n(h1) else f"L{p(fx, fy)}"
        d += f"M{p(h0, head)}H{n(h1)}{side}Q{p(*quad_ctrl(a, k, b, f0, f1))} {p(*quad(a, k, b, f0))}Z"
    return d


def offset_quad(a, k, b, dist):
    """The quadratic curve a-k-b moved by dist along its chord's normal (downward for a curve drawn forward)."""
    dx, dy = b[0] - a[0], b[1] - a[1]
    ln = (dx * dx + dy * dy) ** 0.5
    nx, ny = -dy / ln * dist, dx / ln * dist
    return [(x + nx, y + ny) for x, y in (a, k, b)]


def jibs():
    """The two jibs, each pointed forward with its foot drawn up in a concave curve back to the clew and its aft edge
    upright. The upper one's luff runs straight from its head down to its point; the lower one's is the upper one's
    foot moved down by the gap, so the band between them is even."""
    h, c, t = JIB_HI["head"], JIB_HI["clew"], JIB_HI["tack"]
    mx, my = (c[0] + t[0]) / 2, (c[1] + t[1]) / 2
    k = (mx, my - JIB_HI["sag"])
    upper = f"M{p(*h)}L{p(*t)}Q{p(*k)} {p(*c)}Z"
    # The upper foot, moved down: its piece between the lower jib's clew and point is the lower jib's luff.
    a2, k2, b2 = offset_quad(c, k, t, JIB_GAP)
    c2 = JIB_LO["clew"]

    def x_at(target):  # the curve's parameter where it reaches x = target
        lo, hi = 0.0, 1.0
        for _ in range(40):
            mid = (lo + hi) / 2
            lo, hi = (mid, hi) if quad(a2, k2, b2, mid)[0] < target else (lo, mid)
        return lo

    s0, s1 = x_at(c2[0]), x_at(JIB_LO["point_x"])
    head2, point2 = quad(a2, k2, b2, s0), quad(a2, k2, b2, s1)
    luff_k = quad_ctrl(a2, k2, b2, s0, s1)
    fm = ((c2[0] + point2[0]) / 2, (c2[1] + point2[1]) / 2 - JIB_LO["sag"])
    lower = f"M{p(*head2)}Q{p(*luff_k)} {p(*point2)}Q{p(*fm)} {p(*c2)}Z"
    return upper + lower


def mark_parts(strake):
    """The mark's paths, back to front, as (comment, role, d, holes): role names the colour (rig or hull), and
    holes says the path cuts holes with the even-odd rule."""
    # The main mast shows only below and above the sails: from the deck to the course, and from the top tier to the
    # truck. Inside the block the sails hide it, so the middle seam and the gaps between the tiers stay open like the
    # others and the grid reads as four even columns (a mast drawn in the gaps alone looked dashed down the middle).
    m0, m1 = MAIN - MAST_W / 2, MAIN + MAST_W / 2
    spans = [(DECK + 1, TIERS[0][0] + BELLY), (TIERS[2][1], TRUCK)]
    masts = "".join(poly((m0, a), (m1, a), (m1, b), (m0, b)) for a, b in spans)

    sails = "".join(tier(*t) for t in TIERS)

    # The stern sail and its short mast in one: the mast up from inside the poop, the sail's foot level out past the
    # stern, the leech bowed a little up to the head at the mast's top, forward corner.
    z0, z1 = MIZZEN - MAST_W / 2, MIZZEN + MAST_W / 2
    lk = ((CLEW + z1) / 2 - LEECH_BOW * 0.95, (STERN_SAIL_FOOT + MIZZEN_TOP) / 2 - LEECH_BOW * 0.3)
    stern = (f"M{p(z1, MIZZEN_TOP)}V{n(POOP + 1)}H{n(z0)}V{n(STERN_SAIL_FOOT)}H{n(CLEW)}"
             f"Q{p(*lk)} {p(z1, MIZZEN_TOP)}Z")
    pl, pd, ph = PENNANT
    pennant = poly((MAIN, TRUCK + 1), (m1 + pl, TRUCK + 1 + pd), (MAIN, TRUCK + 1 + ph))
    # The dome: a half circle as wide as the drum, its foot running down into the drum, which the hull paints over.
    drum_top = POOP - SHOULDER[1] - DRUM[1]
    r = DOME_R
    dome = (f"M{p(TOWER_X - r, drum_top + 1)}V{n(drum_top)}A{n(r)} {n(r)} 0 0 1 {p(TOWER_X + r, drum_top)}"
            f"V{n(drum_top + 1)}Z")

    # The pontoon: an ogive nose forward, the body, and aft of a cut the thruster cone.
    top, bot = PONTOON
    mid = (top + bot) / 2
    tip, cone, tail, shoulder, nose = PONTOON_X
    reach = (nose - shoulder) * 0.72
    pontoon = (f"M{p(tail, top)}H{n(shoulder)}Q{p(shoulder + reach, top)} {p(nose, mid)}"
               f"Q{p(shoulder + reach, bot)} {p(shoulder, bot)}H{n(tail)}Z")
    ch = (bot - top) / 2 - 0.3
    pontoon += poly((cone, mid - ch), (cone, mid + ch), (tip, mid))
    struts = "".join(poly((x, KEEL - 1), (x + STRUT_W, KEEL - 1), (x + STRUT_W - STRUT_RAKE, top + 0.5),
                          (x - STRUT_RAKE, top + 0.5)) for x in STRUTS)
    ta, point, tb, ctrl = FIN
    (ax, ay), (bx, by) = cubic(*bow_curve(), ta), cubic(*bow_curve(), tb)
    fin = f"M{p(ax - 1, ay - 0.5)}L{p(*point)}Q{p(*ctrl)} {p(bx - 0.5, by - 1)}Z"

    # The hull, in one path: the raked stern, the poop with the tower's shoulder and drum, its curved step down to
    # the main deck, the ram, the bow sweeping back under it to the keel, and the strake cut out.
    sx, sh = SHOULDER[0] / 2, POOP - SHOULDER[1]
    dx = DRUM[0] / 2
    b0, b1, b2, b3 = bow_curve()
    hull = (f"M{p(STERN, POOP)}H{n(TOWER_X - sx)}V{n(sh)}H{n(TOWER_X - dx)}V{n(drum_top)}H{n(TOWER_X + dx)}"
            f"V{n(sh)}H{n(TOWER_X + sx)}V{n(POOP)}H{n(BREAK)}Q{p(BREAK, DECK)} {p(BREAK + 4, DECK)}"
            f"H{n(STEM)}L{p(*RAM_TIP)}L{p(*b0)}C{p(*b1)} {p(*b2)} {p(*b3)}H{n(STERN_FOOT)}Z")
    s0, s1 = STRAKE
    run = (STRAKE_TO - STRAKE_FROM - PORTS * PORT) / (PORTS + 1)
    if strake == "runs":
        holes = [(STRAKE_FROM + i * (run + PORT), run) for i in range(PORTS + 1)]
        what = "the six light runs between its five ports cut out of it"
    else:
        holes = [(STRAKE_FROM + run + i * (run + PORT), PORT) for i in range(PORTS)]
        what = "its five ports cut out of it"
    for x, w in holes:
        hull += poly((x, s0), (x + w, s0), (x + w, s1), (x, s1))
    return [
        ("the main mast, below and above its sails", "rig", masts, False),
        ("the main sails: three tiers of four panels", "rig", sails, False),
        ("the jibs, the stern sail on its short mast, the pennant and the tower's dome", "rig",
         jibs() + stern + pennant + dome, False),
        ("the pontoon with its thruster cone, its struts and the bow fin", "rig", struts + pontoon + fin, False),
        (f"the hull with the poop, the tower's shoulder and drum and the ram, {what}", "hull", hull, True),
    ]


def mark_paths(colours, indent="  "):
    out = []
    for comment, role, d, holes in mark_parts(colours["strake"]):
        rule = ' fill-rule="evenodd"' if holes else ""
        out.append(f"{indent}<!-- {comment} -->")
        out.append(f'{indent}<path fill="{colours[role]}"{rule} d="{d}"/>')
    return out


# ---------------------------------------------------------------------------------------------------------------
# The wordmark: geometric capitals on a 36-unit cap height, stems 7.2, horizontals 6.62. Each letter is drawn from
# its own left edge with y = 0 at the cap line; the forward (right) ends of the bars belly by 1.35.
def letters():
    hb = 6.62  # horizontal bar
    q = 2.7  # control offset of a bellied end (it bellies by half of this)

    def bar_end(x, y):  # from (x, y) down a bar's height, bellied forward
        return f"Q{n(x + q)} {n(y + hb / 2)} {n(x)} {n(y + hb)}"

    F = (f"M0 0H22.5{bar_end(22.5, 0)}H7.2V14.85H19.8{bar_end(19.8, 14.85)}H7.2V36H0Z", 24)
    R = ("M0 0H12.6A10.8 10.8 0 0 1 12.6 21.6H7.2V36H0ZM7.2 6.62V14.98H12.6A3.6 4.18 0 0 0 12.6 6.62Z"
         "M7.66 14.98H15.47L24.3 36H16.49Z", 24.3)
    I = ("M0 0H7.2V36H0Z", 7.2)
    G = ("M32.32 5.59A18.54 18.54 0 1 0 37.08 18H19.44V24.62H27.3A10.98 10.98 0 1 1 26.7 10.65Z", 37.08)
    A = ("M0 36L11.7 0H17.1L28.8 36H21.23L18.41 27.32H10.39L7.57 36ZM14.4 14.99L12.54 20.7H16.26Z", 28.8)
    T = (f"M0 0H25.2{bar_end(25.2, 0)}H16.2V36H9V{n(hb)}H0Q{n(q)} {n(hb / 2)} 0 0Z", 25.2)
    E = (f"M0 0H22.5{bar_end(22.5, 0)}H7.2V14.85H19.8{bar_end(19.8, 14.85)}H7.2V{n(36 - hb)}H22.5"
         f"{bar_end(22.5, 36 - hb)}H0Z", 24)
    # The space before each letter, between ink, set by eye: rounds and diagonals closer, straights further.
    return [(F, 0), (R, 6.3), (I, 6.8), (G, 6.2), (A, 5.2), (T, 2.0), (E, 5.6)]


def wordmark(x0, baseline, cap):
    s = cap / 36
    paths, x = [], x0
    for (d, w), gap in letters():
        x += gap * s
        paths.append(transform_path(d, x, baseline - cap, s))
        x += w * s
    return paths, x


def transform_path(d, ox, oy, s):
    """Translates and scales a path written with absolute M, H, V, L, Q, A and Z commands."""
    toks = re.findall(r"[MHVLQAZ]|-?\d*\.?\d+", d)
    out, i, cmd = [], 0, None
    fx = lambda v: n(ox + float(v) * s)
    fy = lambda v: n(oy + float(v) * s)
    while i < len(toks):
        if toks[i].isalpha():
            cmd = toks[i]
            out.append(cmd)
            i += 1
            if cmd == "Z":
                continue
        if cmd in "ML":
            out.append(f"{fx(toks[i])} {fy(toks[i + 1])}")
            i += 2
        elif cmd == "H":
            out.append(fx(toks[i]))
            i += 1
        elif cmd == "V":
            out.append(fy(toks[i]))
            i += 1
        elif cmd == "Q":
            out.append(f"{fx(toks[i])} {fy(toks[i + 1])} {fx(toks[i + 2])} {fy(toks[i + 3])}")
            i += 4
        elif cmd == "A":
            rx, ry, rot, large, sweep, x, y = toks[i:i + 7]
            out.append(f"{n(float(rx) * s)} {n(float(ry) * s)} {rot} {large} {sweep} {fx(x)} {fy(y)}")
            i += 7
    return "".join(out)


# ---------------------------------------------------------------------------------------------------------------
def svg(view, title, comments, body):
    return "\n".join([
        f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="{view}" role="img">',
        f"  <title>{title}</title>",
        *[f"  <!-- {c} -->" for c in comments],
        *body,
        "</svg>",
        "",
    ])


def write(out, name, text):
    with open(os.path.join(out, name), "w") as f:
        f.write(text)


def write_mark(out, name, colours, page):
    write(out, name, svg("0 0 96 96", "Frigate", [
        f"Frigate's mark for {page} pages: the Plasma Frigate in profile, flat, on a 96 grid.",
    ], mark_paths(colours)))


CAP = 36  # the wordmark's cap height beside the 96-unit mark
GAP_TO_F = 20  # from the ram's tip to the F
# The lockup's frame on the 96 grid, kept from the first mark (a three-master from its main truck to its keel), so
# that the view and logo.png's 1200 x 333 stay as they were; the ship (y 9 to 87) sits inside it.
FRAME_TOP, FRAME_BOTTOM, FRAME_LEFT, FRAME_RIGHT = 7, 87, 5, 91


def write_logo(out, name, colours, page):
    top, bottom = FRAME_TOP, FRAME_BOTTOM
    margin = 4
    baseline = (top + bottom) / 2 + CAP / 2  # the caps centred on the ship
    paths, right = wordmark(FRAME_RIGHT + GAP_TO_F, baseline, CAP)
    left = FRAME_LEFT - margin
    view = f"{n(left)} {n(top - margin)} {n(right + margin - left)} {n(bottom - top + 2 * margin)}"
    body = mark_paths(colours)
    body.append("  <!-- FRIGATE -->")
    body.append(f'  <g fill="{colours["word"]}">')
    body += [f'    <path d="{d}"/>' for d in paths]
    body.append("  </g>")
    write(out, name, svg(view, "Frigate", [
        f"Frigate's logo for {page} pages: the mark, then FRIGATE drawn as paths.",
    ], body))


# Apple's macOS icon plate: 824 square at 100,100 with continuous corners (radius 185.4).
PLATE = ("M383.41 100H640.59C722.19 100 763 100 799.79 112.14L806.92 113.89C854.88 131.34 892.66 169.12 910.11 217.08"
         "L911.86 224.21C924 261 924 301.81 924 383.41V640.59C924 722.19 924 763 911.86 799.79L910.11 806.92"
         "C892.66 854.88 854.88 892.66 806.92 910.11L799.79 911.86C763 924 722.19 924 640.59 924H383.41"
         "C301.81 924 261 924 224.21 911.86L217.08 910.11C169.12 892.66 131.34 854.88 113.89 806.92L112.14 799.79"
         "C100 763 100 722.19 100 640.59V383.41C100 301.81 100 261 112.14 224.21L113.89 217.08"
         "C131.34 169.12 169.12 131.34 217.08 113.89L224.21 112.14C261 100 301.81 100 383.41 100Z")
ICON_SCALE = 6.6


def write_icon(out, name, title, colours, stops, hairline):
    off = 512 - 48 * ICON_SCALE  # the mark's 96 square centred on the plate
    body = [
        "  <defs>",
        '    <linearGradient id="plate" x1="0" y1="100" x2="0" y2="924" gradientUnits="userSpaceOnUse">',
        f'      <stop offset="0" stop-color="{stops[0]}"/>',
        f'      <stop offset="1" stop-color="{stops[1]}"/>',
        "    </linearGradient>",
        "  </defs>",
        "  <!-- plate -->",
        f'  <path fill="url(#plate)" d="{PLATE}"/>',
    ]
    if hairline:
        # A 2 px edge just inside the plate, so the light plate keeps its shape on white.
        k = 822 / 824
        body.append("  <!-- the plate's edge -->")
        body.append(f'  <path fill="none" stroke="{INK}" stroke-opacity="0.12" stroke-width="{n(2 / k)}" '
                    f'transform="translate({n(512 * (1 - k))} {n(512 * (1 - k))}) scale({k:.5f})" d="{PLATE}"/>')
    body.append(f'  <g transform="translate({n(off)} {n(off)}) scale({n(ICON_SCALE)})">')
    body += mark_paths(colours, "    ")
    body.append("  </g>")
    write(out, name, svg("0 0 1024 1024", title, [
        f"{title}: the app icon, on Apple's 1024 macOS grid, with the mark scaled {n(ICON_SCALE)}x on the plate.",
    ], body))


# ---------------------------------------------------------------------------------------------------------------
# The small images, drawn pixel by pixel. Each string is a row of pixels: r is the rig and the fittings (the masts, the
# sails, the jibs, the stern sail, the pennant, the tower's dome, the pontoon and its struts), h the hull (with the
# poop, the tower's drum and the ram), and on the port strake u is a light run and p a port, which the version's hull
# cuts as the mark's does (Black 2's hull cuts its ports, White 2's its runs); a dot is the plate.
# The mark on a whole-pixel grid, its sail block drawn larger so that its panels can part. At 32 px: the main sails as
# three tiers of four panels two pixels wide, with the open seam down the middle where the mast is hidden and a row
# between the tiers for the yards, the mast showing below the course and at the truck, with the pennant flying forward
# (right); the stern sail on its short mast, and the tower's dome on its drum on the raised poop; the two jibs over the
# ram, which runs on from the deck a row deep; the hull with its five ports between six runs, the stern raked and the
# bow sweeping back under the ram; and the pontoon on three struts, with the row below the keel open. The 16 px ship
# keeps the block as two columns of three panels, the stern sail, the jibs, the ram, two rows of hull and the pontoon
# on two struts, and drops the pennant, the tower and the ports, which 16 pixels can't hold apart.
SHIP_16 = (
    "...rr.rr....",
    "...rr.rr....",
    ".r..........",
    "rr.rr.rr.r..",
    "rr.rr.rr.rr.",
    "rr.......rrr",
    ".r.rr.rr....",
    ".hhrr.rr.rr.",
    ".hhhhhhhhhhh",
    "..hhhhhhh...",
    "....r..r....",
    "...rrrrrr...",
)
SHIP_32 = (
    "............rrrr........",
    "............rr..........",
    ".......rr.rr.rr.rr......",
    ".......rr.rr.rr.rr......",
    ".......rr.rr.rr.rr......",
    "..r.....................",
    ".rr....rr.rr.rr.rr......",
    ".rr....rr.rr.rr.rr.r....",
    ".rr....rr.rr.rr.rr.rr...",
    "rrr................rrr..",
    "rrr....rr.rr.rr.rr.rrrr.",
    "..r.r..rr.rr.rr.rr...rr.",
    "..rhhh.rr.rr.rr.rr....r.",
    ".hhhhhh.....r......rrr..",
    ".hhhhhhhhhhhhhhhhhhhhhhh",
    ".hhhhupupupupupuhhhh....",
    "..hhhhhhhhhhhhhhhhh.....",
    "..hhhhhhhhhhhhhh........",
    "........r..r..r.........",
    "......rrrrrrrrrrr.......",
)
SMALL_EDGE = 0.35  # the opacity of the one-pixel ink edge inside White 2's small plates


def sprite_cells(colours):
    """Which of a sprite's letters take the rig's colour and which the hull's: the strake's runs or its ports go with
    the hull, and the others are cut, showing the plate."""
    return "r", "hu" if colours["strake"] == "ports" else "hp"


def sprite_rows(sprite, letters, ox, oy):
    """A path of whole-pixel rectangles, one for each run of the letters along a row, offset by (ox, oy)."""
    d = ""
    for y, row in enumerate(sprite):
        x = 0
        while x < len(row):
            if row[x] not in letters:
                x += 1
                continue
            x0 = x
            while x < len(row) and row[x] in letters:
                x += 1
            d += f"M{ox + x0} {oy + y}h{x - x0}v1h{x0 - x}Z"
    return d


def sprite_origin(sprite, size):
    """Where a sprite sits on a size-pixel image: centred, on whole pixels."""
    return (size - len(sprite[0])) // 2, (size - len(sprite)) // 2


def plate_transform(x0, x1):
    """The transform that puts the 1024 grid's plate (100 to 924) on [x0, x1] of the image, both ways."""
    k = (x1 - x0) / 824
    offset = f"{x0 - 100 * k:.4f}".rstrip("0").rstrip(".")
    return f"translate({offset} {offset}) scale({k:.6f})", k


def write_small_icon(out, name, title, colours, stops, edge, size, plate, sprite):
    """A small image of an icon: the plate from plate[0] to plate[1] pixels, on Apple's grid for the macOS icon's or
    filling the square for the window's, and the sprite on it, on whole pixels."""
    ox, oy = sprite_origin(sprite, size)
    rig, hull = sprite_cells(colours)
    transform, _ = plate_transform(*plate)
    body = [
        "  <defs>",
        '    <linearGradient id="plate" x1="0" y1="100" x2="0" y2="924" gradientUnits="userSpaceOnUse">',
        f'      <stop offset="0" stop-color="{stops[0]}"/>',
        f'      <stop offset="1" stop-color="{stops[1]}"/>',
        "    </linearGradient>",
        "  </defs>",
        "  <!-- plate -->",
        f'  <path fill="url(#plate)" transform="{transform}" d="{PLATE}"/>',
    ]
    if edge:
        # One pixel of ink just inside the plate, so that the light plate keeps its shape on white at this size
        transform, k = plate_transform(plate[0] + 0.5, plate[1] - 0.5)
        body.append("  <!-- the plate's edge -->")
        body.append(f'  <path fill="none" stroke="{INK}" stroke-opacity="{n(edge)}" stroke-width="{1 / k:.4f}" '
                    f'transform="{transform}" d="{PLATE}"/>')
    body.append("  <!-- the ship, pixel by pixel: the rig and fittings, then the hull with its strake -->")
    body.append(f'  <path fill="{colours["rig"]}" d="{sprite_rows(sprite, rig, ox, oy)}"/>')
    body.append(f'  <path fill="{colours["hull"]}" d="{sprite_rows(sprite, hull, ox, oy)}"/>')
    where = "on Apple's macOS grid" if plate[0] > 0 else "with the plate filling the square, for the window"
    write(out, name, svg(f"0 0 {size} {size}", title, [
        f"{title}: the icon's {size} px image, drawn pixel by pixel, {where}.",
    ], body))


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    write_mark(out, "mark.svg", LIGHT, "light")
    write_mark(out, "mark-dark.svg", DARK, "dark")
    write_logo(out, "logo.svg", LIGHT, "light")
    write_logo(out, "logo-dark.svg", DARK, "dark")
    versions = (("black2", "Frigate Black 2", DARK, ("#1C2431", "#0D1117"), False),
                ("white2", "Frigate White 2", LIGHT, ("#FFFFFF", "#E3E9F0"), True))
    for version, title, colours, stops, light in versions:
        write_icon(out, f"icon-{version}.svg", title, colours, stops, light)
        edge = SMALL_EDGE if light else None
        for size, sprite in ((16, SHIP_16), (32, SHIP_32)):
            grid = (100 * size / 1024, 924 * size / 1024)
            write_small_icon(out, f"icon-{version}-{size}.svg", title, colours, stops, edge, size, grid, sprite)
        write_small_icon(out, f"window-{version}.svg", title, colours, stops, edge, 32, (0, 32), SHIP_32)


main()
