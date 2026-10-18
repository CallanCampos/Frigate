# The PXI bus and the host ARM7's services (port/hal/pxi, pm, tp, nvram, rtc, reset, ctrdg, snd7, mic): the per-file
# options of the decomp files they compile, and their tests (port/tests/pxi).

# The decomp files compiled through host files (pm.c, nvram.c, snd_interface.c): their own code under the platform
# layer's warnings. Their 64-bit casts are the ones the host files deal with (the pointers they note), and the
# game's own sources don't warn about them either
set(port_pxi_decomp_warnings
    "-Wno-unused-function;-Wno-sign-compare;-Wno-unused-parameter;-Wno-switch;-Wno-pointer-to-int-cast"
    "-Wno-int-to-pointer-cast;-Wno-void-pointer-to-int-cast;-Wno-int-to-void-pointer-cast")
set_source_files_properties(
    "${CMAKE_CURRENT_SOURCE_DIR}/hal/pm/pm_host.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/hal/nvram/nvram_host.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/hal/snd7/snd_interface_host.c"
    PROPERTIES COMPILE_OPTIONS "${port_pxi_decomp_warnings}")

# The tests: each scenario runs the decomp's ARM9 libraries (pm.c, tp.c, nvram.c, os_reset.c, ctrdg.c, the SND
# commands and NitroSystem's sound players) against the host's ARM7, as the game's main() would, on the whole
# program's objects, with a main() of their own. One scenario a process, so that the libraries' one-time starts
# (PM_Init and the like) begin from nothing in each
enable_testing()
add_executable(port_pxi_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/pxi/test_pxi.c"
    $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> $<TARGET_OBJECTS:port_launcher> ${port_gfx_objects}
    "${PORT_GEN}/stubs.c")
target_link_libraries(port_pxi_tests PRIVATE port_decomp_env SDL3::SDL3)
target_include_directories(port_pxi_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_pxi_tests PRIVATE -std=gnu11 -Wall -Wextra -Wno-unused-parameter)
if(port_os_stack_probes)
    target_compile_options(port_pxi_tests PRIVATE ${port_os_stack_probes})
endif()

set(port_pxi_scenarios
    bus_ready_tags bus_unknown_tag bus_reply_when_enabled bus_pointer
    pm_battery_backlight pm_amp pm_sleep pm_power_off
    tp_sampling tp_calibration tp_auto_sampling
    nvram_user_info nvram_read nvram_write
    rtc_fixed_time rtc_runs_with_clock rtc_async rtc_host_time
    reset_command ctrdg_empty mic_silence
    snd_init snd_sequence_ends snd_sequence_loops snd_sequence_tracks snd_sequence_variables
    snd_sequence_shifts snd_sequence_outside snd_invalidate snd_alarm snd_driver_info)
foreach(scenario IN LISTS port_pxi_scenarios)
    add_test(NAME pxi_${scenario} COMMAND port_pxi_tests ${scenario})
    set_tests_properties(pxi_${scenario} PROPERTIES TIMEOUT 60)
endforeach()

# An arena that shares low 32 bits with the static data stops the program with pointer.c's message
add_test(NAME pxi_bus_arena_overlap COMMAND port_pxi_tests bus_arena_overlap)
set_tests_properties(pxi_bus_arena_overlap PROPERTIES TIMEOUT 60
    PASS_REGULAR_EXPRESSION "port: pxi: the main memory arena .* low 32 bits of ones of the program's static data")
