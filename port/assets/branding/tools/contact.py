"""Writes the spec of the contact sheet of Frigate's artwork (port/assets/branding) for sheet.swift: the logos on
GitHub's light and dark backgrounds at three widths, the icons and marks large, and every icon and mark at 128, 64, 32
and 16 px on a light and a dark desktop, with 32 and 16 px also enlarged unsmoothed (4x and 8x) to show the actual
pixels. render.py --sheet runs it.

usage: contact.py DIR SPEC_OUT
"""
import re
import sys

d, out = sys.argv[1], sys.argv[2]


def aspect(path):
    v = re.search(r'viewBox="([^"]+)"', open(path).read()).group(1).split()
    return float(v[2]) / float(v[3])


W, pad, label = 1424, 24, 22
rows, y = [], 0


def caption(text, y, colour):
    rows.append(f"text {pad} {y + 6} 13 {colour} {text}")


# The logos: GitHub light, dark, light-subtle and dark-dimmed page colours; 640, 400 and 160 px wide.
la = aspect(f"{d}/logo.svg")
for name, bg, fg, what in (("logo.svg", "FFFFFF", "6E7781", "logo.svg on #FFFFFF"),
                           ("logo-dark.svg", "0D1117", "8B949E", "logo-dark.svg on #0D1117"),
                           ("logo.svg", "F6F8FA", "6E7781", "logo.svg on #F6F8FA"),
                           ("logo-dark.svg", "22272E", "8B949E", "logo-dark.svg on #22272E")):
    h0 = int(640 / la)
    rows.append(f"rect 0 {y} {W} {h0 + 2 * pad + label} {bg}")
    caption(what + " at 640, 400 and 160 px wide", y, fg)
    top = y + pad + label
    x = pad
    for w in (640, 400, 160):
        h = int(w / la)
        rows.append(f"img {d}/{name} {x} {top + h0 - h} {w} {h}")
        x += w + pad
    y += h0 + 2 * pad + label

# The icons and the marks, large.
rows.append(f"rect 0 {y} {W} {400 + 2 * pad + label} 8A8F98")
caption("icon-black2.svg, icon-white2.svg, mark.svg on white, mark-dark.svg on #0D1117", y, "1E1F22")
top, x = y + pad + label, pad
for name in ("icon-black2.svg", "icon-white2.svg"):
    rows.append(f"img {d}/{name} {x} {top} 400 400")
    x += 400 + pad
for name, bg in (("mark.svg", "FFFFFF"), ("mark-dark.svg", "0D1117")):
    rows.append(f"rect {x} {top + 50} 244 244 {bg}")
    rows.append(f"img {d}/{name} {x + 12} {top + 62} 220 220")
    x += 244 + pad
y += 400 + 2 * pad + label

# Small sizes on a light and a dark desktop.
for bg, fg, mark in (("F4F5F7", "6E7781", "mark.svg"), ("1E1F22", "8B949E", "mark-dark.svg")):
    for name in ("icon-black2.svg", "icon-white2.svg", mark):
        h = 128 + 2 * pad + label
        rows.append(f"rect 0 {y} {W} {h} {bg}")
        caption(f"{name}: 128, 64, 32, 16 px; 32 px x4; 16 px x8", y, fg)
        top, x = y + pad + label, pad
        for s in (128, 64, 32, 16):
            rows.append(f"img {d}/{name} {x} {top + (128 - s) // 2} {s} {s}")
            x += s + pad
        x += pad
        rows.append(f"img {d}/{name} {x} {top} 128 128 4")
        x += 128 + pad
        rows.append(f"img {d}/{name} {x} {top} 128 128 8")
        y += h

open(out, "w").write("\n".join([f"canvas {W} {y} none"] + rows) + "\n")
