// DWC's initialization of the Wi-Fi settings (TWL-DWC 5.3, bm). The file name is a guess after DWC_BM_Init, which this
// is; nothing in the ROM ties the function to dwc_backup.c or to the checks of dwc_bm_check.c, whose functions it
// calls.

#include "bm_internal.h"
#include "dwc/bm.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// DWC_BM_Init: checks the Wi-Fi settings, and repairs them where they can be, for the kind of console the game runs
// on, then keeps the Wi-Fi ID that they hold. The work area is 0x1000 bytes
int DWC_BM_Init(void *work) {
    int result;

    MI_CpuFill8(work, 0, DWC_BM_INIT_WORK_SIZE);
    if (OS_IsRunOnTwl()) {
        result = func_0205e0f0(work);
    } else {
        result = func_0205e094(work);
    }
    MI_CpuFill8(work, 0, DWC_BM_INIT_WORK_SIZE);
    if (!DWCBm_ReadAllNtrPages(work)) {
        return DWC_BM_INIT_READ_ERROR;
    }
    DWCi_BACKUPlSetWiFi(((DWCMemPage *)work)->wifi);
    return result;
}
