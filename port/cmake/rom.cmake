# The ROM, the game card and the file system (port/hal/rom, port/hal/card, port/hal/fs), behind every file the game
# reads, every overlay it loads and every save, and the card's infrared chip that the game's genuine-card check asks
# (port/hal/card/card_spi.c): the manifest of the ROM the program is built for, the per-file options of their sources,
# and their tests (port/tests/rom).

set(port_rom_dir "${CMAKE_CURRENT_SOURCE_DIR}/hal/rom")
set(port_card_dir "${CMAKE_CURRENT_SOURCE_DIR}/hal/card")
set(port_fs_dir "${CMAKE_CURRENT_SOURCE_DIR}/hal/fs")

# The manifest (port/hal/rom/rom_manifest.h), written into the build, hashes only. For the decomp's own ROM
# (PORT_ROM_LANGUAGE us), from the version's SHA-1 (<version>.sha1, as the ROM build checks it) and the NitroFS manifest
# of port/tools/research/assets; for another language's dump of the same version, from the entry that
# port/tools/research/assets/rom_manifest.py wrote from that dump into nitrofs_manifest_languages.json there, whose
# files the US code then reads (port/docs/research/es-rom-support.md)
set(port_rom_manifest_json "${REPO_ROOT}/port/tools/research/assets/nitrofs_manifest.json")
set(port_rom_sha1_file "${REPO_ROOT}/${PORT_VERSION}.sha1")
set(port_rom_language_json "${REPO_ROOT}/port/tools/research/assets/nitrofs_manifest_languages.json")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${port_rom_manifest_json}" "${port_rom_sha1_file}")

# A hexadecimal SHA-1 as the C initializer of its 20 bytes
function(port_rom_sha1_bytes out hex)
    string(TOLOWER "${hex}" hex)
    if(NOT hex MATCHES "^[0-9a-f]+$")
        message(FATAL_ERROR "not a SHA-1: ${hex}")
    endif()
    string(LENGTH "${hex}" length)
    if(NOT length EQUAL 40)
        message(FATAL_ERROR "not a SHA-1: ${hex}")
    endif()
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " bytes "${hex}")
    string(REGEX REPLACE ", $" "" bytes "${bytes}")
    set(${out} "${bytes}" PARENT_SCOPE)
endfunction()

# The ARM9 overlay table (the header's 0x50 and 0x54), which a dump that isn't byte for byte the manifest's must have as
# well: FS_LoadOverlayInfo reads every overlay's header from it, and FS_LoadOverlayImage checks the overlay's file
# against it (port/hal/fs/fs_overlay.c). Its size is 32 bytes for each overlay of config/<version>/arm9/overlays, which
# another language's ROM has as many of; its SHA-1, a hash only, is the dump's. The unit test changed_program confirms
# it on the player's dump
file(GLOB port_rom_overlay_dirs LIST_DIRECTORIES true
    "${REPO_ROOT}/config/${PORT_VERSION}/arm9/overlays/ov[0-9][0-9][0-9]")
list(LENGTH port_rom_overlay_dirs port_rom_overlay_count)
math(EXPR port_rom_overlay_table_size "${port_rom_overlay_count} * 32")

