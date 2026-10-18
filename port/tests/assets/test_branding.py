"""Checks Frigate's artwork (port/assets/branding) as committed, on any platform, without rendering it:

- every SVG is what tools/frigate_logo.py writes today, byte for byte, so that the generator and the files never drift;
- every SVG is well-formed XML with a <title>, and draws with paths alone (plus the icons' one gradient): no text,
  fonts, images, scripts or links, which GitHub, browsers and AppKit would render differently or refuse;
- every PNG master is an 8-bit RGBA PNG of the size the build and the pages use: the app icons 1024 x 1024 (the macOS
  app's .icns and the window's icon are made from them), the logos 1200 px wide at their SVG's proportions.

Whether the PNGs show what the SVGs draw needs AppKit: tools/render.py renders them again (README.md).

usage: test_branding.py WORK_DIR   (the CMake test assets_branding, port/cmake/assets.cmake)
"""
import os
import re
import shutil
import struct
import subprocess
import sys
import xml.etree.ElementTree as ET

BRANDING = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "assets", "branding")
GENERATOR = os.path.join(BRANDING, "tools", "frigate_logo.py")
SVGS = ("mark.svg", "mark-dark.svg", "logo.svg", "logo-dark.svg", "icon-black2.svg", "icon-white2.svg")
ICONS = {"icon-black2-1024.png": (1024, 1024), "icon-white2-1024.png": (1024, 1024)}
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
    expect("frigate_logo.py writes the six SVGs and nothing else", sorted(os.listdir(work)) == sorted(SVGS),
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
