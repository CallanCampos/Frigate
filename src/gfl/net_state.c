#include "types.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_irc_wireless.h"
#include "gfl/net_state.h"
#include "gfl/net_system.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/ui.h"
#include "gfl/wm_icon.h"
#include "nitro/math.h"

typedef struct NetState NetState;
typedef void (*NetStateFunc)(NetState *state);

// How the changeover alternates: 0 between parent and child, or it stays the parent or the child
enum {
    NET_STATE_MODE_PARENT = 1,
    NET_STATE_MODE_CHILD,
};

struct NetState {
    u8 unk0[8];
    NetStateFunc func;
    MATHRandContext32 rand;
    u8 unk24[4];
    // Called when the network has ended
    void (*endCallback)(void *work);
    // Called when the network is ready
    void (*connectCallback)(void *work);
    GFLNetErrorInfo error;
    u8 aMacAddress[6];
    // Frames to wait, which some states count down
    u16 timer;
    // The Wi-Fi friend to connect to, -1 for anybody and -2 for a match the caller started
    s8 friendIndex;
    u8 unk49;
    // Set when the other machine asked to disconnect (base command 11)
    u8 disconnectReceived;
    u8 unk4B;
    u8 unk4C;
    u8 errorFlag;
    s8 errorCode;
    u8 unk4F;
    // What the last call set the device's disconnect error to
    u8 disconnectError;
    // Whether ending the network has been requested
    u8 ending;
    u8 unk52;
    // Whether the changeover connects as a child to parentMac rather than scanning
    u8 useParentMac;
    u8 unk54[2];
    u8 mode;
    u8 parentMac[6];
    u8 unk5D[7];
};

static void NetState_SetFunc(NetState *state, NetStateFunc func, int timer);
static void NetState_SetFuncLine(NetState *state, NetStateFunc func, int timer, int line);
static void NetState_Alloc(HeapID heapId);
static void NetState_InitRand(MATHRandContext32 *rand);
static void NetState_FinishEnd(NetState *state);
static void func_02041238(NetState *state);
static void func_020412d0(NetState *state);
static void func_020412d4(void);
static void func_020412ec(NetState *state);
static void func_02041324(NetState *state);
static void func_020413c4(NetState *state);
static void func_020413c8(NetState *state);
static void func_02041440(NetState *state);
static void func_02041444(NetState *state);
static BOOL func_02041448(void);
static void func_02041468(NetState *state);
static void func_020414a0(HeapID heapId);
static u32 NetState_GetShortWait(NetState *state);
static u32 NetState_GetLongWait(NetState *state);
static void func_02041534(NetState *state);
static void func_020415bc(NetState *state);
static void func_020415ec(NetState *state);
static void func_02041624(NetState *state);
static void func_0204165c(NetState *state);
static void func_020416c8(NetState *state);
static void func_02041750(NetState *state);
static BOOL func_02041774(void);
static void func_02041798(NetState *state);
static void func_020417e8(NetState *state);
static void func_02041810(NetState *state);
static void func_02041854(NetState *state);
static void func_020418a0(NetState *state);
static void func_02041910(NetState *state);
static void func_02041940(NetState *state);
static void func_020419a8(NetState *state);
static void func_02041af4(NetState *state);
static void func_02041b70(int mode, int a1, const u8 *mac);
static void func_02041bc8(NetState *state);
static void NetState_ExitIfAlone(NetState *state);
static void func_02041d20(NetState *state);
static void func_02041d70(NetState *state);
static void NetState_Idle(NetState *state);
static void func_02041e30(NetState *state);
static void func_02041e34(NetState *state);
static void func_02041e38(NetState *state);
static void func_02041e3c(NetState *state);
static void NetState_WifiConnected(NetState *state);
static void NetState_SetErrorCode(NetState *state, int code);
static void NetState_WifiConnecting(NetState *state);
static void func_02041f78(NetState *state);
static void func_02042108(NetState *state);
static void func_0204212c(NetState *state);
static void func_02042264(NetState *state);
static void func_020422cc(NetState *state);
static void func_02042350(NetState *state);
static void func_020423ac(NetState *state);
static void func_02042450(void);
static void func_02042568(NetState *state);

