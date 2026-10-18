"""Regenerates Frigate's artwork in port/assets/branding: the SVGs from frigate_logo.py, then their PNG masters, rendered
by AppKit (sheet.swift): the two app icons at 1024 x 1024, which the macOS app's .icns and the window icon on Linux and
Windows are built from (port/cmake/macos_app.cmake, port/cmake/app.cmake), the icons' small images at their own size
(16 and 32 px for the .icns, 32 px for the window), and the two logos at 1200 px wide. With
--sheet, it also writes a contact sheet (contact.py) that shows every file on light and dark backgrounds and at the
small sizes, for looking at a change before committing it. macOS only: it needs swiftc, from Xcode's command line
tools, which it runs to build the renderer in a temporary directory.

usage: python3 port/assets/branding/tools/render.py [--sheet PATH]

The contact sheet is a working file, not part of the set: put it outside the repository or under build/.
"""
import argparse
import os
import re
import subprocess
import sys
import tempfile

TOOLS = os.path.dirname(os.path.abspath(__file__))
BRANDING = os.path.dirname(TOOLS)
ICONS = ("icon-black2", "icon-white2")
# The small images drawn pixel by pixel: each SVG and the size it is drawn for, which its PNG is named with
SMALL = (("icon-black2-16", 16), ("icon-black2-32", 32), ("icon-white2-16", 16), ("icon-white2-32", 32),
         ("window-black2", 32), ("window-white2", 32))
LOGOS = ("logo", "logo-dark")
ICON_SIZE = 1024
LOGO_WIDTH = 1200


def run(*args):
    subprocess.run(args, check=True)


def aspect(svg):
    """An SVG's width over its height, from its viewBox."""
    with open(svg) as f:
        view = re.search(r'viewBox="([^"]+)"', f.read()).group(1).split()
    return float(view[2]) / float(view[3])


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--sheet", help="also write the contact sheet to this PNG")
    args = parser.parse_args()
    if sys.platform != "darwin":
        sys.exit("render.py: the PNGs are rendered with AppKit, on macOS")

    with tempfile.TemporaryDirectory(prefix="frigate-render.") as work:
        sheet = os.path.join(work, "sheet")
        run("swiftc", "-O", "-o", sheet, os.path.join(TOOLS, "sheet.swift"))

        def render(spec, png):
            path = os.path.join(work, "spec.txt")
            with open(path, "w") as f:
                f.write(spec)
            run(sheet, path, png)

        run(sys.executable, os.path.join(TOOLS, "frigate_logo.py"), BRANDING)
        for icon in ICONS:
            svg, png = os.path.join(BRANDING, f"{icon}.svg"), os.path.join(BRANDING, f"{icon}-{ICON_SIZE}.png")
            render(f"canvas {ICON_SIZE} {ICON_SIZE} none\nimg {svg} 0 0 {ICON_SIZE} {ICON_SIZE}\n", png)
        for name, size in SMALL:
            svg = os.path.join(BRANDING, f"{name}.svg")
            png = os.path.join(BRANDING, f"{name}.png" if name.startswith("icon-") else f"{name}-{size}.png")
            render(f"canvas {size} {size} none\nimg {svg} 0 0 {size} {size}\n", png)
        for logo in LOGOS:
            svg, png = os.path.join(BRANDING, f"{logo}.svg"), os.path.join(BRANDING, f"{logo}.png")
            height = round(LOGO_WIDTH / aspect(svg))
            render(f"canvas {LOGO_WIDTH} {height} none\nimg {svg} 0 0 {LOGO_WIDTH} {height}\n", png)
        if args.sheet:
            spec = os.path.join(work, "contact.txt")
            run(sys.executable, os.path.join(TOOLS, "contact.py"), BRANDING, spec)
            run(sheet, spec, os.path.abspath(args.sheet))
    print(f"render.py: the SVGs and PNGs in {BRANDING}" + (f", and the contact sheet {args.sheet}" if args.sheet else ""))


main()
