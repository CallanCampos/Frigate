#include "types.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_queue.h"
#include "gfl/net_ring_buff.h"
#include "gfl/net_state.h"
#include "gfl/net_system.h"
#include "gfl/std.h"
#include "gfl/wm_icon.h"

// Packets between the machines: a header with a CRC, the size, the connected machines, flags and a sequence number,
// then commands from the send queue. Each machine's received commands go through a ring buffer to their handlers

// The packet header
#define PACKET_HEADER_SIZE 7
#define PACKET_CRC_LO 0
#define PACKET_CRC_HI 1
#define PACKET_SIZE 2
#define PACKET_CONNECT_BITS 3
#define PACKET_FLAGS 4
#define PACKET_SEQ 5
// A packet with nothing to send
#define PACKET_FLAGS_NONE 0xff
#define PACKET_FLAG_FIRST 1
#define PACKET_FLAG_CONTINUED 2
// The size field holds the size less 0xff
#define PACKET_FLAG_LONG 8

// Where a machine's commands are while they arrive
typedef struct {
    int pos;
    u8 *buffer;
    u16 remaining;
    u16 size;
    u16 command;
    u8 netId;
    u8 sender;
} NetRecvState;

typedef struct {
    u8 *sendData;
    u8 *sendRingData;
    u8 *mpSendData;
    u8 *mpRingData;
    u8 *recvData;
    u8 *serverRingData;
    u8 *recvTemp;
    NetRingBuff sendRing;
    NetRingBuff serverRing;
    NetRingBuff mpRing;
    NetRingBuff recvRings[GFL_NET_MACHINE_MAX];
    NetQueue sendQueue;
    NetQueue mpQueue;
    NetRecvState recvStates[GFL_NET_MACHINE_MAX];
    NetRecvState serverRecvState;
    // Whether the parent sends its MP packets (Wi-Fi turns it off while it waits)
    BOOL mpSendEnabled;
    int serverSends;
    // The packets sent to each machine that have not come back
    int sends[GFL_NET_MACHINE_MAX];
    u8 mpSeq;
    u8 sendSeq;
    u8 serverSeq;
    u8 recvSeq[GFL_NET_MACHINE_MAX];
    int recvSize;
    u16 connectBits;
    u8 unk1E6;
    // Set while the Wi-Fi is connected
    u8 wifiConnected;
    u8 unk1E8;
    u8 error;
    // Set by an error, which stops the sending and receiving
    u8 stopped;
    // Whether the last step of the data sharing went through
    u8 dataShareOk;
    u8 unk1EC;
    // Set while a Wi-Fi packet is written and not yet sent
    u8 wifiPacketReady;
} NetSystem;

typedef struct {
    u8 sendState;
    u8 recvState;
} NetSystemState;

// Records a network error, from the line it was found on
#define NET_ERROR(line)                                                                                                \
    do {                                                                                                               \
        GFLNetErrorInfo *err;                                                                                          \
        NetSystem_SetError();                                                                                          \
        err = NetState_GetErrorInfo();                                                                                 \
        if (!err->type) {                                                                                              \
            NetState_RecordError(err->code, err->errorType, err->error, line);                                         \
        }                                                                                                              \
    } while (0)

static void NetSystem_SetSendState(u8 state, int line);
static void NetRecvState_Reset(NetRecvState *state);
static int NetSystem_GetParentPacketSize(void);
static BOOL NetSystem_Alloc(int size, HeapID heapId, BOOL useSysSize);
static void NetSystem_ResetBuffers(void);
static void NetSystem_ResetMachine(int netID);
static BOOL NetSystem_RecvInfraredCallback(u16 netId, u8 *data, u16 size);
static void NetSystem_OnConnectAsParent(int netId);
static void NetSystem_OnConnectAsChild(int netId);
static void NetSystem_FreeWhenDone(void);
static BOOL NetSystem_BuildSendPacket(BOOL server);
static void NetSystem_SendOwnPacket(void);
static void NetSystem_UpdateDataSharing(void);
static void NetSystem_UpdateInfrared(void);
static BOOL NetSystem_RecvInfraredPacket(u16 netId, u8 *data, u16 size);
static void NetSystem_SendInfraredPacket(u8 *data, int size);
static void NetSystem_UpdateWifiSend(void);
static void NetSystem_UpdateOwnSend(void);
static void NetSystem_UpdateMPSendStep(void);
static BOOL NetSystem_AllSendsReturned(void);
static void NetSystem_CountSends(void);
static void NetSystem_UpdateMPSend(void);
static BOOL NetSystem_RecvPacket(u16 netId, u8 *data, u16 size);
static BOOL NetSystem_RecvServerPacket(u16 netId, u8 *data, u16 size);
static BOOL NetSystem_OnSendDone(BOOL ok);
static BOOL NetSystem_OnMPSendDone(BOOL ok);
static BOOL NetSystem_WritePacket(u8 *packet, NetQueue *queue, int size, u8 seq);
static void NetSystem_DispatchCommand(int command, int size, void *data, NetRecvState *state);
static void NetSystem_ReadCommands(NetRingBuff *ring, int netId, u8 *temp, NetRecvState *state, int size);
static void NetSystem_ReadServerRing(void);
static void NetSystem_ReadRecvRings(void);

static NetSystemState sState = { 4, 4 };

static NetSystem *sNetSys;

static void NetSystem_SetSendState(u8 state, int line) {
    sState.sendState = state;
}

static void NetRecvState_Reset(NetRecvState *state) {
    state->command = 0;
    state->remaining = 0xffff;
    state->size = 0xffff;
    state->buffer = NULL;
    state->pos = 0;
}

// The size of the packet the parent sends
static int NetSystem_GetParentPacketSize(void) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->aNetInit.bMPMode && sys->aNetInit.parentPacketSize) {
        return sys->aNetInit.parentPacketSize;
    }
    return GFL_NetGetMaxSendSize() * GFL_NetGetMaxConnectNum();
}

