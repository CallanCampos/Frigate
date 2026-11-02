// net_devwifi.c is the name from the ROM's own string. The GFL net's device for Wi-Fi: each slot of its table passes
// the call on to dwc_rap.c or dwc_rapcommon.c. The names are ours: each function is named after the slot it fills
// (net.h's GFLNetDevTable), and those of the slots unk94 and unk98 after the dwc_rap.c function they return.

#include "gfl/net_devwifi.h"
#include "types.h"
#include "dwc/connectinet.h"
#include "dwc/error.h"
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

static void NetDevWifi_Prepare(int heapId, int a1);
static BOOL NetDevWifi_Init(HeapID heapId, GFLNetSys *sys, BOOL (*callback)(BOOL success), void *work);
static BOOL NetDevWifi_Start(void (*callback)(BOOL ok));
static int NetDevWifi_Update(u16 connectBits);
static BOOL NetDevWifi_Disconnect(int a0, int a1);
static BOOL NetDevWifi_Exit(void (*callback)(BOOL ok));
static BOOL NetDevWifi_StartMatch(int key, int numEntry, int a2, int a3);
static BOOL NetDevWifi_ConnectFriend(int friendIndex, int numEntry, int a2);
static BOOL NetDevWifi_Close(BOOL a0, int a1);
static BOOL NetDevWifi_Send(u8 *data, int size, int a2, GFLNetSendDoneFunc callback);
static BOOL NetDevWifi_SetRecvCallback(GFLNetRecvFunc callback);
static BOOL NetDevWifi_IsRunning(void);
static BOOL NetDevWifi_IsConnected(void);
static BOOL NetDevWifi_IsEnded(void);
static BOOL NetDevWifi_IsIdle(void);
static int NetDevWifi_GetConnectBits(void);
static int NetDevWifi_GetNetId(void);
static int NetDevWifi_GetSignalLevel(void);
static BOOL NetDevWifi_IsError(void);
static void NetDevWifi_SetDisconnectError(int report);
static int NetDevWifi_CanConnectFriend(int index);
static int NetDevWifi_IsVoiceChatEnabled(void);
static int NetDevWifi_GetLastNetId(void);
static BOOL NetDevWifi_IsLinked(void);
static void NetDevWifi_SetAcceptConnect(int accept);

static GFLNetDevTable sNetDevWifiTable = {
    .prepare = NetDevWifi_Prepare,
    .init = NetDevWifi_Init,
    .start = NetDevWifi_Start,
    .update = NetDevWifi_Update,
    .disconnect = NetDevWifi_Disconnect,
    .end = NetDevWifi_Exit,
    .startMatch = NetDevWifi_StartMatch,
    .connectFriend = NetDevWifi_ConnectFriend,
    .close = NetDevWifi_Close,
    .send = NetDevWifi_Send,
    .setRecvCallback = NetDevWifi_SetRecvCallback,
    .isRunning = NetDevWifi_IsRunning,
    .isConnected = NetDevWifi_IsConnected,
    .isEnded = NetDevWifi_IsEnded,
    .isIdle = NetDevWifi_IsIdle,
    .getConnectBits = NetDevWifi_GetConnectBits,
    .getNetId = NetDevWifi_GetNetId,
    .getSignalLevel = NetDevWifi_GetSignalLevel,
    .isError = NetDevWifi_IsError,
    .setDisconnectError = NetDevWifi_SetDisconnectError,
    .canConnectFriend = NetDevWifi_CanConnectFriend,
    .unk94 = NetDevWifi_IsVoiceChatEnabled,
    .unk98 = NetDevWifi_GetLastNetId,
    .isLinked = NetDevWifi_IsLinked,
    .setAcceptConnect = NetDevWifi_SetAcceptConnect,
};

static void NetDevWifi_Prepare(int heapId, int a1) {
    DWCRapCommon_StartLibrary(heapId);
}

static BOOL NetDevWifi_Init(HeapID heapId, GFLNetSys *sys, BOOL (*callback)(BOOL success), void *work) {
    GFLNetInitData *pNetInit;

    if (sNetDevWifiWork != NULL) {
        return FALSE;
    }
    pNetInit = GFL_NetGetInitData();
    DWCRapCommon_Create(pNetInit->wifiHeapId, pNetInit->wifiHeapSize);
    sNetDevWifiWork = GFL_HeapAllocate(heapId, sizeof(NetDevWifiWork), TRUE, "net_devwifi.c", 186);
    sNetDevWifiWork->pNetInit = &sys->aNetInit;
    return TRUE;
}

static BOOL NetDevWifi_Start(void (*callback)(BOOL ok)) {
    BOOL result = DWCRap_Init((DWCUserData *)GFL_NetGetUserData(), (DWCFriendData *)GFL_NetGetFriendData());

    if (callback != NULL) {
        callback(TRUE);
    }
    return result;
}

static int NetDevWifi_Update(u16 connectBits) {
    return DWCRap_Process(connectBits);
}

static BOOL NetDevWifi_Disconnect(int a0, int a1) {
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

static BOOL NetDevWifi_StartMatch(int key, int numEntry, int a2, int a3) {
    return DWCRap_StartMatch((const char *)key, numEntry, a2, a3);
}

static BOOL NetDevWifi_ConnectFriend(int friendIndex, int numEntry, int a2) {
    return DWCRap_Connect(friendIndex, numEntry, a2);
}

static BOOL NetDevWifi_Close(BOOL a0, int a1) {
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

static BOOL NetDevWifi_IsRunning(void) {
    return DWCRap_IsInitialized();
}

static BOOL NetDevWifi_IsConnected(void) {
    if (DWCRap_GetNetId() != -1) {
        return TRUE;
    }
    return FALSE;
}

static BOOL NetDevWifi_IsEnded(void) {
    BOOL result = DWCRap_IsEnding();

    if (!DWCRap_IsInitialized()) {
        result |= TRUE;
    }
    return result;
}

static BOOL NetDevWifi_IsIdle(void) {
    return DWCRap_IsReady();
}

static int NetDevWifi_GetConnectBits(void) {
    return DWC_GetAIDBitmap();
}

static int NetDevWifi_GetNetId(void) {
    return DWCRap_GetNetId();
}

static int NetDevWifi_GetSignalLevel(void) {
    return 3 - DWC_GetLinkLevel();
}

static BOOL NetDevWifi_IsError(void) {
    int code;
    DWCErrorType type;
    GFLNetErrorInfo *info;
    int result = DWC_GetLastErrorEx(&code, &type);

    if (result != 0) {
        NetState_RecordError(code, type, result, 0);
    }
    if (result == 0) {
        return 0;
    }
    if (code == 0) {
        return -10000;
    }
    info = NetState_GetErrorInfo();
    return info->type != 0 ? info->type : code;
}

static void NetDevWifi_SetDisconnectError(int report) {
    DWCRap_SetReportError(report);
}

static int NetDevWifi_CanConnectFriend(int index) {
    if (DWCRap_GetFriendStatus(index) == 6) {
        return TRUE;
    }
    return FALSE;
}

static int NetDevWifi_IsVoiceChatEnabled(void) {
    return DWCRap_IsVoiceChatEnabled();
}

static int NetDevWifi_GetLastNetId(void) {
    return DWCRap_GetLastNetId();
}

static void NetDevWifi_SetAcceptConnect(int accept) {
    DWCRap_SetAcceptConnect(accept);
}

static BOOL NetDevWifi_IsLinked(void) {
    return DWCRap_IsConnected();
}

const GFLNetDevTable *NetDevWifi_GetTable(void) {
    return &sNetDevWifiTable;
}
