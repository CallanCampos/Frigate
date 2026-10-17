# The melonDS test oracle (port/tools/oracle, PLAN.md §6 T4) and its tests (port/tests/oracle).
#
# The input file reader (pbwinput.c, which the port can compile as it is), the per-screen 3D decision (display3d.c) and
# compare.py are tested in every build: they need neither melonDS nor a ROM. The oracle itself is a test tool, never
# part of the port's program (PLAN.md §1): it is built only when the port is configured with -DPORT_BUILD_ORACLE=ON,
# which fetches melonDS's pinned release (port/tools/oracle/melonds.cmake) into the build directory at configure time
# and builds the oracle as a project of its own (port/tools/oracle/CMakeLists.txt) in <build>/oracle. Normal builds and
# CI download nothing.
option(PORT_BUILD_ORACLE "Fetch melonDS and build the test oracle in port/tools/oracle (a test tool only)" OFF)
option(PORT_ORACLE_SANITIZE "Build the oracle's own sources with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(port_oracle_dir "${CMAKE_CURRENT_SOURCE_DIR}/tools/oracle")
set(port_oracle_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/oracle")
set(port_oracle_binary "${CMAKE_BINARY_DIR}/oracle/pbw-oracle")

# The input file reader and the per-screen 3D decision, plain and under the sanitizers. Each program's temporary files
# are named after it, so that the variants can run at the same time
foreach(unit IN ITEMS pbwinput display3d)
    foreach(variant IN ITEMS plain sanitized)
        set(target port_test_oracle_${unit}_${variant})
        add_executable(${target} "${port_oracle_tests}/test_${unit}.c" "${port_oracle_dir}/${unit}.c")
        target_include_directories(${target} PRIVATE "${port_oracle_dir}")
        target_compile_options(${target} PRIVATE -std=c11 -Wall -Wextra -Werror)
        if(variant STREQUAL "sanitized")
            target_compile_options(${target} PRIVATE
                -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
            target_link_options(${target} PRIVATE -fsanitize=address,undefined)
        endif()
        add_test(NAME oracle_${unit}_${variant} COMMAND ${target})
    endforeach()
endforeach()

# compare.py and symbols.py on made-up runs
add_test(NAME oracle_compare
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
        "${PORT_PYTHON}" "${port_oracle_tests}/test_compare.py"
    WORKING_DIRECTORY "${REPO_ROOT}")
set_tests_properties(oracle_compare PROPERTIES TIMEOUT 300)

if(PORT_BUILD_ORACLE)
    include("${port_oracle_dir}/melonds.cmake")
    include(FetchContent)
    include(ExternalProject)
    set(port_melonds_source "${CMAKE_BINARY_DIR}/_deps/melonds-${ORACLE_MELONDS_TAG}-src")
    FetchContent_Populate(port_melonds
        QUIET
        URL "${ORACLE_MELONDS_URL}"
        URL_HASH "SHA256=${ORACLE_MELONDS_SHA256}"
        SOURCE_DIR "${port_melonds_source}"
        SUBBUILD_DIR "${CMAKE_BINARY_DIR}/_deps/melonds-${ORACLE_MELONDS_TAG}-subbuild"
        BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/melonds-${ORACLE_MELONDS_TAG}-build")
    ExternalProject_Add(port_oracle
        SOURCE_DIR "${port_oracle_dir}"
        BINARY_DIR "${CMAKE_BINARY_DIR}/oracle"
        CMAKE_GENERATOR "${CMAKE_GENERATOR}"
        CMAKE_ARGS
            "-DMELONDS_SOURCE_DIR=${port_melonds_source}"
            "-DCMAKE_BUILD_TYPE=Release"
            "-DORACLE_SANITIZE=${PORT_ORACLE_SANITIZE}"
        # Its own sources change without the port's build noticing; its ninja is quick when nothing did
        BUILD_ALWAYS ON
        INSTALL_COMMAND ""
        BUILD_BYPRODUCTS "${port_oracle_binary}")
endif()

# The oracle on the player's ROM: the boot sequence runs twice with the same result, and its screens are the hashes
# port/tests/oracle keeps for the pinned melonDS. Skipped (77) without the oracle or the ROM
add_test(NAME oracle_boot
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
        "${PORT_PYTHON}" "${port_oracle_dir}/oracle.py" check
            --oracle "${port_oracle_binary}" --version "${PORT_VERSION}"
            --work "${CMAKE_BINARY_DIR}/oracle-check"
    WORKING_DIRECTORY "${REPO_ROOT}")
set_tests_properties(oracle_boot PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1200)

# The title sequence (port/tests/oracle/title_<version>.pbwinput: the boot screens, the Game Freak logo, the opening
# skipped with START, the title with its 3D on either screen, START to the start menu), held to the oracle in two ways
# (port/tools/oracle/README.md, "The title sequence"):
# - oracle_title_hashes, every build: the port alone, its screens without 3D at the frames that
#   port/tests/oracle/title_<version>.tsv keeps, against the oracle's hashes there. It proves those screens are the
#   oracle's at 6 bits a channel, and that the port reaches each moment of the sequence at the same frame as when the
#   reference was written; it can't judge a screen with 3D, nor the frames between the ones it keeps. Skipped (77)
#   without the ROM; only for a version that has a reference
# - oracle_title, with the oracle built: both programs, every frame of the port but the known differences compared, the
#   3D within compare.py's tolerance; a failure keeps the runs and the difference images and says where. Skipped (77)
#   without the oracle or the ROM
if(EXISTS "${port_oracle_tests}/title_${PORT_VERSION}.tsv")
    add_test(NAME oracle_title_hashes
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
            "${PORT_PYTHON}" "${port_oracle_dir}/oracle.py" title-check
                --port "$<TARGET_FILE:${PORT_PROGRAM}>" --version "${PORT_VERSION}"
                --work "${CMAKE_BINARY_DIR}/oracle-title-check"
        WORKING_DIRECTORY "${REPO_ROOT}")
    set_tests_properties(oracle_title_hashes PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
endif()
add_test(NAME oracle_title
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
        "${PORT_PYTHON}" "${port_oracle_dir}/oracle.py" title
            --oracle "${port_oracle_binary}" --port "$<TARGET_FILE:${PORT_PROGRAM}>" --version "${PORT_VERSION}"
    WORKING_DIRECTORY "${REPO_ROOT}")
set_tests_properties(oracle_title PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 2400)
