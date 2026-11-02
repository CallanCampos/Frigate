# MSL, the DS's C library, on the host (port/hal/msl). The host's C library stands in for it (port/hal/replaced.txt);
# where the two differ in what the game does as it runs, the platform layer gives the game MSL's own. exit is one: the
# ROM calls it twice, both in GFL's heap check (GFL_HeapDumpOnFailure, src/gfl/heap.c), and MSL's only notes whether
# the program is to end in failure and returns, where the host's ends the program with status 1 and no message. So
# src/gfl/heap.c is compiled with port/hal/msl/exit_calls.h forced in, which sends its calls to MSL's own,
# port_msl_exit (port/hal/msl/exit_host.c), and the game goes on from the check as it does on the DS: a heap that runs
# out then stops it with OS_Terminate (status 71). port/tools/gen_stubs.py fails the build when another of the game's
# objects calls the host's exit; its source then goes in port_msl_exit_callers below. atexit differs too, and nothing
# here covers it: DWC's GameSpy core (lib/dwc/src/gscommon/gsCore.c, overlay 11, online play) registers its shutdown
# with it, which MSL's exit never runs, and which the host's C library runs as the program ends, after the game's last
# frame. The tests: msl_exit_heap_check, the check returning, and msl_exit_heap_full, an allocation with no room left
# stopping the game with status 71 and its "port:" line (tests/app/check_exit.py), both on the whole program's objects;
# and msl_exit_game_objects, gen_stubs.py's check on objects it compiles (tests/tools/test_gen_stubs.py). MSL's runtime
# division (port/hal/msl/fp_fastI.c) has its test, hal_msl, in hal_tests.cmake.
set(port_msl_exit_callers "${REPO_ROOT}/src/gfl/heap.c")
foreach(file IN LISTS port_msl_exit_callers)
    if(NOT "${file}" IN_LIST game_sources)
        message(FATAL_ERROR "port/cmake/msl.cmake: ${file}, which calls MSL's exit, is no longer among the game's "
            "sources")
    endif()
endforeach()
# A source's COMPILE_OPTIONS come after the target's -include of port_host.h, and clang-cl, the MSVC front end, spells
# a forced include /FI (overlay_sections.cmake)
if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(port_msl_exit_include "/FI${CMAKE_CURRENT_SOURCE_DIR}/hal/msl/exit_calls.h")
else()
    set(port_msl_exit_include -include "${CMAKE_CURRENT_SOURCE_DIR}/hal/msl/exit_calls.h")
endif()
set_property(SOURCE ${port_msl_exit_callers} APPEND PROPERTY COMPILE_OPTIONS ${port_msl_exit_include})

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
# gen_stubs.py's check, with the build's compiler and the options the build gives it (port/CMakeLists.txt): a game
# object that calls exit fails the generation, the same file with exit_calls.h first passes, and so does a port object
# that calls the host's exit
add_test(NAME msl_exit_game_objects
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/test_gen_stubs.py" --cc "${CMAKE_C_COMPILER}"
        --version "${PORT_VERSION}" --work "${CMAKE_CURRENT_BINARY_DIR}/test_gen_stubs" -- ${port_stubs_host_options})
set_tests_properties(msl_exit_game_objects PROPERTIES TIMEOUT 120)