static BOOL NetSystem_Alloc(int size, HeapID heapId, BOOL useSysSize) {
    if (sNetSys == NULL) {
        int machineMax = GFL_NetGetMaxConnectNum();
        int sendSize = NetSystem_GetParentPacketSize();

        sNetSys = GFL_HeapAllocate(heapId, sizeof(NetSystem), TRUE, "net_system.c", 221);
        if (useSysSize) {
            sNetSys->recvSize = sendSize + 0x40;
        } else {
            sNetSys->recvSize = size + 0x40;
        }
        sNetSys->serverRingData = GFL_HeapAllocate(heapId, sNetSys->recvSize * 3, TRUE, "net_system.c", 230);
        sNetSys->recvTemp = GFL_HeapAllocate(heapId, sNetSys->recvSize * 2, TRUE, "net_system.c", 232);
        sNetSys->recvData = GFL_HeapAllocate(heapId, machineMax * sNetSys->recvSize, TRUE, "net_system.c", 233);
        sNetSys->sendData = GFL_HeapAllocate(heapId, GFL_NetGetMaxSendSize(), TRUE, "net_system.c", 236);
        sNetSys->sendRingData = GFL_HeapAllocate(heapId, 15 * GFL_NetGetMaxSendSize(), TRUE, "net_system.c", 237);
        sNetSys->mpSendData = GFL_HeapAllocate(heapId, sendSize, TRUE, "net_system.c", 238);
        sNetSys->mpRingData = GFL_HeapAllocate(heapId, sendSize * 2, TRUE, "net_system.c", 239);
        NetQueue_Init(&sNetSys->sendQueue, 100, &sNetSys->sendRing, heapId);
        NetQueue_Init(&sNetSys->mpQueue, 180, &sNetSys->mpRing, heapId);
        sNetSys->mpSendEnabled = TRUE;
        NetSystem_ResetBuffers();
    }
    sNetSys->connectBits = 0;
    sNetSys->wifiConnected = 0;
    return TRUE;
}

static void NetSystem_ResetBuffers(void) {
    int i;
    int machineMax = GFL_NetGetMaxConnectNum();
    int sendSize = NetSystem_GetParentPacketSize();
    int dataSize = GFL_NetGetMaxSendSize();

    MI_CpuFill8(sNetSys->recvData, 0, sNetSys->recvSize * machineMax);
    for (i = 0; i < machineMax; i++) {
        NetRingBuff_Init(&sNetSys->recvRings[i], sNetSys->recvData + i * sNetSys->recvSize, sNetSys->recvSize);
        sNetSys->recvSeq[i] = 0;
    }
    MI_CpuFill8(sNetSys->mpRingData, 0, (u32)sendSize * 2);
    NetRingBuff_Init(&sNetSys->mpRing, sNetSys->mpRingData, sendSize * 2);
    for (i = 0; i < sendSize; i++) {
        sNetSys->mpSendData[i] = 0;
    }
    MI_CpuFill8(sNetSys->sendRingData, 0, (u32)dataSize * 15);
    NetRingBuff_Init(&sNetSys->sendRing, sNetSys->sendRingData, dataSize * 15);
    sNetSys->sendData[0] = 0xff;
    for (i = 1; i < dataSize; i++) {
        sNetSys->sendData[i] = 0;
    }
    MI_CpuFill8(sNetSys->serverRingData, 0, sNetSys->recvSize * 3);
    NetRingBuff_Init(&sNetSys->serverRing, sNetSys->serverRingData, sNetSys->recvSize * 3);
    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        NetRecvState_Reset(&sNetSys->recvStates[i]);
        sNetSys->sends[i] = 0;
    }
    sNetSys->serverSends = 0;
    NetRecvState_Reset(&sNetSys->serverRecvState);
    NetSystem_SetSendState(4, 314);
    sState.recvState = 4;
    NetQueue_Clear(&sNetSys->sendQueue);
    NetQueue_Clear(&sNetSys->mpQueue);
    sNetSys->unk1E8 = 0;
    sNetSys->dataShareOk = 1;
    sNetSys->stopped = 0;
    sNetSys->wifiPacketReady = 0;
}

static void NetSystem_ResetMachine(int netID) {
    GFL_ASSERT(netID < GFL_NET_MACHINE_MAX);
    sNetSys->sends[netID] = 0;
    NetRingBuff_Init(&sNetSys->recvRings[netID], sNetSys->recvData + netID * sNetSys->recvSize, sNetSys->recvSize);
    sNetSys->recvSeq[netID] = 0;
    NetRecvState_Reset(&sNetSys->recvStates[netID]);
}

BOOL NetSystem_StartWireless(int size, HeapID heapId) {
    GFLNetInitData *ini = GFL_NetGetInitData();
    GFLNetSys *sys = GFL_NetGetSys();

    NetSystem_Alloc(size, heapId, sys->aNetInit.bMPMode && sys->aNetInit.parentPacketSize);
    sys->pDevTable->init(ini->heapId, sys, 0, sys->devWork);
    sys->pDevTable->start(0);
    sys->pDevTable->setRecvCallback(NetSystem_RecvPacket);
    return TRUE;
}

int NetSystem_CanConnectFriend(int a0) {
    GFLNetSys *sys = GFL_NetGetSys();

    return sys->pDevTable->canConnectFriend(a0);
}

static BOOL NetSystem_RecvInfraredCallback(u16 netId, u8 *data, u16 size) {
    return NetSystem_RecvInfraredPacket(netId, data, size);
}

