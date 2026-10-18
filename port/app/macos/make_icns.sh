#!/bin/sh
# Makes the macOS app's icon file (the .icns that Finder, the Dock and the About window show) from the version's
# artwork in port/assets/branding: Black 2's dark plate or White 2's light one, with Frigate's ship.
# port/cmake/macos_app.cmake runs it when the artwork changes, and make_app.sh puts the result in the app as
# Contents/Resources/AppIcon.icns, which the Info.plist's CFBundleIconFile names. The .icns isn't committed: it is made
# at each build, with macOS's own tools.
#
#   make_icns.sh MASTER.png SMALL16.png SMALL32.png OUT.icns
#
# The .icns holds the ten images of a macOS icon set, 16 to 512 points at 1x and 2x (16 to 1024 pixels), and iconutil
# packs them. The 16 and 32 pixel images are the small ones drawn for their size, pixel by pixel (icon-<version>-16.png
# and -32.png), since a scaled ship blurs its sails and loses its ram there: SMALL16 is the 16 point image, and SMALL32
# both the 16 point image at 2x and the 32 point one. The others are scaled from the 1024 x 1024 master by sips. Each
# must be a PNG of its size with an alpha channel (port/assets/branding/README.md says how they are rendered).
# port/tests/app/test_make_app.sh tests it.
set -eu

if [ $# -ne 4 ]; then
    echo "usage: make_icns.sh MASTER.png SMALL16.png SMALL32.png OUT.icns" >&2
    exit 2
fi
master=$1
small16=$2
small32=$3
out=$4

# A PNG's property, as sips reads it
property() {
    sips -g "$2" "$1" 2>/dev/null | awk -v key="$2:" '$1 == key { print $2 }'
}
# Stops unless the PNG $1 is $2 x $2 with alpha
check() {
    if [ ! -f "$1" ]; then
        echo "make_icns.sh: no image at $1" >&2
        exit 1
    fi
    width=$(property "$1" pixelWidth)
    height=$(property "$1" pixelHeight)
    alpha=$(property "$1" hasAlpha)
    if [ "$width" != "$2" ] || [ "$height" != "$2" ] || [ "$alpha" != yes ]; then
        echo "make_icns.sh: $1 must be a $2 x $2 PNG with alpha (it is ${width:-?} x ${height:-?}," \
            "alpha ${alpha:-?})" >&2
        exit 1
    fi
}
check "$master" 1024
check "$small16" 16
check "$small32" 32

# The icon set's file names and pixel sizes, as iconutil expects them
work=$(mktemp -d "${TMPDIR:-/tmp}/make_icns.XXXXXX")
trap 'rm -rf "$work"' EXIT
iconset="$work/AppIcon.iconset"
mkdir "$iconset"
cp "$small16" "$iconset/icon_16x16.png"
cp "$small32" "$iconset/icon_16x16@2x.png"
cp "$small32" "$iconset/icon_32x32.png"
sips -z 64 64 "$master" --out "$iconset/icon_32x32@2x.png" >/dev/null
for points in 128 256 512; do
    sips -z "$points" "$points" "$master" --out "$iconset/icon_${points}x${points}.png" >/dev/null
    sips -z $((points * 2)) $((points * 2)) "$master" --out "$iconset/icon_${points}x${points}@2x.png" >/dev/null
done

# Packed in the temporary directory (iconutil wants an output named .icns), then moved into place
iconutil --convert icns --output "$work/AppIcon.icns" "$iconset"
mkdir -p "$(dirname "$out")"
mv "$work/AppIcon.icns" "$out"
