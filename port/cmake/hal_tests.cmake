# Tests of the platform layer's host versions of the SDK's asm functions and of the C that isn't the ROM's (port/hal/mi,
# fx, math, crypto):
# - C unit tests with known vectors, which need no ROM and run everywhere, CI included;
# - the differential tests of port/tests/asm_oracle, which run the ROM's own machine code in a test-only CPU emulator
#   and compare. They need the extracted ROM and build/port-venv (port/tests/asm_oracle/README.md), and skip (77)
#   without them.
file(GLOB hal_test_mi_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/hal/mi/*.c")
add_executable(test_hal_mi tests/hal/test_mi.c ${hal_test_mi_sources} hal/hw/hw.c)
target_link_libraries(test_hal_mi PRIVATE port_decomp_env)
target_include_directories(test_hal_mi PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME hal_mi COMMAND test_hal_mi)

file(GLOB hal_test_fx_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/hal/fx/*.c" "${CMAKE_CURRENT_SOURCE_DIR}/hal/math/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/hal/crypto/*.c")
add_executable(test_hal_fx tests/hal/test_fx.c ${hal_test_fx_sources} hal/hw/hw.c)
target_link_libraries(test_hal_fx PRIVATE port_decomp_env)
target_include_directories(test_hal_fx PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME hal_fx COMMAND test_hal_fx)

# The SDK functions in C that port/hal replaces because their decomp C isn't the ROM's (ADR 0008) are still nonmatching
# rows of incomplete files: when one matches, the host version goes
add_test(NAME hal_replaced_c
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/hal/check_replaced_c.py")

# run_all.py itself exits 77 when the venv is missing, so any Python 3 can start it
set(asm_oracle_python "${REPO_ROOT}/build/port-venv/bin/python")
if(NOT EXISTS "${asm_oracle_python}")
    set(asm_oracle_python python3)
endif()
add_test(NAME asm_oracle
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${asm_oracle_python}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/asm_oracle/run_all.py"
    WORKING_DIRECTORY "${REPO_ROOT}")
set_tests_properties(asm_oracle PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 900)
