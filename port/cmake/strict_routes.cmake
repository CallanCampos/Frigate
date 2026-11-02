# The test of port/tools/strict_routes.py, the runner of the strict build's routes from the oracle's saves: T5's runs of
# Frigate's sanitized program alone (port/docs/sanitizers.md, "Phase 5's criterion"; ADR 0033), on the comparison
# sequences that start from the oracle's saves and on the port's legs of the round trips, the routes that play CONTINUE
# into the field, the field menu and its apps, Hugh's battle, Route 19, the catch, the PC and Floccesy Town to Route 20.
# The runner itself runs by hand on a machine with the ROM and the strict build; its test,
# port/tests/tools/test_strict_routes.py, needs nothing (oracle.py's real tables, made-up ones and a stand-in for the
# program), so it runs in every build, the sanitized ones and CI's included, with no label
add_test(NAME strict_routes_tool
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/tools/test_strict_routes.py"
    WORKING_DIRECTORY "${REPO_ROOT}")
set_tests_properties(strict_routes_tool PROPERTIES TIMEOUT 300)
