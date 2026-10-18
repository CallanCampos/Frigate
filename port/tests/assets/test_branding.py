"""Checks Frigate's artwork (port/assets/branding) as committed, on any platform, without rendering it:

- every SVG is what tools/frigate_logo.py writes today, byte for byte, so that the generator and the files never drift;
- every SVG is well-formed XML with a <title>, and draws with paths alone (plus the icons' one gradient): no text,
  fonts, images, scripts or links, which GitHub, browsers and AppKit would render differently or refuse;
- every PNG master is an 8-bit RGBA PNG of the size the build and the pages use: the app icons 1024 x 1024 (the macOS
  app's .icns and the window's icon are made from them), the icons' small images 16 and 32 px (the .icns's) and 32 px
  (the window's), the logos 1200 px wide at their SVG's proportions;
- every small image's PNG holds its SVG's ship pixel for pixel: each pixel of the rig and of the hull, which the SVG
  draws as whole-pixel rectangles, is exactly the artwork's colour and opaque, and no other pixel is, so a render that
  blurred or moved the ship fails;
- every small image's ship stays clear of its plate's outermost pixel, the one White 2's edge darkens and Black 2's
  plate shades off in, so that the edge keeps the plate's shape all round.

Whether the large PNGs show what the SVGs draw needs AppKit: tools/render.py renders them again (README.md).

usage: test_branding.py WORK_DIR   (the CMake test assets_branding, port/cmake/assets.cmake)
"""
import math
import os
import re
import shutil
import struct
import subprocess
import sys
import xml.etree.ElementTree as ET
import zlib

BRANDING = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "assets", "branding")
GENERATOR = os.path.join(BRANDING, "tools", "frigate_logo.py")
SVGS = ("mark.svg", "mark-dark.svg", "logo.svg", "logo-dark.svg", "icon-black2.svg", "icon-white2.svg",
        "icon-black2-16.svg", "icon-black2-32.svg", "icon-white2-16.svg", "icon-white2-32.svg", "window-black2.svg",
        "window-white2.svg")
ICONS = {"icon-black2-1024.png": (1024, 1024), "icon-white2-1024.png": (1024, 1024)}
# The small images: each PNG, its size and the SVG it is rendered from
SMALL = {"icon-black2-16.png": (16, "icon-black2-16.svg"), "icon-black2-32.png": (32, "icon-black2-32.svg"),
         "icon-white2-16.png": (16, "icon-white2-16.svg"), "icon-white2-32.png": (32, "icon-white2-32.svg"),
         "window-black2-32.png": (32, "window-black2.svg"), "window-white2-32.png": (32, "window-white2.svg")}
LOGOS = {"logo.png": "logo.svg", "logo-dark.png": "logo-dark.svg"}
LOGO_WIDTH = 1200
ALLOWED = {"svg", "title", "path", "g", "defs", "linearGradient", "stop"}
SVG_NS = "{http://www.w3.org/2000/svg}"

checks = 0
failures = []


def expect(what, ok, detail=""):
    global checks
    checks += 1
    if not ok:
        failures.append(what + (f": {detail}" if detail else ""))


def png_header(path):
    """A PNG's width, height, bit depth and colour type, from its IHDR chunk."""
    with open(path, "rb") as f:
        head = f.read(33)
    if head[:8] != b"\x89PNG\r\n\x1a\n" or head[12:16] != b"IHDR":
        return None
    return struct.unpack(">IIBB", head[16:26])


def png_pixels(path):
    """An 8-bit RGBA PNG's pixels, as rows of (r, g, b, a) tuples: its IDAT chunks inflated and each row unfiltered
    (PNG's five filters, on 4-byte pixels)."""
    with open(path, "rb") as f:
        data = f.read()
    width, height = struct.unpack(">II", data[16:24])
    pos, idat = 8, b""
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        if kind == b"IDAT":
            idat += data[pos + 8:pos + 8 + length]
        pos += 12 + length
    raw, stride, rows, prev = zlib.decompress(idat), width * 4, [], bytearray(width * 4)
    for y in range(height):
        kind, line = raw[y * (stride + 1)], bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - 4] if i >= 4 else 0
            b, c = prev[i], prev[i - 4] if i >= 4 else 0
            if kind == 1:
                line[i] = (line[i] + a) & 255
            elif kind == 2:
                line[i] = (line[i] + b) & 255
            elif kind == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif kind == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else b if pb <= pc else c)) & 255
        rows.append([tuple(line[x * 4:x * 4 + 4]) for x in range(width)])
        prev = line
    return rows