static NetState *_pNetState;

static void NetState_SetFunc(NetState *state, NetStateFunc func, int timer) {
    state->func = func;
    state->timer = timer;
}

// Changes the state, from a line that the debug build reported
static void NetState_SetFuncLine(NetState *state, NetStateFunc func, int timer, int line) {
    NetState_SetFunc(state, func, timer);
}

static void NetState_Alloc(HeapID heapId) {
    // clang-format off
    GFL_ASSERT(NULL==_pNetState);
    // clang-format on
    _pNetState = GFL_HeapAllocate(heapId, sizeof(NetState), TRUE, "net_state.c", 185);
    _pNetState->timer = 50;
    _pNetState->unk4C = TRUE;
    NetState_InitRand(&_pNetState->rand);
    NetCommand_Init(0, NULL, 0, NULL, heapId);
}

static void NetState_InitRand(MATHRandContext32 *rand) {
    u64 high = GFL_RandomMT();
    u64 low = GFL_RandomMT();

    MATH_InitRand32(rand, (high << 32) + low);
}

// Ends the network once the device has
static void NetState_FinishEnd(NetState *state) {
    void (*callback)(void *work);
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->pDevTable->isEnded()) {

        NetCommand_Exit();
        WmIcon_Delete();
        NetSystem_Free();
        callback = _pNetState->endCallback;
        GFL_HeapFree(_pNetState);
        _pNetState = NULL;
        if (callback != NULL) {
            callback(NULL);
        }
    }
}

static void func_02041238(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();
    GFLNetInitData *ini = GFL_NetGetInitData();

    if (ini->bNetType != GFL_NET_TYPE_WIFI && ini->bNetType != GFL_NET_TYPE_WIFI_LOBBY) {
        sys->pDevTable->init(ini->heapId, sys, 0, sys->devWork);
    }
    NetSystem_ResetBuffersIfAllocated();
    if (state->connectCallback != NULL) {
        state->connectCallback(GFL_NetGetWork());
        state->connectCallback = NULL;
    }
    NetState_SetFuncLine(_pNetState, NetState_Idle, 0, 293);
}

void NetState_Start(HeapID heapId, void (*callback)(void *work)) {
    GCTX_HIDBlockSleep(4);
    NetState_Alloc(heapId);
    _pNetState->connectCallback = callback;
    NetState_SetFuncLine(_pNetState, func_02041238, 0, 309);
}

static void func_020412d0(NetState *state) {
}

static void func_020412d4(void) {
    _pNetState->timer += 60;
}

static void func_020412ec(NetState *state) {
    if (func_0203eec0(FALSE, state->aMacAddress, 0x200, func_020412d4)) {
        NetState_SetFuncLine(_pNetState, func_020412d0, 10, 351);
    }
}

static void func_02041324(NetState *state) {
    GFL_NetGetInitData();
    if (func_0203ede0(1, 0x200)) {
        NetState_SetFuncLine(_pNetState, func_020412ec, 0, 370);
    }
}

void NetState_ConnectMac(const u8 *mac, BOOL parent) {
    // clang-format off
    GFL_ASSERT(((u32)_pNetState->aMacAddress%2)==0);
    // clang-format on
    if (mac != NULL) {
        MI_CpuCopy8(mac, _pNetState->aMacAddress, sizeof(_pNetState->aMacAddress));
    }
    if (parent) {
        NetState_SetFuncLine(_pNetState, func_02041324, 0, 390);
    } else {
        NetState_SetFuncLine(_pNetState, func_020412ec, 0, 393);
    }
}

static void func_020413c4(NetState *state) {
}

