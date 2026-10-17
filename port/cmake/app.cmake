# The program's own files besides main.c (port/app): the window, presentation, input and screenshots, and input replays
# with the oracle's input file reader (port/tools/oracle/pbwinput.c, plain C11). The screens they show are drawn by
# port/gfx/screens.c, which the port_gfx target builds.
target_sources(port_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/app/present.c" "${CMAKE_CURRENT_SOURCE_DIR}/app/replay.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/run_dir.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/tools/oracle/pbwinput.c")
# The version an input file's "version" line must name
target_compile_definitions(port_app PRIVATE "PORT_VERSION_NAME=\"${PORT_VERSION}\"")

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
# needs the player's ROM: without one the program exits 66 and the test is skipped
add_test(NAME replay_start_menu
    COMMAND ${PORT_PROGRAM} --headless --frames 4300 --watchdog 30
        --input "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay/start_menu.pbwinput")
set_tests_properties(replay_start_menu PROPERTIES
    SKIP_RETURN_CODE 66 PASS_REGULAR_EXPRESSION "stopped after 4300 frames" TIMEOUT 120)
