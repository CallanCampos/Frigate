#!/bin/sh
# Tests that port/tools/package_macos.sh refuses to write the disk image where git would take it into a repository: in
# this repository, in another one, through a link, and in one whose directories for the image don't exist yet. Each
# refusal comes before anything is built or written, so the test builds nothing.
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

echo "$checks checks, $failures failed"
[ "$failures" -eq 0 ]
