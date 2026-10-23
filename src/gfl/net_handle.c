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

static void NetHandle_Reset(NetHandle *handle, int netId);
static void NetHandle_ClearNegotiation(NetHandle *handle, int netId);
static NetHandle *NetHandle_Init(int netId);
static void NetHandle_SendReplies(void);
static BOOL NetHandle_SendServerNegotiations(NetHandle *handle);
static void NetHandle_Reject(NetHandle *handle);
static u16 NetHandle_SetBit(u16 bits, int netID, BOOL set);
static BOOL NetHandle_TestBit(u16 bits, int netID);

static NetHandleBaseWork *_pBaseWork;

int NetHandle_GetNetId(NetHandle *handle) {
    int i;

    GFL_NetGetSys();
    if (NetHandle_Get(GFL_NET_NETID_SERVER) == handle) {
        return GFL_NET_NETID_SERVER;
    }
    for (i = 0; i < GFL_NET_HANDLE_MAX; i++) {
        if (NetHandle_Get(i) == handle) {
            return i;
        }
    }
    GFL_ASSERT(0);
    return -1;
}

static void NetHandle_Reset(NetHandle *handle, int netId) {
    NetNegotiation *negotiations[GFL_NET_HANDLE_MAX];
    int i;

    if (netId != GFL_NET_NETID_SERVER) {
        handle->negotiationsSent = 0;
        handle->serverNegotiationsSent = NetHandle_SetBit(handle->serverNegotiationsSent, netId, FALSE);
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
    NetSystem_ForgetMachine(netId);
}

static void NetHandle_ClearNegotiation(NetHandle *handle, int netId) {
    if (netId != GFL_NET_NETID_SERVER) {
        handle->negotiationsSent = 0;
        handle->serverNegotiationsSent = NetHandle_SetBit(handle->serverNegotiationsSent, netId, FALSE);
    }
    if (handle->negotiations[netId] != NULL) {
        MI_CpuFill8(handle->negotiations[netId], 0, sizeof(NetNegotiation));
    }
}

static NetHandle *NetHandle_Init(int netId) {
    GFLNetInitData *ini = GFL_NetGetInitData();
    NetHandle *handle;
    int i;

    GFL_NetGetSys();
    handle = NetHandle_Get(netId);
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

void NetHandle_InitAll(GFLNetSys *sys) {
    GFLNetInitData *ini = GFL_NetGetInitData();
    int i;

    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        NetHandle_Init(i);
    }
    NetHandle_Init(GFL_NET_NETID_SERVER);
    GFL_ASSERT(!_pBaseWork);
    _pBaseWork = GFL_HeapAllocate(ini->heapId, sizeof(NetHandleBaseWork), TRUE, "net_handle.c", 229);
}

void NetHandle_FreeAll(GFLNetSys *sys) {
    NetHandle *handle;
    int i;

    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        NetHandle_Reset(NetHandle_Get(i), i);
    }
    handle = NetHandle_Get(GFL_NET_NETID_SERVER);
    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        if (handle->negotiations[i] != NULL) {
            GFL_HeapFree(handle->negotiations[i]);
            handle->negotiations[i] = NULL;
        }
    }
    NetHandle_Reset(NetHandle_Get(GFL_NET_NETID_SERVER), GFL_NET_NETID_SERVER);
    GFL_ASSERT(_pBaseWork);
    GFL_HeapFree(_pBaseWork);
    _pBaseWork = NULL;
}