BOOL NetSystem_StartInfrared(int a0) {
    GFLNetInitData *ini = GFL_NetGetInitData();
    GFLNetSys *sys = GFL_NetGetSys();

    ini->changeTgid = a0;
    sys->pDevTable->setRecvCallback(NetSystem_RecvInfraredCallback);
    NetSystem_Alloc(0x80, ini->heapId, FALSE);
    NetSystem_ResetMachine(0);
    NetSystem_ResetMachine(1);
    NetSystem_ResetMachine(2);
    return TRUE;
}

static void NetSystem_OnConnectAsParent(int netId) {
    GFLNetInitData *ini = GFL_NetGetInitData();
    GFLNetSys *sys = GFL_NetGetSys();

    NetSystem_ResetMachine(netId);
    if (ini->disconnectCallback != NULL) {
        ini->disconnectCallback(sys->devWork, netId);
    }
}

static void NetSystem_OnConnectAsChild(int netId) {
    GFLNetInitData *ini = GFL_NetGetInitData();
    GFLNetSys *sys = GFL_NetGetSys();

    NetSystem_ResetMachine(netId);
    if (ini->disconnectCallback != NULL) {
        ini->disconnectCallback(sys->devWork, netId);
    }
}

int NetSystem_StartParent(int a0, int size, int a2, BOOL (*callback)(void)) {
    GFLNetInitData *ini = GFL_NetGetInitData();
    GFLNetSys *sys = GFL_NetGetSys();
    BOOL useSysSize;

    ini->changeTgid = a0;
    sys->pDevTable->setDisconnectCallback(NetSystem_OnConnectAsParent);
    sys->pDevTable->setRecvCallback(NetSystem_RecvPacket);
    useSysSize = sys->aNetInit.bMPMode && sys->aNetInit.parentPacketSize;
    NetSystem_Alloc(size, GFL_NetGetInitData()->heapId, useSysSize);
    return sys->pDevTable->startParent(a2, callback);
}

BOOL NetSystem_StartScan(int a0, int size) {
    GFLNetSys *sys = GFL_NetGetSys();
    BOOL result;

    sys->pDevTable->setDisconnectCallback(NetSystem_OnConnectAsChild);
    result = sys->pDevTable->startScan(a0, 0);
    if (result == TRUE) {
        BOOL useSysSize;

        sys->pDevTable->setRecvCallback(NetSystem_RecvServerPacket);
        useSysSize = sys->aNetInit.bMPMode && sys->aNetInit.parentPacketSize;
        NetSystem_Alloc(size, GFL_NetGetInitData()->heapId, useSysSize);
        sState.recvState = 4;
    }
    return result;
}

BOOL NetSystem_StartSearch(int size) {
    GFLNetSys *sys = GFL_NetGetSys();
    BOOL result;

    sys->pDevTable->setDisconnectCallback(NetSystem_OnConnectAsChild);
    result = sys->pDevTable->startSearch();
    if (result == TRUE) {
        sys->pDevTable->setRecvCallback(NetSystem_RecvServerPacket);
        NetSystem_Alloc(size, GFL_NetGetInitData()->heapId, FALSE);
        sState.recvState = 4;
    }
    return result;
}

void NetSystem_HoldScan(int a0) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->pDevTable != NULL) {
        sys->pDevTable->holdScan(a0);
    }
}

int NetSystem_StartChild(BOOL a0, const u8 *mac, int size, void (*callback)(void)) {
    GFLNetSys *sys = GFL_NetGetSys();
    GFLNetInitData *ini = GFL_NetGetInitData();

    sys->pDevTable->setRecvCallback(NetSystem_RecvServerPacket);
    if (a0) {
        BOOL useSysSize = sys->aNetInit.bMPMode && sys->aNetInit.parentPacketSize;

        NetSystem_Alloc(size, GFL_NetGetInitData()->heapId, useSysSize);
        sState.recvState = 4;
    }
    return sys->pDevTable->startChild(a0, mac, 0, 0, callback);
}

void NetSystem_Free(void) {
    GFLNetSys *sys = GFL_NetGetSys();
    GFLNetInitData *ini = GFL_NetGetInitData();

    if (sNetSys != NULL) {
        GFL_HeapFree(sNetSys->serverRingData);
        GFL_HeapFree(sNetSys->recvTemp);
        GFL_HeapFree(sNetSys->recvData);
        NetQueue_Free(&sNetSys->mpQueue);
        NetQueue_Free(&sNetSys->sendQueue);
        GFL_HeapFree(sNetSys->sendData);
        GFL_HeapFree(sNetSys->sendRingData);
        GFL_HeapFree(sNetSys->mpSendData);
        GFL_HeapFree(sNetSys->mpRingData);
        GFL_HeapFree(sNetSys);
        sNetSys = NULL;
    }
}

static void NetSystem_FreeWhenDone(void) {
    if (sNetSys->unk1EC) {
        if (NetSystem_GetNetId() == 0) {
            if (NetSystem_GetChildBits() == 0) {
                NetSystem_Free();
            }
        } else {
            NetSystem_Free();
        }
    }
}

void NetSystem_ForgetMachine(u32 netId) {
    if (sNetSys != NULL && netId < GFL_NET_MACHINE_MAX) {
        NetSystem_ResetMachine(netId);
    }
}

