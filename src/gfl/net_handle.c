#include "types.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_handle.h"
#include "gfl/net_irc_wireless.h"
#include "gfl/net_system.h"
#include "gfl/std.h"
#include "nitro/os.h"

// The timing numbers the server relays
typedef struct {
    u16 timings[GFL_NET_MACHINE_MAX];
    // Who each timing number is for
    u8 timingSend[GFL_NET_MACHINE_MAX];
    // Whether the server has each timing number to relay
    u8 relayPending[GFL_NET_MACHINE_MAX];
} NetHandleBaseWork;

typedef struct {
    u16 timing;
    u8 netIds;
} NetTimingData;

static void func_02040224(NetHandle *handle, int netId);
static void func_02040298(NetHandle *handle, int netId);
static NetHandle *func_020402c8(int netId);
static void func_020404c0(void);
static BOOL func_02040680(NetHandle *handle);
static void func_0204092c(NetHandle *handle);
static u16 func_02040a2c(u16 bits, int netID, BOOL set);
static BOOL func_02040a6c(u16 bits, int netID);

static NetHandleBaseWork *_pBaseWork;

int func_020401dc(NetHandle *handle) {
    int i;

    func_02042e78();
    if (func_02040414(GFL_NET_NETID_SERVER) == handle) {
        return GFL_NET_NETID_SERVER;
    }
    for (i = 0; i < GFL_NET_HANDLE_MAX; i++) {
        if (func_02040414(i) == handle) {
            return i;
        }
    }
    GFL_ASSERT(0);
    return -1;
}

static void func_02040224(NetHandle *handle, int netId) {
    NetNegotiation *negotiations[GFL_NET_HANDLE_MAX];
    int i;

    if (netId != GFL_NET_NETID_SERVER) {
        handle->negotiationsSent = 0;
        handle->serverNegotiationsSent = func_02040a2c(handle->serverNegotiationsSent, netId, FALSE);
    }
    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        if (handle->negotiations[i] != NULL) {
            MI_CpuFill8(handle->negotiations[i], 0, sizeof(NetNegotiation));
        }
        negotiations[i] = handle->negotiations[i];
    }
    if (handle->data != NULL) {
        GFL_HeapFree(handle->data);
        handle->data = NULL;
    }
    MI_CpuFill8(handle, 0, sizeof(NetHandle));
    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        handle->negotiations[i] = negotiations[i];
    }
    func_0203efd0(netId);
}

static void func_02040298(NetHandle *handle, int netId) {
    if (netId != GFL_NET_NETID_SERVER) {
        handle->negotiationsSent = 0;
        handle->serverNegotiationsSent = func_02040a2c(handle->serverNegotiationsSent, netId, FALSE);
    }
    if (handle->negotiations[netId] != NULL) {
        MI_CpuFill8(handle->negotiations[netId], 0, sizeof(NetNegotiation));
    }
}

static NetHandle *func_020402c8(int netId) {
    GFLNetInitData *ini = func_02042e84();
    NetHandle *handle;
    int i;

    func_02042e78();
    handle = func_02040414(netId);
    if (netId == GFL_NET_NETID_SERVER) {
        for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
            if (handle->negotiations[i] != NULL) {
                GFL_HeapFree(handle->negotiations[i]);
            }
        }
    }
    MI_CpuFill8(handle, 0, sizeof(NetHandle));
    if (netId == GFL_NET_NETID_SERVER) {
        for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
            handle->negotiations[i] = GFL_HeapAllocate(ini->heapId, sizeof(NetNegotiation), TRUE, "net_handle.c", 199);
        }
    }
    handle->state = NET_HANDLE_STATE_NONE;
    handle->timingRecv = 0xff;
    handle->timingSend = 0xff;
    handle->timingSendPending = FALSE;
    return handle;
}

void func_0204034c(void *sys) {
    GFLNetInitData *ini = func_02042e84();
    int i;

    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        func_020402c8(i);
    }
    func_020402c8(GFL_NET_NETID_SERVER);
    GFL_ASSERT(!_pBaseWork);
    _pBaseWork = GFL_HeapAllocate(ini->heapId, sizeof(NetHandleBaseWork), TRUE, "net_handle.c", 229);
}

