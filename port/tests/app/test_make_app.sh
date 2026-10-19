#!/bin/sh
# Tests of the macOS app's build scripts on small programs made here, in directories whose names have spaces: the app
# (port/app/macos/make_app.sh) of one that loads a library by each kind of install name dyld knows, with the version's
# icon (port/app/macos/make_icns.sh, from its artwork in port/assets/branding), and both scripts' refusals.
#
#   test_make_app.sh CC WORK_DIR REPO ICON_MASTER ICON_16 ICON_32
#
# The program prints bar() + qux() + abs1(): bar() is baz() * 6 and baz() is 7, so 42; qux() is 1 and abs1() is 100,
# so it prints 143 when every library loads. Its libraries:
#   - libbar, by @rpath, which the program's search path (an absolute directory) finds
#   - libbaz, by @rpath, which only libbar's own search path, @loader_path/../lib b, finds
#   - libqux, by @executable_path, next to the program
#   - libabs1, by its absolute path, as Homebrew names its libraries
# The app must hold all four and load them from itself alone: it is moved and the originals taken away before it runs
# again. Its icon must be the .icns given, with the ten images of a macOS icon set (the 16 and 32 pixel ones the small
# images drawn for their size), and its Info.plist must hold the names, the identifier and the About text as given.
#
# A universal program, arm64 and x86_64 for macOS 11.0 as the distribution build makes it (ADR 0015), with a universal
# library: the app of both slices, its LSMinimumSystemVersion --minimum's, the program started as each slice this Mac
# runs; check_app.sh on that app, which reads each slice's signature and compares versions part by part (11 is 11.0);
# and the refusals of --archs and --minimum: a program or a library without a slice the app is for, and a library
# built for a newer macOS than the app's.
set -u

cc=$1
work=$2
repo=$3
master=$4
small16=$5
small32=$6
script="$repo/port/app/macos/make_app.sh"
check_app="$repo/port/app/macos/check_app.sh"
make_icns="$repo/port/app/macos/make_icns.sh"
plist="$repo/port/app/macos/Info.plist.in"
checks=0
failures=0

expect() {
    checks=$((checks + 1))
    if [ "$2" != "$3" ]; then
        failures=$((failures + 1))
        printf 'FAIL %s: "%s", expected "%s"\n' "$1" "$2" "$3"
    fi
}

