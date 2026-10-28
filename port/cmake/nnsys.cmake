# port/hal/nnsys's build rules: the NitroSystem files whose functions the HAL wraps or replaces are compiled with those
# functions renamed port_sdk_<name>, and the NitroSystem tests (port/tests/nnsys).
#
#   - lib/nnsys/src/fnd: every function that gives heap memory back (freeing a block, shrinking one, freeing a frame
#     heap's ends or back to a state, freeing a unit, destroying a heap). port/hal/nnsys/fnd_hooks.c gives the original
#     names wrappers that call the SDK's and then release the relocated trees in what it freed (port/hal/nnsys/reloc.h).
#     Also the expanded and frame heaps' creation and allocation, which the wrappers align for 64-bit structs, and
#     whose live heaps they list, for the check of a read the DS makes past a block (port_heap_ds_read_begin)
#   - lib/nnsys/src/g2d: every function that relocates a loaded file in place, and the public ones around them.
#     port/hal/nnsys/g2d_load.c replaces them with loaders that build host structs; the renamed originals stay unused.
#     The files' other functions (NNS_G2dFindBinaryBlock, NNS_G2dGetCellDataByIdx and the like) stay the decomp's
#
# A name the file no longer defines fails the link (a wrapper calls port_sdk_<name>, or two definitions meet); a new
# function of one of these files that frees memory or relocates a file needs a line here and a host version.
#
# In the sanitized build (PORT_SANITIZE, port/docs/sanitizers.md) FND's heap files are compiled without AddressSanitizer
# and fnd_hooks.c with PORT_POISON_HEAPS, so that ASan sees the blocks of the game's expanded and frame heaps.

set(port_nnsys_renamed_expheap NNS_FndCreateExpHeapEx NNS_FndAllocFromExpHeapEx
    NNS_FndDestroyExpHeap NNS_FndResizeForMBlockExpHeap NNS_FndFreeToExpHeap)
set(port_nnsys_renamed_frameheap NNS_FndCreateFrmHeapEx NNS_FndAllocFromFrmHeapEx
    NNS_FndDestroyFrmHeap NNS_FndFreeToFrmHeap NNS_FndFreeByStateToFrmHeap)
set(port_nnsys_renamed_unitheap NNS_FndFreeToUnitHeap)
set(port_nnsys_renamed_g2d_ncg
    NNS_G2dGetUnpackedCharacterData NNS_G2dUnpackNCG NNS_G2dGetUnpackedBGCharacterData NNS_G2dUnpackBGNCG)
set(port_nnsys_renamed_g2d_ncl
    NNS_G2dGetUnpackedPaletteData NNS_G2dGetUnpackedPaletteCompressInfo NNSi_G2dUnpackNCLCmpInfo NNS_G2dUnpackNCL)
set(port_nnsys_renamed_g2d_nce NNS_G2dGetUnpackedCellBank NNS_G2dUnpackNCE)
set(port_nnsys_renamed_g2d_nmc NNS_G2dGetUnpackedMultiCellBank NNS_G2dUnpackNMC)
set(port_nnsys_renamed_g2d_nan NNS_G2dGetUnpackedAnimBank NNS_G2dGetUnpackedMCAnimBank NNS_G2dUnpackNAN)
set(port_nnsys_renamed_g2d_nft NNSi_G2dGetUnpackedFont NNSi_G2dUnpackNFT)
set(port_nnsys_renamed_g2d_load NNSi_G2dUnpackUserExCellAttrBank)

function(port_nnsys_rename source)
    set(file "${REPO_ROOT}/${source}")
    if(NOT "${file}" IN_LIST game_sources)
        message(FATAL_ERROR "port/cmake/nnsys.cmake: ${source} is no longer among the game's sources")
    endif()
    set(definitions)
    foreach(name IN LISTS ARGN)
        list(APPEND definitions "${name}=port_sdk_${name}")
    endforeach()
    set_property(SOURCE "${file}" APPEND PROPERTY COMPILE_DEFINITIONS ${definitions})
endfunction()

port_nnsys_rename(lib/nnsys/src/fnd/expheap.c ${port_nnsys_renamed_expheap})
port_nnsys_rename(lib/nnsys/src/fnd/frameheap.c ${port_nnsys_renamed_frameheap})
port_nnsys_rename(lib/nnsys/src/fnd/unitheap.c ${port_nnsys_renamed_unitheap})
port_nnsys_rename(lib/nnsys/src/g2d/g2d_NCG_load.c ${port_nnsys_renamed_g2d_ncg})
port_nnsys_rename(lib/nnsys/src/g2d/g2d_NCL_load.c ${port_nnsys_renamed_g2d_ncl})
port_nnsys_rename(lib/nnsys/src/g2d/g2d_NCE_load.c ${port_nnsys_renamed_g2d_nce})
port_nnsys_rename(lib/nnsys/src/g2d/g2d_NMC_load.c ${port_nnsys_renamed_g2d_nmc})
port_nnsys_rename(lib/nnsys/src/g2d/g2d_NAN_load.c ${port_nnsys_renamed_g2d_nan})
port_nnsys_rename(lib/nnsys/src/g2d/g2d_NFT_load.c ${port_nnsys_renamed_g2d_nft})
port_nnsys_rename(lib/nnsys/src/g2d/g2d_Load.c ${port_nnsys_renamed_g2d_load})

