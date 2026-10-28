# The integration tests (port/tests/integrate), written in phase 1: the host DMA (VRAM and palette uploads, the 3D
# display lists), the OS allocator's whole addresses and the main arena (the game's heaps), the buttons and lid, the
# screens drawn by the scanline hook and the PNG screenshot, and the overlays' sections (with a probe file that
# port/cmake/overlay_sections.cmake compiles as an overlay's); and the field script's relative jumps, calls and actor
# movements, back and forward (script_offsets.c: the starter scene's offsets, which a 64-bit host once read 4 GiB
# forward); each run as the game's main() would on the whole program's objects; and the program's own option checks
# (tests/app/check_exit.py).
enable_testing()
add_executable(port_integrate_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/integrate/test_integrate.c" "${CMAKE_CURRENT_SOURCE_DIR}/app/present.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/gamepad.c" "${CMAKE_CURRENT_SOURCE_DIR}/app/window_icon.c"
    "${PORT_GEN}/window_icon_pixels.c" "${CMAKE_CURRENT_SOURCE_DIR}/app/run_dir.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/integrate/ovsec_probe.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/integrate/script_offsets.c"
    $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> ${port_gfx_objects} "${PORT_GEN}/stubs.c")
target_link_libraries(port_integrate_tests PRIVATE port_decomp_env SDL3::SDL3)
target_include_directories(port_integrate_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_integrate_tests PRIVATE -std=gnu11 -Wall -Wextra -Wno-unused-parameter)
if(port_os_stack_probes)
    target_compile_options(port_integrate_tests PRIVATE ${port_os_stack_probes})
endif()

foreach(scenario IN ITEMS dma_copies dma_async dma_gx_command os_alloc main_arena input screens overlay_sections
                         script_offsets)
    add_test(NAME integrate_${scenario} COMMAND port_integrate_tests ${scenario})
    set_tests_properties(integrate_${scenario} PROPERTIES TIMEOUT 60)
endforeach()

# The program refuses an option it doesn't know, with status 2, instead of running the game. check_exit.py checks the
# status, which a PASS_REGULAR_EXPRESSION would ignore and WILL_FAIL would take any failure for, a sanitizer's report
# in the sanitized build included (WILL_FAIL turns ctest's failure on a report into a pass, so port/CMakeLists.txt
# refuses it)
add_test(NAME integrate_unknown_option
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/check_exit.py" --status 2 --fresh-save-dir
            --expect "port: unknown option --no-such-option" --reject "entering the game's main"
            -- "$<TARGET_FILE:${PORT_PROGRAM}>" --headless --no-such-option)
set_tests_properties(integrate_unknown_option PROPERTIES TIMEOUT 30)

# The boot to the logos: 120 frames, where the Nintendo and The Pokémon Company screen shows, and a screenshot of it.
# Off by default, since the replay tests (port/cmake/app.cmake) and the oracle's opening sequence
# (port/cmake/oracle.cmake) run past the logos already; it needs the player's ROM, and skips (status 66) without one
option(PORT_TEST_BOOT_LOGOS "Test that the game boots to its logo screens" OFF)
if(PORT_TEST_BOOT_LOGOS)
    add_test(NAME integrate_boot_logos
        COMMAND ${PORT_PROGRAM} --headless --frames 120 --watchdog 20
                --save-dir "${CMAKE_CURRENT_BINARY_DIR}/boot_logos_saves"
                --screenshot "${CMAKE_CURRENT_BINARY_DIR}/boot_logos_120.png")
    set_tests_properties(integrate_boot_logos PROPERTIES TIMEOUT 120 SKIP_RETURN_CODE 66
        PASS_REGULAR_EXPRESSION "wrote the screens after frame 120" FAIL_REGULAR_EXPRESSION "has no C yet|watchdog")
    port_test_labels(integrate_boot_logos route)
endif()

# R13: compile the actual VM with its production ignorelist. Only command-table context erasure is excepted;
# incorrect native/verifier calls still fail. Apple clang's missing C function check explicitly skips negatives.
add_executable(port_vm_dispatch_tests tests/integrate/vm_dispatch.c "${REPO_ROOT}/src/system/vm.c")
target_link_libraries(port_vm_dispatch_tests PRIVATE port_decomp_env)
target_compile_options(port_vm_dispatch_tests PRIVATE -fsanitize=undefined -fno-sanitize-recover=all
    "-fsanitize-ignorelist=${CMAKE_CURRENT_SOURCE_DIR}/cmake/sanitizer_ignorelist.txt")
target_link_options(port_vm_dispatch_tests PRIVATE -fsanitize=undefined)
# The control must not inherit an accidentally broadened ignorelist: only the actual VM uses that list.
set_property(SOURCE tests/integrate/vm_dispatch.c APPEND PROPERTY COMPILE_OPTIONS -fno-sanitize-ignorelist)
foreach(mode IN ITEMS main extra)
    add_test(NAME integrate_vm_dispatch_${mode} COMMAND port_vm_dispatch_tests ${mode})
endforeach()
foreach(mode IN ITEMS native verifier)
    add_test(NAME integrate_vm_dispatch_${mode}
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/integrate/check_vm_dispatch.py"
            "$<TARGET_FILE:port_vm_dispatch_tests>" ${mode})
    set_tests_properties(integrate_vm_dispatch_${mode} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 30)
endforeach()

# R14: all 38 encounter creators through the actual table, using their real header signatures. No exemptions.
add_executable(port_encounter_dispatch_tests tests/integrate/encounter_dispatch.c
    "${REPO_ROOT}/src/ov036/enceff_data.c")
target_link_libraries(port_encounter_dispatch_tests PRIVATE port_decomp_env)
target_compile_options(port_encounter_dispatch_tests PRIVATE -fsanitize=undefined -fno-sanitize-recover=all
    -fno-sanitize-ignorelist)
target_link_options(port_encounter_dispatch_tests PRIVATE -fsanitize=undefined)
add_test(NAME integrate_encounter_dispatch COMMAND port_encounter_dispatch_tests)
set_tests_properties(integrate_encounter_dispatch PROPERTIES TIMEOUT 30)

# The archive seeker's host C after upstream's MWCC-only assembly exception, on bounded synthetic FS callbacks.
add_executable(port_archive_seek_tests tests/integrate/archive_seek.c)
target_link_libraries(port_archive_seek_tests PRIVATE port_decomp_env)
target_include_directories(port_archive_seek_tests PRIVATE "${REPO_ROOT}")
target_compile_options(port_archive_seek_tests PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=all)
target_link_options(port_archive_seek_tests PRIVATE -fsanitize=address,undefined)
add_test(NAME integrate_archive_seek COMMAND port_archive_seek_tests)
add_test(NAME integrate_archive_seek_assert
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/check_exit.py" --status 70
        --expect "ArchiveMoveImageTop fileCnt=2, datID=2" -- "$<TARGET_FILE:port_archive_seek_tests>" invalid)
set_tests_properties(integrate_archive_seek integrate_archive_seek_assert PROPERTIES TIMEOUT 30)