NetHandle *NetHandle_Get(int netId) {
    GFLNetSys *sys = GFL_NetGetSys();

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

NetHandle *NetHandle_GetOwn(void) {
    return NetHandle_Get(NetSystem_GetNetId());
}

BOOL NetHandle_IsNegotiated(NetHandle *handle) {
    if (handle != NULL) {
        if (handle->state == NET_HANDLE_STATE_NEGOTIATED) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

u8 NetHandle_GetState(NetHandle *handle) {
    if (handle != NULL) {
        return handle->state;
    }
    return 8;
}

int NetHandle_CountNegotiated(void) {
    int i;
    GFLNetSys *sys = GFL_NetGetSys();
    int count = 0;

    if (sys != NULL) {
        for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
            if (NetHandle_IsNegotiated(NetHandle_Get(i))) {
                count++;
            }
        }
    }
    return count;
}

void NetHandle_RequestReply(int netId) {
    if (NetSystem_GetNetId() == 0) {
        NetHandle *handle = NetHandle_Get(GFL_NET_NETID_SERVER);

        handle->replies[netId].netId = netId;
        handle->replies[netId].pending = TRUE;
    }
}

static void NetHandle_SendReplies(void) {
    if (NetSystem_GetNetId() == 0) {
        NetHandle *handle = NetHandle_Get(GFL_NET_NETID_SERVER);
        int i;

        for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
            if (handle->replies[i].pending && GFL_NetSendCommand(handle, 5, 2, &handle->replies[i])) {
                handle->replies[i].pending = FALSE;
            }
        }
    }
}

BOOL NetHandle_SendNegotiation(void) {
    NetHandle *handle = NetHandle_GetOwn();
    u8 netId = NetHandle_GetNetId(handle);
    GFLNetInitData *ini;

    GFL_NetGetSys();
    ini = GFL_NetGetInitData();
    if (NetSystem_IsMachineConnected(NetSystem_GetNetId())) {
        if (handle->state == NET_HANDLE_STATE_SEND_NEGOTIATION) {
            u8 data[9];

            OS_GetMacAddress(data);
            data[6] = netId;
            data[7] = ini->gameCommandBase;
            data[8] = ini->negotiationCommandBase;
            return GFL_NetSendCommand(handle, 3, sizeof(data), data);
        } else if (handle->state < NET_HANDLE_STATE_SEND_NEGOTIATION) {
            handle->state++;
        }
    }
    return FALSE;
}

void NetHandle_RecvTimingFromMachine(int netId, int size, const void *data, void *work, NetHandle *handle) {
    u16 timing = ((NetTimingData *)data)->timing;
    u8 netIds = ((NetTimingData *)data)->netIds;
    int i;

    if (NetHandle_GetNetId(handle) == GFL_NET_NETID_SERVER && netId != GFL_NET_NETID_SERVER) {
        _pBaseWork->timings[netId] = timing;
        _pBaseWork->timingSend[netId] = netIds;
        for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
            if (NetHandle_IsNegotiated(NetHandle_Get(i))) {
                if (netIds == _pBaseWork->timingSend[i]) {
                    if (timing != _pBaseWork->timings[i]) {
                        return;
                    }
                } else if ((1 << i) & netIds) {
                    return;
                }
            } else if (NetSystem_IsMachineConnected(i) == TRUE) {
                return;
            }
        }
        _pBaseWork->relayPending[netId] = TRUE;
    }
}

void NetHandle_RecvTimingFromServer(int netId, int size, const void *data, void *work, NetHandle *handle) {
    handle->timingRecv = *(u16 *)data;
}

void NetHandle_SendTiming(NetHandle *handle, u16 timing) {
    handle->timingSend = timing;
    handle->timingNetIds = 0xff;
    if (NetHandle_GetNetId(handle) == GFL_NET_NETID_SERVER) {
        handle->timingSendPending = FALSE;
    } else {
        handle->timingSendPending = TRUE;
    }
}

void NetHandle_SendTimingPair(NetHandle *handle, u32 a1, u32 a2) {
    if (handle != NULL) {
        handle->timingSend = a1 + (a2 << 8);
        handle->timingNetIds = 0xff;
        if (NetHandle_GetNetId(handle) == GFL_NET_NETID_SERVER) {
            handle->timingSendPending = FALSE;
        } else {
            handle->timingSendPending = TRUE;
        }
    }
}

BOOL NetHandle_IsTimingRecv(NetHandle *handle, u16 timing) {
    if (handle->timingRecv == timing) {
        return TRUE;
    }
    return FALSE;
}

BOOL NetHandle_IsTimingPairRecv(NetHandle *handle, u32 a1, u32 a2) {
    if (handle == NULL) {
        return TRUE;
    }
    if (handle->timingRecv == a1 + (a2 << 8)) {
        return TRUE;
    }
    return FALSE;
}

