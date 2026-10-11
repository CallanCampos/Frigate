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
    .unk08 = NetDevWireless_Begin,
    .update = NetDevWireless_Update,
    .unk10 = NetDevWireless_Unk10,
    .unk14 = NetDevWireless_End,
    .unk18 = NetDevWireless_ResetPipe,
    .unk1C = NetDevWireless_ReturnTrue1C,
    .unk20 = NetDevWireless_ReturnTrue20,
    .unk24 = NetDevWireless_ReturnTrue24,
    .unk28 = NetDevWireless_GetBeaconData,
    .unk2C = NetDevWireless_GetBeaconMac,
    .unk30 = NetDevWireless_ReturnTrue30,
    .unk34 = NetDevWireless_ReturnTrue34,
    .unk38 = NetDevWireless_Unk38,
    .setDisconnectCallback = NetDevWireless_SetDisconnectCallback,
    .unk40 = NetDevWireless_Unk40,
    .unk44 = NetDevWireless_Unk44,
    .unk48 = NetDevWireless_Unk48,
    .unk4C = NetDevWireless_Unk4C,
    .unk54 = NetDevWireless_Unk54,
    .unk58 = NetDevWireless_IsDataSharing,
    .unk5C = NetDevWireless_Unk5C,
    .getRecvData = NetDevWireless_GetRecvData,
    .send = NetDevWireless_Send,
    .setRecvCallback = NetDevWireless_SetRecvCallback,
    .unk6C = NetDevWireless_IsRunning,
    .isConnected = NetDevWireless_IsConnected,
    .unk74 = NetDevWireless_Unk74,
    .unk78 = NetDevWireless_IsIdle,
    .getConnectBits = NetDevWireless_GetConnectBits,
    .getNetId = NetDevWireless_GetNetId,
    .getSignalLevel = NetDevWireless_GetSignalLevel,
    .isError = NetDevWireless_GetError,
    .unk8C = NetDevWireless_Unk8C,
    .unkB8 = NetDevWireless_UnkB8,
    .unkBC = NetDevWireless_UnkBC,
    .unkC0 = NetDevWireless_UnkC0,
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
    if (func_02042e84()->bNetType != 5) {
        flag = FALSE;
    }
    return func_ov030_02173340(heapId, callback, work, flag);
}

static BOOL NetDevWireless_Begin(void (*callback)(BOOL ok)) {
    if (callback != NULL) {
        callback(func_ov030_02173aa0());
    }
    return func_ov030_02173aa0();
}

static int NetDevWireless_Update(u16 connectBits) {
    func_ov030_02173964(connectBits);
    return TRUE;
}

static BOOL NetDevWireless_Unk10(int a0, int a1) {
    return func_ov030_021736b8();
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
        return func_ov030_02173e9c(NetDevWireless_Finish);
    }
    return TRUE;
}

static BOOL NetDevWireless_ResetPipe(void) {
    func_ov030_02173554();
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
    return func_ov030_02173b24(index);
}

static u8 *NetDevWireless_GetBeaconMac(int index) {
    if (index >= 16) {
        return NULL;
    }
    return func_ov030_02173b50(index);
}

static BOOL NetDevWireless_ReturnTrue30(void) {
    return TRUE;
}

static BOOL NetDevWireless_ReturnTrue34(void) {
    return TRUE;
}

static BOOL NetDevWireless_Unk38(int a0) {
    return func_ov030_02173bf4();
}

static BOOL NetDevWireless_SetDisconnectCallback(void (*callback)(int netId)) {
    WH_SetConnectCallback(callback);
    return TRUE;
}

static int NetDevWireless_Unk40(int a0, BOOL (*callback)(void)) {
    return func_ov030_02173cf8(a0);
}

static BOOL NetDevWireless_Unk44(int a0, int a1) {
    return func_ov030_021735b0(a0);
}

static int NetDevWireless_Unk48(BOOL a0, const u8 *mac, int index, int a3, void (*callback)(void)) {
    return func_ov030_02173e0c(a0, mac, index, callback);
}

static BOOL NetDevWireless_Unk4C(int a0, int a1, int a2, int a3) {
    return FALSE;
}

static BOOL NetDevWireless_Unk54(BOOL a0, int a1) {
    return func_ov030_021736b8();
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
    return func_ov030_02173c1c(data, size, callback);
}

static BOOL NetDevWireless_SetRecvCallback(GFLNetRecvFunc callback) {
    func_ov030_021733d0(callback);
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
    return func_ov030_021739ec();
}

static BOOL NetDevWireless_Unk74(void) {
    return !func_ov030_02173a8c();
}

static BOOL NetDevWireless_IsIdle(void) {
    return WH_GetSystemState() == WH_SYSSTATE_IDLE;
}

static int NetDevWireless_GetConnectBits(void) {
    return func_ov030_02173c28();
}

static int NetDevWireless_GetNetId(void) {
    return func_ov030_02173c30();
}

static int NetDevWireless_GetSignalLevel(void) {
    if (func_02042d34() == 3) {
        return 3 - func_02012ea0();
    }
    return 3 - func_020810fc();
}

static int NetDevWireless_GetError(void) {
    int error = WH_GetLastError();

    if (error == 5) {
        error = 0;
    }
    if (func_ov030_02173ae4() && error == 0) {
        error = 20;
    }
    return error;
}

static void NetDevWireless_Unk8C(int a0) {
    func_ov030_02173b04(a0);
}

static void NetDevWireless_UnkB8(int a0) {
    func_ov030_02173ec8(a0);
}

static BOOL NetDevWireless_UnkBC(void) {
    return func_ov030_02174040();
}

static void NetDevWireless_UnkC0(int a0) {
    func_ov030_021740f0(a0);
}