static void func_020413c8(NetState *state) {
    if (func_0203ede0(1, 0x200)) {
        NetState_SetFuncLine(_pNetState, func_020413c4, 0, 420);
    }
}

void func_020413f0(void) {
    NetState_SetFuncLine(_pNetState, func_020413c8, 0, 434);
}

BOOL NetState_Update(void) {
    if (_pNetState == NULL) {
        return TRUE;
    }
    if (_pNetState->func != NULL) {
        _pNetState->func(_pNetState);
    }
    if (_pNetState == NULL) {
        return TRUE;
    }
    func_02042450();
    return FALSE;
}

static void func_02041440(NetState *state) {
}

static void func_02041444(NetState *state) {
}

static BOOL func_02041448(void) {
    NetState_SetFuncLine(_pNetState, func_02041440, 0, 490);
    return TRUE;
}

static void func_02041468(NetState *state) {
    GFL_NetGetInitData();
    if (func_0203ed68(1, 0x200, 1, func_02041448)) {
        NetState_SetFuncLine(_pNetState, func_02041444, 0, 507);
    }
}

static void func_020414a0(HeapID heapId) {
    NetState_SetFuncLine(_pNetState, func_02041444, 0, 522);
}

void func_020414c0(HeapID unused) {
    NetState_SetFuncLine(_pNetState, func_02041468, 0, 536);
}

static u32 NetState_GetShortWait(NetState *state) {
    return MATH_Rand32(&state->rand, 40) + 60;
}

static u32 NetState_GetLongWait(NetState *state) {
    return MATH_Rand32(&state->rand, 32) + 180;
}

static void func_02041534(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->pDevTable->isIdle()) {
        if (state->useParentMac && state->mode != NET_STATE_MODE_PARENT) {
            if (func_0203eec0(TRUE, state->parentMac, 0x200, func_020412d4)) {
                NetState_SetFuncLine(_pNetState, func_02041940, NetState_GetShortWait(state), 571);
            }
        } else if (func_0203ede0(1, 0x200)) {
            NetState_SetFuncLine(_pNetState, func_02041940, NetState_GetLongWait(state), 576);
        }
    }
}

static void func_020415bc(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->pDevTable->isIdle()) {
        NetState_SetFuncLine(_pNetState, func_02041534, 0, 595);
    }
}

static void func_020415ec(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->pDevTable->disconnect(1, 0)) {
        NetState_SetFuncLine(_pNetState, func_020415bc, 0, 603);
    }
}

static void func_02041624(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->pDevTable->disconnect(1, 0)) {
        NetState_SetFuncLine(_pNetState, func_02041534, 0, 632);
    }
}

static void func_0204165c(NetState *state) {
    GFL_NetGetInitData();
    if (NetSystem_HasError()) {
        return;
    }
    if (NetSystem_GetChildBits() == 0) {
        NetState_SetFuncLine(_pNetState, func_02041624, 0, 653);
        return;
    }
    if (NetHandle_CountNegotiated() && NetHandle_SendNegotiation()) {
        if (state->connectCallback != NULL) {
            state->connectCallback(GFL_NetGetWork());
            state->connectCallback = NULL;
        }
        NetState_SetFuncLine(_pNetState, NetState_Idle, 0, 668);
    }
}

static void func_020416c8(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (NetSystem_GetChildBits()) {
        func_020414a0(GFL_NetGetInitData()->heapId);
        func_02042e18();
        NetState_SetFuncLine(_pNetState, func_0204165c, 0, 695);
        return;
    }
    if (state->mode != NET_STATE_MODE_PARENT && sys->pDevTable->hasBeacons(0)) {
        if (state->timer != 0) {
            state->timer--;
        } else {
            NetState_SetFuncLine(_pNetState, func_020415ec, 0, 713);
        }
    }
}

static void func_02041750(NetState *state) {
    NetState_SetFuncLine(_pNetState, func_020416c8, NetState_GetLongWait(state), 726);
}

