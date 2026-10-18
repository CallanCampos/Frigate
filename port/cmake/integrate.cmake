# The integration tests (port/tests/integrate), written in phase 1: the host DMA (VRAM and palette uploads, the 3D
# display lists), the OS allocator's whole addresses and the main arena (the game's heaps), the buttons and lid, the
# screens drawn by the scanline hook and the PNG screenshot, each run as the game's main() would on the whole program's
# objects; and the program's own option checks.
enable_testing()
add_executable(port_integrate_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/integrate/test_integrate.c" "${CMAKE_CURRENT_SOURCE_DIR}/app/present.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/app/gamepad.c" "${CMAKE_CURRENT_SOURCE_DIR}/app/window_icon.c"
    "${PORT_GEN}/window_icon_pixels.c" "${CMAKE_CURRENT_SOURCE_DIR}/app/run_dir.c"
    $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> ${port_gfx_objects} "${PORT_GEN}/stubs.c")
target_link_libraries(port_integrate_tests PRIVATE port_decomp_env SDL3::SDL3)
target_include_directories(port_integrate_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_integrate_tests PRIVATE -std=gnu11 -Wall -Wextra -Wno-unused-parameter)
if(port_os_stack_probes)
    target_compile_options(port_integrate_tests PRIVATE ${port_os_stack_probes})
endif()

foreach(scenario IN ITEMS dma_copies dma_async dma_gx_command os_alloc main_arena input screens)
    add_test(NAME integrate_${scenario} COMMAND port_integrate_tests ${scenario})
    set_tests_properties(integrate_${scenario} PROPERTIES TIMEOUT 60)
endforeach()

# The program refuses an option it doesn't know, with status 2, instead of running the game
add_test(NAME integrate_unknown_option COMMAND ${PORT_PROGRAM} --headless --no-such-option)
set_tests_properties(integrate_unknown_option PROPERTIES WILL_FAIL TRUE TIMEOUT 30)

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
endif()
