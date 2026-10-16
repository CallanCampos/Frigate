#ifndef POKEBW2_DWC_MEMFUNC_H
#define POKEBW2_DWC_MEMFUNC_H

#include "types.h"

// The memory of DWC (TWL-DWC 5.3's dwc_memfunc.c, in ARM9 main): the library takes every block from the allocator that
// the game gave it, and keeps its size in a header of its own before the block. A block is asked for by a name, which
// says which part of the library asks (0 to 10 so far). The ROM names none of it; the SDK's names are in the comments.

// The game's allocator, which gets the name, the size with the header and the alignment, and its deallocator, which
// gets the name, the block and its size with the header
typedef void *(*DWCAllocFunc)(u32 name, u32 size, int align);
typedef void (*DWCFreeFunc)(u32 name, void *ptr, u32 size);

// Gives the library its allocator and deallocator: DWCi_SetMemFunc
void DWCi_SetMemFunc(DWCAllocFunc alloc, DWCFreeFunc free);
// Allocates a block of size bytes, aligned to 32: DWC_Alloc
void *DWC_Alloc(u32 name, u32 size);
// The same with an alignment: DWC_AllocEx. Returns NULL if the allocator has no memory
void *DWC_AllocEx(u32 name, u32 size, int align);
// Frees a block that DWC_Alloc or DWC_AllocEx gave, NULL included. The third argument, the block's size in the SDK, is
// not used: DWC_Free
void DWC_Free(u32 name, void *ptr, u32 size);

// The allocator that the library hands to GameSpy, which all use the name 9: malloc, realloc, free and memalign
void *DWCi_GsAlloc(u32 size);
void *DWCi_GsRealloc(void *ptr, u32 size);
void DWCi_GsFree(void *ptr);
void *DWCi_GsMemalign(u32 align, u32 size);

#endif // POKEBW2_DWC_MEMFUNC_H