// Sends and receives, each frame
BOOL NetSystem_Update(void) {
    GFLNetSys *sys = GFL_NetGetSys();
    GFLNetInitData *ini = GFL_NetGetInitData();

    if (sNetSys == NULL) {
        if (sys != NULL) {
            sys->pDevTable->update(0);
        }
        return TRUE;
    }
    if (sys->pDevTable->getSignalLevel != NULL) {
        WmIcon_SetStrength(sys->pDevTable->getSignalLevel());
    }
    if (!sNetSys->stopped && !NetSystem_HasError()) {
        if (!ini->bMPMode &&
            (ini->bNetType == GFL_NET_TYPE_WIRELESS || ini->bNetType == GFL_NET_TYPE_WIRELESS_LISTEN)) {
            NetSystem_UpdateDataSharing();
            NetSystem_ReadRecvRings();
        } else if (ini->bNetType == GFL_NET_TYPE_IRC || ini->bNetType == GFL_NET_TYPE_IRC_WIRELESS) {
            NetSystem_UpdateInfrared();
            NetSystem_ReadRecvRings();
        } else if (ini->bNetType == GFL_NET_TYPE_WIFI || ini->bNetType == GFL_NET_TYPE_WIFI_LOBBY) {
            NetSystem_UpdateWifiSend();
            NetSystem_ReadRecvRings();
        } else {
            NetSystem_UpdateOwnSend();
            NetSystem_ReadServerRing();
            if (!NetSystem_GetNetId() && NetSystem_IsMachineConnected(0) && ini->bMPMode) {
                NetSystem_UpdateMPSend();
            }
            if (!NetSystem_GetNetId()) {
                NetSystem_ReadRecvRings();
            }
        }
        if (ini->bNetType != GFL_NET_TYPE_WIFI && ini->bNetType != GFL_NET_TYPE_WIFI_LOBBY) {
            sys->pDevTable->update(sNetSys->connectBits);
        }
        NetCommand_UpdateHugeSend();
        NetSystem_FreeWhenDone();
    } else {
        sys->pDevTable->update(0);
    }
    return TRUE;
}

void NetSystem_ResetBuffersIfAllocated(void) {
    if (sNetSys != NULL) {
        NetSystem_ResetBuffers();
    }
}

// Writes the next packet to send
static BOOL NetSystem_BuildSendPacket(BOOL server) {
    BOOL result = FALSE;

    if (sState.recvState != 4) {
        return result;
    }
    if (server) {
        NetSystem *net = sNetSys;

        if (net->serverSends > 0) {
            return result;
        }
        if (NetSystem_WritePacket(net->sendData, &net->sendQueue, GFL_NetGetMaxSendSize(), net->sendSeq)) {
            sNetSys->sendSeq++;
        }
        sState.recvState = 0;
        result = TRUE;
    } else {
        NetSystem *net = sNetSys;

        if (NetSystem_WritePacket(net->sendData, &net->sendQueue, GFL_NetGetMaxSendSize(), net->sendSeq)) {
            sNetSys->sendSeq++;
            sState.recvState = 0;
            result = TRUE;
        }
    }
    return result;
}

static void NetSystem_SendOwnPacket(void) {
    GFLNetSys *sys = GFL_NetGetSys();
    int dataSize = GFL_NetGetMaxSendSize();

    if (sys->pDevTable->isConnected() && NetSystem_IsMachineConnected(NetSystem_GetNetId()) && sState.recvState == 0) {
        if (NetSystem_GetNetId() != 0) {
            if (sys->pDevTable->send(sNetSys->sendData, dataSize, 0, NetSystem_OnSendDone)) {
                sState.recvState = 2;
                sNetSys->serverSends++;
            }
        } else {
            sState.recvState = 2;
            NetSystem_OnSendDone(TRUE);
            NetSystem_RecvPacket(0, sNetSys->sendData, dataSize);
            sNetSys->serverSends++;
        }
    }
}

static void NetSystem_UpdateDataSharing(void) {
    int i;
    GFLNetSys *sys = GFL_NetGetSys();

    if (sys->pDevTable->isDataSharing()) {
        if (sNetSys->dataShareOk) {
            NetSystem_BuildSendPacket(TRUE);
        }
        if (sys->pDevTable->stepDataSharing(sNetSys->sendData) == TRUE) {
            int dataSize;
            int machineMax;

            sState.recvState = 4;
            sNetSys->dataShareOk = TRUE;
            dataSize = GFL_NetGetMaxSendSize();
            machineMax = GFL_NetGetMaxConnectNum();
            for (i = 0; i < machineMax; i++) {
                NetSystem_RecvPacket(i, sys->pDevTable->getRecvData(i), dataSize);
            }
        } else {
            sNetSys->dataShareOk = FALSE;
        }
    }
}

static void NetSystem_UpdateInfrared(void) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (NetQueue_IsEmpty(&sNetSys->sendQueue)) {
        GFL_NetSendCommand(NetHandle_GetOwn(), 0x10, 0, NULL);
    }
    if (sys->pDevTable->poll != NULL) {
        sys->pDevTable->poll();
    }
    if (sys->pDevTable->isRunning() == TRUE) {
        int dataSize = GFL_NetGetMaxSendSize();

        if (sys->pDevTable->canSend() == TRUE && NetSystem_BuildSendPacket(FALSE)) {
            NetSystem_SendInfraredPacket(sNetSys->sendData, dataSize);
            sState.recvState = 4;
        }
    }
}

static BOOL NetSystem_RecvInfraredPacket(u16 netId, u8 *data, u16 size) {
    int crc;
    int len;

    GFL_NetGetInitData();
    GFL_NetGetMaxSendSize();
    if (data[PACKET_FLAGS] == PACKET_FLAGS_NONE) {
        return FALSE;
    }
    if (!sNetSys->recvRings[netId].received && (data[PACKET_FLAGS] & PACKET_FLAG_CONTINUED)) {
        return FALSE;
    }
    crc = getCRC16(data + PACKET_SIZE, size - 2);
    if (data[PACKET_CRC_LO] != (u8)crc || (crc & 0xff00) >> 8 != data[PACKET_CRC_HI]) {
        return FALSE;
    }
    if (sNetSys->recvSeq[netId] != 0 && (u8)(data[PACKET_SEQ] - 1) != sNetSys->recvSeq[netId]) {
        return FALSE;
    }
    sNetSys->recvSeq[netId] = data[PACKET_SEQ];
    len = data[PACKET_SIZE];
    if (data[PACKET_FLAGS] & PACKET_FLAG_LONG) {
        len += 0xff;
    }
    NetRingBuff_Write(&sNetSys->recvRings[netId], data + PACKET_HEADER_SIZE, len);
    sNetSys->recvRings[netId].received = TRUE;
    return TRUE;
}

