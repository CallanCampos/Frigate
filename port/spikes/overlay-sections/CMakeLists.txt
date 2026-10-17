# Spike: per-overlay .data/.bss sections for reload semantics (PLAN.md 4.5, "Files and overlays"). See README.md.
#
#   cmake -S port/spikes/overlay-sections -B build/port-spikes/overlay-sections/plain
#   cmake --build build/port-spikes/overlay-sections/plain && ctest --test-dir build/port-spikes/overlay-sections/plain
#
# Options: OVSEC_SANITIZE=ON (ASan and UBSan), OVSEC_LTO=ON (ThinLTO), OVSEC_COVERAGE=ON (clang's source-based
# coverage). CMAKE_C_COMPILER picks Apple clang or Homebrew clang. OVSEC_LIMIT_COUNT is internal:
# tests/limit_check.cmake sets it in a second build tree that builds only the programs at the section limit.
cmake_minimum_required(VERSION 3.20)
project(overlay_sections C)
enable_testing()

option(OVSEC_SANITIZE "Build with -fsanitize=address,undefined" OFF)
option(OVSEC_LTO "Build with -flto=thin" OFF)
option(OVSEC_COVERAGE "Build with -fprofile-instr-generate -fcoverage-mapping" OFF)
set(OVSEC_LIMIT_COUNT "" CACHE STRING "Internal: build only the section-limit programs, with this many overlays")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()
set(CMAKE_C_STANDARD 11)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
get_filename_component(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../.." ABSOLUTE)
set(OVERLAY_CONFIG "${REPO_ROOT}/config/b2_us/arm9/overlays")
set(GEN "${CMAKE_CURRENT_SOURCE_DIR}/gen_overlays.py")
set(CHECK "${CMAKE_CURRENT_SOURCE_DIR}/check_sections.py")
set(GEN_DIR "${CMAKE_BINARY_DIR}/gen")
set(SECTIONS_DIR "${CMAKE_BINARY_DIR}/sections")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${GEN}")

# The host flags of PLAN.md 4.4 that bear on where statics land.
add_compile_options(-fno-common -ftrivial-auto-var-init=zero -fwrapv -fno-strict-aliasing -fsigned-char -Wall)
if(OVSEC_SANITIZE)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=undefined)
    add_link_options(-fsanitize=address,undefined)
endif()
if(OVSEC_LTO)
    add_compile_options(-flto=thin)
    add_link_options(-flto=thin)
endif()
if(OVSEC_COVERAGE)
    add_compile_options(-fprofile-instr-generate -fcoverage-mapping)
    add_link_options(-fprofile-instr-generate)
endif()

# Under LTO the back end decides afresh which globals are zero-initialized (bss) and does not see the overlay
# files' -fno-zero-initialized-in-bss, so the combined layout needs the option at link time as well (for the whole
# program). OVSEC_LTO_COMBINED_FIX=OFF shows what happens without it: the combined tests fail.
option(OVSEC_LTO_COMBINED_FIX "Pass -nozero-initialized-in-bss to the LTO back end" ON)
set(COMBINED_LINK_OPTIONS "")
if(OVSEC_LTO AND OVSEC_LTO_COMBINED_FIX)
    set(COMBINED_LINK_OPTIONS "LINKER:-mllvm,-nozero-initialized-in-bss")
endif()

# Under LTO, check_sections.py can't see the sections that instrumentation names explicitly in bitcode, so it keeps
# room for them, as measured from the native objects: ASan's __asan_globals, __asan_liveness, __asan_binder,
# __mod_init_func and __mod_term_func, and coverage's __llvm_prf_cnts, __llvm_prf_data, __llvm_prf_names,
# __llvm_prf_bits, __llvm_covfun, __llvm_covmap and __llvm_orderfile.
set(BITCODE_EXTRA 0)
if(OVSEC_SANITIZE)
    math(EXPR BITCODE_EXTRA "${BITCODE_EXTRA} + 5")
endif()
if(OVSEC_COVERAGE)
    math(EXPR BITCODE_EXTRA "${BITCODE_EXTRA} + 7")
endif()