# Builds an app from the program $1 into $2 with the icon $3 (the version's by default) and the bundle identifier $4
# (a test one by default), and the options that follow (--archs, --minimum), and prints the script's exit status, its
# output in $work/output and its messages in $work/messages. The About text has the characters that XML and sed's
# replacements give a meaning to
about='Test & <About> | text \ with é; and more'
make_app() {
    program=$1
    bundle=$2
    icon=${3:-$icns}
    id=${4:-io.github.callancampos.frigate.test}
    shift 2
    [ $# -gt 0 ] && shift
    [ $# -gt 0 ] && shift
    "$script" --program "$program" --app "$bundle" --plist "$plist" --icon "$icon" --bundle-id "$id" \
        --name "Test Name" --display-name "Prog App" --about "$about" --repo "$repo" "$@" >"$work/output" \
        2>"$work/messages"
    echo $?
}

# The search paths (LC_RPATH) of a binary, one per line
search_paths() {
    otool -l "$1" | awk '
        $1 == "cmd" { rpath = ($2 == "LC_RPATH") }
        rpath && $1 == "path" { sub(/^ *path /, ""); sub(/ \(offset [0-9]+\)$/, ""); print }'
}

rm -rf "$work"
mkdir -p "$work/lib a" "$work/lib b" "$work/bin x" "$work/dup" "$work/out dir"
cd "$work" || exit 1
printf 'int baz(void) { return 7; }\n' >baz.c
printf 'int baz(void);\nint bar(void) { return baz() * 6; }\n' >bar.c
printf 'int qux(void) { return 1; }\n' >qux.c
printf 'int abs1(void) { return 100; }\n' >abs1.c
cat >prog.c <<'EOF'
#include <stdio.h>
int bar(void);
int qux(void);
int abs1(void);
int main(int argc, char **argv) {
    (void)argv;
    if (argc > 1) {
        return 0;
    }
    printf("%d\n", bar() + qux() + abs1());
    return 0;
}
EOF
"$cc" -dynamiclib -install_name @rpath/libbaz.dylib -o "lib b/libbaz.dylib" baz.c
"$cc" -dynamiclib -install_name @rpath/libbar.dylib -o "lib a/libbar.dylib" bar.c -L"lib b" -lbaz \
    "-Wl,-rpath,@loader_path/../lib b"
"$cc" -dynamiclib -install_name @executable_path/libqux.dylib -o "bin x/libqux.dylib" qux.c
"$cc" -dynamiclib -install_name "$work/lib a/libabs1.dylib" -o "lib a/libabs1.dylib" abs1.c
"$cc" -o "bin x/prog" prog.c -L"lib a" -L"bin x" -lbar -lqux -labs1 "-Wl,-rpath,$work/lib a"
expect "the program before the app" "$("$work/bin x/prog")" 143

# The icon: the version's master and its small images as an .icns, holding the ten images of a macOS icon set at their
# pixel sizes, the 16 and 32 pixel ones the small images drawn for their size, not the master scaled
icns="$work/icon dir/App Icon.icns"
"$make_icns" "$master" "$small16" "$small32" "$icns" 2>"$work/messages"
expect "make_icns.sh's status" $? 0
iconutil --convert iconset --output "$work/unpacked.iconset" "$icns"
sizes=$(for image in "$work/unpacked.iconset"/*.png; do
    printf '%s:%s\n' "$(basename "$image" .png)" \
        "$(sips -g pixelWidth "$image" | awk '$1 == "pixelWidth:" { print $2 }')"
done | LC_ALL=C sort | tr '\n' ' ')
expect "the icon's images" "$sizes" "icon_128x128:128 icon_128x128@2x:256 icon_16x16:16 icon_16x16@2x:32\
 icon_256x256:256 icon_256x256@2x:512 icon_32x32:32 icon_32x32@2x:64 icon_512x512:512 icon_512x512@2x:1024 "
# The opaque pixels of an image, each as its index and colour, summed up for comparing: sips writes the image as a BMP
# of 32-bit pixels (blue, green, red, alpha) from the offset its header gives at byte 10. Only the opaque ones, since
# iconutil gives back the 1x images' half-transparent edge pixels in other colours than it was given, which is
# iconutil's own round trip and not the image's
opaque_pixels() {
    sips -s format bmp "$1" --out "$work/pixels.bmp" >/dev/null
    offset=$(od -An -v -tu4 -j 10 -N 4 "$work/pixels.bmp" | tr -d ' ')
    od -An -v -tu1 -j "$offset" "$work/pixels.bmp" | awk '
        { for (i = 1; i <= NF; i++) b[n++] = $i }
        END { for (i = 0; i + 3 < n; i += 4) if (b[i + 3] == 255) printf "%d:%d,%d,%d\n", i / 4, b[i], b[i + 1], b[i + 2] }' |
        cksum
}
expect "the 16 px image is the one drawn for it" "$(opaque_pixels "$work/unpacked.iconset/icon_16x16.png")" \
    "$(opaque_pixels "$small16")"
expect "the 32 px image is the one drawn for it" "$(opaque_pixels "$work/unpacked.iconset/icon_32x32.png")" \
    "$(opaque_pixels "$small32")"
expect "the 16 point image at 2x is the 32 px one drawn" \
    "$(opaque_pixels "$work/unpacked.iconset/icon_16x16@2x.png")" "$(opaque_pixels "$small32")"
# and not the master scaled, which make_icns.sh used to put there
sips -z 16 16 "$master" --out "$work/scaled16.png" >/dev/null
expect "the 16 px image isn't the master scaled" \
    "$(test "$(opaque_pixels "$work/unpacked.iconset/icon_16x16.png")" = "$(opaque_pixels "$work/scaled16.png")" &&
        echo same || echo different)" different

# The app: its four libraries, its program's output, and only the app's own search paths
app="$work/out dir/Prog.app"
expect "make_app.sh's status" "$(make_app "$work/bin x/prog" "$app")" 0
expect "the app's libraries" "$(cd "$app/Contents/Frameworks" && ls | tr '\n' ' ')" \
    "libabs1.dylib libbar.dylib libbaz.dylib libqux.dylib "
expect "the program in the app" "$("$app/Contents/MacOS/prog")" 143
expect "the program's search paths" "$(search_paths "$app/Contents/MacOS/prog")" "@executable_path/../Frameworks"
expect "libbar's search paths" "$(search_paths "$app/Contents/Frameworks/libbar.dylib")" "@loader_path"
expect "libbaz's search paths" "$(search_paths "$app/Contents/Frameworks/libbaz.dylib")" ""
expect "nothing left beside the app" "$(ls "$work/out dir")" "Prog.app"
# Its Info.plist: the names and the identifier as given, the program's file as the executable
info() {
    plutil -extract "$1" raw -o - "$app/Contents/Info.plist"
}
expect "CFBundleName" "$(info CFBundleName)" "Test Name"
expect "CFBundleDisplayName" "$(info CFBundleDisplayName)" "Prog App"
expect "CFBundleIdentifier" "$(info CFBundleIdentifier)" io.github.callancampos.frigate.test
expect "CFBundleExecutable" "$(info CFBundleExecutable)" prog
expect "CFBundleIconFile" "$(info CFBundleIconFile)" AppIcon
expect "NSHumanReadableCopyright, the About window's text" "$(info NSHumanReadableCopyright)" "$about"
cmp -s "$icns" "$app/Contents/Resources/AppIcon.icns"
expect "the app's icon is the .icns given" $? 0
mv "$app" "$work/Moved.app"
mv "$work/lib a" "$work/lib a gone"
mv "$work/lib b" "$work/lib b gone"
mv "$work/bin x/libqux.dylib" "$work/libqux.gone"
expect "the moved app, without the original libraries" "$("$work/Moved.app/Contents/MacOS/prog" 2>&1)" 143
codesign --verify --deep --strict "$work/Moved.app" 2>/dev/null
expect "the moved app's signature" $? 0
mv "$work/lib a gone" "$work/lib a"
mv "$work/lib b gone" "$work/lib b"
mv "$work/libqux.gone" "$work/bin x/libqux.dylib"

# A library that no search path finds: no app
mv "$work/lib b/libbaz.dylib" "$work/libbaz.away"
expect "a missing library: status" "$(make_app "$work/bin x/prog" "$work/out dir/Missing.app")" 1
expect "a missing library: message" \
    "$(grep -c "libbar.dylib loads @rpath/libbaz.dylib, which isn't on this machine" "$work/messages")" 1
expect "a missing library: nothing made" "$(ls "$work/out dir")" ""
mv "$work/libbaz.away" "$work/lib b/libbaz.dylib"

# A library that only the program's search path finds, a path from @loader_path: dyld takes it from the program, not
# from the library that loads it, so the app is made from libone in "bin y/lib" loading libtwo there by @rpath
mkdir -p "$work/bin y/lib"
printf 'int two(void) { return 5; }\n' >two.c
printf 'int two(void);\nint one(void) { return two() * 2; }\n' >one.c
cat >prog3.c <<'EOF'
#include <stdio.h>
int one(void);
int main(int argc, char **argv) {
    (void)argv;
    if (argc > 1) {
        return 0;
    }
    printf("%d\n", one());
    return 0;
}
EOF
"$cc" -dynamiclib -install_name @rpath/libtwo.dylib -o "bin y/lib/libtwo.dylib" two.c
"$cc" -dynamiclib -install_name @rpath/libone.dylib -o "bin y/lib/libone.dylib" one.c -L"bin y/lib" -ltwo
"$cc" -o "bin y/prog3" prog3.c -L"bin y/lib" -lone "-Wl,-rpath,@loader_path/lib"
expect "the program's @loader_path search path, before the app" "$("$work/bin y/prog3")" 10
expect "the program's @loader_path search path: status" "$(make_app "$work/bin y/prog3" "$work/out dir/Three.app")" 0
expect "the program's @loader_path search path: the app" "$("$work/out dir/Three.app/Contents/MacOS/prog3")" 10
rm -rf "$work/out dir/Three.app"

# Two libraries with one file name: no app
printf 'int qux(void) { return 2; }\n' >qux2.c
printf 'int qux(void);\nint other(void) { return qux(); }\n' >other.c
"$cc" -dynamiclib -install_name "$work/dup/libqux.dylib" -o "$work/dup/libqux.dylib" qux2.c
"$cc" -dynamiclib -install_name "$work/dup/libother.dylib" -o "$work/dup/libother.dylib" other.c -L"$work/dup" -lqux
printf 'int bar(void);\nint qux(void);\nint other(void);\nint main(void) { return bar() + qux() + other(); }\n' \
    >prog2.c
"$cc" -o "bin x/prog2" prog2.c -L"lib a" -L"bin x" -L"$work/dup" -lbar -lqux -lother "-Wl,-rpath,$work/lib a"
expect "two libraries with one name: status" "$(make_app "$work/bin x/prog2" "$work/out dir/Dup.app")" 1
expect "two libraries with one name: message" "$(grep -c 'two libraries named libqux.dylib' "$work/messages")" 1
expect "two libraries with one name: nothing made" "$(ls "$work/out dir")" ""

# A program that doesn't start from the app (here, one whose --help fails): no app
printf 'int main(int argc, char **argv) { (void)argv; return argc > 1 ? 3 : 0; }\n' >bad.c
"$cc" -o "bin x/bad" bad.c
expect "a program that doesn't start: status" "$(make_app "$work/bin x/bad" "$work/out dir/Bad.app")" 1
expect "a program that doesn't start: message" \
    "$(grep -c "the program doesn't start from the app" "$work/messages")" 1
expect "a program that doesn't start: nothing made" "$(ls "$work/out dir")" ""

# A universal program and library, both slices for macOS 11.0: the app of both, for macOS 11.0, started as each slice
# this Mac runs (x86_64 needs Rosetta on Apple Silicon)
mkdir -p "$work/uni"
universal="-arch arm64 -arch x86_64 -mmacosx-version-min=11.0"
printf 'int uni(void) { return 11; }\n' >uni.c
printf '#include <stdio.h>\nint uni(void);\nint main(void) { printf("%%d\\n", uni()); return 0; }\n' >uniprog.c
# shellcheck disable=SC2086 # the flags are words
"$cc" $universal -dynamiclib -install_name @rpath/libuni.dylib -o "$work/uni/libuni.dylib" uni.c
# shellcheck disable=SC2086
"$cc" $universal -o "$work/uni/uniprog" uniprog.c -L"$work/uni" -luni "-Wl,-rpath,$work/uni"
app="$work/out dir/Uni.app"
expect "a universal program: status" \
    "$(make_app "$work/uni/uniprog" "$app" "" "" --archs "arm64 x86_64" --minimum 11.0)" 0
expect "a universal program: the program's slices" "$(lipo -archs "$app/Contents/MacOS/uniprog")" "x86_64 arm64"
expect "a universal program: the library's slices" "$(lipo -archs "$app/Contents/Frameworks/libuni.dylib")" \
    "x86_64 arm64"
expect "a universal program: LSMinimumSystemVersion" \
    "$(plutil -extract LSMinimumSystemVersion raw -o - "$app/Contents/Info.plist")" 11.0
started=$(sed -n 's/.*; started as \(.*\)$/\1/p' "$work/output")
for slice in arm64 x86_64; do
    if arch "-$slice" /usr/bin/true 2>/dev/null; then
        expect "a universal program: started as $slice" "$(printf '%s\n' "$started" | grep -c "$slice")" 1
        expect "a universal program: runs as $slice" "$(arch "-$slice" "$app/Contents/MacOS/uniprog")" 11
    fi
done
# check_app.sh on it: both slices, each signed ad hoc on its own, for macOS 11.0 however it is written, and not 12.0
"$check_app" "$app" --archs "arm64 x86_64" --minimum 11.0 >"$work/check" 2>&1
expect "check_app.sh on the universal app: status" $? 0
for slice in arm64 x86_64; do
    expect "check_app.sh: the $slice slice's own signature" \
        "$(grep -c "Uni.app ($slice): .*CodeDirectory .*flags=0x2(adhoc).* Signature=adhoc" "$work/check")" 1
done
"$check_app" "$app" --archs "arm64 x86_64" --minimum 11 >"$work/check" 2>&1
expect "check_app.sh with --minimum 11: status" $? 0
"$check_app" "$app" --archs "arm64 x86_64" --minimum 12.0 >"$work/check" 2>&1
expect "check_app.sh with --minimum 12.0: status" $? 1
expect "check_app.sh with --minimum 12.0: the slices and the Info.plist" "$(grep -c '^FAIL' "$work/check")" 3
rm -rf "$app"

# --minimum written 11, as CMake takes it: LSMinimumSystemVersion 11, which check_app.sh takes for 11.0
expect "--minimum 11: status" \
    "$(make_app "$work/uni/uniprog" "$app" "" "" --archs "arm64 x86_64" --minimum 11)" 0
expect "--minimum 11: LSMinimumSystemVersion" \
    "$(plutil -extract LSMinimumSystemVersion raw -o - "$app/Contents/Info.plist")" 11
"$check_app" "$app" --archs "arm64 x86_64" --minimum 11.0 >"$work/check" 2>&1
expect "--minimum 11: check_app.sh with --minimum 11.0" $? 0
rm -rf "$app"

# The same app with --archs in CMake's own spelling, a list with a semicolon
expect "--archs as a CMake list: status" \
    "$(make_app "$work/uni/uniprog" "$app" "" "" --archs "arm64;x86_64" --minimum 11.0)" 0
rm -rf "$app"

# Without --minimum, LSMinimumSystemVersion is the newest macOS a slice was built for: here a library's 13.0
# shellcheck disable=SC2086
"$cc" -arch arm64 -arch x86_64 -mmacosx-version-min=13.0 -dynamiclib -install_name @rpath/libuni.dylib \
    -o "$work/uni/libuni.dylib" uni.c
expect "a library for macOS 13: status without --minimum" "$(make_app "$work/uni/uniprog" "$app")" 0
expect "a library for macOS 13: LSMinimumSystemVersion without --minimum" \
    "$(plutil -extract LSMinimumSystemVersion raw -o - "$app/Contents/Info.plist")" 13.0
rm -rf "$app"
# and with --minimum 11.0, no app
expect "a library for macOS 13: status" \
    "$(make_app "$work/uni/uniprog" "$app" "" "" --archs "arm64 x86_64" --minimum 11.0)" 1
expect "a library for macOS 13: message" \
    "$(grep -c "libuni.dylib's [a-z0-9_]* slice is built for macOS 13.0, newer than the app's 11.0" "$work/messages")" 1
expect "a library for macOS 13: nothing made" "$(ls "$work/out dir")" ""

# A library with one slice where the app is for two: no app
"$cc" -arch arm64 -mmacosx-version-min=11.0 -dynamiclib -install_name @rpath/libuni.dylib -o "$work/uni/libuni.dylib" \
    uni.c
expect "a library without its x86_64 slice: status" \
    "$(make_app "$work/uni/uniprog" "$app" "" "" --archs "arm64 x86_64" --minimum 11.0)" 1
expect "a library without its x86_64 slice: message" \
    "$(grep -c "libuni.dylib has no x86_64 slice (it has arm64); the app is for arm64 x86_64" "$work/messages")" 1
expect "a library without its x86_64 slice: nothing made" "$(ls "$work/out dir")" ""

# A program with one slice, this Mac's, where the app is for two: no app
expect "a program with one slice: status" \
    "$(make_app "$work/bin x/prog" "$work/out dir/One.app" "" "" --archs "arm64 x86_64")" 1
expect "a program with one slice: message" "$(grep -c "prog has no [a-z0-9_]* slice" "$work/messages")" 1
expect "a program with one slice: nothing made" "$(ls "$work/out dir")" ""

# A --minimum that isn't a version: no app
expect "a --minimum that isn't a version: status" \
    "$(make_app "$work/bin x/prog" "$work/out dir/Bad.app" "" "" --minimum eleven)" 2
expect "a --minimum that isn't a version: message" "$(grep -c "takes a macOS version" "$work/messages")" 1

# A master that isn't 1024 x 1024, or a small image that isn't its size: no .icns
sips -z 512 512 "$master" --out "$work/small.png" >/dev/null
"$make_icns" "$work/small.png" "$small16" "$small32" "$work/small.icns" 2>"$work/messages"
expect "a 512 px master: status" $? 1
expect "a 512 px master: message" "$(grep -c 'must be a 1024 x 1024 PNG with alpha (it is 512 x 512' "$work/messages")" 1
expect "a 512 px master: nothing made" "$(ls "$work" | grep -c '^small.icns$')" 0
"$make_icns" "$master" "$small32" "$small32" "$work/small.icns" 2>"$work/messages"
expect "a 32 px image as the 16 px one: status" $? 1
expect "a 32 px image as the 16 px one: message" \
    "$(grep -c 'must be a 16 x 16 PNG with alpha (it is 32 x 32' "$work/messages")" 1
expect "a 32 px image as the 16 px one: nothing made" "$(ls "$work" | grep -c '^small.icns$')" 0

# A placeholder bundle identifier, as a build directory configured before the rename had: no app
expect "a com.example bundle identifier: status" \
    "$(make_app "$work/bin x/prog" "$work/out dir/Example.app" "" com.example.pokebw2-native.black2)" 2
expect "a com.example bundle identifier: message" "$(grep -c 'is a placeholder bundle identifier' "$work/messages")" 1
expect "a com.example bundle identifier: nothing made" "$(ls "$work/out dir")" ""

# An icon that isn't an .icns: no app
expect "a PNG as the icon: status" "$(make_app "$work/bin x/prog" "$work/out dir/Png.app" "$master")" 2
expect "a PNG as the icon: message" "$(grep -c "isn't an .icns file" "$work/messages")" 1
expect "a PNG as the icon: nothing made" "$(ls "$work/out dir")" ""

echo "$checks checks, $failures failed"
[ "$failures" -eq 0 ]
