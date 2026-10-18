# A project of nothing but the macOS app's bundle identifier (port/app/macos/bundle_id.cmake), which
# port/tests/app/test_bundle_id.sh configures in build directories of its own, as the host build includes it, and
# whose value it reads back from the cache. No language, no target: it configures in a moment.
cmake_minimum_required(VERSION 3.28)
project(bundle_id_test NONE)
include("${CMAKE_CURRENT_SOURCE_DIR}/../../../app/macos/bundle_id.cmake")