# The runtime archives that the compiler driver adds to every link (the builtins, and the profile runtime for
# coverage) bring sections too, so check_sections.py reads them. "clang -###" prints the link command without
# running it.
get_directory_property(link_options LINK_OPTIONS)
file(WRITE "${CMAKE_BINARY_DIR}/driver_probe.o" "")
execute_process(COMMAND "${CMAKE_C_COMPILER}" "-###" ${link_options} driver_probe.o -o driver_probe
                WORKING_DIRECTORY "${CMAKE_BINARY_DIR}" ERROR_VARIABLE driver OUTPUT_QUIET)
string(REGEX MATCHALL "\"[^\"]*\\.a\"" RUNTIME_ARCHIVES "${driver}")
string(REPLACE "\"" "" RUNTIME_ARCHIVES "${RUNTIME_ARCHIVES}")

# Writable objects that instrumentation adds to every module outside the pragma's reach, which the leak check
# skips: UBSan's check data (source locations and type descriptors, private and unnamed, which its runtime writes
# to report each check once; under LTO, GlobalMerge merges them into _MergedGlobals) and ASan's ODR indicators
# (__odr_asan_gen_NAME, one per external global, which Homebrew clang 23 emits and Apple clang 21 doesn't). A
# compound literal could hide in _MergedGlobals too, so the builds without sanitizers are the ones that count.
set(IGNORE_LEAKS "")
if(OVSEC_SANITIZE)
    list(APPEND IGNORE_LEAKS --ignore-prefix l___unnamed_ --ignore-prefix ___odr_asan_gen_)
    if(OVSEC_LTO)
        list(APPEND IGNORE_LEAKS --ignore-prefix __MergedGlobals)
    endif()
endif()

# The forced include. clang-cl (the MSVC front end, a Windows option of PLAN.md 4.4) spells it /FI: it ignores
# -include with a warning and then takes the header for a second source file.
function(force_include out header)
    if(CMAKE_C_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        set(${out} "/FI${header}" PARENT_SCOPE)
    else()
        set(${out} "-include;${header}" PARENT_SCOPE)
    endif()
endfunction()

# Generates one set of overlays (gen_overlays.py) into an object library <name>_overlays. Every overlay file gets
# its own forced include, as the real source-list generator would do.
function(overlay_set name mode)
    set(dir "${GEN_DIR}/${name}")
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${GEN}" overlays "${dir}" --mode ${mode} ${ARGN}
                    RESULT_VARIABLE rc)
    if(NOT rc EQUAL 0)
        message(FATAL_ERROR "gen_overlays.py failed for ${name}")
    endif()
    file(READ "${dir}/sources.txt" ids)
    set(sources "${dir}/ov_table.c")
    foreach(id IN LISTS ids)
        force_include(options "${dir}/ov${id}_sections.h")
        if(mode STREQUAL "combined")
            list(APPEND options -fno-zero-initialized-in-bss)
        endif()
        set_source_files_properties("${dir}/ov${id}.c" PROPERTIES COMPILE_OPTIONS "${options}")
        list(APPEND sources "${dir}/ov${id}.c")
    endforeach()
    add_library(${name}_overlays OBJECT ${sources})
    target_include_directories(${name}_overlays PRIVATE tests)
endfunction()

# The runtime sketch, as a static library, as the HAL would be: check_sections.py reads archives member by member.
add_library(ovsec_rt STATIC ovsec/ovsec.c)
target_include_directories(ovsec_rt PUBLIC ovsec)

# Puts check_sections.py around a program. Before the link, "predict" counts the sections that every input brings
# (the program's objects, INPUTS and ovsec_rt) and stops a link that would pass 255. After it, the test
# image_<program> compares the prediction with the image, and reads the link map for writable objects of overlay
# files outside their overlay's sections (EXPECT_LEAKS lists the known ones, as NAME@ID). NOGUARD leaves out both,
# so that limit_check.cmake can see what ld64 does on its own.
function(check_program target overlays)
    cmake_parse_arguments(C "NOGUARD" "" "INPUTS;EXPECT_LEAKS" ${ARGN})
    set(base "${SECTIONS_DIR}/${target}")
    file(GENERATE OUTPUT "${base}.inputs"
         CONTENT "$<JOIN:$<TARGET_OBJECTS:${target}>;${C_INPUTS};$<TARGET_FILE:ovsec_rt>;${RUNTIME_ARCHIVES},\n>\n")
    target_link_options(${target} PRIVATE "LINKER:-map,${base}.map")
    if(OVSEC_LTO)
        # Names the ThinLTO objects, to which the map attributes each module's local symbols.
        file(MAKE_DIRECTORY "${base}.lto")
        target_link_options(${target} PRIVATE "LINKER:-object_path_lto,${base}.lto")
    endif()
    if(C_NOGUARD)
        return()
    endif()
    add_custom_command(TARGET ${target} PRE_LINK
                       COMMAND "${Python3_EXECUTABLE}" "${CHECK}" predict --inputs "${base}.inputs"
                               --overlays "${overlays}" --out "${base}.predicted" --nm "${CMAKE_NM}"
                               --bitcode-extra ${BITCODE_EXTRA}
                       VERBATIM)
    # A pre-link step runs only when the program links again, so a change to the check relinks it.
    set_property(TARGET ${target} APPEND PROPERTY LINK_DEPENDS "${CHECK}" "${overlays}")
    set(leaks "")
    foreach(leak IN LISTS C_EXPECT_LEAKS)
        list(APPEND leaks --expect-leak ${leak})
    endforeach()
    add_test(NAME image_${target}
             COMMAND "${Python3_EXECUTABLE}" "${CHECK}" image $<TARGET_FILE:${target}> --map "${base}.map"
                     --overlays "${overlays}" --predicted "${base}.predicted" ${leaks} ${IGNORE_LEAKS})
