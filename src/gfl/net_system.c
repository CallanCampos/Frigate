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

#define GFL_NET_MACHINE_MAX 8

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
    BOOL unk1AC;
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
    u8 unk1E7;
    u8 unk1E8;
    u8 error;
    u8 unk1EA;
    u8 unk1EB;
    u8 unk1EC;
    u8 unk1ED;
} NetSystem;

typedef struct {
    u8 sendState;
    u8 recvState;
} NetSystemState;

// Records a network error, from the line it was found on
#define NET_ERROR(line)                                                                                                \
    do {                                                                                                               \
        GFLNetErrorInfo *err;                                                                                          \
        func_02040158();                                                                                               \
        err = func_02042540();                                                                                         \
        if (!err->type) {                                                                                              \
            func_020424ac(err->code, err->unk4, err->unk8, line);                                                      \
        }                                                                                                              \
    } while (0)

static void func_0203e8c8(u8 state, int line);
static void func_0203e8d4(NetRecvState *state);
static int func_0203e8e8(void);
static BOOL func_0203e910(int size, HeapID heapId, BOOL useSysSize);
static void func_0203ea5c(void);
static void func_0203ebe0(int netID);
static BOOL func_0203ecc8(u16 netId, u8 *data, u16 size);
static void func_0203ed18(int netId);
static void func_0203ed40(int netId);
static void func_0203efa0(void);
static BOOL func_0203f10c(BOOL server);
static void func_0203f19c(void);
static void func_0203f234(void);
static void func_0203f2cc(void);
static BOOL func_0203f348(u16 netId, u8 *data, u16 size);
static void func_0203f3f0(u8 *data, int size);
static void func_0203f46c(void);
static void func_0203f4f0(void);
static void func_0203f520(void);
static BOOL func_0203f63c(void);
static void func_0203f670(void);
static void func_0203f6a8(void);
static BOOL func_0203f7f0(u16 netId, u8 *data, u16 size);
static BOOL func_0203f8f8(u16 netId, u8 *data, u16 size);
static BOOL func_0203f9c0(BOOL ok);
static BOOL func_0203f9f0(BOOL ok);
static BOOL func_0203fa28(u8 *packet, NetQueue *queue, int size, u8 seq);
static void func_0203fbe0(int command, int size, void *data, NetRecvState *state);
static void func_0203fc34(NetRingBuff *ring, int netId, u8 *temp, NetRecvState *state, int size);
static void func_0203fdbc(void);
static void func_0203fe00(void);

static NetSystemState sState = { 4, 4 };

static NetSystem *sNetSys;

static void func_0203e8c8(u8 state, int line) {
    sState.sendState = state;
}

static void func_0203e8d4(NetRecvState *state) {
    state->command = 0;
    state->remaining = 0xffff;
    state->size = 0xffff;
    state->buffer = NULL;
    state->pos = 0;
}

// The size of the packet the parent sends
static int func_0203e8e8(void) {
    GFLNetSys *sys = func_02042e78();

    if (sys->aNetInit.bMPMode && sys->aNetInit.unk6C) {
        return sys->aNetInit.unk6C;
    }
    return func_02042de8() * func_02042dc0();
}

static BOOL func_0203e910(int size, HeapID heapId, BOOL useSysSize) {
    if (sNetSys == NULL) {
        int machineMax = func_02042dc0();
        int sendSize = func_0203e8e8();

        sNetSys = GFL_HeapAllocate(heapId, sizeof(NetSystem), TRUE, "net_system.c", 221);
        if (useSysSize) {
            sNetSys->recvSize = sendSize + 0x40;
        } else {
            sNetSys->recvSize = size + 0x40;
        }
        sNetSys->serverRingData = GFL_HeapAllocate(heapId, sNetSys->recvSize * 3, TRUE, "net_system.c", 230);
        sNetSys->recvTemp = GFL_HeapAllocate(heapId, sNetSys->recvSize * 2, TRUE, "net_system.c", 232);
        sNetSys->recvData = GFL_HeapAllocate(heapId, machineMax * sNetSys->recvSize, TRUE, "net_system.c", 233);
        sNetSys->sendData = GFL_HeapAllocate(heapId, func_02042de8(), TRUE, "net_system.c", 236);
        sNetSys->sendRingData = GFL_HeapAllocate(heapId, 15 * func_02042de8(), TRUE, "net_system.c", 237);
        sNetSys->mpSendData = GFL_HeapAllocate(heapId, sendSize, TRUE, "net_system.c", 238);
        sNetSys->mpRingData = GFL_HeapAllocate(heapId, sendSize * 2, TRUE, "net_system.c", 239);
        func_0203e418(&sNetSys->sendQueue, 100, &sNetSys->sendRing, heapId);
        func_0203e418(&sNetSys->mpQueue, 180, &sNetSys->mpRing, heapId);
        sNetSys->unk1AC = TRUE;
        func_0203ea5c();
    }
    sNetSys->connectBits = 0;
    sNetSys->unk1E7 = 0;
    return TRUE;
}

