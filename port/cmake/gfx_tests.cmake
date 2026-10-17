# Tests of the CPU reference 2D renderer (port/gfx/ref) and display capture, none of which needs a ROM: the unit tests, the same tests
# under AddressSanitizer and UndefinedBehaviorSanitizer, and the renderer's timing at -O2. Each builds the renderer's
# sources itself, so that each can have its own flags; the game program gets them through port_gfx
set(port_gfx_ref_sources "${CMAKE_CURRENT_SOURCE_DIR}/gfx/ref/ds2d.c")

add_executable(port_test_gfx_ds2d "${CMAKE_CURRENT_SOURCE_DIR}/tests/gfx/test_ds2d.c" ${port_gfx_ref_sources})
target_include_directories(port_test_gfx_ds2d PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(port_test_gfx_ds2d PRIVATE -std=c11 -Wall -Wextra -Werror)
add_test(NAME gfx_ds2d COMMAND port_test_gfx_ds2d)

add_executable(port_test_gfx_ds2d_sanitized "${CMAKE_CURRENT_SOURCE_DIR}/tests/gfx/test_ds2d.c" ${port_gfx_ref_sources})
target_include_directories(port_test_gfx_ds2d_sanitized PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(port_test_gfx_ds2d_sanitized PRIVATE
    -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
target_link_options(port_test_gfx_ds2d_sanitized PRIVATE -fsanitize=address,undefined)
add_test(NAME gfx_ds2d_sanitized COMMAND port_test_gfx_ds2d_sanitized)

# Display capture (port/gfx/capture.c), with the platform layer's VRAM and register services stubbed by the test
foreach(variant IN ITEMS plain sanitized)
    set(target port_test_gfx_capture_${variant})
    add_executable(${target} "${CMAKE_CURRENT_SOURCE_DIR}/tests/gfx/test_capture.c"
        "${CMAKE_CURRENT_SOURCE_DIR}/gfx/capture.c")
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(${target} PRIVATE -std=c11 -Wall -Wextra -Werror)
    if(variant STREQUAL "sanitized")
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME gfx_capture_${variant} COMMAND ${target})
endforeach()

# The timing is always measured at -O2, whatever the configuration (a later -O wins). The bench is always built, so
# that it compiles on every host, but it is a test only with PORT_GFX_TIMING_GATE on: wall-clock limits depend on the
# machine, and CI's runners and a loaded developer machine are slower than the one the limits were set on. Its argument
# is the limit in microseconds: the median frame of the "typical" scene must stay below half of it and the best frame
# of the "max" scene below it (bench_ds2d.c); run without an argument, it only prints the times
option(PORT_GFX_TIMING_GATE "Run the CPU 2D renderer's timing gate under ctest" OFF)
set(PORT_GFX_TIMING_LIMIT_US 2000 CACHE STRING "The 2D renderer's timing gate limit, in microseconds")
add_executable(port_bench_gfx_ds2d "${CMAKE_CURRENT_SOURCE_DIR}/tests/gfx/bench_ds2d.c" ${port_gfx_ref_sources})
target_include_directories(port_bench_gfx_ds2d PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(port_bench_gfx_ds2d PRIVATE -std=c11 -Wall -Wextra -Werror -O2)
if(PORT_GFX_TIMING_GATE)
    add_test(NAME gfx_ds2d_timing COMMAND port_bench_gfx_ds2d ${PORT_GFX_TIMING_LIMIT_US})
    set_tests_properties(gfx_ds2d_timing PROPERTIES LABELS timing)
endif()
