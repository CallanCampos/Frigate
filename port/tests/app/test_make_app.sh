#!/bin/sh
# Tests of the macOS app's build script (port/app/macos/make_app.sh) on small programs made here, in directories whose
# names have spaces: one that loads a library by each kind of install name dyld knows, and the script's refusals.
#
#   test_make_app.sh CC WORK_DIR REPO
#
# The program prints bar() + qux() + abs1(): bar() is baz() * 6 and baz() is 7, so 42; qux() is 1 and abs1() is 100,
# so it prints 143 when every library loads. Its libraries:
#   - libbar, by @rpath, which the program's search path (an absolute directory) finds
#   - libbaz, by @rpath, which only libbar's own search path, @loader_path/../lib b, finds
#   - libqux, by @executable_path, next to the program
#   - libabs1, by its absolute path, as Homebrew names its libraries
# The app must hold all four and load them from itself alone: it is moved and the originals taken away before it runs
# again.
set -u

cc=$1
work=$2
repo=$3
script="$repo/port/app/macos/make_app.sh"
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

# Builds an app from the program $1 into $2 and prints the script's exit status, its messages in $work/messages
make_app() {
    "$script" --program "$1" --app "$2" --plist "$plist" --bundle-id com.example.test.prog --display-name Prog \
        --repo "$repo" >/dev/null 2>"$work/messages"
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

echo "$checks checks, $failures failed"
[ "$failures" -eq 0 ]
