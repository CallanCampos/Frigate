"""Writes Frigate's logo set: the mark, the horizontal logo for light and for dark pages, and the two app icons.

The mark is a three-masted frigate drawn flat on a 96-unit grid: three masts carrying a course, a topsail and a
topgallant each, their feet bellied by the wind, a jib, a pennant at the main truck, and a hull whose gunport strake
is cut out of it, so the strake takes the colour of whatever the ship sits on. On a dark hull (light pages, White 2's
icon) the strake's light runs between the gunports are cut, giving a frigate's black-and-white chequered side; on a
light hull (dark pages, Black 2's icon) the gunports themselves are cut. The wordmark FRIGATE is drawn as paths; the
forward ends of its bars belly like the jib.

It writes the six SVGs of port/assets/branding, whose README says what each is for: the mark (mark.svg, mark-dark.svg),
the logo of the landing page and the READMEs (logo.svg, logo-dark.svg), and the app icons that the macOS app and the
window on Linux and Windows show, through their PNG masters (icon-black2.svg, icon-white2.svg). The SVGs are committed
as it writes them, and the test assets_branding (port/tests/assets/test_branding.py) checks that they still are;
render.py runs it and renders the PNGs.

usage: python3 port/assets/branding/tools/frigate_logo.py OUTDIR
"""
import os
import re
import sys

# Palette
INK = "#10151C"  # hull and wordmark on light pages
SNOW = "#F1F5F9"  # hull and wordmark on dark pages
STEEL = "#4A7FB0"  # the rig (masts, sails, jib, pennant) on light pages
ICE = "#9DC0E0"  # the rig on dark pages

LIGHT = dict(rig=STEEL, hull=INK, word=INK, strake="runs")
DARK = dict(rig=ICE, hull=SNOW, word=SNOW, strake="ports")


def n(v):
    s = f"{v:.2f}".rstrip("0").rstrip(".")
    return "0" if s in ("-0", "") else s


# ---------------------------------------------------------------------------------------------------------------
# The mark, on a 96 grid. DX and DY centre the artwork in the square, a unit above the middle (optical centre).
DX, DY = -1, 3
DECK, KEEL = 66 + DY, 84 + DY
SAIL_FOOT = DECK - 6  # the gap between the courses and the deck
TIER_H = (16, 13, 10)  # course, topsail, topgallant
TAPER = (1.0, 0.86, 0.72, 0.58)  # sail width at each yard, from the course's foot up
TIER_GAP = 3.5  # the yards show as gaps
BELLY = 1.6  # how far each foot curves below its corners
MAST_W = 1.7
MASTS = ((17, 14, 0.82), (38.5, 19, 1.0), (61.5, 17, 0.92))  # mizzen, main, fore: centre x, course width, scale
PORTS, PORT = 6, 3.5  # gunports and their size


def sail(cx, top, bot, w_top, w_bot, belly):
    l0, r0, l1, r1 = cx - w_top / 2, cx + w_top / 2, cx - w_bot / 2, cx + w_bot / 2
    return f"M{n(l0)} {n(top)}H{n(r0)}L{n(r1)} {n(bot)}Q{n(cx)} {n(bot + 2 * belly)} {n(l1)} {n(bot)}Z"


def mark_parts(strake):
    """The mark's paths, back to front, as (comment, role, d, holes): role names the colour (rig or hull), and
    holes says the path cuts holes with the even-odd rule."""
    sails, heads = [], []
    for cx, w, sc in MASTS:
        cx += DX
        y = SAIL_FOOT
        for i, h in enumerate(TIER_H):
            top = y - h * sc
            sails.append(sail(cx, top, y, w * TAPER[i + 1], w * TAPER[i], BELLY * sc))
            y = top - TIER_GAP
        heads.append((cx, y + TIER_GAP, w))
    (zx, ztop, _), (mx, mtop, _), (fx, ftop, fw) = heads
    # Masts, from inside the hull to a short head above each topgallant; the main's carries the pennant.
    masts = ""
    for cx, top in ((zx, ztop - 3), (mx, mtop - 10), (fx, ftop - 3)):
        masts += f"M{n(cx - MAST_W / 2)} {n(top)}h{n(MAST_W)}V{n(DECK + 1)}h{n(-MAST_W)}Z"
    # Jib: from below the fore topgallant to the bowsprit's end, its forward edge bellied.
    jx = fx + fw / 2 + 4
    jib = f"M{n(jx)} {n(ftop + 2)}Q{n(jx + 13)} {n(ftop + 22)} {n(92 + DX)} {n(SAIL_FOOT)}H{n(jx)}Z"
    # Pennant flying forward from the main truck.
    px = mx + MAST_W / 2
    pennant = f"M{n(px)} {n(mtop - 9)}L{n(px + 13)} {n(mtop - 6)}L{n(px)} {n(mtop - 3)}Z"
    # Hull: raked stern, round bow, and the gunport strake cut out of it.
    r = KEEL - DECK
    hull = f"M{n(6 + DX)} {n(DECK)}H{n(90 + DX)}A{n(r)} {n(r)} 0 0 1 {n(90 + DX - r)} {n(KEEL)}H{n(12 + DX)}Z"
    y0, x0, x1 = DECK + 4, 12.5 + DX, 84 + DX
    run = (x1 - x0 - PORTS * PORT) / (PORTS + 1)
    if strake == "runs":
        holes = [(x0 + i * (run + PORT), run) for i in range(PORTS + 1)]
        what = "hull, with the strake's light runs between its gunports cut out"
    else:
        holes = [(x0 + run + i * (run + PORT), PORT) for i in range(PORTS)]
        what = "hull, with its gunports cut out"
    for x, w in holes:
        hull += f"M{n(x)} {n(y0)}h{n(w)}v{n(PORT)}h{n(-w)}Z"
    return [
        ("masts, in the colour of the canvas they carry", "rig", masts, False),
        ("sails: mizzen, main and fore, each a course, a topsail and a topgallant", "rig", "".join(sails), False),
        ("jib and pennant", "rig", jib + pennant, False),
        (what, "hull", hull, True),
    ]


def art_top():
    """The top of the artwork: the main truck."""
    d = mark_parts("runs")[0][2]
    return min(float(v) for v in re.findall(r"M[\d.]+ ([\d.]+)", d))


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
        f"Frigate's mark for {page} pages: a three-masted frigate, flat, on a 96 grid.",
    ], mark_paths(colours)))


CAP = 36  # the wordmark's cap height beside the 96-unit mark
GAP = 20  # from the jib's foot to the F


def write_logo(out, name, colours, page):
    top, bottom = art_top(), KEEL
    margin = 4
    baseline = (top + bottom) / 2 + CAP / 2  # the caps centred on the ship
    paths, right = wordmark(92 + DX + GAP, baseline, CAP)
    left = 6 + DX - margin
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


def main():
    out = sys.argv[1]
    os.makedirs(out, exist_ok=True)
    write_mark(out, "mark.svg", LIGHT, "light")
    write_mark(out, "mark-dark.svg", DARK, "dark")
    write_logo(out, "logo.svg", LIGHT, "light")
    write_logo(out, "logo-dark.svg", DARK, "dark")
    write_icon(out, "icon-black2.svg", "Frigate Black 2", DARK, ("#1C2431", "#0D1117"), False)
    write_icon(out, "icon-white2.svg", "Frigate White 2", LIGHT, ("#FFFFFF", "#E3E9F0"), True)


main()
