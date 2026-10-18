#!/bin/sh
# Tests of the macOS app's bundle identifier prefix (port/app/macos/bundle_id.cmake), through a project of nothing else
# (port/tests/app/bundle_id) configured in build directories of the test's own: a new build directory gets Frigate's
# prefix; one whose cache holds the placeholder from before the port was named Frigate, com.example.pokebw2-native, is
# moved to Frigate's once, with a message, and stays there; a prefix of the builder's own is kept.
#
#   test_bundle_id.sh CMAKE GENERATOR MAKE_PROGRAM PROJECT_DIR WORK_DIR
set -u

cmake=$1
generator=$2
make_program=$3
project=$4
work=$5
checks=0
failures=0

expect() {
    checks=$((checks + 1))
    if [ "$2" != "$3" ]; then
        failures=$((failures + 1))
        printf 'FAIL %s: "%s", expected "%s"\n' "$1" "$2" "$3"
    fi
}

# Configures the project into the build directory $1 with the rest of the arguments, its messages in $1.log
configure() {
    dir=$1
    shift
    "$cmake" -S "$project" -B "$dir" -G "$generator" "-DCMAKE_MAKE_PROGRAM=$make_program" "$@" >"$dir.log" 2>&1
}

# The prefix in a build directory's cache
prefix() {
    sed -n 's/^PORT_MACOS_BUNDLE_ID_PREFIX:STRING=//p' "$1/CMakeCache.txt"
}

rm -rf "$work"
mkdir -p "$work"

# A new build directory
configure "$work/new"
expect "a new build directory: status" $? 0
expect "a new build directory: Frigate's prefix" "$(prefix "$work/new")" io.github.callancampos.frigate
expect "a new build directory: no message" "$(grep -c 'had the placeholder' "$work/new.log")" 0

# A build directory whose cache holds the placeholder, as one configured before the rename does
configure "$work/old" -DPORT_MACOS_BUNDLE_ID_PREFIX=com.example.pokebw2-native
expect "the placeholder: status" $? 0
expect "the placeholder: moved to Frigate's" "$(prefix "$work/old")" io.github.callancampos.frigate
expect "the placeholder: the message" "$(grep -c 'had the placeholder com.example.pokebw2-native' "$work/old.log")" 1
configure "$work/old"
expect "the placeholder, configured again: status" $? 0
expect "the placeholder, configured again: Frigate's" "$(prefix "$work/old")" io.github.callancampos.frigate
expect "the placeholder, configured again: no message" "$(grep -c 'had the placeholder' "$work/old.log")" 0

# A prefix of the builder's own, given once and kept
configure "$work/own" -DPORT_MACOS_BUNDLE_ID_PREFIX=net.example-builder.frigate-fork
expect "a prefix of one's own: status" $? 0
expect "a prefix of one's own: kept" "$(prefix "$work/own")" net.example-builder.frigate-fork
configure "$work/own"
expect "a prefix of one's own, configured again: kept" "$(prefix "$work/own")" net.example-builder.frigate-fork

echo "$checks checks, $failures failed"
[ "$failures" -eq 0 ]