static void NetSystem_SendInfraredPacket(u8 *data, int size) {
    GFLNetSys *sys;

    GFL_NetGetInitData();
    sys = GFL_NetGetSys();
    if (!sys->pDevTable->isSendTurn() || !sys->pDevTable->isLinked()) {
        GFL_ASSERT(0);
        return;
    }
    if (sState.recvState == 0) {
        sys->pDevTable->send(data, size, 0, NULL);
        NetSystem_RecvInfraredPacket(sys->pDevTable->getNetId(), data, size);
        NetSystem_SetSendState(4, 1036);
    }
}

static void NetSystem_UpdateWifiSend(void) {
    GFLNetSys *sys;
    int dataSize;
    NetSystem *net;

    GFL_NetGetInitData();
    sys = GFL_NetGetSys();
    dataSize = GFL_NetGetMaxSendSize();
    net = sNetSys;
    if (net->wifiConnected) {
        if (!net->wifiPacketReady) {
            if (NetSystem_WritePacket(net->sendData, &net->sendQueue, GFL_NetGetMaxSendSize(), net->sendSeq)) {
                sNetSys->wifiPacketReady = TRUE;
            }
        }
        if (sNetSys->wifiPacketReady == TRUE) {
            if (sys->pDevTable->send(sNetSys->sendData, dataSize, 0, NULL)) {
                sNetSys->wifiPacketReady = FALSE;
            }
        }
    }
}

static void NetSystem_UpdateOwnSend(void) {
    GFLNetSys *sys;

    GFL_NetGetInitData();
    sys = GFL_NetGetSys();
    if (sys->pDevTable->isConnected() && NetSystem_IsMachineConnected(NetSystem_GetNetId())) {
        NetSystem_BuildSendPacket(TRUE);
        NetSystem_SendOwnPacket();
    }
}

static void NetSystem_UpdateMPSendStep(void) {
    GFLNetSys *sys = GFL_NetGetSys();

    GFL_NetGetMaxConnectNum();
    if (sState.sendState == 1 && NetSystem_CountConnected() > 1) {
        NetSystem_SetSendState(2, 1130);
        if (sys->pDevTable->isConnected()) {
            if (!sys->pDevTable->send(sNetSys->mpSendData, NetSystem_GetParentPacketSize(), 0,
                                      NetSystem_OnMPSendDone)) {
                NetSystem_SetSendState(1, 1134);
            }
        } else if (GFL_NetGetInitData()->bNetType == GFL_NET_TYPE_IRC ||
                   GFL_NetGetInitData()->bNetType == GFL_NET_TYPE_IRC_WIRELESS) {
            if (!sys->pDevTable->canSend()) {
                NetSystem_SetSendState(1, 1141);
            } else {
                if (sNetSys->mpSendData[PACKET_SIZE] == 0) {
                    return;
                }
                sys->pDevTable->send(sNetSys->mpSendData, GFL_NetGetMaxSendSize(), 100, NULL);
            }
        }
    }
    if (sState.sendState == 2) {
        NetSystem_CountSends();
        NetSystem_RecvServerPacket(0, sNetSys->mpSendData, NetSystem_GetParentPacketSize());
        NetSystem_SetSendState(3, 1165);
    }
    if (!sys->pDevTable->isConnected() || GFL_NetGetInitData()->bNetType == GFL_NET_TYPE_IRC ||
        GFL_NetGetInitData()->bNetType == GFL_NET_TYPE_IRC_WIRELESS) {
        NetSystem_SetSendState(4, 1170);
    }
}

// Whether no machine has packets that have not come back
static BOOL NetSystem_AllSendsReturned(void) {
    int i;

    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        if (NetSystem_IsMachineConnected(i) && sNetSys->sends[i] > 0) {
            return FALSE;
        }
    }
    return TRUE;
}

static void NetSystem_CountSends(void) {
    int i;
    int machineMax = GFL_NetGetMaxConnectNum();

    for (i = 0; i < machineMax; i++) {
        if (NetSystem_IsMachineConnected(i)) {
            sNetSys->sends[i]++;
        }
    }
}

static void NetSystem_UpdateMPSend(void) {
    GFLNetSys *sys = GFL_NetGetSys();
    GFLNetInitData *pNetIni = GFL_NetGetInitData();

    if (pNetIni->bNetType == GFL_NET_TYPE_WIFI || pNetIni->bNetType == GFL_NET_TYPE_WIFI_LOBBY) {
        if (!NetSystem_IsMachineConnected(0)) {
            return;
        }
        GFL_ASSERT(!pNetIni->bMPMode);
        if (sState.sendState == 4 && sNetSys->mpSendEnabled && NetSystem_AllSendsReturned()) {
            NetSystem_SetSendState(1, 1242);
        }
        if (sState.sendState == 1) {
            if (sys->pDevTable->send(sNetSys->mpSendData, NetSystem_GetParentPacketSize(), 0, NULL)) {
                NetSystem_SetSendState(4, 1249);
                NetSystem_CountSends();
            }
        }
    } else if (pNetIni->bNetType == GFL_NET_TYPE_IRC || pNetIni->bNetType == GFL_NET_TYPE_IRC_WIRELESS) {
        if (sState.sendState == 4) {
            NetSystem *net = sNetSys;

            NetSystem_WritePacket(net->mpSendData, &net->mpQueue, NetSystem_GetParentPacketSize(), net->mpSeq);
            sNetSys->mpSeq++;
            NetSystem_SetSendState(1, 1268);
        }
        NetSystem_UpdateMPSendStep();
    } else if (sys->pDevTable->isConnected() && NetSystem_AllSendsReturned()) {
        if (sState.sendState == 4) {
            NetSystem *net = sNetSys;

            NetSystem_WritePacket(net->mpSendData, &net->mpQueue, NetSystem_GetParentPacketSize(), net->mpSeq);
            sNetSys->mpSeq++;
            NetSystem_SetSendState(1, 1281);
        }
        NetSystem_UpdateMPSendStep();
    }
}