def sprite_cells(svg):
    """The ship of a small image's SVG: for each of its two last paths (the rig, then the hull), its colour and the
    pixels its whole-pixel rectangles cover (M x y h w v1 h-w Z)."""
    root = ET.parse(svg).getroot()
    ship = []
    for path in root.findall(SVG_NS + "path")[-2:]:
        cells = set()
        for x, y, w in re.findall(r"M(\d+) (\d+)h(\d+)v1h-\d+Z", path.get("d")):
            cells.update((int(x) + i, int(y)) for i in range(int(w)))
        colour = path.get("fill").lstrip("#")
        ship.append((tuple(int(colour[i:i + 2], 16) for i in (0, 2, 4)), cells))
    return ship


def plate_inside(svg):
    """The pixels of a small image's plate inside its outermost pixel, both ways: the plate is the first path, the 1024
    grid's plate (100 to 924) under a translate and a scale, and its outermost pixel is the one-pixel band inside its
    outline."""
    plate = ET.parse(svg).getroot().findall(SVG_NS + "path")[0]
    offset, _, k = (float(v) for v in re.fullmatch(r"translate\((\S+) (\S+)\) scale\((\S+)\)",
                                                   plate.get("transform")).groups())
    first, last = offset + 100 * k, offset + 924 * k
    return range(math.ceil(first + 1), math.floor(last - 1))


def view_box(path):
    with open(path) as f:
        return [float(v) for v in re.search(r'viewBox="([^"]+)"', f.read()).group(1).split()]


def main():
    work = sys.argv[1]
    shutil.rmtree(work, ignore_errors=True)
    os.makedirs(work)

    # The generator writes the committed SVGs
    subprocess.run([sys.executable, GENERATOR, work], check=True)
    for name in SVGS:
        with open(os.path.join(BRANDING, name), "rb") as f:
            committed = f.read()
        with open(os.path.join(work, name), "rb") as f:
            generated = f.read()
        expect(f"{name} is what frigate_logo.py writes", committed == generated,
               "run port/assets/branding/tools/render.py and commit what it writes")
    expect(f"frigate_logo.py writes the {len(SVGS)} SVGs and nothing else", sorted(os.listdir(work)) == sorted(SVGS),
           ", ".join(sorted(os.listdir(work))))

    # Paths alone, with a title
    for name in SVGS:
        root = ET.parse(os.path.join(BRANDING, name)).getroot()
        expect(f"{name} is an SVG", root.tag == SVG_NS + "svg", root.tag)
        title = root.find(SVG_NS + "title")
        expect(f"{name} has a title", title is not None and title.text and title.text.startswith("Frigate"))
        for element in root.iter():
            tag = element.tag.replace(SVG_NS, "")
            expect(f"{name} draws with paths alone", tag in ALLOWED, tag)
            for attribute in element.attrib:
                expect(f"{name} links to nothing", "href" not in attribute, attribute)

    # The PNG masters' sizes
    for name, size in ICONS.items():
        header = png_header(os.path.join(BRANDING, name))
        expect(f"{name} is a 1024 x 1024 8-bit RGBA PNG", header == (*size, 8, 6), str(header))
    # The small images: their sizes, and the ship's pixels exactly where and as the SVG draws them
    for name, (size, svg) in SMALL.items():
        header = png_header(os.path.join(BRANDING, name))
        expect(f"{name} is a {size} x {size} 8-bit RGBA PNG", header == (size, size, 8, 6), str(header))
        if header != (size, size, 8, 6):
            continue
        pixels = png_pixels(os.path.join(BRANDING, name))
        inside = plate_inside(os.path.join(BRANDING, svg))
        for part, (colour, cells) in zip(("rig", "hull"), sprite_cells(os.path.join(BRANDING, svg))):
            out = sorted(cell for cell in cells if cell[0] not in inside or cell[1] not in inside)
            expect(f"{name}: the {part} clears the plate's outermost pixel (rows and columns {inside.start} to "
                   f"{inside.stop - 1})", not out, f"{len(out)} don't, first {out[:3]}")
            expect(f"{name} draws its {part}", len(cells) > 0)
            wrong = [cell for cell in cells if pixels[cell[1]][cell[0]] != (*colour, 255)]
            expect(f"{name}: every pixel of the {part} is its colour", not wrong,
                   f"{len(wrong)} aren't, first {wrong[:3]}")
            stray = [(x, y) for y in range(size) for x in range(size)
                     if pixels[y][x] == (*colour, 255) and (x, y) not in cells]
            expect(f"{name}: no other pixel is the {part}'s colour", not stray, f"{len(stray)} are, first {stray[:3]}")
    for name, svg in LOGOS.items():
        header = png_header(os.path.join(BRANDING, name))
        box = view_box(os.path.join(BRANDING, svg))
        height = round(LOGO_WIDTH * box[3] / box[2])
        expect(f"{name} is a {LOGO_WIDTH} x {height} 8-bit RGBA PNG", header == (LOGO_WIDTH, height, 8, 6), str(header))

    for failure in failures:
        print("FAIL " + failure)
    print(f"{checks} checks, {len(failures)} failed")
    shutil.rmtree(work, ignore_errors=True)
    return 1 if failures else 0


sys.exit(main())
