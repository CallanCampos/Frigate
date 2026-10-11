// net_devwifi.c is the name from the ROM's own string. The GFL net's device for Wi-Fi: each slot of its table passes
// the call on to dwc_rap.c or dwc_rapcommon.c. The names are ours.

#include "gfl/net_devwifi.h"
#include "types.h"
#include "gfl/dwc_rap.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_state.h"

typedef struct {
    GFLNetInitData *pNetInit;
    u32 unk4;
} NetDevWifiWork;

static NetDevWifiWork *sNetDevWifiWork;

static void NetDevWifi_Prepare(int a0, int a1);
static BOOL NetDevWifi_Init(HeapID heapId, void *sys, BOOL (*callback)(BOOL success), void *work);
static BOOL NetDevWifi_Start(void (*callback)(BOOL ok));
static int NetDevWifi_Update(u16 connectBits);
static BOOL NetDevWifi_Unk10(int a0, int a1);
static BOOL NetDevWifi_Exit(void (*callback)(BOOL ok));
static BOOL NetDevWifi_Unk4C(int a0, int a1, int a2, int a3);
static BOOL NetDevWifi_Unk50(int a0, int a1, int a2);
static BOOL NetDevWifi_Unk54(BOOL a0, int a1);
static BOOL NetDevWifi_Send(u8 *data, int size, int a2, GFLNetSendDoneFunc callback);
static BOOL NetDevWifi_SetRecvCallback(GFLNetRecvFunc callback);
static BOOL NetDevWifi_Unk6C(void);
static BOOL NetDevWifi_IsConnected(void);
static BOOL NetDevWifi_Unk74(void);
static BOOL NetDevWifi_Unk78(void);
static int NetDevWifi_GetConnectBits(void);
static int NetDevWifi_GetNetId(void);
static int NetDevWifi_GetSignalLevel(void);
static BOOL NetDevWifi_IsError(void);
static void NetDevWifi_Unk8C(int a0);
static int NetDevWifi_Unk90(int a0);
static int NetDevWifi_Unk94(void);
static int NetDevWifi_Unk98(void);
static BOOL NetDevWifi_UnkAC(void);
static void NetDevWifi_UnkB8(int a0);

static GFLNetDevTable sNetDevWifiTable = {
    .unk00 = NetDevWifi_Prepare,
    .init = NetDevWifi_Init,
    .unk08 = NetDevWifi_Start,
    .update = NetDevWifi_Update,
    .unk10 = NetDevWifi_Unk10,
    .unk14 = NetDevWifi_Exit,
    .unk4C = NetDevWifi_Unk4C,
    .unk50 = NetDevWifi_Unk50,
    .unk54 = NetDevWifi_Unk54,
    .send = NetDevWifi_Send,
    .setRecvCallback = NetDevWifi_SetRecvCallback,
    .unk6C = NetDevWifi_Unk6C,
    .isConnected = NetDevWifi_IsConnected,
    .unk74 = NetDevWifi_Unk74,
    .unk78 = NetDevWifi_Unk78,
    .getConnectBits = NetDevWifi_GetConnectBits,
    .getNetId = NetDevWifi_GetNetId,
    .getSignalLevel = NetDevWifi_GetSignalLevel,
    .isError = NetDevWifi_IsError,
    .unk8C = NetDevWifi_Unk8C,
    .unk90 = NetDevWifi_Unk90,
    .unk94 = NetDevWifi_Unk94,
    .unk98 = NetDevWifi_Unk98,
    .unkAC = NetDevWifi_UnkAC,
    .unkB8 = NetDevWifi_UnkB8,
};

static void NetDevWifi_Prepare(int a0, int a1) {
    DWCRapCommon_StartLibrary(a0);
}

static BOOL NetDevWifi_Init(HeapID heapId, void *sys, BOOL (*callback)(BOOL success), void *work) {
    GFLNetInitData *pNetInit;

    if (sNetDevWifiWork != NULL) {
        return FALSE;
    }
    pNetInit = func_02042e84();
    DWCRapCommon_Create(pNetInit->wifiHeapId, pNetInit->wifiHeapSize);
    sNetDevWifiWork = GFL_HeapAllocate(heapId, sizeof(NetDevWifiWork), TRUE, "net_devwifi.c", 186);
    sNetDevWifiWork->pNetInit = sys;
    return TRUE;
}

