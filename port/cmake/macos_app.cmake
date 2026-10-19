# The macOS app (PLAN.md phase 5, ADR 0003): the program as "Frigate Black 2.app" ("Frigate White 2.app" with
# -DPORT_VERSION=w2_us) in the build directory, made by the target <program>_app from the program the build links
# (port/app/macos/make_app.sh), signed ad hoc and never notarized. It holds the program, the libraries it loads from
# outside macOS, the version's icon and an Info.plist, and no ROM or game data:
#
#   cmake --build build/port --target frigate-black2_app        (frigate-white2_app with -DPORT_VERSION=w2_us)
#
# -DPORT_MACOS_APP=ON makes it part of every build. port/tools/package_macos.sh puts it in a disk image, but not a
# sanitized build's (PORT_SANITIZE), which carries the sanitizer runtime and is for testing on this Mac. The tests of
# make_app.sh, of the log an app opened from the Finder writes and of the program's slices are here too, since all are
# macOS's alone.
if(NOT APPLE)
    return()
endif()

option(PORT_MACOS_APP "Build the macOS app with every build, not only as the target <program>_app" OFF)
# The bundle identifier is PORT_MACOS_BUNDLE_ID_PREFIX and the version's own word, black2 or white2
include("${CMAKE_CURRENT_SOURCE_DIR}/app/macos/bundle_id.cmake")

# The app's names (Info.plist.in says where each shows): the product's (CFBundleName, PORT_PRODUCT_NAME, which
# port/CMakeLists.txt reads from port/hal/os/port_app.h), the app's own, which is also its file's and its disk image's
# (CFBundleDisplayName), and the bundle identifier's last word. The About window says what Frigate is and that it is not
# affiliated with the game's owners, from the same header (PORT_PRODUCT_DESCRIPTION and PORT_PRODUCT_DISCLAIMER)
if(PORT_VERSION STREQUAL "b2_us")
    set(port_app_name "${PORT_PRODUCT_NAME} Black 2")
    set(port_app_id_suffix black2)
else()
    set(port_app_name "${PORT_PRODUCT_NAME} White 2")
    set(port_app_id_suffix white2)
endif()
set(port_app_bundle "${CMAKE_CURRENT_BINARY_DIR}/${port_app_name}.app")
# For port/tools/package_macos.sh, which reads them from the build's CMakeCache.txt
set(PORT_MACOS_APP_TARGET "${PORT_PROGRAM}_app" CACHE INTERNAL "The target that builds the macOS app")
set(PORT_MACOS_APP_NAME "${port_app_name}" CACHE INTERNAL "The macOS app's name, without .app")
if(PORT_MACOS_APP)
    set(port_app_all ALL)
else()
    set(port_app_all)
endif()

# The app's icon: the version's 1024 px master (PORT_APP_ICON, port/assets/branding), scaled, and its 16 and 32 px images
# drawn for their size (PORT_APP_ICON_16 and _32), as an .icns made by sips and iconutil (port/app/macos/make_icns.sh)
# when the artwork changes, never committed
set(port_app_icns "${CMAKE_CURRENT_BINARY_DIR}/${PORT_PROGRAM}.icns")
add_custom_command(OUTPUT "${port_app_icns}"
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/app/macos/make_icns.sh" "${PORT_APP_ICON}" "${PORT_APP_ICON_16}"
        "${PORT_APP_ICON_32}" "${port_app_icns}"
    DEPENDS "${PORT_APP_ICON}" "${PORT_APP_ICON_16}" "${PORT_APP_ICON_32}"
        "${CMAKE_CURRENT_SOURCE_DIR}/app/macos/make_icns.sh"
    COMMENT "Making ${PORT_PROGRAM}.icns, the app's icon"
    VERBATIM)

# What the app is built for, when the build says (the distribution build, PORT_MACOS_DISTRIBUTION, says both: arm64 and
# x86_64, macOS 11.0; ADR 0015): make_app.sh refuses a binary without a slice for each architecture, or with a slice
# for a newer macOS, and LSMinimumSystemVersion is the deployment target. A development build gives neither, and the
# app is for the slices and the macOS its binaries were built for
string(REPLACE ";" " " port_app_archs "${CMAKE_OSX_ARCHITECTURES}")
set(port_app_targets)
if(CMAKE_OSX_ARCHITECTURES)
    list(APPEND port_app_targets --archs "${port_app_archs}")
endif()
if(CMAKE_OSX_DEPLOYMENT_TARGET)
    list(APPEND port_app_targets --minimum "${CMAKE_OSX_DEPLOYMENT_TARGET}")
endif()

