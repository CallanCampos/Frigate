#include "gfl/net_devwireless.h"
#include "types.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_whpipe.h"
#include "gfl/wih.h"
#include "nitro/wm.h"

// The 8-byte work that the device keeps while it runs
typedef struct {
    void *sys;
    // Called when the wireless has ended
    void (*endCallback)(BOOL ok);
} NetDevWireless;

static BOOL NetDevWireless_Init(HeapID heapId, void *sys, WHCallback callback, void *work);
static BOOL NetDevWireless_Begin(void (*callback)(BOOL ok));
static int NetDevWireless_Update(u16 connectBits);
static BOOL NetDevWireless_Unk10(int a0, int a1);
static BOOL NetDevWireless_End(void (*endCallback)(BOOL ok));
static BOOL NetDevWireless_ResetPipe(void);
static BOOL NetDevWireless_ReturnTrue1C(void);
static BOOL NetDevWireless_ReturnTrue20(void);
static BOOL NetDevWireless_ReturnTrue24(void);
static void *NetDevWireless_GetBeaconData(int index);
static u8 *NetDevWireless_GetBeaconMac(int index);
static BOOL NetDevWireless_ReturnTrue30(void);
static BOOL NetDevWireless_ReturnTrue34(void);
static BOOL NetDevWireless_Unk38(int a0);
static BOOL NetDevWireless_SetDisconnectCallback(void (*callback)(int netId));
static int NetDevWireless_Unk40(int a0, BOOL (*callback)(void));
static BOOL NetDevWireless_Unk44(int a0, int a1);
static int NetDevWireless_Unk48(BOOL a0, const u8 *mac, int index, int a3, void (*callback)(void));
static BOOL NetDevWireless_Unk4C(int a0, int a1, int a2, int a3);
static BOOL NetDevWireless_Unk54(BOOL a0, int a1);
static BOOL NetDevWireless_IsDataSharing(void);
static BOOL NetDevWireless_Unk5C(u8 *data);
static u8 *NetDevWireless_GetRecvData(int netId);
static BOOL NetDevWireless_Send(u8 *data, int size, int a2, GFLNetSendDoneFunc callback);
static BOOL NetDevWireless_SetRecvCallback(GFLNetRecvFunc callback);
static BOOL NetDevWireless_IsRunning(void);
static BOOL NetDevWireless_IsConnected(void);
static BOOL NetDevWireless_Unk74(void);
static BOOL NetDevWireless_IsIdle(void);
static int NetDevWireless_GetConnectBits(void);
static int NetDevWireless_GetNetId(void);
static int NetDevWireless_GetSignalLevel(void);
static int NetDevWireless_GetError(void);
static void NetDevWireless_Unk8C(int a0);
static void NetDevWireless_UnkB8(int a0);
static BOOL NetDevWireless_UnkBC(void);
static void NetDevWireless_UnkC0(int a0);

static NetDevWireless *sDevWireless;

static GFLNetDevTable sNetDevWirelessTable = {
    .init = NetDevWireless_Init,
    .start = NetDevWireless_Begin,
    .update = NetDevWireless_Update,
    .disconnect = NetDevWireless_Unk10,
    .end = NetDevWireless_End,
    .unk18 = NetDevWireless_ResetPipe,
    .unk1C = NetDevWireless_ReturnTrue1C,
    .unk20 = NetDevWireless_ReturnTrue20,
    .unk24 = NetDevWireless_ReturnTrue24,
    .getBeaconData = NetDevWireless_GetBeaconData,
    .getBeaconMac = NetDevWireless_GetBeaconMac,
    .unk30 = NetDevWireless_ReturnTrue30,
    .unk34 = NetDevWireless_ReturnTrue34,
    .hasBeacons = NetDevWireless_Unk38,
    .setDisconnectCallback = NetDevWireless_SetDisconnectCallback,
    .startParent = NetDevWireless_Unk40,
    .startScan = NetDevWireless_Unk44,
    .startChild = NetDevWireless_Unk48,
    .startMatch = NetDevWireless_Unk4C,
    .close = NetDevWireless_Unk54,
    .isDataSharing = NetDevWireless_IsDataSharing,
    .stepDataSharing = NetDevWireless_Unk5C,
    .getRecvData = NetDevWireless_GetRecvData,
    .send = NetDevWireless_Send,
    .setRecvCallback = NetDevWireless_SetRecvCallback,
    .isRunning = NetDevWireless_IsRunning,
    .isConnected = NetDevWireless_IsConnected,
    .isEnded = NetDevWireless_Unk74,
    .isIdle = NetDevWireless_IsIdle,
    .getConnectBits = NetDevWireless_GetConnectBits,
    .getNetId = NetDevWireless_GetNetId,
    .getSignalLevel = NetDevWireless_GetSignalLevel,
    .isError = NetDevWireless_GetError,
    .setDisconnectError = NetDevWireless_Unk8C,
    .setAcceptConnect = NetDevWireless_UnkB8,
    .startSearch = NetDevWireless_UnkBC,
    .holdScan = NetDevWireless_UnkC0,
};

const GFLNetDevTable *NetDevWireless_GetTable(void) {
    return &sNetDevWirelessTable;
}

static BOOL NetDevWireless_Init(HeapID heapId, void *sys, WHCallback callback, void *work) {
    BOOL flag;

    if (sDevWireless != NULL) {
        return FALSE;
    }
    sDevWireless = GFL_HeapAllocate(heapId, sizeof(NetDevWireless), TRUE, "net_devwireless.c", 180);
    sDevWireless->sys = sys;
    flag = TRUE;
    if (GFL_NetGetInitData()->bNetType != 5) {
        flag = FALSE;
    }
    return NetWhpipe_Init(heapId, callback, work, flag);
}