static BOOL NetDevWifi_Start(void (*callback)(BOOL ok)) {
    BOOL result = DWCRap_Init(func_02042d48(), func_02042d64());

    if (callback != NULL) {
        callback(TRUE);
    }
    return result;
}

static int NetDevWifi_Update(u16 connectBits) {
    return DWCRap_Process(connectBits);
}

static BOOL NetDevWifi_Unk10(int a0, int a1) {
    if (DWCRap_RequestClose(a0 == 0)) {
        DWCRap_ResetToReady();
        return TRUE;
    }
    DWCRap_Process(a0);
    return TRUE;
}

static BOOL NetDevWifi_Exit(void (*callback)(BOOL ok)) {
    if (sNetDevWifiWork != NULL) {
        DWCRap_Shutdown();
        if (callback != NULL) {
            callback(TRUE);
        }
        GFL_HeapFree(sNetDevWifiWork);
        sNetDevWifiWork = NULL;
    }
    DWCRapCommon_Delete();
    return TRUE;
}

static BOOL NetDevWifi_Unk4C(int a0, int a1, int a2, int a3) {
    return DWCRap_StartMatch(a0, a1, a2, a3);
}

static BOOL NetDevWifi_Unk50(int a0, int a1, int a2) {
    return DWCRap_Connect(a0, a1, a2);
}

static BOOL NetDevWifi_Unk54(BOOL a0, int a1) {
    if (DWCRap_RequestClose(a0 == 0)) {
        DWCRap_ResetToReady();
        return TRUE;
    }
    return FALSE;
}

static BOOL NetDevWifi_Send(u8 *data, int size, int a2, GFLNetSendDoneFunc callback) {
    if (DWCRap_Send(data, size)) {
        if (callback != NULL) {
            callback(TRUE);
        }
        return TRUE;
    }
    return FALSE;
}

static BOOL NetDevWifi_SetRecvCallback(GFLNetRecvFunc callback) {
    DWCRap_SetRecvFuncs(callback, callback);
    return TRUE;
}

static BOOL NetDevWifi_Unk6C(void) {
    return DWCRap_IsInitialized();
}

static BOOL NetDevWifi_IsConnected(void) {
    if (DWCRap_GetNetId() != -1) {
        return TRUE;
    }
    return FALSE;
}

static BOOL NetDevWifi_Unk74(void) {
    BOOL result = DWCRap_IsEnding();

    if (!DWCRap_IsInitialized()) {
        result |= TRUE;
    }
    return result;
}

static BOOL NetDevWifi_Unk78(void) {
    return DWCRap_IsReady();
}

static int NetDevWifi_GetConnectBits(void) {
    return func_ov011_02160344();
}

static int NetDevWifi_GetNetId(void) {
    return DWCRap_GetNetId();
}

static int NetDevWifi_GetSignalLevel(void) {
    return 3 - func_0205b250();
}

static BOOL NetDevWifi_IsError(void) {
    int code;
    int type;
    GFLNetErrorInfo *info;
    int result = func_020583b0(&code, &type);

    if (result != 0) {
        func_020424ac(code, type, result, 0);
    }
    if (result == 0) {
        return 0;
    }
    if (code == 0) {
        return -10000;
    }
    info = func_02042540();
    return info->type != 0 ? info->type : code;
}

static void NetDevWifi_Unk8C(int a0) {
    DWCRap_SetReportError(a0);
}

static int NetDevWifi_Unk90(int a0) {
    if (DWCRap_GetFriendStatus(a0) == 6) {
        return TRUE;
    }
    return FALSE;
}

static int NetDevWifi_Unk94(void) {
    return func_ov011_02151dec();
}

static int NetDevWifi_Unk98(void) {
    return DWCRap_GetLastNetId();
}

static void NetDevWifi_UnkB8(int a0) {
    func_ov011_02151fec(a0);
}

static BOOL NetDevWifi_UnkAC(void) {
    return DWCRap_IsConnected();
}

const GFLNetDevTable *NetDevWifi_GetTable(void) {
    return &sNetDevWifiTable;
}
