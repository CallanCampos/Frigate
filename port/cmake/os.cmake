# The OS core's build rules (port/hal/os): minicoro for the fibers, the per-file options of its sources, the stack
# probes that keep the fibers' guard regions effective, and the OS tests (port/tests/os).
#
# Sanitizer runs use a build directory of their own, with the flags for every file:
#   cmake -S port -B build/port-os-asan -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
#         "-DCMAKE_C_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer" \
#         -DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address,undefined
# mco_impl.c always builds without ASan (fiber.c makes the switch annotations itself).

include(FetchContent)
# minicoro, pinned to a commit (public domain or MIT-0, one header); the spike's
FetchContent_Declare(minicoro
    GIT_REPOSITORY https://github.com/edubart/minicoro.git
    GIT_TAG 02dad0f8b7cbb12fe6e216ae7a76db15ca55cd7b)
FetchContent_MakeAvailable(minicoro)
target_include_directories(port_hal SYSTEM PRIVATE "${minicoro_SOURCE_DIR}")

set(port_os_dir "${CMAKE_CURRENT_SOURCE_DIR}/hal/os")
# minicoro's own ASan annotations miss the switches back to the host stack; fiber.c annotates instead. Its code is all
# in minicoro.h, a system header through port_hal's include path above, which keeps its warnings out. That would also
# keep out the macOS availability check (port/CMakeLists.txt, ADR 0015), so a build with a deployment target compiles
# minicoro's implementation as a header of its own: it gives no warning under port_hal's -Wall -Wextra
set(port_os_mco_options -fno-sanitize=address)
if(APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
    list(APPEND port_os_mco_options --no-system-header-prefix=minicoro.h)
endif()
set_source_files_properties("${port_os_dir}/mco_impl.c" PROPERTIES COMPILE_OPTIONS "${port_os_mco_options}")
# The decomp's os_thread.c, compiled through the host file: its four renamed DS functions still truncate pointers
# (they never run), and OS_DisableScheduler's unset result at 0xffffffff nested locks is the SDK's own
set_source_files_properties("${port_os_dir}/os_thread_host.c" PROPERTIES COMPILE_OPTIONS
    "-Wno-pointer-to-int-cast;-Wno-int-to-pointer-cast;-Wno-void-pointer-to-int-cast;-Wno-sometimes-uninitialized;-Wno-unused-parameter")

# Stack probes, so that a frame larger than a page touches each page on its way down and faults in a fiber's guard
# instead of stepping over it (port/hal/os/fiber.c). Everything that runs on a fiber needs them: the game and the
# platform layer. Linux needs -fstack-clash-protection; on macOS both Apple's and upstream clang's drivers drop that
# flag for Mach-O, but their code generators honour it as a frontend flag, and Apple clang's default probes skip the
# frames that ASan realigns. The Windows compilers call __chkstk for frames over 4 or 8 KiB
if(APPLE)
    set(port_os_stack_probes "SHELL:-Xclang -fstack-clash-protection")
elseif(NOT WIN32)
    set(port_os_stack_probes -fstack-clash-protection)
endif()
if(port_os_stack_probes)
    target_compile_options(pokebw2_game PRIVATE ${port_os_stack_probes})
    target_compile_options(port_hal PRIVATE ${port_os_stack_probes})
endif()

# The OS tests: scenarios of the DS's scheduling, interrupts and timing, each run as the game's main() would run, on
# the whole program's objects (the decomp's scheduler and the HAL as the game links them), with a main() of their own
enable_testing()
add_executable(port_os_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/os/test_os.c"
    $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> $<TARGET_OBJECTS:port_launcher> ${port_gfx_objects}
    "${PORT_GEN}/stubs.c")
target_link_libraries(port_os_tests PRIVATE port_decomp_env SDL3::SDL3)
target_include_directories(port_os_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_os_tests PRIVATE -std=gnu11 -Wall -Wextra -Wno-unused-parameter)
if(port_os_stack_probes)
    target_compile_options(port_os_tests PRIVATE ${port_os_stack_probes})
endif()

set(port_os_scenarios
    wakeup_preempts yield_order queue_wake_order join_destructor mutex message_queue vblank_and_alarms
    irq_wakeup_deferred irq_while_disabled busy_wait_preempted scheduler_lock destroy_and_recreate kill_thread
    switch_callback hblank_lines hblank_main_loop hblank_flag_in_handler dispstat_gates_blanks
    frame_hooks_on_host_stack timed_work irq_on_switch_in async_completion waits request_irq scanline_hook
    display_registers timers_and_tick stop_from_frame_hook arenas locks console deadlock determinism boot_cpsr
    destroy_many wait_vcount0 valarm_each_run)
foreach(scenario IN LISTS port_os_scenarios)
    add_test(NAME os_${scenario} COMMAND port_os_tests ${scenario})
    set_tests_properties(os_${scenario} PROPERTIES TIMEOUT 60)
endforeach()
# Every scenario again with every host stack cut to 64 KiB, a quarter of a thread's default, so that a scenario that
# needs the default sizes shows
add_test(NAME os_all_small_stacks COMMAND port_os_tests all)
set_tests_properties(os_all_small_stacks PROPERTIES TIMEOUT 120 ENVIRONMENT "PORT_OS_STACK_KIB=64")
# Overflows of a thread's host stack that must fault in its fiber's guard: 1 KiB frames, and one frame larger than the
# stack and the guard together, which only the stack probes catch
add_test(NAME os_guard_small_frames COMMAND port_os_tests guard_small_frames)
add_test(NAME os_guard_huge_frame COMMAND port_os_tests guard_huge_frame)
set_tests_properties(os_guard_small_frames os_guard_huge_frame PROPERTIES TIMEOUT 60
    PASS_REGULAR_EXPRESSION "fault in guard page" FAIL_REGULAR_EXPRESSION "fault outside|never:")
# In a sanitizer build, every scenario again with ASan's fake stacks, which move locals off the fibers' stacks
if(CMAKE_C_FLAGS MATCHES "-fsanitize=[^ ]*address")
    add_test(NAME os_all_fake_stacks COMMAND port_os_tests all)
    set_tests_properties(os_all_fake_stacks PROPERTIES TIMEOUT 120
        ENVIRONMENT_MODIFICATION "ASAN_OPTIONS=string_append::detect_stack_use_after_return=1")
endif()
