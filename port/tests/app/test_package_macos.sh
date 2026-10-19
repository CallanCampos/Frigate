#!/bin/sh
# Tests that port/tools/package_macos.sh refuses to write the disk image where git would take it into a repository: in
# this repository, in another one, through a link, and in one whose directories for the image don't exist yet; and its
# refusals of the distribution build's options (ADR 0015): --distribution on a build directory configured without
# PORT_MACOS_DISTRIBUTION, or for another game than --version's, and --version without --distribution or with a game
# that isn't one; and a sanitized build, whose app carries the sanitizer runtime (port/docs/sanitizers.md), configured
# with PORT_SANITIZE, with or without --distribution, or with -fsanitize= in any of CMake's flags in its cache (the C
# compiler's, a configuration's, an executable's or a shared library's linker's), while one with PORT_SANITIZE off and
# no such flag goes on past that check. Each refusal comes before anything is built, configured or written, so those
# cases build nothing: their build directories are a CMakeCache.txt alone. Then the check of the image itself, which
# stands behind those refusals: a small CMake project of the test's own, configured with BUILD_DIR's generator and C
# compiler, makes a stand-in app, signed ad hoc, with a library named as the sanitizer's runtime is
# (libclang_rt.*_osx_dynamic.dylib) in its Frameworks, as make_app.sh copies the real one there, though its cache says
# nothing of a sanitizer. The script packages it, fails the image's check with status 1 and leaves no image; the same
# app without the library is packaged with status 0, and its image stays. Those two runs make real images (hdiutil).
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
# case, as CMake reads it, and -fsanitize= found in whichever of CMake's flags holds it. One with PORT_SANITIZE off goes
# on, to the check of its app's name that a cache of these alone fails, the cache's comments and its flags' -ADVANCED
# entries aside
mkdir -p "$work/san" "$work/san-dist" "$work/plain"
printf 'PORT_SANITIZE:BOOL=ON\nPORT_VERSION:STRING=b2_us\n' >"$work/san/CMakeCache.txt"
printf 'PORT_MACOS_DISTRIBUTION:BOOL=ON\nPORT_SANITIZE:BOOL=on\nPORT_VERSION:STRING=b2_us\n' \
    >"$work/san-dist/CMakeCache.txt"
printf '%s\n' '//Flags such as -fsanitize=address' 'CMAKE_C_FLAGS:STRING=-g' 'CMAKE_C_FLAGS-ADVANCED:INTERNAL=1' \
    'PORT_SANITIZE:BOOL=OFF' 'PORT_VERSION:STRING=b2_us' >"$work/plain/CMakeCache.txt"
expect "a sanitized build: status" "$(options --build-dir "$work/san")" 2
expect "a sanitized build: message" \
    "$(grep -c "is a sanitized build (-DPORT_SANITIZE=ON): its app carries the sanitizer runtime" "$work/messages")" 1
expect "a sanitized distribution build: status" "$(options --distribution --build-dir "$work/san-dist")" 2
expect "a sanitized distribution build: message" \
    "$(grep -c "is a sanitized build (-DPORT_SANITIZE=ON)" "$work/messages")" 1
for flags in CMAKE_C_FLAGS CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_EXE_LINKER_FLAGS CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO \
    CMAKE_SHARED_LINKER_FLAGS; do
    mkdir -p "$work/san-$flags"
    printf '%s:STRING=-O2 -g -DNDEBUG -fsanitize=address,undefined\nPORT_VERSION:STRING=b2_us\n' "$flags" \
        >"$work/san-$flags/CMakeCache.txt"
    expect "a build sanitized by $flags: status" "$(options --build-dir "$work/san-$flags")" 2
    expect "a build sanitized by $flags: message" \
        "$(grep -c "is a sanitized build (-fsanitize= in $flags)" "$work/messages")" 1
done
expect "a build with PORT_SANITIZE off: status" "$(options --build-dir "$work/plain")" 2
expect "a build with PORT_SANITIZE off: past the check" \
    "$(grep -c -e "sanitized build" "$work/messages"),$(grep -c "doesn't name its macOS app" "$work/messages")" 0,1
expect "the refusals built nothing" "$(ls "$work/dev" "$work/dist" "$work/san" "$work/san-dist" "$work"/san-CMAKE_* \
    "$work/plain" | grep -vc -e CMakeCache.txt -e '^$' -e ':$')" 0

