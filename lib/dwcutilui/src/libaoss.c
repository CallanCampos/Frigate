#include "libaoss.h"
#include "dwc_util_heap.h"
#include "dwc_util_shared.h"
#include "dwcutil/aoss.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "setting.h"

typedef struct DWCUtilAoss {
    // Whether the setup succeeded
    u8 done;
    AOSS_INFO *param;
} DWCUtilAoss;

static void *aossAlloc(u32 size);
static void aossFree(void *ptr);

static const char sClientName[] = "NINTENDO-DS";
static DWCUtilAoss sAoss;

void DWCi_AOSSlInit(void) {
    PRODUCT_INFO client;

    sAoss.param = DWCi_HEAPlAllocEx(sizeof(AOSS_INFO), 4);
    sAoss.done = FALSE;
    MI_CpuClear16(&client, sizeof(client));
    client.dataType = 0x50;
    client.dataLen = sizeof(sClientName);
    MI_CpuCopy8(sClientName, client.productName, sizeof(sClientName));
    sAoss.param->supportSecurity = AOSS_TYPE_WEP40 | AOSS_TYPE_WEP104;
    sAoss.param->productInfo = client;
    sAoss.param->retryCount_Scan = 1;
    sAoss.param->retryInterval_Scan = -1;
    sAoss.param->retryCount_Comu = 1;
    sAoss.param->retryInterval_Comu = -1;
    sAoss.param->selectInterval = -1;
    OS_GetMacAddress(sAoss.param->macAddress);
    if (AOSSi_InitLocal(aossAlloc, aossFree) != 0) {
        OS_Terminate();
    }
}

void DWCi_AOSSlEnd(BOOL saveKeys) {
    AOSSi_EndLocal();
    if (saveKeys && sAoss.param->aossResult == 0 && sAoss.done == TRUE) {
        DWCi_SETTINGlSetAoss(&sAoss.param->secureInfo);
    }
    DWCi_HEAPlFree_(&sAoss.param);
}

int DWCi_AOSSlStart(void) {
    u8 error;

    if (AOSSi_Init(sAoss.param) == 0) {
        sAoss.done = TRUE;
        return 1;
    }
    error = sAoss.param->aossResult;
    if (error == 1 || error == 3 || error == 4 || error == 5) {
        return 0;
    }
    return 2;
}

static void *aossAlloc(u32 size) {
    return DWCi_HEAPlAllocEx(size, 32);
}

static void aossFree(void *ptr) {
    DWCi_HEAPlFree2(ptr);
}
