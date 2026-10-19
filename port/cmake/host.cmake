# The host's system (port/hal/host), what differs between macOS, Linux and Windows: its tests (port/tests/host), on the
# whole program's objects as the program links them (port/hal/trap.c and the trap stubs' placeholders), each scenario in
# a process of its own: a trap placeholder touched (the game reaching data with no C yet), a crash reported, both again
# on a fiber with the C library's code making the access (as the game's threads run), a fiber's guard touched there, the
# crashes that aren't memory faults (a division by zero, an illegal instruction, a breakpoint) on the host's stack and
# on a fiber, and __builtin_trap on the host's stack, the watchdog going off, kicked and off, the program's static data
# (the PXI bus's pointers), files, a folder and an argument whose names aren't ASCII, the environment and private file
# mappings. None needs a ROM, so every CI job runs them.
enable_testing()
add_executable(port_host_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/host/test_host.c"
    $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> $<TARGET_OBJECTS:port_launcher> ${port_gfx_objects}
    "${PORT_GEN}/stubs.c")
target_link_libraries(port_host_tests PRIVATE port_decomp_env SDL3::SDL3)
target_include_directories(port_host_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_host_tests PRIVATE -std=gnu11 -Wall -Wextra -Wno-unused-parameter)

# A trap: the placeholder's name, a backtrace and status 70 (port/hal/trap.h), checked by tests/app/check_exit.py, since
# a PASS_REGULAR_EXPRESSION ignores the status. Skipped (77) once the game has C for all the data it references
set(port_host_check_exit "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/check_exit.py")
add_test(NAME host_trap_placeholder
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}" "${port_host_check_exit}"
        --status 70 --skip-status 77 --expect "port: the game accessed [A-Za-z_][A-Za-z0-9_]*\\+0x0 \\("
        --expect "which has no C yet" --expect "Backtrace:" -- $<TARGET_FILE:port_host_tests> trap_placeholder)
set_tests_properties(host_trap_placeholder PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
# On a fiber the C library's memset clears the placeholder's first 4 bytes, in its own order: the report names the
# byte it touched first, the first on Apple Silicon (the x86_64 slice under Rosetta too), Linux and Windows, the last
# (+0x3) on an Intel Mac
add_test(NAME host_trap_placeholder_fiber
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}" "${port_host_check_exit}"
        --status 70 --skip-status 77 --expect "port: the game accessed [A-Za-z_][A-Za-z0-9_]*\\+0x[0-3] \\("
        --expect "which has no C yet" --expect "Backtrace:" -- $<TARGET_FILE:port_host_tests> trap_placeholder_fiber)
set_tests_properties(host_trap_placeholder_fiber PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)

# A crash: the fault as the host reports it and a backtrace, then the fault ends the program, by a signal or with an
# exception's status as the host ends a crashed program, the one the report names (check_exit.py)
add_test(NAME host_crash_report
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}" "${port_host_check_exit}"
        --crash --expect "port: crash: (signal [0-9]+|exception 0x[0-9a-f]+) at address" --expect "Backtrace:"
        -- $<TARGET_FILE:port_host_tests> crash_report)
set_tests_properties(host_crash_report PROPERTIES TIMEOUT 60)

# The same on a fiber, with the C library's memset making the access: to no memory, and to the top of the fiber's own
# guard (on Windows a guard page, whose exception's status is a warning's, 0x80000001)
foreach(scenario IN ITEMS crash_report_fiber guard_fiber)
    add_test(NAME host_${scenario}
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}" "${port_host_check_exit}"
            --crash --expect "port: crash: (signal [0-9]+|exception 0x[0-9a-f]+) at address" --expect "Backtrace:"
            -- $<TARGET_FILE:port_host_tests> ${scenario})
    set_tests_properties(host_${scenario} PROPERTIES TIMEOUT 60)
endforeach()