endfunction()

# Links a program from one overlay set, the runtime sketch and a main. ARGN goes to check_program.
function(overlay_program target set main mode)
    add_executable(${target} ${main} $<TARGET_OBJECTS:${set}_overlays>)
    target_link_libraries(${target} PRIVATE ovsec_rt)
    target_include_directories(${target} PRIVATE tests "${GEN_DIR}/${set}")
    if(mode STREQUAL "combined")
        target_link_options(${target} PRIVATE ${COMBINED_LINK_OPTIONS})
    endif()
    check_program(${target} "${GEN_DIR}/${set}/overlays.txt" INPUTS $<TARGET_OBJECTS:${set}_overlays> ${ARGN})
endfunction()

# The second build tree of limit_check.cmake: N combined overlays (exactly 255 sections in all) and N + 1.
if(OVSEC_LIMIT_COUNT)
    math(EXPR over "${OVSEC_LIMIT_COUNT} + 1")
    overlay_set(limit_at combined --count ${OVSEC_LIMIT_COUNT})
    overlay_set(limit_over combined --count ${over})
    overlay_program(limit_at limit_at tests/test_reload.c combined)
    overlay_program(limit_over limit_over tests/test_reload.c combined)
    overlay_program(limit_over_ld limit_over tests/test_reload.c combined NOGUARD)
    return()
endif()

# Every overlay with a .data or .bss on the DS, at its real size: 168 overlays, 201 sections split, 168 combined.
if(EXISTS "${OVERLAY_CONFIG}")
    set(REAL --real "${OVERLAY_CONFIG}")
else()
    set(REAL --count 168)
endif()
overlay_set(real_split split ${REAL})
overlay_set(real_combined combined ${REAL})
# About 220 overlay sections either way.
overlay_set(s220_split split --count 110)
overlay_set(s220_combined combined --count 220)

foreach(set real_split s220_split)
    overlay_program(test_reload_${set} ${set} tests/test_reload.c split)
    add_test(NAME reload_${set} COMMAND test_reload_${set})
endforeach()
foreach(set real_combined s220_combined)
    overlay_program(test_reload_${set} ${set} tests/test_reload.c combined)
    add_test(NAME reload_${set} COMMAND test_reload_${set})
endforeach()
overlay_program(bench_split real_split tests/bench_restore.c split)
overlay_program(bench_combined real_combined tests/bench_restore.c combined)
add_test(NAME bench_split COMMAND bench_split)
add_test(NAME bench_combined COMMAND bench_combined)

# What goes where: kinds.c as overlay 900 (split) and 901 (combined).
set(KINDS_DIR "${GEN_DIR}/kinds")
foreach(pair "900;split" "901;combined")
    list(GET pair 0 id)
    list(GET pair 1 mode)
    execute_process(COMMAND "${Python3_EXECUTABLE}" "${GEN}" header "${KINDS_DIR}/ov${id}_sections.h" ${id}
                            --mode ${mode})
    add_library(kinds${id} OBJECT tests/kinds.c)
    target_compile_definitions(kinds${id} PRIVATE KINDS_OV=${id})
    force_include(options "${KINDS_DIR}/ov${id}_sections.h")
    # unset[] in Automatic() is read on purpose: -ftrivial-auto-var-init=zero makes it 0.
    target_compile_options(kinds${id} PRIVATE ${options} -Wno-uninitialized)
    target_include_directories(kinds${id} PRIVATE tests)
    if(mode STREQUAL "combined")
        target_compile_options(kinds${id} PRIVATE -fno-zero-initialized-in-bss)
    endif()
