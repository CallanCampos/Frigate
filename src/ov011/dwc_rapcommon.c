// dwc_rapcommon.c: the memory the Wi-Fi connection's DWC library allocates from, its start-up, and what an
// application does with the connection's error. The names are ours

#include "gfl/dwc_rapcommon.h"
#include "types.h"
#include "app/wfc_user_info_warning.h"
#include "dwc/dwc.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_state.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "nnsys/fnd.h"

typedef struct DWCRapCommonWork {
    NNSFndHeapHandle heap;
    void *heapPtr;
    // A second heap for an application, which the library allocates from while it is set
    NNSFndHeapHandle headHandleSub;
    void *heapPtrSub;
    u32 subName;
    int subCount;
    u32 subMinSize;
    int destroySub;
    u16 subHeapId;
} DWCRapCommonWork;

static DWCRapCommonWork *pDwcRapWork;

static void DWCRapCommon_DestroySubHeap(void);
static int DWCRapCommon_Init(HeapID heapId);
static void DWCRapCommon_ShutdownLibrary(void);

void DWCRapCommon_StartLibrary(u32 heapId) {
    if (DWCRapCommon_Init(heapId) == 3) {
        GFL_OvlLoad(OVERLAY_WFC_USER_INFO_WARNING);
        WFCUserInfoWarning_Show();
        GFL_OvlUnload(OVERLAY_WFC_USER_INFO_WARNING);
    }
}

void DWCRapCommon_SetSubHeap(u32 name, u32 size, HeapID heapId) {
    DWCRapCommon_SetSubHeapEx(name, size, 0, heapId);
}

void DWCRapCommon_SetSubHeapEx(u32 name, u32 size, u32 minSize, HeapID heapId) {
    GFL_ASSERT(pDwcRapWork != NULL);
    GFL_ASSERT(pDwcRapWork->heapPtrSub == NULL);
    GFL_ASSERT(pDwcRapWork->headHandleSub == NULL);

    pDwcRapWork->heapPtrSub = GFL_HeapAllocate(heapId, size - 0x80, FALSE, "dwc_rapcommon.c", 98);
    pDwcRapWork->headHandleSub =
        NNS_FndCreateExpHeapEx((void *)(((uptr)pDwcRapWork->heapPtrSub + 0x1f) & ~0x1f), size - 0xc0, 0);
    pDwcRapWork->subName = name;
    NNS_FndSetAllocModeForExpHeap(pDwcRapWork->headHandleSub, 1);
    NNS_FndSetGroupIDForExpHeap(pDwcRapWork->headHandleSub, 0xea);
    pDwcRapWork->subMinSize = minSize;
}

void DWCRapCommon_EndSubHeap(void) {
    GFL_ASSERT(pDwcRapWork);

    if (pDwcRapWork->subCount != 0) {
        // Blocks of the second heap are still allocated
        NetState_RecordError(0, 0, 0, 0x3ee);
        pDwcRapWork->destroySub = TRUE;
    } else {
        DWCRapCommon_DestroySubHeap();
    }
}

static void DWCRapCommon_DestroySubHeap(void) {
    if (pDwcRapWork->heapPtrSub != NULL) {
        NNS_FndDestroyExpHeap(pDwcRapWork->headHandleSub);
        GFL_HeapFree(pDwcRapWork->heapPtrSub);
        pDwcRapWork->headHandleSub = NULL;
        pDwcRapWork->heapPtrSub = NULL;
        if (pDwcRapWork->subHeapId != 0) {
            GFL_HeapDelete(pDwcRapWork->subHeapId);
            pDwcRapWork->subHeapId = 0;
        }
    }
}

void DWCRapCommon_SetSubHeapOwner(HeapID heapId) {
    pDwcRapWork->subHeapId = heapId;
}

void *DWCRapCommon_Alloc(DWCAllocType name, u32 size, int align) {
    void *ptr;
    u32 mask;

    if (pDwcRapWork->headHandleSub != NULL && name == pDwcRapWork->subName && size > pDwcRapWork->subMinSize) {
        mask = OS_DisableInterrupts();
        ptr = NNS_FndAllocFromExpHeapEx(pDwcRapWork->headHandleSub, size, align);
        OS_RestoreInterrupts(mask);
        pDwcRapWork->subCount++;
    } else {
        mask = OS_DisableInterrupts();
        ptr = NNS_FndAllocFromExpHeapEx(pDwcRapWork->heap, size, align);
        OS_RestoreInterrupts(mask);
    }

    if (ptr == NULL) {
        AssertFailErrorDisp();
        NetState_RecordError(0, 0, 0, 0x3ee);
        return NULL;
    }
    return ptr;
}

