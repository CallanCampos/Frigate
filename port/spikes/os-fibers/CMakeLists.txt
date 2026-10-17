# Spike: the SDK's threads on fibers with the DS's scheduling (PLAN.md §4.5, "OS").
#
#   cmake -S port/spikes/os-fibers -B build/port-spikes/os-fibers && cmake --build build/port-spikes/os-fibers
#   ctest --test-dir build/port-spikes/os-fibers --output-on-failure
#   (add -DOS_FIBERS_SANITIZE=ON, in another build directory, for AddressSanitizer and UBSan)
#
# The decomp's own os_thread.c, os_mutex.c, os_message.c and os_event.c are compiled from their paths, read-only.
cmake_minimum_required(VERSION 3.24)
project(os_fibers_spike C)

option(OS_FIBERS_SANITIZE "Build everything with -fsanitize=address,undefined" OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "" FORCE)
endif()

get_filename_component(POKEBW2_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)

# minicoro, pinned to a commit (public domain or MIT-0, one header)
include(FetchContent)
FetchContent_Declare(minicoro
    GIT_REPOSITORY https://github.com/edubart/minicoro.git
    GIT_TAG 02dad0f8b7cbb12fe6e216ae7a76db15ca55cd7b)
FetchContent_MakeAvailable(minicoro)