static BOOL NetSystem_RecvPacket(u16 netId, u8 *data, u16 size) {
    int crc;
    int len;

    GFL_NetGetMaxSendSize();
    if (data == NULL) {
        return FALSE;
    }
    sNetSys->sends[netId]--;
    if (netId == 0) {
        sNetSys->connectBits = data[PACKET_CONNECT_BITS];
    }
    if (data[PACKET_FLAGS] == PACKET_FLAGS_NONE) {
        return FALSE;
    }
    if (!sNetSys->recvRings[netId].received && (data[PACKET_FLAGS] & PACKET_FLAG_CONTINUED)) {
        return FALSE;
    }
    crc = getCRC16(data + PACKET_SIZE, size - 2);
    if (data[PACKET_CRC_LO] != (u8)crc || (crc & 0xff00) >> 8 != data[PACKET_CRC_HI]) {
        NET_ERROR(1010);
        return FALSE;
    }
    if (sNetSys->recvSeq[netId] != 0 && (u8)(data[PACKET_SEQ] - 1) != sNetSys->recvSeq[netId]) {
        NET_ERROR(1011);
        return FALSE;
    }
    sNetSys->recvSeq[netId] = data[PACKET_SEQ];
    len = data[PACKET_SIZE];
    if (data[PACKET_FLAGS] & PACKET_FLAG_LONG) {
        len += 0xff;
    }
    NetRingBuff_Write(&sNetSys->recvRings[netId], data + PACKET_HEADER_SIZE, len);
    sNetSys->recvRings[netId].received = TRUE;
    return TRUE;
}

static BOOL NetSystem_RecvServerPacket(u16 netId, u8 *data, u16 size) {
    int crc;
    int len;

    GFL_NetGetInitData();
    GFL_NetGetMaxSendSize();
    sNetSys->serverSends--;
    if (data == NULL) {
        return FALSE;
    }
    if (netId == 0) {
        sNetSys->connectBits = data[PACKET_CONNECT_BITS];
    }
    if (data[PACKET_FLAGS] == PACKET_FLAGS_NONE) {
        return FALSE;
    }
    if (!sNetSys->serverRing.received && (data[PACKET_FLAGS] & PACKET_FLAG_CONTINUED)) {
        return FALSE;
    }
    crc = getCRC16(data + PACKET_SIZE, size - 2);
    if (data[PACKET_CRC_LO] != (u8)crc || (crc & 0xff00) >> 8 != data[PACKET_CRC_HI]) {
        NET_ERROR(1010);
        return FALSE;
    }
    sNetSys->serverSeq = data[PACKET_SEQ];
    len = data[PACKET_SIZE];
    if (data[PACKET_FLAGS] & PACKET_FLAG_LONG) {
        len += 0xff;
    }
    NetRingBuff_Write(&sNetSys->serverRing, data + PACKET_HEADER_SIZE, len);
    sNetSys->serverRing.received = TRUE;
    return TRUE;
}

static BOOL NetSystem_OnSendDone(BOOL ok) {
    if (ok && sState.recvState == 2) {
        sState.recvState = 4;
    } else {
        GFL_ASSERT(0);
    }
    return TRUE;
}

static BOOL NetSystem_OnMPSendDone(BOOL ok) {
    GFL_NetGetMaxSendSize();
    GFL_NetGetMaxConnectNum();
    GFL_NetGetInitData();
    if (ok) {
        NetSystem_SetSendState(4, 1492);
    } else {
        GFL_ASSERT(0);
    }
    return TRUE;
}

static BOOL NetSystem_WritePacket(u8 *packet, NetQueue *queue, int size, u8 seq) {
    GFLNetInitData *ini = GFL_NetGetInitData();
    GFLNetSys *sys = GFL_NetGetSys();
    NetSendBuffer buffer;
    int len;
    int crc;

    MI_CpuFill8(packet, 0, size);
    packet[PACKET_FLAGS] = queue->pending == FALSE ? PACKET_FLAG_FIRST : PACKET_FLAG_CONTINUED;
    queue->pending = FALSE;
    packet[PACKET_CONNECT_BITS] = sys->pDevTable->getConnectBits();
    if (NetQueue_IsEmpty(queue) || ((ini->bNetType == GFL_NET_TYPE_IRC || ini->bNetType == GFL_NET_TYPE_IRC_WIRELESS) &&
                                    sys->pDevTable->unkB0() == TRUE)) {
        packet[PACKET_FLAGS] = PACKET_FLAGS_NONE;
        return FALSE;
    }
    buffer.size = size - PACKET_HEADER_SIZE;
    buffer.ptr = packet + PACKET_HEADER_SIZE;
    if (!NetQueue_FillBuffer(queue, &buffer)) {
        queue->pending = TRUE;
    }
    if (buffer.size != 0xffff) {
        len = size - buffer.size - PACKET_HEADER_SIZE;
    } else {
        len = size - PACKET_HEADER_SIZE;
    }
    if (len > 0xff) {
        packet[PACKET_FLAGS] |= PACKET_FLAG_LONG;
        // The low byte of len - 0xff
        len++;
    }
    packet[PACKET_SIZE] = len;
    packet[PACKET_SEQ] = seq;
    crc = getCRC16(packet + PACKET_SIZE, size - 2);
    packet[PACKET_CRC_LO] = crc;
    packet[PACKET_CRC_HI] = (crc & 0xff00) >> 8;
    return TRUE;
}