# Made again at each build of the target, since its version is the commit's and the script takes a moment
add_custom_target(${PORT_PROGRAM}_app ${port_app_all}
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/app/macos/make_app.sh"
        --program "$<TARGET_FILE:${PORT_PROGRAM}>" --app "${port_app_bundle}"
        --plist "${CMAKE_CURRENT_SOURCE_DIR}/app/macos/Info.plist.in" --icon "${port_app_icns}"
        --bundle-id "${PORT_MACOS_BUNDLE_ID_PREFIX}.${port_app_id_suffix}" --name "${PORT_PRODUCT_NAME}"
        --display-name "${port_app_name}" --about "${PORT_PRODUCT_DESCRIPTION} ${PORT_PRODUCT_DISCLAIMER}"
        --repo "${REPO_ROOT}" ${port_app_targets}
    DEPENDS "${port_app_icns}"
    BYPRODUCTS "${port_app_bundle}/Contents/MacOS/${PORT_PROGRAM}" "${port_app_bundle}/Contents/Info.plist"
        "${port_app_bundle}/Contents/Resources/AppIcon.icns"
    COMMENT "Building and signing ${port_app_name}.app"
    VERBATIM)
add_dependencies(${PORT_PROGRAM}_app ${PORT_PROGRAM})

# The bundle identifier's prefix (port/app/macos/bundle_id.cmake), configured alone: Frigate's by default, the
# placeholder of a build directory from before the name moved to Frigate's, a builder's own kept
# (port/tests/app/test_bundle_id.sh)
add_test(NAME app_bundle_id
    COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_bundle_id.sh" "${CMAKE_COMMAND}" "${CMAKE_GENERATOR}"
        "${CMAKE_MAKE_PROGRAM}" "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/bundle_id"
        "${CMAKE_CURRENT_BINARY_DIR}/test_bundle_id")
set_tests_properties(app_bundle_id PROPERTIES TIMEOUT 120)

# make_app.sh on small programs made at test time, whose libraries are named by every kind of install name dyld knows,
# with the version's icon made by make_icns.sh, and both scripts' refusals (port/tests/app/test_make_app.sh)
add_test(NAME app_make_app
    COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_make_app.sh" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_BINARY_DIR}/test_make_app" "${REPO_ROOT}" "${PORT_APP_ICON}" "${PORT_APP_ICON_16}"
        "${PORT_APP_ICON_32}")
set_tests_properties(app_make_app PROPERTIES TIMEOUT 300)

# The log of an app opened from the Finder (port/app/message_log.c): the program copied into a stand-in app and run with
# stderr on /dev/null, as LaunchServices runs it, under a home of its own (port/tests/app/test_message_log.sh)
add_test(NAME app_message_log
    COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_message_log.sh" $<TARGET_FILE:${PORT_PROGRAM}>
        "${CMAKE_CURRENT_BINARY_DIR}/test_message_log")
set_tests_properties(app_message_log PROPERTIES TIMEOUT 120)

# The program's slices and the macOS each is built for, from its load commands, against the build's architectures and
# deployment target (the distribution build's arm64 and x86_64 for macOS 11.0, ADR 0015; a development build's one
# slice, this Mac's), and that it starts as each slice this Mac runs (port/app/macos/check_app.sh, which CI also runs
# on the finished app)
add_test(NAME app_slices
    COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/app/macos/check_app.sh" $<TARGET_FILE:${PORT_PROGRAM}>
        --archs "${port_app_archs}" --minimum "${CMAKE_OSX_DEPLOYMENT_TARGET}")
set_tests_properties(app_slices PROPERTIES TIMEOUT 60)
# A universal program's slices running the opening the same, frame by frame, the x86_64 one under Rosetta on Apple
# Silicon (port/tests/app/test_universal_slices.sh; it skips without a second slice, without Rosetta or without the ROM)
add_test(NAME app_universal_slices
    COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_universal_slices.sh" $<TARGET_FILE:${PORT_PROGRAM}>
        "${CMAKE_CURRENT_BINARY_DIR}/test_universal_slices")
set_tests_properties(app_universal_slices PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 900)

# package_macos.sh's refusals, which come before it builds anything: an image path that git would take into a
# repository, the distribution build's options, and a sanitized build, whose app carries the sanitizer runtime, by
# PORT_SANITIZE or by -fsanitize= in any of CMake's flags; and its check of the image behind them, which fails on a
# sanitizer's runtime in it and removes the image, on a stand-in app that the test builds with this build's CMake and
# compiler and packages for real (port/tests/app/test_package_macos.sh)
add_test(NAME app_package_macos_out
    COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_package_macos.sh" "${REPO_ROOT}" "${CMAKE_BINARY_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/test_package_macos")
set_tests_properties(app_package_macos_out PROPERTIES TIMEOUT 120)
