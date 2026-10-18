#!/bin/sh
# Tests of the macOS app's build scripts on small programs made here, in directories whose names have spaces: the app
# (port/app/macos/make_app.sh) of one that loads a library by each kind of install name dyld knows, with the version's
# icon (port/app/macos/make_icns.sh, from its master in port/assets/branding), and both scripts' refusals.
#
#   test_make_app.sh CC WORK_DIR REPO ICON_MASTER
#
# The program prints bar() + qux() + abs1(): bar() is baz() * 6 and baz() is 7, so 42; qux() is 1 and abs1() is 100,
# so it prints 143 when every library loads. Its libraries:
#   - libbar, by @rpath, which the program's search path (an absolute directory) finds
#   - libbaz, by @rpath, which only libbar's own search path, @loader_path/../lib b, finds
#   - libqux, by @executable_path, next to the program
#   - libabs1, by its absolute path, as Homebrew names its libraries
# The app must hold all four and load them from itself alone: it is moved and the originals taken away before it runs
# again. Its icon must be the .icns given, with the ten images of a macOS icon set.
set -u

cc=$1
work=$2
repo=$3
master=$4
script="$repo/port/app/macos/make_app.sh"
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

# Builds an app from the program $1 into $2 with the icon $3 (the version's by default) and prints the script's exit
# status, its messages in $work/messages
make_app() {
    "$script" --program "$1" --app "$2" --plist "$plist" --icon "${3:-$icns}" \
        --bundle-id io.github.callancampos.frigate.test --name "Test Name" --display-name "Prog App" --repo "$repo" \
        >/dev/null 2>"$work/messages"
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

# The icon: the version's master as an .icns, holding the ten images of a macOS icon set at their pixel sizes
icns="$work/icon dir/App Icon.icns"
"$make_icns" "$master" "$icns" 2>"$work/messages"
expect "make_icns.sh's status" $? 0
iconutil --convert iconset --output "$work/unpacked.iconset" "$icns"
sizes=$(for image in "$work/unpacked.iconset"/*.png; do
    printf '%s:%s\n' "$(basename "$image" .png)" \
        "$(sips -g pixelWidth "$image" | awk '$1 == "pixelWidth:" { print $2 }')"
done | LC_ALL=C sort | tr '\n' ' ')
expect "the icon's images" "$sizes" "icon_128x128:128 icon_128x128@2x:256 icon_16x16:16 icon_16x16@2x:32\
 icon_256x256:256 icon_256x256@2x:512 icon_32x32:32 icon_32x32@2x:64 icon_512x512:512 icon_512x512@2x:1024 "

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

# A master that isn't 1024 x 1024: no .icns
sips -z 512 512 "$master" --out "$work/small.png" >/dev/null
"$make_icns" "$work/small.png" "$work/small.icns" 2>"$work/messages"
expect "a 512 px master: status" $? 1
expect "a 512 px master: message" "$(grep -c 'must be a 1024 x 1024 PNG with alpha (it is 512 x 512' "$work/messages")" 1
expect "a 512 px master: nothing made" "$(ls "$work" | grep -c '^small.icns$')" 0

# An icon that isn't an .icns: no app
expect "a PNG as the icon: status" "$(make_app "$work/bin x/prog" "$work/out dir/Png.app" "$master")" 2
expect "a PNG as the icon: message" "$(grep -c "isn't an .icns file" "$work/messages")" 1
expect "a PNG as the icon: nothing made" "$(ls "$work/out dir")" ""

echo "$checks checks, $failures failed"
[ "$failures" -eq 0 ]
