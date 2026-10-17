# The 255-section limit of a Mach-O image (nlist's n_sect is one byte, MAX_SECT in <mach-o/nlist.h>), at the
# boundary. PROBE, test_reload_s220_combined, has the same code as the programs built here, so the same number of
# other sections; with N = 255 minus that, a second build tree (OVSEC_LIMIT_COUNT=N) builds:
#   limit_at       N combined overlays, 255 sections in all: must link, pass, and have 255 sections;
#   limit_over     N + 1: check_sections.py must stop it before ld64 runs (under LTO, whose sections it can only
#                  bound, it must at least warn before ld64 fails);
#   limit_over_ld  N + 1 without the check: ld64 must fail, and not because a symbol is missing.
# A link past the limit can crash ld64 instead of failing; it then writes an 80 MB "linker snapshot" to the
# temporary directory, so the builds here point TMPDIR into the build tree.
#   cmake -DBUILD_DIR=... -DSRC=... -DPROBE=... -DCC=... -DGENERATOR=... -DBUILD_TYPE=... -DSANITIZE=... -DLTO=...
#         -DLTO_FIX=... -DCOVERAGE=... -P limit_check.cmake
set(env "${CMAKE_COMMAND}" -E env "LLVM_PROFILE_FILE=${BUILD_DIR}/profiles/%p.profraw")
execute_process(COMMAND ${env} "${PROBE}" OUTPUT_VARIABLE out)
if(NOT out MATCHES "image sections ([0-9]+) \\(others ([0-9]+)\\)")
    message(FATAL_ERROR "cannot read the probe's section count:\n${out}")
endif()
math(EXPR at "255 - ${CMAKE_MATCH_2}")
math(EXPR over "${at} + 1")
message(STATUS "other sections: ${CMAKE_MATCH_2}; overlays at the limit: ${at}")

set(sub "${BUILD_DIR}/limit")
execute_process(COMMAND "${CMAKE_COMMAND}" -S "${SRC}" -B "${sub}" -G "${GENERATOR}" "-DCMAKE_C_COMPILER=${CC}"
                        "-DCMAKE_BUILD_TYPE=${BUILD_TYPE}" "-DOVSEC_SANITIZE=${SANITIZE}" "-DOVSEC_LTO=${LTO}"
                        "-DOVSEC_LTO_COMBINED_FIX=${LTO_FIX}" "-DOVSEC_COVERAGE=${COVERAGE}"
                        "-DOVSEC_LIMIT_COUNT=${at}"
                RESULT_VARIABLE rc OUTPUT_VARIABLE log ERROR_VARIABLE log)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "configuring the limit build failed:\n${log}")
endif()

file(MAKE_DIRECTORY "${sub}/tmp")
function(build target)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E env "TMPDIR=${sub}/tmp/"
                            "${CMAKE_COMMAND}" --build "${sub}" --target ${target} --parallel 8
                    RESULT_VARIABLE rc OUTPUT_VARIABLE out ERROR_VARIABLE err)
    set(rc ${rc} PARENT_SCOPE)
    set(log "${out}${err}" PARENT_SCOPE)
endfunction()

# The positive control: exactly 255 sections link and run.
build(limit_at)
if(NOT rc EQUAL 0)
    message(FATAL_ERROR "limit_at (${at} overlays) did not link:\n${log}")
endif()
execute_process(COMMAND ${env} "${sub}/limit_at" OUTPUT_VARIABLE run RESULT_VARIABLE rc)
if(NOT rc EQUAL 0 OR NOT run MATCHES "image sections 255 ")
    message(FATAL_ERROR "limit_at should run with 255 sections:\n${run}")
endif()
string(STRIP "${run}" run)
message(STATUS "limit_at: ${run}")

# One more section, with the check: stopped before ld64 runs, or under LTO at least explained just before it fails.
build(limit_over)
if(rc EQUAL 0)
    message(FATAL_ERROR "limit_over (${over} overlays) linked")
endif()
file(STRINGS "${sub}/sections/limit_over.predicted" predicted REGEX "^low ")
string(REGEX REPLACE "^low " "" low "${predicted}")
if(low GREATER 255)
    if(NOT log MATCHES "check_sections: [0-9]+ sections, more than the 255" OR log MATCHES "\nld: ")
        message(FATAL_ERROR "check_sections.py should have stopped limit_over before ld64:\n${log}")
    endif()
    message(STATUS "limit_over: stopped before the link by check_sections.py")
else()
    if(NOT log MATCHES "check_sections: warning: up to [0-9]+ sections" OR NOT log MATCHES "linker command failed")
        message(FATAL_ERROR "check_sections.py should have warned before ld64 failed on limit_over:\n${log}")
    endif()
    message(STATUS "limit_over: check_sections.py warned (LTO bitcode), then ld64 failed")
endif()

# One more section, without the check: ld64 fails on its own, for no reason it names, or crashes.
build(limit_over_ld)
if(rc EQUAL 0)
    message(FATAL_ERROR "limit_over_ld (${over} overlays) linked with more than 255 sections")
endif()
if(log MATCHES "Undefined symbols|symbol\\(s\\) not found|check_sections")
    message(FATAL_ERROR "limit_over_ld failed for another reason:\n${log}")
endif()
if(NOT log MATCHES "linker command failed")
    message(FATAL_ERROR "limit_over_ld failed before the link:\n${log}")
endif()
string(REGEX MATCHALL "ld: [^\n]*|clang: error: unable to execute command[^\n]*" lines "${log}")
list(LENGTH lines n)
if(n GREATER 2)
    list(SUBLIST lines 0 2 lines)
endif()
foreach(line IN LISTS lines)
    message(STATUS "limit_over_ld: ${line}")
endforeach()
