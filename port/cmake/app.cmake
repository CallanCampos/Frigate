# The program's own files besides main.c (port/app): the window, presentation, input and screenshots, the window's icon,
# the gamepads, the log of an app opened from the Finder, the run directory for the oracle's comparisons, the sound
# driver's trace, and input replays with the oracle's input file reader (port/tools/oracle/pbwinput.c, plain C11). The
# screens they show are drawn by port/gfx/screens.c, which the port_gfx target builds. Their tests are here too, with
# those of check_exit.py, the checker of the tests of the program's endings.
target_sources(port_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app/present.c" "${CMAKE_CURRENT_SOURCE_DIR}/app/gamepad.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/message_log.c" "${CMAKE_CURRENT_SOURCE_DIR}/app/replay.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/run_dir.c" "${CMAKE_CURRENT_SOURCE_DIR}/app/sound_trace.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/oracle/pbwinput.c")
# The version an input file's "version" line must name
target_compile_definitions(port_app PRIVATE "PORT_VERSION_NAME=\"${PORT_VERSION}\"")

# --run-dump finds the game's globals by name (port_host_global_address, port/hal/host) without exporting them: dlsym on
# macOS, the program's own symbol table on Linux and Windows
target_link_libraries(${PORT_PROGRAM} PRIVATE ${CMAKE_DL_LIBS})

# The window's icon on Linux and Windows (port/app/window_icon.c). port/tools/gen_window_icon.c, built for the machine
# that builds, makes two bitmaps into a C file when the artwork changes: the version's 32 px window icon, drawn pixel by
# pixel (PORT_WINDOW_ICON, port/assets/branding), as it is, and its app icon's master (PORT_APP_ICON) scaled to 64 px
# for a display at 200%; the program carries those bitmaps instead of the PNGs. On macOS the window sets none: the
# app's icon is the Dock's (port/cmake/macos_app.cmake)
set(port_window_icon_c "${PORT_GEN}/window_icon_pixels.c")
add_executable(port_gen_window_icon "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_window_icon.c")
target_compile_options(port_gen_window_icon PRIVATE -std=c11 -Wall -Wextra -Werror)
target_link_libraries(port_gen_window_icon PRIVATE SDL3::SDL3)
add_custom_command(OUTPUT "${port_window_icon_c}"
    COMMAND port_gen_window_icon "${PORT_WINDOW_ICON}" "${PORT_APP_ICON}" "${port_window_icon_c}"
    DEPENDS port_gen_window_icon "${PORT_WINDOW_ICON}" "${PORT_APP_ICON}"
    COMMENT "Making the window's icon from the artwork"
    VERBATIM)
target_sources(port_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app/window_icon.c" "${port_window_icon_c}")

# The window's icon, plain and under the sanitizers: the 32 px bitmap is the window icon drawn for it, pixel for pixel,
# the 64 px one its alternate for 200%, both the version's with the plate filling the square, and giving the icon to a
# window of SDL's dummy video driver goes through (port/tests/app/test_window_icon.c)
foreach(variant IN ITEMS plain sanitized)
    set(target port_test_window_icon_${variant})
    add_executable(${target} "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_window_icon.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/app/window_icon.c" "${port_window_icon_c}")
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_definitions(${target} PRIVATE ${PORT_VERSION_DEFINE} "PORT_WINDOW_ICON_PNG=\"${PORT_WINDOW_ICON}\"")
    target_compile_options(${target} PRIVATE -std=c11 -Wall -Wextra -Werror)
    target_link_libraries(${target} PRIVATE SDL3::SDL3)
    if(variant STREQUAL "sanitized")
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME app_window_icon_${variant} COMMAND ${target})
    set_tests_properties(app_window_icon_${variant} PROPERTIES TIMEOUT 60)
endforeach()

# check_exit.py, the checker of the tests of the program's endings (port/tests/app/test_check_exit.py): each ending,
# met and missed, a skip, a crash that its report names and one that it doesn't, and a sanitizer's report, which fails
# a run whatever ending it wants
add_test(NAME app_check_exit
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_check_exit.py")
set_tests_properties(app_check_exit PROPERTIES TIMEOUT 120)

# --help's first lines against the product's names that the build read from port/hal/os/port_app.h, which the macOS
# app's names and About window use too; then each option of its usage described (port/tests/app/check_about.cmake)
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/app_about_expected.txt"
    "${PORT_PRODUCT_NAME} — ${PORT_GAME_NAME}\n${PORT_PRODUCT_DESCRIPTION}\n${PORT_PRODUCT_DISCLAIMER}\n\n")
add_test(NAME app_about COMMAND "${CMAKE_COMMAND}" "-DPROGRAM=$<TARGET_FILE:${PORT_PROGRAM}>"
    "-DEXPECTED=${CMAKE_CURRENT_BINARY_DIR}/app_about_expected.txt" "-DSAVE_DIR=${CMAKE_CURRENT_BINARY_DIR}/app_about_saves"
    -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/check_about.cmake")
set_tests_properties(app_about PROPERTIES TIMEOUT 60)

# The lookup of the program's globals that --run-dump uses (port/hal/host), on the test program's own symbols
add_executable(port_test_symbols "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_symbols.c" ${port_host_sources})
target_include_directories(port_test_symbols PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(port_test_symbols PRIVATE -std=c11 -Wall -Wextra -Werror)
target_link_libraries(port_test_symbols PRIVATE ${CMAKE_DL_LIBS})
add_test(NAME app_symbols COMMAND port_test_symbols)

# The run directory's writer (port/app/run_dir.c), plain and under the sanitizers, with the platform layer's SHA-1
foreach(variant IN ITEMS plain sanitized)
    set(target port_test_run_dir_${variant})
    add_executable(${target} "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_run_dir.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/app/run_dir.c" "${CMAKE_CURRENT_SOURCE_DIR}/hal/rom/sha1.c" ${port_host_sources})
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(${target} PRIVATE -std=c11 -Wall -Wextra -Werror)
    if(variant STREQUAL "sanitized")
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME app_run_dir_${variant} COMMAND ${target})
endforeach()

# The gamepads (port/app/gamepad.c), plain and under the sanitizers: the mapping, the stick's dead zone and the pad that
# drives, then SDL's virtual joysticks as pads that come and go, so that no device is needed
foreach(variant IN ITEMS plain sanitized)
    set(target port_test_gamepad_${variant})
    add_executable(${target} "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_gamepad.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/app/gamepad.c")
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(${target} PRIVATE -std=c11 -Wall -Wextra -Werror)
    target_link_libraries(${target} PRIVATE SDL3::SDL3)
    if(variant STREQUAL "sanitized")
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME app_gamepad_${variant} COMMAND ${target})
    set_tests_properties(app_gamepad_${variant} PROPERTIES TIMEOUT 60)
endforeach()

# A replay from power-on through the title screen into the start menu (port/tests/replay/start_menu.pbwinput). It
# needs the player's ROM: without one the program exits 66 and the test is skipped. It takes about 40 s of CPU in an
# unoptimized build and a few in an optimized one; the limit is wall-clock time, which a loaded machine stretches
# A save would change the start menu, so the replay has a save directory of its own, emptied before each run
set(replay_start_menu_saves "${CMAKE_CURRENT_BINARY_DIR}/replay_start_menu_saves")
add_test(NAME replay_start_menu_clean COMMAND ${CMAKE_COMMAND} -E rm -rf "${replay_start_menu_saves}")
set_tests_properties(replay_start_menu_clean PROPERTIES FIXTURES_SETUP replay_start_menu_saves)
add_test(NAME replay_start_menu
    COMMAND ${PORT_PROGRAM} --headless --frames 4300 --watchdog 30 --save-dir "${replay_start_menu_saves}"
        --input "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay/start_menu.pbwinput")
set_tests_properties(replay_start_menu PROPERTIES FIXTURES_REQUIRED replay_start_menu_saves
    SKIP_RETURN_CODE 66 PASS_REGULAR_EXPRESSION "stopped after 4300 frames" TIMEOUT 900)
port_test_labels(replay_start_menu_clean route)
port_test_labels(replay_start_menu route)

# Black 2 only: the screens it checks are the oracle's hashes of Black 2's
if(PORT_VERSION STREQUAL "b2_us")
    # A replay from power-on through the start menu's NEW GAME and the intro (overlay 294) into the opening's 3D flight
    # over Aspertia City (port/tests/replay/new_game.pbwinput), up to just before the field: a test of level T3.
    # check_screens.py runs it with --run-dir and checks that it shows, each within a range of frames, nine screens
    # from the start menu to the opening's season banner exactly as the DS does, by the oracle's hashes of them
    # (port/tests/replay/new_game.screens.tsv), and that it runs all its frames. The intro takes the start menu's first
    # item, NEW GAME, only without a save, so the replay's save directory is emptied before each run. Without the
    # player's ROM the program exits 66 and the test is skipped. replay_new_game_house, below, goes on into the field
    set(replay_new_game_saves "${CMAKE_CURRENT_BINARY_DIR}/replay_new_game_saves")
    add_test(NAME replay_new_game_clean COMMAND ${CMAKE_COMMAND} -E rm -rf "${replay_new_game_saves}")
    set_tests_properties(replay_new_game_clean PROPERTIES FIXTURES_SETUP replay_new_game_saves)
    add_test(NAME replay_new_game
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay/check_screens.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay/new_game.screens.tsv" --
            $<TARGET_FILE:${PORT_PROGRAM}> --headless --frames 10600 --watchdog 30 --save-dir "${replay_new_game_saves}"
            --input "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay/new_game.pbwinput")
    set_tests_properties(replay_new_game PROPERTIES
        FIXTURES_REQUIRED replay_new_game_saves SKIP_RETURN_CODE 66 TIMEOUT 1800)
    port_test_labels(replay_new_game_clean route)
    port_test_labels(replay_new_game route)
endif()

# check_screens.py, the checker of the replays' screens (port/tests/replay/test_check_screens.py): its tables, the 3d
# column that leaves a screen with the field's 3D to the oracle's comparison, and whole runs of a stand-in program
add_test(NAME replay_check_screens
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay/test_check_screens.py")
set_tests_properties(replay_check_screens PROPERTIES TIMEOUT 120)

# New Game on into the field and the house (T3): the first save's input file (port/tests/oracle), which the oracle
# plays to make that save, from power-on through NEW GAME, the intro and the opening's flight into Aspertia City, the
# first scripts in front of the player's house and inside it, to the player's room and the field menu's SAVE, 18,800
# frames. It runs past frame 10,759, where the field's heap ran out before ADR 0021, and past the first change of map,
# into the house at frame 14,902, where the field's lens flare data divides with MSL's routine (port/hal/msl).
# check_screens.py holds it to the DS's screens (port/tests/replay/new_game_house_<version>.screens.tsv, the oracle's
# hashes): a screen without 3D bit for bit, and where the field's top screen shows 3D, which only the oracle's
# comparison holds, the bottom screen alone. Its own save directory, emptied before each run, so that the start menu
# offers NEW GAME. Without the player's ROM the program exits 66 and the test is skipped
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay/new_game_house_${PORT_VERSION}.screens.tsv")
    set(replay_new_game_house_saves "${CMAKE_CURRENT_BINARY_DIR}/replay_new_game_house_saves")
    add_test(NAME replay_new_game_house_clean COMMAND ${CMAKE_COMMAND} -E rm -rf "${replay_new_game_house_saves}")
    set_tests_properties(replay_new_game_house_clean PROPERTIES FIXTURES_SETUP replay_new_game_house_saves)
    add_test(NAME replay_new_game_house
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay/check_screens.py"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay/new_game_house_${PORT_VERSION}.screens.tsv" --
            $<TARGET_FILE:${PORT_PROGRAM}> --headless --frames 18800 --watchdog 30
            --save-dir "${replay_new_game_house_saves}"
            --input "${CMAKE_CURRENT_SOURCE_DIR}/tests/oracle/first_save_${PORT_VERSION}.pbwinput")
    set_tests_properties(replay_new_game_house PROPERTIES
        FIXTURES_REQUIRED replay_new_game_house_saves SKIP_RETURN_CODE 66 TIMEOUT 2400)
    port_test_labels(replay_new_game_house_clean route)
    port_test_labels(replay_new_game_house route)
endif()

# The port's saves depend only on the input file and the clock it names (T3, T6): New Game's formatting of the blank
# backup, done by frame 5600 as first_save_<version>.pbwinput starts it (port/tests/oracle), run twice, each run with a
# save directory of its own, writes the same save byte for byte (port/tests/replay/check_same_save.py). The console's
# pending Wi-Fi ID that New Game copies into both copies of the main save is the oracle's, which DWC makes from the
# clock and the DS's address of OS_GetTick: a host's address, which moves at every launch, once made each run's save
# different (port/include/port_host.h). Without the player's ROM the program exits 66 and the test is skipped
# The save file is named after the ROM's game code (port/hal/card/port_card.h); the ID's 8 bytes are the oracle's
if(PORT_VERSION STREQUAL "b2_us")
    set(replay_same_save_file IREO.sav)
    set(replay_same_save_wifi_id 280c0000f2a0b315)
else()
    set(replay_same_save_file IRDO.sav)
    set(replay_same_save_wifi_id 69080000f2a0b3ff)
endif()
add_test(NAME replay_same_save
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay/check_same_save.py"
        --expect "${replay_same_save_file}:0x1aa04:${replay_same_save_wifi_id}"
        --expect "${replay_same_save_file}:0x40a04:${replay_same_save_wifi_id}" --
        $<TARGET_FILE:${PORT_PROGRAM}> --headless --frames 5600 --watchdog 30
        --input "${CMAKE_CURRENT_SOURCE_DIR}/tests/oracle/first_save_${PORT_VERSION}.pbwinput")
set_tests_properties(replay_same_save PROPERTIES SKIP_RETURN_CODE 66 TIMEOUT 1200)
port_test_labels(replay_same_save route)
