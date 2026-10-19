#include "dwc/gsMemory.h"
#include <stdlib.h>

// GameSpy's memory functions (gsMemory.c). The ROM names neither the file nor its functions; both are GameSpy's, as
// pret's pokerevo Wii build has them (gsiMemoryCallbacksSet, gsimalloc, gsirealloc and gsifree in nonport.s), with the
// later SDK's names for the defaults

static void *_gsi_malloc(u32 size);
static void _gsi_free(void *ptr);
static void *_gsi_realloc(void *ptr, u32 size);
static void *_gsi_memalign(u32 align, u32 size);

// The functions that the allocations go through
static struct {
    gsMallocCB malloc;
    gsFreeCB free;
    gsReallocCB realloc;
    gsMemalignCB memalign;
} sMemoryCallbacks = { _gsi_malloc, _gsi_free, _gsi_realloc, _gsi_memalign };

static void *_gsi_malloc(u32 size) {
    return malloc(size);
}

static void _gsi_free(void *ptr) {
    free(ptr);
}

static void *_gsi_realloc(void *ptr, u32 size) {
    return realloc(ptr, size);
}

// MSL has no memalign: this allocates size bytes, zeroed, as size / align elements of align bytes
static void *_gsi_memalign(u32 align, u32 size) {
    return calloc(size / align, align);
}

void gsiMemoryCallbacksSet(gsMallocCB malloc, gsFreeCB free, gsReallocCB realloc, gsMemalignCB memalign) {
    sMemoryCallbacks.malloc = malloc;
    sMemoryCallbacks.free = free;
    sMemoryCallbacks.realloc = realloc;
    sMemoryCallbacks.memalign = memalign;
}

void *gsimalloc(u32 size) {
    return sMemoryCallbacks.malloc(size);
}

void *gsirealloc(void *ptr, u32 size) {
    return sMemoryCallbacks.realloc(ptr, size);
}

void gsifree(void *ptr) {
    if (ptr != NULL) {
        sMemoryCallbacks.free(ptr);
    }
}
