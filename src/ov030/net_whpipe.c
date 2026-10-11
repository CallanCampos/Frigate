#include "gfl/net_whpipe.h"
#include "types.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_devwireless.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/wih.h"
#include "nitro/os.h"
#include "nitro/wm.h"
#include "system/game_beacon.h"

// net_whpipe.c is named after its embedded string. It is the GFL net's wireless pipe over wih.c: it keeps the beacons
// that the scan found, sends the game's own, and runs the steps of starting as a parent or a child and of ending.
// The names are guesses from the code, except those of NitroSDK's WM and WH

// The most that the game's data in a beacon can be
#define _BEACON_USER_SIZE_MAX 0x68

// How many beacons the pipe keeps, and for how long (in frames) after the last time one was heard
#define BEACON_MAX 16
#define BEACON_LIFETIME 600

// What the game gives to tell the beacons of its game: this machine's game command base, the beacon's and the work
typedef BOOL (*NetWhpipeGameFilter)(u8 gameCommandBase, u8 beaconCommandBase, void *work);

// What the game puts in the user part of a beacon, after a CRC-16 of the rest
typedef struct {
    u16 crc;
    // The low 16 bits of the game's ID
    u16 gameId;
    u8 gameCommandBase;
    // Must be 0
    u8 unk5;
    // How many children are connected
    u8 childCount;
    // Whether the parent no longer takes children
    u8 closed;
    u8 data[_BEACON_USER_SIZE_MAX];
} NetWhpipeBeaconInfo;

// A beacon as the scan found it: NitroSDK's WMBssDesc, with its game info after
typedef struct {
    u8 unk00[4];
    u8 mac[6];
    u8 unk0A[0x2c];
    u16 channel;
    u8 unk38[0x18];
    NetWhpipeBeaconInfo info;
} NetWhpipeBeacon;

typedef struct {
    NetWhpipeState base;
    // What the game gets each machine's data with
    GFLNetRecvFunc recvCallback;
    NetWhpipeBeacon beacons[BEACON_MAX];
    u8 unkC08[0x30];
    void (*scanCallback)(void *arg, const void *beacon, u8 gameCommandBase, u8 childCount);
    void *scanArg;
    // How many frames are left of each beacon's life, 0 when its slot is free
    u16 beaconTimer[BEACON_MAX];
    void (*onConnect)(void);
    void *gameWork;
    u16 tgid;
    u16 startCount;
    u16 holdFrames;
    // The beacon that this machine sends as a parent
    NetWhpipeBeaconInfo info;
    u8 parentMac[6];
    u8 unkCE4;
    u8 unkCE5;
    // The channel to start the parent on, 0xff for the one that WH measures the quietest
    u8 channel;
    // How to end: 1 to finalize, 2 to end
    u8 endKind;
    u8 running;
    u8 unkCE9;
    // Whether the parent's beacon says that it no longer takes children
    u8 entryClosed;
    // Whether the beacon stopped
    u8 stopped;
    // Whether to stop the beacon when the children are gone
    u8 lockEnabled;
    u8 unkCED;
    u8 unkCEE;
    u8 beaconListChanged;
    u8 beaconCounter;
    u8 channelIndex;
    u8 waitTimer;
    u8 holding;
    u8 holdPause;
    u8 shutdownTimer;
    u8 pauseSearch;
    u8 unkCF7;
} NetWhpipeWork;

static NetWhpipeWork *sWork;

static void StateEnd(void *arg);
static void StateWaitEnd(void *arg);
static void SetBeaconInfo(NetWhpipeBeaconInfo *info, int size);
static int CountChildren(void);
static int GetChildrenBitmap(void);
static void StateSearch(void *arg);

void NetWhpipe_SetState(NetWhpipeState *work, NetWhpipeStateFunc state) {
    work->state = state;
}

BOOL func_ov030_02173340(HeapID heapId, WHCallback callback, void *gameWork, BOOL unused) {
    NetWhpipeWork *work;
    GFLNetInitData *init;

    func_02042e84();
    init = func_02042e84();
    if (sWork != NULL) {
        return FALSE;
    }
    work = GFL_HeapAllocate(heapId, sizeof(NetWhpipeWork), TRUE, "net_whpipe.c", 237);
    sWork = work;
    work->channel = 0xff;
    if (!WH_Initialize(heapId, callback, unused)) {
        return FALSE;
    }
    WH_SetGgid(init->unk4C.id.gameId);
    work->gameWork = gameWork;
    work->endKind = 0;
    work->unkCE4 = 0;
    work->tgid = func_020812b8();
    return TRUE;
}