static void func_0203ea5c(void) {
    int i;
    int machineMax = func_02042dc0();
    int sendSize = func_0203e8e8();
    int dataSize = func_02042de8();

    MI_CpuFill8(sNetSys->recvData, 0, sNetSys->recvSize * machineMax);
    for (i = 0; i < machineMax; i++) {
        func_0203dfc0(&sNetSys->recvRings[i], sNetSys->recvData + i * sNetSys->recvSize, sNetSys->recvSize);
        sNetSys->recvSeq[i] = 0;
    }
    MI_CpuFill8(sNetSys->mpRingData, 0, (u32)sendSize * 2);
    func_0203dfc0(&sNetSys->mpRing, sNetSys->mpRingData, sendSize * 2);
    for (i = 0; i < sendSize; i++) {
        sNetSys->mpSendData[i] = 0;
    }
    MI_CpuFill8(sNetSys->sendRingData, 0, (u32)dataSize * 15);
    func_0203dfc0(&sNetSys->sendRing, sNetSys->sendRingData, dataSize * 15);
    sNetSys->sendData[0] = 0xff;
    for (i = 1; i < dataSize; i++) {
        sNetSys->sendData[i] = 0;
    }
    MI_CpuFill8(sNetSys->serverRingData, 0, sNetSys->recvSize * 3);
    func_0203dfc0(&sNetSys->serverRing, sNetSys->serverRingData, sNetSys->recvSize * 3);
    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        func_0203e8d4(&sNetSys->recvStates[i]);
        sNetSys->sends[i] = 0;
    }
    sNetSys->serverSends = 0;
    func_0203e8d4(&sNetSys->serverRecvState);
    func_0203e8c8(4, 314);
    sState.recvState = 4;
    func_0203e44c(&sNetSys->sendQueue);
    func_0203e44c(&sNetSys->mpQueue);
    sNetSys->unk1E8 = 0;
    sNetSys->unk1EB = 1;
    sNetSys->unk1EA = 0;
    sNetSys->unk1ED = 0;
}

static void func_0203ebe0(int netID) {
    GFL_ASSERT(netID < GFL_NET_MACHINE_MAX);
    sNetSys->sends[netID] = 0;
    func_0203dfc0(&sNetSys->recvRings[netID], sNetSys->recvData + netID * sNetSys->recvSize, sNetSys->recvSize);
    sNetSys->recvSeq[netID] = 0;
    func_0203e8d4(&sNetSys->recvStates[netID]);
}

BOOL func_0203ec44(int size, HeapID heapId) {
    GFLNetInitData *ini = func_02042e84();
    GFLNetSys *sys = func_02042e78();

    func_0203e910(size, heapId, sys->aNetInit.bMPMode && sys->aNetInit.unk6C);
    sys->pDevTable->init(ini->heapId, sys, 0, sys->devWork);
    sys->pDevTable->unk08(0);
    sys->pDevTable->setRecvCallback(func_0203f7f0);
    return TRUE;
}

int func_0203ecac(int a0) {
    GFLNetSys *sys = func_02042e78();

    return sys->pDevTable->unk90(a0);
}

static BOOL func_0203ecc8(u16 netId, u8 *data, u16 size) {
    return func_0203f348(netId, data, size);
}

