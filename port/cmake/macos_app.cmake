# The macOS app (PLAN.md phase 5, ADR 0003): the program as <program>.app in the build directory, made by the target
# <program>_app from the program the build links (port/app/macos/make_app.sh), signed ad hoc and never notarized. It
# holds the program, the libraries it loads from outside macOS and an Info.plist, and no ROM or game data:
#
#   cmake --build build/port --target pokeblack2_app        (pokewhite2_app with -DPORT_VERSION=w2_us)
#
# -DPORT_MACOS_APP=ON makes it part of every build. port/tools/package_macos.sh puts it in a disk image.
if(NOT APPLE)
    return()
endif()

option(PORT_MACOS_APP "Build the macOS app with every build, not only as the target <program>_app" OFF)
# A placeholder: change it to a reverse-DNS name of a domain of your own (port/README.md, "The macOS app")
set(PORT_MACOS_BUNDLE_ID_PREFIX "com.example.pokebw2-native" CACHE STRING
    "The macOS app's bundle identifier, before the program's name")

if(PORT_VERSION STREQUAL "b2_us")
    set(port_app_display_name "Pokémon Black 2 (native port)")
else()
    set(port_app_display_name "Pokémon White 2 (native port)")
endif()
set(port_app_bundle "${CMAKE_CURRENT_BINARY_DIR}/${PORT_PROGRAM}.app")
if(PORT_MACOS_APP)
    set(port_app_all ALL)
else()
    set(port_app_all)
endif()

# Made again at each build of the target, since its version is the commit's and the script takes a moment
add_custom_target(${PORT_PROGRAM}_app ${port_app_all}
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/app/macos/make_app.sh"
        --program "$<TARGET_FILE:${PORT_PROGRAM}>" --app "${port_app_bundle}"
        --plist "${CMAKE_CURRENT_SOURCE_DIR}/app/macos/Info.plist.in"
        --bundle-id "${PORT_MACOS_BUNDLE_ID_PREFIX}.${PORT_PROGRAM}" --display-name "${port_app_display_name}"
        --repo "${REPO_ROOT}"
    BYPRODUCTS "${port_app_bundle}/Contents/MacOS/${PORT_PROGRAM}" "${port_app_bundle}/Contents/Info.plist"
    COMMENT "Building and signing ${PORT_PROGRAM}.app"
    VERBATIM)
add_dependencies(${PORT_PROGRAM}_app ${PORT_PROGRAM})
