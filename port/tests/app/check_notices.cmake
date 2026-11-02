# The test app_notices (port/cmake/app.cmake): the third-party notice that Frigate's program carries beside it,
# THIRD-PARTY-NOTICES.txt, which app.cmake writes at configure time, names xxHash and the release it is, and ends with
# the fetched xxHash archive's LICENSE byte for byte: the BSD-2-Clause notice that a program with xxHash compiled into
# its per-game-frame digest (--digest) must carry. The macOS app and its disk image carry the same file
# (port/app/macos/make_app.sh, port/tools/package_macos.sh), whose own tests check it there.
#
#   cmake -DNOTICES=FILE -DLICENSE=FILE -DTAG=VERSION -P check_notices.cmake
foreach(variable IN ITEMS NOTICES LICENSE TAG)
    if(NOT DEFINED ${variable})
        message(FATAL_ERROR "check_notices.cmake needs -D${variable}")
    endif()
endforeach()
foreach(file IN ITEMS "${NOTICES}" "${LICENSE}")
    if(NOT EXISTS "${file}")
        message(FATAL_ERROR "no ${file}")
    endif()
endforeach()

# As hexadecimal, so that every byte counts, line ends and all
file(READ "${NOTICES}" notices HEX)
file(READ "${LICENSE}" license HEX)
string(LENGTH "${notices}" notices_length)
string(LENGTH "${license}" license_length)
if(license_length EQUAL 0 OR notices_length LESS license_length)
    message(FATAL_ERROR "${NOTICES} is shorter than xxHash's LICENSE (${LICENSE})")
endif()
math(EXPR tail_start "${notices_length} - ${license_length}")
string(SUBSTRING "${notices}" ${tail_start} ${license_length} tail)
if(NOT tail STREQUAL license)
    message(FATAL_ERROR "${NOTICES} doesn't end with xxHash's LICENSE (${LICENSE}) byte for byte")
endif()

# The header before it says what it is the notice of
math(EXPR header_length "${tail_start} / 2")
file(READ "${NOTICES}" header LIMIT ${header_length})
if(NOT header MATCHES "xxHash ${TAG}")
    message(FATAL_ERROR "${NOTICES}'s header doesn't name xxHash ${TAG}:\n${header}")
endif()
message(STATUS "${NOTICES} names xxHash ${TAG} and ends with its LICENSE, byte for byte")