BOOL func_0203ecd0(int a0) {
    GFLNetInitData *ini = func_02042e84();
    GFLNetSys *sys = func_02042e78();

    ini->unk66 = a0;
    sys->pDevTable->setRecvCallback(func_0203ecc8);
    func_0203e910(0x80, ini->heapId, FALSE);
    func_0203ebe0(0);
    func_0203ebe0(1);
    func_0203ebe0(2);
    return TRUE;
}

static void func_0203ed18(int netId) {
    GFLNetInitData *ini = func_02042e84();
    GFLNetSys *sys = func_02042e78();

    func_0203ebe0(netId);
    if (ini->disconnectCallback != NULL) {
        ini->disconnectCallback(sys->devWork, netId);
    }
}

static void func_0203ed40(int netId) {
    GFLNetInitData *ini = func_02042e84();
    GFLNetSys *sys = func_02042e78();

    func_0203ebe0(netId);
    if (ini->disconnectCallback != NULL) {
        ini->disconnectCallback(sys->devWork, netId);
    }
}

int func_0203ed68(int a0, int size, int a2, BOOL (*callback)(void)) {
    GFLNetInitData *ini = func_02042e84();
    GFLNetSys *sys = func_02042e78();
    BOOL useSysSize;

    ini->unk66 = a0;
    sys->pDevTable->setDisconnectCallback(func_0203ed18);
    sys->pDevTable->setRecvCallback(func_0203f7f0);
    useSysSize = sys->aNetInit.bMPMode && sys->aNetInit.unk6C;
    func_0203e910(size, func_02042e84()->heapId, useSysSize);
    return sys->pDevTable->unk40(a2, callback);
}

BOOL func_0203ede0(int a0, int size) {
    GFLNetSys *sys = func_02042e78();
    BOOL result;

    sys->pDevTable->setDisconnectCallback(func_0203ed40);
    result = sys->pDevTable->unk44(a0, 0);
    if (result == TRUE) {
        BOOL useSysSize;

        sys->pDevTable->setRecvCallback(func_0203f8f8);
        useSysSize = sys->aNetInit.bMPMode && sys->aNetInit.unk6C;
        func_0203e910(size, func_02042e84()->heapId, useSysSize);
        sState.recvState = 4;
    }
    return result;
}

BOOL func_0203ee50(int size) {
    GFLNetSys *sys = func_02042e78();
    BOOL result;

    sys->pDevTable->setDisconnectCallback(func_0203ed40);
    result = sys->pDevTable->unkBC();
    if (result == TRUE) {
        sys->pDevTable->setRecvCallback(func_0203f8f8);
        func_0203e910(size, func_02042e84()->heapId, FALSE);
        sState.recvState = 4;
    }
    return result;
}

void func_0203eea4(int a0) {
    GFLNetSys *sys = func_02042e78();

    if (sys->pDevTable != NULL) {
        sys->pDevTable->unkC0(a0);
    }
}

int func_0203eec0(BOOL a0, const u8 *mac, int size, void (*callback)(void)) {
    GFLNetSys *sys = func_02042e78();
    GFLNetInitData *ini = func_02042e84();

    sys->pDevTable->setRecvCallback(func_0203f8f8);
    if (a0) {
        BOOL useSysSize = sys->aNetInit.bMPMode && sys->aNetInit.unk6C;

        func_0203e910(size, func_02042e84()->heapId, useSysSize);
        sState.recvState = 4;
    }
    return sys->pDevTable->unk48(a0, mac, 0, 0, callback);
}

void func_0203ef38(void) {
    GFLNetSys *sys = func_02042e78();
    GFLNetInitData *ini = func_02042e84();

    if (sNetSys != NULL) {
        GFL_HeapFree(sNetSys->serverRingData);
        GFL_HeapFree(sNetSys->recvTemp);
        GFL_HeapFree(sNetSys->recvData);
        func_0203e46c(&sNetSys->mpQueue);
        func_0203e46c(&sNetSys->sendQueue);
        GFL_HeapFree(sNetSys->sendData);
        GFL_HeapFree(sNetSys->sendRingData);
        GFL_HeapFree(sNetSys->mpSendData);
        GFL_HeapFree(sNetSys->mpRingData);
        GFL_HeapFree(sNetSys);
        sNetSys = NULL;
    }
}

