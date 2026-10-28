# Frigate's host build. The ROM build (configure.py and ninja) stays separate and untouched; this one compiles the same
# decomp sources with the host's clang into one program, frigate-black2 or frigate-white2, with the platform layer in
# port/: the whole game, from the decomp's src/ and lib/, on port/hal, port/gfx, port/audio and port/app
# (port/docs/ARCHITECTURE.md).
#
#   cmake -S port -B build/port -G Ninja
#   cmake --build build/port
#   ctest --test-dir build/port
#
# Options: -DPORT_VERSION=b2_us|w2_us (default b2_us), -DPORT_ROM_LANGUAGE=us|es (default us), -DPORT_FETCH_SDL3,
# -DPORT_MACOS_DISTRIBUTION, and -DPORT_SANITIZE with -DPORT_SANITIZE_RECOVER (the sanitized build) here; the others
# are in the fragments of port/cmake/, included at the end, and port/cmake/README.md lists them all. Each fragment
# labels its tests where it defines them (port_test_labels).
cmake_minimum_required(VERSION 3.28)

# The macOS build for other Macs than the one that builds it (ADR 0015): one universal program, arm64 and x86_64, for
# macOS 11.0 and later, with the pinned SDL3 built along for the same target and linked statically, never an installed
# one (Homebrew's is built for the macOS Homebrew builds on, and for one architecture). The deployment target and the
# architectures must be set before project(), which checks the compiler with them; a value given on the command line,
# or already in the cache, stays. A development build sets neither, so it is built for the machine it is built on. One
# build directory per kind: build/port-dist is the one port/tools/package_macos.sh --distribution configures. A
# directory once configured with the option keeps its architectures, its deployment target and its fetched SDL3 in the
# cache, so turning the option off there stops the configure instead of building a development build for 11.0 that
# would link an installed SDL3 of one architecture
option(PORT_MACOS_DISTRIBUTION
    "macOS: a universal (arm64 and x86_64) build for macOS 11.0 and later, with the pinned SDL3 linked statically" OFF)
if(PORT_MACOS_DISTRIBUTION)
    if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "The oldest macOS the program runs on" FORCE)
    endif()
    if(NOT CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "The architectures of the program's slices" FORCE)
    endif()
    set(PORT_MACOS_DISTRIBUTION_DIRECTORY ON CACHE INTERNAL "This directory was configured as the distribution build")
elseif(PORT_MACOS_DISTRIBUTION_DIRECTORY)
    message(FATAL_ERROR "${CMAKE_BINARY_DIR} was configured as the macOS distribution build "
        "(-DPORT_MACOS_DISTRIBUTION=ON), whose architectures, deployment target and SDL3 stay in its cache; configure "
        "the development build in a directory of its own (cmake -S port -B build/port), or keep the option on here")
endif()

# The product's name, description and disclaimer, which port/hal/os/port_app.h sets for the C (port/CLAUDE.md, "The
# port is Frigate"): the build reads them from there, so that each is written once. Sets the variable of the macro's
# name to its string, the literals that follow `#define NAME` joined as C joins them
function(port_product_string name)
    set(header "${CMAKE_CURRENT_SOURCE_DIR}/hal/os/port_app.h")
    # A literal, and what may come between two: blanks, and a line's end after a backslash
    set(literal "\"[^\"\n]*\"")
    set(between "[ \t]*(\\\\\n[ \t]*)?")
    file(READ "${header}" text ENCODING UTF-8)
    string(REGEX MATCH "#define ${name}[ \t]+(\\\\\n[ \t]*)?${literal}(${between}${literal})*" value "${text}")
    if(value STREQUAL "")
        message(FATAL_ERROR "${header} doesn't define ${name} as a string")
    endif()
    # Without the #define and the outer quotes, and with the quotes and blanks between the literals taken out (no list
    # operations, since the text may hold semicolons)
    string(REGEX REPLACE "^#define ${name}[ \t]+(\\\\\n[ \t]*)?\"" "" value "${value}")
    string(REGEX REPLACE "\"${between}\"" "" value "${value}")
    string(REGEX REPLACE "\"$" "" value "${value}")
    if(value MATCHES "\\\\")
        message(FATAL_ERROR "${name} in ${header} has an escape, which the build doesn't read; write it plainly")
    endif()
    set(${name} "${value}" PARENT_SCOPE)
