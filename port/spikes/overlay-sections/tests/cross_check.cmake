# Compiles kinds.c (as overlay 900, split layout) and ovsec.c for ELF and COFF targets and checks with llvm-readobj
# where the objects land, also through clang-cl's /FI. Also checks that the forced include refuses a compiler that
# isn't clang. Nothing is linked: there is no ELF or PE linker on this Mac.
#   cmake -DCC=<clang> -DCLANG_CL=<clang-cl or empty> -DREADOBJ=<llvm-readobj> -DSRC=<spike dir>
#         -DGEN=<gen_overlays.py> -DPYTHON=<python> -DOUT=<dir> -P cross_check.cmake
file(MAKE_DIRECTORY "${OUT}")
execute_process(COMMAND "${PYTHON}" "${GEN}" header "${OUT}/ov900_sections.h" 900 --mode split)
file(WRITE "${OUT}/ovsec_list.h" "OVSEC_OVERLAY(012, 12)\nOVSEC_OVERLAY(900, 900)\n")

set(failures 0)
# Expected section of a symbol, per format: ELF names, then COFF names.
set(ELF_gInit900 "ov900_d")
set(ELF_gZero900 "ov900_b")
set(ELF_gTentative900 "ov900_b")
set(ELF_gPtrTable900 "ov900_d")
set(ELF_gConstTable900 "\\.rodata")
set(ELF_gConstPtrTable900 "\\.data\\.rel\\.ro")
set(COFF_gInit900 "\\.data\\$ov900m")
set(COFF_gZero900 "\\.bss\\$ov900m")
set(COFF_gTentative900 "\\.bss\\$ov900m")
set(COFF_gPtrTable900 "\\.data\\$ov900m")
set(COFF_gConstTable900 "\\.rdata")
set(COFF_gConstPtrTable900 "\\.rdata")

foreach(target x86_64-unknown-linux-gnu aarch64-unknown-linux-gnu x86_64-pc-windows-msvc aarch64-pc-windows-msvc
               x86_64-w64-windows-gnu)
    if(target MATCHES "windows")
        set(format COFF)
    else()
        set(format ELF)
    endif()
    set(flags --target=${target} -ffreestanding -O2 -fno-common -ftrivial-auto-var-init=zero -c -w)
    execute_process(COMMAND "${CC}" ${flags} -DKINDS_OV=900 -include "${OUT}/ov900_sections.h" -I "${SRC}/tests"
                            "${SRC}/tests/kinds.c" -o "${OUT}/kinds-${target}.o"
                    RESULT_VARIABLE rc ERROR_VARIABLE err)
    execute_process(COMMAND "${CC}" ${flags} -I "${SRC}/ovsec" -I "${OUT}" "${SRC}/ovsec/ovsec.c"
                            -o "${OUT}/ovsec-${target}.o"
                    RESULT_VARIABLE rc2 ERROR_VARIABLE err2)
    if(NOT rc EQUAL 0 OR NOT rc2 EQUAL 0)
        message(SEND_ERROR "${target}: compile failed\n${err}${err2}")
        math(EXPR failures "${failures} + 1")
        continue()
    endif()
    execute_process(COMMAND "${READOBJ}" --symbols "${OUT}/kinds-${target}.o" OUTPUT_VARIABLE syms)
    set(report "")
    foreach(sym gInit900 gZero900 gTentative900 gPtrTable900 gConstTable900 gConstPtrTable900)
        set(want "${${format}_${sym}}")
        if(syms MATCHES "Name: ${sym}( \\([0-9]+\\))?\n[^}]*Section: ([^ \n]+)")
            set(got "${CMAKE_MATCH_2}")
        else()
            set(got "?")
        endif()
        string(APPEND report " ${sym}=${got}")
        if(NOT got MATCHES "^${want}")
            message(SEND_ERROR "${target}: ${sym} is in ${got}, expected ${want}")
            math(EXPR failures "${failures} + 1")
        endif()
    endforeach()
    # ovsec.c: COFF defines the begin and end markers of each overlay in grouped sections; ELF references
    # __start_/__stop_ weakly.
    execute_process(COMMAND "${READOBJ}" --symbols "${OUT}/ovsec-${target}.o" OUTPUT_VARIABLE osyms)
    if(format STREQUAL "COFF")
        set(marker "Name: sDataBegin900( \\([0-9]+\\))?\n[^}]*Section: \\.data\\$ov900a")
    else()
        set(marker "Name: __start_ov900_d( \\([0-9]+\\))?\n[^}]*Binding: Weak")
    endif()
    if(NOT osyms MATCHES "${marker}")
        message(SEND_ERROR "${target}: ovsec.c lacks the expected bound symbol")
        math(EXPR failures "${failures} + 1")
    endif()
    message(STATUS "${target}:${report}")