static void func_0203efa0(void) {
    if (sNetSys->unk1EC) {
        if (func_0203ffc4() == 0) {
            if (func_0204001c() == 0) {
                func_0203ef38();
            }
        } else {
            func_0203ef38();
        }
    }
}

void func_0203efd0(u32 netId) {
    if (sNetSys != NULL && netId < GFL_NET_MACHINE_MAX) {
        func_0203ebe0(netId);
    }
}

// Sends and receives, each frame
BOOL func_0203efe8(void) {
    GFLNetSys *sys = func_02042e78();
    GFLNetInitData *ini = func_02042e84();

    if (sNetSys == NULL) {
        if (sys != NULL) {
            sys->pDevTable->update(0);
        }
        return TRUE;
    }
    if (sys->pDevTable->getSignalLevel != NULL) {
        func_0203e7f8(sys->pDevTable->getSignalLevel());
    }
    if (!sNetSys->unk1EA && !func_0204003c()) {
        if (!ini->bMPMode && (ini->bNetType == 0 || ini->bNetType == 5)) {
            func_0203f234();
            func_0203fe00();
        } else if (ini->bNetType == 3 || ini->bNetType == 4) {
            func_0203f2cc();
            func_0203fe00();
        } else if (ini->bNetType == 1 || ini->bNetType == 2) {
            func_0203f46c();
            func_0203fe00();
        } else {
            func_0203f4f0();
            func_0203fdbc();
            if (!func_0203ffc4() && func_0203fe74(0) && ini->bMPMode) {
                func_0203f6a8();
            }
            if (!func_0203ffc4()) {
                func_0203fe00();
            }
        }
        if (ini->bNetType != 1 && ini->bNetType != 2) {
            sys->pDevTable->update(sNetSys->connectBits);
        }
        func_020410dc();
        func_0203efa0();
    } else {
        sys->pDevTable->update(0);
    }
    return TRUE;
}

void func_0203f0f8(void) {
    if (sNetSys != NULL) {
        func_0203ea5c();
    }
}

// Writes the next packet to send
static BOOL func_0203f10c(BOOL server) {
    BOOL result = FALSE;

    if (sState.recvState != 4) {
        return result;
    }
    if (server) {
        NetSystem *net = sNetSys;

        if (net->serverSends > 0) {
            return result;
        }
        if (func_0203fa28(net->sendData, &net->sendQueue, func_02042de8(), net->sendSeq)) {
            sNetSys->sendSeq++;
        }
        sState.recvState = 0;
        result = TRUE;
    } else {
        NetSystem *net = sNetSys;

        if (func_0203fa28(net->sendData, &net->sendQueue, func_02042de8(), net->sendSeq)) {
            sNetSys->sendSeq++;
            sState.recvState = 0;
            result = TRUE;
        }
    }
    return result;
}

static void func_0203f19c(void) {
    GFLNetSys *sys = func_02042e78();
    int dataSize = func_02042de8();

    if (sys->pDevTable->isConnected() && func_0203fe74(func_0203ffc4()) && sState.recvState == 0) {
        if (func_0203ffc4() != 0) {
            if (sys->pDevTable->send(sNetSys->sendData, dataSize, 0, func_0203f9c0)) {
                sState.recvState = 2;
                sNetSys->serverSends++;
            }
        } else {
            sState.recvState = 2;
            func_0203f9c0(TRUE);
            func_0203f7f0(0, sNetSys->sendData, dataSize);
            sNetSys->serverSends++;
        }
    }
}

static void func_0203f234(void) {
    int i;
    GFLNetSys *sys = func_02042e78();

    if (sys->pDevTable->unk58()) {
        if (sNetSys->unk1EB) {
            func_0203f10c(TRUE);
        }
        if (sys->pDevTable->unk5C(sNetSys->sendData) == TRUE) {
            int dataSize;
            int machineMax;

            sState.recvState = 4;
            sNetSys->unk1EB = TRUE;
            dataSize = func_02042de8();
            machineMax = func_02042dc0();
            for (i = 0; i < machineMax; i++) {
                func_0203f7f0(i, sys->pDevTable->getRecvData(i), dataSize);
            }
        } else {
            sNetSys->unk1EB = FALSE;
        }
    }
}