void func_020403a4(void *sys) {
    NetHandle *handle;
    int i;

    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        func_02040224(func_02040414(i), i);
    }
    handle = func_02040414(GFL_NET_NETID_SERVER);
    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        if (handle->negotiations[i] != NULL) {
            GFL_HeapFree(handle->negotiations[i]);
            handle->negotiations[i] = NULL;
        }
    }
    func_02040224(func_02040414(GFL_NET_NETID_SERVER), GFL_NET_NETID_SERVER);
    GFL_ASSERT(_pBaseWork);
    GFL_HeapFree(_pBaseWork);
    _pBaseWork = NULL;
}

NetHandle *func_02040414(int netId) {
    GFLNetSys *sys = func_02042e78();

    if (sys != NULL) {
        if (netId == GFL_NET_NETID_SERVER) {
            return &sys->handles[GFL_NET_HANDLE_MAX - 1];
        }
        if (netId < GFL_NET_HANDLE_MAX) {
            return &sys->handles[netId];
        }
    }
    return NULL;
}

NetHandle *func_02040440(void) {
    return func_02040414(func_0203ffc4());
}

BOOL func_0204044c(NetHandle *handle) {
    if (handle != NULL) {
        if (handle->state == NET_HANDLE_STATE_NEGOTIATED) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

u8 func_02040464(NetHandle *handle) {
    if (handle != NULL) {
        return handle->state;
    }
    return 8;
}

int func_02040474(void) {
    int i;
    GFLNetSys *sys = func_02042e78();
    int count = 0;

    if (sys != NULL) {
        for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
            if (func_0204044c(func_02040414(i))) {
                count++;
            }
        }
    }
    return count;
}

void func_0204049c(int netId) {
    if (func_0203ffc4() == 0) {
        NetHandle *handle = func_02040414(GFL_NET_NETID_SERVER);

        handle->replies[netId].netId = netId;
        handle->replies[netId].pending = TRUE;
    }
}

static void func_020404c0(void) {
    if (func_0203ffc4() == 0) {
        NetHandle *handle = func_02040414(GFL_NET_NETID_SERVER);
        int i;

        for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
            if (handle->replies[i].pending && func_02042be8(handle, 5, 2, &handle->replies[i])) {
                handle->replies[i].pending = FALSE;
            }
        }
    }
}

BOOL func_02040504(void) {
    NetHandle *handle = func_02040440();
    u8 netId = func_020401dc(handle);
    GFLNetInitData *ini;

    func_02042e78();
    ini = func_02042e84();
    if (func_0203fe74(func_0203ffc4())) {
        if (handle->state == NET_HANDLE_STATE_SEND_NEGOTIATION) {
            u8 data[9];

            OS_GetMacAddress(data);
            data[6] = netId;
            data[7] = ini->gameCommandBase;
            data[8] = ini->unk6E;
            return func_02042be8(handle, 3, sizeof(data), data);
        } else if (handle->state < NET_HANDLE_STATE_SEND_NEGOTIATION) {
            handle->state++;
        }
    }
    return FALSE;
}

void func_02040574(int netId, int size, const void *data, void *work, NetHandle *handle) {
    u16 timing = ((NetTimingData *)data)->timing;
    u8 netIds = ((NetTimingData *)data)->netIds;
    int i;

    if (func_020401dc(handle) == GFL_NET_NETID_SERVER && netId != GFL_NET_NETID_SERVER) {
        _pBaseWork->timings[netId] = timing;
        _pBaseWork->timingSend[netId] = netIds;
        for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
            if (func_0204044c(func_02040414(i))) {
                if (netIds == _pBaseWork->timingSend[i]) {
                    if (timing != _pBaseWork->timings[i]) {
                        return;
                    }
                } else if ((1 << i) & netIds) {
                    return;
                }
            } else if (func_0203fe74(i) == TRUE) {
                return;
            }
        }
        _pBaseWork->relayPending[netId] = TRUE;
    }
}

void func_020405ec(int netId, int size, const void *data, void *work, NetHandle *handle) {
    handle->timingRecv = *(u16 *)data;
}

void func_020405f8(NetHandle *handle, u16 timing) {
    handle->timingSend = timing;
    handle->unk48 = 0xff;
    if (func_020401dc(handle) == GFL_NET_NETID_SERVER) {
        handle->timingSendPending = FALSE;
    } else {
        handle->timingSendPending = TRUE;
    }
}

void func_02040624(NetHandle *handle, u32 a1, u32 a2) {
    if (handle != NULL) {
        handle->timingSend = a1 + (a2 << 8);
        handle->unk48 = 0xff;
        if (func_020401dc(handle) == GFL_NET_NETID_SERVER) {
            handle->timingSendPending = FALSE;
        } else {
            handle->timingSendPending = TRUE;
        }
    }
}

