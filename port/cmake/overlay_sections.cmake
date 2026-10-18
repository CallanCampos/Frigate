# Overlay sections: each overlay's C files are compiled with a forced include that puts their writable statics in
# sections of that overlay alone, which port/hal/fs/ovsec.c snapshots and sets back whenever the game loads the overlay,
# as the DS reloads an overlay's .data and clears its .bss (PLAN.md §4.5; port/spikes/overlay-sections/README.md). The
# forced includes and the list of overlay files come from config/<version>/arm9/overlays/*/delinks.txt
# (port/tools/gen_overlay_sections.py), again at every configure.
#
# Layouts (-DPORT_OVSEC_LAYOUT): split, the default, gives an overlay a data section and a zerofill bss section;
# combined gives it one section, with its zeroed statics in the file (-fno-zero-initialized-in-bss), for when the
# split layout's sections no longer fit Mach-O's 255. On macOS a step before the link of every program that links the
# game's objects (the game and the tests that link it, whichever fragment adds them) counts the sections, of each slice
# of a universal program (the distribution build's, ADR 0015), and stops a link that would pass 255, or the budget of
# 150 overlay sections, with a message that names the cause; it also stops one whose objects put a writable static of
# an overlay file outside its overlay's sections, where no reset would find it (a leak). After the link, a test per
# program compares each slice's sections with the prediction, by name.

set(PORT_OVSEC_LAYOUT "split" CACHE STRING "Overlay sections: split (data and bss apart) or combined (one per overlay)")
set_property(CACHE PORT_OVSEC_LAYOUT PROPERTY STRINGS split combined)
if(NOT PORT_OVSEC_LAYOUT MATCHES "^(split|combined)$")
    message(FATAL_ERROR "PORT_OVSEC_LAYOUT must be split or combined, not ${PORT_OVSEC_LAYOUT}")
endif()
set(PORT_OVSEC_BUDGET 150 CACHE STRING
    "The most overlay sections a macOS link may have before it asks for the combined layout")

set(port_ovsec_gen "${PORT_GEN}/ovsec")
set(port_ovsec_tool "${REPO_ROOT}/port/tools/gen_overlay_sections.py")
execute_process(
    COMMAND "${PORT_PYTHON}" "${port_ovsec_tool}" generate --version "${PORT_VERSION}" --out "${port_ovsec_gen}"
        --layout "${PORT_OVSEC_LAYOUT}"
    WORKING_DIRECTORY "${REPO_ROOT}"
    RESULT_VARIABLE port_ovsec_result)
if(NOT port_ovsec_result EQUAL 0)
    message(FATAL_ERROR "port/tools/gen_overlay_sections.py failed")
endif()
file(GLOB port_ovsec_delinks "${REPO_ROOT}/config/${PORT_VERSION}/arm9/overlays/*/delinks.txt")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${port_ovsec_tool}" ${port_ovsec_delinks})

# The forced include of one overlay file. clang-cl, the MSVC front end, spells it /FI: it ignores -include with a
# warning and then takes the header for a second source file. A source's COMPILE_OPTIONS take no SHELL: prefix, and
# CMake doesn't merge them with the target's, whose -include of port_host.h comes first
function(port_ovsec_source path number)
    if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(options "/FI${port_ovsec_gen}/ov${number}_sections.h")
    else()
        set(options -include "${port_ovsec_gen}/ov${number}_sections.h")
    endif()
    if(PORT_OVSEC_LAYOUT STREQUAL "combined")
        list(APPEND options -fno-zero-initialized-in-bss)
    endif()
    set_property(SOURCE "${path}" APPEND PROPERTY COMPILE_OPTIONS ${options})
endfunction()
include("${port_ovsec_gen}/overlay_sources.cmake")

# ovsec.c reads the overlays' list on ELF and COFF, where the sections' bounds are symbols
target_include_directories(port_hal PRIVATE "${port_ovsec_gen}")