static void func_0203f2cc(void) {
    GFLNetSys *sys = func_02042e78();

    if (func_0203e174(&sNetSys->sendQueue)) {
        func_02042be8(func_02040440(), 0x10, 0, NULL);
    }
    if (sys->pDevTable->unk9C != NULL) {
        sys->pDevTable->unk9C();
    }
    if (sys->pDevTable->unk6C() == TRUE) {
        int dataSize = func_02042de8();

        if (sys->pDevTable->unkA0() == TRUE && func_0203f10c(FALSE)) {
            func_0203f3f0(sNetSys->sendData, dataSize);
            sState.recvState = 4;
        }
    }
}

static BOOL func_0203f348(u16 netId, u8 *data, u16 size) {
    int crc;
    int len;

    func_02042e84();
    func_02042de8();
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
    func_0203dfd0(&sNetSys->recvRings[netId], data + PACKET_HEADER_SIZE, len);
    sNetSys->recvRings[netId].received = TRUE;
    return TRUE;
}

static void func_0203f3f0(u8 *data, int size) {
    GFLNetSys *sys;

    func_02042e84();
    sys = func_02042e78();
    if (!sys->pDevTable->unkA4() || !sys->pDevTable->unkAC()) {
        GFL_ASSERT(0);
        return;
    }
    if (sState.recvState == 0) {
        sys->pDevTable->send(data, size, 0, NULL);
        func_0203f348(sys->pDevTable->getNetId(), data, size);
        func_0203e8c8(4, 1036);
    }
}

static void func_0203f46c(void) {
    GFLNetSys *sys;
    int dataSize;
    NetSystem *net;

    func_02042e84();
    sys = func_02042e78();
    dataSize = func_02042de8();
    net = sNetSys;
    if (net->unk1E7) {
        if (!net->unk1ED) {
            if (func_0203fa28(net->sendData, &net->sendQueue, func_02042de8(), net->sendSeq)) {
                sNetSys->unk1ED = TRUE;
            }
        }
        if (sNetSys->unk1ED == TRUE) {
            if (sys->pDevTable->send(sNetSys->sendData, dataSize, 0, NULL)) {
                sNetSys->unk1ED = FALSE;
            }
        }
    }
}

static void func_0203f4f0(void) {
    GFLNetSys *sys;

    func_02042e84();
    sys = func_02042e78();
    if (sys->pDevTable->isConnected() && func_0203fe74(func_0203ffc4())) {
        func_0203f10c(TRUE);
        func_0203f19c();
    }
}

static void func_0203f520(void) {
    GFLNetSys *sys = func_02042e78();

    func_02042dc0();
    if (sState.sendState == 1 && func_0203ff6c() > 1) {
        func_0203e8c8(2, 1130);
        if (sys->pDevTable->isConnected()) {
            if (!sys->pDevTable->send(sNetSys->mpSendData, func_0203e8e8(), 0, func_0203f9f0)) {
                func_0203e8c8(1, 1134);
            }
        } else if (func_02042e84()->bNetType == 3 || func_02042e84()->bNetType == 4) {
            if (!sys->pDevTable->unkA0()) {
                func_0203e8c8(1, 1141);
            } else {
                if (sNetSys->mpSendData[PACKET_SIZE] == 0) {
                    return;
                }
                sys->pDevTable->send(sNetSys->mpSendData, func_02042de8(), 100, NULL);
            }
        }
    }
    if (sState.sendState == 2) {
        func_0203f670();
        func_0203f8f8(0, sNetSys->mpSendData, func_0203e8e8());
        func_0203e8c8(3, 1165);
    }
    if (!sys->pDevTable->isConnected() || func_02042e84()->bNetType == 3 || func_02042e84()->bNetType == 4) {
        func_0203e8c8(4, 1170);
    }
}

// Whether no machine has packets that have not come back
static BOOL func_0203f63c(void) {
    int i;

    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        if (func_0203fe74(i) && sNetSys->sends[i] > 0) {
            return FALSE;
        }
    }
    return TRUE;
}

static void func_0203f670(void) {
    int i;
    int machineMax = func_02042dc0();

    for (i = 0; i < machineMax; i++) {
        if (func_0203fe74(i)) {
            sNetSys->sends[i]++;
        }
    }
}

