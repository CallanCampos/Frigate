#ifndef POKEBW2_DWC_GSMEMORY_H
#define POKEBW2_DWC_GSMEMORY_H

#include "types.h"

// GameSpy's memory functions (gsMemory.c), in overlay 11: every allocation of GameSpy's code in overlays 11, 189 and
// 260 goes through the four functions that the Wi-Fi library sets at its start (dwc_init.c), which are MSL's malloc,
// free, realloc and calloc until then

typedef void *(*gsMallocCB)(u32 size);
typedef void (*gsFreeCB)(void *ptr);
typedef void *(*gsReallocCB)(void *ptr, u32 size);
typedef void *(*gsMemalignCB)(u32 align, u32 size);

void gsiMemoryCallbacksSet(gsMallocCB malloc, gsFreeCB free, gsReallocCB realloc, gsMemalignCB memalign);
// Allocates (not zeroed), reallocates and frees, which ignores NULL
void *gsimalloc(u32 size);
void *gsirealloc(void *ptr, u32 size);
void gsifree(void *ptr);

#endif // POKEBW2_DWC_GSMEMORY_H