// Sends the server's negotiations that have not been sent
static BOOL NetHandle_SendServerNegotiations(NetHandle *handle) {
    int i;

    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        if (handle->negotiations[i]->valid && !NetHandle_TestBit(handle->negotiationsSent, i)) {
            if (GFL_NetSendCommandToMachine(handle, GFL_NET_NETID_SERVER, 4, sizeof(NetNegotiation),
                                            handle->negotiations[i], 1, 0, 1)) {
                NetHandle_SetBit(handle->negotiationsSent, i, TRUE);
            } else {
                return FALSE;
            }
        }
    }
    handle->negotiationsSent = 0;
    return TRUE;
}

void NetHandle_Update(void) {
    NetHandle *handle;
    int i;

    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        int netId = NetSystem_ClampNetId(i);

        handle = NetHandle_Get(netId);
        if (!NetSystem_IsMachineConnected(netId)) {
            NetHandle_IsNegotiated(handle);
        }
    }
    NetHandle_SendReplies();
    if (GFL_NetIsParent()) {
        handle = NetHandle_Get(GFL_NET_NETID_SERVER);
        for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
            if (_pBaseWork->relayPending[i] == TRUE &&
                GFL_NetSendCommandToBits(handle, _pBaseWork->timingSend[i], 8, 2, &_pBaseWork->timings[i], 0, 0, 0)) {
                GFL_ASSERT(_pBaseWork->timingSend[i]);
                _pBaseWork->relayPending[i] = FALSE;
            }
        }
    }
    for (i = 0; i < GFL_NET_HANDLE_MAX; i++) {
        handle = NetHandle_Get(NetSystem_ClampNetId(i));
        if (GFL_NetIsHandleNegotiated(handle) && handle->timingSendPending &&
            NetHandle_GetNetId(handle) != GFL_NET_NETID_SERVER) {
            if (GFL_NetSendCommand(handle, 7, 4, &handle->timingSend)) {
                handle->timingSendPending = FALSE;
            }
        }
    }
    if (NetSystem_GetNetId() == 0) {
        handle = NetHandle_Get(GFL_NET_NETID_SERVER);
        if (handle != NULL) {
            for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
                if (handle->negotiations[i] != NULL && handle->negotiations[i]->valid &&
                    !NetHandle_TestBit(handle->serverNegotiationsSent, i) && NetHandle_SendServerNegotiations(handle)) {
                    handle->serverNegotiationsSent = NetHandle_SetBit(handle->serverNegotiationsSent, i, TRUE);
                }
            }
        }
    }
    {
        GFLNetInitData *ini = GFL_NetGetInitData();

        if (ini->bNetType != GFL_NET_TYPE_IRC_WIRELESS) {
            handle = NetHandle_GetOwn();
            if (handle->infoPending == TRUE) {
                if (ini->getInfo != NULL && ini->getInfoSize != NULL) {
                    int size = ini->getInfoSize(GFL_NetGetWork());

                    if (GFL_NetSendCommand(handle, 10, size, ini->getInfo(GFL_NetGetWork()))) {
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
void NetHandle_RecvNegotiation(int netId, int size, u8 *data, void *work, NetHandle *handle) {
    GFLNetInitData *ini = GFL_NetGetInitData();

    if (NetHandle_Get(GFL_NET_NETID_SERVER) == handle) {
        NetNegotiation *negotiation = handle->negotiations[netId];

        if (!negotiation->valid) {
            negotiation->valid = TRUE;
            MI_CpuCopy8(data, negotiation->mac, sizeof(negotiation->mac));
            negotiation->netId = data[6];
            OS_GetMacAddress(negotiation->serverMac);
            if (data[8] != 0) {
                if (data[6] != 0 && data[7] != ini->gameCommandBase && ini->negotiationCommandBase == data[8]) {
                    negotiation->serverCommandBase = ini->negotiationCommandBase;
                    negotiation->machineCommandBase = ini->negotiationCommandBase;
                    return;
                }
                if (data[6] == 0 && data[7] == ini->gameCommandBase && ini->negotiationCommandBase == data[8]) {
                    negotiation->serverCommandBase = ini->negotiationCommandBase;
                    negotiation->machineCommandBase = ini->negotiationCommandBase;
                    return;
                }
                negotiation->serverCommandBase = 0xff;
                negotiation->machineCommandBase = 0;
            } else {
                negotiation->serverCommandBase = ini->gameCommandBase;
                negotiation->machineCommandBase = data[7];
            }
        }
    } else {
        handle->infoPending = TRUE;
    }
}

static void NetHandle_Reject(NetHandle *handle) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->rejectNegotiated == 0) {
        if (handle->state != NET_HANDLE_STATE_NEGOTIATED) {
            handle->state = NET_HANDLE_STATE_REJECTED;
        }
    } else {
        handle->state = NET_HANDLE_STATE_REJECTED;
    }
}

// A machine receives the server's reply to its negotiation
void NetHandle_RecvNegotiationReply(int netId, int size, NetNegotiation *negotiation, void *work, NetHandle *handle) {
    GFLNetInitData *ini = GFL_NetGetInitData();
    u8 mac[6];

    if (NetHandle_GetNetId(handle) == GFL_NET_NETID_SERVER) {
        handle->state = NET_HANDLE_STATE_NEGOTIATED;
        return;
    }
    if (ini->negotiationCommandBase != 0) {
        if (ini->negotiationCommandBase != negotiation->serverCommandBase) {
            NetHandle_Reject(handle);
        }
        if (ini->negotiationCommandBase != negotiation->machineCommandBase) {
            NetHandle_Reject(handle);
        }
    } else {
        if (ini->gameCommandBase != negotiation->serverCommandBase) {
            NetHandle_Reject(handle);
        }
        if (ini->gameCommandBase != negotiation->machineCommandBase) {
            NetHandle_Reject(handle);
        }
    }
    OS_GetMacAddress(mac);
    if (GFL_STD_MemCmp(negotiation->mac, mac, sizeof(mac)) == 0 && negotiation->netId != NetHandle_GetNetId(handle)) {
        NetHandle_Reject(handle);
    }
    if (handle->state != NET_HANDLE_STATE_REJECTED) {
        NetHandle_Get(negotiation->netId)->state = NET_HANDLE_STATE_NEGOTIATED;
        if (ini->negotiationCallback != NULL) {
            ini->negotiationCallback(work, negotiation->netId);
        }
        NetIw_AddCollectedMac(negotiation->mac);
        NetIw_AddCollectedMac(negotiation->serverMac);
    }
}

void NetHandle_RecvForget(int netId, int size, u8 *data) {
    NetHandle_Forget(*data);
}

static u16 NetHandle_SetBit(u16 bits, int netID, BOOL set) {
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

static BOOL NetHandle_TestBit(u16 bits, int netID) {
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

void NetHandle_Forget(int netID) {
    // clang-format off
    GFL_ASSERT(netID!=GFL_NET_NETID_SERVER);
    // clang-format on
    NetHandle_Reset(NetHandle_Get(netID), netID);
    NetHandle_ClearNegotiation(NetHandle_Get(GFL_NET_NETID_SERVER), netID);
}

// Keeps data from a machine
void NetHandle_RecvData(int netID, int size, const void *data, void *work, NetHandle *handle) {
    GFLNetInitData *ini = GFL_NetGetInitData();

    if (NetHandle_GetNetId(handle) != GFL_NET_NETID_SERVER) {
        NetHandle *target;

        GFL_ASSERT(netID < GFL_NET_HANDLE_MAX);
        target = NetHandle_Get(netID);
        if (target->data != NULL) {
            GFL_HeapFree(target->data);
        }
        target->data = GFL_HeapAllocate(ini->heapId, size, TRUE, "net_handle.c", 984);
        target->dataSize = size;
        MI_CpuCopy8(data, target->data, size);
    }
}

void NetHandle_SendTimingPairToMachines(NetHandle *handle, int a1, int a2, int a3) {
    if (handle != NULL) {
        handle->timingSend = a1 + (a2 << 8);
        handle->timingNetIds = a3;
        if (NetHandle_GetNetId(handle) == GFL_NET_NETID_SERVER) {
            handle->timingSendPending = FALSE;
        } else {
            handle->timingSendPending = TRUE;
        }
    }
}

void NetHandle_SetNegotiationCommandBase(NetHandle *handle, int value) {
    if (handle != NULL) {
        int i;

        for (i = 0; i < GFL_NET_HANDLE_MAX; i++) {
            NetNegotiation *negotiation = handle->negotiations[i];

            if (negotiation != NULL && negotiation->valid) {
                negotiation->machineCommandBase = value;
                negotiation->serverCommandBase = value;
            }
        }
    }
}
