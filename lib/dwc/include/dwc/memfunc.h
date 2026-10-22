#ifndef POKEBW2_DWC_MEMFUNC_H
#define POKEBW2_DWC_MEMFUNC_H

#include "types.h"
#include <stddef.h>

// The memory of DWC (TWL-DWC 5.3's dwc_memfunc.c, in ARM9 main): the library takes every block from the allocator that
// the game gave it, and keeps its size in a header of its own before the block. A block is asked for by a name, which
// says which part of the library asks. The ROM names none of it; the SDK's names are in the comments.

// The names of the blocks: the SDK's DWCAllocType, in its order (recalled from the SDK, which the values the code
// passes fit: 1 for the auto connect, 2 for the settings, 9 for GameSpy)
typedef enum DWCAllocType {
    DWC_ALLOCTYPE_AUTH,
    DWC_ALLOCTYPE_AC,
    DWC_ALLOCTYPE_BM,
    DWC_ALLOCTYPE_UTIL,
    DWC_ALLOCTYPE_BASE,
    DWC_ALLOCTYPE_LOGIN,
    DWC_ALLOCTYPE_FRIEND,
    DWC_ALLOCTYPE_MATCH,
    DWC_ALLOCTYPE_TRANSPORT,
    DWC_ALLOCTYPE_GS,
    DWC_ALLOCTYPE_ND,
} DWCAllocType;

// The game's allocator, which gets the name, the size with the header and the alignment, and its deallocator, which
// gets the name, the block and its size with the header
typedef void *(*DWCAllocFunc)(DWCAllocType name, u32 size, int align);
typedef void (*DWCFreeFunc)(DWCAllocType name, void *ptr, u32 size);

// Gives the library its allocator and deallocator: DWCi_SetMemFunc
void DWCi_SetMemFunc(DWCAllocFunc alloc, DWCFreeFunc free);
// Allocates a block of size bytes, aligned to 32: DWC_Alloc
void *DWC_Alloc(DWCAllocType name, u32 size);
// The same with an alignment: DWC_AllocEx. Returns NULL if the allocator has no memory
void *DWC_AllocEx(DWCAllocType name, u32 size, int align);
// Frees a block that DWC_Alloc or DWC_AllocEx gave, NULL included. The third argument, the block's size in the SDK, is
// not used: DWC_Free
void DWC_Free(DWCAllocType name, void *ptr, u32 size);

// The allocator that the library hands to GameSpy, which all use the name 9: malloc, realloc, free and memalign
void *DWCi_GsAlloc(size_t size);
void *DWCi_GsRealloc(void *ptr, size_t size);
void DWCi_GsFree(void *ptr);
void *DWCi_GsMemalign(size_t align, size_t size);

#endif // POKEBW2_DWC_MEMFUNC_H
