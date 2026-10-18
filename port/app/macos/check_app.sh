#!/bin/sh
# Checks what a macOS build of Frigate is built for, as another Mac will meet it (ADR 0013): the program alone (the test
# app_slices, port/cmake/macos_app.cmake) or the app made from it ("Frigate Black 2.app", CI's macOS distribution job in
# .github/workflows/port-ci.yml, after port/tools/package_macos.sh --distribution). It prints what each binary's load
# commands say, slice by slice (LC_BUILD_VERSION's platform, minos and SDK), and fails when:
#   - a binary has no slice for an architecture of --archs, or the program has a slice of another one;
#   - a slice is for another platform than macOS, the program's for another macOS than --minimum (it is linked for
#     exactly that one), or a library's for a newer one;
#   - for an app: its LSMinimumSystemVersion isn't --minimum; a binary loads a library from outside macOS and the app;
#     its signature fails codesign's strict check, isn't ad hoc, or turns on the hardened runtime (ADR 0003);
#   - the program doesn't start (--help) as each of its slices this Mac runs, or, with --run-every-slice, as every one
#     of them (an x86_64 slice on Apple Silicon needs Rosetta; an arm64 one never runs on an Intel Mac).
#
#   check_app.sh PATH [--archs "ARCH ..."] [--minimum VERSION] [--run-every-slice]
#
# --archs takes CMAKE_OSX_ARCHITECTURES, blank, comma or semicolon separated; without it (or empty) the program's one
# slice must be this Mac's. --minimum takes CMAKE_OSX_DEPLOYMENT_TARGET; without it (or empty) the macOS isn't checked,
# as a development build is built for the SDK's. make_app.sh refuses the same faults before it signs; this checks the
# finished app, or the program before any app is made.
set -u

path=${1:-}
if [ -z "$path" ]; then
    echo "check_app.sh: give the app or the program to check (see the script's header)" >&2
    exit 2
fi
shift
archs=
minimum=
every=
while [ $# -gt 0 ]; do
    case "$1" in
    --archs)
        archs=$(printf '%s' "$2" | tr ',;' '  ')
        shift 2
        ;;
    --minimum)
        minimum=$2
        shift 2
        ;;
    --run-every-slice)
        every=yes
        shift
        ;;
    *)
        echo "check_app.sh: unknown argument $1" >&2
        exit 2
        ;;
    esac
done
if [ -z "$archs" ]; then
    archs=$(uname -m)
fi

failures=0
fail() {
    echo "FAIL: $*"
    failures=$((failures + 1))
}