static BOOL func_02041774(void) {
    NetState_SetFuncLine(_pNetState, func_020416c8, NetState_GetLongWait(_pNetState), 732);
    return TRUE;
}

static void func_02041798(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->pDevTable->isIdle() && func_0203ed68(0, 0x200, 0, func_02041774)) {
        state->unk4C = FALSE;
        NetState_SetFuncLine(_pNetState, func_02041750, 30, 754);
    }
}

static void func_020417e8(NetState *state) {
    GFL_NetGetInitData();
    if (NetHandle_SendNegotiation()) {
        NetState_SetFuncLine(_pNetState, func_02041810, 0, 772);
    }
}

static void func_02041810(NetState *state) {
    GFL_NetGetInitData();
    if (NetHandle_IsNegotiated(NetHandle_GetOwn())) {
        if (state->connectCallback != NULL) {
            state->connectCallback(GFL_NetGetWork());
            state->connectCallback = NULL;
        }
        NetState_SetFuncLine(_pNetState, NetState_Idle, 0, 795);
    }
}

static void func_02041854(NetState *state) {
    GFL_NetGetInitData();
    NetHandle_Get(GFL_NET_NETID_SERVER);
    NetHandle_GetOwn();
    if (NetHandle_CountNegotiated() >= 2) {
        NetSystem_GetNetId();
        if (state->connectCallback != NULL) {
            state->connectCallback(GFL_NetGetWork());
            state->connectCallback = NULL;
        }
        NetState_SetFuncLine(_pNetState, NetState_Idle, 0, 821);
    }
}

static void func_020418a0(NetState *state) {
    GFLNetSys *sys;

    GFL_NetGetInitData();
    sys = GFL_NetGetSys();
    if (sys->pDevTable->isPeerSeen() == TRUE && NetHandle_SendNegotiation()) {
        if (NetIw_IsRunning() == TRUE) {
            NetState_SetFuncLine(_pNetState, func_02041854, 0, 842);
            return;
        }
        if (state->connectCallback != NULL) {
            state->connectCallback(GFL_NetGetWork());
            state->connectCallback = NULL;
        }
        NetState_SetFuncLine(_pNetState, NetState_Idle, 0, 849);
    }
}

static void func_02041910(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->pDevTable->isIdle()) {
        NetState_SetFuncLine(_pNetState, func_02041798, 0, 871);
    }
}

static void func_02041940(NetState *state) {
    if (NetSystem_GetNetId() != 0) {
        NetState_SetFuncLine(_pNetState, func_020417e8, 0, 889);
        return;
    }
    if (state->mode == NET_STATE_MODE_CHILD) {
        return;
    }
    if (state->timer != 0) {
        state->timer--;
        return;
    }
    NetState_SetFuncLine(_pNetState, func_02041bc8, 0, 902);
    func_02041bc8(state);
}

static void func_020419a8(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->pDevTable->isLinked() == TRUE) {
        sys->pDevTable->startLink();
        NetState_SetFuncLine(_pNetState, func_020418a0, 0, 923);
        return;
    }
    if (state->timer == 0) {
        state->timer = MATH_Rand32(&state->rand, 30) + 15;
        sys->pDevTable->start(0);
    } else {
        state->timer--;
    }
}

void func_02041a30(int a0, void (*callback)(void *work), int a2) {
    GFLNetInitData *ini = GFL_NetGetInitData();

    GFL_NetGetSys();
    func_02041b70(0, a2, NULL);
    if (ini->bNetType == 3 || ini->bNetType == 4) {
        NetSystem_StartInfrared(0);
        _pNetState->connectCallback = callback;
        NetState_SetFuncLine(_pNetState, func_020419a8, MATH_Rand32(&_pNetState->rand, 30), 956);
    } else {
        if (_pNetState->useParentMac && _pNetState->mode != NET_STATE_MODE_PARENT) {
            func_0203eec0(TRUE, _pNetState->parentMac, 0x200, func_020412d4);
        } else {
            func_0203ede0(1, 0x200);
        }
        _pNetState->connectCallback = callback;
        NetState_SetFuncLine(_pNetState, func_02041940, 30, 968);
    }
}

