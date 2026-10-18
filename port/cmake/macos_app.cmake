# The macOS app (PLAN.md phase 5, ADR 0003): the program as "Frigate Black 2.app" ("Frigate White 2.app" with
# -DPORT_VERSION=w2_us) in the build directory, made by the target <program>_app from the program the build links
# (port/app/macos/make_app.sh), signed ad hoc and never notarized. It holds the program, the libraries it loads from
# outside macOS, the version's icon and an Info.plist, and no ROM or game data:
#
#   cmake --build build/port --target frigate-black2_app        (frigate-white2_app with -DPORT_VERSION=w2_us)
#
# -DPORT_MACOS_APP=ON makes it part of every build. port/tools/package_macos.sh puts it in a disk image. The tests of
# make_app.sh and of the log an app opened from the Finder writes are here too, since both are macOS's alone.
if(NOT APPLE)
    return()
endif()

option(PORT_MACOS_APP "Build the macOS app with every build, not only as the target <program>_app" OFF)
# The bundle identifier is this prefix and the version's own word, black2 or white2. A build of your own that isn't
# Frigate's can take a reverse-DNS name of a domain of yours: macOS keeps an app's preferences and permissions under it
set(PORT_MACOS_BUNDLE_ID_PREFIX "io.github.callancampos.frigate" CACHE STRING
    "The macOS app's bundle identifier, before the version's black2 or white2")

# The app's names (Info.plist.in says where each shows): the product's (CFBundleName, PORT_PRODUCT_NAME), the app's
# own, which is also its file's and its disk image's (CFBundleDisplayName), and the bundle identifier's last word
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

# The app's icon: the version's 1024 px master (PORT_APP_ICON, port/assets/branding) as an .icns, made by sips and
# iconutil (port/app/macos/make_icns.sh) when the master changes, never committed
set(port_app_icns "${CMAKE_CURRENT_BINARY_DIR}/${PORT_PROGRAM}.icns")
add_custom_command(OUTPUT "${port_app_icns}"
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/app/macos/make_icns.sh" "${PORT_APP_ICON}" "${port_app_icns}"
    DEPENDS "${PORT_APP_ICON}" "${CMAKE_CURRENT_SOURCE_DIR}/app/macos/make_icns.sh"
    COMMENT "Making ${PORT_PROGRAM}.icns, the app's icon"
    VERBATIM)

# Made again at each build of the target, since its version is the commit's and the script takes a moment
add_custom_target(${PORT_PROGRAM}_app ${port_app_all}
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/app/macos/make_app.sh"
        --program "$<TARGET_FILE:${PORT_PROGRAM}>" --app "${port_app_bundle}"
        --plist "${CMAKE_CURRENT_SOURCE_DIR}/app/macos/Info.plist.in" --icon "${port_app_icns}"
        --bundle-id "${PORT_MACOS_BUNDLE_ID_PREFIX}.${port_app_id_suffix}" --name "${PORT_PRODUCT_NAME}"
        --display-name "${port_app_name}" --repo "${REPO_ROOT}"
    DEPENDS "${port_app_icns}"
    BYPRODUCTS "${port_app_bundle}/Contents/MacOS/${PORT_PROGRAM}" "${port_app_bundle}/Contents/Info.plist"
        "${port_app_bundle}/Contents/Resources/AppIcon.icns"
    COMMENT "Building and signing ${port_app_name}.app"
    VERBATIM)
add_dependencies(${PORT_PROGRAM}_app ${PORT_PROGRAM})

# make_app.sh on small programs made at test time, whose libraries are named by every kind of install name dyld knows,
# with the version's icon made by make_icns.sh, and both scripts' refusals (port/tests/app/test_make_app.sh)
add_test(NAME app_make_app
    COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_make_app.sh" "${CMAKE_C_COMPILER}"
        "${CMAKE_CURRENT_BINARY_DIR}/test_make_app" "${REPO_ROOT}" "${PORT_APP_ICON}")
set_tests_properties(app_make_app PROPERTIES TIMEOUT 300)

# The log of an app opened from the Finder (port/app/message_log.c): the program copied into a stand-in app and run with
# stderr on /dev/null, as LaunchServices runs it, under a home of its own (port/tests/app/test_message_log.sh)
add_test(NAME app_message_log
    COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_message_log.sh" $<TARGET_FILE:${PORT_PROGRAM}>
        "${CMAKE_CURRENT_BINARY_DIR}/test_message_log")
set_tests_properties(app_message_log PROPERTIES TIMEOUT 120)

# package_macos.sh's refusals of an image path that git would take into a repository, which come before it builds
# anything (port/tests/app/test_package_macos.sh)
add_test(NAME app_package_macos_out
    COMMAND sh "${CMAKE_CURRENT_SOURCE_DIR}/tests/app/test_package_macos.sh" "${REPO_ROOT}" "${CMAKE_BINARY_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}/test_package_macos")
set_tests_properties(app_package_macos_out PROPERTIES TIMEOUT 60)