# The program and the libraries an app holds, or the program alone
if [ -d "$path/Contents" ]; then
    plist="$path/Contents/Info.plist"
    program="$path/Contents/MacOS/$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$plist")"
    binaries=$program
    if [ -d "$path/Contents/Frameworks" ]; then
        for library in "$path/Contents/Frameworks"/*; do
            [ -f "$library" ] && binaries="$binaries
$library"
        done
    fi
else
    plist=
    program=$path
    binaries=$path
fi
if [ ! -f "$program" ]; then
    echo "check_app.sh: no program at $program" >&2
    exit 2
fi

# Whether version $1 is newer than $2, part by part
newer() {
    printf '%s %s\n' "$1" "$2" | awk '{
        n = split($1, a, "."); m = split($2, b, ".")
        for (i = 1; i <= 3; i++) { x = (i <= n) ? a[i] + 0 : 0; y = (i <= m) ? b[i] + 0 : 0
            if (x != y) exit !(x > y) }
        exit 1 }'
}

# One slice of a binary: "PLATFORM MINOS SDK" from LC_BUILD_VERSION (otool gives the platform as its number, 1 for
# macOS), or LC_VERSION_MIN_MACOSX's version and SDK for a binary built before it
slice_build() {
    otool -arch "$2" -l "$1" | awk '
        $1 == "cmd" { build = ($2 == "LC_BUILD_VERSION"); old = ($2 == "LC_VERSION_MIN_MACOSX") }
        build && $1 == "platform" { platform = ($2 == "1" || $2 == "MACOS") ? "MACOS" : "platform-" $2 }
        build && $1 == "minos" { minos = $2 }
        build && $1 == "sdk" { sdk = $2 }
        old && $1 == "version" { platform = "MACOS"; minos = $2 }
        old && $1 == "sdk" { sdk = $2 }
        END { printf "%s %s %s\n", platform ? platform : "none", minos ? minos : "none", sdk ? sdk : "none" }'
}

# Each binary's slices, which a subshell reads (a pipe's loop), so its findings are counted from its report
wanted=$(printf '%s\n' $archs | LC_ALL=C sort | paste -sd' ' -)
report=$(printf '%s\n' "$binaries" | while IFS= read -r binary; do
    present=$(lipo -archs "$binary" 2>/dev/null)
    if [ -z "$present" ]; then
        echo "FAIL: $binary isn't a Mach-O binary"
        continue
    fi
    for slice in $present; do
        set -- $(slice_build "$binary" "$slice")
        echo "$binary ($slice): platform $1, minos $2, sdk $3"
        if [ "$1" != MACOS ]; then
            echo "FAIL: $binary's $slice slice is for $1, not macOS"
        elif [ -n "$minimum" ] && [ "$binary" = "$program" ] && [ "$2" != "$minimum" ]; then
            echo "FAIL: the program's $slice slice is built for macOS $2, not $minimum"
        elif [ -n "$minimum" ] && newer "$2" "$minimum"; then
            echo "FAIL: $binary's $slice slice is built for macOS $2, newer than $minimum"
        fi
    done
    sorted=$(printf '%s\n' $present | LC_ALL=C sort | paste -sd' ' -)
    if [ "$binary" = "$program" ] && [ "$sorted" != "$wanted" ]; then
        echo "FAIL: the program's slices are $sorted, not $wanted"
    elif [ "$binary" != "$program" ]; then
        for slice in $archs; do
            printf '%s\n' $present | grep -qxF -e "$slice" || echo "FAIL: $binary has no $slice slice"
        done
    fi
done)
printf '%s\n' "$report"
failures=$((failures + $(printf '%s\n' "$report" | grep -c '^FAIL')))

# An app: its minimum macOS, the libraries its binaries load, its signature
if [ -n "$plist" ]; then
    app_minimum=$(/usr/libexec/PlistBuddy -c 'Print :LSMinimumSystemVersion' "$plist" 2>/dev/null)
    echo "$path: LSMinimumSystemVersion ${app_minimum:-none}"
    if [ -n "$minimum" ] && [ "$app_minimum" != "$minimum" ]; then
        fail "the app's LSMinimumSystemVersion is ${app_minimum:-none}, not $minimum"
    fi
    if [ -d "$path/Contents/Frameworks" ]; then
        echo "$path: Contents/Frameworks holds $(ls "$path/Contents/Frameworks" | paste -sd' ' -)"
    else
        echo "$path: no Contents/Frameworks: the program loads only macOS's own libraries"
    fi
    outside=$(printf '%s\n' "$binaries" | while IFS= read -r binary; do
        otool -arch all -L "$binary" | sed -n 's/^[[:space:]]\{1,\}\(.*\) (compatibility version.*$/\1/p'
    done | sort -u | while IFS= read -r library; do
        # Each pattern opens with a parenthesis, which bash 3.2, macOS's sh, needs for a case inside $(...)
        case "$library" in
        (/usr/lib/* | /System/*) ;;
        (@rpath/*) [ -f "$path/Contents/Frameworks/${library#@rpath/}" ] || echo "$library" ;;
        (*) echo "$library" ;;
        esac
    done)
    if [ -n "$outside" ]; then
        fail "the app loads libraries from outside macOS and itself: $(printf '%s\n' "$outside" | paste -sd' ' -)"
    fi
    if ! codesign --verify --deep --strict "$path" 2>/dev/null; then
        fail "the app's signature fails codesign --verify --deep --strict"
    fi
    signature=$(codesign -dv "$path" 2>&1)
    echo "$path: $(printf '%s\n' "$signature" | grep -E '^(Signature|CodeDirectory)' | paste -sd' ' -)"
    printf '%s\n' "$signature" | grep -qx 'Signature=adhoc' || fail "the app isn't signed ad hoc"
    if printf '%s\n' "$signature" | grep '^CodeDirectory' | grep -q 'runtime'; then
        fail "the app's signature turns on the hardened runtime, which ADR 0003 leaves off"
    fi
fi

# The program starts as each slice (--help returns before anything opens). Its output is kept, not sent to /dev/null:
# a program in an app whose stderr goes nowhere writes its messages to ~/Library/Logs/Frigate (port/app/message_log.c),
# which a check must leave alone
for slice in $(lipo -archs "$program"); do
    if ! arch "-$slice" /usr/bin/true 2>/dev/null; then
        if [ -n "$every" ]; then
            fail "this Mac can't run the program's $slice slice (an x86_64 slice on Apple Silicon needs Rosetta)"
        else
            echo "$program ($slice): not started, since this Mac can't run it"
        fi
        continue
    fi
    if output=$(arch "-$slice" "$program" --help 2>&1); then
        echo "$program ($slice): starts (--help)"
    else
        printf '%s\n' "$output" | tail -5
        fail "the program doesn't start as its $slice slice (--help)"
    fi
done

if [ "$failures" -ne 0 ]; then
    echo "check_app.sh: $failures failed"
    exit 1
fi
echo "check_app.sh: $path is built for $wanted${minimum:+, macOS $minimum and later}"
