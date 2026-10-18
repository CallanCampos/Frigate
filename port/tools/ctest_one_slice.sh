#!/bin/sh
# Runs a universal macOS build's tests as one of its slices (ADR 0015), the x86_64 one on Apple Silicon under Rosetta.
# ctest runs each test program as the slice of the Mac it runs on, arm64 there, and the slices run the same game: so to
# run the whole suite, the oracle's comparisons with melonDS and the replays included, on the x86_64 code, this thins
# every universal executable of the build directory (the program, the test programs) to that slice for the run, and
# puts the universal ones back afterwards with their times, so that nothing relinks. On CI's Intel runners ctest runs
# the x86_64 slices natively, and this isn't needed.
#
#   port/tools/ctest_one_slice.sh --build-dir DIR --arch ARCH [--exclude REGEX] [-- CTEST_ARGUMENTS...]
#
#   --build-dir DIR   a configured and built universal build (build/port-dist)
#   --arch ARCH       the slice to run, x86_64 (or arm64); this Mac must run it: arch -ARCH /usr/bin/true
#   --exclude REGEX   more tests to leave out, a ctest regular expression
#
# The tests that check which slices the build links are left out, since a thinned tree fails them by design:
# app_slices (one slice where the build is for two) and overlay_sections_image_* (one slice where two were predicted);
# app_universal_slices skips (one slice). asm_oracle builds its own library, with the build's slices, and checks the
# one its Python runs, arm64 here. The app bundles in DIR are left alone (thinning would break their signature).
# A run that stops before the end (a ctest that is killed) leaves the universal executables in DIR/one-slice-originals;
# the next run puts them back first. The tests' saves and logs stay where ctest puts them, in DIR.
set -eu

build_dir=
slice=
exclude='^(app_slices|overlay_sections_image_.*)$'
while [ $# -gt 0 ]; do
    case "$1" in
    --build-dir)
        build_dir=$2
        shift 2
        ;;
    --arch)
        slice=$2
        shift 2
        ;;
    --exclude)
        exclude="$exclude|$2"
        shift 2
        ;;
    --)
        shift
        break
        ;;
    -h | --help)
        awk 'NR > 1 && /^#/ { sub(/^# ?/, ""); print; next } NR > 1 { exit }' "$0"
        exit 0
        ;;
    *)
        echo "ctest_one_slice.sh: unknown argument $1 (ctest's go after --)" >&2
        exit 2
        ;;
    esac
done
if [ -z "$build_dir" ] || [ -z "$slice" ]; then
    echo "ctest_one_slice.sh: give --build-dir and --arch (see the script's header)" >&2
    exit 2
fi
if [ ! -f "$build_dir/CTestTestfile.cmake" ]; then
    echo "ctest_one_slice.sh: $build_dir isn't a configured build with tests" >&2
    exit 2
fi
if ! arch "-$slice" /usr/bin/true 2>/dev/null; then
    echo "ctest_one_slice.sh: this Mac can't run $slice code (an x86_64 slice on Apple Silicon needs Rosetta)" >&2
    exit 2
fi
build_dir=$(cd "$build_dir" && pwd -P)
originals="$build_dir/one-slice-originals"
manifest="$originals/manifest"

# Puts every universal executable of the manifest back where it was, with its own time (mv keeps it), into its
# directory again if a test removed that (a fixture's); one that can't be put back stays in $originals, said so
restore() {
    [ -f "$manifest" ] || return 0
    left=0
    while IFS= read -r relative; do
        if [ -e "$originals/$relative" ] && ! { mkdir -p "$build_dir/$(dirname "$relative")" &&
            mv -f "$originals/$relative" "$build_dir/$relative"; }; then
            echo "ctest_one_slice.sh: couldn't put $relative back; it is in $originals" >&2
            left=$((left + 1))
        fi
    done <"$manifest"
    if [ "$left" -eq 0 ]; then
        rm -rf "$originals"
    fi
}
if [ -d "$originals" ]; then
    echo "ctest_one_slice.sh: putting back the universal executables an earlier run left in $originals"
    restore
fi

# The build's universal executables with an $slice slice: Mach-O programs (MH_EXECUTE) outside CMake's own files and
# the app bundles
candidates=$(find "$build_dir" \( -name CMakeFiles -o -name '*.app' -o -name one-slice-originals \) -prune -o \
    -type f -perm -u+x -print)
mkdir -p "$originals"
: >"$manifest"
trap 'restore' EXIT
trap 'exit 130' INT TERM
printf '%s\n' "$candidates" | while IFS= read -r file; do
    [ -n "$file" ] || continue
    archs=$(lipo -archs "$file" 2>/dev/null) || continue
    case " $archs " in
    *" $slice "*) ;;
    *) continue ;;
    esac
    case "$archs" in
    *" "*) ;;
    *) continue ;;
    esac
    otool -arch "$slice" -hv "$file" | grep -q ' EXECUTE ' || continue
    relative=${file#"$build_dir"/}
    mkdir -p "$originals/$(dirname "$relative")"
    mv "$file" "$originals/$relative"
    printf '%s\n' "$relative" >>"$manifest"
    lipo -thin "$slice" "$originals/$relative" -output "$file"
    touch -r "$originals/$relative" "$file"
done
count=$(wc -l <"$manifest" | tr -d ' ')
if [ "$count" -eq 0 ]; then
    echo "ctest_one_slice.sh: $build_dir has no universal executable with an $slice slice" >&2
    exit 2
fi
echo "ctest_one_slice.sh: $count executables thinned to $slice; leaving out $exclude"

status=0
ctest --test-dir "$build_dir" -E "$exclude" "$@" || status=$?
restore
trap - EXIT
echo "ctest_one_slice.sh: the $count universal executables are back"
exit "$status"