BOOL func_02040654(NetHandle *handle, u16 timing) {
    if (handle->timingRecv == timing) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_02040664(NetHandle *handle, u32 a1, u32 a2) {
    if (handle == NULL) {
        return TRUE;
    }
    if (handle->timingRecv == a1 + (a2 << 8)) {
        return TRUE;
    }
    return FALSE;
}

// Sends the server's negotiations that have not been sent
static BOOL func_02040680(NetHandle *handle) {
    int i;

    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        if (handle->negotiations[i]->valid && !func_02040a6c(handle->negotiationsSent, i)) {
            if (func_02042c18(handle, GFL_NET_NETID_SERVER, 4, sizeof(NetNegotiation), handle->negotiations[i], 1, 0,
                              1)) {
                func_02040a2c(handle->negotiationsSent, i, TRUE);
            } else {
                return FALSE;
            }
        }
    }
    handle->negotiationsSent = 0;
    return TRUE;
}

void func_020406e0(void) {
    NetHandle *handle;
    int i;

    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        int netId = func_020401d4(i);

        handle = func_02040414(netId);
        if (!func_0203fe74(netId)) {
            func_0204044c(handle);
        }
    }
    func_020404c0();
    if (func_02042bc4()) {
        handle = func_02040414(GFL_NET_NETID_SERVER);
        for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
            if (_pBaseWork->relayPending[i] == TRUE &&
                func_02042c9c(handle, _pBaseWork->timingSend[i], 8, 2, &_pBaseWork->timings[i], 0, 0, 0)) {
                GFL_ASSERT(_pBaseWork->timingSend[i]);
                _pBaseWork->relayPending[i] = FALSE;
            }
        }
    }
    for (i = 0; i < GFL_NET_HANDLE_MAX; i++) {
        handle = func_02040414(func_020401d4(i));
        if (func_02042bd8(handle) && handle->timingSendPending && func_020401dc(handle) != GFL_NET_NETID_SERVER) {
            if (func_02042be8(handle, 7, 4, &handle->timingSend)) {
                handle->timingSendPending = FALSE;
            }
        }
    }
    if (func_0203ffc4() == 0) {
        handle = func_02040414(GFL_NET_NETID_SERVER);
        if (handle != NULL) {
            for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
                if (handle->negotiations[i] != NULL && handle->negotiations[i]->valid &&
                    !func_02040a6c(handle->serverNegotiationsSent, i) && func_02040680(handle)) {
                    handle->serverNegotiationsSent = func_02040a2c(handle->serverNegotiationsSent, i, TRUE);
                }
            }
        }
    }
    {
        GFLNetInitData *ini = func_02042e84();

        if (ini->bNetType != 4) {
            handle = func_02040440();
            if (handle->infoPending == TRUE) {
                if (ini->getInfo != NULL && ini->getInfoSize != NULL) {
                    int size = ini->getInfoSize(func_02042d94());

                    if (func_02042be8(handle, 10, size, ini->getInfo(func_02042d94()))) {
                        handle->infoPending = FALSE;
                    }
                } else {
                    handle->infoPending = FALSE;
                }
            }
        }
    }
}

// The server receives a machine's negotiation
void func_02040880(int netId, int size, u8 *data, void *work, NetHandle *handle) {
    GFLNetInitData *ini = func_02042e84();

    if (func_02040414(GFL_NET_NETID_SERVER) == handle) {
        NetNegotiation *negotiation = handle->negotiations[netId];

        if (!negotiation->valid) {
            negotiation->valid = TRUE;
            MI_CpuCopy8(data, negotiation->mac, sizeof(negotiation->mac));
            negotiation->netId = data[6];
            OS_GetMacAddress(negotiation->serverMac);
            if (data[8] != 0) {
                if (data[6] != 0 && data[7] != ini->gameCommandBase && ini->unk6E == data[8]) {
                    negotiation->unkF = ini->unk6E;
                    negotiation->unkE = ini->unk6E;
                    return;
                }
                if (data[6] == 0 && data[7] == ini->gameCommandBase && ini->unk6E == data[8]) {
                    negotiation->unkF = ini->unk6E;
                    negotiation->unkE = ini->unk6E;
                    return;
                }
                negotiation->unkF = 0xff;
                negotiation->unkE = 0;
            } else {
                negotiation->unkF = ini->gameCommandBase;
                negotiation->unkE = data[7];
            }
        }
    } else {
        handle->infoPending = TRUE;
    }
}