if(PORT_ROM_LANGUAGE STREQUAL "us")
    set(port_rom_manifest_version "${PORT_VERSION}")
    set(port_rom_manifest_sources "${PORT_VERSION}.sha1, port/tools/research/assets/nitrofs_manifest.json and
// port/cmake/rom.cmake's overlay table hashes")
    file(READ "${port_rom_sha1_file}" port_rom_sha1_line)
    string(REGEX MATCH "^[0-9a-fA-F]+" port_rom_sha1 "${port_rom_sha1_line}")
    file(READ "${port_rom_manifest_json}" port_rom_json)
    string(JSON port_rom_files GET "${port_rom_json}" "${PORT_VERSION}" files)
    string(JSON port_rom_game_code GET "${port_rom_json}" "${PORT_VERSION}" gamecode)
    string(JSON port_rom_manifest_sha1 GET "${port_rom_json}" "${PORT_VERSION}" rom_sha1)
    if(NOT port_rom_manifest_sha1 STREQUAL port_rom_sha1)
        message(FATAL_ERROR "${port_rom_sha1_file} and ${port_rom_manifest_json} disagree on the ROM's SHA-1")
    endif()
    # The decomp's dumps' overlay tables
    if(port_rom_sha1 STREQUAL "e51e6dfb8678a3d19dcd2a10691b96a569ca0abb")
        set(port_rom_overlay_table_sha1 2c34526168ddacaa5bdc95349cdada2451aa9f67)
    elseif(port_rom_sha1 STREQUAL "b5d7490be7b415b8f1e672a53e978a9cc667e56a")
        set(port_rom_overlay_table_sha1 cdc8792cb926b71f6182ff777b90d6eae3378fb7)
    else()
        message(FATAL_ERROR "port/cmake/rom.cmake has no overlay table hash for the ROM ${port_rom_sha1}")
    endif()
else()
    if(NOT PORT_ROM_LANGUAGE STREQUAL "es" OR NOT PORT_VERSION STREQUAL "b2_us")
        message(FATAL_ERROR "experimental es requires b2_us; Spanish White 2 is not validated")
    endif()
    set(port_rom_manifest_version "b2_es")
    set(port_rom_manifest_sources
        "port/tools/research/assets/nitrofs_manifest_languages.json's ${port_rom_manifest_version}")
    string(CONCAT port_rom_language_help "write it from your ROM with: "
        "python3 port/tools/research/assets/rom_manifest.py ROM.nds ${port_rom_language_json}")
    if(NOT EXISTS "${port_rom_language_json}")
        message(FATAL_ERROR "PORT_ROM_LANGUAGE ${PORT_ROM_LANGUAGE} needs ${port_rom_language_json}; "
            "${port_rom_language_help}")
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${port_rom_language_json}"
        "${REPO_ROOT}/port/tools/research/assets/rom_manifest.py")
    # Validate schema and safe C-string paths before embedding a player's manifest in generated C.
    execute_process(COMMAND "${PORT_PYTHON}" "${REPO_ROOT}/port/tools/research/assets/rom_manifest.py"
        --validate-manifest "${port_rom_language_json}" --overlay-count "${port_rom_overlay_count}"
        RESULT_VARIABLE port_rom_validation ERROR_VARIABLE port_rom_validation_error)
    if(NOT port_rom_validation EQUAL 0)
        message(FATAL_ERROR "Invalid experimental IRES manifest: ${port_rom_validation_error}")
    endif()
    file(READ "${port_rom_language_json}" port_rom_json)
    string(JSON port_rom_entry ERROR_VARIABLE port_rom_entry_error
        GET "${port_rom_json}" "${port_rom_manifest_version}")
    if(port_rom_entry_error)
        message(FATAL_ERROR "${port_rom_language_json} has no ${port_rom_manifest_version}; ${port_rom_language_help}")
    endif()
    string(JSON port_rom_files GET "${port_rom_entry}" files)
    string(JSON port_rom_game_code GET "${port_rom_entry}" gamecode)
    string(JSON port_rom_sha1 GET "${port_rom_entry}" rom_sha1)
    if(NOT port_rom_game_code STREQUAL "IRES" OR NOT PORT_VERSION STREQUAL "b2_us")
        message(FATAL_ERROR "experimental es requires b2_us and an IRES manifest")
    endif()
    string(JSON port_rom_language_table_size GET "${port_rom_entry}" overlay_table size)
    string(JSON port_rom_overlay_table_sha1 GET "${port_rom_entry}" overlay_table sha1)
    # The US code loads the overlays it knows, by number: the ROM must have as many
    if(NOT port_rom_language_table_size EQUAL port_rom_overlay_table_size)
        message(FATAL_ERROR "the ${port_rom_manifest_version} ROM's overlay table has ${port_rom_language_table_size} "
            "bytes, not the ${port_rom_overlay_table_size} of config/${PORT_VERSION}'s overlays")
    endif()