// Queues a command to send to a machine, or every machine for GFL_NET_NETID_SERVER
BOOL NetSystem_QueueCommand(int command, u8 *data, int size, u32 unused, int dest, int netId, BOOL noCopy) {
    BOOL copy = TRUE;
    GFLNetInitData *ini;
    NetQueue *queue;

    if (noCopy) {
        copy = FALSE;
    }
    ini = GFL_NetGetInitData();
    if (sNetSys == NULL) {
        return FALSE;
    }
    if (NetSystem_HasErrorChecked() || NetState_GetError()) {
        return FALSE;
    }
    if (!NetSystem_IsDataShareOk()) {
        return FALSE;
    }
    if (size >= sNetSys->recvSize && copy == TRUE) {
        copy = FALSE;
    }
    if (ini->bMPMode && dest == GFL_NET_NETID_SERVER) {
        queue = &sNetSys->mpQueue;
    } else {
        queue = &sNetSys->sendQueue;
    }
    if (NetQueue_Push(queue, command, data, size, unused, copy, netId)) {
        return TRUE;
    }
    NET_ERROR(1012);
    return FALSE;
}

int NetSystem_GetSendRingUsed(void) {
    return NetRingBuff_GetUsedSize(&sNetSys->sendRing);
}

int NetSystem_GetMPRingUsed(void) {
    return NetRingBuff_GetUsedSize(&sNetSys->mpRing);
}

// Hands a command that has arrived to its handler, and for the server to the server's handler too
static void NetSystem_DispatchCommand(int command, int size, void *data, NetRecvState *state) {
    NetHandle *handle;

    GFL_NetGetInitData();
    handle = NetHandle_Get(NetSystem_GetNetId());
    NetCommand_Dispatch(state->netId, state->sender, command, size, data, handle);
    if (NetSystem_GetNetId() == 0) {
        handle = NetHandle_Get(GFL_NET_NETID_SERVER);
        if (command < 0x11) {
            NetCommand_Dispatch(state->netId, state->sender, command, size, data, handle);
        }
    }
    NetRecvState_Reset(state);
}

// Reads the commands that have arrived in a ring buffer
static void NetSystem_ReadCommands(NetRingBuff *ring, int netId, u8 *temp, NetRecvState *state, int size) {
    u8 *dest;
    int start;
    int read;
    int copied;
    int command;
    int savedStart;

    GFL_NetGetInitData();
    if (NetRingBuff_GetUsedSize(ring) == 0) {
        return;
    }
    do {
        command = state->command;
        if (command == 0) {
            command = NetRingBuff_ReadU16(ring);
            if (command == 0) {
                GFL_ASSERT(0);
                continue;
            }
        }
        savedStart = ring->startPos;
        state->command = command;
        if (state->size == 0xffff) {
            if (NetRingBuff_GetUsedSize(ring) < 3) {
                ring->startPos = savedStart;
                return;
            }
            u16 commandSize = NetRingBuff_ReadU16(ring);

            state->remaining = commandSize;
            state->size = commandSize;
            state->netId = netId;
            state->sender = NetRingBuff_ReadByte(ring);
        }
        start = ring->startPos;
        if (NetCommand_HasBufferFunc(command) && NetCommand_IsForThisMachine(state->sender)) {
            if (state->buffer == NULL) {
                state->buffer = NetCommand_GetRecvBuffer(command, state->netId, state->size);
            }
            dest = state->buffer + state->pos;
        } else {
            state->buffer = temp;
            dest = temp;
        }
        if (!NetCommand_IsForThisMachine(state->sender)) {
            dest = NULL;
        }
        if (!NetCommand_HasBufferFunc(command)) {
            read = NetRingBuff_Read(ring, temp, state->remaining, state->remaining);
        } else {
            read = NetRingBuff_Read(ring, temp, state->remaining, size - PACKET_HEADER_SIZE);
        }
        copied = read;
        state->remaining -= (u16)read;
        if (state->size > state->pos) {
            copied = state->size - state->pos;
            if (copied > read) {
                copied = read;
            }
            if (dest != NULL) {
                MI_CpuCopy8(temp, dest, copied);
            }
        }
        state->pos += copied;
        if (state->remaining == 0) {
            NetSystem_DispatchCommand(command, state->size, state->buffer, state);
            continue;
        }
        if (NetCommand_HasCallback(command) && !NetCommand_HasBufferFunc(command) &&
            NetCommand_IsForThisMachine(state->sender)) {
            state->remaining += (u16)read;
            ring->startPos = start;
        }
        return;
    } while (NetRingBuff_GetUsedSize(ring) != 0);
}

static void NetSystem_ReadServerRing(void) {
    int i;

    if (sNetSys != NULL) {
        NetRingBuff_Commit(&sNetSys->serverRing);
        for (i = 0; i < 4; i++) {
            NetSystem *net;

            if (NetRingBuff_GetUsedSize(&sNetSys->serverRing) <= 0) {
                break;
            }
            net = sNetSys;
            NetSystem_ReadCommands(&net->serverRing, GFL_NET_NETID_SERVER, net->recvTemp, &net->serverRecvState,
                                   NetSystem_GetParentPacketSize());
        }
    }
}

static void NetSystem_ReadRecvRings(void) {
    int machineMax;
    int dataSize = GFL_NetGetMaxSendSize();
    int i;
    int j;

    if (sNetSys != NULL) {
        machineMax = GFL_NetGetMaxConnectNum();

        for (i = 0; i < machineMax; i++) {
            NetRingBuff_Commit(&sNetSys->recvRings[i]);
            for (j = 0; j < 4; j++) {
                if (NetRingBuff_GetUsedSize(&sNetSys->recvRings[i]) > 0) {
                    NetSystem *net = sNetSys;

                    NetSystem_ReadCommands(&net->recvRings[i], i, net->recvTemp, &net->recvStates[i], dataSize);
                }
            }
        }
    }
}