static void func_02041af4(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (_pNetState->mode == NET_STATE_MODE_CHILD && _pNetState->func == func_02041940 &&
        sys->pDevTable->disconnect(1, 0)) {
        NetState_SetFuncLine(_pNetState, func_02041910, 0, 982);
    }
    if (_pNetState->mode == NET_STATE_MODE_PARENT && _pNetState->func == func_020416c8) {
        NetState_SetFuncLine(_pNetState, func_020415ec, 0, 991);
    }
}

static void func_02041b70(int mode, int a1, const u8 *mac) {
    GFL_NetGetSys();
    _pNetState->useParentMac = a1;
    _pNetState->mode = mode;
    if (mac != NULL) {
        MI_CpuCopy8(mac, _pNetState->parentMac, sizeof(_pNetState->parentMac));
    } else {
        MI_CpuFill8(_pNetState->parentMac, 0xff, sizeof(_pNetState->parentMac));
    }
    NetState_SetFuncLine(_pNetState, func_02041af4, 0, 1020);
    func_02041af4(_pNetState);
}

static void func_02041bc8(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->pDevTable->disconnect(1, 0)) {
        NetState_SetFuncLine(_pNetState, func_02041910, 0, 1031);
    }
}

void func_02041c00(int mode, int a1, const u8 *mac) {
    GFL_NetGetSys();
    _pNetState->useParentMac = a1;
    _pNetState->mode = mode;
    if (mac != NULL) {
        MI_CpuCopy8(mac, _pNetState->parentMac, sizeof(_pNetState->parentMac));
    } else {
        MI_CpuFill8(_pNetState->parentMac, 0xff, sizeof(_pNetState->parentMac));
    }
    if (_pNetState->mode == NET_STATE_MODE_CHILD) {
        NetState_SetFuncLine(_pNetState, func_020415ec, 0, 1062);
    }
    if (_pNetState->mode == NET_STATE_MODE_PARENT) {
        NetState_SetFuncLine(_pNetState, func_02041bc8, 0, 1070);
        func_02041bc8(_pNetState);
    }
}

static void NetState_ExitIfAlone(NetState *state) {
    GFLNetInitData *ini = GFL_NetGetInitData();
    GFLNetSys *sys = GFL_NetGetSys();

    if (ini->bNetType == 3 || ini->bNetType == 4) {
        if (!sys->pDevTable->isLinked()) {
            GFL_NetExit(0);
        }
    } else if (!(GFL_NetGetSys()->pDevTable->getConnectBits() & 0xfffe)) {
        GFL_NetExit(0);
    }
}

void NetState_RecvExit(int netId, int size, const void *data, void *work, NetHandle *handle) {
    GFL_NetSendCommand(handle, 2, 0, NULL);
    if (NetSystem_GetNetId() == 0) {
        NetState_SetFuncLine(_pNetState, NetState_ExitIfAlone, 0, 1122);
    }
}

void NetState_RecvExitReply(int netId, int size, const void *data, void *work, NetHandle *handle) {
    if (NetSystem_GetNetId() != 0) {
        GFL_NetExit(0);
    }
}

static void func_02041d20(NetState *state) {
    GFLNetSys *sys;

    GFL_NetGetInitData();
    sys = GFL_NetGetSys();
    if (sys->pDevTable->isIdle()) {
        if (sys->pDevTable->end(0)) {
            NetState_FinishEnd(state);
        }
    } else if (!sys->pDevTable->isRunning() && sys->pDevTable->end(0)) {
        NetState_FinishEnd(state);
    }
}

