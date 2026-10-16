// The Metrowerks runtime's list of the functions that exit runs (the name is a guess: nothing in the ROM gives it).
// __register_atexit, which MSL's atexit calls, registers each function as a destructor without an object in one of the
// 64 links of the global destructor chain that this file provides, and fails once they are all taken.
#include "register_atexit.h"
#include "types.h"
#include <NMWException.h>

static DestructorChain atexit_funcs[64];
static int atexit_curr_func;

int __register_atexit(void (*func)(void)) {
    if (atexit_curr_func == 64) {
        return -1;
    }
    // exit calls a link's destructor with its object, which is NULL here and which func ignores
    __register_global_object(NULL, (void (*)(void *))func, &atexit_funcs[atexit_curr_func++]);
    return 0;
}
