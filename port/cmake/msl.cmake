# MSL, the DS's C library, on the host (port/hal/msl). The host's C library stands in for it (port/hal/replaced.txt)
# wherever the two behave alike for the game; where they don't, the platform layer gives the game MSL's own. exit is
# one: the ROM calls it twice, both in GFL's heap check (GFL_HeapDumpOnFailure, src/gfl/heap.c), and MSL's only notes
# whether the program is to end in failure and returns, where the host's ends the program with status 1 and no message.
# So src/gfl/heap.c is compiled with exit renamed to MSL's, port_msl_exit (port/hal/msl/msl_exit.c), and the game goes
# on from the check as it does on the DS: a heap that runs out then stops it with OS_Terminate (status 71). The tests
# run the check on the whole program's objects: msl_exit_heap_check, the check returning, and msl_exit_heap_full, an
# allocation with no room left stopping the game with status 71 and its "port:" line (tests/app/check_exit.py). MSL's
# runtime division (port/hal/msl/fp_fastI.c) has its test, hal_msl, in hal_tests.cmake.
set(port_msl_exit_callers "${REPO_ROOT}/src/gfl/heap.c")
foreach(file IN LISTS port_msl_exit_callers)
    if(NOT "${file}" IN_LIST game_sources)
        message(FATAL_ERROR "port/cmake/msl.cmake: ${file}, which calls MSL's exit, is no longer among the game's "
            "sources")
    endif()
endforeach()
set_property(SOURCE ${port_msl_exit_callers} APPEND PROPERTY COMPILE_DEFINITIONS exit=port_msl_exit)

enable_testing()
add_executable(port_msl_exit_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/hal/test_msl_exit.c"
    $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> ${port_gfx_objects} "${PORT_GEN}/stubs.c")
target_link_libraries(port_msl_exit_tests PRIVATE port_decomp_env SDL3::SDL3)
target_include_directories(port_msl_exit_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_msl_exit_tests PRIVATE -std=gnu11 -Wall -Wextra -Wno-unused-parameter)
add_test(NAME msl_exit_heap_check COMMAND port_msl_exit_tests heap_check)
set_tests_properties(msl_exit_heap_check PROPERTIES TIMEOUT 60)
add_test(NAME msl_exit_heap_full
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/check_exit.py" --status 71
        --expect "port: the game stopped itself \\(OS_Terminate\\)" -- $<TARGET_FILE:port_msl_exit_tests> heap_full)
set_tests_properties(msl_exit_heap_full PROPERTIES TIMEOUT 60)
