// MSL C's abort_exit_arm.c (the name is a guess from the ARM platform group of MSL_C_NITRO_Ai_LE.a): atexit, which
// takes the lock of the atexit table around the runtime's __register_atexit. exit (exit.c) lies before this file in the
// ROM.
#include "critical_regions.h"
#include "runtime/register_atexit.h"
#include <stdlib.h>

int atexit(void (*func)(void)) {
    int result;

    __begin_critical_region(atexit_funcs_access);
    result = __register_atexit(func);
    __end_critical_region(atexit_funcs_access);
    return result;
}
