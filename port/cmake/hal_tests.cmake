# Tests of the platform layer's host versions of the SDK's asm functions (port/hal/mi, fx, math, crypto), of MSL's
# runtime division (port/hal/msl) and of the card's SPI bus (port/hal/card/card_spi.c):
# - C unit tests with known vectors, which need no ROM and run everywhere, CI included;
# - the differential tests of port/tests/asm_oracle, which run the ROM's own machine code in a test-only CPU emulator
#   and compare. They need the extracted ROM, build/port-venv and a build directory made with the Ninja generator,
#   whose build.ninja gives them the game's flags (port/tests/asm_oracle/README.md), and skip (77) without them.
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

# The card slot's SPI bus and its infrared chip (port/hal/card/card_spi.c) against melonDS's and GBATEK's behaviour,
# plain and under the sanitizers: plain C, without the decomp's headers
foreach(variant IN ITEMS plain sanitized)
    set(target test_hal_card_spi_${variant})
    add_executable(${target} tests/hal/test_card_spi.c hal/card/card_spi.c)
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    target_compile_options(${target} PRIVATE -std=c11 -Wall -Wextra -Werror)
    if(variant STREQUAL "sanitized")
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME hal_card_spi_${variant} COMMAND ${target})
endforeach()

# run_all.py itself exits 77 when the venv is missing, so any Python 3 can start it
set(asm_oracle_python "${REPO_ROOT}/build/port-venv/bin/python")
if(NOT EXISTS "${asm_oracle_python}")
    set(asm_oracle_python python3)
endif()
add_test(NAME asm_oracle
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "ASM_ORACLE_BUILD_DIR=${CMAKE_BINARY_DIR}"
        "${asm_oracle_python}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/asm_oracle/run_all.py" --version "${PORT_VERSION}"
    WORKING_DIRECTORY "${REPO_ROOT}")
set_tests_properties(asm_oracle PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 900)

# MSL's runtime division as the game's C calls it (port/hal/msl), against values worked out by hand from the ARM routine
add_executable(test_hal_msl tests/hal/test_msl.c hal/msl/fp_fastI.c)
target_link_libraries(test_hal_msl PRIVATE port_decomp_env)
add_test(NAME hal_msl COMMAND test_hal_msl)

# DWC authentication names use MSL's 16-bit code units, independently of the host libc's wchar_t ABI.
add_executable(test_hal_wstring tests/hal/test_wstring.c hal/stubs/wstring_host.c hal/stubs/extras_string.c)
target_link_libraries(test_hal_wstring PRIVATE port_decomp_env)
target_compile_options(test_hal_wstring PRIVATE -UNDEBUG)
add_test(NAME hal_wstring COMMAND test_hal_wstring)
