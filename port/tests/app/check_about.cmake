# The test app_about (port/cmake/app.cmake): what --help says first (About in port/app/main.c, from the macros of
# port/hal/os/port_app.h) is exactly what the build read from that header for the macOS app's names and About window
# (port_product_string in port/CMakeLists.txt), written to EXPECTED at configure time: the product and the game, the
# description, the disclaimer and a blank line. So the C and the build can't name Frigate differently. After the usage,
# --help says what each option of the usage does (main.c's sOptions), and --launch's line names the apps that Frigate's
# test launcher says it opens when it is given one it doesn't (port/app/launcher_spec.c), so --help is where a reader
# finds the options.
#
#   cmake -DPROGRAM=PATH -DEXPECTED=FILE -DSAVE_DIR=DIR -P check_about.cmake
foreach(variable IN ITEMS PROGRAM EXPECTED SAVE_DIR)
    if(NOT DEFINED ${variable})
        message(FATAL_ERROR "check_about.cmake needs -D${variable}")
    endif()
endforeach()

file(READ "${EXPECTED}" expected ENCODING UTF-8)
# --help returns before the program opens anything; the save directory is given all the same, as to every run of a test
execute_process(COMMAND "${PROGRAM}" --help --save-dir "${SAVE_DIR}"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE messages)
if(NOT status EQUAL 0)
    message(FATAL_ERROR "--help exited with ${status}:\n${messages}")
endif()
string(FIND "${messages}" "${expected}" at)
if(NOT at EQUAL 0)
    message(FATAL_ERROR "--help doesn't start with what the build read from port/hal/os/port_app.h.\n"
        "Expected:\n${expected}\nGot:\n${messages}")
endif()
string(FIND "${messages}" "\nusage: " usage)
if(usage EQUAL -1)
    message(FATAL_ERROR "--help has no usage after the product:\n${messages}")
endif()
message(STATUS "--help starts with the product, the game, the description and the disclaimer the build read")

# Each option of the usage has its own line in the options after it
string(FIND "${messages}" "\noptions:\n" options)
string(FIND "${messages}" "\nexit statuses:\n" statuses)
if(options LESS usage OR statuses LESS options)
    message(FATAL_ERROR "--help has no options and exit statuses after the usage:\n${messages}")
endif()
math(EXPR usage_length "${options} - ${usage}")
string(SUBSTRING "${messages}" ${usage} ${usage_length} usage_text)
math(EXPR options_length "${statuses} - ${options}")
string(SUBSTRING "${messages}" ${options} ${options_length} options_text)
string(REGEX MATCHALL "\\[--[a-z0-9-]+" usage_options "${usage_text}")
# Without their brackets, which would keep CMake's list from splitting at the semicolons
string(REPLACE "[" "" usage_options "${usage_options}")
list(LENGTH usage_options count)
if(count LESS 2)
    message(FATAL_ERROR "--help's usage names no options:\n${usage_text}")
endif()
foreach(option IN LISTS usage_options)
    if(NOT options_text MATCHES "\n  ${option}[ \n]")
        message(FATAL_ERROR "--help's usage has ${option}, which the options after it don't describe:\n${options_text}")
    endif()
endforeach()

# --launch's line names the apps the launcher itself names, as it does to a run given one it doesn't open
execute_process(COMMAND "${PROGRAM}" --headless --launch nosuchapp --save-dir "${SAVE_DIR}"
    RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE launch_messages)
if(NOT launch_messages MATCHES "isn't an app the launcher opens (\\([a-z, ]+\\))")
    message(FATAL_ERROR "--launch nosuchapp (status ${status}) doesn't name the apps the launcher opens:\n"
        "${launch_messages}")
endif()
set(apps "${CMAKE_MATCH_1}")
# What an option does goes on over lines indented further than the option's own two spaces
string(REGEX REPLACE "\n   +" " " options_flat "${options_text}")
string(REGEX MATCH "\n  --launch [^\n]*" launch_line "${options_flat}")
string(FIND "${launch_line}" "${apps}" at)
if(at EQUAL -1)
    message(FATAL_ERROR "--help's --launch doesn't name the apps ${apps}:\n${options_text}")
endif()
message(STATUS "--help describes the ${count} options of its usage, and --launch's names the apps ${apps}")
