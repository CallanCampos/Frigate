# Struct layouts that the game's files must agree on in the host build (port/tests/layout/check_struct_layout.py): a
# file that declared its own copy of a struct whose header has another layout on a host read the wrong bytes there,
# though both agree on the DS. Each test compares the members' offsets in the objects' debug information, so it skips
# (77) in a build without -g or without llvm-dwarfdump.
#
# The trainer card's param (include/field/trcard_sys.h): src/ov012/event_field_proclink.c once declared a copy that
# put result and appParam at 0x14 and 0x18, where the DS has them, while the header's pointers put them at 0x20 and
# 0x24 on a host, so the card's result was read from gameData's high half (port/docs/game-changes.md)
add_test(NAME layout_trainer_card_param
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1
        "${PORT_PYTHON}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/layout/check_struct_layout.py"
            --objects-dir "${CMAKE_BINARY_DIR}/CMakeFiles/pokebw2_game.dir" --struct TrainerCardParam
            --members result,appParam src/ov012/event_field_proclink.c src/ov012/trcard_sys.c
    WORKING_DIRECTORY "${REPO_ROOT}")
set_tests_properties(layout_trainer_card_param PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 120)
