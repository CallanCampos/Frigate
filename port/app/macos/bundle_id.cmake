# The macOS app's bundle identifier (port/cmake/macos_app.cmake, which includes this file): PORT_MACOS_BUNDLE_ID_PREFIX
# followed by the version's black2 or white2, io.github.callancampos.frigate.black2 for Frigate's own builds. A build of
# your own that isn't Frigate's can take a reverse-DNS name of a domain of yours: macOS keeps an app's preferences and
# permissions under it. It is in a file of its own so that its test, app_bundle_id (port/tests/app/test_bundle_id.sh),
# can configure it alone.
#
# A build directory configured before the port was named Frigate (2026-10-18) holds the placeholder of the time,
# com.example.pokebw2-native, in its cache, where a new default never replaces it, and the placeholder followed by
# black2 or white2 would name an app of neither scheme. That value is moved to Frigate's here, once, with a message;
# any other value is the builder's own and stays. make_app.sh refuses every com.example identifier.
set(PORT_MACOS_BUNDLE_ID_DEFAULT "io.github.callancampos.frigate")
set(PORT_MACOS_BUNDLE_ID_PREFIX "${PORT_MACOS_BUNDLE_ID_DEFAULT}" CACHE STRING
    "The macOS app's bundle identifier, before the version's black2 or white2")
if(PORT_MACOS_BUNDLE_ID_PREFIX STREQUAL "com.example.pokebw2-native")
    set(PORT_MACOS_BUNDLE_ID_PREFIX "${PORT_MACOS_BUNDLE_ID_DEFAULT}" CACHE STRING
        "The macOS app's bundle identifier, before the version's black2 or white2" FORCE)
    message(STATUS "PORT_MACOS_BUNDLE_ID_PREFIX: this build directory had the placeholder com.example.pokebw2-native "
        "from before the port was named Frigate; it is now ${PORT_MACOS_BUNDLE_ID_DEFAULT}")
endif()