BOOL NetSystem_IsMachineConnected(u16 netId) {
    GFLNetSys *sys = GFL_NetGetSys();
    GFLNetInitData *ini = GFL_NetGetInitData();

    if (sNetSys == NULL) {
        return FALSE;
    }
    if (ini->bNetType == GFL_NET_TYPE_WIFI || ini->bNetType == GFL_NET_TYPE_WIFI_LOBBY) {
        if (sNetSys->wifiConnected && sys->pDevTable->getNetId() != -1) {
            if (netId == 0) {
                return TRUE;
            }
            if (netId == 1) {
                return TRUE;
            }
        }
        return FALSE;
    }
    if (ini->bNetType == GFL_NET_TYPE_IRC || ini->bNetType == GFL_NET_TYPE_IRC_WIRELESS) {
        if ((netId < 2 || netId == GFL_NET_NETID_SERVER) && sys->pDevTable->isConnected()) {
            return TRUE;
        }
        return FALSE;
    }
    if (!NetSystem_IsDeviceReady()) {
        return FALSE;
    }
    if (!sys->pDevTable->isConnected()) {
        return FALSE;
    }
    if (netId == NetSystem_GetNetId()) {
        return TRUE;
    }
    if (NetSystem_GetNetId() == 0 || !ini->bMPMode) {
        u16 bits = sys->pDevTable->getConnectBits();

        if ((1 << netId) & bits) {
            return TRUE;
        }
    } else {
        u16 bits = sNetSys->connectBits;

        if (bits & (1 << netId)) {
            return TRUE;
        }
    }
    return FALSE;
}

int NetSystem_CountConnected(void) {
    int count = 0;
    int i;

    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        if (NetSystem_IsMachineConnected(i)) {
            count++;
        }
    }
    return count;
}

BOOL NetSystem_IsDeviceReady(void) {
    GFLNetSys *sys = GFL_NetGetSys();
    GFLNetInitData *ini = GFL_NetGetInitData();

    if (sNetSys != NULL && (ini->bNetType == GFL_NET_TYPE_WIFI || ini->bNetType == GFL_NET_TYPE_WIFI_LOBBY ||
                            ini->bNetType == GFL_NET_TYPE_IRC || ini->bNetType == GFL_NET_TYPE_IRC_WIRELESS)) {
        return TRUE;
    }
    return sys->pDevTable->isRunning();
}

u16 NetSystem_GetNetId(void) {
    if (sNetSys != NULL) {
        GFLNetSys *sys = GFL_NetGetSys();
        GFLNetInitData *ini = GFL_NetGetInitData();

        if (ini->bNetType == GFL_NET_TYPE_WIFI || ini->bNetType == GFL_NET_TYPE_WIFI_LOBBY) {
            int netId = sys->pDevTable->getNetId();

            if (netId != -1) {
                return netId;
            }
        } else {
            return sys->pDevTable->getNetId();
        }
    }
    return 0;
}

u32 NetSystem_GetChildBits(void) {
    GFLNetSys *sys = GFL_NetGetSys();

    return (u16)sys->pDevTable->getConnectBits() & 0xfffe;
}

BOOL NetSystem_HasError(void) {
    GFLNetSys *sys = GFL_NetGetSys();

    if (sNetSys != NULL && sNetSys->error) {
        return TRUE;
    }
    if (sys->pDevTable->isError()) {
        return TRUE;
    }
    return FALSE;
}

BOOL NetSystem_HasErrorChecked(void) {
    GFLNetSys *sys = GFL_NetGetSys();
    BOOL error = FALSE;

    if (sys != NULL && sys->pDevTable != NULL) {
        error = sys->pDevTable->isError();
    }
    if (!error && sNetSys != NULL && sNetSys->error) {
        error = TRUE;
    }
    return error;
}

BOOL NetSystem_HasQueuedCommand(u16 command, int dest) {
    if (GFL_NetGetInitData()->bMPMode && dest == GFL_NET_NETID_SERVER) {
        return NetQueue_HasCommand(&sNetSys->mpQueue, command);
    }
    return NetQueue_HasCommand(&sNetSys->sendQueue, command);
}

BOOL NetSystem_IsSendQueueEmpty(void) {
    return NetQueue_IsEmpty(&sNetSys->sendQueue);
}

void NetSystem_SetWifiConnected(u8 a0) {
    sNetSys->wifiConnected = a0;
}

void NetSystem_SetMPSendEnabled(BOOL a0) {
    GFLNetInitData *ini = GFL_NetGetInitData();

    if (ini->bNetType == GFL_NET_TYPE_WIFI || ini->bNetType == GFL_NET_TYPE_WIFI_LOBBY) {
        sNetSys->mpSendEnabled = a0;
        if (a0) {
            sNetSys->serverSends = 0;
            sNetSys->sends[0] = 0;
            sNetSys->sends[1] = 0;
        }
    }
}

void NetSystem_SetError(void) {
    sNetSys->error = TRUE;
}

void NetSystem_ClearError(void) {
    sNetSys->error = FALSE;
}

void NetSystem_Stop(void) {
    if (sNetSys != NULL) {
        sNetSys->stopped = TRUE;
    }
}

BOOL NetSystem_IsDataShareOk(void) {
    if (sNetSys != NULL) {
        GFLNetInitData *ini = GFL_NetGetInitData();

        if (!ini->bMPMode &&
            (ini->bNetType == GFL_NET_TYPE_WIRELESS || ini->bNetType == GFL_NET_TYPE_WIRELESS_LISTEN)) {
            return sNetSys->dataShareOk;
        }
    }
    return TRUE;
}

int NetSystem_ClampNetId(int netId) {
    if (netId >= GFL_NET_MACHINE_MAX) {
        netId = GFL_NET_NETID_SERVER;
    }
    return netId;
}
