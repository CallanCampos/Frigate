# Unit tests of the decomp's libfx C as the host compiles it (port/tests/fx): the matrix products and vector transforms
# into their own inputs, and GFL's Euler rotation built on them. The test builds the decomp files it calls with the
# game's language model and port/hal's versions of the SDK's assembly they use, and needs no ROM.
set(port_fx_test_decomp_sources
    "${REPO_ROOT}/lib/nitro/src/fx/fx.c" "${REPO_ROOT}/lib/nitro/src/fx/fx_atanidx.c"
    "${REPO_ROOT}/lib/nitro/src/fx/fx_cp.c" "${REPO_ROOT}/lib/nitro/src/fx/fx_mtx22.c"
    "${REPO_ROOT}/lib/nitro/src/fx/fx_mtx33.c" "${REPO_ROOT}/lib/nitro/src/fx/fx_mtx43.c"
    "${REPO_ROOT}/lib/nitro/src/fx/fx_mtx44.c" "${REPO_ROOT}/lib/nitro/src/fx/fx_sincos.c"
    "${REPO_ROOT}/lib/nitro/src/fx/fx_vec.c" "${REPO_ROOT}/src/gfl/calctool.c")
file(GLOB port_fx_test_hal_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/hal/fx/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/hal/mi/*.c")
add_executable(port_fx_tests tests/fx/test_libfx.c ${port_fx_test_decomp_sources} ${port_fx_test_hal_sources}
    hal/hw/hw.c)
target_link_libraries(port_fx_tests PRIVATE port_decomp_env)
target_include_directories(port_fx_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
# MWCC's #pragma thumb in the decomp's files, which the game's own build silences too (port/CMakeLists.txt)
target_compile_options(port_fx_tests PRIVATE -Wno-unknown-pragmas)
add_test(NAME fx_libfx COMMAND port_fx_tests)