void func_ov030_021733d0(GFLNetRecvFunc callback) {
    sWork->recvCallback = callback;
}

static BOOL CheckBeacon(const NetWhpipeBeacon *beacon) {
    NetWhpipeWork *work;
    GFLNetInitData *init;
    const NetWhpipeBeaconInfo *info;
    int gameCommandBase;
    u16 gameId;

    work = sWork;
    init = func_02042e84();
    gameCommandBase = init->gameCommandBase;
    gameId = init->unk4C.id.gameId;
    info = &beacon->info;

    if (info->closed != 0) {
        return FALSE;
    }
    if (info->unk5 != 0) {
        return FALSE;
    }
    if (info->crc != getCRC16(&info->gameId, sizeof(NetWhpipeBeaconInfo) - 2)) {
        return FALSE;
    }
    if (info->gameId != gameId) {
        return FALSE;
    }
    if (sWork->scanCallback != NULL) {
        sWork->scanCallback(sWork->scanArg, beacon, info->gameCommandBase, info->childCount);
    }
    if (init->unk20 != NULL) {
        if (!((NetWhpipeGameFilter)init->unk20)(gameCommandBase, info->gameCommandBase, work->gameWork)) {
            return FALSE;
        }
    } else if (gameCommandBase != info->gameCommandBase) {
        return FALSE;
    }
    if (gameCommandBase != 0x14 && info->childCount >= init->maxConnectNum && init->maxConnectNum > 1) {
        return FALSE;
    }
    return TRUE;
}

static BOOL OnBeaconFound(const void *found) {
    const NetWhpipeBeacon *beacon = found;
    int i;
    NetWhpipeWork *work = sWork;

    func_02042e84();
    if (!CheckBeacon(beacon)) {
        return FALSE;
    }
    for (i = 0; i < BEACON_MAX; i++) {
        if (work->beaconTimer[i] != 0 && GFL_STD_MemCmp(work->beacons[i].mac, beacon->mac, 6) == 0) {
            work->beaconTimer[i] = BEACON_LIFETIME;
            sys_memcpy(beacon, &work->beacons[i], sizeof(NetWhpipeBeacon));
            return TRUE;
        }
    }
    for (i = 0; i < BEACON_MAX; i++) {
        if (work->beaconTimer[i] == 0) {
            break;
        }
    }
    if (i >= BEACON_MAX) {
        return FALSE;
    }
    work->beaconTimer[i] = BEACON_LIFETIME;
    sys_memcpy(beacon, &work->beacons[i], sizeof(NetWhpipeBeacon));
    work->beaconListChanged = TRUE;
    return TRUE;
}

void func_ov030_02173554(void) {
    NetWhpipeWork *work = sWork;
    int i;

    for (i = 0; i < BEACON_MAX; i++) {
        work->beaconTimer[i] = 0;
    }
    sys_memset(work->beacons, 0, sizeof(work->beacons));
}

static void ResetFlags(NetWhpipeWork *work) {
    work->entryClosed = FALSE;
    work->beaconListChanged = FALSE;
    work->stopped = FALSE;
    work->lockEnabled = FALSE;
    work->unkCE9 = FALSE;
    work->running = FALSE;
}

static BOOL RecvCallback(u16 netId, u8 *data, u16 size) {
    return sWork->recvCallback(netId, data, size);
}

BOOL func_ov030_021735b0(BOOL clearBeacons) {
    NetWhpipeWork *work = sWork;

    if (work == NULL) {
        return FALSE;
    }
    ResetFlags(work);
    if (clearBeacons) {
        func_ov030_02173554();
    }
    WH_SetReceiver(RecvCallback);
    work->running = TRUE;
    if (WH_GetSystemState() == WH_SYSSTATE_IDLE && WH_StartScan(OnBeaconFound, 0, 0)) {
        return TRUE;
    }
    return FALSE;
}

