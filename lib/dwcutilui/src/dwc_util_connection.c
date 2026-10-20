#include "dwc_util_connection.h"
#include "dwc_util_heap.h"
#include "dwc_util_shared.h"
#include "dwc_util_wifi_settings.h"
#include "dwcutil/dwcutil_ac.h"
#include "nitro/os.h"

static void func_ov182_021ae4cc(const DWCUtilAcStatus *status);
static void *func_ov182_021ae4e0(u32 size);
static void func_ov182_021ae4ec(void *ptr);

// Defined in this file, which doesn't read it, since dwc_util_main_menu.c reaches its own .bss from the address of
// its first static
DWCUtilMsg *data_ov182_021c40b8;
static DWCUtilAcStatus *sStatus;

void func_ov182_021ae3d8(void) {
    sStatus = func_ov182_021bc4d8(sizeof(DWCUtilAcStatus), -4);
    if (DWCUtilAc_Start(0xf, 0x40, func_ov182_021ae4cc, func_ov182_021ae4e0, func_ov182_021ae4ec, 0x800) != TRUE) {
        OS_Terminate();
    }
    OS_Sleep(10);
}

void func_ov182_021ae424(void) {
    if (DWCUtilAc_Cancel() != TRUE) {
        OS_Terminate();
    }
    func_ov182_021bc4f0(&sStatus);
}

int func_ov182_021ae440(void) {
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
        if (result.wepMode >= 0 && result.wepMode <= 3 && result.unk_24 == 1) {
            return 3;
        }
        return 5;
    case 7:
        return 4;
    }
}

void func_ov182_021ae4b0(void) {
    DWCUtilAcResult result;

    if (DWCUtilAc_GetResult(&result) != TRUE) {
        OS_Terminate();
    }
    func_ov182_021b7c60(&result);
}

static void func_ov182_021ae4cc(const DWCUtilAcStatus *status) {
    *sStatus = *status;
}

static void *func_ov182_021ae4e0(u32 size) {
    return func_ov182_021bc4d8(size, 32);
}

static void func_ov182_021ae4ec(void *ptr) {
    func_ov182_021bc51c(ptr);
}
