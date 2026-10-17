# The program's own files besides main.c (port/app): the window, presentation, input, screenshots and the screens'
# hashes, and input replays with the oracle's input file reader (port/tools/oracle/pbwinput.c, plain C11). The screens
# they show are drawn by port/gfx/screens.c, which the port_gfx target builds.
target_sources(port_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app/present.c" "${CMAKE_CURRENT_SOURCE_DIR}/app/replay.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/oracle/pbwinput.c")
# The version an input file's "version" line must name
target_compile_definitions(port_app PRIVATE "PORT_VERSION_NAME=\"${PORT_VERSION}\"")

# A replay from power-on through the title screen into the start menu (port/tests/replay/start_menu.pbwinput). It
# needs the player's ROM: without one the program exits 66 and the test is skipped
add_test(NAME replay_start_menu
    COMMAND ${PORT_PROGRAM} --headless --frames 4300 --watchdog 30
        --input "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay/start_menu.pbwinput")
set_tests_properties(replay_start_menu PROPERTIES
    SKIP_RETURN_CODE 66 PASS_REGULAR_EXPRESSION "stopped after 4300 frames" TIMEOUT 120)

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
    FIXTURES_REQUIRED replay_new_game_saves SKIP_RETURN_CODE 66 TIMEOUT 600)
