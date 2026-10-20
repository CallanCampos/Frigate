#include "dwc_util_connection.h"
#include "dwc_util_heap.h"
#include "dwc_util_shared.h"
#include "dwc_util_wifi_settings.h"
#include "dwcutil/dwcutil_ac.h"
#include "nitro/os.h"

static void DWCUtilConnection_OnStatus(const DWCUtilAcStatus *status);
static void *DWCUtilConnection_Alloc(u32 size);
static void DWCUtilConnection_Free(void *ptr);

// Defined in this file, which doesn't read it, since dwc_util_main_menu.c reaches its own .bss from the address of
// its first static
DWCUtilMsg *gDWCUtilMsg;
static DWCUtilAcStatus *sStatus;

void DWCUtilConnection_Start(void) {
    sStatus = DWCUtilHeap_AllocClear(sizeof(DWCUtilAcStatus), -4);
    if (DWCUtilAc_Start(0xf, 0x40, DWCUtilConnection_OnStatus, DWCUtilConnection_Alloc, DWCUtilConnection_Free,
                        0x800) != TRUE) {
        OS_Terminate();
    }
    OS_Sleep(10);
}

void DWCUtilConnection_Stop(void) {
    if (DWCUtilAc_Cancel() != TRUE) {
        OS_Terminate();
    }
    DWCUtilHeap_FreeAndNull(&sStatus);
}

int DWCUtilConnection_GetState(void) {
    DWCUtilAcResult result;

    switch (sStatus->state) {
    case 0:
    case 1:
    case 3:
    case 5:
        return 0;
    case 2:
        return 1;
    case 4:
        return 2;
    case 6:
        if (DWCUtilAc_GetResult(&result) != TRUE) {
            OS_Terminate();
        }
        if (result.wepMode >= 0 && result.wepMode <= 3 && result.keyCount == 1) {
            return 3;
        }
        return 5;
    case 7:
        return 4;
    }
}

void DWCUtilConnection_SaveResult(void) {
    DWCUtilAcResult result;

    if (DWCUtilAc_GetResult(&result) != TRUE) {
        OS_Terminate();
    }
    func_ov182_021b7c60(&result);
}

static void DWCUtilConnection_OnStatus(const DWCUtilAcStatus *status) {
    *sStatus = *status;
}

static void *DWCUtilConnection_Alloc(u32 size) {
    return DWCUtilHeap_AllocClear(size, 32);
}

static void DWCUtilConnection_Free(void *ptr) {
    DWCUtilHeap_Free(ptr);
}