# The sanitized build (PORT_SANITIZE): AddressSanitizer sees the blocks of the expanded and frame heaps, which
# port/hal/nnsys/fnd_hooks.c poisons and unpoisons as they are created, allocated and freed. FND's heaps are compiled
# without ASan, as a C library's malloc is, since they read and write their own heads and free lists in the poisoned
# memory between the blocks
if(PORT_SANITIZE)
    foreach(source IN ITEMS expheap.c frameheap.c unitheap.c heapcommon.c)
        set_property(SOURCE "${REPO_ROOT}/lib/nnsys/src/fnd/${source}" APPEND PROPERTY COMPILE_OPTIONS
            -fno-sanitize=address)
    endforeach()
    set_property(SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/hal/nnsys/fnd_hooks.c" APPEND PROPERTY COMPILE_DEFINITIONS
        PORT_POISON_HEAPS)
endif()

# The tests: the loaders on files made by the tests, the registry through FND's heaps, a read the DS makes past a heap
# block, and every G2D file of the player's ROM against a simulation of the DS's unpack (skipped without the ROM), run
# on the whole program's objects
enable_testing()
add_executable(port_nnsys_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/nnsys/test_nnsys.c" "${CMAKE_CURRENT_SOURCE_DIR}/tests/nnsys/test_g2d_rom.c"
    $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> ${port_gfx_objects} "${PORT_GEN}/stubs.c")
target_link_libraries(port_nnsys_tests PRIVATE port_decomp_env SDL3::SDL3)
target_include_directories(port_nnsys_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_nnsys_tests PRIVATE -std=gnu11 -Wall -Wextra -Wno-unused-parameter)

foreach(scenario IN ITEMS character palette cell_bank multicell_bank anim_bank anim_frames font font_nodes heaps reuse
                         refill malformed registry ds_reads)
    add_test(NAME nnsys_${scenario} COMMAND port_nnsys_tests ${scenario})
    set_tests_properties(nnsys_${scenario} PROPERTIES TIMEOUT 60)
endforeach()

# A query-only free-list fixture crosses a 4 GiB address boundary without allocating its synthetic payload.
add_executable(port_expheap_capacity_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/nnsys/test_expheap_capacity.c"
    $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> ${port_gfx_objects} "${PORT_GEN}/stubs.c")
target_link_libraries(port_expheap_capacity_tests PRIVATE port_decomp_env SDL3::SDL3)
target_include_directories(port_expheap_capacity_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_expheap_capacity_tests PRIVATE -std=gnu11 -Wall -Wextra -Wno-unused-parameter)
add_test(NAME nnsys_expheap_capacity COMMAND port_expheap_capacity_tests)
set_tests_properties(nnsys_expheap_capacity PROPERTIES TIMEOUT 10)

# A read the DS makes past a heap block (port_heap_ds_read_begin) that would leave the game's heaps and the main arena
# stops the program with its message (abort), in every build, rather than read, and in the sanitized build unpoison,
# memory that isn't the game's. check_exit.py checks how it ends
add_test(NAME nnsys_ds_read_outside
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/check_exit.py" --abort
            --expect "port: fnd_hooks: a read past a heap block, 0x2000 bytes from 0x[0-9a-fA-F]+,"
            --expect "reaches outside the game's heaps and the main arena"
            --reject "the read went on"
            -- $<TARGET_FILE:port_nnsys_tests> ds_read_outside)
set_tests_properties(nnsys_ds_read_outside PROPERTIES TIMEOUT 60)

add_test(NAME nnsys_g2d_rom_equivalence
    COMMAND port_nnsys_tests rom "${REPO_ROOT}/orig/baserom_${PORT_VERSION}.nds")
set_tests_properties(nnsys_g2d_rom_equivalence PROPERTIES TIMEOUT 600 SKIP_RETURN_CODE 77)

# The DS layouts of port/hal/nnsys/g2d_file.h against the decomp's structs as MWCC lays them out: compiled, not run,
# for i386 SysV, whose layouts are MWCC's (clang only: it is a cross compiler)
if(CMAKE_C_COMPILER_ID MATCHES "Clang")
    list(TRANSFORM library_includes PREPEND "-I" OUTPUT_VARIABLE port_nnsys_probe_includes)
    add_test(NAME nnsys_ds_layout
        COMMAND "${CMAKE_C_COMPILER}" -target i386-unknown-linux-gnu -fsyntax-only -std=gnu11 -fsigned-char
                -fshort-wchar "-I${CMAKE_CURRENT_SOURCE_DIR}" "-I${PORT_GEN}/include"
                "-I${REPO_ROOT}/build/include/generated" ${port_nnsys_probe_includes}
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/nnsys/ds_layout_probe.c")
    set_tests_properties(nnsys_ds_layout PROPERTIES TIMEOUT 60)
endif()

# R15: exercise the actual SDK NODEMIX and FIFO sender under ASan without rebuilding the whole game sanitized.
# Replace only these two cached objects with instrumented copies; all other SDK/HAL/GE behavior stays real.
add_executable(port_nodemix_tests tests/nnsys/test_nodemix.c "${REPO_ROOT}/lib/nnsys/src/g3d/sbc.c" hal/hw/hw.c
    "$<FILTER:$<TARGET_OBJECTS:pokebw2_game>,EXCLUDE,/sbc[.]c[.](o|obj)$>"
    "$<FILTER:$<TARGET_OBJECTS:port_hal>,EXCLUDE,/hw[.]c[.](o|obj)$>"
    ${port_gfx_objects} "${PORT_GEN}/stubs.c")
target_link_libraries(port_nodemix_tests PRIVATE port_decomp_env SDL3::SDL3)
target_include_directories(port_nodemix_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_nodemix_tests PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=all
    -fno-omit-frame-pointer)
target_link_options(port_nodemix_tests PRIVATE -fsanitize=address,undefined)
add_test(NAME nnsys_nodemix COMMAND port_nodemix_tests)
set_tests_properties(nnsys_nodemix PROPERTIES TIMEOUT 30)