static void func_0203f6a8(void) {
    GFLNetSys *sys = func_02042e78();
    GFLNetInitData *pNetIni = func_02042e84();

    if (pNetIni->bNetType == 1 || pNetIni->bNetType == 2) {
        if (!func_0203fe74(0)) {
            return;
        }
        GFL_ASSERT(!pNetIni->bMPMode);
        if (sState.sendState == 4 && sNetSys->unk1AC && func_0203f63c()) {
            func_0203e8c8(1, 1242);
        }
        if (sState.sendState == 1) {
            if (sys->pDevTable->send(sNetSys->mpSendData, func_0203e8e8(), 0, NULL)) {
                func_0203e8c8(4, 1249);
                func_0203f670();
            }
        }
    } else if (pNetIni->bNetType == 3 || pNetIni->bNetType == 4) {
        if (sState.sendState == 4) {
            NetSystem *net = sNetSys;

            func_0203fa28(net->mpSendData, &net->mpQueue, func_0203e8e8(), net->mpSeq);
            sNetSys->mpSeq++;
            func_0203e8c8(1, 1268);
        }
        func_0203f520();
    } else if (sys->pDevTable->isConnected() && func_0203f63c()) {
        if (sState.sendState == 4) {
            NetSystem *net = sNetSys;

            func_0203fa28(net->mpSendData, &net->mpQueue, func_0203e8e8(), net->mpSeq);
            sNetSys->mpSeq++;
            func_0203e8c8(1, 1281);
        }
        func_0203f520();
    }
}

static BOOL func_0203f7f0(u16 netId, u8 *data, u16 size) {
    int crc;
    int len;

    func_02042de8();
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
    func_0203dfd0(&sNetSys->recvRings[netId], data + PACKET_HEADER_SIZE, len);
    sNetSys->recvRings[netId].received = TRUE;
    return TRUE;
}

static BOOL func_0203f8f8(u16 netId, u8 *data, u16 size) {
    int crc;
    int len;

    func_02042e84();
    func_02042de8();
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
    func_0203dfd0(&sNetSys->serverRing, data + PACKET_HEADER_SIZE, len);
    sNetSys->serverRing.received = TRUE;
    return TRUE;
}

static BOOL func_0203f9c0(BOOL ok) {
    if (ok && sState.recvState == 2) {
        sState.recvState = 4;
    } else {
        GFL_ASSERT(0);
    }
    return TRUE;
}

static BOOL func_0203f9f0(BOOL ok) {
    func_02042de8();
    func_02042dc0();
    func_02042e84();
    if (ok) {
        func_0203e8c8(4, 1492);
    } else {
        GFL_ASSERT(0);
    }
    return TRUE;
}

