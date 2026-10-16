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

static void func_ov012_02152c0c(void *work, int netId);
static void func_ov012_02152c18(DeliveryIrcWork *pWork, DeliveryIrcSeq seq);
static void func_ov012_02152c24(DeliveryIrcWork *pWork, DeliveryIrcSeq seq, u32 line);
static void func_ov012_02152c2c(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov012_02152c30(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov012_02152c44(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov012_02152c5c(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *func_ov012_02152c68(int netId, void *work, int size);
static void func_ov012_02152c70(void *work);
static void func_ov012_02152c74(void *work);
static void func_ov012_02152cf4(void *work);
static void func_ov012_02152d1c(void *work);
static void func_ov012_02152d3c(void *work);
static void func_ov012_02152d8c(void *work);
static void func_ov012_02152db4(void *work);
static void func_ov012_02152e4c(void *work);
static void func_ov012_02152f0c(void *work);
static void func_ov012_02152fd0(void *work);
static void func_ov012_0215302c(void *work);
static void func_ov012_0215305c(void *work);
static void func_ov012_0215307c(void *work);
static BOOL func_ov012_021530cc(DeliveryIrcWork *pWork);

// clang-format off
static const NetCommand data_ov012_0216af3c[] = {
    {func_ov012_02152c2c, func_ov012_02152c68},
    {func_ov012_02152c30, NULL},
    {func_ov012_02152c44, NULL},
    {func_ov012_02152c5c, NULL},
};

// clang-format on

// clang-format off
static GFLNetInitData data_ov012_0216e110 = {
    data_ov012_0216af3c,
    NELEMS(data_ov012_0216af3c),
    NULL,
    func_ov012_02152c0c,
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

static void func_ov012_02152c0c(void *work, int netId) {
    DeliveryIrcWork *pWork = work;

    pWork->negotiated = TRUE;
}

static void func_ov012_02152c18(DeliveryIrcWork *pWork, DeliveryIrcSeq seq) {
    pWork->seq = seq;
}

static void func_ov012_02152c24(DeliveryIrcWork *pWork, DeliveryIrcSeq seq, u32 line) {
    func_ov012_02152c18(pWork, seq);
}

static void func_ov012_02152c2c(int netId, int size, const void *data, void *work, NetHandle *handle) {
}

static void func_ov012_02152c30(int netId, int size, const void *data, void *work, NetHandle *handle) {
    DeliveryIrcWork *pWork = work;
    const DeliveryIrcHeader *header = data;

    pWork->crc = header->crc;
    pWork->seed = header->seed;
}

static void func_ov012_02152c44(int netId, int size, const void *data, void *work, NetHandle *handle) {
    DeliveryIrcWork *pWork = work;
    const DeliveryIrcWanted *wanted = data;

    pWork->wanted = *wanted;
}

static void func_ov012_02152c5c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    DeliveryIrcWork *pWork = work;
    const u32 *noData = data;

    pWork->noData = *noData;
}

static void *func_ov012_02152c68(int netId, void *work, int size) {
    DeliveryIrcWork *pWork = work;

    return pWork->buffer;
}

static void func_ov012_02152c70(void *work) {
}

// The receiver checks the data that came
static void func_ov012_02152c74(void *work) {
    DeliveryIrcWork *pWork = work;

    if (func_02042788()) {
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
    func_ov012_02152c24(pWork, func_ov012_02152c70, 248);
}

static void func_ov012_02152cf4(void *work) {
    if (NetDevIrc_IsConnected()) {
        func_02042860(NULL);
    }
    func_ov012_02152c24(work, func_ov012_02152c74, 258);
}

static void func_ov012_02152d1c(void *work) {
    func_02042860(NULL);
    func_ov012_02152c24(work, func_ov012_02152c74, 266);
}

static void func_ov012_02152d3c(void *work) {
    DeliveryIrcWork *pWork = work;

    if (func_02040664(func_02040440(), 35, (u8)pWork->aInit.code)) {
        if (func_02042bc4()) {
            NetDevIrc_SetTimeout(300);
            func_ov012_02152c24(pWork, func_ov012_02152cf4, 275);
        } else {
            func_ov012_02152c24(pWork, func_ov012_02152d1c, 278);
        }
    }
}

static void func_ov012_02152d8c(void *work) {
    DeliveryIrcWork *pWork = work;

    func_02040624(func_02040440(), 35, (u8)pWork->aInit.code);
    func_ov012_02152c24(pWork, func_ov012_02152d3c, 287);
}

// The sender sends the data
static void func_ov012_02152db4(void *work) {
    DeliveryIrcWork *pWork = work;
    DeliveryData *data;

    if (func_02040664(func_02040440(), 24, (u8)pWork->aInit.code)) {
        if (pWork->isSender) {
            if (pWork->found == TRUE) {
                data = &pWork->aInit.data[pWork->index];
                if (func_02042c18(func_02040440(), 0xff, pWork->aInit.code << 8, data->datasize, data->pData, 0, FALSE,
                                  TRUE)) {
                    func_ov012_02152c24(pWork, func_ov012_02152d8c, 302);
                }
            } else {
                func_ov012_02152c24(pWork, func_ov012_02152d8c, 307);
            }
        } else {
            func_ov012_02152c24(pWork, func_ov012_02152d8c, 311);
        }
    }
}

// The sender sends the header, or that it has no data
static void func_ov012_02152e4c(void *work) {
    DeliveryIrcWork *pWork = work;
    DeliveryIrcHeader header;

    if (pWork->found == TRUE) {
        header.crc = getCRC16(pWork->sendData->pData, pWork->sendData->datasize);
        header.seed = pWork->seed;
        if (func_02042be8(func_02040440(), (u16)((pWork->aInit.code << 8) + 1), sizeof(DeliveryIrcHeader), &header)) {
            func_02040624(func_02040440(), 24, (u8)pWork->aInit.code);
            pWork->negotiated = FALSE;
            func_ov012_02152c24(pWork, func_ov012_02152db4, 338);
        }
    } else {
        pWork->noData = TRUE;
        if (func_02042be8(func_02040440(), (u16)((pWork->aInit.code << 8) + 3), sizeof(pWork->noData),
                          &pWork->noData)) {
            func_02040624(func_02040440(), 24, (u8)pWork->aInit.code);
            pWork->negotiated = FALSE;
            func_ov012_02152c24(pWork, func_ov012_02152db4, 354);
        }
    }
}

// The sender looks for data the receiver wants
static void func_ov012_02152f0c(void *work) {
    DeliveryIrcWork *pWork = work;
    DeliveryData *data;
    u32 i;

    if (func_02040664(func_02040440(), 23, (u8)pWork->aInit.code)) {
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
            func_ov012_02152c24(pWork, func_ov012_02152e4c, 392);
            if (pWork->found == TRUE) {
                pWork->seed = GFL_RandomLC(0);
                decryptData(data->pData, data->datasize, pWork->seed);
                pWork->sendData = data;
            }
        } else {
            func_02040624(func_02040440(), 24, (u8)pWork->aInit.code);
            func_ov012_02152c24(pWork, func_ov012_02152db4, 403);
        }
    }
}

// The receiver says what it wants
static void func_ov012_02152fd0(void *work) {
    DeliveryIrcWork *pWork = work;

    if (pWork->isSender == FALSE) {
        pWork->wanted.region = pWork->aInit.data[0].region;
        pWork->wanted.mask = pWork->aInit.data[0].mask;
        if (func_02042be8(func_02040440(), (u16)((pWork->aInit.code << 8) + 2), sizeof(DeliveryIrcWanted),
                          &pWork->wanted)) {
            func_ov012_02152c24(pWork, func_ov012_02152f0c, 418);
        }
    } else {
        func_ov012_02152c24(pWork, func_ov012_02152f0c, 422);
    }
}

static void func_ov012_0215302c(void *work) {
    DeliveryIrcWork *pWork = work;

    if (func_02042a78() > 1) {
        func_02040624(func_02040440(), 23, (u8)pWork->aInit.code);
        func_ov012_02152c24(pWork, func_ov012_02152fd0, 431);
    }
}

static void func_ov012_0215305c(void *work) {
    func_020429f8(NULL);
    func_ov012_02152c24(work, func_ov012_0215302c, 439);
}

static void func_ov012_0215307c(void *work) {
    if (func_02042788() == TRUE) {
        func_ov012_02152c24(work, func_ov012_0215305c, 446);
    }
}

void *func_ov012_0215309c(const DeliveryIrcInit *init) {
    DeliveryIrcWork *pWork = GFL_HeapAllocate(init->heapId, sizeof(DeliveryIrcWork), TRUE, "delivery_irc.c", 461);

    MI_CpuCopy8(init, &pWork->aInit, sizeof(DeliveryIrcInit));
    return pWork;
}

static BOOL func_ov012_021530cc(DeliveryIrcWork *pWork) {
    if (func_02042788()) {
        return FALSE;
    }
    data_ov012_0216e110.gameCommandBase = pWork->aInit.code;
    func_020425ec(&data_ov012_0216e110, NULL, pWork);
    return TRUE;
}

BOOL func_ov012_021530f8(void *work) {
    DeliveryIrcWork *pWork = work;
    BOOL started = func_ov012_021530cc(pWork);

    pWork->isSender = FALSE;
    pWork->found = FALSE;
    pWork->index = 0;
    pWork->noData = 0;
    func_ov012_02152c24(pWork, func_ov012_0215307c, 561);
    return started;
}

u8 func_ov012_02153130(void *work) {
    DeliveryIrcWork *pWork = work;

    return pWork->result;
}

void func_ov012_0215313c(void *work) {
    DeliveryIrcWork *pWork = work;

    if (pWork->seq != NULL) {
        pWork->seq(pWork);
    }
}

void func_ov012_02153150(void *work) {
    GFL_HeapFree(work);
    func_02042860(NULL);
}
