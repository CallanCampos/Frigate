# The host's sound: the ARM7's sound driver and sound unit (port/hal/snd7, which the platform layer's glob compiles)
# and the audio output (port/audio), with their tests (port/tests/snd).

# The audio output is the program's: SDL3's audio stream for the device, and the WAV dump
target_sources(port_app PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/audio/audio.c")

# The tests: each scenario on the whole program's objects with a main() of its own, one a process, as port/tests/pxi
# does, since the ARM9's sound libraries start once a process. The rom_ scenarios need the player's ROM (POKEBW2_ROM,
# else orig/baserom_<version>.nds) and skip (77) without it
enable_testing()
add_executable(port_snd_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/snd/test_snd.c" "${CMAKE_CURRENT_SOURCE_DIR}/audio/audio.c"
    $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> ${port_gfx_objects} "${PORT_GEN}/stubs.c")
target_link_libraries(port_snd_tests PRIVATE port_decomp_env SDL3::SDL3)
target_include_directories(port_snd_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_snd_tests PRIVATE -std=gnu11 -Wall -Wextra -Wno-unused-parameter)
if(port_os_stack_probes)
    target_compile_options(port_snd_tests PRIVATE ${port_os_stack_probes})
endif()

set(port_snd_scenarios
    adpcm_decoder adpcm_channel pcm8_loop short_sound fifo psg_duty timer_write noise mixer calc_timer alloc
    lock_stops seq_note seq_params seq_programs seq_envelope seq_tie seq_sweep seq_modulation seq_flow seq_variables
    seq_mute seq_outside seq_nothing player_controls driver_info trace_layout bank_host pxi_wave_out pxi_sequence
    pxi_wake_ups pxi_alarm_timing audio_dump)
foreach(scenario IN LISTS port_snd_scenarios)
    add_test(NAME snd_${scenario} COMMAND port_snd_tests ${scenario})
    set_tests_properties(snd_${scenario} PROPERTIES TIMEOUT 60 ENVIRONMENT "PORT_SND_TMP=${CMAKE_CURRENT_BINARY_DIR}")
endforeach()
foreach(scenario rom_sequences rom_every_sequence)
    add_test(NAME snd_${scenario} COMMAND port_snd_tests ${scenario})
    set_tests_properties(snd_${scenario} PROPERTIES TIMEOUT 1200 SKIP_RETURN_CODE 77)
endforeach()