static void func_02041d70(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->pDevTable->disconnect(1, 0)) {
        NetState_SetFuncLine(_pNetState, func_02041d20, 0, 1178);
    }
}

void NetState_End(void (*callback)(void *work)) {
    // BUG: When the network is not running, this writes through a NULL pointer
#ifdef BUGFIX
    if (_pNetState != NULL && !_pNetState->ending) {
#else
    if (_pNetState == NULL || !_pNetState->ending) {
#endif
        _pNetState->endCallback = callback;
        _pNetState->ending = TRUE;
        NetState_SetFuncLine(_pNetState, func_02041d70, 5, 1197);
    }
}

void NetState_CancelEnd(void) {
    if (_pNetState != NULL) {
        _pNetState->ending = FALSE;
    }
}

static void NetState_Idle(NetState *state) {
}

void func_02041dfc(void) {
    func_0203ee50(0x200);
    NetState_SetFuncLine(_pNetState, NetState_Idle, 0, 1232);
}

void NetState_RecvMPMode(int netId, int size, const void *data, void *work, NetHandle *handle) {
    GFL_NetGetInitData()->bMPMode = *(u8 *)data;
}

static void func_02041e30(NetState *state) {
}

static void func_02041e34(NetState *state) {
}

static void func_02041e38(NetState *state) {
}

static void func_02041e3c(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->pDevTable->update(0) < 0) {
        NetState_SetFuncLine(_pNetState, func_02041e30, 0, 1314);
    }
}

static void NetState_WifiConnected(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();

    func_02040104(1);
    sys->pDevTable->update(0);
}

static void NetState_SetErrorCode(NetState *state, int code) {
    if ((u32)(code - 1005) > 1) {
        code = -code;
    }
    state->errorCode = code;
}

static void NetState_WifiConnecting(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();
    int result = sys->pDevTable->update(0);

    if (result == 1000) {
        NetState_SetFuncLine(_pNetState, NetState_WifiConnected, 0, 1393);
        return;
    }
    if (sys->pDevTable->isLinked()) {
        NetState_SetFuncLine(_pNetState, NetState_WifiConnected, 0, 1397);
        return;
    }
    if (result == 1007) {
        return;
    }
    if (result == 1001) {
        NetState_SetFuncLine(_pNetState, func_02041e34, 0, 1403);
        return;
    }
    if (result == 1002) {
        NetState_SetFuncLine(_pNetState, func_02041e38, 0, 1406);
        return;
    }
    if (result >= 10) {
        NetState_SetFuncLine(_pNetState, func_02041e38, 0, 1410);
        return;
    }
    if (result < 0) {
        NetState_SetErrorCode(state, result);
        NetState_SetFuncLine(_pNetState, func_02041e30, 0, 1414);
    }
}

static void func_02041f78(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();
    GFLNetInitData *ini = GFL_NetGetInitData();

    if (!sys->pDevTable->connectFriend(state->friendIndex, ini->maxConnectNum, 0)) {
        NetSystem_ResetBuffersIfAllocated();
        NetHandle_FreeAll(sys);
        NetHandle_InitAll(sys);
        NetState_SetFuncLine(_pNetState, NetState_WifiConnecting, 0, 1442);
    }
}

BOOL func_02041fd0(int a0) {
    if (a0 != -1 && !func_0203ecac(a0)) {
        return FALSE;
    }
    _pNetState->friendIndex = a0;
    NetState_SetFuncLine(_pNetState, func_02041f78, 0, 1466);
    return TRUE;
}

BOOL func_0204200c(void) {
    GFLNetSys *sys = GFL_NetGetSys();

    _pNetState->friendIndex = -2;
    NetSystem_ResetBuffersIfAllocated();
    NetHandle_FreeAll(sys);
    NetHandle_InitAll(sys);
    NetState_SetFuncLine(_pNetState, NetState_WifiConnecting, 0, 1487);
    return TRUE;
}

