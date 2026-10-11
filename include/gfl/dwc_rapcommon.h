#ifndef POKEBW2_GFL_DWC_RAPCOMMON_H
#define POKEBW2_GFL_DWC_RAPCOMMON_H

#include "types.h"
#include "gfl/heap.h"

// dwc_rapcommon.c, in overlay 11: what the Wi-Fi connection shares with the applications that use it

// Starts the DWC library with a heap of its own in heapId, and shows the user info warning if that returns 3
void DWCRapCommon_StartLibrary(u32 heapId);
// Gives the library a second heap of size bytes in heapId, which allocations named name and larger than minSize use
void DWCRapCommon_SetSubHeap(u32 name, u32 size, HeapID heapId);
void DWCRapCommon_SetSubHeapEx(u32 name, u32 size, u32 minSize, HeapID heapId);
// Ends the second heap, now or once its blocks are freed
void DWCRapCommon_EndSubHeap(void);
// The heap to delete with the second heap
void DWCRapCommon_SetSubHeapOwner(HeapID heapId);
// The library's allocator and deallocator
void *DWCRapCommon_Alloc(u32 name, u32 size, int align);
void DWCRapCommon_Free(u32 name, void *ptr, u32 size);
// Creates and destroys the work and the library's heap
void DWCRapCommon_Create(HeapID heapId, u32 size);
void DWCRapCommon_Delete(void);
// Clears the user data and creates it if it is not valid
void DWCRapCommon_ResetUserData(void *userData);
// A state of the connection from 0 to 4, which callers switch on
int DWCRapCommon_CheckError(u32 a0, u32 a1);

#endif // POKEBW2_GFL_DWC_RAPCOMMON_H
