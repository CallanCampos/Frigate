#!/bin/sh
# Builds the macOS app from the program the host build made (port/cmake/macos_app.cmake runs it as the target
# <program>_app): <program>.app with the program in Contents/MacOS, the libraries it loads from outside macOS (SDL3,
# when it isn't linked statically) in Contents/Frameworks, and an Info.plist from port/app/macos/Info.plist.in, signed
# ad hoc (ADR 0003: no Apple Developer account, no notarization, no hardened runtime). The app holds no ROM and no
# game data: the player's ROM is found as the program finds it (port/hal/rom: --rom, POKEBW2_ROM, then the checkout's
# orig/ that the build was made from), and saves go where the program puts them (port/hal/card).
#
#   make_app.sh --program PATH --app PATH --plist TEMPLATE --bundle-id ID --display-name NAME --repo DIR
#
# The app is built next to its final path and moved there once it is signed and verified, so that a failed build
# leaves no half-made app behind.
set -eu

program=
app=
plist=
bundle_id=
display_name=
repo=
while [ $# -gt 0 ]; do
    case "$1" in
    --program) program=$2 ;;
    --app) app=$2 ;;
    --plist) plist=$2 ;;
    --bundle-id) bundle_id=$2 ;;
    --display-name) display_name=$2 ;;
    --repo) repo=$2 ;;
    *)
        echo "make_app.sh: unknown argument $1" >&2
        exit 2
        ;;
    esac
    shift 2
done
for value in "$program" "$app" "$plist" "$bundle_id" "$display_name" "$repo"; do
    if [ -z "$value" ]; then
        echo "make_app.sh: every option is needed (see the script's header)" >&2
        exit 2
    fi
done

name=$(basename "$program")
staging="$app.partial"
contents="$staging/Contents"
rm -rf "$staging"
mkdir -p "$contents/MacOS" "$contents/Frameworks"
cp "$program" "$contents/MacOS/$name"

# The libraries a binary loads from outside macOS: every dependency that isn't in /usr/lib or /System and isn't
# already relative to the binary (@rpath, @executable_path, @loader_path)
foreign_libraries() {
    otool -L "$1" | tail -n +2 | awk '{ print $1 }' | grep -v -e '^/usr/lib/' -e '^/System/' -e '^@' || true
}

# install_name_tool without its warning that a change invalidates the binary's signature: the app is signed last
name_tool() {
    if ! output=$(install_name_tool "$@" 2>&1); then
        printf '%s\n' "$output" >&2
        return 1
    fi
    if [ -n "$output" ]; then
        printf '%s\n' "$output" | grep -v 'invalidate the code signature' >&2 || true
    fi
}

# Copies each foreign library into Frameworks under its file name, makes its install name @rpath/NAME, points the
# binary that loads it there, and does the same for the libraries it loads in turn (the binary's own path is $1 in
# every call; the loop's variables are shared, so nothing reads them after the call that recurses)
embed_libraries() {
    for library in $(foreign_libraries "$1"); do
        leaf=$(basename "$library")
        name_tool -change "$library" "@rpath/$leaf" "$1"
        if [ ! -f "$contents/Frameworks/$leaf" ]; then
            cp -L "$library" "$contents/Frameworks/$leaf"
            chmod u+w "$contents/Frameworks/$leaf"
            name_tool -id "@rpath/$leaf" "$contents/Frameworks/$leaf"
            embed_libraries "$contents/Frameworks/$leaf"
        fi
    done
}
embed_libraries "$contents/MacOS/$name"

# The program's own search paths: only the bundle's Frameworks, so that a library of the same name elsewhere on the
# machine (Homebrew's) is never loaded in place of the one in the bundle
otool -l "$contents/MacOS/$name" | awk '$1 == "cmd" { rpath = ($2 == "LC_RPATH") } rpath && $1 == "path" { print $2 }' |
    while read -r path; do
        name_tool -delete_rpath "$path" "$contents/MacOS/$name"
    done
if [ -n "$(ls "$contents/Frameworks")" ]; then
    name_tool -add_rpath "@executable_path/../Frameworks" "$contents/MacOS/$name"
else
    rmdir "$contents/Frameworks"
fi

# The newest macOS any binary of the app was built for (LC_BUILD_VERSION's minos, or LC_VERSION_MIN_MACOSX's version
# for binaries built before it), compared part by part
minimum=$(
    for binary in "$contents/MacOS/$name" "$contents"/Frameworks/*; do
        [ -f "$binary" ] || continue
        otool -l "$binary" | awk '
            $1 == "cmd" { build = ($2 == "LC_BUILD_VERSION"); old = ($2 == "LC_VERSION_MIN_MACOSX") }
            build && $1 == "minos" { print $2 }
            old && $1 == "version" { print $2 }'
    done | awk -F. '
        { major = $1 + 0; minor = $2 + 0; patch = $3 + 0
          if (!seen || major > bm || (major == bm && (minor > bn || (minor == bn && patch > bp)))) {
              bm = major; bn = minor; bp = patch; seen = 1
          } }
        END { if (seen) { printf "%d.%d", bm, bn; if (bp) printf ".%d", bp; print "" } }'
)
if [ -z "$minimum" ]; then
    echo "make_app.sh: no binary of the app says which macOS it was built for" >&2
    exit 1
fi

# The versions from git (Info.plist.in says what each is)
count=$(git -C "$repo" rev-list --count HEAD)
revision=$(git -C "$repo" describe --always --dirty --abbrev=12)

sed -e "s|@DISPLAY_NAME@|$display_name|" -e "s|@EXECUTABLE@|$name|g" -e "s|@BUNDLE_ID@|$bundle_id|" \
    -e "s|@SHORT_VERSION@|0.0.$count|" -e "s|@BUILD_VERSION@|$count|" -e "s|@MINIMUM_SYSTEM_VERSION@|$minimum|" \
    -e "s|@REVISION@|$revision|" "$plist" >"$contents/Info.plist"
unfilled=$(grep -o '@[A-Z_]*@' "$contents/Info.plist" | head -1 || true)
if [ -n "$unfilled" ]; then
    echo "make_app.sh: Info.plist still has a field to fill: $unfilled" >&2
    exit 1
fi
plutil -lint -s "$contents/Info.plist"
printf 'APPL????' >"$contents/PkgInfo"

# Ad hoc, the whole bundle with what it embeds, then checked as Gatekeeper's assessment checks it
codesign --force --deep --sign - "$staging"
codesign --verify --deep --strict "$staging"

rm -rf "$app"
mv "$staging" "$app"
echo "make_app.sh: $app (bundle $bundle_id, version 0.0.$count, $revision, macOS $minimum or later), signed ad hoc"
