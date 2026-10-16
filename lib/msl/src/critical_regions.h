#ifndef POKEBW2_MSL_CRITICAL_REGIONS_H
#define POKEBW2_MSL_CRITICAL_REGIONS_H

#include "nitro/os.h"

// The C library's critical regions, which keep two threads out of the same part of it (the atexit table, the heap,
// the standard streams and the other files). Each has a NitroSDK mutex in __cs, the id of the thread that has entered
// it in __cs_id and how many times in __cs_ref (critical_regions_nitro.c). The names of the arrays are pokediamond's
// object's, the regions' are MSL's: the ROM numbers the streams' regions 2 to 4 and the other files' 5
enum {
    atexit_funcs_access,
    malloc_pool_access,
    stdin_access,
    stdout_access,
    stderr_access,
    files_access,
    console_status_access,
    signal_funcs_access,
    thread_local_data_access,
    num_critical_regions,
};

extern u32 __cs_id[num_critical_regions];
extern s32 __cs_ref[num_critical_regions];
extern OSMutex __cs[num_critical_regions];

// Enters a region, as compiled in every caller of it
//
// BUG: OS_TryLockMutex returns TRUE when it took the mutex, or the thread had it already, and FALSE when another
// thread has it. Here the failed attempt enters without the mutex, and the successful one locks it a second time
// unless __cs_id and the thread's id agree, which __end_critical_region unlocks once. The fix is the usual one,
// entering again by counting and otherwise taking the mutex (which NitroSDK lets a thread take again)
static inline void __begin_critical_region(int region) {
#ifdef BUGFIX
    if (__cs_ref[region] != 0 && __cs_id[region] == OS_GetCurrentThread()->id) {
        __cs_ref[region]++;
    } else {
        OS_LockMutex(&__cs[region]);
        __cs_id[region] = OS_GetCurrentThread()->id;
        __cs_ref[region] = 1;
    }
#else
    if (!OS_TryLockMutex(&__cs[region])) {
        __cs_id[region] = OS_GetCurrentThread()->id;
        __cs_ref[region] = 1;
    } else if (__cs_id[region] == OS_GetCurrentThread()->id) {
        __cs_ref[region]++;
    } else {
        OS_LockMutex(&__cs[region]);
        __cs_id[region] = OS_GetCurrentThread()->id;
        __cs_ref[region] = 1;
    }
#endif
}

// Leaves it, giving the mutex back when the thread has left as often as it entered
static inline void __end_critical_region(int region) {
    if (--__cs_ref[region] == 0) {
        OS_UnlockMutex(&__cs[region]);
    }
}

#endif // POKEBW2_MSL_CRITICAL_REGIONS_H
