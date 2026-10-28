# Tests of the platform layer's host versions of the SDK's asm functions (port/hal/mi, fx, math, crypto), of MSL's
# runtime division (port/hal/msl), of the heaps' host sizes and the root heaps' host memory (port/hal/gfl/heap_policy.c),
# of the random seeds an input file gives the game (port/hal/gfl/random_seed.c), of DWC's wide strings (port/hal/stubs),
# of GameSpy's absent bucket-set functions (port/hal/stubs/gbucket.c), and of the card's SPI bus (port/hal/card/card_spi.c):
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

# The heaps' host sizes and the host memory of the root heaps the game creates in its own (port/hal/gfl/heap_policy.c,
# ADR 0021): kept for the caller's memory, zeroed when its overlay loads again, poisoned between two heaps in the
# sanitized build; with the program's options given by the test
add_executable(test_hal_heap_policy tests/hal/test_heap_policy.c hal/gfl/heap_policy.c)
target_link_libraries(test_hal_heap_policy PRIVATE port_decomp_env)
target_include_directories(test_hal_heap_policy PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME hal_heap_policy COMMAND test_hal_heap_policy)

# The seeds of the game's random numbers (port/hal/gfl/random_seed.c): an input file's, taken in order in place of the
# ones buildSeed builds, then the game's own, each recorded for run.txt
add_executable(test_hal_random_seed tests/hal/test_random_seed.c hal/gfl/random_seed.c)
target_include_directories(test_hal_random_seed PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(test_hal_random_seed PRIVATE -std=c11 -Wall -Wextra -Werror)
add_test(NAME hal_random_seed COMMAND test_hal_random_seed)

# DWC authentication names use MSL's 16-bit code units, independently of the host libc's wchar_t ABI.
add_executable(test_hal_wstring tests/hal/test_wstring.c hal/stubs/wstring_host.c hal/stubs/extras_string.c)
target_link_libraries(test_hal_wstring PRIVATE port_decomp_env)
target_compile_options(test_hal_wstring PRIVATE -UNDEBUG)
add_test(NAME hal_wstring COMMAND test_hal_wstring)

# The GameSpy functions that gstats.c references but the ROM does not contain: each stops even on a NULL argument.
add_executable(test_hal_gbucket tests/hal/test_gbucket.c hal/stubs/gbucket.c)
target_link_libraries(test_hal_gbucket PRIVATE port_decomp_env)
target_include_directories(test_hal_gbucket PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(test_hal_gbucket PRIVATE -Wall -Wextra -Werror)
foreach(function IN ITEMS NewBucketSet FreeBucketSet DumpBucketSet)
    add_test(NAME hal_gbucket_${function}
        COMMAND "${PORT_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/hal/check_gbucket.py"
            "$<TARGET_FILE:test_hal_gbucket>" "${function}")
endforeach()

# R10: the actual NitroSDK string-length function must not read past the NUL's C object or a readable page.
foreach(variant IN ITEMS plain sanitized)
    set(target test_hal_std_string_${variant})
    add_executable(${target} tests/hal/test_std_string.c
        "${REPO_ROOT}/lib/nitro/src/std/std_string.c" hal/mi/mi_memory.c hal/hw/hw.c)
    target_link_libraries(${target} PRIVATE port_decomp_env)
    target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
    if(NOT WIN32)
        target_compile_definitions(${target} PRIVATE _DEFAULT_SOURCE)
    endif()
    if(variant STREQUAL "sanitized")
        target_compile_options(${target} PRIVATE
            -fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer)
        target_link_options(${target} PRIVATE -fsanitize=address,undefined)
    endif()
    add_test(NAME hal_std_string_${variant} COMMAND ${target})
endforeach()

# AuthService's reconstructed password logins reference RSA encryption absent from the DS link and ROM.
add_executable(test_hal_gscrypt tests/hal/test_gscrypt.c hal/stubs/gscrypt.c)
target_link_libraries(test_hal_gscrypt PRIVATE port_decomp_env)
target_include_directories(test_hal_gscrypt PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(test_hal_gscrypt PRIVATE -Wall -Wextra -Werror)
foreach(mode IN ITEMS null buffers)
    add_test(NAME hal_gscrypt_${mode}
        COMMAND "${PORT_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/hal/check_gscrypt.py"
            "$<TARGET_FILE:test_hal_gscrypt>" "${mode}")
    set_tests_properties(hal_gscrypt_${mode} PROPERTIES TIMEOUT 30)
endforeach()