endif()
# The game's name (PORT_GAME_NAME, port/CMakeLists.txt) and, for the messages about the ROM, its region
set(port_rom_title "${PORT_GAME_NAME} (${PORT_ROM_REGION})")
port_rom_sha1_bytes(port_rom_sha1_c "${port_rom_sha1}")
port_rom_sha1_bytes(port_rom_overlay_table_sha1_c "${port_rom_overlay_table_sha1}")
string(JSON port_rom_count LENGTH "${port_rom_files}")
set(port_rom_entries "")
math(EXPR port_rom_last "${port_rom_count} - 1")
foreach(index RANGE ${port_rom_last})
    string(JSON path MEMBER "${port_rom_files}" ${index})
    string(JSON entry GET "${port_rom_files}" "${path}")
    string(JSON size GET "${entry}" size)
    string(JSON sha1 GET "${entry}" sha1)
    port_rom_sha1_bytes(sha1_c "${sha1}")
    string(APPEND port_rom_entries "    { \"${path}\", ${size}, { ${sha1_c} } },\n")
endforeach()
set(port_rom_manifest_c "${PORT_GEN}/rom/rom_manifest.c")
file(WRITE "${port_rom_manifest_c}.new"
"// Generated by port/cmake/rom.cmake from ${port_rom_manifest_sources}; don't edit.
// Hashes of the ROM ${port_rom_title}, of its ARM9 overlay table and of its NitroFS files (port/hal/rom/rom_manifest.h)
#include \"hal/rom/rom_manifest.h\"

const char port_rom_manifest_version[] = \"${port_rom_manifest_version}\";
const char port_rom_manifest_game_code[] = \"${port_rom_game_code}\";
const char port_rom_manifest_game[] = \"${PORT_GAME_NAME}\";
const char port_rom_manifest_title[] = \"${port_rom_title}\";
const uint8_t port_rom_manifest_sha1[20] = { ${port_rom_sha1_c} };
const uint32_t port_rom_manifest_overlay_table_size = ${port_rom_overlay_table_size};
const uint8_t port_rom_manifest_overlay_table_sha1[20] = { ${port_rom_overlay_table_sha1_c} };
const PortRomManifestFile port_rom_manifest_files[] = {
${port_rom_entries}};
const size_t port_rom_manifest_count = ${port_rom_count};
")
# Copied only when it changed, so that configuring again rebuilds nothing
configure_file("${port_rom_manifest_c}.new" "${port_rom_manifest_c}" COPYONLY)
target_sources(port_hal PRIVATE "${port_rom_manifest_c}")

# The development default of the ROM's path, when neither --rom nor FRIGATE_ROM gives one (port_rom.h)
set_property(SOURCE "${port_rom_dir}/rom.c" APPEND PROPERTY COMPILE_DEFINITIONS
    "PORT_ROM_DEV_PATH=\"${REPO_ROOT}/orig/baserom_${port_rom_manifest_version}.nds\"")

# The ARM7's card services go on the PXI bus once the tree has it (port/hal/card/backup.c). The tree has
# port/hal/pxi, so this is always 1 now, and backup.c's branch for a tree without it is never compiled
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/hal/pxi/pxi_bus.h")
    set(port_card_pxi_bus 1)
else()
    set(port_card_pxi_bus 0)
endif()
set_property(SOURCE "${port_card_dir}/backup.c" APPEND PROPERTY
    COMPILE_DEFINITIONS "PORT_CARD_PXI_BUS=${port_card_pxi_bus}")

# The game's file that calls card.h's backup reads and writes (CARD_ReadBackup, CARD_ReadBackupAsync,
# CARD_WriteAndVerifyBackupAsync), whose inline functions cut its buffers to 32 bits: a forced include sends its calls
# to the host's functions, which take the buffers whole (port/hal/card/card_backup_calls.h). The file itself is
# compiled as it is, from its own path. A source's COMPILE_OPTIONS come after the target's -include of port_host.h
set(port_card_backup_callers "${REPO_ROOT}/src/gfl/backup_card.c")
if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
    set(port_card_backup_include "/FI${port_card_dir}/card_backup_calls.h")
else()
    set(port_card_backup_include -include "${port_card_dir}/card_backup_calls.h")
endif()
set_property(SOURCE ${port_card_backup_callers} APPEND PROPERTY COMPILE_OPTIONS ${port_card_backup_include})

# The overlays' snapshot and resets copy with a plain loop under ASan, which only -O2 makes fast (port/hal/fs/ovsec.c)
set_property(SOURCE "${port_fs_dir}/ovsec.c" APPEND PROPERTY COMPILE_OPTIONS -O2)

# Tests (port/tests/rom). The ones that need the player's ROM skip (77) without it: FRIGATE_ROM, else the repository's
# orig/baserom_<version>.nds, as the program finds it
enable_testing()
set(port_rom_test_options -std=gnu11 -Wall -Wextra -Wno-unused-parameter)

# The ROM's check and the system area, on port/hal/rom alone
add_executable(port_rom_unit_tests
    tests/rom/test_rom_unit.c "${port_rom_dir}/rom.c" "${port_rom_dir}/sha1.c" "${port_rom_manifest_c}"
    hal/os/app_options.c hal/hw/hw.c ${port_host_sources})
target_link_libraries(port_rom_unit_tests PRIVATE port_decomp_env)
target_include_directories(port_rom_unit_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(port_rom_unit_tests PRIVATE ${port_rom_test_options})
foreach(test sha1 card_id_without_rom default_path exact trimmed changed_program changed_file changed_overlay_table
        changed_overlay_file other_games damaged_tables open_and_system_area card_reads)
    add_test(NAME rom_${test} COMMAND port_rom_unit_tests ${test})
    set_tests_properties(rom_${test} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endforeach()

# The ARM7's backup service, driven word by word through a stand-in PXI bus; no ROM needed
add_executable(port_card_backup_tests
    tests/rom/test_backup.c "${port_card_dir}/backup.c" "${port_rom_manifest_c}" hal/os/app_options.c
    ${port_host_sources})
target_link_libraries(port_card_backup_tests PRIVATE port_decomp_env)
target_include_directories(port_card_backup_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_options(port_card_backup_tests PRIVATE ${port_rom_test_options})
foreach(test init identify flash unreachable save_file write_failure round_trip other_saves linked_save save_path)
    add_test(NAME card_backup_${test} COMMAND port_card_backup_tests ${test})
    set_tests_properties(card_backup_${test} PROPERTIES TIMEOUT 60)
endforeach()
# A save that is a symbolic link is macOS's and Linux's: the saves follow no links on Windows, where the test skips
set_tests_properties(card_backup_linked_save PROPERTIES SKIP_RETURN_CODE 77)
# The files the program refuses, each in a process of its own, since the program stops: with what the file seems to be.
# A size only suggests another chip's save, so the message says it has that size
add_test(NAME card_backup_bad_file COMMAND port_card_backup_tests bad_file)
set_tests_properties(card_backup_bad_file PROPERTIES TIMEOUT 60
    PASS_REGULAR_EXPRESSION "isn't a backup image of 524288 bytes.*: it is 10 bytes;" FAIL_REGULAR_EXPRESSION "went on")
add_test(NAME card_backup_refuse_eeprom COMMAND port_card_backup_tests refuse_eeprom)
set_tests_properties(card_backup_refuse_eeprom PROPERTIES TIMEOUT 60
    PASS_REGULAR_EXPRESSION
        "isn't a backup image of 524288 bytes.*: it is 65536 bytes, the size of the save of a 512 Kbit EEPROM, another"
    FAIL_REGULAR_EXPRESSION "went on")
add_test(NAME card_backup_refuse_dsv COMMAND port_card_backup_tests refuse_dsv)
set_tests_properties(card_backup_refuse_dsv PROPERTIES TIMEOUT 60
    PASS_REGULAR_EXPRESSION "isn't a backup image of 524288 bytes.*: it ends with DeSmuME's footer"
    FAIL_REGULAR_EXPRESSION "went on")
add_test(NAME card_backup_refuse_padded COMMAND port_card_backup_tests refuse_padded)
set_tests_properties(card_backup_refuse_padded PROPERTIES TIMEOUT 60
    PASS_REGULAR_EXPRESSION
        "it is 1048576 bytes, .*8 Mbit flash, but every byte after the first 524288 is 0xff.*keep only its first 524288"
    FAIL_REGULAR_EXPRESSION "went on")
add_test(NAME card_backup_refuse_larger COMMAND port_card_backup_tests refuse_larger)
set_tests_properties(card_backup_refuse_larger PROPERTIES TIMEOUT 60
    PASS_REGULAR_EXPRESSION "it is 1048576 bytes, the size of the save of an 8 Mbit flash, another game's chip;"
    FAIL_REGULAR_EXPRESSION "went on|padded")
add_test(NAME card_backup_refuse_directory COMMAND port_card_backup_tests refuse_directory)
set_tests_properties(card_backup_refuse_directory PROPERTIES TIMEOUT 60
    PASS_REGULAR_EXPRESSION "isn't a backup image of 524288 bytes.*: it is a directory;"
    FAIL_REGULAR_EXPRESSION "went on")

# The file system, the card's ROM reads, the overlays, the backup and the game's genuine-card check against the card's
# infrared chip (port/hal/card/card_spi.c) as the game links them: each scenario on the whole program's objects, with
# a main() of its own, in a process of its own (the SDK's libraries initialize once a process)
add_executable(port_rom_tests
    tests/rom/test_rom.c $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> ${port_gfx_objects}
    "${PORT_GEN}/stubs.c")
target_link_libraries(port_rom_tests PRIVATE port_decomp_env SDL3::SDL3)
target_include_directories(port_rom_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_rom_tests PRIVATE ${port_rom_test_options})
if(port_os_stack_probes)
    target_compile_options(port_rom_tests PRIVATE ${port_os_stack_probes})
endif()
foreach(test file_system card_read overlays backup memory_files ir_genuine_card)
    add_test(NAME rom_game_${test} COMMAND port_rom_tests ${test})
    set_tests_properties(rom_game_${test} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endforeach()
add_test(NAME rom_game_backup_cut_pointer COMMAND port_rom_tests backup_cut_pointer)
set_tests_properties(rom_game_backup_cut_pointer PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120
    PASS_REGULAR_EXPRESSION "cut to 32 bits, through CARDi_RequestStreamCommand|skipped"
    FAIL_REGULAR_EXPRESSION "went on")

# Synthetic ROM tables only: no Nintendo data or game execution. Also checks the configure-time manifest schema.
add_test(NAME rom_manifest_tool
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/rom/test_manifest.py")
set_tests_properties(rom_manifest_tool PROPERTIES TIMEOUT 60)

# Standalone wire controller only: synthetic immutable bytes, no SDK objects, ROM or scheduler integration.
add_executable(port_card_bus_tests tests/card/test_card_bus.c hal/card/port_card_bus.c)
target_include_directories(port_card_bus_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
set_target_properties(port_card_bus_tests PROPERTIES C_STANDARD 11 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
add_test(NAME card_bus COMMAND port_card_bus_tests)
set_tests_properties(card_bus PROPERTIES TIMEOUT 30 LABELS "unit")

# Experimental typed physical CARD events, not selected by production CARD reads. The genuine SDK OS scheduler and
# native core service synthetic READY/COMMIT words through masks, old IRQ Spin, Halt and stop/reset.
add_executable(port_card_hardware_tests tests/card/test_card_hardware.c
    $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> ${port_gfx_objects} "${PORT_GEN}/stubs.c")
target_link_libraries(port_card_hardware_tests PRIVATE port_decomp_env SDL3::SDL3)
target_include_directories(port_card_hardware_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${REPO_ROOT}")
if(port_os_stack_probes)
    target_compile_options(port_card_hardware_tests PRIVATE ${port_os_stack_probes})
endif()
foreach(scenario pacing absolute own_work masked timer_stall equal_cycle irq_spin halt bounds cancel inactive reset)
    add_test(NAME card_hardware_${scenario} COMMAND port_card_hardware_tests ${scenario})
    set_tests_properties(card_hardware_${scenario} PROPERTIES TIMEOUT 60)
endforeach()

add_test(NAME card_hardware_generic_conflict COMMAND "${PORT_PYTHON}"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/check_exit.py" --status 71
    --expect "generic operation conflicts with experimental CARD DMA channel ownership"
    -- $<TARGET_FILE:port_card_hardware_tests> generic_conflict)
set_tests_properties(card_hardware_generic_conflict PROPERTIES TIMEOUT 60)
