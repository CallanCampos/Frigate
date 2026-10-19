#include "types.h"
#include "field/delivery_irc.h"
#include "field/net_devirc.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/random.h"
#include "gfl/std.h"

#define DELIVERY_IRC_BUFFER_SIZE 0x5e8

// What the sender sends before the data
typedef struct {
    u32 seed;
    u16 crc;
} DeliveryIrcHeader;

// What the receiver wants
typedef struct {
    u32 region;
    u32 mask;
} DeliveryIrcWanted;

typedef struct DeliveryIrcWork DeliveryIrcWork;
typedef void (*DeliveryIrcSeq)(void *work);

struct DeliveryIrcWork {
    DeliveryIrcInit aInit;
    u8 buffer[DELIVERY_IRC_BUFFER_SIZE];
    // The data being sent
    DeliveryData *sendData;
    u32 seed;
    // 1 when the sender has no data for the receiver
    u32 noData;
    u16 negotiated;
    u16 crc;
    u8 result;
    u8 isSender;
    u8 found;
    u8 index;
    DeliveryIrcSeq seq;
    DeliveryIrcWanted wanted;
};

static void DeliveryIrc_OnNegotiation(void *work, int netId);
static void DeliveryIrc_SetSeq(DeliveryIrcWork *pWork, DeliveryIrcSeq seq);
static void DeliveryIrc_SetSeqWithLine(DeliveryIrcWork *pWork, DeliveryIrcSeq seq, u32 line);
static void DeliveryIrc_RecvData(int netId, int size, const void *data, void *work, NetHandle *handle);
static void DeliveryIrc_RecvHeader(int netId, int size, const void *data, void *work, NetHandle *handle);
static void DeliveryIrc_RecvWanted(int netId, int size, const void *data, void *work, NetHandle *handle);
static void DeliveryIrc_RecvNoData(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *DeliveryIrc_GetRecvBuffer(int netId, void *work, int size);
static void DeliveryIrc_SeqIdle(void *work);
static void DeliveryIrc_SeqCheckResult(void *work);
static void DeliveryIrc_SeqExitParent(void *work);
static void DeliveryIrc_SeqExitChild(void *work);
static void DeliveryIrc_SeqWaitDoneTiming(void *work);
static void DeliveryIrc_SeqSendDoneTiming(void *work);
static void DeliveryIrc_SeqSendData(void *work);
static void DeliveryIrc_SeqSendHeader(void *work);
static void DeliveryIrc_SeqFindData(void *work);
static void DeliveryIrc_SeqSendWanted(void *work);
static void DeliveryIrc_SeqWaitConnect(void *work);
static void DeliveryIrc_SeqConnect(void *work);
static void DeliveryIrc_SeqWaitNetInit(void *work);
static BOOL DeliveryIrc_InitNet(DeliveryIrcWork *pWork);

// clang-format off
static const NetCommand data_ov012_0216af3c[] = {
    {DeliveryIrc_RecvData, DeliveryIrc_GetRecvBuffer},
    {DeliveryIrc_RecvHeader, NULL},
    {DeliveryIrc_RecvWanted, NULL},
    {DeliveryIrc_RecvNoData, NULL},
};

// clang-format on

// clang-format off
static GFLNetInitData data_ov012_0216e110 = {
    data_ov012_0216af3c,
    NELEMS(data_ov012_0216af3c),
    NULL,
    DeliveryIrc_OnNegotiation,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    {0},
    NULL,
    NULL,
    0,
    {1, 0, 0, 0, 0x80, 0x13, 0, 0},
    HEAPID_USER,
    0xd,
    0xf,
    0x10,
    0xf0,
    0,
    2,
    0x40,
    0x10,
    1,
    0,
    3,
    1,
    0x1f,
    {0xff, 0xff, 0xff, 0xff},
    0,
    0,
};

// clang-format on

static void DeliveryIrc_OnNegotiation(void *work, int netId) {
    DeliveryIrcWork *pWork = work;

    pWork->negotiated = TRUE;
}

static void DeliveryIrc_SetSeq(DeliveryIrcWork *pWork, DeliveryIrcSeq seq) {
    pWork->seq = seq;
}

static void DeliveryIrc_SetSeqWithLine(DeliveryIrcWork *pWork, DeliveryIrcSeq seq, u32 line) {
    DeliveryIrc_SetSeq(pWork, seq);
}

static void DeliveryIrc_RecvData(int netId, int size, const void *data, void *work, NetHandle *handle) {
}

static void DeliveryIrc_RecvHeader(int netId, int size, const void *data, void *work, NetHandle *handle) {
    DeliveryIrcWork *pWork = work;
    const DeliveryIrcHeader *header = data;

    pWork->crc = header->crc;
    pWork->seed = header->seed;
}

static void DeliveryIrc_RecvWanted(int netId, int size, const void *data, void *work, NetHandle *handle) {
    DeliveryIrcWork *pWork = work;
    const DeliveryIrcWanted *wanted = data;

    pWork->wanted = *wanted;
}

static void DeliveryIrc_RecvNoData(int netId, int size, const void *data, void *work, NetHandle *handle) {
    DeliveryIrcWork *pWork = work;
    const u32 *noData = data;

    pWork->noData = *noData;
}

static void *DeliveryIrc_GetRecvBuffer(int netId, void *work, int size) {
    DeliveryIrcWork *pWork = work;

    return pWork->buffer;
}

static void DeliveryIrc_SeqIdle(void *work) {
}

// The receiver checks the data that came
static void DeliveryIrc_SeqCheckResult(void *work) {
    DeliveryIrcWork *pWork = work;

    if (GFL_NetIsInitialized()) {
        return;
    }
    if (pWork->isSender == FALSE) {
        if (pWork->noData == TRUE) {
            pWork->result = 3;
        } else {
            pWork->result = 1;
            MI_CpuCopy8(pWork->buffer, pWork->aInit.data[0].pData, pWork->aInit.data[0].datasize);
            if (getCRC16(pWork->aInit.data[0].pData, pWork->aInit.data[0].datasize) != pWork->crc) {
                pWork->result = 2;
            }
            decryptData(pWork->aInit.data[0].pData, pWork->aInit.data[0].datasize, pWork->seed);
        }
    } else if (pWork->found == TRUE) {
        pWork->result = 1;
    } else {
        pWork->result = 3;
    }
    DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqIdle, 248);
}

