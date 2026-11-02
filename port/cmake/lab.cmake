# The save lab (port/tools/lab/README.md): savelab.py mints a save from a save the game wrote and a recipe in the game's
# own constant names, and reads a save back as facts. Its test, lab_savelab, needs neither the ROM nor the oracle, so it
# runs in every build and in CI: port/tests/lab/test_savelab.py builds synthetic saves from the layout that
# port_test_savelab_layout prints (the offsets and sizes of the decomp's save headers, compiled as the game's own files
# are, with port_decomp_env) and holds savelab's minter and reader to them. Game functionality: the saves (the player's
# place and the spawn CONTINUE takes, the event flags and work values, the play time, the money and badges, the party).
add_executable(port_test_savelab_layout tests/lab/savelab_layout.c)
target_link_libraries(port_test_savelab_layout PRIVATE port_decomp_env)
target_compile_options(port_test_savelab_layout PRIVATE -Wall -Wextra -Werror)
add_test(NAME lab_savelab
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
        "${PORT_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/lab/test_savelab.py"
            --layout "$<TARGET_FILE:port_test_savelab_layout>"
    WORKING_DIRECTORY "${REPO_ROOT}")
set_tests_properties(lab_savelab PROPERTIES TIMEOUT 300)