# The crashes that aren't memory faults, a division by zero, an illegal instruction and a breakpoint on the host's stack
# and on a fiber, and __builtin_trap on the host's stack, reported the same way on every host (port/hal/host/host.h),
# and each ending the program by what its report names, which check_exit.py checks: on macOS and Linux, a division by
# zero is SIGFPE (8), an illegal instruction SIGILL (4) and a breakpoint SIGTRAP (5), and __builtin_trap is SIGILL on
# x86-64 and SIGTRAP on arm64 (a universal program runs either slice); on Windows they are exceptions 0xc0000094,
# 0xc000001d and 0x80000003, and __builtin_trap is an illegal instruction. arm64 divides by zero without a fault, so
# there the scenario raises SIGFPE itself (port/tests/host/test_host.c). On macOS the report also names the function
# that holds the instruction, "  in NAME+0x...", from the program's symbols: the scenario's own, or for arm64's division
# the system's __pthread_kill, where raise() stops the thread. Linux's dladdr reads only the symbols a program exports,
# and an ELF program exports none of its functions unless it links with --export-dynamic, which Frigate's don't;
# Windows' backtrace starts at the faulting instruction instead of naming it apart
if(WIN32)
    set(port_host_divide "exception 0xc0000094")
    set(port_host_illegal "exception 0xc000001d")
    set(port_host_breakpoint "exception 0x80000003")
    set(port_host_trap "exception 0xc000001d")
else()
    set(port_host_divide "signal 8")
    set(port_host_illegal "signal 4")
    set(port_host_breakpoint "signal 5")
    set(port_host_trap "signal (4|5)")
endif()
function(port_host_crash_test scenario report function)
    set(where)
    if(APPLE)
        set(where --expect "port: crash: ${report} at address 0x[0-9a-f]+\\n  in (${function})\\+0x[0-9a-f]+\\n")
    endif()
    add_test(NAME host_${scenario}
        COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}" "${port_host_check_exit}"
            --crash --expect "port: crash: ${report} at address" ${where} --expect "Backtrace:"
            -- $<TARGET_FILE:port_host_tests> ${scenario})
    set_tests_properties(host_${scenario} PROPERTIES TIMEOUT 60)
endfunction()
port_host_crash_test(divide_by_zero "${port_host_divide}" "DivideByZero|__pthread_kill")
port_host_crash_test(divide_by_zero_fiber "${port_host_divide}" "DivideByZero|__pthread_kill")
port_host_crash_test(illegal_instruction "${port_host_illegal}" IllegalInstruction)
port_host_crash_test(illegal_instruction_fiber "${port_host_illegal}" IllegalInstruction)
port_host_crash_test(breakpoint "${port_host_breakpoint}" Breakpoint)
port_host_crash_test(breakpoint_fiber "${port_host_breakpoint}" Breakpoint)
port_host_crash_test(trap "${port_host_trap}" Trap)

# The watchdog: its message and its status after a second of spinning; and no report while it is kicked
add_test(NAME host_watchdog
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}" "${port_host_check_exit}"
        --status 73 --expect "host test: the watchdog went off" -- $<TARGET_FILE:port_host_tests> watchdog)
add_test(NAME host_watchdog_kicked COMMAND port_host_tests watchdog_kicked)
set_tests_properties(host_watchdog PROPERTIES TIMEOUT 60)
set_tests_properties(host_watchdog_kicked PROPERTIES TIMEOUT 60
    PASS_REGULAR_EXPRESSION "watchdog_kicked: ok" FAIL_REGULAR_EXPRESSION "went off")
# 0 seconds turns it off, on every host
add_test(NAME host_watchdog_off COMMAND port_host_tests watchdog_off)
set_tests_properties(host_watchdog_off PROPERTIES TIMEOUT 60
    PASS_REGULAR_EXPRESSION "watchdog_off: ok" FAIL_REGULAR_EXPRESSION "went off")

foreach(scenario IN ITEMS static_data files)
    add_test(NAME host_${scenario} COMMAND port_host_tests ${scenario})
    set_tests_properties(host_${scenario} PROPERTIES TIMEOUT 60)
endforeach()
# An argument that isn't ASCII, as a path given on the command line may be: "Pokémon"
add_test(NAME host_utf8_argument COMMAND port_host_tests utf8_argument "Pokémon")
set_tests_properties(host_utf8_argument PROPERTIES TIMEOUT 60)