static BOOL NetDevWireless_Begin(void (*callback)(BOOL ok)) {
    if (callback != NULL) {
        callback(NetWhpipe_IsIdle());
    }
    return NetWhpipe_IsIdle();
}

static int NetDevWireless_Update(u16 connectBits) {
    NetWhpipe_Update(connectBits);
    return TRUE;
}

static BOOL NetDevWireless_Unk10(int a0, int a1) {
    return NetWhpipe_Finalize();
}

static BOOL NetDevWireless_Finish(BOOL success) {
    if (sDevWireless->endCallback != NULL) {
        sDevWireless->endCallback(success);
    }
    GFL_HeapFree(sDevWireless);
    sDevWireless = NULL;
    return TRUE;
}

static BOOL NetDevWireless_End(void (*endCallback)(BOOL ok)) {
    if (sDevWireless != NULL) {
        sDevWireless->endCallback = endCallback;
        return NetWhpipe_End(NetDevWireless_Finish);
    }
    return TRUE;
}

static BOOL NetDevWireless_ResetPipe(void) {
    NetWhpipe_ClearBeacons();
    return TRUE;
}

static BOOL NetDevWireless_ReturnTrue1C(void) {
    return TRUE;
}

static BOOL NetDevWireless_ReturnTrue20(void) {
    return TRUE;
}

static BOOL NetDevWireless_ReturnTrue24(void) {
    return TRUE;
}

static void *NetDevWireless_GetBeaconData(int index) {
    if (index >= 16) {
        return NULL;
    }
    return NetWhpipe_GetBeaconData(index);
}

static u8 *NetDevWireless_GetBeaconMac(int index) {
    if (index >= 16) {
        return NULL;
    }
    return NetWhpipe_GetBeaconMac(index);
}

static BOOL NetDevWireless_ReturnTrue30(void) {
    return TRUE;
}

static BOOL NetDevWireless_ReturnTrue34(void) {
    return TRUE;
}

static BOOL NetDevWireless_Unk38(int a0) {
    return NetWhpipe_HasBeacons();
}

static BOOL NetDevWireless_SetDisconnectCallback(void (*callback)(int netId)) {
    WH_SetConnectCallback(callback);
    return TRUE;
}

static int NetDevWireless_Unk40(int a0, BOOL (*callback)(void)) {
    return NetWhpipe_StartParent(a0);
}

static BOOL NetDevWireless_Unk44(int a0, int a1) {
    return NetWhpipe_StartScan(a0);
}

static int NetDevWireless_Unk48(BOOL a0, const u8 *mac, int index, int a3, void (*callback)(void)) {
    return NetWhpipe_StartChild(a0, mac, index, callback);
}

static BOOL NetDevWireless_Unk4C(int a0, int a1, int a2, int a3) {
    return FALSE;
}

static BOOL NetDevWireless_Unk54(BOOL a0, int a1) {
    return NetWhpipe_Finalize();
}

static BOOL NetDevWireless_IsDataSharing(void) {
    return WH_GetSystemState() == WH_SYSSTATE_DATASHARING;
}

static BOOL NetDevWireless_Unk5C(u8 *data) {
    return WH_StepDS(data);
}

static u8 *NetDevWireless_GetRecvData(int netId) {
    return WH_GetSharedDataAdr(netId);
}

static BOOL NetDevWireless_Send(u8 *data, int size, int a2, GFLNetSendDoneFunc callback) {
    return NetWhpipe_SendData(data, size, callback);
}

static BOOL NetDevWireless_SetRecvCallback(GFLNetRecvFunc callback) {
    NetWhpipe_SetRecvCallback(callback);
    return TRUE;
}

static BOOL NetDevWireless_IsRunning(void) {
    switch (WH_GetSystemState()) {
    case WH_SYSSTATE_IDLE:
    case WH_SYSSTATE_SCANNING:
    case WH_SYSSTATE_BUSY:
    case WH_SYSSTATE_CONNECTED:
    case WH_SYSSTATE_DATASHARING:
    case WH_SYSSTATE_KEYSHARING:
    case WH_SYSSTATE_MEASURECHANNEL:
    case WH_SYSSTATE_CONNECT_FAIL:
    case WH_SYSSTATE_ERROR:
    case WH_SYSSTATE_FATAL:
        return TRUE;
    }
    return FALSE;
}

static BOOL NetDevWireless_IsConnected(void) {
    return NetWhpipe_IsConnected();
}

static BOOL NetDevWireless_Unk74(void) {
    return !NetWhpipe_IsInitialized();
}

static BOOL NetDevWireless_IsIdle(void) {
    return WH_GetSystemState() == WH_SYSSTATE_IDLE;
}

static int NetDevWireless_GetConnectBits(void) {
    return NetWhpipe_GetBitmap();
}

static int NetDevWireless_GetNetId(void) {
    return NetWhpipe_GetAid();
}

static int NetDevWireless_GetSignalLevel(void) {
    if (GFL_NetGetGameCommandBase() == 3) {
        return 3 - WihDwc_GetLinkLevel();
    }
    return 3 - WM_GetLinkLevel();
}

static int NetDevWireless_GetError(void) {
    int error = WH_GetLastError();

    if (error == 5) {
        error = 0;
    }
    if (NetWhpipe_IsStopped() && error == 0) {
        error = 20;
    }
    return error;
}

static void NetDevWireless_Unk8C(int a0) {
    NetWhpipe_SetLockEnabled(a0);
}

static void NetDevWireless_UnkB8(int a0) {
    NetWhpipe_SetScanAcceptAll(a0);
}

static BOOL NetDevWireless_UnkBC(void) {
    return NetWhpipe_StartSearch();
}

static void NetDevWireless_UnkC0(int a0) {
    NetWhpipe_HoldScan(a0);
}
