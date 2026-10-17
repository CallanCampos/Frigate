#!/bin/sh
# Builds the macOS app and puts it in a disk image (PLAN.md phase 5, ADR 0003): the target <program>_app of a
# configured host build (port/cmake/macos_app.cmake) makes <program>.app, signed ad hoc, and hdiutil makes a compressed
# disk image with the app and a link to /Applications to drag it onto. The image is checked before the script ends: it
# verifies, the app in it passes codesign's strict check, and it holds no ROM, save or other file of the game's. It is
# only ever made locally: the port's compiled game code is never uploaded or distributed (port/CLAUDE.md).
#
#   port/tools/package_macos.sh [--build-dir DIR] [--out PATH]
#
#   --build-dir DIR  a host build configured with CMake (build/port by default); its PORT_VERSION picks the program
#   --out PATH       the disk image (DIR/<program>-<version>.dmg by default). A path in the repository must be one git
#                    ignores, such as one under build/, so that the image is never committed
set -eu

repo=$(cd "$(dirname "$0")/../.." && pwd)
build_dir="$repo/build/port"
out=
while [ $# -gt 0 ]; do
    case "$1" in
    --build-dir)
        build_dir=$2
        shift 2
        ;;
    --out)
        out=$2
        shift 2
        ;;
    -h | --help)
        awk 'NR > 1 && /^#/ { sub(/^# ?/, ""); print; next } NR > 1 { exit }' "$0"
        exit 0
        ;;
    *)
        echo "package_macos.sh: unknown argument $1" >&2
        exit 2
        ;;
    esac
done
if [ "$(uname -s)" != Darwin ]; then
    echo "package_macos.sh: the app and its disk image are made on macOS" >&2
    exit 2
fi
if [ ! -f "$build_dir/CMakeCache.txt" ]; then
    echo "package_macos.sh: $build_dir isn't a configured build; run cmake -S port -B $build_dir -G Ninja first" >&2
    exit 2
fi

# Makes $out absolute and refuses a path in the repository that git would track
check_out() {
    case "$out" in
    /*) ;;
    *) out="$(pwd)/$out" ;;
    esac
    case "$out" in
    "$repo"/*)
        if ! git -C "$repo" check-ignore -q "$out"; then
            echo "package_macos.sh: $out is in the repository and git doesn't ignore it; put the image under build/" >&2
            exit 2
        fi
        ;;
    esac
}
if [ -n "$out" ]; then
    check_out
fi

version_name=$(sed -n 's/^PORT_VERSION:[A-Z]*=//p' "$build_dir/CMakeCache.txt")
case "$version_name" in
b2_us) program=pokeblack2 ;;
w2_us) program=pokewhite2 ;;
*)
    echo "package_macos.sh: $build_dir has no PORT_VERSION this script knows ($version_name)" >&2
    exit 2
    ;;
esac

cmake --build "$build_dir" --target "${program}_app"
app="$build_dir/$program.app"
plist="$app/Contents/Info.plist"
version=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$plist")
display_name=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleDisplayName' "$plist")
if [ -z "$out" ]; then
    out="$build_dir/$program-$version.dmg"
fi
check_out
mkdir -p "$(dirname "$out")"

work=$(mktemp -d "${TMPDIR:-/tmp}/package_macos.XXXXXX")
mount_point="$work/mount"
cleanup() {
    if [ -d "$mount_point" ]; then
        hdiutil detach -quiet "$mount_point" 2>/dev/null || true
    fi
    rm -rf "$work"
}
trap cleanup EXIT
trap 'exit 130' INT TERM

# The image's contents: the app, copied with ditto to keep its signature's extended attributes, and /Applications
mkdir "$work/image"
ditto "$app" "$work/image/$program.app"
ln -s /Applications "$work/image/Applications"
rm -f "$out"
hdiutil create -quiet -volname "$display_name" -srcfolder "$work/image" -fs HFS+ -format UDZO "$out"

# The checks: the image's checksums, the app's signature as it is in the image, and nothing of the game's in it
hdiutil verify -quiet "$out"
mkdir "$mount_point"
hdiutil attach -quiet -readonly -nobrowse -mountpoint "$mount_point" "$out"
codesign --verify --deep --strict "$mount_point/$program.app"
found=$(find "$mount_point" \( -iname '*.nds' -o -iname '*.srl' -o -iname '*.sav' -o -iname '*.dsv' -o -iname '*.narc' \
    -o -iname '*.sdat' \) -print)
if [ -n "$found" ]; then
    echo "package_macos.sh: the image holds files of the game's:" >&2
    echo "$found" >&2
    exit 1
fi
hdiutil detach -quiet "$mount_point"
rmdir "$mount_point"

size=$(du -h "$out" | cut -f1)
echo "package_macos.sh: $out ($size): $program.app $version, signed ad hoc"
