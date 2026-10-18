#!/bin/sh
# Tests of the log of an app opened from the Finder (port/app/message_log.c). The program is copied into a stand-in app
# and run as LaunchServices runs an app's program, with stdout and stderr on /dev/null, under a home directory of the
# test's own; a ROM path that doesn't exist makes it stop at once, with status 66 (no usable ROM, port/app/main.c).
#
#   test_message_log.sh PROGRAM WORK_DIR
set -u

program=$1
work=$2
name=$(basename "$program")
app_program="$work/Test.app/Contents/MacOS/$name"
log="$work/home/Library/Logs/pokebw2-native/$name.log"
missing="$work/no such rom.nds"
checks=0
failures=0

expect() {
    checks=$((checks + 1))
    if [ "$2" != "$3" ]; then
        failures=$((failures + 1))
        printf 'FAIL %s: "%s", expected "%s"\n' "$1" "$2" "$3"
    fi
}

# Runs a copy of the program, $1, with the rest of the arguments, under the test's home, as headless as the options say
run() {
    copy=$1
    shift
    HOME="$work/home" "$copy" --save-dir "$work/saves" "$@"
}

rm -rf "$work"
mkdir -p "$work/Test.app/Contents/MacOS" "$work/plain" "$work/home"
cp "$program" "$app_program"
cp "$program" "$work/plain/$name"

# From the app, with nowhere to write: the log, made with its directories, has the reason and SDL's messages too
run "$app_program" --headless --rom "$missing" >/dev/null 2>&1
expect "from the app, stderr on /dev/null: status" $? 66
expect "the reason in the log" "$(grep -cF "port: rom: $missing: " "$log")" 1
expect "SDL's messages in the log" "$(grep -cF "port: entering the game's main()" "$log")" 1
# Each run empties it
run "$app_program" --headless --rom "$missing" >/dev/null 2>&1
expect "a second run: the reason once" "$(grep -cF "port: rom: $missing: " "$log")" 1
# Bad options and --help as well, which come before everything else
run "$app_program" --help >/dev/null 2>&1
expect "--help: status" $? 0
expect "--help: the usage in the log" "$(grep -c '^usage: ' "$log")" 1

# From the app, with stderr on a file: no log, the messages where stderr goes
rm -f "$log"
run "$app_program" --headless --rom "$missing" >/dev/null 2>"$work/stderr.txt"
expect "from the app, stderr on a file: status" $? 66
expect "from the app, stderr on a file: no log" "$(test -e "$log" && echo made || echo none)" none
expect "from the app, stderr on a file: the reason there" "$(grep -cF "port: rom: $missing: " "$work/stderr.txt")" 1

# Not from an app, with stderr on /dev/null: no log
run "$work/plain/$name" --headless --rom "$missing" >/dev/null 2>&1
expect "not from an app: status" $? 66
expect "not from an app: no log" "$(test -e "$log" && echo made || echo none)" none

echo "$checks checks, $failures failed"
[ "$failures" -eq 0 ]