endforeach()

# clang-cl, the MSVC front end: the forced include is /FI (clang-cl ignores -include and then takes the header for a
# second source file). The "--" keeps the macOS path of the source, /Users/..., from reading as the /U option.
if(CLANG_CL)
    execute_process(COMMAND "${CLANG_CL}" --target=x86_64-pc-windows-msvc /c /O2 -w -DKINDS_OV=900
                            "/FI${OUT}/ov900_sections.h" "/I${SRC}/tests" "/Fo${OUT}/kinds-clang-cl.obj"
                            -- "${SRC}/tests/kinds.c"
                    RESULT_VARIABLE rc ERROR_VARIABLE err)
    execute_process(COMMAND "${READOBJ}" --symbols "${OUT}/kinds-clang-cl.obj" OUTPUT_VARIABLE syms)
    if(NOT rc EQUAL 0 OR NOT syms MATCHES "Name: gInit900( \\([0-9]+\\))?\n[^}]*Section: \\.data\\$ov900m")
        message(SEND_ERROR "clang-cl with /FI: gInit900 is not in .data$ov900m\n${err}")
        math(EXPR failures "${failures} + 1")
    else()
        message(STATUS "clang-cl /FI: gInit900 in .data$ov900m")
    endif()
endif()

# A compiler other than clang (GCC ignores "#pragma clang section") must be refused by the forced include unless
# the build says it doesn't reset overlays. -U__clang__ stands in for GCC, which this Mac doesn't have.
execute_process(COMMAND "${CC}" --target=x86_64-unknown-linux-gnu -ffreestanding -c -U__clang__
                        -include "${OUT}/ov900_sections.h" -I "${SRC}/tests" -DKINDS_OV=900 "${SRC}/tests/kinds.c"
                        -o "${OUT}/kinds-noclang.o"
                RESULT_VARIABLE rc ERROR_VARIABLE err)
if(rc EQUAL 0 OR NOT err MATCHES "overlay sections need clang")
    message(SEND_ERROR "the forced include accepted a compiler without __clang__\n${err}")
    math(EXPR failures "${failures} + 1")
endif()
execute_process(COMMAND "${CC}" --target=x86_64-unknown-linux-gnu -ffreestanding -c -w -U__clang__ -DOVSEC_DISABLED
                        -include "${OUT}/ov900_sections.h" -I "${SRC}/tests" -DKINDS_OV=900 "${SRC}/tests/kinds.c"
                        -o "${OUT}/kinds-disabled.o"
                RESULT_VARIABLE rc ERROR_VARIABLE err)
execute_process(COMMAND "${CC}" --target=x86_64-unknown-linux-gnu -ffreestanding -c -w -DOVSEC_DISABLED
                        -I "${SRC}/ovsec" -I "${OUT}" "${SRC}/ovsec/ovsec.c" -o "${OUT}/ovsec-disabled.o"
                RESULT_VARIABLE rc2 ERROR_VARIABLE err2)
if(NOT rc EQUAL 0 OR NOT rc2 EQUAL 0)
    message(SEND_ERROR "OVSEC_DISABLED builds should compile\n${err}${err2}")
    math(EXPR failures "${failures} + 1")
else()
    message(STATUS "without clang: refused unless OVSEC_DISABLED, which builds")
endif()

if(failures GREATER 0)
    message(FATAL_ERROR "${failures} cross-check failures")
endif()