void DWCRapCommon_Free(DWCAllocType name, void *ptr, u32 size) {
    u32 mask;

    if (ptr != NULL) {
        if (NNS_FndGetGroupIDForMBlockExpHeap(ptr) == 0xea) {
            if (pDwcRapWork->headHandleSub != NULL) {
                mask = OS_DisableInterrupts();
                NNS_FndFreeToExpHeap(pDwcRapWork->headHandleSub, ptr);
                OS_RestoreInterrupts(mask);
                pDwcRapWork->subCount--;
            } else {
                NetState_RecordError(0, 0, 0, 0x3ee);
            }
        } else {
            mask = OS_DisableInterrupts();
            NNS_FndFreeToExpHeap(pDwcRapWork->heap, ptr);
            OS_RestoreInterrupts(mask);
        }
    }
}

void DWCRapCommon_Create(HeapID heapId, u32 size) {
    // clang-format off
    GFL_ASSERT(pDwcRapWork==NULL);
    // clang-format on

    pDwcRapWork = GFL_HeapAllocate(0, sizeof(DWCRapCommonWork), TRUE, "dwc_rapcommon.c", 332);
    pDwcRapWork->heapPtr = GFL_HeapAllocate(heapId, size - 0x80, FALSE, "dwc_rapcommon.c", 333);
    pDwcRapWork->heap = NNS_FndCreateExpHeapEx((void *)(((uptr)pDwcRapWork->heapPtr + 0x1f) & ~0x1f), size - 0xc0, 0);
    pDwcRapWork->subHeapId = 0;
}

void DWCRapCommon_Delete(void) {
    GFL_ASSERT(pDwcRapWork);

    if (pDwcRapWork->destroySub == TRUE || pDwcRapWork->heapPtrSub != NULL) {
        DWCRapCommon_DestroySubHeap();
    }
    NNS_FndDestroyExpHeap(pDwcRapWork->heap);
    GFL_HeapFree(pDwcRapWork->heapPtr);
    GFL_HeapFree(pDwcRapWork);
    pDwcRapWork = NULL;
}

static int DWCRapCommon_Init(HeapID heapId) {
    int result;

    DWCRapCommon_Create(heapId, 0x40000);
    result = DWC_InitForProduction("syachi2ds", 0x4952414a, DWCRapCommon_Alloc, DWCRapCommon_Free);
    DWCRapCommon_Delete();
    return result;
}

void DWCRapCommon_ResetUserData(void *userData) {
    MIi_CpuClearFast(0, userData, 0x40);
    if (!DWC_CheckUserData(userData)) {
        DWC_CreateUserData(userData);
        DWC_ClearDirtyFlag(userData);
    }
}

static void DWCRapCommon_ShutdownLibrary(void) {
    NetState_ClearWifiError();
    NetState_ClearError();
    DWC_ClearError();
}

int DWCRapCommon_CheckError(u32 a0, u32 a1) {
    GFLNetErrorInfo *error;
    int type;

    if (GFL_NetIsInitialized()) {
        error = NetState_GetErrorInfo();
        if (GFL_NetErrCheck()) {
            switch (error->errorType) {
            case 1:
            case 2:
                if (a0 == 0) {
                    DWCRapCommon_ShutdownLibrary();
                    GFL_NetErrShowScreen();
                    GFL_NetErrClear();
                    return DWCRAP_RESULT_DISCONNECTED;
                }
                DWC_ClearError();
                // fall through
            case 3:
            case 4:
            case 5:
                GFL_NetErrMarkShown();
                GFL_NetErrRecordWifi(13);
                return DWCRAP_RESULT_FAILED;
            case 6:
                GFL_NetErrMarkShown();
                return DWCRAP_RESULT_FAILED;
            case 7:
                GFL_NetErrShowWifi(15);
                return DWCRAP_RESULT_NET_ERROR;
            }
        }

        type = error->type;
        if (type == 0x3f0 || type == 0x3f1) {
            if (a1 != 0) {
                GFL_NetErrRecord();
                DWCRapCommon_ShutdownLibrary();
                NetState_WifiDisconnect(TRUE);
                GFL_NetErrShowScreen();
                GFL_NetErrClear();
                return DWCRAP_RESULT_TIMEOUT;
            }
            NetState_ClearWifiError();
            GFL_NetErrClear();
            NetState_ClearError();
            NetState_WifiDisconnect(TRUE);
            return DWCRAP_RESULT_OK;
        }
        if (type == 0x3f2 || type == 0x3f3 || type == 0x3f4 || type == 0x3f6) {
            if (a0 == 0) {
                DWCRapCommon_ShutdownLibrary();
                GFL_NetErrShowScreen();
                GFL_NetErrClear();
                return DWCRAP_RESULT_DISCONNECTED;
            }
            GFL_NetErrMarkShown();
            GFL_NetErrRecordWifi(13);
            return DWCRAP_RESULT_FAILED;
        }
        if (type == 0x3ee || type == 0x3f7 || type == 0x3f5) {
            GFL_NetErrMarkShown();
            GFL_NetErrRecordWifi(13);
            return DWCRAP_RESULT_FAILED;
        }
    }
    return DWCRAP_RESULT_OK;
}