endfunction()
port_product_string(PORT_PRODUCT_NAME)
port_product_string(PORT_PRODUCT_DESCRIPTION)
port_product_string(PORT_PRODUCT_DISCLAIMER)

project(frigate
    DESCRIPTION "${PORT_PRODUCT_DESCRIPTION}"
    HOMEPAGE_URL "https://github.com/CallanCampos/Frigate"
    LANGUAGES C)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/hal/os/port_app.h")
# compile_commands.json in the build directory: the pointer cast guard (port/tools/check_ptrcasts.py, the test
# game_ptrcasts of port/cmake/game.cmake) runs the game's compile commands again from it, and editors read it too
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(PORT_MACOS_DISTRIBUTION AND NOT APPLE)
    message(FATAL_ERROR "PORT_MACOS_DISTRIBUTION is the macOS build for other Macs; configure without it here")
endif()

# The names the player sees. The product is Frigate, PORT_PRODUCT_NAME above; the macOS app's names are made from it in
# port/cmake/macos_app.cmake. Each version is its own program: frigate-black2 or frigate-white2, built from the same
# sources with the decomp's BLACK2 or WHITE2 defined, and PORT_GAME_NAME is the game it runs, as the window's title and
# the ROM's messages name it. PORT_APP_ICON is the version's icon, Frigate's ship on Black 2's dark plate or White 2's
# light one (port/assets/branding), as its 1024 px master, and PORT_APP_ICON_16 and _32 its small images, drawn pixel by
# pixel for their size: the macOS app's icon is made from the three (port/cmake/macos_app.cmake). PORT_WINDOW_ICON is
# the window's 32 px icon on Linux and Windows, whose plate fills its square, made with the master at twice its size
# (port/cmake/app.cmake)
set(PORT_VERSION "b2_us" CACHE STRING "The game version to build: b2_us (Black 2) or w2_us (White 2)")
set_property(CACHE PORT_VERSION PROPERTY STRINGS b2_us w2_us)
if(PORT_VERSION STREQUAL "b2_us")
    set(PORT_VERSION_DEFINE BLACK2)
    set(PORT_PROGRAM frigate-black2)
    set(PORT_GAME_NAME "Pokémon Black 2")
    set(PORT_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/branding/icon-black2-1024.png")
    set(PORT_APP_ICON_16 "${CMAKE_CURRENT_SOURCE_DIR}/assets/branding/icon-black2-16.png")
    set(PORT_APP_ICON_32 "${CMAKE_CURRENT_SOURCE_DIR}/assets/branding/icon-black2-32.png")
    set(PORT_WINDOW_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/branding/window-black2-32.png")
elseif(PORT_VERSION STREQUAL "w2_us")
    set(PORT_VERSION_DEFINE WHITE2)
    set(PORT_PROGRAM frigate-white2)
    set(PORT_GAME_NAME "Pokémon White 2")
    set(PORT_APP_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/branding/icon-white2-1024.png")
    set(PORT_APP_ICON_16 "${CMAKE_CURRENT_SOURCE_DIR}/assets/branding/icon-white2-16.png")
    set(PORT_APP_ICON_32 "${CMAKE_CURRENT_SOURCE_DIR}/assets/branding/icon-white2-32.png")
    set(PORT_WINDOW_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/branding/window-white2-32.png")
else()
    message(FATAL_ERROR "PORT_VERSION must be b2_us or w2_us, not ${PORT_VERSION}")
endif()

# The ROM data the program plays: us, the dump the decomp builds, or experimental Spanish Black 2
# (IRES only), whose files the US code reads instead, by the manifest that
# port/tools/research/assets/rom_manifest.py writes from that dump (port/cmake/rom.cmake). The game's code stays the
# decomp's US C, with English game logic where it has any (port/docs/research/es-rom-support.md, section 4)
set(PORT_ROM_LANGUAGE "us" CACHE STRING "The language of the ROM to play: us (the decomp's) or es (Spanish)")
set_property(CACHE PORT_ROM_LANGUAGE PROPERTY STRINGS us es)
if(PORT_ROM_LANGUAGE STREQUAL "us")
    set(PORT_ROM_REGION "US")
elseif(PORT_ROM_LANGUAGE STREQUAL "es")
    if(NOT PORT_VERSION STREQUAL "b2_us")
        message(FATAL_ERROR "PORT_ROM_LANGUAGE=es is experimental and supports only Black 2 (b2_us/IRES)")
    endif()
    message(WARNING "Experimental IRES data loader: Spanish gameplay/save compatibility is not established")
    set(PORT_ROM_REGION "Spain, experimental")
else()
    message(FATAL_ERROR "PORT_ROM_LANGUAGE must be us or es, not ${PORT_ROM_LANGUAGE}")
endif()

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(PORT_GEN "${CMAKE_CURRENT_BINARY_DIR}/gen")

# The repository's virtual environment when there is one (it has the decomp's Python packages), else any Python 3: on
# Windows the one on PATH first, the CLANG64 shell's own (port/README.md), before any the registry names
if(EXISTS "${REPO_ROOT}/.venv/bin/python")
    set(PORT_PYTHON "${REPO_ROOT}/.venv/bin/python")
else()
    set(Python3_FIND_REGISTRY LAST)
    find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
    set(PORT_PYTHON "${Python3_EXECUTABLE}")
endif()
# The decomp's headers and data files are UTF-8, which Python on Windows reads in the ANSI code page unless its UTF-8
# mode is on: on for the generators that configuring runs. The build's and the tests' scripts read only ASCII, or the
# environment's PYTHONUTF8 (port/README.md, Windows)
if(WIN32)
    set(ENV{PYTHONUTF8} 1)
endif()

# Generated inputs: the constants headers, the include/ mirror without MSL's C library headers, and the source list.
# Configuring again picks up new upstream files; CMake does that by itself when the inputs below change.
execute_process(
    COMMAND "${PORT_PYTHON}" "${REPO_ROOT}/port/tools/gen_host_tree.py" --out "${PORT_GEN}"
    WORKING_DIRECTORY "${REPO_ROOT}"
    RESULT_VARIABLE gen_result)
if(NOT gen_result EQUAL 0)
    message(FATAL_ERROR "port/tools/gen_host_tree.py failed")
endif()
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
    "${REPO_ROOT}/port/hal/replaced.txt" "${REPO_ROOT}/port/tools/gen_host_tree.py"
    "${REPO_ROOT}/port/include/wchar.h")
file(GLOB_RECURSE port_watched CONFIGURE_DEPENDS
    "${REPO_ROOT}/src/*.c" "${REPO_ROOT}/lib/*.c" "${REPO_ROOT}/lib/*.h" "${REPO_ROOT}/include/*")
# Where the include/ mirror is copies (Windows without the right to make links), a header's change reaches the build
# only through a configure, which copies it again: every header is then a configure dependency
file(STRINGS "${PORT_GEN}/include_mirror.txt" port_include_mirror)
if(port_include_mirror STREQUAL "copies")
    file(GLOB_RECURSE port_include_headers "${REPO_ROOT}/include/*")
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${port_include_headers})
endif()
file(STRINGS "${PORT_GEN}/sources.txt" game_sources_relative)
list(TRANSFORM game_sources_relative PREPEND "${REPO_ROOT}/" OUTPUT_VARIABLE game_sources)

# SDL3 3.4: an installed one when there is one (Homebrew, a distribution), else the pinned release built along. The
# macOS distribution build always builds the pinned release, statically, for its own deployment target and
# architectures, which CMAKE_OSX_DEPLOYMENT_TARGET and CMAKE_OSX_ARCHITECTURES carry into SDL3's build
option(PORT_FETCH_SDL3 "Build the pinned SDL3 release when no installed SDL3 3.4 is found" ON)
if(PORT_MACOS_DISTRIBUTION)
    if(NOT PORT_FETCH_SDL3)
        message(FATAL_ERROR
            "PORT_MACOS_DISTRIBUTION builds the pinned SDL3 release; configure with -DPORT_FETCH_SDL3=ON")
    endif()
else()
    find_package(SDL3 3.4 CONFIG QUIET)
endif()
if(NOT SDL3_FOUND)
    if(NOT PORT_FETCH_SDL3)
        message(FATAL_ERROR "SDL3 3.4 not found; install it, or configure with -DPORT_FETCH_SDL3=ON")
    endif()
    include(FetchContent)
    FetchContent_Declare(SDL3
        URL "https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.tar.gz"
        URL_HASH SHA256=9c75cf16330322c217dedd2e0609f1124f1b54b8633e763467b4684d0f4334a3)
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC ON CACHE BOOL "" FORCE)
    set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
    if(PORT_MACOS_DISTRIBUTION)
        # Without libusb, which SDL3 otherwise opens at run time (dlopen) when the builder's Mac has Homebrew's: an app
        # for other Macs loads nothing from outside macOS (ADR 0010), and its SDL3 doesn't depend on the builder's Mac.
        # SDL3 reads gamepads through IOKit and GameController either way
        set(SDL_HIDAPI_LIBUSB OFF CACHE BOOL "" FORCE)
    endif()
    FetchContent_MakeAvailable(SDL3)
endif()

# A macOS build for an older macOS than the SDK's (a deployment target, such as the distribution build's 11.0): a call
# to an API newer than the target, which would stop the program on the older macOS (or, weakly imported, call a null
# pointer there), fails the build instead. Every target defined from here on gets it, the game's, the platform layer's,
# the program's and the tests'; and so does SDL3 when it is built along, the only code of the program that calls
# AppKit, Metal, GameController and CoreHaptics: it guards its newer calls itself, and the check holds every pinned
# release to that, C and Objective-C (ADR 0015). An installed SDL3 is a development build's, built for its own macOS
if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    add_compile_options(-Werror=unguarded-availability)
    if(TARGET SDL3-static)
        target_compile_options(SDL3-static PRIVATE -Werror=unguarded-availability)
    endif()
endif()

# The sanitized build (port/docs/sanitizers.md): every target from here on, the game, the platform layer, the program
# and the tests, under AddressSanitizer and UndefinedBehaviorSanitizer, with every report fatal, so that a test that
# reaches one fails (a report aborts the program, port/hal/os/sanitizers.c, and the tests' output is checked for one,
# at the end of this file). SDL3, above, stays as it was built, and so does the oracle, a project of its own
# (port/cmake/oracle.cmake, PORT_ORACLE_SANITIZE). The ignorelist leaves out the checks of the few places where the
# game is right on every host and only C's rules call it undefined, each said there; port/hal/os/sanitizers.c and
# port/hal/os/fiber.c tell the runtime about the fibers, and port/hal/nnsys/fnd_hooks.c about the game's heaps. Use a
# build directory of its own (build/port-san). Apple clang emits no function check for C; upstream clang does.
#
# PORT_SANITIZE_RECOVER, with PORT_SANITIZE, instead lets a run go on after each report, to survey every report of a
# route in one run
option(PORT_SANITIZE "Build everything under AddressSanitizer and UndefinedBehaviorSanitizer, every report fatal" OFF)
option(PORT_SANITIZE_RECOVER "With PORT_SANITIZE, go on after a report instead of stopping, to survey them all" OFF)
if(PORT_SANITIZE_RECOVER AND NOT PORT_SANITIZE)
    message(FATAL_ERROR "PORT_SANITIZE_RECOVER makes the sanitized build's checks recover: configure it with "
        "-DPORT_SANITIZE=ON")
endif()
if(PORT_SANITIZE)
    if(NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "PORT_SANITIZE needs clang, for its ignorelist and runtime, not ${CMAKE_C_COMPILER_ID}")
    endif()
    set(PORT_SANITIZE_IGNORELIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sanitizer_ignorelist.txt")
    if(PORT_SANITIZE_RECOVER)
        # port/hal/os/sanitizers.c sets ASan's halt_on_error=0 then, which needs the recovering checks
        set(port_sanitize_recover -fsanitize-recover=all)
        add_compile_definitions(PORT_SANITIZE_RECOVER)
    else()
        set(port_sanitize_recover -fno-sanitize-recover=all)
    endif()
    add_compile_options(-fsanitize=address,undefined ${port_sanitize_recover} -fno-omit-frame-pointer
        "-fsanitize-ignorelist=${PORT_SANITIZE_IGNORELIST}")
    # clang names the ignorelist in each object's dependencies, so Ninja compiles again when it changes
    add_link_options(-fsanitize=address,undefined)
endif()

# What every file that includes the decomp's headers needs, the decomp's own C and the platform layer alike: MWCC's
# language model (signed char, int-sized enums, 16-bit wchar_t, wrapping signed arithmetic, no strict aliasing since
# the code puns types, no float contraction, and zeroed locals, since uninitialized reads gave deterministic values on
# the DS; PLAN.md §4.4), the version, and the include path with the host variants of the headers that name DS
# addresses first (ADR 0004). The decomp's headers are system headers here, so their warnings stay out of port code.
# With 16-bit wchar_t, clang would turn a loop that counts a u16 string (NitroSDK's mystrlen in mb_gameinfo.c, GameSpy's
# gsUnicodeStringLen in gsXML.c) into a call of the C library's wcslen, whose wchar_t is 32-bit on macOS and Linux:
# -fno-builtin-wcslen keeps the loop, on every host (port/tools/gen_stubs.py warns of any such call)
add_library(port_decomp_env INTERFACE)
target_compile_definitions(port_decomp_env INTERFACE PLATFORM_PC ${PORT_VERSION_DEFINE})
file(GLOB library_includes LIST_DIRECTORIES true "${REPO_ROOT}/lib/*/include")
target_include_directories(port_decomp_env SYSTEM INTERFACE
    "${PORT_GEN}/hwinclude" "${PORT_GEN}/include" "${CMAKE_CURRENT_SOURCE_DIR}/include"
    ${library_includes} "${REPO_ROOT}/build/include/generated")
target_compile_options(port_decomp_env INTERFACE
    -fsigned-char -fshort-wchar -fno-builtin-wcslen -fwrapv -fno-strict-aliasing -ffp-contract=off
    -ftrivial-auto-var-init=zero -fno-common
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/port_host.h"
    # The decomp declares C library functions itself (DWC's ext.h), which the macOS headers' fortify macros rename
    -D_FORTIFY_SOURCE=0
    # Errors that are never a porting artefact. MWCC accepts dropping const or volatile through a pointer, and the
    # decomp does it where the original did, so that part of the group stays a warning
    -Werror=implicit-function-declaration -Werror=int-conversion -Werror=incompatible-pointer-types
    -Wno-error=incompatible-pointer-types-discards-qualifiers)
# Windows: MinGW's clang lays out structs as MSVC does unless told otherwise (-mms-bitfields, its default there), and
# that layout ignores the lower alignment PORT_DS_ALIGN_64 gives the 64-bit typedefs (include/types.h): a u64 after a
# u32 goes to offset 8, not the DS's 4, so a save block holding one moves its fields and its footer (the day care's egg
# seed, block 0x32). Every file that includes the decomp's headers takes GCC's layout instead, as on macOS and Linux,
# and port/include/port_host.h stops a build without it. The Windows structs the platform layer uses (CONTEXT, the PE
# image's headers, WIN32_FIND_DATAA, SRWLOCK) have no bitfields, so they come out the same in both layouts
if(WIN32)
    target_compile_options(port_decomp_env INTERFACE -mno-ms-bitfields)
endif()
# The decomp's C calls the C library's math (sqrt, which clang turns into sqrtf where its argument and result are
# float), which glibc keeps in libm. The pinned SDL3 brings it on its link line, a distribution's shared SDL3 doesn't,
# so on Linux it is linked by name; macOS's and MSYS2's C libraries have it already
if(UNIX AND NOT APPLE)
    set(PORT_LIBM m)
endif()
target_link_libraries(port_decomp_env INTERFACE ${PORT_LIBM})
# The decomp defines functions under the C library's names, GameSpy's socket, connect, send and recv for the DS
# (lib/dwc/src/gscommon/gsPlatformSocket.c). On Linux an executable exports a definition that a shared library refers
# to, and that definition wins over the C library's: a distribution's shared SDL3 then reached Wayland and X11 through
# GameSpy's socket and found no display ("No available video device"). Hidden, the game's symbols stay inside the
# program and its calls are unchanged. macOS's two-level namespace and Windows' imports never bind across like this
if(UNIX AND NOT APPLE)
    target_compile_options(port_decomp_env INTERFACE -fvisibility=hidden)
endif()

add_library(pokebw2_game OBJECT ${game_sources})
target_link_libraries(pokebw2_game PRIVATE port_decomp_env)
target_compile_options(pokebw2_game PRIVATE
    -std=gnu99
    # MWCC's inline: a function defined `inline` without `static` (ov194's BackupMail) is also an external definition,
    # which C99 would leave to another file, so an unoptimized build that doesn't inline the calls fails to link
    -fgnu89-inline
    # See port_decomp_env: the decomp drops const and volatile where the original did
    -Wno-incompatible-pointer-types-discards-qualifiers
    # The 64-bit warnings: the pointer cast guard turns them on again and holds them to its baseline (the test
    # game_ptrcasts, port/cmake/game.cmake); here they only make noise
    -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-void-pointer-to-int-cast
    -Wno-int-to-void-pointer-cast -Wno-unknown-pragmas)
# MWCC places middleware signature strings in a custom ROM section; host code refers to the same constants
# normally. These files contain only the signatures, so discard only their MWCC section spelling.
set_source_files_properties(
    "${REPO_ROOT}/lib/dwc/src/base/dwc_init_version.c"
    "${REPO_ROOT}/lib/dwc/src/vct/vct_version.c"
    "${REPO_ROOT}/lib/nitrowifi/src/cps/cps_version.c"
    "${REPO_ROOT}/lib/nitrowifi/src/ssl/ssl_version.c"
    "${REPO_ROOT}/lib/nitro/src/card/card_backup_version.c"
    "${REPO_ROOT}/lib/nitrowifi/src/soc/socl_startup_version.c"
    PROPERTIES COMPILE_OPTIONS "-D__declspec(x)=")
# The game's main() becomes NitroMain(), which the platform layer calls on the game's thread
set_source_files_properties("${REPO_ROOT}/src/system/main.c" PROPERTIES COMPILE_DEFINITIONS "main=NitroMain")

# The host operating system's services (port/hal/host): one file for macOS and Linux and one for Windows, each compiled
# to nothing on the others. The platform layer compiles them with the rest of hal/; the tests that build a few files of
# their own add them
set(port_host_sources
    "${CMAKE_CURRENT_SOURCE_DIR}/hal/host/host_posix.c" "${CMAKE_CURRENT_SOURCE_DIR}/hal/host/host_windows.c")
# glibc's dladdr and dl_iterate_phdr: _GNU_SOURCE on the command line, so that it comes ahead of the decomp's forced
# include, whose system headers come first, in every target that compiles the file
set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/hal/host/host_posix.c" PROPERTIES
    COMPILE_DEFINITIONS _GNU_SOURCE)
# Windows gives the main thread 1 MiB of stack, where macOS and Linux give 8 MiB; the dispatcher, the interrupt
# handlers, the frame hooks and the renderers run on it (port/hal/os/core.c)
if(WIN32)
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -Wl,--stack,8388608")
endif()

# The platform layer: every C file under hal/, one directory per SDK module it replaces, compiled against the
# decomp's headers. A module lists the decomp files it replaces in its own replaced.txt
file(GLOB_RECURSE port_hal_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/hal/*.c")
file(GLOB port_hal_replaced CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/hal/*/replaced.txt")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${port_hal_replaced})
add_library(port_hal OBJECT ${port_hal_sources})
target_link_libraries(port_hal PRIVATE port_decomp_env)
target_include_directories(port_hal PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_hal PRIVATE -std=gnu11 -Wall -Wextra -Wno-unused-parameter)
# glibc's memfd_create for the VRAM's aliasing (hal/gx/vram_os.c), on the command line since port_host.h, force-included
# first, already pulls in system headers
target_compile_definitions(port_hal PRIVATE _GNU_SOURCE)

# The renderers and other code that doesn't include the decomp's headers: every C file under gfx/
file(GLOB_RECURSE port_gfx_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/gfx/*.c")
if(port_gfx_sources)
    add_library(port_gfx OBJECT ${port_gfx_sources})
    target_include_directories(port_gfx PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(port_gfx PRIVATE -std=c11 -Wall -Wextra)
    set(port_gfx_objects $<TARGET_OBJECTS:port_gfx>)
    set(port_gfx_targets port_gfx)
endif()

# Trap stubs for every DS symbol the game references that no compiled C defines yet (port/tools/gen_stubs.py). The
# objects' paths go in a file, one a line: on a command line, their 190 KB would pass Windows' limit of 32 KB. On Linux
# and Windows the compiler driver's libraries say what the C library provides too, read with the toolchain's nm (glibc
# keeps atexit in libc_nonshared.a, where Python's ctypes doesn't see it; Windows' Python has no such view at all)
set(port_stubs_objects "$<TARGET_OBJECTS:pokebw2_game>" "$<TARGET_OBJECTS:port_hal>" "$<TARGET_OBJECTS:port_app>"
    ${port_gfx_objects})
file(GENERATE OUTPUT "${PORT_GEN}/stubs_objects.txt" CONTENT "$<JOIN:${port_stubs_objects},\n>\n")
set(port_stubs_host_options "")
if(NOT APPLE)
    if(NOT CMAKE_NM)
        message(FATAL_ERROR "No nm was found for port/tools/gen_stubs.py; the toolchain's nm is the one it needs")
    endif()
    set(port_stubs_host_options --nm "${CMAKE_NM}" --compiler "${CMAKE_C_COMPILER}")
endif()
add_custom_command(
    OUTPUT "${PORT_GEN}/stubs.c"
    COMMAND "${PORT_PYTHON}" "${REPO_ROOT}/port/tools/gen_stubs.py"
        --version "${PORT_VERSION}" --out "${PORT_GEN}/stubs.c" --objects-file "${PORT_GEN}/stubs_objects.txt"
        ${port_stubs_host_options}
    # The objects themselves, not only their targets, so that the stubs follow every recompiled file
    DEPENDS pokebw2_game port_hal port_app ${port_gfx_targets} "${REPO_ROOT}/port/tools/gen_stubs.py"
        "${PORT_GEN}/stubs_objects.txt" ${port_stubs_objects}
    WORKING_DIRECTORY "${REPO_ROOT}"
    COMMAND_EXPAND_LISTS
    VERBATIM)

# The program's own code: main.c here; port/cmake/app.cmake adds the rest of port/app and the oracle's input file reader
# (port/tools/oracle/pbwinput.c), and port/cmake/sound.cmake adds port/audio
add_library(port_app OBJECT app/main.c)
target_include_directories(port_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(port_app PRIVATE -std=c11 -Wall -Wextra)
target_link_libraries(port_app PRIVATE SDL3::SDL3)

add_executable(${PORT_PROGRAM}
    $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> $<TARGET_OBJECTS:port_app> ${port_gfx_objects}
    "${PORT_GEN}/stubs.c")
target_include_directories(${PORT_PROGRAM} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${PORT_PROGRAM} PRIVATE SDL3::SDL3 ${PORT_LIBM})

# Labels, so that a part of the tests can be run by what it needs (ctest -L LABEL, or -LE LABEL to leave it out;
# port/tests/README.md). A test gets its labels where it is defined, with port_test_labels(TEST LABEL...):
#   route   the program runs the game from power-on on the player's ROM, and the test skips without it: the replays,
#           the oracle's sequences and the port's checks of their hashes, the smoke test (which passes without a ROM
#           too, stopping where the program looks for one). In the sanitized build these are its routes
#           (port/docs/sanitizers.md)
#   oracle  needs the melonDS oracle, or the saves it makes (skips without them). The routes that need no oracle are
#           ctest -L route -LE oracle
#   timing  a limit on the host's time, which a slow or loaded machine can fail: the frame_times_* routes, in an
#           optimized build only (port/cmake/frame_times.cmake), and the timing gates (PORT_G3D_TIMING_GATE,
#           PORT_GFX_TIMING_GATE)
#   static  the game's C read, not run: the pointer cast guard and its own test (port/cmake/game.cmake)
function(port_test_labels test)
    set_property(TEST ${test} APPEND PROPERTY LABELS ${ARGN})
endfunction()

# Modules' own build rules (per-file definitions, generated inputs): every port/cmake/*.cmake, in name order
file(GLOB port_cmake_fragments CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.cmake")
list(SORT port_cmake_fragments)
foreach(fragment IN LISTS port_cmake_fragments)
    include("${fragment}")
endforeach()
# The timing gates' limits are for the renderers as the plain build compiles them, not instrumented
if(PORT_SANITIZE AND (PORT_G3D_TIMING_GATE OR PORT_GFX_TIMING_GATE))
    message(FATAL_ERROR "The timing gates (PORT_G3D_TIMING_GATE, PORT_GFX_TIMING_GATE) would time the renderers under "
        "the sanitizers: configure them in a build without PORT_SANITIZE")
endif()

# Windows: every program of the build, the game's and the tests', carries port/app/windows/utf8.manifest, which makes
# UTF-8 its code page, so that its arguments, the C library's file calls and SDL's paths agree on a name that isn't
# ASCII. The resource script, written here, names the manifest by its path
if(WIN32)
    enable_language(RC)
    set(port_manifest "${CMAKE_CURRENT_SOURCE_DIR}/app/windows/utf8.manifest")
    set(port_manifest_rc "${PORT_GEN}/utf8_manifest.rc")
    file(CONFIGURE OUTPUT "${port_manifest_rc}" CONTENT "1 24 \"${port_manifest}\"\n")
    add_library(port_windows_manifest OBJECT "${port_manifest_rc}")
    set_property(SOURCE "${port_manifest_rc}" APPEND PROPERTY OBJECT_DEPENDS "${port_manifest}")
    get_property(port_targets DIRECTORY PROPERTY BUILDSYSTEM_TARGETS)
    foreach(target IN LISTS port_targets)
        get_target_property(type ${target} TYPE)
        if(type STREQUAL "EXECUTABLE")
            target_sources(${target} PRIVATE $<TARGET_OBJECTS:port_windows_manifest>)
        endif()
    endforeach()
endif()

# Tests (ctest --test-dir build/port). The smoke test runs the game's first frame: it passes on the line the program
# logs before it looks for the ROM and enters the game's main(), whatever status the program then exits with, so a trap
# stub stopping the frame is accepted, and so is the stop for want of a ROM (port_rom_boot, which is where it ends in
# CI); a crash, which the fault handler ends as the fault would have (a signal, or an exception's status on Windows),
# fails it. Like every test that runs the program, it has a save directory in the build tree, never the player's own
enable_testing()
add_test(NAME smoke_reaches_game_main COMMAND ${PORT_PROGRAM} --headless --frames 1 --watchdog 30
    --save-dir "${CMAKE_CURRENT_BINARY_DIR}/smoke_saves")
set_tests_properties(smoke_reaches_game_main PROPERTIES
    PASS_REGULAR_EXPRESSION "entering the game's main\\(\\)" TIMEOUT 60)
port_test_labels(smoke_reaches_game_main route)

# In the sanitized build a sanitizer's report in a test's output fails the test, whatever else the test expects. A
# report aborts the programs that link the platform layer (port/hal/os/sanitizers.c), which ctest fails, but a test
# program without it ends a report with status 1 off macOS, which a test that passes on its output or on a failure
# accepts. That holds for every test because none inverts its result with WILL_FAIL, which would make the failure a
# pass: a test of a failure checks how its program ends through tests/app/check_exit.py, which fails a run with a
# report whatever ending it wants, and configuring stops on a test with WILL_FAIL, in every build. A wrapper that runs
# the program shows ctest only what it prints, so each fails on the program's abort itself (port/docs/sanitizers.md).
# Deferred to the end of this directory, after the tests port/cmake/overlay_sections.cmake adds there
function(port_check_tests)
    get_property(tests DIRECTORY PROPERTY TESTS)
    foreach(test IN LISTS tests)
        get_property(will_fail TEST ${test} PROPERTY WILL_FAIL)
        if(will_fail)
            message(FATAL_ERROR "The test ${test} has WILL_FAIL, which would turn the sanitized build's failure on a "
                "sanitizer's report into a pass: check how its program ends with tests/app/check_exit.py instead")
        endif()
        if(PORT_SANITIZE)
            set_property(TEST ${test} APPEND PROPERTY FAIL_REGULAR_EXPRESSION
                "ERROR: (Address|Leak)Sanitizer" "SUMMARY: [A-Za-z]+Sanitizer" "runtime error: "
                "LeakSanitizer has encountered a fatal error")
        endif()
    endforeach()
endfunction()
cmake_language(DEFER CALL port_check_tests)
