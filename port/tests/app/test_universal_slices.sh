#!/bin/sh
# The test app_universal_slices (port/cmake/macos_app.cmake): a universal program's slices run the game the same (the
# macOS distribution build's arm64 and x86_64, ADR 0013). Each slice runs (arch -ARCH) for FRAMES frames, 900 by
# default, which take the game from power-on through the logos and the opening's first scenes, headless, with
# --fixed-time and a save directory and run directory of its own, and the runs' frames.tsv (port/app/run_dir.c: a row a
# frame, with both screens' hashes) must be the same byte for byte: the x86_64 slice draws every frame the arm64 one
# draws, on the same frame. On Apple Silicon the x86_64 slice runs under Rosetta, which translates its code, so the run
# checks the x86-64 code the compiler made, not the CPU.
#
#   test_universal_slices.sh PROGRAM WORK_DIR [FRAMES]
#
# It exits 77 (skipped) for a program of one slice, a slice this Mac can't run (an x86_64 one on Apple Silicon without
# Rosetta, an arm64 one on an Intel Mac) and a missing ROM (the program's 66). What the slices are built for is the test
# app_slices' (port/app/macos/check_app.sh).
set -u

program=$1
work=$2
frames=${3:-900}

present=$(lipo -archs "$program")
if [ "$(printf '%s\n' $present | wc -l | tr -d ' ')" -lt 2 ]; then
    echo "skipped: the program has one slice ($present)"
    exit 77
fi
rm -rf "$work"
mkdir -p "$work"
for slice in $present; do
    if ! arch "-$slice" /usr/bin/true 2>/dev/null; then
        echo "skipped: this Mac can't run the program's $slice slice (an x86_64 slice on Apple Silicon needs Rosetta)"
        exit 77
    fi
    arch "-$slice" "$program" --headless --frames "$frames" --watchdog 120 --fixed-time "2026-10-18 12:00:00" \
        --save-dir "$work/saves-$slice" --run-dir "$work/run-$slice" >"$work/$slice.log" 2>&1
    status=$?
    if [ "$status" -eq 66 ]; then
        echo "skipped: no ROM (the program's status 66)"
        exit 77
    fi
    if [ "$status" -ne 0 ]; then
        tail -5 "$work/$slice.log"
        echo "FAIL: the $slice slice exited with status $status"
        exit 1
    fi
    # frames.tsv's two header lines, then a row a frame whose eighth column is the top screen's hash
    echo "$slice: $(awk 'NR > 2' "$work/run-$slice/frames.tsv" | wc -l | tr -d ' ') frames," \
        "$(awk -F '\t' 'NR > 2 { print $8 }' "$work/run-$slice/frames.tsv" | sort -u | wc -l | tr -d ' ')" \
        "distinct top screens"
done
first=$(printf '%s\n' $present | head -1)
for slice in $present; do
    if ! cmp -s "$work/run-$first/frames.tsv" "$work/run-$slice/frames.tsv"; then
        echo "FAIL: the $slice slice's frames differ from the $first slice's; the first difference:"
        diff "$work/run-$first/frames.tsv" "$work/run-$slice/frames.tsv" | head -4
        exit 1
    fi
done
echo "the slices' frames are the same: $(printf '%s\n' $present | paste -sd' ' -)"
