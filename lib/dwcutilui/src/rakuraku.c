#include "rakuraku.h"
#include "dwc_util_heap.h"
#include "dwc_util_shared.h"
#include "dwcutil/atermset.h"
#include "nitro/os.h"
#include "setting.h"

static void cbConfig(const ATERMApConfigState *status);
static void *memAlloc(u32 size);
static void memFree(void *ptr);

static ATERMApConfigState *sStatus;

void DWCi_RAKUlInit(void) {
    sStatus = DWCi_HEAPlAllocEx(sizeof(ATERMApConfigState), -4);
    if (ATERMi_ApConfigStart(0xf, 0x40, cbConfig, memAlloc, memFree, 0x800) != TRUE) {
        OS_Terminate();
    }
    OS_Sleep(10);
}

void DWCi_RAKUlEnd(void) {
    if (ATERMi_ApConfigEnd() != TRUE) {
        OS_Terminate();
    }
    DWCi_HEAPlFree_(&sStatus);
}

int DWCi_RAKUlGetState(void) {
    ATERMApConfigParam result;

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
        if (ATERMi_ApConfigGetResult(&result) != TRUE) {
            OS_Terminate();
        }
        if (result.auth_mode >= 0 && result.auth_mode <= 3 && result.wepkeyid == 1) {
            return 3;
        }
        return 5;
    case 7:
        return 4;
    }
}

void DWCi_RAKUlSetResult(void) {
    ATERMApConfigParam result;

    if (ATERMi_ApConfigGetResult(&result) != TRUE) {
        OS_Terminate();
    }
    DWCi_SETTINGlSetRaku(&result);
}

static void cbConfig(const ATERMApConfigState *status) {
    *sStatus = *status;
}

static void *memAlloc(u32 size) {
    return DWCi_HEAPlAllocEx(size, 32);
}

static void memFree(void *ptr) {
    DWCi_HEAPlFree2(ptr);
}