static void func_0204092c(NetHandle *handle) {
    GFLNetSys *sys = func_02042e78();

    if (sys->unk352 == 0) {
        if (handle->state != NET_HANDLE_STATE_NEGOTIATED) {
            handle->state = NET_HANDLE_STATE_REJECTED;
        }
    } else {
        handle->state = NET_HANDLE_STATE_REJECTED;
    }
}

// A machine receives the server's reply to its negotiation
void func_0204095c(int netId, int size, NetNegotiation *negotiation, void *work, NetHandle *handle) {
    GFLNetInitData *ini = func_02042e84();
    u8 mac[6];

    if (func_020401dc(handle) == GFL_NET_NETID_SERVER) {
        handle->state = NET_HANDLE_STATE_NEGOTIATED;
        return;
    }
    if (ini->unk6E != 0) {
        if (ini->unk6E != negotiation->unkF) {
            func_0204092c(handle);
        }
        if (ini->unk6E != negotiation->unkE) {
            func_0204092c(handle);
        }
    } else {
        if (ini->gameCommandBase != negotiation->unkF) {
            func_0204092c(handle);
        }
        if (ini->gameCommandBase != negotiation->unkE) {
            func_0204092c(handle);
        }
    }
    OS_GetMacAddress(mac);
    if (GFL_STD_MemCmp(negotiation->mac, mac, sizeof(mac)) == 0 && negotiation->netId != func_020401dc(handle)) {
        func_0204092c(handle);
    }
    if (handle->state != NET_HANDLE_STATE_REJECTED) {
        func_02040414(negotiation->netId)->state = NET_HANDLE_STATE_NEGOTIATED;
        if (ini->negotiationCallback != NULL) {
            ini->negotiationCallback(work, negotiation->netId);
        }
        func_020430bc(negotiation->mac);
        func_020430bc(negotiation->serverMac);
    }
}

void func_02040a20(int netId, int size, u8 *data) {
    func_02040a9c(*data);
}

static u16 func_02040a2c(u16 bits, int netID, BOOL set) {
    u16 bit;

    // clang-format off
    GFL_ASSERT(netID!=GFL_NET_NETID_SERVER);
    // clang-format on
    bit = 1 << netID;
    if (set) {
        bits |= bit;
    } else {
        bits = (u16)~bit & bits;
    }
    return bits;
}

static BOOL func_02040a6c(u16 bits, int netID) {
    u8 bit;

    // clang-format off
    GFL_ASSERT(netID!=GFL_NET_NETID_SERVER);
    // clang-format on
    bit = 1 << netID;
    if (bit & bits) {
        return TRUE;
    }
    return FALSE;
}

void func_02040a9c(int netID) {
    // clang-format off
    GFL_ASSERT(netID!=GFL_NET_NETID_SERVER);
    // clang-format on
    func_02040224(func_02040414(netID), netID);
    func_02040298(func_02040414(GFL_NET_NETID_SERVER), netID);
}

// Keeps data from a machine
void func_02040ad0(int netID, int size, const void *data, void *work, NetHandle *handle) {
    GFLNetInitData *ini = func_02042e84();

    if (func_020401dc(handle) != GFL_NET_NETID_SERVER) {
        NetHandle *target;

        GFL_ASSERT(netID < GFL_NET_HANDLE_MAX);
        target = func_02040414(netID);
        if (target->data != NULL) {
            GFL_HeapFree(target->data);
        }
        target->data = GFL_HeapAllocate(ini->heapId, size, TRUE, "net_handle.c", 984);
        target->dataSize = size;
        MI_CpuCopy8(data, target->data, size);
    }
}

void func_02040b38(NetHandle *handle, int a1, int a2, int a3) {
    if (handle != NULL) {
        handle->timingSend = a1 + (a2 << 8);
        handle->unk48 = a3;
        if (func_020401dc(handle) == GFL_NET_NETID_SERVER) {
            handle->timingSendPending = FALSE;
        } else {
            handle->timingSendPending = TRUE;
        }
    }
}

void func_02040b68(NetHandle *handle, int value) {
    if (handle != NULL) {
        int i;

        for (i = 0; i < GFL_NET_HANDLE_MAX; i++) {
            NetNegotiation *negotiation = handle->negotiations[i];

            if (negotiation != NULL && negotiation->valid) {
                negotiation->unkE = value;
                negotiation->unkF = value;
            }
        }
    }
}