BOOL func_0204204c(void) {
    GFLNetSys *sys = GFL_NetGetSys();

    _pNetState->friendIndex = -2;
    NetSystem_ResetBuffersIfAllocated();
    NetHandle_FreeAll(sys);
    NetHandle_InitAll(sys);
    NetState_SetFuncLine(_pNetState, NetState_WifiConnecting, 0, 1523);
    return TRUE;
}

void func_0204208c(void) {
    _pNetState->friendIndex = -1;
    NetState_SetFuncLine(_pNetState, func_02041f78, 0, 1539);
}

int NetState_GetWifiStatus(void) {
    NetStateFunc func = _pNetState->func;

    if (func == NetState_WifiConnecting) {
        return 0;
    }
    if (func == NetState_WifiConnected) {
        return 1;
    }
    if (func == func_02041e3c) {
        return 3;
    }
    if (func == func_02041e34) {
        return 4;
    }
    if (func == func_02041e38) {
        return 5;
    }
    return 2;
}

static void func_02042108(NetState *state) {
    GFL_NetGetSys();
    NetState_SetFuncLine(_pNetState, NetState_WifiConnecting, 0, 1581);
}

static void func_0204212c(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();
    BOOL a0 = FALSE;

    func_02040104(0);
    if (!state->unk4B) {
        a0 = TRUE;
    }
    if (sys->pDevTable->close(a0, 0)) {
        NetSystem_ResetBuffersIfAllocated();
        NetHandle_FreeAll(sys);
        NetHandle_InitAll(sys);
        NetState_SetFuncLine(_pNetState, func_02042108, 0, 1605);
        return;
    }
    if (sys->pDevTable->update(0) < 0) {
        NetState_SetFuncLine(_pNetState, func_02041e30, 0, 1611);
    }
}

void func_020421ac(BOOL a0) {
    if (_pNetState != NULL) {
        if (a0) {
            if (NetSystem_GetNetId() == 0) {
                _pNetState->unk4B = FALSE;
            } else {
                _pNetState->unk4B = TRUE;
            }
        } else {
            _pNetState->unk4B = FALSE;
        }
        NetState_SetFuncLine(_pNetState, func_0204212c, 0, 1639);
        func_0204212c(_pNetState);
    }
}

void func_020421f8(int netId, int size, const void *data, void *work, NetHandle *handle) {
    func_020421ac(TRUE);
    _pNetState->disconnectReceived = TRUE;
}

u8 func_02042210(void) {
    return _pNetState->disconnectReceived;
}

