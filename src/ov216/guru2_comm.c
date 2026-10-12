#include "types.h"
#include "app/guru2.h"
#include "field/union_app.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_system.h"
#include "gfl/std.h"
#include "nitro/math.h"

// guru2_comm.c, which the ROM names: the Spin Trade's net commands. Names are ours

static UnionApp *Guru2Comm_GetUnionApp(Guru2CommWork *g2c);
static BOOL Guru2Comm_SendData(Guru2CommWork *g2c, u32 command, const void *data, u32 size);
static void Guru2Comm_RecvBlock(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_RecvStart(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_RecvSignal(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_RecvCancel(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_Recv3(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_RecvJoin(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_Recv5(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_Recv6(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_Recv9(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_Recv10(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_Recv7(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_Recv8(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_Recv11(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_Recv12(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_Recv13(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Guru2Comm_Recv14(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *Guru2Comm_GetBlockRecvBuffer(int netId, void *work, int size);

// The commands from GURU2_NET_CMD_BASE
static const NetCommand sGuru2Commands[GURU2_NET_CMD_COUNT] = {
    { Guru2Comm_RecvJoin, NULL },
    { Guru2Comm_RecvSignal, NULL },
    { Guru2Comm_RecvCancel, NULL },
    { Guru2Comm_Recv3, NULL },
    { Guru2Comm_RecvStart, NULL },
    { Guru2Comm_Recv5, NULL },
    { Guru2Comm_Recv6, NULL },
    { Guru2Comm_Recv7, NULL },
    { Guru2Comm_Recv8, NULL },
    { Guru2Comm_Recv9, NULL },
    { Guru2Comm_Recv10, NULL },
    { Guru2Comm_Recv11, NULL },
    { Guru2Comm_Recv12, NULL },
    { Guru2Comm_Recv13, NULL },
    { Guru2Comm_Recv14, NULL },
    { Guru2Comm_RecvBlock, Guru2Comm_GetBlockRecvBuffer },
};

static UnionApp *Guru2Comm_GetUnionApp(Guru2CommWork *g2c) {
    return g2c->g2p->param->app;
}

Guru2CommWork *Guru2Comm_Create(Guru2ProcWork *g2p, u32 heapId) {
    Guru2CommWork *g2c;

    g2c = GFL_HeapAllocate(heapId, sizeof(Guru2CommWork), FALSE, "guru2_comm.c", 0x98);
    sys_memset(g2c, 0, sizeof(Guru2CommWork));
    g2c->g2p = g2p;
    g2c->blockRecvBuffer =
        GFL_HeapAllocate(heapId, GURU2_COMM_BLOCK_SIZE * GURU2_MEMBER_MAX, FALSE, "guru2_comm.c", 0x9f);
    g2c->blocks = GFL_HeapAllocate(heapId, GURU2_COMM_BLOCK_SIZE * GURU2_MEMBER_MAX, FALSE, "guru2_comm.c", 0xa1);
    return g2c;
}

void Guru2Comm_Free(Guru2CommWork *g2c) {
    GFL_HeapFree(g2c->blockRecvBuffer);
    GFL_HeapFree(g2c->blocks);
    GFL_HeapFree(g2c);
}

void Guru2Comm_AddCommands(Guru2CommWork *g2c) {
    func_02040c20(GURU2_NET_CMD_BASE, sGuru2Commands, GURU2_NET_CMD_COUNT, g2c);
}

void Guru2Comm_End(Guru2CommWork *g2c) {
}

static BOOL Guru2Comm_SendData(Guru2CommWork *g2c, u32 command, const void *data, u32 size) {
    NetHandle *handle = func_02040440();
    Guru2CommSendBuffer *send = &g2c->send;

    sys_memcpy(data, send->data, size);
    return func_02042be8(handle, (u16)command, size + 4, send->data);
}

BOOL Guru2Comm_Send(Guru2CommWork *g2c, u32 command, const void *data, u32 size) {
    return Guru2Comm_SendData(g2c, command, data, size);
}

static void Guru2Comm_RecvBlock(int netId, int size, const void *data, void *work, NetHandle *handle) {
    Guru2CommWork *g2c = work;

    g2c->blockBits |= 1 << netId;
    sys_memcpy(data, Guru2Comm_GetBlock(g2c, netId), GURU2_COMM_BLOCK_SIZE);
}

static void Guru2Comm_RecvStart(int netId, int size, const void *data, void *work, NetHandle *handle) {
    Guru2CommWork *g2c = work;

    if (func_0203ffc4() != 0) {
        g2c->startCount = 0;
        g2c->started = TRUE;
        Guru2Receipt_SetSeq(g2c->g2p->g2r, GURU2_RECEIPT_SEQ_START, netId);
    }
}

static void Guru2Comm_RecvSignal(int netId, int size, const void *data, void *work, NetHandle *handle) {
    const Guru2CommSignal *signal = data;
    Guru2CommWork *g2c = work;

    if (netId != 0) {
        // A member's signal reaches the parent, which answers it
        if (func_0203ffc4() == 0) {
            Guru2CommSignal answer = *signal;

            answer.netId = netId;
            answer.count = g2c->joinCount;
            switch (signal->type) {
            case GURU2_SIGNAL_LEAVE_REQUEST:
                if (g2c->joinCount != UnionApp_CountJoined(Guru2Comm_GetUnionApp(g2c))
                    || g2c->joinCount != countOneBits(UnionApp_GetJoined(Guru2Comm_GetUnionApp(g2c)))) {
                    answer.ok = FALSE;
                } else {
                    g2c->leavingBits |= 1 << netId;
                    answer.ok = TRUE;
                }
                break;
            case GURU2_SIGNAL_LEAVE:
                break;
            }
            Guru2Comm_Send(g2c, GURU2_NET_CMD_SIGNAL, &answer, sizeof(answer));
        }
    } else {
        switch (signal->type) {
        case GURU2_SIGNAL_LEAVE_REQUEST: {
            u8 id = signal->netId;

            if (id == func_0203ffc4()) {
                if (!signal->ok) {
                    Guru2Receipt_SetSeq(g2c->g2p->g2r, GURU2_RECEIPT_SEQ_CANCEL_REFUSED, id);
                } else {
                    g2c->signalCount = signal->count;
                    Guru2Receipt_SetSeq(g2c->g2p->g2r, GURU2_RECEIPT_SEQ_CANCEL_OK, id);
                }
            }
            break;
        }
        case GURU2_SIGNAL_LEAVE:
            Guru2Receipt_SetSeq(g2c->g2p->g2r, GURU2_RECEIPT_SEQ_MEMBER_LEFT, signal->netId);
            break;
        }
    }
}

static void Guru2Comm_RecvCancel(int netId, int size, const void *data, void *work, NetHandle *handle) {
    Guru2CommWork *g2c = work;

    if (func_0203ffc4() != 0) {
        Guru2Receipt_SetSeq(g2c->g2p->g2r, GURU2_RECEIPT_SEQ_PARENT_CANCEL_MSG, 0);
    }
}

static void Guru2Comm_Recv3(int netId, int size, const void *data, void *work, NetHandle *handle) {
    func_0203ffc4();
}

static void Guru2Comm_RecvJoin(int netId, int size, const void *data, void *work, NetHandle *handle) {
    Guru2CommWork *g2c = work;
    const u8 *id = data;

    Guru2Receipt_SetSeqOnJoin(g2c->g2p->g2r, GURU2_RECEIPT_SEQ_1, *id);
    if (func_0203ffc4() == 0) {
        g2c->unk34 = 0;
    }
}

static void Guru2Comm_Recv5(int netId, int size, const void *data, void *work, NetHandle *handle) {
    Guru2CommWork *g2c = work;
    const u16 *bits = data;

    g2c->unk40 |= *bits;
}

static void Guru2Comm_Recv6(int netId, int size, const void *data, void *work, NetHandle *handle) {
    Guru2CommWork *g2c = work;

    g2c->unk42 |= 1 << netId;
}

static void Guru2Comm_Recv9(int netId, int size, const void *data, void *work, NetHandle *handle) {
    Guru2CommWork *g2c = work;

    Guru2Main_OnPosition(g2c->g2p->g2m, data);
}

static void Guru2Comm_Recv10(int netId, int size, const void *data, void *work, NetHandle *handle) {
    Guru2CommWork *g2c = work;
    const u32 *value = data;

    Guru2Main_OnPlayerCount(g2c->g2p->g2m, *value);
}

static void Guru2Comm_Recv7(int netId, int size, const void *data, void *work, NetHandle *handle) {
    Guru2CommWork *g2c = work;
    const u8 *value = data;

    Guru2Main_OnButton(g2c->g2p->g2m, *value);
}

static void Guru2Comm_Recv8(int netId, int size, const void *data, void *work, NetHandle *handle) {
    Guru2CommWork *g2c = work;

    if (netId == 0) {
        Guru2Main_OnGameData(g2c->g2p->g2m, data);
    }
}

static void Guru2Comm_Recv11(int netId, int size, const void *data, void *work, NetHandle *handle) {
    Guru2CommWork *g2c = work;
    const u32 *value = data;

    Guru2Main_OnPartyIndex(g2c->g2p->g2m, netId, *value);
}

static void Guru2Comm_Recv12(int netId, int size, const void *data, void *work, NetHandle *handle) {
    Guru2CommWork *g2c = work;

    Guru2Main_OnResult(g2c->g2p->g2m, data);
}

static void Guru2Comm_Recv13(int netId, int size, const void *data, void *work, NetHandle *handle) {
    Guru2CommWork *g2c = work;

    g2c->unk48 |= 1 << netId;
}

static void Guru2Comm_Recv14(int netId, int size, const void *data, void *work, NetHandle *handle) {
    Guru2CommWork *g2c = work;

    g2c->unk4A |= 1 << netId;
}

static void *Guru2Comm_GetBlockRecvBuffer(int netId, void *work, int size) {
    Guru2CommWork *g2c = work;

    return g2c->blockRecvBuffer + netId * GURU2_COMM_BLOCK_SIZE;
}

void *Guru2Comm_GetBlock(Guru2CommWork *g2c, int netId) {
    return g2c->blocks + netId * GURU2_COMM_BLOCK_SIZE;
}