# The image's own check: the stand-in app, built with BUILD_DIR's CMake, generator and C compiler, the CMake on PATH
# first, since the script runs the one there
cache_value() {
    sed -n "s/^$1:[A-Z]*=//p" "$build_dir/CMakeCache.txt"
}
cmake_command=$(cache_value CMAKE_COMMAND)
PATH="$(dirname "$cmake_command"):$PATH"
export PATH
mkdir -p "$work/stand-in" "$work/images"
cat >"$work/stand-in/CMakeLists.txt" <<'EOF'
# A stand-in for Frigate's build, for package_macos.sh's check of the image (port/tests/app/test_package_macos.sh): an
# app named as the build's cache names it, signed ad hoc, with STAND_IN_RUNTIME a library named as a sanitizer's
# runtime in its Frameworks
cmake_minimum_required(VERSION 3.20)
project(package_macos_stand_in C)
option(STAND_IN_RUNTIME "A library named as a sanitizer's runtime in the app" OFF)
add_executable(stand_in MACOSX_BUNDLE main.c)
set_target_properties(stand_in PROPERTIES OUTPUT_NAME "Stand In"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in")
if(STAND_IN_RUNTIME)
    add_library(stand_in_runtime SHARED runtime.c)
    set_target_properties(stand_in_runtime PROPERTIES OUTPUT_NAME clang_rt.stand_in_osx_dynamic)
    add_dependencies(stand_in stand_in_runtime)
    add_custom_command(TARGET stand_in POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:stand_in>/Frameworks"
        COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:stand_in_runtime>"
            "$<TARGET_BUNDLE_CONTENT_DIR:stand_in>/Frameworks/"
        COMMAND codesign --force --sign -
            "$<TARGET_BUNDLE_CONTENT_DIR:stand_in>/Frameworks/$<TARGET_FILE_NAME:stand_in_runtime>"
        VERBATIM)
endif()
add_custom_command(TARGET stand_in POST_BUILD COMMAND codesign --force --sign - "$<TARGET_BUNDLE_DIR:stand_in>"
    VERBATIM)
set(PORT_MACOS_APP_TARGET stand_in CACHE INTERNAL "The target that builds the macOS app")
set(PORT_MACOS_APP_NAME "Stand In" CACHE INTERNAL "The macOS app's name, without .app")
EOF
cat >"$work/stand-in/Info.plist.in" <<'EOF'
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleExecutable</key>
    <string>${MACOSX_BUNDLE_EXECUTABLE_NAME}</string>
    <key>CFBundleIdentifier</key>
    <string>io.github.callancampos.frigate.stand-in</string>
    <key>CFBundlePackageType</key>
    <string>APPL</string>
    <key>CFBundleShortVersionString</key>
    <string>0.0.0</string>
    <key>LSMinimumSystemVersion</key>
    <string>${CMAKE_OSX_DEPLOYMENT_TARGET}</string>
</dict>
</plist>
EOF
printf 'int main(void) {\n    return 0;\n}\n' >"$work/stand-in/main.c"
printf 'int stand_in_runtime(void) {\n    return 0;\n}\n' >"$work/stand-in/runtime.c"

# The script's status for the stand-in, with STAND_IN_RUNTIME $1, and the image $2, its messages in $work/messages; a
# stand-in that doesn't configure gives "no stand-in", with the end of its log on stderr
stand_in() {
    "$cmake_command" -S "$work/stand-in" -B "$work/stand-in-$1" -G "$(cache_value CMAKE_GENERATOR)" \
        -DCMAKE_MAKE_PROGRAM="$(cache_value CMAKE_MAKE_PROGRAM)" -DCMAKE_C_COMPILER="$(cache_value CMAKE_C_COMPILER)" \
        -DCMAKE_C_FLAGS= -DSTAND_IN_RUNTIME="$1" >"$work/configure-$1.log" 2>&1 || {
        tail -n 12 "$work/configure-$1.log" >&2
        echo "no stand-in"
        return
    }
    "$script" --build-dir "$work/stand-in-$1" --out "$2" >/dev/null 2>"$work/messages"
    echo $?
}
expect "an image with a sanitizer's runtime: status" "$(stand_in ON "$work/images/runtime.dmg")" 1
expect "an image with a sanitizer's runtime: message" "$(grep -c "the image holds a sanitizer's runtime" \
    "$work/messages"),$(grep -c "Contents/Frameworks/libclang_rt.stand_in_osx_dynamic.dylib" "$work/messages")" 1,1
expect "an image with a sanitizer's runtime: removed" \
    "$(grep -c "removed $work/images/runtime.dmg, which didn't pass its checks" "$work/messages")" 1
expect "an image with a sanitizer's runtime: no image" \
    "$(test -e "$work/images/runtime.dmg" && echo made || echo none)" none
expect "an image without one: status" "$(stand_in OFF "$work/images/plain.dmg")" 0
expect "an image without one: the image stays" "$(test -f "$work/images/plain.dmg" && echo made || echo none)" made
if [ "$failures" -ne 0 ]; then
    tail -n 12 "$work/messages"
fi

echo "$checks checks, $failures failed"
[ "$failures" -eq 0 ]