endforeach()
file(WRITE "${KINDS_DIR}/ovsec_list.h" "OVSEC_OVERLAY(900, 900)\nOVSEC_OVERLAY(901, 901)\n")
file(WRITE "${KINDS_DIR}/overlays.txt"
     "900 split /kinds900.dir/tests/kinds.c.o\n901 combined /kinds901.dir/tests/kinds.c.o\n")
add_executable(test_kinds tests/test_kinds.c $<TARGET_OBJECTS:kinds900> $<TARGET_OBJECTS:kinds901>)
target_link_libraries(test_kinds PRIVATE ovsec_rt)
target_include_directories(test_kinds PRIVATE tests "${KINDS_DIR}")
target_link_options(test_kinds PRIVATE ${COMBINED_LINK_OPTIONS})
# The file-scope compound literal is the pragma's one gap (README.md, "What goes where").
check_program(test_kinds "${KINDS_DIR}/overlays.txt" INPUTS $<TARGET_OBJECTS:kinds900> $<TARGET_OBJECTS:kinds901>
              EXPECT_LEAKS _.compoundliteral@900 _.compoundliteral@901)
add_test(NAME kinds COMMAND test_kinds)

# The budget: the real sizes need 201 overlay sections split, over the 150 at which README.md recommends switching
# to the combined layout, so the check must refuse them with that message.
add_test(NAME overlay_budget
         COMMAND "${Python3_EXECUTABLE}" "${CHECK}" predict --inputs "${SECTIONS_DIR}/bench_split.inputs"
                 --overlays "${GEN_DIR}/real_split/overlays.txt" --out "${SECTIONS_DIR}/budget.predicted"
                 --nm "${CMAKE_NM}" --overlay-budget 150)
set_tests_properties(overlay_budget PROPERTIES PASS_REGULAR_EXPRESSION "over the budget of 150")

# The limit itself, at 255 and 256 sections, in a second build tree (see limit_check.cmake).
add_test(NAME section_limit
         COMMAND "${CMAKE_COMMAND}" "-DBUILD_DIR=${CMAKE_BINARY_DIR}" "-DSRC=${CMAKE_CURRENT_SOURCE_DIR}"
                 "-DPROBE=$<TARGET_FILE:test_reload_s220_combined>" "-DCC=${CMAKE_C_COMPILER}"
                 "-DGENERATOR=${CMAKE_GENERATOR}" "-DBUILD_TYPE=${CMAKE_BUILD_TYPE}" "-DSANITIZE=${OVSEC_SANITIZE}"
                 "-DLTO=${OVSEC_LTO}" "-DLTO_FIX=${OVSEC_LTO_COMBINED_FIX}" "-DCOVERAGE=${OVSEC_COVERAGE}"
                 -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/limit_check.cmake")

# ELF and COFF: compile kinds.c and ovsec.c for Linux and Windows targets and check the sections (no linking: this
# Mac has no ELF or PE linker).
find_program(LLVM_READOBJ llvm-readobj HINTS /opt/homebrew/opt/llvm/bin)
find_program(CLANG_CL clang-cl HINTS /opt/homebrew/opt/llvm/bin)
if(LLVM_READOBJ)
    add_test(NAME cross_elf_coff
             COMMAND "${CMAKE_COMMAND}" "-DCC=${CMAKE_C_COMPILER}" "-DCLANG_CL=${CLANG_CL}" "-DREADOBJ=${LLVM_READOBJ}"
                     "-DSRC=${CMAKE_CURRENT_SOURCE_DIR}" "-DGEN=${GEN}" "-DPYTHON=${Python3_EXECUTABLE}"
                     "-DOUT=${CMAKE_BINARY_DIR}/cross" -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cross_check.cmake")
endif()

# Coverage builds write a profile per run; keep them in the build tree.
if(OVSEC_COVERAGE)
    get_property(tests DIRECTORY PROPERTY TESTS)
    set_tests_properties(${tests} PROPERTIES ENVIRONMENT "LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/profiles/%p.profraw")
endif()
