# How long Frigate's frames take on the machine that builds it: the program's --frame-times (port/hal/os/frame_time.c,
# which port_hal builds) and port/tools/frame_times.py, which reads its files. The tool's own test needs nothing; the
# timing tests play routes of the game from power-on, the title screen's 3D, New Game's intro and 3D flight, and the
# field menu's apps on Frigate's test launcher, 3 times each, take each frame's least time over the runs and fail when
# a scene's 95th percentile, without the pacing wait, is above PORT_FRAME_TIME_BUDGET_MS. They run alone (RUN_SERIAL),
# skip without the ROM (or the oracle's save, for the apps) like every route, and are labelled timing, so that a slow
# or loaded machine leaves them out with ctest -LE timing; PORT_FRAME_TIME_REPORT_ONLY makes them report only. The
# sanitized build has none: it times the sanitizers.
set(PORT_FRAME_TIME_BUDGET_MS 8.36 CACHE STRING
    "The most a timing test's scene may take at its 95th percentile, in ms (half the DS's frame by default)")
option(PORT_FRAME_TIME_REPORT_ONLY "Make the frame-time tests report their scenes without failing" OFF)

# The tool's summary on files made up for it: each frame's least over the runs, the quantiles, the scenes, the budget
add_test(NAME frame_times_tool
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_frame_times.py")
set_tests_properties(frame_times_tool PROPERTIES TIMEOUT 120)

if(NOT PORT_SANITIZE)
    set(port_frame_times_report "")
    if(PORT_FRAME_TIME_REPORT_ONLY)
        set(port_frame_times_report --report-only)
    endif()
    # port_frame_times_test(NAME FRAMES INPUT [SCENE...] [SAVE FILE] [LAUNCH LIST] [LABELS ...]): the route's input
    # file played for FRAMES frames, its scenes (NAME=FIRST-LAST, the program's frame numbers) held to the budget
    function(port_frame_times_test name frames input)
        cmake_parse_arguments(PARSE_ARGV 3 arg "" "SAVE;LAUNCH" "SCENES;LABELS")
        set(save_options "")
        if(arg_SAVE)
            set(save_options --save "${arg_SAVE}" --save-name ${port_frame_times_save_name})
        endif()
        set(launch_options "")
        if(arg_LAUNCH)
            set(launch_options --launch ${arg_LAUNCH})
        endif()
        set(scene_options "")
        foreach(scene IN LISTS arg_SCENES)
            list(APPEND scene_options --scene ${scene})
        endforeach()
        add_test(NAME frame_times_${name}
            COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/frame_times.py" check --runs 3
                    --budget-ms ${PORT_FRAME_TIME_BUDGET_MS} ${port_frame_times_report} ${scene_options} ${save_options}
                    --work "${CMAKE_CURRENT_BINARY_DIR}/frame_times_${name}"
                    -- $<TARGET_FILE:${PORT_PROGRAM}> --headless --frames ${frames} --watchdog 30
                    --input "${input}" ${launch_options})
        set_tests_properties(frame_times_${name} PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 1800 RUN_SERIAL TRUE)
        port_test_labels(frame_times_${name} route timing ${arg_LABELS})
    endfunction()

    if(PORT_VERSION STREQUAL "b2_us")
        set(port_frame_times_save_name IREO.sav)
    else()
        set(port_frame_times_save_name IRDO.sav)
    endif()
    # The title screen's 3D Kyurem, then the start menu over the title's last 3D frame
    port_frame_times_test(title 3600 "${CMAKE_CURRENT_SOURCE_DIR}/tests/oracle/title_${PORT_VERSION}.pbwinput"
        SCENES title=1110-3100 start_menu=3110-3599)
    # The field menu's apps from the starter save, which the oracle makes (the oracle_saves fixture)
    port_frame_times_test(party_menus 8400
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/oracle/launch_party_menus_${PORT_VERSION}.pbwinput"
        SCENES apps=4050-8030 SAVE "${CMAKE_BINARY_DIR}/oracle-saves/starter_save_${PORT_VERSION}.sav"
        LAUNCH party@4044,pokedex@5473,card@6974,bag@7673 LABELS oracle)
    set_tests_properties(frame_times_party_menus PROPERTIES FIXTURES_REQUIRED oracle_saves)
    # Black 2 only, as its replay: New Game's intro with Juniper's sprites, then the 3D flight over Aspertia
    if(PORT_VERSION STREQUAL "b2_us")
        port_frame_times_test(new_game 10600 "${CMAKE_CURRENT_SOURCE_DIR}/tests/replay/new_game.pbwinput"
            SCENES intro=4450-8400 flight=8850-10599)
    endif()
endif()
