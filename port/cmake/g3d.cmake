# Tests of the geometry engine and the CPU 3D renderer (port/gfx/ge, port/gfx/ref/ds3d.c), none of which needs a ROM:
# the engine's unit tests, the renderer's scene tests and the program glue (port/gfx/ge/ge_hal.c) against fakes of the
# platform layer, each plain and under AddressSanitizer and UndefinedBehaviorSanitizer, and the timing at -O2. Each
# builds the sources it tests itself, so that each can have its own flags; the game program gets them through
# port_gfx, which builds every C file under gfx/.
set(port_g3d_ge "${CMAKE_CURRENT_SOURCE_DIR}/gfx/ge/ge.c")
set(port_g3d_ds3d "${CMAKE_CURRENT_SOURCE_DIR}/gfx/ref/ds3d.c")
set(port_g3d_hal "${CMAKE_CURRENT_SOURCE_DIR}/gfx/ge/ge_hal.c")
set(port_g3d_tests "${CMAKE_CURRENT_SOURCE_DIR}/tests/g3d")

enable_testing()

# port_g3d_test(name sources...): port_test_g3d_<name> as test g3d_<name>, and the same sanitized as
# g3d_<name>_sanitized
function(port_g3d_test name)
    add_executable(port_test_g3d_${name} ${ARGN})
    target_include_directories(port_test_g3d_${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(port_test_g3d_${name} PRIVATE -std=c11 -Wall -Wextra -Werror)
    add_test(NAME g3d_${name} COMMAND port_test_g3d_${name})

    add_executable(port_test_g3d_${name}_sanitized ${ARGN})
    target_include_directories(port_test_g3d_${name}_sanitized PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(port_test_g3d_${name}_sanitized PRIVATE
        -std=c11 -Wall -Wextra -Werror -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
    target_link_options(port_test_g3d_${name}_sanitized PRIVATE -fsanitize=address,undefined)
    add_test(NAME g3d_${name}_sanitized COMMAND port_test_g3d_${name}_sanitized)
endfunction()

port_g3d_test(ge "${port_g3d_tests}/test_ge.c" ${port_g3d_ge})
port_g3d_test(ds3d "${port_g3d_tests}/test_ds3d.c" ${port_g3d_ge} ${port_g3d_ds3d})
port_g3d_test(hal "${port_g3d_tests}/test_g3d_hal.c" ${port_g3d_ge} ${port_g3d_ds3d} ${port_g3d_hal})

# The timing, always at -O2 (a later -O wins). The bench is always built, so that it compiles on every host, but it is a
# test only with PORT_G3D_TIMING_GATE (or the 2D renderer's PORT_GFX_TIMING_GATE) on: wall-clock limits depend on the
# machine. Its argument is the limit in microseconds for the median frame of its "typical" scene, 2,000 textured
# triangles (bench_ds3d.c); run without an argument, it only prints the times
option(PORT_G3D_TIMING_GATE "Run the CPU 3D renderer's timing gate under ctest" OFF)
set(PORT_G3D_TIMING_LIMIT_US 5000 CACHE STRING "The 3D renderer's timing gate limit, in microseconds")
add_executable(port_bench_g3d "${port_g3d_tests}/bench_ds3d.c" ${port_g3d_ge} ${port_g3d_ds3d})
target_include_directories(port_bench_g3d PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(port_bench_g3d PRIVATE -std=c11 -Wall -Wextra -Werror -O2)
if(PORT_G3D_TIMING_GATE OR PORT_GFX_TIMING_GATE)
    add_test(NAME g3d_timing COMMAND port_bench_g3d ${PORT_G3D_TIMING_LIMIT_US})
    set_tests_properties(g3d_timing PROPERTIES LABELS timing)
endif()
