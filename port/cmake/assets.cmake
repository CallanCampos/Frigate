# Frigate's artwork (port/assets/branding): the check that the committed SVGs are what their generator writes, that
# each draws with paths alone, and that the PNG masters have the sizes the build and the pages use
# (port/tests/assets/test_branding.py). The build makes the macOS app's icon (port/cmake/macos_app.cmake) and the
# window's icon (port/cmake/app.cmake) from those masters.
add_test(NAME assets_branding
    COMMAND "${CMAKE_COMMAND}" -E env PYTHONDONTWRITEBYTECODE=1 "${PORT_PYTHON}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/assets/test_branding.py" "${CMAKE_CURRENT_BINARY_DIR}/test_branding")
set_tests_properties(assets_branding PROPERTIES TIMEOUT 60)
