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
# Before it signs, it checks that every binary of the app loads nothing from outside macOS and the app, and after, that
# the program starts from the app (--help). The app is built next to its final path and moved there once it is signed
# and verified, so that a failed build leaves no half-made app behind. port/tests/app/test_make_app.sh tests it.
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
frameworks="$contents/Frameworks"
queue="$staging.queue"
# A build that fails leaves nothing behind it; one that succeeds has moved the staging app into place by then
trap 'rm -rf "$staging" "$queue" "$queue.names"' EXIT
rm -rf "$staging" "$queue"
mkdir -p "$contents/MacOS" "$frameworks"
cp "$program" "$contents/MacOS/$name"
program_dir=$(cd "$(dirname "$program")" && pwd -P)

# The install names a binary loads, one per line (of every slice of a universal binary), without a library's own id
dependencies() {
    id=$(otool -D "$1" | tail -n +2 | head -1)
    otool -L "$1" | sed -n 's/^[[:space:]]\{1,\}\(.*\) (compatibility version.*$/\1/p' | sort -u |
        while IFS= read -r dependency; do
            if [ "$dependency" != "$id" ]; then
                printf '%s\n' "$dependency"
            fi
        done
}

# A binary's search paths (LC_RPATH), one per line
search_paths() {
    otool -l "$1" | awk '
        $1 == "cmd" { rpath = ($2 == "LC_RPATH") }
        rpath && $1 == "path" { sub(/^ *path /, ""); sub(/ \(offset [0-9]+\)$/, ""); print }'
}

