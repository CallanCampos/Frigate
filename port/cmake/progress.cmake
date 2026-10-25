# The port-readiness measurement (port/tools/progress, which STATUS.md's Tiers line and sync_upstream.sh --metrics
# come from): no game functionality of its own, it measures how much of the game is C. Its compile step,
# compile_check.py, compiles the decomp's C with the ROM's compilers through the repository's own compile probe
# (tools/decomp/compiler_probe.py, configure.py, tools/build/deadstrip.py), which upstream changes; its test
# (port/tests/progress/test_compile_check.py) holds it to them in this build's version: the build's compiler and flags
# for a game file, a game file with its own flags and two libraries with their own compilers, the functions and
# bytes of five complete files, one of them dead-stripped, against the ROM's, and MSL's assembly runtime's code. It
# skips (77) without the decomp's .venv (PORT_PYTHON is the system's then) and, after the settings, without wibo,
# sjiswrap or the compilers and assembler in tools/.
# Labelled static: it reads the game's C, with MWCC, and runs nothing of the game
add_test(NAME progress_compile_check
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/progress/test_compile_check.py" "${REPO_ROOT}" "${PORT_VERSION}")
set_tests_properties(progress_compile_check PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 300)
port_test_labels(progress_compile_check static)
