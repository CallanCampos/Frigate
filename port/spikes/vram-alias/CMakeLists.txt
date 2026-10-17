# The VRAM page aliasing spike (PLAN.md §4.5). Standalone:
#   cmake -S port/spikes/vram-alias -B build/port-spikes/vram-alias && cmake --build build/port-spikes/vram-alias
#   ctest --test-dir build/port-spikes/vram-alias --output-on-failure
# With -DVRAM_SANITIZE=ON everything builds under ASan and UBSan.
cmake_minimum_required(VERSION 3.20)
project(vram_alias_spike C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

option(VRAM_SANITIZE "Build with AddressSanitizer and UndefinedBehaviorSanitizer" OFF)

set(VRAM_WARNINGS -Wall -Wextra -Wpedantic -Werror)
add_compile_options(${VRAM_WARNINGS})
if(VRAM_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-sanitize-recover=undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    # memfd_create, MAP_ANON and sigsetjmp are outside strict C11
    add_compile_definitions(_GNU_SOURCE)
elseif(APPLE)
    add_compile_definitions(_DARWIN_C_SOURCE)
endif()

add_library(vram STATIC vram.c vram_layout.c vram_os.c)
target_include_directories(vram PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test_vram test_vram.c)
target_link_libraries(test_vram PRIVATE vram)

add_executable(bench_vram bench_vram.c)
target_link_libraries(bench_vram PRIVATE vram)

add_executable(region_check region_check.c)
target_link_libraries(region_check PRIVATE vram)

enable_testing()
add_test(NAME vram_layout COMMAND test_vram layout)
foreach(backend shm mach-entry mach-remap copy)
    add_test(NAME vram_${backend} COMMAND test_vram ${backend})
    add_test(NAME vram_${backend}_static COMMAND test_vram ${backend} static)
    # The views left mapped in .bss at exit: LeakSanitizer scans them on Linux under ASan
    add_test(NAME vram_${backend}_exit_live COMMAND test_vram ${backend} exit-live)
    # Exit code 77 means the backend doesn't exist on this host
    set_tests_properties(vram_${backend} vram_${backend}_static vram_${backend}_exit_live PROPERTIES SKIP_RETURN_CODE 77)
endforeach()
add_test(NAME vram_bench_quick COMMAND bench_vram 50)
set_tests_properties(vram_bench_quick PROPERTIES LABELS bench)

# The static-region design: its pointer table, and what a u32 does to a host address
add_test(NAME vram_region_check COMMAND region_check)

if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    set(src ${CMAKE_CURRENT_SOURCE_DIR})
    # gx_load3d.c's u16 tables of shifted addresses can't be built from host addresses: the error is the expected result
    add_test(NAME vram_sdk_u16_table
             COMMAND ${CMAKE_C_COMPILER} -std=c11 -ffreestanding -fsyntax-only -DSDK_U16_TABLE -I${src}
                     ${src}/region_check.c)
    set_tests_properties(vram_sdk_u16_table PROPERTIES PASS_REGULAR_EXPRESSION "not a compile-time constant")

    # The library and the static-region design compiled for Windows. No Windows SDK or MinGW sysroot is needed:
    # windows-stubs/ stands in for the few UCRT headers the library includes, without aligned_alloc, as the UCRT
    foreach(target x86_64-pc-windows-msvc aarch64-pc-windows-msvc x86_64-w64-windows-gnu)
        set(dir ${CMAKE_CURRENT_BINARY_DIR}/windows-${target})
        file(MAKE_DIRECTORY ${dir})
        add_test(NAME vram_windows_compile_${target}
                 COMMAND ${CMAKE_C_COMPILER} --target=${target} -std=c11 -ffreestanding ${VRAM_WARNINGS}
                         -isystem ${src}/windows-stubs -I${src} -c ${src}/vram.c ${src}/vram_layout.c ${src}/vram_os.c
                         ${src}/region_check.c
                 WORKING_DIRECTORY ${dir})
    endforeach()
endif()
