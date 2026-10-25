# Checks of the decomp's own C as this build compiles it. The pointer cast guard and its test have no game
# functionality of their own: they guard every part of the game that the C reaches. game_script_vm runs one part of
# it, the field script VM's jumps and calls, on the host. game_ptrcasts is the pointer cast guard
# (port/tools/check_ptrcasts.py): each command of compile_commands.json that compiles the decomp's C (src/, lib/, and
# the decomp files port/hal's wrappers #include) run again, syntax only, with the cast warnings the build silences
# turned on, and its findings held to port/tools/ptrcast_baseline.tsv. A cast the baseline doesn't list, one that
# would cut an address to 32 bits on a 64-bit host, fails it with the way out (a uptr or sptr, with its row in
# port/docs/game-changes.md, or a classified baseline row; port/docs/SYNC.md). The guard skips (77) without clang,
# without the compile database, and on Windows, whose LLP64 gives other findings than the baseline's macOS and Linux.
# Where this build can run it (clang, a 64-bit target other than Windows, a generator that writes the compile
# database), --require makes each of those a failure, so that a guard that stopped running doesn't pass in CI as a
# skip. Labelled static: it reads the code and runs nothing of the game
set(port_ptrcasts_require)
if(CMAKE_C_COMPILER_ID MATCHES "Clang" AND CMAKE_SIZEOF_VOID_P EQUAL 8 AND NOT WIN32
   AND CMAKE_GENERATOR MATCHES "Ninja|Makefiles")
    set(port_ptrcasts_require --require)
endif()
add_test(NAME game_ptrcasts
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${REPO_ROOT}/port/tools/check_ptrcasts.py" --build-dir "${CMAKE_BINARY_DIR}" --jobs 4 ${port_ptrcasts_require})
set_tests_properties(game_ptrcasts PROPERTIES SKIP_RETURN_CODE 77 PROCESSORS 4 TIMEOUT 600)
port_test_labels(game_ptrcasts static)

# The guard's own test, on a small tree it makes in the build directory with this build's compiler and commands like
# this build's (port/tests/game/test_check_ptrcasts.py): new findings, the baseline's counts and classes, renames and
# what isn't one, the narrowing and callback kinds, the headers reached through -isystem and their host variants, a
# wrapper's decomp file, flags that hide the warnings, a file that doesn't compile, --require. It skips without clang,
# and on Windows once it has seen the guard skip there
add_test(NAME game_ptrcasts_tool
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/game/test_check_ptrcasts.py" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_BINARY_DIR}/test_ptrcasts")
set_tests_properties(game_ptrcasts_tool PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
port_test_labels(game_ptrcasts_tool static)

# The field script VM's jumps and calls as the host compiles them (port/tests/game/test_script_vm.c), on the whole
# program's objects as the program links them: VMJump, VMJumpIf, VMCall and VMCallIf of src/ov012/scrcmd_vm.c move the
# script's position by an offset read from the script, negative for a jump back, as a menu's loop takes (the Pokemon
# Center's PC: res/field/scripts/global_10090.s). Read as a u32 it moved a 64-bit position 4 GiB on and the PC's menu
# crashed when the PC Box closed; read as an s32 (port/docs/game-changes.md) it lands where the DS's does. Needs no ROM
add_executable(port_game_script_vm_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/game/test_script_vm.c"
    $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> ${port_gfx_objects} "${PORT_GEN}/stubs.c")
target_link_libraries(port_game_script_vm_tests PRIVATE port_decomp_env SDL3::SDL3)
target_include_directories(port_game_script_vm_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_game_script_vm_tests PRIVATE -std=gnu11 -Wall -Wextra -Wno-unused-parameter)
add_test(NAME game_script_vm COMMAND port_game_script_vm_tests)
set_tests_properties(game_script_vm PROPERTIES TIMEOUT 60)
