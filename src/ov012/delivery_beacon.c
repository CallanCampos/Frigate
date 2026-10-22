#include "types.h"
#include "field/delivery_beacon.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_whpipe.h"
#include "gfl/std.h"

// A beacon of the data
typedef struct {
    u16 crc;
    u16 unk2;
    // The seed of the data's encryption
    u32 seed;
    u8 data[DELIVERY_BEACON_ONCE_NUM];
    // Which of the beacons this is, from 1, and how many there are
    u8 index;
    u8 count;
    u8 unk5E;
    u8 region;
    u32 mask;
} DeliveryBeaconData;

typedef struct DeliveryBeaconWork DeliveryBeaconWork;
typedef void (*DeliveryBeaconSeq)(void *work);

struct DeliveryBeaconWork {
    DeliveryInit aInit;
    DeliveryBeaconData beacons[7][DELIVERY_BEACON_MAX_NUM];
    int sendIndex;
    // The unk5E of the beacons taken, 0xff until the first one
    u8 unk5E;
    DeliveryBeaconSeq seq;
};

static void DeliveryBeacon_OnDisconnect(void *work, int netId);
static void DeliveryBeacon_RecvDummy(int netId, int size, const void *data, void *work, NetHandle *handle);
static void DeliveryBeacon_OnNegotiation(void *work, int netId);
static void func_ov012_02152928(void *work);
static void *DeliveryBeacon_GetSendBeacon(void *work);
static int DeliveryBeacon_GetBeaconSize(void *work);
static BOOL DeliveryBeacon_IsMatchingBeacon(u32 a, u32 b);
static void DeliveryBeacon_SetSeq(DeliveryBeaconWork *pWork, DeliveryBeaconSeq seq);
static void DeliveryBeacon_SetSeqWithLine(DeliveryBeaconWork *pWork, DeliveryBeaconSeq seq, u32 line);
static void DeliveryBeacon_SeqIdle(void *work);
static void DeliveryBeacon_CheckDataSize(DeliveryBeaconWork *pWork);
static BOOL DeliveryBeacon_IsAllReceived(DeliveryBeaconWork *pWork);
static void DeliveryBeacon_SeqReceive(void *work);
static void DeliveryBeacon_DecryptData(DeliveryBeaconWork *pWork);
static void DeliveryBeacon_SeqStartNet(void *work);
static void DeliveryBeacon_OnNetInit(void *work);

static const NetCommand data_ov012_0216af34[] = {
    { DeliveryBeacon_RecvDummy, NULL },
};

// clang-format off
static GFLNetInitData data_ov012_0216e034 = {
    data_ov012_0216af34,
    NELEMS(data_ov012_0216af34),
    DeliveryBeacon_OnDisconnect,
    DeliveryBeacon_OnNegotiation,
    NULL,
    NULL,
    DeliveryBeacon_GetSendBeacon,
    DeliveryBeacon_GetBeaconSize,
    DeliveryBeacon_IsMatchingBeacon,
    NULL,
    NULL,
    func_ov012_02152928,
    NULL,
    {0},
    NULL,
    NULL,
    0,
    {1, 0, 0, 0, 0x80, 0x13, 0, 0},
    HEAPID_USER,
    0xd,
    0xf,
    0xd,
    0xf0,
    0,
    1,
    0x58,
    0x10,
    1,
    0,
    0,
    1,
    5,
    {0x2c, 1, 0, 0},
    0x1f4,
    0,
};

// clang-format on

static void DeliveryBeacon_OnDisconnect(void *work, int netId) {
}

static void DeliveryBeacon_RecvDummy(int netId, int size, const void *data, void *work, NetHandle *handle) {
}

static void DeliveryBeacon_OnNegotiation(void *work, int netId) {
}

static void func_ov012_02152928(void *work) {
}

// The beacon to send
static void *DeliveryBeacon_GetSendBeacon(void *work) {
    DeliveryBeaconWork *pWork = work;
    int index = pWork->sendIndex % DELIVERY_BEACON_MAX_NUM;
    int dataIndex = pWork->sendIndex / DELIVERY_BEACON_MAX_NUM;

    if (dataIndex >= pWork->aInit.dataNum) {
        pWork->sendIndex = 0;
        dataIndex = 0;
    }
    return &pWork->beacons[dataIndex][index];
}

static int DeliveryBeacon_GetBeaconSize(void *work) {
    return sizeof(DeliveryBeaconData);
}

static BOOL DeliveryBeacon_IsMatchingBeacon(u32 a, u32 b) {
    if (a == b) {
        return TRUE;
    }
    return FALSE;
}

static void DeliveryBeacon_SetSeq(DeliveryBeaconWork *pWork, DeliveryBeaconSeq seq) {
    pWork->seq = seq;
}

static void DeliveryBeacon_SetSeqWithLine(DeliveryBeaconWork *pWork, DeliveryBeaconSeq seq, u32 line) {
    DeliveryBeacon_SetSeq(pWork, seq);
}

static void DeliveryBeacon_SeqIdle(void *work) {
}

void *DeliveryBeacon_Init(const DeliveryInit *init) {
    DeliveryBeaconWork *pWork =
        GFL_HeapAllocate(init->heapId, sizeof(DeliveryBeaconWork), TRUE, "delivery_beacon.c", 295);

    MI_CpuCopy8(init, &pWork->aInit, sizeof(DeliveryInit));
    return pWork;
}

