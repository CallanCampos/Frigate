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

# sound.py, the sound comparison, on made-up sound traces and WAV dumps
add_test(NAME oracle_sound_tool
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
        "${PORT_PYTHON}" "${port_oracle_tests}/test_sound.py"
    WORKING_DIRECTORY "${REPO_ROOT}")
set_tests_properties(oracle_sound_tool PROPERTIES TIMEOUT 300)

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

# The comparison sequences (port/tools/oracle/README.md, "The comparison sequences"), each an input file
# port/tests/oracle/<name>_<version>.pbwinput: opening, power-on through the boot screens, the Game Freak logo and the
# whole opening to the title's first frames with nothing pressed; title, the opening skipped with START, the title with
# its 3D on either screen and START to the start menu. Each is held to the oracle in two ways:
# - oracle_<name>_hashes, every build: the port alone against port/tests/oracle/<name>_<version>.tsv, the oracle's
#   hashes at the frames it keeps. Where the game counts passes, it proves the port reaches each of those frames at
#   the oracle's count (plus the declared lag) and that its screens without 3D are the oracle's there at 6 bits a
#   channel; where the game waits on a clock (the movies), that the port shows the oracle's pictures at the frames it
#   showed them when the reference was written. It can't judge a screen with 3D, the game's state, nor the frames
#   between the ones it keeps. Skipped (77) without the ROM; only for a version that has a reference
# - oracle_<name>, with the oracle built: both programs, every frame of the port but the known differences compared,
#   on the game's count (by their pictures in the movies), the 3D within compare.py's tolerance, with every picture
#   the oracle shows between two of the port's frames shown by one of them, and NNS G3D's state compared on the count
#   at the frames the sequence names. It can't time the movies: there the DS's passes take 3 to 5 frames by its CPU's
#   load, so only the pictures and their order are held. A failure keeps the runs and the difference images and says
#   where. Skipped (77) without the oracle or the ROM
foreach(sequence IN ITEMS opening title)
    if(EXISTS "${port_oracle_tests}/${sequence}_${PORT_VERSION}.tsv")
        add_test(NAME oracle_${sequence}_hashes
            COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
                "${PORT_PYTHON}" "${port_oracle_dir}/oracle.py" sequence-check ${sequence}
                    --port "$<TARGET_FILE:${PORT_PROGRAM}>" --version "${PORT_VERSION}"
                    --work "${CMAKE_BINARY_DIR}/oracle-${sequence}-check"
            WORKING_DIRECTORY "${REPO_ROOT}")
        set_tests_properties(oracle_${sequence}_hashes PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
    endif()
    add_test(NAME oracle_${sequence}
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
            "${PORT_PYTHON}" "${port_oracle_dir}/oracle.py" sequence ${sequence}
                --oracle "${port_oracle_binary}" --port "$<TARGET_FILE:${PORT_PROGRAM}>" --version "${PORT_VERSION}"
        WORKING_DIRECTORY "${REPO_ROOT}")
    set_tests_properties(oracle_${sequence} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 2400)
endforeach()

# The same sequences' sound (port/tools/oracle/README.md, "Sound"), held to the oracle in two ways too:
# - oracle_sound_<name>_hashes, every build: the port alone, with its sound trace and WAV dump, against
#   port/tests/oracle/sound_<name>_<version>.tsv, the oracle's hashes and statistics: the sound driver's state at every
#   periodic run of each play the comparison pairs, by the sequence's own clock; what the ARM9 sees of the driver as
#   the frames end where the game counts passes; and by segment the sound's envelope, levels and samples at full
#   scale, and a stream's and a cry's samples to the bit. Skipped (77) without the ROM; only for a version that has a
#   reference
# - oracle_sound_<name>, with the oracle built: both programs, compared the same way. Skipped (77) without the oracle
#   or the ROM
foreach(sequence IN ITEMS opening title)
    if(EXISTS "${port_oracle_tests}/sound_${sequence}_${PORT_VERSION}.tsv")
        add_test(NAME oracle_sound_${sequence}_hashes
            COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
                "${PORT_PYTHON}" "${port_oracle_dir}/oracle.py" sound-check ${sequence}
                    --port "$<TARGET_FILE:${PORT_PROGRAM}>" --version "${PORT_VERSION}"
            WORKING_DIRECTORY "${REPO_ROOT}")
        set_tests_properties(oracle_sound_${sequence}_hashes PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 900)
    endif()
    add_test(NAME oracle_sound_${sequence}
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
            "${PORT_PYTHON}" "${port_oracle_dir}/oracle.py" sound ${sequence}
                --oracle "${port_oracle_binary}" --port "$<TARGET_FILE:${PORT_PROGRAM}>" --version "${PORT_VERSION}"
        WORKING_DIRECTORY "${REPO_ROOT}")
    set_tests_properties(oracle_sound_${sequence} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 2400)
endforeach()