# The section count and the leak check before the link, on Mach-O only: ELF has no such limit, and COFF groups every
# overlay into .data and .bss. The inputs are everything the program links: its own objects, the object libraries its
# sources name ($<TARGET_OBJECTS:...>), the static libraries it links (a static SDL3 when SDL3 was built here), and the
# runtime archives the compiler driver adds itself, which "clang -###" names without linking
if(APPLE)
    file(WRITE "${port_ovsec_gen}/driver_probe.o" "")
    execute_process(COMMAND "${CMAKE_C_COMPILER}" "-###" driver_probe.o -o driver_probe
        WORKING_DIRECTORY "${port_ovsec_gen}" ERROR_VARIABLE port_ovsec_driver OUTPUT_QUIET)
    string(REGEX MATCHALL "\"[^\"]*\\.a\"" port_ovsec_runtime "${port_ovsec_driver}")
    string(REPLACE "\"" "" port_ovsec_runtime "${port_ovsec_runtime}")

    # The files of the static libraries that a target links, through aliases and interface libraries
    function(port_ovsec_static_libraries out target)
        set(files "")
        set(pending "")
        get_target_property(libraries ${target} LINK_LIBRARIES)
        if(libraries)
            list(APPEND pending ${libraries})
        endif()
        set(seen "")
        while(pending)
            list(POP_FRONT pending library)
            if(NOT TARGET "${library}" OR library IN_LIST seen)
                continue()
            endif()
            list(APPEND seen "${library}")
            get_target_property(aliased ${library} ALIASED_TARGET)
            if(aliased)
                list(APPEND pending ${aliased})
                continue()
            endif()
            get_target_property(type ${library} TYPE)
            if(type STREQUAL "STATIC_LIBRARY")
                list(APPEND files "$<TARGET_FILE:${library}>")
            elseif(type STREQUAL "INTERFACE_LIBRARY")
                get_target_property(interface ${library} INTERFACE_LINK_LIBRARIES)
                if(interface)
                    list(APPEND pending ${interface})
                endif()
            endif()
        endwhile()
        set(${out} "${files}" PARENT_SCOPE)
    endfunction()

    # The check before one program's link, and the test that its image has the sections the check counted. A pre-link
    # step runs only when the program links again, so a change to the check or to the list of overlay files relinks it
    function(port_ovsec_guard target)
        get_target_property(sources ${target} SOURCES)
        set(inputs "$<TARGET_OBJECTS:${target}>")
        foreach(source IN LISTS sources)
            if(source MATCHES "^\\$<TARGET_OBJECTS:[^>]+>$")
                list(APPEND inputs "${source}")
            endif()
        endforeach()
        port_ovsec_static_libraries(libraries ${target})
        list(APPEND inputs ${libraries} ${port_ovsec_runtime})
        file(GENERATE OUTPUT "${port_ovsec_gen}/link_inputs_${target}.txt" CONTENT "$<JOIN:${inputs},\n>\n")
        add_custom_command(TARGET ${target} PRE_LINK
            COMMAND "${PORT_PYTHON}" "${port_ovsec_tool}" check --inputs "${port_ovsec_gen}/link_inputs_${target}.txt"
                --out "${port_ovsec_gen}/link_sections_${target}.json" --overlay-budget "${PORT_OVSEC_BUDGET}"
                --overlay-objects "${port_ovsec_gen}/overlay_objects.txt"
            VERBATIM)
        set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS
            "${port_ovsec_tool}" "${port_ovsec_gen}/overlay_objects.txt")
        add_test(NAME overlay_sections_image_${target}
            COMMAND "${PORT_PYTHON}" "${port_ovsec_tool}" image $<TARGET_FILE:${target}>
                --predicted "${port_ovsec_gen}/link_sections_${target}.json")
    endfunction()

    # Every program of this directory whose sources name the game's objects, once every fragment has added its own
    function(port_ovsec_guard_programs)
        get_property(targets DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY BUILDSYSTEM_TARGETS)
        foreach(target IN LISTS targets)
            get_target_property(type ${target} TYPE)
            get_target_property(sources ${target} SOURCES)
            if(type STREQUAL "EXECUTABLE" AND "$<TARGET_OBJECTS:pokebw2_game>" IN_LIST sources)
                port_ovsec_guard(${target})
            endif()
        endforeach()
    endfunction()
    cmake_language(DEFER CALL port_ovsec_guard_programs)

    # The tests of the guard itself: a count over the budget stops the link with its message, and so does a leak, here
    # the statics of a file of ARM9 main (src/gfl/backup_card.c, whose zeroed sLocked is in __bss) taken for a file of
    # overlay 0, beside the real overlay files
    enable_testing()
    add_test(NAME overlay_sections_budget
        COMMAND "${PORT_PYTHON}" "${port_ovsec_tool}" check
            --inputs "${port_ovsec_gen}/link_inputs_${PORT_PROGRAM}.txt"
            --out "${port_ovsec_gen}/budget_test.json" --overlay-budget 1)
    set_tests_properties(overlay_sections_budget PROPERTIES
        PASS_REGULAR_EXPRESSION "overlay sections, over the budget of 1")
    file(READ "${port_ovsec_gen}/overlay_objects.txt" port_ovsec_objects)
    file(WRITE "${port_ovsec_gen}/leak_test_objects.txt.new"
        "${port_ovsec_objects}0 ${PORT_OVSEC_LAYOUT} src/gfl/backup_card.c\n")
    configure_file("${port_ovsec_gen}/leak_test_objects.txt.new" "${port_ovsec_gen}/leak_test_objects.txt" COPYONLY)
    add_test(NAME overlay_sections_leaks
        COMMAND "${PORT_PYTHON}" "${port_ovsec_tool}" check
            --inputs "${port_ovsec_gen}/link_inputs_${PORT_PROGRAM}.txt"
            --out "${port_ovsec_gen}/leak_test.json" --overlay-budget 0
            --overlay-objects "${port_ovsec_gen}/leak_test_objects.txt")
    set_tests_properties(overlay_sections_leaks PROPERTIES
        PASS_REGULAR_EXPRESSION "_sLocked of overlay 0's src/gfl/backup_card.c in __DATA,__(bss|data|common)")
endif()