// File metadata for failed assertions.
static char sDeliveryBeaconAssertFileName[] = "";

static void DeliveryBeacon_CheckDataSize(DeliveryBeaconWork *pWork) {
    if (pWork->aInit.data[0].datasize != 0) {
        // The assert's text is the game's, spacing included
        // clang-format off
        GFL_ASSERT_WITH_FILE(sDeliveryBeaconAssertFileName, pWork->aInit.data[0].datasize < (DELIVERY_BEACON_MAX_NUM*DELIVERY_BEACON_ONCE_NUM));
        // clang-format on
    }
}

// Whether every beacon has come
static BOOL DeliveryBeacon_IsAllReceived(DeliveryBeaconWork *pWork) {
    int i;
    int count = pWork->beacons[0][0].count;

    if (count == 0) {
        return FALSE;
    }
    for (i = 0; i < count; i++) {
        if (count != pWork->beacons[0][i].count) {
            return FALSE;
        }
    }
    return TRUE;
}

// Takes the beacons that have come
static void DeliveryBeacon_SeqReceive(void *work) {
    DeliveryBeaconWork *pWork = work;
    int i;
    int index;
    DeliveryBeaconData *beacon;
    u16 crc;

    for (i = 0; i < 16; i++) {
        beacon = NetWhpipe_GetBeaconData(i);
        if (beacon == NULL) {
            continue;
        }
        if (beacon->region != pWork->aInit.data[0].region) {
            NetWhpipe_RemoveBeacon(i);
        } else if ((beacon->mask & pWork->aInit.data[0].mask) == 0) {
            NetWhpipe_RemoveBeacon(i);
        } else if (beacon->unk5E != pWork->unk5E && pWork->unk5E != 0xff) {
            NetWhpipe_RemoveBeacon(i);
        } else {
            index = beacon->index - 1;
            if (index < DELIVERY_BEACON_MAX_NUM && pWork->beacons[0][index].index == 0) {
                crc = getCRC16(beacon->data, DELIVERY_BEACON_ONCE_NUM);
                if (crc != beacon->crc) {
                    NetWhpipe_RemoveBeacon(i);
                } else {
                    if (pWork->unk5E == 0xff) {
                        pWork->unk5E = beacon->unk5E;
                    }
                    NetWhpipe_SetScanTarget(i);
                    MI_CpuCopy8(beacon, &pWork->beacons[0][index], sizeof(DeliveryBeaconData));
                    NetWhpipe_RemoveAllBeacons();
                }
            }
        }
    }
}

// Decrypts the beacons' data into the buffer
static void DeliveryBeacon_DecryptData(DeliveryBeaconWork *pWork) {
    int i;
    DeliveryBeaconData *beacon;
    u16 pos;

    for (i = 0; i < DELIVERY_BEACON_MAX_NUM; i++) {
        pos = i * DELIVERY_BEACON_ONCE_NUM;
        beacon = &pWork->beacons[0][i];
        decryptData(beacon->data, DELIVERY_BEACON_ONCE_NUM, beacon->seed);
        if (pWork->aInit.data[0].datasize > pos + DELIVERY_BEACON_ONCE_NUM) {
            MI_CpuCopy8(beacon->data, pWork->aInit.data[0].pData + i * DELIVERY_BEACON_ONCE_NUM,
                        DELIVERY_BEACON_ONCE_NUM);
        } else {
            MI_CpuCopy8(beacon->data, pWork->aInit.data[0].pData + i * DELIVERY_BEACON_ONCE_NUM,
                        pWork->aInit.data[0].datasize - pos);
        }
    }
}

static void DeliveryBeacon_SeqStartNet(void *work) {
    GFL_NetStartScan();
    DeliveryBeacon_SetSeqWithLine(work, DeliveryBeacon_SeqReceive, 471);
}

static void DeliveryBeacon_OnNetInit(void *work) {
    DeliveryBeacon_SetSeqWithLine(work, DeliveryBeacon_SeqStartNet, 477);
}

BOOL DeliveryBeacon_RecvStart(void *work) {
    DeliveryBeaconWork *pWork = work;

    if (GFL_NetIsInitialized()) {
        return FALSE;
    }
    data_ov012_0216e034.gameCommandBase = pWork->aInit.code;
    pWork->unk5E = 0xff;
    DeliveryBeacon_CheckDataSize(pWork);
    GFL_NetInit(&data_ov012_0216e034, DeliveryBeacon_OnNetInit, pWork);
    DeliveryBeacon_SetSeqWithLine(pWork, DeliveryBeacon_SeqIdle, 498);
    return TRUE;
}

BOOL DeliveryBeacon_IsAnyReceived(void *work) {
    DeliveryBeaconWork *pWork = work;
    int i;

    for (i = 0; i < DELIVERY_BEACON_MAX_NUM; i++) {
        if (pWork->beacons[0][i].count != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL DeliveryBeacon_IsReceiveDone(void *work) {
    BOOL done = DeliveryBeacon_IsAllReceived(work);

    if (done) {
        DeliveryBeacon_DecryptData(work);
    }
    return done;
}

void DeliveryBeacon_Main(void *work) {
    DeliveryBeaconWork *pWork = work;

    pWork->seq(pWork);
}

void DeliveryBeacon_End(void *work) {
    GFL_HeapFree(work);
    GFL_NetExit(NULL);
}
