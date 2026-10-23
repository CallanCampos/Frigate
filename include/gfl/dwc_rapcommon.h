#ifndef POKEBW2_GFL_DWC_RAPCOMMON_H
#define POKEBW2_GFL_DWC_RAPCOMMON_H

#include "types.h"
#include "dwc/account.h"
#include "dwc/memfunc.h"
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
void *DWCRapCommon_Alloc(DWCAllocType name, u32 size, int align);
void DWCRapCommon_Free(DWCAllocType name, void *ptr, u32 size);
// Creates and destroys the work and the library's heap
void DWCRapCommon_Create(HeapID heapId, u32 size);
void DWCRapCommon_Delete(void);
// Clears the user data and creates it if it is not valid
void DWCRapCommon_ResetUserData(DWCUserData *userData);
// How DWCRapCommon_CheckError found the connection, which WifiBattleMatchNetResult follows: no error, or one it
// cleared; DWC shut down and the error screen shown (an error with a0 FALSE); the error recorded for the caller to
// end on; the Wi-Fi error screen shown; and, with a1 TRUE, the connection to the other player lost (error types
// 0x3f0 and 0x3f1), Wi-Fi disconnected and the error screen shown
enum {
    DWCRAP_RESULT_OK,
    DWCRAP_RESULT_DISCONNECTED,
    DWCRAP_RESULT_FAILED,
    DWCRAP_RESULT_NET_ERROR,
    DWCRAP_RESULT_TIMEOUT,
};

// Checks the network's error and handles it, returning a DWCRAP_RESULT_*
int DWCRapCommon_CheckError(u32 a0, u32 a1);

#endif // POKEBW2_GFL_DWC_RAPCOMMON_H
