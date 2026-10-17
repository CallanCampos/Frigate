# The program's own files besides main.c (port/app): the window, presentation, input, screenshots and the screens'
# hashes, and input replays with the oracle's input file reader (port/tools/oracle/pbwinput.c, plain C11). The screens
# they show are drawn by port/gfx/screens.c, which the port_gfx target builds.
target_sources(port_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app/present.c" "${CMAKE_CURRENT_SOURCE_DIR}/app/replay.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/run_dir.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/oracle/pbwinput.c")
# The version an input file's "version" line must name
target_compile_definitions(port_app PRIVATE "PORT_VERSION_NAME=\"${PORT_VERSION}\"")
# --run-dump looks the game's globals up by name with dlsym: a Mach-O program exports them as it is, an ELF one only
# when it links with --export-dynamic, which ENABLE_EXPORTS gives
set_target_properties(${PORT_PROGRAM} PROPERTIES ENABLE_EXPORTS ON)
target_link_libraries(${PORT_PROGRAM} PRIVATE ${CMAKE_DL_LIBS})

# The run directory's writer (port/app/run_dir.c), plain and under the sanitizers, with the platform layer's SHA-1
foreach(variant IN ITEMS plain sanitized)
    set(target port_test_run_dir_${variant})
    add_executable(${target} "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_run_dir.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/app/run_dir.c" "${CMAKE_CURRENT_SOURCE_DIR}/hal/rom/sha1.c")
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(${target} PRIVATE -std=c11 -Wall -Wextra -Werror)
    if(variant STREQUAL "sanitized")
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME app_run_dir_${variant} COMMAND ${target})
endforeach()

# A replay from power-on through the title screen into the start menu (port/tests/replay/start_menu.pbwinput). It
# needs the player's ROM: without one the program exits 66 and the test is skipped. It takes about 40 s of CPU in an
# unoptimized build and a few in an optimized one; the limit is wall-clock time, which a loaded machine stretches
add_test(NAME replay_start_menu
    COMMAND ${PORT_PROGRAM} --headless --frames 4300 --watchdog 30
        --input "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay/start_menu.pbwinput")
set_tests_properties(replay_start_menu PROPERTIES
    SKIP_RETURN_CODE 66 PASS_REGULAR_EXPRESSION "stopped after 4300 frames" TIMEOUT 900)

# A replay from power-on through the start menu's NEW GAME and the intro (overlay 294) into the opening's 3D flight
# over Aspertia City (port/tests/replay/new_game.pbwinput), up to just before the field's first script, whose tables
# have no C yet: a test of level T3. check_screens.py runs it with --screen-hashes and checks that it shows, each within
# a range of frames, nine screens from the start menu to the opening's season banner exactly as the DS does, by the
# oracle's hashes of them (port/tests/replay/new_game.screens.tsv), and that it runs all its frames. The intro takes
# the start menu's first item, NEW GAME, only without a save, so the replay's save directory is emptied before each
# run. Without the player's ROM the program exits 66 and the test is skipped
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