if(OS_FIBERS_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# Stack probes, so that a frame larger than a page touches each page on its way down and faults in a fiber's guard
# instead of stepping over it (hal_fiber.c). Linux needs -fstack-clash-protection (upstream clang emits no probes
# without it). On macOS both Apple's and upstream clang's drivers drop that flag for Mach-O ("argument unused"), but
# their code generators honour it as a frontend flag; Apple clang's default ___chkstk_darwin probes are not enough,
# since they skip the frames that ASan realigns. The Windows compilers call __chkstk for frames over 4 or 8 KiB
if(WIN32)
    # probes by default
elseif(APPLE)
    add_compile_options("SHELL:-Xclang -fstack-clash-protection")
else()
    add_compile_options(-fstack-clash-protection)
endif()

# The SDK's files, with the language model PLAN.md §4.4 sets for game code: the decomp's include tree (its own libc
# headers), signed char, wrapping overflow, no strict aliasing, zeroed locals
add_library(sdk_os OBJECT
    src/os_thread_host.c
    ${POKEBW2_ROOT}/lib/nitro/src/os/os_mutex.c
    ${POKEBW2_ROOT}/lib/nitro/src/os/os_message.c
    ${POKEBW2_ROOT}/lib/nitro/src/os/os_event.c)
set_target_properties(sdk_os PROPERTIES C_STANDARD 99 C_EXTENSIONS ON)
target_include_directories(sdk_os PRIVATE
    ${POKEBW2_ROOT}/include ${POKEBW2_ROOT}/lib/nitro/include ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_compile_definitions(sdk_os PRIVATE BLACK2 PLATFORM_PC)
target_compile_options(sdk_os PRIVATE
    "SHELL:-iquote ${POKEBW2_ROOT}"
    -fsigned-char -fwrapv -fno-strict-aliasing -ffp-contract=off -fno-common -ftrivial-auto-var-init=zero
    -Wall
    # The 64-bit truncations in os.h's HW_DTCM and in os_thread.c's OS_InitThread, OS_CreateThread,
    # OSi_ExitThread_ArgSpecified and OSi_KillThreadWithPriority, which the README lists; none of them runs here.
    # OS_DisableScheduler's unset result at 0xffffffff nested locks is the SDK's own
    -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast -Wno-sometimes-uninitialized)

# The host side: the fibers and the OS layer, in one library with the SDK's objects. The SDK's headers are reached
# with -iquote so that the host's own libc headers stay first for angle includes
add_library(os_fibers STATIC src/hal_fiber.c src/mco_impl.c src/os_host.c $<TARGET_OBJECTS:sdk_os>)
if(OS_FIBERS_SANITIZE)
    # hal_fiber.c annotates the switches for ASan; minicoro's own annotations would contradict them
    set_source_files_properties(src/mco_impl.c PROPERTIES COMPILE_OPTIONS -fno-sanitize=address)
endif()
set_target_properties(os_fibers PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
target_include_directories(os_fibers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_include_directories(os_fibers SYSTEM PRIVATE ${minicoro_SOURCE_DIR})
target_compile_definitions(os_fibers PUBLIC BLACK2 PLATFORM_PC)
target_compile_options(os_fibers PUBLIC
    "SHELL:-iquote ${POKEBW2_ROOT}/include" "SHELL:-iquote ${POKEBW2_ROOT}/lib/nitro/include"
    "SHELL:-iquote ${POKEBW2_ROOT}/lib/twl/include" -fsigned-char -fwrapv
    -fno-strict-aliasing
    # os.h's inline OS_SetIrqCheckFlag truncates HW_DTCM; the host code here never calls it
    -Wno-pointer-to-int-cast -Wno-int-to-pointer-cast)
target_compile_options(os_fibers PRIVATE -Wall -Wextra -Wno-unused-parameter)

foreach(program test_os bench_os crash_os)
    add_executable(${program} tests/${program}.c)
    set_target_properties(${program} PROPERTIES C_STANDARD 11 C_EXTENSIONS ON)
    target_compile_options(${program} PRIVATE -Wall -Wextra -Wno-unused-parameter)
    target_link_libraries(${program} PRIVATE os_fibers)
endforeach()

enable_testing()
foreach(name wakeup_preempts yield_order queue_wake_order join_destructor mutex message_queue vblank_and_alarms
        irq_wakeup_deferred irq_while_disabled busy_wait_preempted scheduler_lock destroy_and_recreate kill_thread
        switch_callback hblank_lines hblank_main_loop dispstat_gates_blanks frame_hook_on_host_stack irq_on_switch_in
        async_completion deadlock determinism)
    add_test(NAME os_${name} COMMAND test_os ${name})
endforeach()
add_test(NAME bench_quick COMMAND bench_os --quick)

# Overflows that must fault in the guard: 1 KiB frames, frames larger than a page, and a frame larger than the stack
# and the guard together, which only the stack probes catch
add_test(NAME crash_guard_page COMMAND crash_os guard)
add_test(NAME crash_big_frames COMMAND crash_os bigframes)
add_test(NAME crash_frame_over_guard COMMAND crash_os hugeframe)
set_tests_properties(crash_guard_page crash_big_frames crash_frame_over_guard PROPERTIES
    PASS_REGULAR_EXPRESSION "fault in guard page" FAIL_REGULAR_EXPRESSION "fault outside|fault in the fiber's|never:")

# ASan's options are appended to the caller's (a later option wins), so that a run with detect_leaks=1 keeps it
if(OS_FIBERS_SANITIZE)
    # ASan names the frame only if it knows the fiber's stack; without the annotations it reports the same error
    # type at "a wild pointer". It aborts on Darwin by default, which CTest counts as a failure whatever the output
    add_test(NAME crash_asan_sees_fiber_stack COMMAND crash_os asan)
    set_tests_properties(crash_asan_sees_fiber_stack PROPERTIES
        PASS_REGULAR_EXPRESSION "is located in stack of thread" FAIL_REGULAR_EXPRESSION "wild pointer"
        ENVIRONMENT_MODIFICATION "ASAN_OPTIONS=string_append::abort_on_error=0")
    # Every scenario again with ASan's fake stacks, which move locals off the fiber's stack
    add_test(NAME os_all_fake_stacks COMMAND test_os all)
    set_tests_properties(os_all_fake_stacks PROPERTIES
        ENVIRONMENT_MODIFICATION "ASAN_OPTIONS=string_append::detect_stack_use_after_return=1")
    # LeakSanitizer, where the ASan runtime has it (not Apple clang's on macOS, nor Windows'): a real leak is still
    # reported after fiber switches, and a block that only a suspended fiber's stack holds is not
    if(NOT CMAKE_C_COMPILER_ID STREQUAL "AppleClang" AND NOT WIN32)
        add_test(NAME lsan_reports_leak_after_fibers COMMAND crash_os leak)
        set_tests_properties(lsan_reports_leak_after_fibers PROPERTIES
            PASS_REGULAR_EXPRESSION "LeakSanitizer: detected memory leaks"
            FAIL_REGULAR_EXPRESSION "stack pointer not in stack range|not supported"
            ENVIRONMENT_MODIFICATION "ASAN_OPTIONS=string_append::detect_leaks=1:abort_on_error=0")
        add_test(NAME lsan_scans_suspended_fibers COMMAND crash_os held)
        set_tests_properties(lsan_scans_suspended_fibers PROPERTIES
            PASS_REGULAR_EXPRESSION "held only by a suspended" FAIL_REGULAR_EXPRESSION "LeakSanitizer"
            ENVIRONMENT_MODIFICATION "ASAN_OPTIONS=string_append::detect_leaks=1:abort_on_error=0")
    endif()
endif()
