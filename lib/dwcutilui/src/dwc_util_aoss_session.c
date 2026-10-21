#include "dwc_util_aoss_session.h"
#include "dwc_util_heap.h"
#include "dwc_util_shared.h"
#include "dwc_util_wifi_settings.h"
#include "dwcutil/dwc_aoss.h"
#include "nitro/mi.h"
#include "nitro/os.h"

typedef struct DWCUtilAoss {
    // Whether the setup succeeded
    u8 done;
    AossParam *param;
} DWCUtilAoss;

static void *DWCUtilAossSession_Alloc(u32 size);
static void DWCUtilAossSession_Free(void *ptr);

static const char sClientName[] = "NINTENDO-DS";
static DWCUtilAoss sAoss;

void DWCUtilAossSession_Init(void) {
    AossClientInfo client;

    sAoss.param = DWCUtilHeap_AllocClear(sizeof(AossParam), 4);
    sAoss.done = FALSE;
    MI_CpuClear16(&client, sizeof(client));
    client.version = 0x50;
    client.length = sizeof(sClientName);
    MI_CpuCopy8(sClientName, client.data, sizeof(sClientName));
    sAoss.param->types = AOSS_TYPE_WEP40 | AOSS_TYPE_WEP104;
    sAoss.param->client = client;
    sAoss.param->scanCount = 1;
    sAoss.param->scanDelay = -1;
    sAoss.param->sendCount = 1;
    sAoss.param->sendDelay = -1;
    sAoss.param->timeout = -1;
    OS_GetMacAddress(sAoss.param->mac);
    if (DWC_Aoss_Init(DWCUtilAossSession_Alloc, DWCUtilAossSession_Free) != 0) {
        OS_Terminate();
    }
}

void DWCUtilAossSession_Exit(BOOL saveKeys) {
    DWC_Aoss_Shutdown();
    if (saveKeys && sAoss.param->error == 0 && sAoss.done == TRUE) {
        DWCUtilWifiSettings_SaveAossResult(&sAoss.param->result);
    }
    DWCUtilHeap_FreeAndNull(&sAoss.param);
}

int DWCUtilAossSession_Run(void) {
    u8 error;

    if (DWC_Aoss_Start(sAoss.param) == 0) {
        sAoss.done = TRUE;
        return 1;
    }
    error = sAoss.param->error;
    if (error == 1 || error == 3 || error == 4 || error == 5) {
        return 0;
    }
    return 2;
}

static void *DWCUtilAossSession_Alloc(u32 size) {
    return DWCUtilHeap_AllocClear(size, 32);
}

static void DWCUtilAossSession_Free(void *ptr) {
    DWCUtilHeap_Free(ptr);
}
