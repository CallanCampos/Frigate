# The save lab (port/tools/lab/README.md): savelab.py mints a save from a save the game wrote and a recipe in the game's
# own constant names, and reads a save back as facts. Its test, lab_savelab, needs neither the ROM nor the oracle, so it
# runs in every build and in CI: port/tests/lab/test_savelab.py builds synthetic saves from the layout that
# port_test_savelab_layout prints (the offsets and sizes of the decomp's save headers, compiled as the game's own files
# are, with port_decomp_env) and holds savelab's minter and reader to them. Game functionality: the saves (the player's
# place and the spawn CONTINUE takes, the event flags and work values, the play time, the money and badges, the party).
# lab_stations does the same for the stations (port/tools/lab/stations.py, ADR 0032): port/tests/lab/test_stations.py
# holds each station's files and spec, the checks of a station's minted and resulting saves on synthetic saves built
# the same way, and oracle.py's station and station-check commands with stand-ins for both programs, with a table of
# deliberate bugs it must catch. Game functionality: the start menu's saved game, the field's continue at a spawn and
# the field menu's SAVE. The stations' own tests, which need the oracle and its saves, are in port/cmake/oracle.cmake.
add_executable(port_test_savelab_layout tests/lab/savelab_layout.c)
target_link_libraries(port_test_savelab_layout PRIVATE port_decomp_env)
target_compile_options(port_test_savelab_layout PRIVATE -Wall -Wextra -Werror)
add_test(NAME lab_savelab
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
        "${PORT_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/lab/test_savelab.py"
            --layout "$<TARGET_FILE:port_test_savelab_layout>"
    WORKING_DIRECTORY "${REPO_ROOT}")
set_tests_properties(lab_savelab PROPERTIES TIMEOUT 300)
add_test(NAME lab_stations
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
        "${PORT_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/lab/test_stations.py"
            --layout "$<TARGET_FILE:port_test_savelab_layout>"
    WORKING_DIRECTORY "${REPO_ROOT}")
set_tests_properties(lab_stations PROPERTIES TIMEOUT 300)
