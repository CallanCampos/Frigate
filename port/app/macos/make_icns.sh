#!/bin/sh
# Makes the macOS app's icon file (the .icns that Finder, the Dock and the About window show) from the version's
# 1024 x 1024 PNG master in port/assets/branding: Black 2's dark plate or White 2's light one, with Frigate's ship.
# port/cmake/macos_app.cmake runs it when the master changes, and make_app.sh puts the result in the app as
# Contents/Resources/AppIcon.icns, which the Info.plist's CFBundleIconFile names. The .icns isn't committed: it is
# made from the master at each build, with macOS's own tools.
#
#   make_icns.sh MASTER.png OUT.icns
#
# The .icns holds the ten images of a macOS icon set, 16 to 512 points at 1x and 2x (16 to 1024 pixels), each scaled
# from the master by sips; iconutil packs them. The master must be a 1024 x 1024 PNG with an alpha channel
# (port/assets/branding/README.md says how it is rendered). port/tests/app/test_make_app.sh tests it.
set -eu

if [ $# -ne 2 ]; then
    echo "usage: make_icns.sh MASTER.png OUT.icns" >&2
    exit 2
fi
master=$1
out=$2

# The master's size and alpha, as sips reads them
property() {
    sips -g "$1" "$master" 2>/dev/null | awk -v key="$1:" '$1 == key { print $2 }'
}
if [ ! -f "$master" ]; then
    echo "make_icns.sh: no master at $master" >&2
    exit 1
fi
width=$(property pixelWidth)
height=$(property pixelHeight)
alpha=$(property hasAlpha)
if [ "$width" != 1024 ] || [ "$height" != 1024 ] || [ "$alpha" != yes ]; then
    echo "make_icns.sh: $master must be a 1024 x 1024 PNG with alpha (it is ${width:-?} x ${height:-?}," \
        "alpha ${alpha:-?})" >&2
    exit 1
fi

# The icon set's file names and pixel sizes, as iconutil expects them
work=$(mktemp -d "${TMPDIR:-/tmp}/make_icns.XXXXXX")
trap 'rm -rf "$work"' EXIT
iconset="$work/AppIcon.iconset"
mkdir "$iconset"
for points in 16 32 128 256 512; do
    sips -z "$points" "$points" "$master" --out "$iconset/icon_${points}x${points}.png" >/dev/null
    sips -z $((points * 2)) $((points * 2)) "$master" --out "$iconset/icon_${points}x${points}@2x.png" >/dev/null
done

# Packed in the temporary directory (iconutil wants an output named .icns), then moved into place
iconutil --convert icns --output "$work/AppIcon.icns" "$iconset"
mkdir -p "$(dirname "$out")"
mv "$work/AppIcon.icns" "$out"
