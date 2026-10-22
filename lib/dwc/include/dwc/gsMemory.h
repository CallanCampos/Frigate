#ifndef POKEBW2_DWC_GSMEMORY_H
#define POKEBW2_DWC_GSMEMORY_H

#include "dwc/nonport.h"
#include <stddef.h>

// GameSpy's memory functions (gsMemory.c), in overlay 11: every allocation of GameSpy's code in overlays 11, 189 and
// 260 goes through the four functions that the Wi-Fi library sets at its start (dwc_init.c), which are MSL's malloc,
// free, realloc and calloc until then

typedef void *(*gsMallocCB)(size_t size);
typedef void (*gsFreeCB)(void *ptr);
typedef void *(*gsReallocCB)(void *ptr, size_t size);
typedef void *(*gsMemalignCB)(size_t align, size_t size);

void gsiMemoryCallbacksSet(gsMallocCB malloc, gsFreeCB free, gsReallocCB realloc, gsMemalignCB memalign);
// Allocates (not zeroed), reallocates and frees, which ignores NULL
void *gsimalloc(size_t size);
void *gsirealloc(void *ptr, size_t size);
void gsifree(void *ptr);

#endif // POKEBW2_DWC_GSMEMORY_H