BOOL NetState_IsWifiConnecting(void) {
    if (_pNetState->func == NetState_WifiConnecting) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_0204223c(void) {
    if (_pNetState->func == NetState_WifiConnecting || _pNetState->func == func_02042108) {
        return TRUE;
    }
    return FALSE;
}

static void func_02042264(NetState *state) {
    GFLNetSys *sys;

    GFL_NetGetInitData();
    sys = GFL_NetGetSys();
    sys->pDevTable->update(1);
    if (sys->pDevTable->isIdle()) {
        sys->pDevTable->end(0);
        NetState_FinishEnd(state);
        return;
    }
    if (!sys->pDevTable->isRunning()) {
        sys->pDevTable->end(0);
        NetState_FinishEnd(state);
        return;
    }
    if (sys->pDevTable->isEnded()) {
        sys->pDevTable->end(0);
        NetState_FinishEnd(state);
    }
}

static void func_020422cc(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();

    sys->pDevTable->update(0);
    if (sys->pDevTable->disconnect(1, 0)) {
        NetState_SetFuncLine(_pNetState, func_02042264, 0, 1784);
    }
}

void func_0204230c(void (*callback)(void *work)) {
    if (_pNetState != NULL) {
        GFLNetSys *sys = GFL_NetGetSys();

        GCTX_HIDUnblockSoftReset(1);
        _pNetState->endCallback = callback;
        sys->pDevTable->close(TRUE, 0);
        NetState_SetFuncLine(_pNetState, func_020422cc, 5, 1807);
    }
}

static void func_02042350(NetState *state) {
    GFLNetSys *sys = GFL_NetGetSys();
    int result = sys->pDevTable->update(0);

    if (result < 0) {
        NetState_SetErrorCode(state, result);
        NetState_SetFuncLine(_pNetState, func_02041e30, 0, 1828);
        return;
    }
    if (result > 0) {
        NetState_SetFuncLine(_pNetState, func_02042108, 0, 1837);
    }
}

static void func_020423ac(NetState *state) {
    if (NetSystem_StartWireless(0x200, GFL_NetGetInitData()->heapId)) {
        NetState_SetFuncLine(_pNetState, func_02042350, 0, 1856);
    }
}

void func_020423e0(void) {
    if (!NetSystem_IsDeviceReady()) {
        GFL_NetGetInitData();
        GCTX_HIDBlockSoftReset(1);
        NetState_SetFuncLine(_pNetState, func_020423ac, 0, 1878);
    }
}

void func_02042410(u8 a0) {
    if (_pNetState != NULL) {
        _pNetState->unk4F = a0;
    }
}

void func_02042424(int a0) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (_pNetState != NULL) {
        _pNetState->disconnectError = a0;
    }
    sys->pDevTable->setDisconnectError(a0);
}

static void func_02042450(void) {
}

BOOL NetState_SetError(u8 a0) {
    if (_pNetState != NULL) {
        _pNetState->errorFlag = a0;
        NetSystem_SetError();
        func_02040180();
        return TRUE;
    }
    return FALSE;
}

void NetState_ClearError(void) {
    if (_pNetState != NULL) {
        _pNetState->errorFlag = 0;
        NetSystem_ClearError();
    }
}

u8 NetState_GetError(void) {
    if (_pNetState != NULL) {
        return _pNetState->errorFlag;
    }
    return 0;
}

void NetState_RecordError(int code, u32 errorType, u32 error, int type) {
    NetState *state = _pNetState;

    if (state != NULL) {
        if ((u32)(code - 1005) > 1) {
            code = -code;
        }
        state->error.code = code;
        _pNetState->error.errorType = errorType;
        _pNetState->error.error = error;
        _pNetState->error.type = type;
        if (type) {
            NetSystem_SetError();
        }
    }
}

void NetState_ClearWifiError(void) {
    if (_pNetState != NULL) {
        GFLNetInitData *pNetIni = GFL_NetGetInitData();
        BOOL isWifi = pNetIni->bNetType == GFL_NET_TYPE_WIFI || pNetIni->bNetType == GFL_NET_TYPE_WIFI_LOBBY ||
                      pNetIni->bNetType == GFL_NET_TYPE_WIFI_GTS;

        if (!isWifi) {
            GFL_DebugAssertFail("", 0,
                                "pNetIni->bNetType == GFL_NET_TYPE_WIFI || pNetIni->bNetType==GFL_NET_TYPE_WIFI_LOBBY "
                                "|| pNetIni->bNetType==GFL_NET_TYPE_WIFI_GTS");
        }
        _pNetState->error.code = 0;
        _pNetState->error.errorType = 0;
        _pNetState->error.error = 0;
        _pNetState->error.type = 0;
    }
}

GFLNetErrorInfo *NetState_GetErrorInfo(void) {
    GFL_ASSERT(_pNetState);
    return &_pNetState->error;
}

static void func_02042568(NetState *state) {
    GFL_DebugAssertFail("", 0, "0 && \"_wifiBattleError\x81\x46\x96\xa2\x8d\xec\x90\xac\\n\"");
}

BOOL NetState_IsWifiBattleError(void) {
    if (_pNetState != NULL && _pNetState->func == func_02042568) {
        return TRUE;
    }
    return FALSE;
}
