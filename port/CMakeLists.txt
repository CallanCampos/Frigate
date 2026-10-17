# The native port's host build. The ROM build (configure.py and ninja) stays separate and untouched; this one compiles
# the same decomp sources with the host's clang into one program, with the platform layer in port/.
#
#   cmake -S port -B build/port -G Ninja
#   cmake --build build/port
#
# Options: -DPORT_VERSION=b2_us|w2_us (default b2_us).
cmake_minimum_required(VERSION 3.28)
project(pokebw2_native LANGUAGES C)

set(PORT_VERSION "b2_us" CACHE STRING "The game version to build: b2_us (Black 2) or w2_us (White 2)")
set_property(CACHE PORT_VERSION PROPERTY STRINGS b2_us w2_us)
if(PORT_VERSION STREQUAL "b2_us")
    set(PORT_VERSION_DEFINE BLACK2)
    set(PORT_PROGRAM pokeblack2)
elseif(PORT_VERSION STREQUAL "w2_us")
    set(PORT_VERSION_DEFINE WHITE2)
    set(PORT_PROGRAM pokewhite2)
else()
    message(FATAL_ERROR "PORT_VERSION must be b2_us or w2_us, not ${PORT_VERSION}")
endif()

get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
set(PORT_GEN "${CMAKE_CURRENT_BINARY_DIR}/gen")

# The repository's virtual environment when there is one (it has the decomp's Python packages), else any Python 3
if(EXISTS "${REPO_ROOT}/.venv/bin/python")
    set(PORT_PYTHON "${REPO_ROOT}/.venv/bin/python")
else()
    find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
    set(PORT_PYTHON "${Python3_EXECUTABLE}")
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
    "${REPO_ROOT}/port/hal/replaced.txt" "${REPO_ROOT}/port/tools/gen_host_tree.py")
file(GLOB_RECURSE port_watched CONFIGURE_DEPENDS
    "${REPO_ROOT}/src/*.c" "${REPO_ROOT}/lib/*.c" "${REPO_ROOT}/lib/*.h" "${REPO_ROOT}/include/*")
file(STRINGS "${PORT_GEN}/sources.txt" game_sources_relative)
list(TRANSFORM game_sources_relative PREPEND "${REPO_ROOT}/" OUTPUT_VARIABLE game_sources)

# SDL3 3.4: an installed one when there is one (Homebrew, a distribution), else the pinned release built along
option(PORT_FETCH_SDL3 "Build the pinned SDL3 release when no installed SDL3 3.4 is found" ON)
find_package(SDL3 3.4 CONFIG QUIET)
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
    FetchContent_MakeAvailable(SDL3)
endif()

# What every file that includes the decomp's headers needs, the decomp's own C and the platform layer alike: MWCC's
# language model (signed char, int-sized enums, 16-bit wchar_t, wrapping signed arithmetic, no strict aliasing since
# the code puns types, no float contraction, and zeroed locals, since uninitialized reads gave deterministic values on
# the DS; PLAN.md §4.4), the version, and the include path with the host variants of the headers that name DS
# addresses first (ADR 0004). The decomp's headers are system headers here, so their warnings stay out of port code
add_library(port_decomp_env INTERFACE)
target_compile_definitions(port_decomp_env INTERFACE PLATFORM_PC ${PORT_VERSION_DEFINE})
file(GLOB library_includes LIST_DIRECTORIES true "${REPO_ROOT}/lib/*/include")
target_include_directories(port_decomp_env SYSTEM INTERFACE
    "${PORT_GEN}/hwinclude" "${PORT_GEN}/include" ${library_includes} "${REPO_ROOT}/build/include/generated")
target_compile_options(port_decomp_env INTERFACE
    -fsigned-char -fshort-wchar -fwrapv -fno-strict-aliasing -ffp-contract=off -ftrivial-auto-var-init=zero
    -fno-common
    "SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/include/port_host.h"
    # The decomp declares C library functions itself (DWC's ext.h), which the macOS headers' fortify macros rename
    -D_FORTIFY_SOURCE=0
    # Errors that are never a porting artefact. MWCC accepts dropping const or volatile through a pointer, and the
    # decomp does it where the original did, so that part of the group stays a warning
    -Werror=implicit-function-declaration -Werror=int-conversion -Werror=incompatible-pointer-types
    -Wno-error=incompatible-pointer-types-discards-qualifiers)