static BOOL func_0203fa28(u8 *packet, NetQueue *queue, int size, u8 seq) {
    GFLNetInitData *ini = func_02042e84();
    GFLNetSys *sys = func_02042e78();
    NetSendBuffer buffer;
    int len;
    int crc;

    MI_CpuFill8(packet, 0, size);
    packet[PACKET_FLAGS] = queue->pending == FALSE ? PACKET_FLAG_FIRST : PACKET_FLAG_CONTINUED;
    queue->pending = FALSE;
    packet[PACKET_CONNECT_BITS] = sys->pDevTable->getConnectBits();
    if (func_0203e174(queue) || ((ini->bNetType == 3 || ini->bNetType == 4) && sys->pDevTable->unkB0() == TRUE)) {
        packet[PACKET_FLAGS] = PACKET_FLAGS_NONE;
        return FALSE;
    }
    buffer.size = size - PACKET_HEADER_SIZE;
    buffer.ptr = packet + PACKET_HEADER_SIZE;
    if (!func_0203e3bc(queue, &buffer)) {
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
BOOL func_0203fafc(int command, u8 *data, int size, u32 unused, int dest, int netId, BOOL noCopy) {
    BOOL copy = TRUE;
    GFLNetInitData *ini;
    NetQueue *queue;

    if (noCopy) {
        copy = FALSE;
    }
    ini = func_02042e84();
    if (sNetSys == NULL) {
        return FALSE;
    }
    if (func_02040078() || func_02042494()) {
        return FALSE;
    }
    if (!func_02040198()) {
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
    if (func_0203e2c0(queue, command, data, size, unused, copy, netId)) {
        return TRUE;
    }
    NET_ERROR(1012);
    return FALSE;
}

int func_0203fbb8(void) {
    return func_0203e0f4(&sNetSys->sendRing);
}

int func_0203fbcc(void) {
    return func_0203e0f4(&sNetSys->mpRing);
}

// Hands a command that has arrived to its handler, and for the server to the server's handler too
static void func_0203fbe0(int command, int size, void *data, NetRecvState *state) {
    NetHandle *handle;

    func_02042e84();
    handle = func_02040414(func_0203ffc4());
    func_02040d78(state->netId, state->sender, command, size, data, handle);
    if (func_0203ffc4() == 0) {
        handle = func_02040414(GFL_NET_NETID_SERVER);
        if (command < 0x11) {
            func_02040d78(state->netId, state->sender, command, size, data, handle);
        }
    }
    func_0203e8d4(state);
}

// Reads the commands that have arrived in a ring buffer
static void func_0203fc34(NetRingBuff *ring, int netId, u8 *temp, NetRecvState *state, int size) {
    u8 *dest;
    int start;
    int read;
    int copied;
    int command;
    int savedStart;

    func_02042e84();
    if (func_0203e0f4(ring) == 0) {
        return;
    }
    do {
        command = state->command;
        if (command == 0) {
            command = func_0203e080(ring);
            if (command == 0) {
                GFL_ASSERT(0);
                continue;
            }
        }
        savedStart = ring->startPos;
        state->command = command;
        if (state->size == 0xffff) {
            if (func_0203e0f4(ring) < 3) {
                ring->startPos = savedStart;
                return;
            }
            u16 commandSize = func_0203e080(ring);

            state->remaining = commandSize;
            state->size = commandSize;
            state->netId = netId;
            state->sender = func_0203e054(ring);
        }
        start = ring->startPos;
        if (func_02040dd4(command) && func_02040c94(state->sender)) {
            if (state->buffer == NULL) {
                state->buffer = func_02040de8(command, state->netId, state->size);
            }
            dest = state->buffer + state->pos;
        } else {
            state->buffer = temp;
            dest = temp;
        }
        if (!func_02040c94(state->sender)) {
            dest = NULL;
        }
        if (!func_02040dd4(command)) {
            read = func_0203e038(ring, temp, state->remaining, state->remaining);
        } else {
            read = func_0203e038(ring, temp, state->remaining, size - PACKET_HEADER_SIZE);
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
            func_0203fbe0(command, state->size, state->buffer, state);
            continue;
        }
        if (func_02040dc0(command) && !func_02040dd4(command) && func_02040c94(state->sender)) {
            state->remaining += (u16)read;
            ring->startPos = start;
        }
        return;
    } while (func_0203e0f4(ring) != 0);
}

static void func_0203fdbc(void) {
    int i;

    if (sNetSys != NULL) {
        func_0203e150(&sNetSys->serverRing);
        for (i = 0; i < 4; i++) {
            NetSystem *net;

            if (func_0203e0f4(&sNetSys->serverRing) <= 0) {
                break;
            }
            net = sNetSys;
            func_0203fc34(&net->serverRing, GFL_NET_NETID_SERVER, net->recvTemp, &net->serverRecvState,
                          func_0203e8e8());
        }
    }
}

static void func_0203fe00(void) {
    int machineMax;
    int dataSize = func_02042de8();
    int i;
    int j;

    if (sNetSys != NULL) {
        machineMax = func_02042dc0();

        for (i = 0; i < machineMax; i++) {
            func_0203e150(&sNetSys->recvRings[i]);
            for (j = 0; j < 4; j++) {
                if (func_0203e0f4(&sNetSys->recvRings[i]) > 0) {
                    NetSystem *net = sNetSys;

                    func_0203fc34(&net->recvRings[i], i, net->recvTemp, &net->recvStates[i], dataSize);
                }
            }
        }
    }
}

BOOL func_0203fe74(u16 netId) {
    GFLNetSys *sys = func_02042e78();
    GFLNetInitData *ini = func_02042e84();

    if (sNetSys == NULL) {
        return FALSE;
    }
    if (ini->bNetType == 1 || ini->bNetType == 2) {
        if (sNetSys->unk1E7 && sys->pDevTable->getNetId() != -1) {
            if (netId == 0) {
                return TRUE;
            }
            if (netId == 1) {
                return TRUE;
            }
        }
        return FALSE;
    }
    if (ini->bNetType == 3 || ini->bNetType == 4) {
        if ((netId < 2 || netId == GFL_NET_NETID_SERVER) && sys->pDevTable->isConnected()) {
            return TRUE;
        }
        return FALSE;
    }
    if (!func_0203ff8c()) {
        return FALSE;
    }
    if (!sys->pDevTable->isConnected()) {
        return FALSE;
    }
    if (netId == func_0203ffc4()) {
        return TRUE;
    }
    if (func_0203ffc4() == 0 || !ini->bMPMode) {
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

int func_0203ff6c(void) {
    int count = 0;
    int i;

    for (i = 0; i < GFL_NET_MACHINE_MAX; i++) {
        if (func_0203fe74(i)) {
            count++;
        }
    }
    return count;
}

BOOL func_0203ff8c(void) {
    GFLNetSys *sys = func_02042e78();
    GFLNetInitData *ini = func_02042e84();

    if (sNetSys != NULL && (ini->bNetType == 1 || ini->bNetType == 2 || ini->bNetType == 3 || ini->bNetType == 4)) {
        return TRUE;
    }
    return sys->pDevTable->unk6C();
}

u16 func_0203ffc4(void) {
    if (sNetSys != NULL) {
        GFLNetSys *sys = func_02042e78();
        GFLNetInitData *ini = func_02042e84();

        if (ini->bNetType == 1 || ini->bNetType == 2) {
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

u32 func_0204001c(void) {
    GFLNetSys *sys = func_02042e78();

    return (u16)sys->pDevTable->getConnectBits() & 0xfffe;
}

BOOL func_0204003c(void) {
    GFLNetSys *sys = func_02042e78();

    if (sNetSys != NULL && sNetSys->error) {
        return TRUE;
    }
    if (sys->pDevTable->isError()) {
        return TRUE;
    }
    return FALSE;
}

BOOL func_02040078(void) {
    GFLNetSys *sys = func_02042e78();
    BOOL error = FALSE;

    if (sys != NULL && sys->pDevTable != NULL) {
        error = sys->pDevTable->isError();
    }
    if (!error && sNetSys != NULL && sNetSys->error) {
        error = TRUE;
    }
    return error;
}

BOOL func_020400b8(u16 command, int dest) {
    if (func_02042e84()->bMPMode && dest == GFL_NET_NETID_SERVER) {
        return func_0203e478(&sNetSys->mpQueue, command);
    }
    return func_0203e478(&sNetSys->sendQueue, command);
}

BOOL func_020400f0(void) {
    return func_0203e174(&sNetSys->sendQueue);
}

void func_02040104(u8 a0) {
    sNetSys->unk1E7 = a0;
}

void func_02040118(BOOL a0) {
    GFLNetInitData *ini = func_02042e84();

    if (ini->bNetType == 1 || ini->bNetType == 2) {
        sNetSys->unk1AC = a0;
        if (a0) {
            sNetSys->serverSends = 0;
            sNetSys->sends[0] = 0;
            sNetSys->sends[1] = 0;
        }
    }
}

void func_02040158(void) {
    sNetSys->error = TRUE;
}

void func_0204016c(void) {
    sNetSys->error = FALSE;
}

void func_02040180(void) {
    if (sNetSys != NULL) {
        sNetSys->unk1EA = TRUE;
    }
}

BOOL func_02040198(void) {
    if (sNetSys != NULL) {
        GFLNetInitData *ini = func_02042e84();

        if (!ini->bMPMode && (ini->bNetType == 0 || ini->bNetType == 5)) {
            return sNetSys->unk1EB;
        }
    }
    return TRUE;
}

int func_020401d4(int netId) {
    if (netId >= GFL_NET_MACHINE_MAX) {
        netId = GFL_NET_NETID_SERVER;
    }
    return netId;
}
