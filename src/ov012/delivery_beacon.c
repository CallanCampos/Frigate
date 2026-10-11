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

static void func_ov012_0215291c(void *work, int netId);
static void func_ov012_02152920(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov012_02152924(void *work, int netId);
static void func_ov012_02152928(void *work);
static void *func_ov012_0215292c(void *work);
static int func_ov012_02152968(void *work);
static BOOL func_ov012_0215296c(u32 a, u32 b);
static void func_ov012_02152978(DeliveryBeaconWork *pWork, DeliveryBeaconSeq seq);
static void func_ov012_02152984(DeliveryBeaconWork *pWork, DeliveryBeaconSeq seq, u32 line);
static void func_ov012_0215298c(void *work);
static void func_ov012_021529c0(DeliveryBeaconWork *pWork);
static BOOL func_ov012_021529e8(DeliveryBeaconWork *pWork);
static void func_ov012_02152a24(void *work);
static void func_ov012_02152adc(DeliveryBeaconWork *pWork);
static void func_ov012_02152b34(void *work);
static void func_ov012_02152b50(void *work);

static const NetCommand data_ov012_0216af34[] = {
    {func_ov012_02152920, NULL},
};

static GFLNetInitData data_ov012_0216e034 = {
    data_ov012_0216af34,
    NELEMS(data_ov012_0216af34),
    func_ov012_0215291c,
    func_ov012_02152924,
    NULL,
    NULL,
    func_ov012_0215292c,
    func_ov012_02152968,
    func_ov012_0215296c,
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

static void func_ov012_0215291c(void *work, int netId) {
}

static void func_ov012_02152920(int netId, int size, const void *data, void *work, NetHandle *handle) {
}

static void func_ov012_02152924(void *work, int netId) {
}

static void func_ov012_02152928(void *work) {
}

// The beacon to send
static void *func_ov012_0215292c(void *work) {
    DeliveryBeaconWork *pWork = work;
    int index = pWork->sendIndex % DELIVERY_BEACON_MAX_NUM;
    int dataIndex = pWork->sendIndex / DELIVERY_BEACON_MAX_NUM;

    if (dataIndex >= pWork->aInit.dataNum) {
        pWork->sendIndex = 0;
        dataIndex = 0;
    }
    return &pWork->beacons[dataIndex][index];
}

static int func_ov012_02152968(void *work) {
    return sizeof(DeliveryBeaconData);
}

static BOOL func_ov012_0215296c(u32 a, u32 b) {
    if (a == b) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov012_02152978(DeliveryBeaconWork *pWork, DeliveryBeaconSeq seq) {
    pWork->seq = seq;
}

static void func_ov012_02152984(DeliveryBeaconWork *pWork, DeliveryBeaconSeq seq, u32 line) {
    func_ov012_02152978(pWork, seq);
}

static void func_ov012_0215298c(void *work) {
}

void *func_ov012_02152990(const DeliveryInit *init) {
    DeliveryBeaconWork *pWork =
        GFL_HeapAllocate(init->heapId, sizeof(DeliveryBeaconWork), TRUE, "delivery_beacon.c", 295);

    sys_memcpy(init, &pWork->aInit, sizeof(DeliveryInit));
    return pWork;
}

static void func_ov012_021529c0(DeliveryBeaconWork *pWork) {
    if (pWork->aInit.data[0].datasize != 0) {
        // The assert's text is the game's, spacing included
        // clang-format off
        GFL_ASSERT(pWork->aInit.data[0].datasize < (DELIVERY_BEACON_MAX_NUM*DELIVERY_BEACON_ONCE_NUM));
        // clang-format on
    }
}

// Whether every beacon has come
static BOOL func_ov012_021529e8(DeliveryBeaconWork *pWork) {
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
static void func_ov012_02152a24(void *work) {
    DeliveryBeaconWork *pWork = work;
    int i;
    int index;
    DeliveryBeaconData *beacon;
    u16 crc;

    for (i = 0; i < 16; i++) {
        beacon = func_ov030_02173b24(i);
        if (beacon == NULL) {
            continue;
        }
        if (beacon->region != pWork->aInit.data[0].region) {
            func_ov030_02173ba4(i);
        } else if ((beacon->mask & pWork->aInit.data[0].mask) == 0) {
            func_ov030_02173ba4(i);
        } else if (beacon->unk5E != pWork->unk5E && pWork->unk5E != 0xff) {
            func_ov030_02173ba4(i);
        } else {
            index = beacon->index - 1;
            if (index < DELIVERY_BEACON_MAX_NUM && pWork->beacons[0][index].index == 0) {
                crc = getCRC16(beacon->data, DELIVERY_BEACON_ONCE_NUM);
                if (crc != beacon->crc) {
                    func_ov030_02173ba4(i);
                } else {
                    if (pWork->unk5E == 0xff) {
                        pWork->unk5E = beacon->unk5E;
                    }
                    func_ov030_021740d0(i);
                    sys_memcpy(beacon, &pWork->beacons[0][index], sizeof(DeliveryBeaconData));
                    func_ov030_02173bc4();
                }
            }
        }
    }
}

// Decrypts the beacons' data into the buffer
static void func_ov012_02152adc(DeliveryBeaconWork *pWork) {
    int i;
    DeliveryBeaconData *beacon;
    u16 pos;

    for (i = 0; i < DELIVERY_BEACON_MAX_NUM; i++) {
        pos = i * DELIVERY_BEACON_ONCE_NUM;
        beacon = &pWork->beacons[0][i];
        decryptData(beacon->data, DELIVERY_BEACON_ONCE_NUM, beacon->seed);
        if (pWork->aInit.data[0].datasize > pos + DELIVERY_BEACON_ONCE_NUM) {
            sys_memcpy(beacon->data, pWork->aInit.data[0].pData + i * DELIVERY_BEACON_ONCE_NUM,
                       DELIVERY_BEACON_ONCE_NUM);
        } else {
            sys_memcpy(beacon->data, pWork->aInit.data[0].pData + i * DELIVERY_BEACON_ONCE_NUM,
                       pWork->aInit.data[0].datasize - pos);
        }
    }
}

static void func_ov012_02152b34(void *work) {
    func_02042968();
    func_ov012_02152984(work, func_ov012_02152a24, 471);
}

static void func_ov012_02152b50(void *work) {
    func_ov012_02152984(work, func_ov012_02152b34, 477);
}

BOOL func_ov012_02152b64(void *work) {
    DeliveryBeaconWork *pWork = work;

    if (func_02042788()) {
        return FALSE;
    }
    data_ov012_0216e034.gameCommandBase = pWork->aInit.code;
    pWork->unk5E = 0xff;
    func_ov012_021529c0(pWork);
    func_020425ec(&data_ov012_0216e034, func_ov012_02152b50, pWork);
    func_ov012_02152984(pWork, func_ov012_0215298c, 498);
    return TRUE;
}

BOOL func_ov012_02152bb4(void *work) {
    DeliveryBeaconWork *pWork = work;
    int i;

    for (i = 0; i < DELIVERY_BEACON_MAX_NUM; i++) {
        if (pWork->beacons[0][i].count != 0) {
            return TRUE;
        }
    }
    return FALSE;
}

BOOL func_ov012_02152bd4(void *work) {
    BOOL done = func_ov012_021529e8(work);

    if (done) {
        func_ov012_02152adc(work);
    }
    return done;
}

void func_ov012_02152bec(void *work) {
    DeliveryBeaconWork *pWork = work;

    pWork->seq(pWork);
}

void func_ov012_02152bfc(void *work) {
    GFL_HeapFree(work);
    func_02042860(NULL);
}
