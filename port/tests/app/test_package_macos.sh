#!/bin/sh
# Tests that port/tools/package_macos.sh refuses to write the disk image where git would take it into a repository: in
# this repository, in another one, through a link, and in one whose directories for the image don't exist yet; and its
# refusals of the distribution build's options (ADR 0015): --distribution on a build directory configured without
# PORT_MACOS_DISTRIBUTION, or for another game than --version's, and --version without --distribution or with a game
# that isn't one; and a sanitized build, whose app carries the sanitizer runtime (port/docs/sanitizers.md), configured
# with PORT_SANITIZE, with or without --distribution, or with -fsanitize= in its C flags, while one with PORT_SANITIZE
# off goes on past that check. Each refusal comes before anything is built, configured or written, so the test builds
# nothing: those cases use build directories of a CMakeCache.txt alone.
#
#   test_package_macos.sh REPO BUILD_DIR WORK_DIR
set -u

repo=$1
build_dir=$2
work=$3
script="$repo/port/tools/package_macos.sh"
checks=0
failures=0

expect() {
    checks=$((checks + 1))
    if [ "$2" != "$3" ]; then
        failures=$((failures + 1))
        printf 'FAIL %s: "%s", expected "%s"\n' "$1" "$2" "$3"
    fi
}

# The script's status for an --out path, its messages in $work/messages
package() {
    "$script" --build-dir "$build_dir" --out "$1" >/dev/null 2>"$work/messages"
    echo $?
}

# A refusal: status 2, the message naming the work tree, and no image
refused() {
    expect "$1: status" "$(package "$2")" 2
    expect "$1: message" "$(grep -cF "is in the git work tree $3, and git doesn't ignore it" "$work/messages")" 1
    expect "$1: no image" "$(test -e "$2" && echo made || echo none)" none
}

rm -rf "$work"
mkdir -p "$work/other"
git init -q "$work/other"
other=$(cd "$work/other" && pwd -P)
ln -s "$work/other" "$work/link"
top=$(git -C "$repo" rev-parse --show-toplevel)

refused "this repository, a tracked directory" "$repo/port/test-image.dmg" "$top"
refused "another repository" "$work/other/image.dmg" "$other"
refused "another repository, directories to make" "$work/other/new/dir/image.dmg" "$other"
refused "another repository, through a link" "$work/link/image.dmg" "$other"
expect "a refusal makes no directory" "$(test -e "$work/other/new" && echo made || echo none)" none

# The script's status for its options, its messages in $work/messages
options() {
    "$script" "$@" >/dev/null 2>"$work/messages"
    echo $?
}
mkdir -p "$work/dev" "$work/dist"
printf 'PORT_MACOS_DISTRIBUTION:BOOL=OFF\nPORT_VERSION:STRING=b2_us\n' >"$work/dev/CMakeCache.txt"
printf 'PORT_MACOS_DISTRIBUTION:BOOL=ON\nPORT_VERSION:STRING=b2_us\n' >"$work/dist/CMakeCache.txt"
expect "--distribution on a development build: status" "$(options --distribution --build-dir "$work/dev")" 2
expect "--distribution on a development build: message" \
    "$(grep -c "is configured without -DPORT_MACOS_DISTRIBUTION=ON" "$work/messages")" 1
expect "--distribution for another game: status" \
    "$(options --distribution --version w2_us --build-dir "$work/dist")" 2
expect "--distribution for another game: message" "$(grep -c "builds b2_us, not w2_us" "$work/messages")" 1
expect "--version without --distribution: status" "$(options --version w2_us --build-dir "$work/dist")" 2
expect "--version without --distribution: message" "$(grep -c "goes with --distribution" "$work/messages")" 1
expect "--version of no game: status" "$(options --distribution --version b2_eu --build-dir "$work/dist")" 2
expect "--version of no game: message" "$(grep -c "is b2_us or w2_us, not b2_eu" "$work/messages")" 1

# A sanitized build: refused, with or without --distribution and however it was made sanitized, the option read in any
# case, as CMake reads it. One with PORT_SANITIZE off goes on, to the check of its app's name that a cache of these
# alone fails
mkdir -p "$work/san" "$work/san-dist" "$work/san-flags" "$work/plain"
printf 'PORT_SANITIZE:BOOL=ON\nPORT_VERSION:STRING=b2_us\n' >"$work/san/CMakeCache.txt"
printf 'PORT_MACOS_DISTRIBUTION:BOOL=ON\nPORT_SANITIZE:BOOL=on\nPORT_VERSION:STRING=b2_us\n' \
    >"$work/san-dist/CMakeCache.txt"
printf 'CMAKE_C_FLAGS:STRING=-g -fsanitize=address,undefined\nPORT_VERSION:STRING=b2_us\n' \
    >"$work/san-flags/CMakeCache.txt"
printf 'PORT_SANITIZE:BOOL=OFF\nPORT_VERSION:STRING=b2_us\n' >"$work/plain/CMakeCache.txt"
expect "a sanitized build: status" "$(options --build-dir "$work/san")" 2
expect "a sanitized build: message" \
    "$(grep -c "is a sanitized build (-DPORT_SANITIZE=ON): its app carries the sanitizer runtime" "$work/messages")" 1
expect "a sanitized distribution build: status" "$(options --distribution --build-dir "$work/san-dist")" 2
expect "a sanitized distribution build: message" \
    "$(grep -c "is a sanitized build (-DPORT_SANITIZE=ON)" "$work/messages")" 1
expect "a build sanitized by its flags: status" "$(options --build-dir "$work/san-flags")" 2
expect "a build sanitized by its flags: message" \
    "$(grep -c "is a sanitized build (-fsanitize= in CMAKE_C_FLAGS)" "$work/messages")" 1
expect "a build with PORT_SANITIZE off: status" "$(options --build-dir "$work/plain")" 2
expect "a build with PORT_SANITIZE off: past the check" \
    "$(grep -c -e "sanitized build" "$work/messages"),$(grep -c "doesn't name its macOS app" "$work/messages")" 0,1
expect "the refusals built nothing" "$(ls "$work/dev" "$work/dist" "$work/san" "$work/san-dist" "$work/san-flags" \
    "$work/plain" | grep -vc -e CMakeCache.txt -e '^$' -e ':$')" 0

echo "$checks checks, $failures failed"
[ "$failures" -eq 0 ]
