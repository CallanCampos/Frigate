#!/bin/sh
# Builds the macOS app and puts it in a disk image (PLAN.md phase 5, ADR 0003): the target <program>_app of a
# configured host build (port/cmake/macos_app.cmake) makes the app ("Frigate Black 2.app" or "Frigate White 2.app"),
# signed ad hoc, and hdiutil makes a compressed disk image named after it, with the app and a link to /Applications to
# drag it onto. The image is checked before the script ends: it verifies, the app in it passes codesign's strict check,
# its program has the slices and its Info.plist the minimum macOS the build was configured for, and it holds no ROM,
# save or other file of the game's. It is only ever made locally: the port's compiled game code is never uploaded or
# distributed (port/CLAUDE.md).
#
#   port/tools/package_macos.sh [--build-dir DIR] [--out PATH]
#   port/tools/package_macos.sh --distribution [--version b2_us|w2_us] [--build-dir DIR] [--out PATH]
#
#   --build-dir DIR  a host build configured with CMake (build/port by default); its PORT_VERSION picks the app
#   --out PATH       the disk image (by default DIR/<app> <version>.dmg, "Frigate Black 2 0.0.N.dmg"). A path in a git
#                    work tree, this one or another, must be one git ignores, such as one under build/, so that the
#                    image is never committed
#   --distribution   the app for other Macs than this one, in one command (ADR 0013): one universal program, arm64 and
#                    x86_64, for macOS 11.0 and later, with the pinned SDL3 built along and linked statically. DIR is
#                    build/port-dist by default (build/port-dist-w2 for White 2), and the script configures it with
#                    -DPORT_MACOS_DISTRIBUTION=ON when it isn't configured yet; one configured without it is refused
#   --version V      with --distribution, the game: b2_us (Black 2, the default) or w2_us (White 2)
set -eu

repo=$(cd "$(dirname "$0")/../.." && pwd)
build_dir=
out=
distribution=
version=
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
    --distribution)
        distribution=yes
        shift
        ;;
    --version)
        version=$2
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
case "$version" in
'' | b2_us | w2_us) ;;
*)
    echo "package_macos.sh: --version is b2_us or w2_us, not $version" >&2
    exit 2
    ;;
esac
if [ -n "$version" ] && [ -z "$distribution" ]; then
    echo "package_macos.sh: --version goes with --distribution; a development build's game is its PORT_VERSION" >&2
    exit 2
fi
if [ -z "$build_dir" ]; then
    if [ -z "$distribution" ]; then
        build_dir="$repo/build/port"
    elif [ "$version" = w2_us ]; then
        build_dir="$repo/build/port-dist-w2"
    else
        build_dir="$repo/build/port-dist"
    fi
fi

# Makes $out absolute and refuses a path that git would track, in this repository or in any other (the clone this
# worktree sits in, say): the image goes only where no git add takes it, under an ignored directory such as build/ or
# outside every work tree. Git looks from the nearest directory of the path that exists, through any links
check_out() {
    case "$out" in
    /*) ;;
    *) out="$(pwd)/$out" ;;
    esac
    existing=$(dirname "$out")
    while [ ! -d "$existing" ]; do
        existing=$(dirname "$existing")
    done
    rest=${out#"$existing"/}
    existing=$(cd "$existing" && pwd -P)
    if [ "$(git -C "$existing" rev-parse --is-inside-work-tree 2>/dev/null)" = true ] &&
        ! git -C "$existing" check-ignore -q "$existing/$rest"; then
        echo "package_macos.sh: $out is in the git work tree $(git -C "$existing" rev-parse --show-toplevel), and git" \
            "doesn't ignore it; put the image under build/ or outside every repository" >&2
        exit 2
    fi
}
if [ -n "$out" ]; then
    check_out
fi

# A value of the build's cache
cache_value() {
    sed -n "s/^$1:[A-Z]*=//p" "$build_dir/CMakeCache.txt"
}

# The distribution build: configured here when it isn't yet, refused when it was configured as another build. Its
# configuration builds SDL3's pinned release, so the first one takes a few minutes
if [ -n "$distribution" ]; then
    if [ ! -f "$build_dir/CMakeCache.txt" ]; then
        cmake -S "$repo/port" -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo -DPORT_MACOS_DISTRIBUTION=ON \
            -DPORT_VERSION="${version:-b2_us}"
    fi
    case "$(cache_value PORT_MACOS_DISTRIBUTION)" in
    ON | on | TRUE | true | YES | yes | 1) ;;
    *)
        echo "package_macos.sh: $build_dir is configured without -DPORT_MACOS_DISTRIBUTION=ON; give --distribution" \
            "a build directory of its own" >&2
        exit 2
        ;;
    esac
    if [ -n "$version" ] && [ "$(cache_value PORT_VERSION)" != "$version" ]; then
        echo "package_macos.sh: $build_dir builds $(cache_value PORT_VERSION), not $version" >&2
        exit 2
    fi
fi
if [ ! -f "$build_dir/CMakeCache.txt" ]; then
    echo "package_macos.sh: $build_dir isn't a configured build; run cmake -S port -B $build_dir -G Ninja first" >&2
    exit 2
fi

# The app's target and name, as port/cmake/macos_app.cmake records them in the build's cache, and what it is built for
target=$(cache_value PORT_MACOS_APP_TARGET)
app_name=$(cache_value PORT_MACOS_APP_NAME)
if [ -z "$target" ] || [ -z "$app_name" ]; then
    echo "package_macos.sh: $build_dir doesn't name its macOS app; configure it again:" \
        "cmake -S port -B $build_dir" >&2
    exit 2
fi

archs=$(cache_value CMAKE_OSX_ARCHITECTURES | tr ';' ' ')
minimum=$(cache_value CMAKE_OSX_DEPLOYMENT_TARGET)

cmake --build "$build_dir" --target "$target"
app="$build_dir/$app_name.app"
plist="$app/Contents/Info.plist"
app_version=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' "$plist")
executable=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$plist")
if [ -z "$out" ]; then
    out="$build_dir/$app_name $app_version.dmg"
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
ditto "$app" "$work/image/$app_name.app"
ln -s /Applications "$work/image/Applications"
rm -f "$out"
hdiutil create -quiet -volname "$app_name" -srcfolder "$work/image" -fs HFS+ -format UDZO "$out"

# The checks: the image's checksums, the app's signature as it is in the image, the slices and the minimum macOS the
# build was configured for (make_app.sh checked every binary already; this is the app as it is in the image), and
# nothing of the game's in it
hdiutil verify -quiet "$out"
mkdir "$mount_point"
hdiutil attach -quiet -readonly -nobrowse -mountpoint "$mount_point" "$out"
codesign --verify --deep --strict "$mount_point/$app_name.app"
slices=$(lipo -archs "$mount_point/$app_name.app/Contents/MacOS/$executable")
for wanted in $archs; do
    if ! printf '%s\n' $slices | grep -qxF -e "$wanted"; then
        echo "package_macos.sh: the app's program has no $wanted slice (it has $slices)" >&2
        exit 1
    fi
done
app_minimum=$(/usr/libexec/PlistBuddy -c 'Print :LSMinimumSystemVersion' \
    "$mount_point/$app_name.app/Contents/Info.plist")
if [ -n "$minimum" ] && [ "$app_minimum" != "$minimum" ]; then
    echo "package_macos.sh: the app says macOS $app_minimum or later; the build is for $minimum" >&2
    exit 1
fi
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
echo "package_macos.sh: $out ($size): $app_name.app $app_version, $slices, macOS $app_minimum or later, signed ad hoc"
