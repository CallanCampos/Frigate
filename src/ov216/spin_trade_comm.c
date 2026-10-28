// The ROM names this file guru2_comm.c.
#include "types.h"
#include "app/spin_trade.h"
#include "field/union_app.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_system.h"
#include "gfl/std.h"
#include "nitro/math.h"

// spin_trade_comm.c, which the ROM names: the Spin Trade's net commands. Names are ours

static UnionApp *SpinTradeComm_GetUnionApp(SpinTradeCommWork *g2c);
static BOOL SpinTradeComm_SendData(SpinTradeCommWork *g2c, u32 command, const void *data, u32 size);
static void SpinTradeComm_RecvBlock(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_RecvStart(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_RecvSignal(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_RecvCancel(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_Recv3(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_RecvJoin(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_Recv5(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_Recv6(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_Recv9(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_Recv10(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_Recv7(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_Recv8(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_Recv11(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_Recv12(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_Recv13(int netId, int size, const void *data, void *work, NetHandle *handle);
static void SpinTradeComm_Recv14(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *SpinTradeComm_GetBlockRecvBuffer(int netId, void *work, int size);

// The commands from SPIN_TRADE_NET_CMD_BASE
// clang-format off
static const NetCommand sSpinTradeCommands[SPIN_TRADE_NET_CMD_COUNT] = {
    { SpinTradeComm_RecvJoin, NULL },
    { SpinTradeComm_RecvSignal, NULL },
    { SpinTradeComm_RecvCancel, NULL },
    { SpinTradeComm_Recv3, NULL },
    { SpinTradeComm_RecvStart, NULL },
    { SpinTradeComm_Recv5, NULL },
    { SpinTradeComm_Recv6, NULL },
    { SpinTradeComm_Recv7, NULL },
    { SpinTradeComm_Recv8, NULL },
    { SpinTradeComm_Recv9, NULL },
    { SpinTradeComm_Recv10, NULL },
    { SpinTradeComm_Recv11, NULL },
    { SpinTradeComm_Recv12, NULL },
    { SpinTradeComm_Recv13, NULL },
    { SpinTradeComm_Recv14, NULL },
    { SpinTradeComm_RecvBlock, SpinTradeComm_GetBlockRecvBuffer },
};

// clang-format on

static UnionApp *SpinTradeComm_GetUnionApp(SpinTradeCommWork *g2c) {
    return g2c->g2p->param->app;
}

SpinTradeCommWork *SpinTradeComm_Create(SpinTradeProcWork *g2p, u32 heapId) {
    SpinTradeCommWork *g2c;

    g2c = GFL_HeapAllocate(heapId, sizeof(SpinTradeCommWork), FALSE, "guru2_comm.c", 0x98);
    MI_CpuFill8(g2c, 0, sizeof(SpinTradeCommWork));
    g2c->g2p = g2p;
    g2c->blockRecvBuffer =
        GFL_HeapAllocate(heapId, SPIN_TRADE_COMM_BLOCK_SIZE * SPIN_TRADE_MEMBER_MAX, FALSE, "guru2_comm.c", 0x9f);
    g2c->blocks =
        GFL_HeapAllocate(heapId, SPIN_TRADE_COMM_BLOCK_SIZE * SPIN_TRADE_MEMBER_MAX, FALSE, "guru2_comm.c", 0xa1);
    return g2c;
}

void SpinTradeComm_Free(SpinTradeCommWork *g2c) {
    GFL_HeapFree(g2c->blockRecvBuffer);
    GFL_HeapFree(g2c->blocks);
    GFL_HeapFree(g2c);
}

void SpinTradeComm_AddCommands(SpinTradeCommWork *g2c) {
    NetCommand_AddTable(SPIN_TRADE_NET_CMD_BASE, sSpinTradeCommands, SPIN_TRADE_NET_CMD_COUNT, g2c);
}

void SpinTradeComm_End(SpinTradeCommWork *g2c) {
}

static BOOL SpinTradeComm_SendData(SpinTradeCommWork *g2c, u32 command, const void *data, u32 size) {
    NetHandle *handle = NetHandle_GetOwn();
    SpinTradeCommSendBuffer *send = &g2c->send;

    MI_CpuCopy8(data, send->data, size);
    return GFL_NetSendCommand(handle, (u16)command, size + 4, send->data);
}

BOOL SpinTradeComm_Send(SpinTradeCommWork *g2c, u32 command, const void *data, u32 size) {
    return SpinTradeComm_SendData(g2c, command, data, size);
}

static void SpinTradeComm_RecvBlock(int netId, int size, const void *data, void *work, NetHandle *handle) {
    SpinTradeCommWork *g2c = work;

    g2c->blockBits |= 1 << netId;
    MI_CpuCopy8(data, SpinTradeComm_GetBlock(g2c, netId), SPIN_TRADE_COMM_BLOCK_SIZE);
}

static void SpinTradeComm_RecvStart(int netId, int size, const void *data, void *work, NetHandle *handle) {
    SpinTradeCommWork *g2c = work;

    if (NetSystem_GetNetId() != 0) {
        g2c->startCount = 0;
        g2c->started = TRUE;
        SpinTradeReceipt_SetSeq(g2c->g2p->g2r, SPIN_TRADE_RECEIPT_SEQ_START, netId);
    }
}

static void SpinTradeComm_RecvSignal(int netId, int size, const void *data, void *work, NetHandle *handle) {
    const SpinTradeCommSignal *signal = data;
    SpinTradeCommWork *g2c = work;

    if (netId != 0) {
        // A member's signal reaches the parent, which answers it
        if (NetSystem_GetNetId() == 0) {
            SpinTradeCommSignal answer = *signal;

            answer.netId = netId;
            answer.count = g2c->joinCount;
            switch (signal->type) {
            case SPIN_TRADE_SIGNAL_LEAVE_REQUEST:
                if (g2c->joinCount != UnionApp_CountJoined(SpinTradeComm_GetUnionApp(g2c)) ||
                    g2c->joinCount != MATH_CountPopulation(UnionApp_GetJoined(SpinTradeComm_GetUnionApp(g2c)))) {
                    answer.ok = FALSE;
                } else {
                    g2c->leavingBits |= 1 << netId;
                    answer.ok = TRUE;
                }
                break;
            case SPIN_TRADE_SIGNAL_LEAVE:
                break;
            }
            SpinTradeComm_Send(g2c, SPIN_TRADE_NET_CMD_SIGNAL, &answer, sizeof(answer));
        }
    } else {
        switch (signal->type) {
        case SPIN_TRADE_SIGNAL_LEAVE_REQUEST: {
            u8 id = signal->netId;

            if (id == NetSystem_GetNetId()) {
                if (!signal->ok) {
                    SpinTradeReceipt_SetSeq(g2c->g2p->g2r, SPIN_TRADE_RECEIPT_SEQ_CANCEL_REFUSED, id);
                } else {
                    g2c->signalCount = signal->count;
                    SpinTradeReceipt_SetSeq(g2c->g2p->g2r, SPIN_TRADE_RECEIPT_SEQ_CANCEL_OK, id);
                }
            }
            break;
        }
        case SPIN_TRADE_SIGNAL_LEAVE:
            SpinTradeReceipt_SetSeq(g2c->g2p->g2r, SPIN_TRADE_RECEIPT_SEQ_MEMBER_LEFT, signal->netId);
            break;
        }
    }
}

static void SpinTradeComm_RecvCancel(int netId, int size, const void *data, void *work, NetHandle *handle) {
    SpinTradeCommWork *g2c = work;

    if (NetSystem_GetNetId() != 0) {
        SpinTradeReceipt_SetSeq(g2c->g2p->g2r, SPIN_TRADE_RECEIPT_SEQ_PARENT_CANCEL_MSG, 0);
    }
}

static void SpinTradeComm_Recv3(int netId, int size, const void *data, void *work, NetHandle *handle) {
    NetSystem_GetNetId();
}

static void SpinTradeComm_RecvJoin(int netId, int size, const void *data, void *work, NetHandle *handle) {
    SpinTradeCommWork *g2c = work;
    const u8 *id = data;

    SpinTradeReceipt_SetSeqOnJoin(g2c->g2p->g2r, SPIN_TRADE_RECEIPT_SEQ_1, *id);
    if (NetSystem_GetNetId() == 0) {
        g2c->unk34 = 0;
    }
}

static void SpinTradeComm_Recv5(int netId, int size, const void *data, void *work, NetHandle *handle) {
    SpinTradeCommWork *g2c = work;
    const u16 *bits = data;

    g2c->unk40 |= *bits;
}

static void SpinTradeComm_Recv6(int netId, int size, const void *data, void *work, NetHandle *handle) {
    SpinTradeCommWork *g2c = work;

    g2c->unk42 |= 1 << netId;
}

static void SpinTradeComm_Recv9(int netId, int size, const void *data, void *work, NetHandle *handle) {
    SpinTradeCommWork *g2c = work;

    SpinTradeMain_OnPosition(g2c->g2p->g2m, data);
}

static void SpinTradeComm_Recv10(int netId, int size, const void *data, void *work, NetHandle *handle) {
    SpinTradeCommWork *g2c = work;
    const u32 *value = data;

    SpinTradeMain_OnPlayerCount(g2c->g2p->g2m, *value);
}

static void SpinTradeComm_Recv7(int netId, int size, const void *data, void *work, NetHandle *handle) {
    SpinTradeCommWork *g2c = work;
    const u8 *value = data;

    SpinTradeMain_OnButton(g2c->g2p->g2m, *value);
}

static void SpinTradeComm_Recv8(int netId, int size, const void *data, void *work, NetHandle *handle) {
    SpinTradeCommWork *g2c = work;

    if (netId == 0) {
        SpinTradeMain_OnGameData(g2c->g2p->g2m, data);
    }
}

static void SpinTradeComm_Recv11(int netId, int size, const void *data, void *work, NetHandle *handle) {
    SpinTradeCommWork *g2c = work;
    const u32 *value = data;

    SpinTradeMain_OnPartyIndex(g2c->g2p->g2m, netId, *value);
}

static void SpinTradeComm_Recv12(int netId, int size, const void *data, void *work, NetHandle *handle) {
    SpinTradeCommWork *g2c = work;

    SpinTradeMain_OnResult(g2c->g2p->g2m, data);
}

static void SpinTradeComm_Recv13(int netId, int size, const void *data, void *work, NetHandle *handle) {
    SpinTradeCommWork *g2c = work;

    g2c->unk48 |= 1 << netId;
}

static void SpinTradeComm_Recv14(int netId, int size, const void *data, void *work, NetHandle *handle) {
    SpinTradeCommWork *g2c = work;

    g2c->unk4A |= 1 << netId;
}

static void *SpinTradeComm_GetBlockRecvBuffer(int netId, void *work, int size) {
    SpinTradeCommWork *g2c = work;

    return g2c->blockRecvBuffer + netId * SPIN_TRADE_COMM_BLOCK_SIZE;
}

void *SpinTradeComm_GetBlock(SpinTradeCommWork *g2c, int netId) {
    return g2c->blocks + netId * SPIN_TRADE_COMM_BLOCK_SIZE;
}
