# The melonDS test oracle (port/tools/oracle, PLAN.md §6 T4) and its tests (port/tests/oracle), which hold Frigate's
# run of the game to melonDS's: the screens and memory of the boot, the opening, the title and start menu, CONTINUE and
# a save; the saves made on the oracle from saves (the first save, and the starter save with a Pokémon and the
# Pokédex), which the sequences and launch checks start from; the field menu's apps (the bag, the trainer card, the
# party list and the summary) through Frigate's test launcher (ADR 0012), and the launcher's own exit statuses; and the
# sound of the opening and the title (the ARM7 sound driver run by run; its output: a stream and a cry to the bit, the
# music by its envelope and levels).
#
# The input file reader (pbwinput.c, which the port can compile as it is), the per-screen 3D decision (display3d.c),
# compare.py and sound.py are tested in every build: they need neither melonDS nor a ROM. The oracle itself is a test
# tool, never part of Frigate (PLAN.md §1): it is built only when the port is configured with -DPORT_BUILD_ORACLE=ON,
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
# The continue sequence starts from a save that the oracle makes first, from power-on through New Game into the field
# and its SAVE (oracle.py save first_save, checked against the SHA-1 oracle.py keeps): the start menu's CONTINUE with
# the saved game, then the field, where the port stops on overlay 36 for now. continue_starter plays the same presses
# from the starter save, made from the first save's CONTINUE through the story to the first Pokémon, whose start menu
# shows the Pokédex line. Only the oracle can make their saves, so they have no reference and no _hashes test; the
# oracle_saves test makes the saves into the build tree first (a fixture), where a later run reuses each while its
# SHA-1 is still the one oracle.py keeps
set(port_oracle_saves "${CMAKE_BINARY_DIR}/oracle-saves")
add_test(NAME oracle_saves
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
        "${PORT_PYTHON}" "${port_oracle_dir}/oracle.py" save first_save starter_save
            --oracle "${port_oracle_binary}" --version "${PORT_VERSION}" --save-cache "${port_oracle_saves}"
    WORKING_DIRECTORY "${REPO_ROOT}")
# Each save from scratch runs the oracle some 18,000 frames
set_tests_properties(oracle_saves PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 3600 FIXTURES_SETUP oracle_saves)
foreach(sequence IN ITEMS opening title continue continue_starter)
    if(EXISTS "${port_oracle_tests}/${sequence}_${PORT_VERSION}.tsv")
        add_test(NAME oracle_${sequence}_hashes
            COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
                "${PORT_PYTHON}" "${port_oracle_dir}/oracle.py" sequence-check ${sequence}
                    --port "$<TARGET_FILE:${PORT_PROGRAM}>" --version "${PORT_VERSION}"
                    --work "${CMAKE_BINARY_DIR}/oracle-${sequence}-check"
            WORKING_DIRECTORY "${REPO_ROOT}")
        # A few minutes on an idle machine; a loaded one, or a slow CI runner, takes several times that (the title's
        # check passed 600 s at a load average of 60), so the limit only catches a run that hangs
        set_tests_properties(oracle_${sequence}_hashes PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1800)
    endif()
    add_test(NAME oracle_${sequence}
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
            "${PORT_PYTHON}" "${port_oracle_dir}/oracle.py" sequence ${sequence}
                --oracle "${port_oracle_binary}" --port "$<TARGET_FILE:${PORT_PROGRAM}>" --version "${PORT_VERSION}"
                --save-cache "${port_oracle_saves}"
        WORKING_DIRECTORY "${REPO_ROOT}")
    set_tests_properties(oracle_${sequence} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 2400)
endforeach()
# The sequences from a save take it from oracle_saves; without the cache (a failed fixture) they would make it again
set_tests_properties(oracle_continue oracle_continue_starter PROPERTIES FIXTURES_REQUIRED oracle_saves TIMEOUT 3600)

# The field menu's apps on Frigate's test launcher (ADR 0012) against the oracle (oracle.py launch, README.md's "The
# launch checks"): the oracle reaches each app through the field and its menu with the route's input file
# (port/tests/oracle/launch_<route>_<version>.pbwinput), the port plays the same file from the same save with --launch
# at the oracle's counts; the oracle's app screens (launch_<route>_<version>.screens.tsv) must show on the port at their
# counts, and every pass of each app the oracle's. field_menus starts from the first save (the bag, the trainer card),
# party_menus from the starter save (the party list and its Pokémon's menu, the card with the Pokédex line, the bag;
# the route's summary and Pokédex are known differences, left out: the oracle README's "The launch checks"). Skipped
# (77) without the oracle or the ROM, or for a version the route isn't measured in
foreach(route IN ITEMS field_menus party_menus)
    add_test(NAME oracle_launch_${route}
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
            "${PORT_PYTHON}" "${port_oracle_dir}/oracle.py" launch ${route}
                --oracle "${port_oracle_binary}" --port "$<TARGET_FILE:${PORT_PROGRAM}>" --version "${PORT_VERSION}"
                --save-cache "${port_oracle_saves}"
        WORKING_DIRECTORY "${REPO_ROOT}")
    set_tests_properties(oracle_launch_${route} PROPERTIES
        FIXTURES_REQUIRED oracle_saves SKIP_RETURN_CODE 77 TIMEOUT 3600)
endforeach()