static void StateEnd(void *arg) {
    NetWhpipeWork *work = arg;

    if (WH_GetSystemState() == WH_SYSSTATE_IDLE && WH_Finalize()) {
        work->shutdownTimer = 0xff;
        NetWhpipe_SetState(&sWork->base, StateWaitEnd);
    }
    if (WH_GetSystemState() == WH_SYSSTATE_ERROR) {
        work->shutdownTimer = 0xff;
        NetWhpipe_SetState(&sWork->base, StateWaitEnd);
    }
}

static void StateWaitEnd(void *arg) {
    NetWhpipeWork *work = arg;

    work->shutdownTimer--;
    if (WH_GetSystemState() == WH_SYSSTATE_STOP || WH_GetSystemState() == WH_SYSSTATE_IDLE) {
        NetWhpipe_SetState(&sWork->base, NULL);
        return;
    }
    if (WH_GetSystemState() == WH_SYSSTATE_ERROR) {
        WH_Reset();
        NetWhpipe_SetState(&sWork->base, StateEnd);
        return;
    }
    if (work->shutdownTimer == 0) {
        WH_Reset();
        NetWhpipe_SetState(&sWork->base, StateEnd);
    }
}

BOOL func_ov030_021736b8(void) {
    NetWhpipeWork *work = sWork;

    if (work == NULL) {
        return TRUE;
    }
    if (work->endKind == 0 && WH_Finalize()) {
        work->shutdownTimer = 0xff;
        NetWhpipe_SetState(&sWork->base, StateWaitEnd);
        return TRUE;
    }
    return FALSE;
}

static void FreeWork(void) {
    if (sWork != NULL) {
        GFL_HeapFree(sWork);
        sWork = NULL;
    }
}

static BOOL OnParentBeacon(const void *beacon) {
    NetWhpipeWork *work = sWork;

    if (!CheckBeacon(beacon)) {
        return FALSE;
    }
    if (work->onConnect != NULL) {
        work->onConnect();
    }
    return TRUE;
}

static void TickBeaconTimers(void) {
    NetWhpipeWork *work = sWork;
    int i;

    for (i = 0; i < BEACON_MAX; i++) {
        if (work->beaconTimer[i] != 0) {
            work->beaconTimer[i]--;
            if (work->beaconTimer[i] == 0) {
                work->beaconListChanged = TRUE;
            }
        }
    }
}

void func_ov030_02173780(void) {
    if (func_02042788()) {
        NetWhpipeBeaconInfo *info;
        NetWhpipeWork *work = sWork;
        GFLNetInitData *init = func_02042e84();
        int size;

        if (init->unk1C == NULL) {
            return;
        }
        size = init->unk1C(work->gameWork);
        if (size > _BEACON_USER_SIZE_MAX) {
            sys_exit();
            return;
        }
        info = &work->info;
        info->gameCommandBase = init->gameCommandBase;
        info->gameId = init->unk4C.id.gameId;
        info->unk5 = 0;
        info->closed = work->entryClosed;
        sys_memcpy(init->unk18(work->gameWork), info->data, size);
        SetBeaconInfo(&work->info, sizeof(NetWhpipeBeaconInfo));
    }
}

static void UpdateBeacon(void) {
    NetWhpipeWork *work = sWork;
    NetWhpipeBeaconInfo *info = &work->info;
    GFLNetInitData *init = func_02042e84();

    if (init->unk1C != NULL) {
        int size = init->unk1C(work->gameWork);

        GFL_ASSERT(size <= _BEACON_USER_SIZE_MAX);
        sys_memcpy(init->unk18(work->gameWork), info->data, size);
    }
    if (info->childCount != CountChildren()) {
        work->beaconCounter = 0;
        info->childCount = CountChildren();
    }
    if (work->beaconCounter == 0) {
        SetBeaconInfo(&work->info, sizeof(NetWhpipeBeaconInfo));
        work->beaconCounter = 2;
    }
    work->beaconCounter--;
}

