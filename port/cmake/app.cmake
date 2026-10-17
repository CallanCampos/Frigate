# The program's own files besides main.c (port/app): the window, presentation, input and screenshots, and input replays
# with the oracle's input file reader (port/tools/oracle/pbwinput.c, plain C11). The screens they show are drawn by
# port/gfx/screens.c, which the port_gfx target builds.
target_sources(port_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app/present.c" "${CMAKE_CURRENT_SOURCE_DIR}/app/replay.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/oracle/pbwinput.c")
# The version an input file's "version" line must name
target_compile_definitions(port_app PRIVATE "PORT_VERSION_NAME=\"${PORT_VERSION}\"")

# A replay from power-on through the title screen into the start menu (port/tests/replay/start_menu.pbwinput). It
# needs the player's ROM: without one the program exits 66 and the test is skipped. It takes about 40 s of CPU in an
# unoptimized build and a few in an optimized one; the limit is wall-clock time, which a loaded machine stretches
add_test(NAME replay_start_menu
    COMMAND ${PORT_PROGRAM} --headless --frames 4300 --watchdog 30
        --input "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay/start_menu.pbwinput")
set_tests_properties(replay_start_menu PROPERTIES
    SKIP_RETURN_CODE 66 PASS_REGULAR_EXPRESSION "stopped after 4300 frames" TIMEOUT 900)