static void DeliveryIrc_SeqExitParent(void *work) {
    if (NetDevIrc_IsConnected()) {
        GFL_NetExit(NULL);
    }
    DeliveryIrc_SetSeqWithLine(work, DeliveryIrc_SeqCheckResult, 258);
}

static void DeliveryIrc_SeqExitChild(void *work) {
    GFL_NetExit(NULL);
    DeliveryIrc_SetSeqWithLine(work, DeliveryIrc_SeqCheckResult, 266);
}

static void DeliveryIrc_SeqWaitDoneTiming(void *work) {
    DeliveryIrcWork *pWork = work;

    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 35, (u8)pWork->aInit.code)) {
        if (GFL_NetIsParent()) {
            NetDevIrc_SetTimeout(300);
            DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqExitParent, 275);
        } else {
            DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqExitChild, 278);
        }
    }
}

static void DeliveryIrc_SeqSendDoneTiming(void *work) {
    DeliveryIrcWork *pWork = work;

    NetHandle_SendTimingPair(NetHandle_GetOwn(), 35, (u8)pWork->aInit.code);
    DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqWaitDoneTiming, 287);
}

// The sender sends the data
static void DeliveryIrc_SeqSendData(void *work) {
    DeliveryIrcWork *pWork = work;
    DeliveryData *data;

    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 24, (u8)pWork->aInit.code)) {
        if (pWork->isSender) {
            if (pWork->found == TRUE) {
                data = &pWork->aInit.data[pWork->index];
                if (GFL_NetSendCommandToMachine(NetHandle_GetOwn(), 0xff, pWork->aInit.code << 8, data->datasize,
                                                data->pData, 0, FALSE, TRUE)) {
                    DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqSendDoneTiming, 302);
                }
            } else {
                DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqSendDoneTiming, 307);
            }
        } else {
            DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqSendDoneTiming, 311);
        }
    }
}

// The sender sends the header, or that it has no data
static void DeliveryIrc_SeqSendHeader(void *work) {
    DeliveryIrcWork *pWork = work;
    DeliveryIrcHeader header;

    if (pWork->found == TRUE) {
        header.crc = getCRC16(pWork->sendData->pData, pWork->sendData->datasize);
        header.seed = pWork->seed;
        if (GFL_NetSendCommand(NetHandle_GetOwn(), (u16)((pWork->aInit.code << 8) + 1), sizeof(DeliveryIrcHeader),
                               &header)) {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 24, (u8)pWork->aInit.code);
            pWork->negotiated = FALSE;
            DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqSendData, 338);
        }
    } else {
        pWork->noData = TRUE;
        if (GFL_NetSendCommand(NetHandle_GetOwn(), (u16)((pWork->aInit.code << 8) + 3), sizeof(pWork->noData),
                               &pWork->noData)) {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 24, (u8)pWork->aInit.code);
            pWork->negotiated = FALSE;
            DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqSendData, 354);
        }
    }
}