# The launcher on the port alone, from the starter save the oracle made: --launch summary opens Tepig's summary as the
# game does without the party list, and B closes it, without a trap and without writing the save. Black 2 only, since
# White 2 has no starter save yet; skipped (77) without the save (no oracle) or the ROM
if(PORT_VERSION STREQUAL "b2_us")
    add_test(NAME launch_summary_standalone
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
            "${PORT_PYTHON}" "${port_oracle_tests}/check_launch_smoke.py"
                --save "${port_oracle_saves}/starter_save_b2_us.sav" --save-name IREO.sav
                --expect "--launch: summary: opened at count [0-9]+, closed at count [0-9]+: result 0, event type 0"
                -- "$<TARGET_FILE:${PORT_PROGRAM}>" --headless --frames 4200 --watchdog 30
                --input "${port_oracle_tests}/launch_summary_b2_us.pbwinput" --launch summary
        WORKING_DIRECTORY "${REPO_ROOT}")
    set_tests_properties(launch_summary_standalone PROPERTIES
        FIXTURES_REQUIRED oracle_saves SKIP_RETURN_CODE 77 TIMEOUT 900)

    # Each app opens as the field menu's choice by a key leaves it, whatever the app before it did: here the trainer
    # card closed by a touch, which sets the UI's touch flag, then the party list, which must show the oracle's party
    # list of the party_menus route at count 4250 (opened by a key there), not the one a touch leaves (its bottom
    # screen differs). Black 2 only, from the starter save
    add_test(NAME launch_touch_then_party
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
            "${PORT_PYTHON}" "${port_oracle_tests}/check_launch_smoke.py"
                --save "${port_oracle_saves}/starter_save_b2_us.sav" --save-name IREO.sav
                --expect "--launch: party: opened at count 4044, closed at count [0-9]+: result 0, event type 0"
                --table "${port_oracle_tests}/launch_party_menus_b2_us.screens.tsv" --show party
                -- "$<TARGET_FILE:${PORT_PROGRAM}>" --headless --frames 4500 --watchdog 30
                --input "${port_oracle_tests}/launch_touch_card_b2_us.pbwinput" --launch card,party@4044
        WORKING_DIRECTORY "${REPO_ROOT}")
    set_tests_properties(launch_touch_then_party PROPERTIES
        FIXTURES_REQUIRED oracle_saves SKIP_RETURN_CODE 77 TIMEOUT 900)
endif()

# The launcher's other exit statuses (port/app/launcher.h), from the first save the oracle made, in both versions:
# - launch_unfinished: field_menus' presses with --frames short of the card's count, so the bag opens and closes but
#   the card never opens: status 1, and the run says so;
# - launch_not_offered: the party list from a save without a Pokémon, whose field menu (type 3, event flag 0x961
#   clear) has no POKéMON item, as the oracle's menu on that save shows: status 2 before anything opens.
# Skipped (77) without the save (no oracle) or the ROM
if(PORT_VERSION STREQUAL "b2_us")
    set(port_launch_save_name IREO.sav)
else()
    set(port_launch_save_name IRDO.sav)
endif()
add_test(NAME launch_unfinished
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
        "${PORT_PYTHON}" "${port_oracle_tests}/check_launch_smoke.py"
            --save "${port_oracle_saves}/first_save_${PORT_VERSION}.sav" --save-name ${port_launch_save_name} --status 1
            --expect "--launch: the run ended before the launch did: card not opened"
            -- "$<TARGET_FILE:${PORT_PROGRAM}>" --headless --frames 4400 --watchdog 30
            --input "${port_oracle_tests}/launch_field_menus_${PORT_VERSION}.pbwinput" --launch bag@4044,card@4584
    WORKING_DIRECTORY "${REPO_ROOT}")
add_test(NAME launch_not_offered
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
        "${PORT_PYTHON}" "${port_oracle_tests}/check_launch_smoke.py"
            --save "${port_oracle_saves}/first_save_${PORT_VERSION}.sav" --save-name ${port_launch_save_name} --status 2
            --expect "--launch: party: the field menu doesn't offer it in this save, which gets the menu without a Pok"
            -- "$<TARGET_FILE:${PORT_PROGRAM}>" --headless --frames 3700 --watchdog 30
            --input "${port_oracle_tests}/launch_field_menus_${PORT_VERSION}.pbwinput" --launch party
    WORKING_DIRECTORY "${REPO_ROOT}")
set_tests_properties(launch_unfinished launch_not_offered PROPERTIES
    FIXTURES_REQUIRED oracle_saves SKIP_RETURN_CODE 77 TIMEOUT 900)

# The other way, a save from the port to melonDS (T6): the port's New Game formats the blank backup, and the oracle
# boots that save to the start menu frame for frame as it boots its own save of the same moment, writing neither; the
# console's Wi-Fi ID that New Game copied into the save is the oracle's (oracle.py port-save). Skipped (77) without
# the oracle or the ROM
add_test(NAME oracle_port_save
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
        "${PORT_PYTHON}" "${port_oracle_dir}/oracle.py" port-save
            --oracle "${port_oracle_binary}" --port "$<TARGET_FILE:${PORT_PROGRAM}>" --version "${PORT_VERSION}"
    WORKING_DIRECTORY "${REPO_ROOT}")
set_tests_properties(oracle_port_save PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 2400)

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
