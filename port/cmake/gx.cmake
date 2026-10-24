# port/hal/gx's build rules: the SDK files whose exported functions the HAL wraps or replaces are compiled with those
# functions renamed port_sdk_<name>, and the GX tests (port/tests/gx).
#
#   - gx_vramcnt.c, gx_state.c, mi_wram.c: the only C that writes VRAMCNT (and WRAMCNT). Every exported function is
#     renamed; port/hal/gx/vramcnt.c gives the original names wrappers that call the SDK's and then apply the
#     registers to the VRAM views (ADR 0004)
#   - gx_load2d.c: the main engine's extended palette loads, which keep a bank's address in a static u32; port/hal/gx/
#     gx_load2d_extpltt.c replaces them, and the renamed originals stay unused
#
# A name the file no longer defines fails the link (the wrapper calls port_sdk_<name>); a new exported function of one
# of these files that writes VRAMCNT needs a line here and a wrapper.

set(port_gx_renamed_gx_vramcnt
    GX_SetBankForBG GX_SetBankForOBJ GX_SetBankForBGExtPltt GX_SetBankForOBJExtPltt GX_SetBankForTex
    GX_SetBankForTexPltt GX_SetBankForClearImage GX_SetBankForARM7 GX_SetBankForLCDC GX_SetBankForSubBG
    GX_SetBankForSubOBJ GX_SetBankForSubBGExtPltt GX_SetBankForSubOBJExtPltt
    GX_ResetBankForBG GX_ResetBankForOBJ GX_ResetBankForBGExtPltt GX_ResetBankForOBJExtPltt GX_ResetBankForTex
    GX_ResetBankForTexPltt GX_ResetBankForClearImage GX_ResetBankForSubBG GX_ResetBankForSubOBJ
    GX_ResetBankForSubBGExtPltt GX_ResetBankForSubOBJExtPltt
    GX_DisableBankForBG GX_DisableBankForOBJ GX_DisableBankForBGExtPltt GX_DisableBankForOBJExtPltt
    GX_DisableBankForTex GX_DisableBankForTexPltt GX_DisableBankForClearImage GX_DisableBankForARM7
    GX_DisableBankForLCDC GX_DisableBankForSubBG GX_DisableBankForSubOBJ GX_DisableBankForSubBGExtPltt
    GX_DisableBankForSubOBJExtPltt
    GX_GetBankForOBJ GX_GetBankForOBJExtPltt GX_GetBankForTex GX_GetBankForLCDC GX_GetBankForSubOBJ
    GX_GetBankForSubOBJExtPltt)
set(port_gx_renamed_gx_state GX_InitGXState)
set(port_gx_renamed_mi_wram MI_SetWramBank)
set(port_gx_renamed_gx_load2d
    GX_BeginLoadBGExtPltt GX_LoadBGExtPltt GX_EndLoadBGExtPltt
    GX_BeginLoadOBJExtPltt GX_LoadOBJExtPltt GX_EndLoadOBJExtPltt)

function(port_gx_rename source)
    set(file "${REPO_ROOT}/${source}")
    if(NOT "${file}" IN_LIST game_sources)
        message(FATAL_ERROR "port/cmake/gx.cmake: ${source} is no longer among the game's sources")
    endif()
    set(definitions)
    foreach(name IN LISTS ARGN)
        list(APPEND definitions "${name}=port_sdk_${name}")
    endforeach()
    set_property(SOURCE "${file}" APPEND PROPERTY COMPILE_DEFINITIONS ${definitions})
endfunction()

port_gx_rename(lib/nitro/src/gx/gx_vramcnt.c ${port_gx_renamed_gx_vramcnt})
port_gx_rename(lib/nitro/src/gx/gx_state.c ${port_gx_renamed_gx_state})
port_gx_rename(lib/nitro/src/mi/mi_wram.c ${port_gx_renamed_mi_wram})
port_gx_rename(lib/nitro/src/gx/gx_load2d.c ${port_gx_renamed_gx_load2d})

# The GX tests: the layout, every backend against the DS's behaviour and a reference model, and the SDK's calls through
# the HAL, each run as the game's main() on the whole program's objects (as port/cmake/os.cmake's tests are)
enable_testing()
add_executable(port_gx_tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/gx/test_gx.c"
    $<TARGET_OBJECTS:pokebw2_game> $<TARGET_OBJECTS:port_hal> ${port_gfx_objects} "${PORT_GEN}/stubs.c")
target_link_libraries(port_gx_tests PRIVATE port_decomp_env SDL3::SDL3)
target_include_directories(port_gx_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_compile_options(port_gx_tests PRIVATE -std=gnu11 -Wall -Wextra -Wno-unused-parameter)

add_test(NAME gx_layout COMMAND port_gx_tests layout)
foreach(backend IN ITEMS shm mach-entry mach-remap copy)
    add_test(NAME gx_backend_${backend} COMMAND port_gx_tests backend ${backend})
    set_tests_properties(gx_backend_${backend} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
endforeach()
set(port_gx_scenarios
    sdk_settings wrappers tex_upload texpltt_upload clear_image ext_palettes fill_source fifo cpu_pointer lcdc_clear)
# Each SDK scenario with an aliasing backend and with copying
foreach(scenario IN LISTS port_gx_scenarios)
    foreach(backend IN ITEMS shm copy)
        add_test(NAME gx_${scenario}_${backend} COMMAND port_gx_tests ${scenario} ${backend})
        set_tests_properties(gx_${scenario}_${backend} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 60)
    endforeach()
endforeach()
# The scenarios that run once, on the default backend: the backend's choice, POWCNT1, and the geometry command log's
# accesses to a parameter port with SPL's billboard through the geometry engine, which use no VRAM bank
add_test(NAME gx_default_backend COMMAND port_gx_tests default_backend)
add_test(NAME gx_display COMMAND port_gx_tests display)
add_test(NAME gx_parameter_read COMMAND port_gx_tests parameter_read)
set_tests_properties(gx_default_backend gx_display gx_parameter_read PROPERTIES TIMEOUT 60)
