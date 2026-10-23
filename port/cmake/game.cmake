# Checks of the decomp's own C as this build compiles it, with no game functionality of their own: they guard every
# part of the game that the C reaches. game_ptrcasts is the pointer cast guard (port/tools/check_ptrcasts.py): each
# src/ and lib/ command of compile_commands.json run again, syntax only, with the cast warnings the build silences
# turned on, and its findings held to port/tools/ptrcast_baseline.tsv. A cast the baseline doesn't list, one that
# would cut an address to 32 bits on a 64-bit host, fails it with the way out (a uptr or sptr, with its row in
# port/docs/game-changes.md, or a classified baseline row; port/docs/SYNC.md). It skips (77) without clang, without the
# compile database, and on Windows, whose LLP64 gives other findings than the baseline's macOS and Linux. Labelled
# static: it reads the code and runs nothing of the game
add_test(NAME game_ptrcasts
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${REPO_ROOT}/port/tools/check_ptrcasts.py" --build-dir "${CMAKE_BINARY_DIR}" --jobs 4)
set_tests_properties(game_ptrcasts PROPERTIES SKIP_RETURN_CODE 77 PROCESSORS 4 TIMEOUT 600)
port_test_labels(game_ptrcasts static)

# The guard's own test, on a small tree it makes in the build directory with this build's compiler: new findings, the
# baseline's counts and classes, renames, the narrowing and callback kinds, the headers' host variants, a file that
# doesn't compile (port/tests/game/test_check_ptrcasts.py). It skips without clang, and on Windows once it has seen the
# guard skip there
add_test(NAME game_ptrcasts_tool
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/game/test_check_ptrcasts.py" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_BINARY_DIR}/test_ptrcasts")
set_tests_properties(game_ptrcasts_tool PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
port_test_labels(game_ptrcasts_tool static)