// The sender looks for data the receiver wants
static void DeliveryIrc_SeqFindData(void *work) {
    DeliveryIrcWork *pWork = work;
    DeliveryData *data;
    u32 i;

    if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 23, (u8)pWork->aInit.code)) {
        if (pWork->isSender) {
            data = NULL;
            pWork->found = FALSE;
            for (i = 0; i < pWork->aInit.dataNum; i++) {
                data = &pWork->aInit.data[i];
                if (data->region == pWork->wanted.region && (data->mask & pWork->wanted.mask)) {
                    pWork->index = i;
                    pWork->found = TRUE;
                    break;
                }
            }
            DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqSendHeader, 392);
            if (pWork->found == TRUE) {
                pWork->seed = GFL_RandomLC(0);
                decryptData(data->pData, data->datasize, pWork->seed);
                pWork->sendData = data;
            }
        } else {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 24, (u8)pWork->aInit.code);
            DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqSendData, 403);
        }
    }
}

// The receiver says what it wants
static void DeliveryIrc_SeqSendWanted(void *work) {
    DeliveryIrcWork *pWork = work;

    if (pWork->isSender == FALSE) {
        pWork->wanted.region = pWork->aInit.data[0].region;
        pWork->wanted.mask = pWork->aInit.data[0].mask;
        if (GFL_NetSendCommand(NetHandle_GetOwn(), (u16)((pWork->aInit.code << 8) + 2), sizeof(DeliveryIrcWanted),
                               &pWork->wanted)) {
            DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqFindData, 418);
        }
    } else {
        DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqFindData, 422);
    }
}

static void DeliveryIrc_SeqWaitConnect(void *work) {
    DeliveryIrcWork *pWork = work;

    if (GFL_NetGetNegotiatedCount() > 1) {
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 23, (u8)pWork->aInit.code);
        DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqSendWanted, 431);
    }
}

static void DeliveryIrc_SeqConnect(void *work) {
    func_020429f8(NULL);
    DeliveryIrc_SetSeqWithLine(work, DeliveryIrc_SeqWaitConnect, 439);
}

static void DeliveryIrc_SeqWaitNetInit(void *work) {
    if (GFL_NetIsInitialized() == TRUE) {
        DeliveryIrc_SetSeqWithLine(work, DeliveryIrc_SeqConnect, 446);
    }
}

void *DeliveryIrc_Init(const DeliveryIrcInit *init) {
    DeliveryIrcWork *pWork = GFL_HeapAllocate(init->heapId, sizeof(DeliveryIrcWork), TRUE, "delivery_irc.c", 461);

    MI_CpuCopy8(init, &pWork->aInit, sizeof(DeliveryIrcInit));
    return pWork;
}

static BOOL DeliveryIrc_InitNet(DeliveryIrcWork *pWork) {
    if (GFL_NetIsInitialized()) {
        return FALSE;
    }
    data_ov012_0216e110.gameCommandBase = pWork->aInit.code;
    GFL_NetInit(&data_ov012_0216e110, NULL, pWork);
    return TRUE;
}

BOOL DeliveryIrc_RecvStart(void *work) {
    DeliveryIrcWork *pWork = work;
    BOOL started = DeliveryIrc_InitNet(pWork);

    pWork->isSender = FALSE;
    pWork->found = FALSE;
    pWork->index = 0;
    pWork->noData = 0;
    DeliveryIrc_SetSeqWithLine(pWork, DeliveryIrc_SeqWaitNetInit, 561);
    return started;
}

u8 DeliveryIrc_GetResult(void *work) {
    DeliveryIrcWork *pWork = work;

    return pWork->result;
}

void DeliveryIrc_Main(void *work) {
    DeliveryIrcWork *pWork = work;

    if (pWork->seq != NULL) {
        pWork->seq(pWork);
    }
}

void DeliveryIrc_End(void *work) {
    GFL_HeapFree(work);
    GFL_NetExit(NULL);
}
