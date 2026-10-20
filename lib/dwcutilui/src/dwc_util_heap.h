#ifndef POKEBW2_DWCUTIL_DWC_UTIL_HEAP_H
#define POKEBW2_DWCUTIL_DWC_UTIL_HEAP_H

#include "types.h"

// The connection utility's memory: an expanded heap in the 256 KB that the caller gives it, which the utility's
// allocations come from with the VBlank interrupt held off. Every allocation failure stops the game.
// The ROM does not embed this source file's name; dwc_util_heap.c is descriptive.
void DWCUtilHeap_Init(void *memory);
void DWCUtilHeap_Exit(void);
void *DWCUtilHeap_Alloc(int size, int align);
// Allocates and clears the block
void *DWCUtilHeap_AllocClear(int size, int align);
// Frees the block that the pointer at this address points to, if any, and clears the pointer
void DWCUtilHeap_FreeAndNull(void *pointer);
void DWCUtilHeap_Free(void *block);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_HEAP_H