add_library(pokebw2_game OBJECT ${game_sources})
target_link_libraries(pokebw2_game PRIVATE port_decomp_env)
target_compile_options(pokebw2_game PRIVATE
    -std=gnu99
    # MWCC's inline: a function defined `inline` without `static` (ov194's BackupMail) is also an external definition,
    # which C99 would leave to another file, so an unoptimized build that doesn't inline the calls fails to link
    -fgnu89-inline
    # See port_decomp_env: the decomp drops const and volatile where the original did
    -Wno-incompatible-pointer-types-discards-qualifiers
    # The 64-bit warnings are counted by port/tools/compile-probe; here they only make noise
    -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-void-pointer-to-int-cast
    -Wno-int-to-void-pointer-cast -Wno-unknown-pragmas)
# The game's main() becomes NitroMain(), which the platform layer calls on the game's thread
set_source_files_properties("${REPO_ROOT}/src/system/main.c" PROPERTIES COMPILE_DEFINITIONS "main=NitroMain")

# The platform layer: every C file under hal/, one directory per SDK module it replaces, compiled against the
# decomp's headers. A module lists the decomp files it replaces in its own replaced.txt
file(GLOB_RECURSE port_hal_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/hal/*.c")
file(GLOB port_hal_replaced CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/hal/*/replaced.txt")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${port_hal_replaced})
add_library(port_hal OBJECT ${port_hal_sources})
target_link_libraries(port_hal PRIVATE port_decomp_env)
target_include_directories(port_hal PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_hal PRIVATE -std=gnu11 -Wall -Wextra -Wno-unused-parameter)
# dladdr and Dl_info on glibc; port_host.h, force-included first, already pulls in system headers
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

# Trap stubs for every DS symbol the game references that no compiled C defines yet (port/tools/gen_stubs.py)
add_custom_command(
    OUTPUT "${PORT_GEN}/stubs.c"
    COMMAND "${PORT_PYTHON}" "${REPO_ROOT}/port/tools/gen_stubs.py"
        --version "${PORT_VERSION}" --out "${PORT_GEN}/stubs.c"
        --objects "$<TARGET_OBJECTS:pokebw2_game>" "$<TARGET_OBJECTS:port_hal>" "$<TARGET_OBJECTS:port_app>"
                  ${port_gfx_objects}
    # The objects themselves, not only their targets, so that the stubs follow every recompiled file
    DEPENDS pokebw2_game port_hal port_app ${port_gfx_targets} "${REPO_ROOT}/port/tools/gen_stubs.py"
        "$<TARGET_OBJECTS:pokebw2_game>" "$<TARGET_OBJECTS:port_hal>" "$<TARGET_OBJECTS:port_app>" ${port_gfx_objects}
    WORKING_DIRECTORY "${REPO_ROOT}"
    COMMAND_EXPAND_LISTS
    VERBATIM)

add_library(port_app OBJECT app/main.c)
target_include_directories(port_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(port_app PRIVATE -std=c11 -Wall -Wextra)
target_link_libraries(port_app PRIVATE SDL3::SDL3)

add_executable(${PORT_PROGRAM}
    $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> $<TARGET_OBJECTS:port_app> ${port_gfx_objects}
    "${PORT_GEN}/stubs.c")
target_include_directories(${PORT_PROGRAM} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(${PORT_PROGRAM} PRIVATE SDL3::SDL3)

# Modules' own build rules (per-file definitions, generated inputs): every port/cmake/*.cmake, in name order
file(GLOB port_cmake_fragments CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.cmake")
list(SORT port_cmake_fragments)
foreach(fragment IN LISTS port_cmake_fragments)
    include("${fragment}")
endforeach()

# Tests (ctest --test-dir build/port). The smoke test runs the game's first frame: it passes once the program enters
# the game's main() and the frame ends without a crash (a trap stub stopping it is still accepted)
enable_testing()
add_test(NAME smoke_reaches_game_main COMMAND ${PORT_PROGRAM} --headless --frames 1 --watchdog 30)
set_tests_properties(smoke_reaches_game_main PROPERTIES
    PASS_REGULAR_EXPRESSION "entering the game's main\\(\\)" TIMEOUT 60)