static void Update(u16 count) {
    int state = WH_GetSystemState();
    NetWhpipeWork *work = sWork;

    if (!func_ov030_02173ae4()) {
        UpdateBeacon();
    }
    if (work->startCount == 0xffff) {
        work->startCount = count;
    }
    if (work->lockEnabled) {
        if (!WH_GetAid() && !GetChildrenBitmap()) {
            work->stopped = TRUE;
        }
        if (work->startCount > count) {
            work->stopped = TRUE;
        }
    }
    if (WH_GetLastError() == 0x19) {
        func_02042e30(0);
    }
    switch (state) {
    case WH_SYSSTATE_STOP:
        if (work->endKind == 1) {
            FreeWork();
        }
        break;
    case WH_SYSSTATE_IDLE:
        if (work->endKind == 1) {
            if (WH_End(NULL)) {
                break;
            }
        }
        if (work->endKind == 2) {
            WH_End(NULL);
        }
        break;
    case WH_SYSSTATE_CONNECT_FAIL:
    case WH_SYSSTATE_ERROR:
        if (work != NULL) {
            if (!work->stopped) {
                func_02042e54(WH_GetLastError());
            }
            work->stopped = TRUE;
        }
        break;
    }
}

void func_ov030_02173964(u16 count) {
    if (sWork != NULL && sWork->base.state != NULL) {
        sWork->base.state(sWork);
    }
    if (sWork != NULL) {
        Update(count);
    }
    if (sWork != NULL && func_ov030_02173c08()) {
        TickBeaconTimers();
    }
    if (sWork != NULL) {
        if (sWork->holdFrames != 0) {
            WH_SetScanPaused(TRUE);
            sWork->holding = TRUE;
            if (sWork->holdPause == 0) {
                sWork->holdFrames--;
            }
        } else {
            WH_SetScanPaused(FALSE);
            sWork->holding = FALSE;
        }
    }
    WH_UpdateScan();
}

BOOL func_ov030_021739ec(void) {
    if (sWork == NULL) {
        return FALSE;
    }
    if (!WH_GetAid() && !GetChildrenBitmap()) {
        return FALSE;
    }
    if (WH_GetSystemState() == WH_SYSSTATE_CONNECTED || WH_GetSystemState() == WH_SYSSTATE_DATASHARING) {
        return TRUE;
    }
    return FALSE;
}

static BOOL IsConnected(u16 netId) {
    if (sWork == NULL) {
        return FALSE;
    }
    if (WH_GetSystemState() != WH_SYSSTATE_CONNECTED && WH_GetSystemState() != WH_SYSSTATE_DATASHARING) {
        return FALSE;
    }
    if (WH_GetBitmap() & (1 << netId)) {
        return TRUE;
    }
    return FALSE;
}

static int CountChildren(void) {
    int count = 0;
    int i;

    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        if (IsConnected(i)) {
            count++;
        }
    }
    return count;
}

