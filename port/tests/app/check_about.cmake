# The test app_about (port/cmake/app.cmake): what --help says first (About in port/app/main.c, from the macros of
# port/hal/os/port_app.h) is exactly what the build read from that header for the macOS app's names and About window
# (port_product_string in port/CMakeLists.txt), written to EXPECTED at configure time: the product and the game, the
# description, the disclaimer and a blank line. So the C and the build can't name Frigate differently.
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
