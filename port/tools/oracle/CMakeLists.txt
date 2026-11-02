# The melonDS test oracle (README.md): a project of its own, so that C++ and melonDS stay out of the port's build and
# nothing of them can reach the port's program. port/cmake/oracle.cmake builds it as an external project when the port
# is configured with -DPORT_BUILD_ORACLE=ON, giving it the melonDS source and the xxHash release it fetched; on its own
# it fetches them itself:
#
#   cmake -S port/tools/oracle -B build/oracle -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build build/oracle
#
# Options: MELONDS_SOURCE_DIR, an unpacked melonDS of the pinned release (melonds.cmake) instead of fetching it;
# XXHASH_SOURCE_DIR, an unpacked xxHash of the pinned release (xxhash.cmake), whose top directory holds the xxhash.h
# that the per-game-frame digest (digest.c, --digest) compiles, instead of fetching it; ORACLE_SANITIZE=ON builds the
# oracle's own sources (not melonDS's) with AddressSanitizer and UndefinedBehaviorSanitizer.
cmake_minimum_required(VERSION 3.24)
project(pbw_oracle LANGUAGES C CXX)

include("${CMAKE_CURRENT_SOURCE_DIR}/melonds.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/xxhash.cmake")

# melonDS's headers need C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

set(MELONDS_SOURCE_DIR "" CACHE PATH "An unpacked melonDS ${ORACLE_MELONDS_TAG}; fetched into the build when empty")
set(XXHASH_SOURCE_DIR "" CACHE PATH "An unpacked xxHash ${DIGEST_XXHASH_TAG}; fetched into the build when empty")
option(ORACLE_SANITIZE "Build the oracle's own sources with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

if(NOT MELONDS_SOURCE_DIR)
    include(FetchContent)
    # SOURCE_SUBDIR names no CMakeLists.txt, so that this only downloads and unpacks: the core is added below, with
    # its options set first
    FetchContent_Declare(melonds
        URL "${ORACLE_MELONDS_URL}"
        URL_HASH "SHA256=${ORACLE_MELONDS_SHA256}"
        SOURCE_SUBDIR "-not-added-")
    FetchContent_MakeAvailable(melonds)
    set(MELONDS_SOURCE_DIR "${melonds_SOURCE_DIR}")
endif()
if(NOT EXISTS "${MELONDS_SOURCE_DIR}/src/NDS.h")
    message(FATAL_ERROR "${MELONDS_SOURCE_DIR} isn't a melonDS source tree")
endif()
# xxHash's archive has no CMakeLists.txt at its top, so making it available only unpacks it. Its xxhash.h is byte for
# byte the one melonDS bundles as src/xxhash/xxhash.h (xxhash.cmake), which isn't at the top of an include path
if(NOT XXHASH_SOURCE_DIR)
    include(FetchContent)
    FetchContent_Declare(xxhash
        URL "${DIGEST_XXHASH_URL}"
        URL_HASH "SHA256=${DIGEST_XXHASH_SHA256}")
    FetchContent_MakeAvailable(xxhash)
    set(XXHASH_SOURCE_DIR "${xxhash_SOURCE_DIR}")
endif()
if(NOT EXISTS "${XXHASH_SOURCE_DIR}/xxhash.h")
    message(FATAL_ERROR "${XXHASH_SOURCE_DIR} isn't an xxHash source tree")
endif()

# melonDS's core library only: no frontend, no JIT (the interpreter runs the same instructions in the same order on
# every run), no OpenGL, no GDB stub, no LTO, no ccache
set(BUILD_QT_SDL OFF CACHE BOOL "" FORCE)
set(ENABLE_JIT OFF CACHE BOOL "" FORCE)
set(ENABLE_OGLRENDERER OFF CACHE BOOL "" FORCE)
set(ENABLE_GDBSTUB OFF CACHE BOOL "" FORCE)
set(ENABLE_LTO_RELEASE OFF CACHE BOOL "" FORCE)
set(ENABLE_LTO OFF CACHE BOOL "" FORCE)
set(USE_CCACHE OFF CACHE BOOL "" FORCE)
set(USE_VCPKG OFF CACHE BOOL "" FORCE)
add_subdirectory("${MELONDS_SOURCE_DIR}" melonds EXCLUDE_FROM_ALL)
# melonDS's headers are system headers here, so that the oracle's warnings are its own
set_target_properties(core PROPERTIES SYSTEM ON)
# The core compiles with -fwrapv and hands it on to what links it (PUBLIC). Its own sources keep it; the oracle's are
# compiled by the languages' rules, where UndefinedBehaviorSanitizer checks signed overflow, which -fwrapv turns off
get_target_property(core_interface_options core INTERFACE_COMPILE_OPTIONS)
if(core_interface_options)
    list(REMOVE_ITEM core_interface_options -fwrapv)
    set_target_properties(core PROPERTIES INTERFACE_COMPILE_OPTIONS "${core_interface_options}")
endif()

add_executable(pbw-oracle oracle.cpp platform.cpp pngout.cpp symtab.cpp pbwinput.c display3d.c digest.c)
target_link_libraries(pbw-oracle PRIVATE core)
target_include_directories(pbw-oracle SYSTEM PRIVATE "${MELONDS_SOURCE_DIR}/src")
# digest.c's hash: XXH_INLINE_ALL keeps xxHash's functions private to digest.c, apart from the copy melonDS's core links
target_include_directories(pbw-oracle SYSTEM PRIVATE "${XXHASH_SOURCE_DIR}")
target_compile_definitions(pbw-oracle PRIVATE ORACLE_MELONDS_VERSION="${ORACLE_MELONDS_TAG}")
target_compile_options(pbw-oracle PRIVATE -Wall -Wextra)
set_target_properties(pbw-oracle PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    target_compile_definitions(pbw-oracle PRIVATE ORACLE_HAVE_ZLIB)
    target_link_libraries(pbw-oracle PRIVATE ZLIB::ZLIB)
endif()
if(ORACLE_SANITIZE)
    target_compile_options(pbw-oracle PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=all
                                              -fno-omit-frame-pointer)
    target_link_options(pbw-oracle PRIVATE -fsanitize=address,undefined)
endif()