# Whether a path is macOS's own: its libraries are in /usr/lib and /System, some only in dyld's shared cache
is_system() {
    case "$1" in
    /usr/lib/* | /System/*) return 0 ;;
    *) return 1 ;;
    esac
}

# @loader_path and @executable_path in a path, as dyld expands them for a binary that was at $2 (the program's own
# directory for @executable_path)
expand() {
    case "$1" in
    @loader_path/*) printf '%s/%s\n' "$(dirname "$2")" "${1#@loader_path/}" ;;
    @loader_path) dirname "$2" ;;
    @executable_path/*) printf '%s/%s\n' "$program_dir" "${1#@executable_path/}" ;;
    @executable_path) printf '%s\n' "$program_dir" ;;
    *) printf '%s\n' "$1" ;;
    esac
}

# The file that dyld loads for an install name ($1) of the binary that was at $2: an absolute path as it is, a path
# from @loader_path or @executable_path expanded, and @rpath/NAME in the binary's search paths, then the program's,
# in the order dyld searches them for a library the program loads. "system" for macOS's own, and for @rpath/NAME when
# the first search path is macOS's, which only names a library macOS has (maybe only in dyld's shared cache, with no
# file to look for); empty for a file that isn't there
locate() {
    case "$1" in
    @rpath/*)
        # A search path of the binary's own is relative to the binary, and one of the program's to the program
        { search_paths "$2" | sed 's/^/own /'; search_paths "$program" | sed 's/^/program /'; } |
        while read -r whose directory; do
            if [ "$whose" = own ]; then owner=$2; else owner=$program; fi
            candidate="$(expand "$directory" "$owner")/${1#@rpath/}"
            if is_system "$candidate"; then
                echo system
                break
            elif [ -f "$candidate" ]; then
                printf '%s\n' "$candidate"
                break
            fi
        done
        ;;
    *)
        candidate=$(expand "$1" "$2")
        if is_system "$candidate"; then
            echo system
        elif [ -f "$candidate" ]; then
            printf '%s\n' "$candidate"
        fi
        ;;
    esac
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

# The libraries the program loads from outside macOS, and those they load in turn, go into Frameworks under their file
# names with @rpath/NAME as their install names, whether the binary named them by an absolute path, by @rpath (as
# SDL3's own CMake install does) or from @loader_path or @executable_path. The queue holds each binary of the app and
# where it was copied from, which its @loader_path and its own search paths are relative to; each line is handled once
printf '%s\t%s\n' "$contents/MacOS/$name" "$program" >"$queue"
handled=0
while :; do
    entry=$(sed -n "$((handled + 1))p" "$queue")
    if [ -z "$entry" ]; then
        break
    fi
    handled=$((handled + 1))
    binary=${entry%%	*}
    source=${entry#*	}
    dependencies "$binary" >"$queue.names"
    while IFS= read -r dependency; do
        if is_system "$dependency"; then
            continue
        fi
        file=$(locate "$dependency" "$source")
        if [ "$file" = system ]; then
            continue
        fi
        if [ -z "$file" ]; then
            echo "make_app.sh: $source loads $dependency, which isn't on this machine" >&2
            exit 1
        fi
        leaf=$(basename "$dependency")
        if [ "$dependency" != "@rpath/$leaf" ]; then
            name_tool -change "$dependency" "@rpath/$leaf" "$binary"
        fi
        if [ ! -f "$frameworks/$leaf" ]; then
            cp -L "$file" "$frameworks/$leaf"
            chmod u+w "$frameworks/$leaf"
            name_tool -id "@rpath/$leaf" "$frameworks/$leaf"
            printf '%s\t%s\n' "$frameworks/$leaf" "$file" >>"$queue"
        else
            # The same library by another path is the same file; another library of the same name has no place
            first=$(copy="$frameworks/$leaf" awk -F '\t' '$1 == ENVIRON["copy"] { print $2; exit }' "$queue")
            if ! cmp -s "$file" "$first"; then
                echo "make_app.sh: two libraries named $leaf: $file and $first" >&2
                exit 1
            fi
        fi
    done <"$queue.names"
done

# Search paths: macOS's own stay; every other is replaced by the bundle's Frameworks, @executable_path/../Frameworks for
# the program and @loader_path for the libraries there, so that a library of the same name elsewhere on the machine
# (Homebrew's, or a build's) is never loaded in place of the one in the app
cut -f1 "$queue" | while IFS= read -r binary; do
    search_paths "$binary" | while IFS= read -r directory; do
        if ! is_system "$directory"; then
            name_tool -delete_rpath "$directory" "$binary"
        fi
    done
    if dependencies "$binary" | grep -q '^@rpath/'; then
        if [ "$binary" = "$contents/MacOS/$name" ]; then
            name_tool -add_rpath "@executable_path/../Frameworks" "$binary"
        else
            name_tool -add_rpath "@loader_path" "$binary"
        fi
    fi
done
rm -f "$queue" "$queue.names"
if [ -z "$(ls "$frameworks")" ]; then
    rmdir "$frameworks"
fi

# Every binary of the app now loads only macOS's libraries and the app's own: a dependency outside macOS is
# @rpath/NAME with NAME in Frameworks, and the only search paths outside macOS are the app's
for binary in "$contents/MacOS/$name" "$frameworks"/*; do
    [ -f "$binary" ] || continue
    dependencies "$binary" | while IFS= read -r dependency; do
        case "$dependency" in
        /usr/lib/* | /System/*) ;;
        @rpath/*)
            if [ ! -f "$frameworks/${dependency#@rpath/}" ]; then
                echo "make_app.sh: $binary loads $dependency, which the app doesn't hold" >&2
                exit 1
            fi
            ;;
        *)
            echo "make_app.sh: $binary loads $dependency from outside the app" >&2
            exit 1
            ;;
        esac
    done
    search_paths "$binary" | while IFS= read -r directory; do
        case "$directory" in
        /usr/lib/* | /System/* | @executable_path/../Frameworks | @loader_path) ;;
        *)
            echo "make_app.sh: $binary still searches $directory" >&2
            exit 1
            ;;
        esac
    done
done

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

# The program starts from the app: dyld loads every library from where the app says, or stops it at once. --help
# returns before anything opens, and its output is kept (the program writes its messages to a log file when it runs
# from an app with nowhere to write them, port/app/main.c). A program for another architecture isn't run
if lipo -archs "$contents/MacOS/$name" | tr ' ' '\n' | grep -qx "$(uname -m)"; then
    if ! output=$(env -u DYLD_LIBRARY_PATH -u DYLD_FALLBACK_LIBRARY_PATH -u DYLD_FRAMEWORK_PATH \
        -u DYLD_FALLBACK_FRAMEWORK_PATH -u DYLD_INSERT_LIBRARIES "$contents/MacOS/$name" --help 2>&1); then
        printf '%s\n' "$output" >&2
        echo "make_app.sh: the program doesn't start from the app" >&2
        exit 1
    fi
fi

rm -rf "$app"
mv "$staging" "$app"
echo "make_app.sh: $app (bundle $bundle_id, version 0.0.$count, $revision, macOS $minimum or later), signed ad hoc"