BOOL func_ov030_02173a8c(void) {
    if (sWork != NULL) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov030_02173aa0(void) {
    if (sWork != NULL) {
        if (WH_GetSystemState() == WH_SYSSTATE_IDLE) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

static int GetChildrenBitmap(void) {
    if (sWork != NULL) {
        return WH_GetBitmap() & 0xfffe;
    }
    return 0;
}

BOOL func_ov030_02173ae4(void) {
    if (sWork != NULL && sWork->stopped) {
        return TRUE;
    }
    return FALSE;
}

void func_ov030_02173b04(int lock) {
    NetWhpipeWork *work = sWork;

    if (work != NULL) {
        work->lockEnabled = lock;
        work->startCount = 0xffff;
    }
}

void *func_ov030_02173b24(int index) {
    NetWhpipeWork *work = sWork;

    if (work != NULL && work->beaconTimer[index] != 0) {
        NetWhpipeBeaconInfo *info = &work->beacons[index].info;

        return info->data;
    }
    return NULL;
}

u8 *func_ov030_02173b50(int index) {
    NetWhpipeWork *work = sWork;

    if (work != NULL && work->beaconTimer[index] != 0) {
        return work->beacons[index].mac;
    }
    return NULL;
}

u8 func_ov030_02173b78(int index) {
    NetWhpipeWork *work = sWork;

    if (work != NULL && work->beaconTimer[index] != 0) {
        return work->beacons[index].info.gameCommandBase;
    }
    return 0;
}

void func_ov030_02173ba4(int index) {
    NetWhpipeWork *work = sWork;

    if (work != NULL && work->beaconTimer[index] != 0) {
        work->beaconTimer[index] = 0;
    }
}

void func_ov030_02173bc4(void) {
    NetWhpipeWork *work = sWork;
    int i;

    if (work != NULL) {
        for (i = 0; i < BEACON_MAX; i++) {
            work->beaconTimer[i] = 0;
        }
    }
}

void func_ov030_02173bec(int index) {
    func_ov030_02173ba4(index);
}

BOOL func_ov030_02173bf4(void) {
    if (WH_GetBeaconCount()) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov030_02173c08(void) {
    if (WH_GetSystemState() == WH_SYSSTATE_SCANNING) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_ov030_02173c1c(u8 *data, int size, BOOL (*callback)(BOOL ok)) {
    return WH_SendData(data, size, callback);
}

int func_ov030_02173c28(void) {
    return WH_GetBitmap();
}

int func_ov030_02173c30(void) {
    return WH_GetAid();
}

static void StateStartParent(void *arg) {
    NetWhpipeWork *work = arg;
    int state = WH_GetSystemState();
    u16 modes[2] = { 4, 0 };

    if (state == WH_SYSSTATE_MEASURECHANNEL) {
        GFLNetInitData *init = func_02042e84();
        u16 channel;

        work->entryClosed = FALSE;
        work->beaconListChanged = FALSE;
        work->unkCE9 = FALSE;
        work->running = FALSE;
        channel = WH_GetMeasureChannel();
        if (work->channel == 0xff) {
            work->channel = channel;
        } else {
            channel = work->channel;
        }
        if (init->unk66 != 0) {
            work->tgid = func_020812b8();
        }
        func_ov030_02173780();
        WH_ParentConnect(modes[init->bMPMode], work->tgid, channel, init->maxConnectNum - 1);
        NetWhpipe_SetState(&sWork->base, NULL);
    }
}

static void StateMeasureChannel(void *arg) {
    if (WH_GetSystemState() == WH_SYSSTATE_IDLE && WH_StartMeasureChannel()) {
        NetWhpipe_SetState(&sWork->base, StateStartParent);
    }
}

BOOL func_ov030_02173cf8(BOOL anyChannel) {
    NetWhpipeWork *work = sWork;

    if (work->base.state != NULL) {
        return FALSE;
    }
    if (anyChannel) {
        work->channel = 0xff;
    }
    WH_SetReceiver(RecvCallback);
    sWork->running = TRUE;
    NetWhpipe_SetState(&sWork->base, StateMeasureChannel);
    return TRUE;
}

static void SetBeaconInfo(NetWhpipeBeaconInfo *info, int size) {
    NetWhpipeWork *work = sWork;
    GFLNetInitData *init = func_02042e84();

    if (work != NULL && init != NULL) {
        info->crc = getCRC16(&info->gameId, size - 2);
        cp15_flushDC(info, size);
        if (WH_GetSystemState() == WH_SYSSTATE_IDLE) {
            WH_SetUserGameInfo(info, size);
        } else {
            WH_SetGameInfo(info, size, init->unk4C.id.gameId, work->tgid);
        }
    }
}

static void StateStartChild(void *arg) {
    GFLNetInitData *init = func_02042e84();
    u16 modes[2] = { 5, 1 };

    if (WH_GetSystemState() == WH_SYSSTATE_IDLE || WH_GetSystemState() == WH_SYSSTATE_SCANNING) {
        WH_ChildConnectAuto(modes[init->bMPMode], sWork->parentMac, 0);
        WH_SetScanCallback(OnParentBeacon);
        WH_SetReceiver(RecvCallback);
        sWork->running = TRUE;
        NetWhpipe_SetState(&sWork->base, NULL);
    }
}

BOOL func_ov030_02173e0c(BOOL unused, const u8 *mac, int index, void (*callback)(void)) {
    func_02042e84();
    if (sWork->base.state != NULL) {
        return FALSE;
    }
    if (WH_GetSystemState() == WH_SYSSTATE_SCANNING) {
        sWork->unkCE9 = FALSE;
        WH_EndScan();
    }
    NetWhpipe_SetState(&sWork->base, StateStartChild);
    if (mac != NULL) {
        sys_memcpy(mac, sWork->parentMac, 6);
    } else {
        sys_memcpy(sWork->beacons[index].mac, sWork->parentMac, 6);
    }
    sWork->onConnect = callback;
    return TRUE;
}

static void StateFinish(void *arg) {
    if (WH_GetSystemState() == WH_SYSSTATE_STOP) {
        WH_Release();
        FreeWork();
    }
}

BOOL func_ov030_02173e9c(WHCallback callback) {
    if (WH_End(callback)) {
        NetWhpipe_SetState(&sWork->base, StateFinish);
        return TRUE;
    }
    return FALSE;
}

static BOOL AcceptAll(const void *info, void *work) {
    return TRUE;
}

static BOOL AcceptNone(const void *info, void *work) {
    return FALSE;
}

void func_ov030_02173ec8(BOOL acceptAll) {
    if (acceptAll) {
        WH_SetScanFilter(AcceptAll);
    } else {
        WH_SetScanFilter(AcceptNone);
    }
}

static void StateParentWait(void *arg) {
    NetWhpipeWork *work = arg;

    if (!sWork->holding) {
        if (work->waitTimer != 0) {
            work->waitTimer--;
        }
        if (work->waitTimer == 0 && WH_Finalize()) {
            NetWhpipe_SetState(&sWork->base, StateSearch);
        }
    }
}

static void StateParentRetry(void *arg) {
    NetWhpipeWork *work = arg;

    if (func_ov030_02173aa0()) {
        u8 channels[3] = { 1, 7, 13 };

        work->channelIndex = GFL_RandomLC(3);
        if (work->channelIndex >= 3) {
            work->channelIndex = 0;
        }
        func_ov030_02173780();
        GameBeaconSys_ToggleB2W2Only();
        if (WH_ParentConnect(0, work->tgid, channels[work->channelIndex], 1)) {
            work->waitTimer = GFL_RandomLC(0x98) + 0x3c;
            NetWhpipe_SetState(&sWork->base, StateParentWait);
        }
    }
}

static void StateSearchWait(void *arg) {
    NetWhpipeWork *work = arg;

    if (!sWork->pauseSearch) {
        if (work->waitTimer != 0) {
            work->waitTimer--;
        }
        if (work->waitTimer == 0) {
            WH_EndScan();
            NetWhpipe_SetState(&sWork->base, StateParentRetry);
        }
    }
}

static void StateSearch(void *arg) {
    NetWhpipeWork *work = arg;

    if (WH_GetSystemState() == WH_SYSSTATE_IDLE && WH_StartScan(OnBeaconFound, 0, 0)) {
        work->waitTimer = GFL_RandomLC(0x28) + 0x3c;
        NetWhpipe_SetState(&sWork->base, StateSearchWait);
    }
}

BOOL func_ov030_02174040(void) {
    NetWhpipeWork *work = sWork;

    if (work == NULL) {
        return FALSE;
    }
    func_ov030_021740f0(0);
    ResetFlags(work);
    func_ov030_02173554();
    WH_SetReceiver(RecvCallback);
    work->running = TRUE;
    NetWhpipe_SetState(&sWork->base, StateSearch);
    return TRUE;
}

void func_ov030_02174088(void) {
    if (sWork->lockEnabled) {
        sWork->stopped = TRUE;
    }
}

u8 func_ov030_021740a4(u8 index) {
    NetWhpipeWork *work = sWork;

    if (work != NULL && work->beaconTimer[index] != 0) {
        return work->beacons[index].info.gameCommandBase;
    }
    return 0;
}

void func_ov030_021740d0(int index) {
    WH_SetScanTarget(sWork->beacons[index].channel, sWork->beacons[index].mac);
}

void func_ov030_021740f0(int unused) {
    if (sWork != NULL) {
        sWork->holdFrames = 20;
    }
}

void func_ov030_02174108(u32 enabled) {
    if (sWork != NULL) {
        sWork->pauseSearch = enabled;
    }
}

void func_ov030_02174120(void (*callback)(void *arg, const void *beacon, u8 gameCommandBase, u8 childCount),
                         void *arg) {
    if (sWork != NULL) {
        sWork->scanCallback = callback;
        sWork->scanArg = arg;
    }
}
