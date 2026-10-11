#include "types.h"
#include "app/mystery/mystery_net.h"
#include "constants/version.h"
#include "dwc/dwc.h"
#include "dwc/nd.h"
#include "field/delivery_beacon.h"
#include "field/delivery_irc.h"
#include "gfl/dwc_rap.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_state.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "nitro/gx.h"
#include "save/mystery_gift.h"
#include "system/version.h"

// Mystery Gift's receiving of gifts: by local wireless beacons and by infrared through overlay 12's delivery_beacon.c
// and delivery_irc.c, and by Wi-Fi through NitroDWC's download library. Our names; swan has none for this overlay

#define MYSTERY_NET_FILE_MAX 10
#define MYSTERY_NET_BUFFER_SIZE 0x1000

typedef struct MysteryNetSeq MysteryNetSeq;

typedef void (*MysteryNetSeqFunc)(MysteryNetSeq *seq, int *state, void *work);

struct MysteryNetSeq {
    MysteryNetSeqFunc func;
    BOOL end;
    int state;
    void *work;
};

// The states of a Wi-Fi download that wait for the library
#define DOWNLOAD_WAIT 100
#define DOWNLOAD_WAIT_CLEANUP 200

typedef struct {
    DWCNdFileInfo files[MYSTERY_NET_FILE_MAX];
    BOOL available[MYSTERY_NET_FILE_MAX];
    // Where a wait goes on to
    u32 nextState;
    BOOL cancel;
    BOOL cancelled;
    int count;
    u32 percent;
    u32 received;
    u32 contentLength;
    u32 selected;
    void *buffer;
    u32 timeout;
    BOOL initialized;
} MysteryNetDownload;

struct MysteryNet {
    MysteryNetSeq seq;
    void *wifiWork;
    u8 info[0x20];
    u8 unk34[0x20];
    u32 beaconFlags;
    HeapID heapId;
    BOOL exited;
    MysteryNetDownload download;
    SaveControl *save;
    void *beacon;
    void *irc;
    u32 unk7A0;
    u8 buffer[MYSTERY_NET_BUFFER_SIZE];
    u32 result;
};

// What the download library's callback last gave
typedef struct {
    BOOL done;
    BOOL ready;
    u32 unk8;
    u32 error;
} MysteryNetNdState;

// A hexadecimal digit and its value
typedef struct {
    s8 c;
    s8 value;
} MysteryHexDigit;

static u32 MysteryNet_CheckErrorWifi(MysteryNet *net);
static u32 MysteryNet_CheckErrorWireless(MysteryNet *net);
static u32 MysteryNet_CheckErrorIrc(MysteryNet *net);
static void MysteryNet_Idle(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_WirelessStart(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_WirelessEnd(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_WirelessReady(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_BeaconStart(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_BeaconEnd(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_BeaconWait(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_IrcStart(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_IrcEnd(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_IrcWait(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_WifiDownload(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNet_WifiEnd(MysteryNetSeq *seq, int *state, void *work);
static void MysteryNetDownload_Wait(MysteryNetDownload *download, int *state, u32 nextState);
static void MysteryNetDownload_Cleanup(MysteryNetDownload *download, u32 error, int *state, u32 nextState,
                                       int failState);
static void MysteryNetSeq_Init(MysteryNetSeq *seq, void *work, MysteryNetSeqFunc func);
static void MysteryNetSeq_Exit(MysteryNetSeq *seq);
static void MysteryNetSeq_Main(MysteryNetSeq *seq);
static void MysteryNetSeq_SetNext(MysteryNetSeq *seq, MysteryNetSeqFunc func);
static BOOL MysteryNetSeq_IsCurrent(MysteryNetSeq *seq, MysteryNetSeqFunc func);
static void *MysteryNet_GetInfo(void *work);
static int MysteryNet_GetInfoSize(void *work);
static BOOL MysteryNet_IsSameInfo(u32 a, u32 b);
static void MysteryNet_OnExit(void *work);
static void MysteryNet_OnNdEvent(u32 reason, u32 error);
static void MysteryNet_SetNdDone(void);
static u32 Mystery_ParseHex(const char *str);
static BOOL Mystery_IsZero(const u32 *data, u32 size);
static u32 MysteryNet_OnDwcEvent(void *work, int a1, int event, int a3);

static const MysteryHexDigit sHexDigits[] = {
    { '0', 0 },  { '1', 1 },  { '2', 2 },  { '3', 3 },  { '4', 4 },  { '5', 5 },  { '6', 6 },   { '7', 7 },
    { '8', 8 },  { '9', 9 },  { 'A', 10 }, { 'B', 11 }, { 'C', 12 }, { 'D', 13 }, { 'E', 14 },  { 'F', 15 },
    { 'a', 10 }, { 'b', 11 }, { 'c', 12 }, { 'd', 13 }, { 'e', 14 }, { 'f', 15 }, { '\0', -1 },
};

static const GFLNetInitData sNetInit = {
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    MysteryNet_GetInfo,
    MysteryNet_GetInfoSize,
    MysteryNet_IsSameInfo,
    NULL,
    NULL,
    NULL,
    NULL,
    { 0 },
    NULL,
    NULL,
    0,
    { 1, 0, 0, 0, 0x80, 0x13, 0, 0 },
    HEAPID_USER,
    0xd,
    0xf,
    0xd,
    0xf0,
    0,
    1,
    0x58,
    4,
    1,
    0,
    0,
    0,
    9,
    { 0x2c, 1, 0, 0 },
    0x1f4,
    0,
};

static MysteryNetNdState sNdState;

MysteryNet *MysteryNet_Create(SaveControl *save, HeapID heapId) {
    MysteryNet *net = GFL_HeapAllocate(heapId, sizeof(MysteryNet), FALSE, "mystery_net.c", 315);

    sys_memset(net, 0, sizeof(MysteryNet));
    net->heapId = heapId;
    net->save = save;
    MysteryNetSeq_Init(&net->seq, net, MysteryNet_Idle);
    return net;
}

void MysteryNet_Delete(MysteryNet *net) {
    MysteryNetSeq_Exit(&net->seq);
    GFL_HeapFree(net);
}

void MysteryNet_Main(MysteryNet *net) {
    MysteryNetSeq_Main(&net->seq);
}

void MysteryNet_ChangeState(MysteryNet *net, u32 state) {
    switch (state) {
    case MYSTERY_NET_STATE_WIRELESS_START:
        if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_Idle)) {
            MysteryNetSeq_SetNext(&net->seq, MysteryNet_WirelessStart);
        }
        break;
    case MYSTERY_NET_STATE_WIRELESS_END:
        if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_WirelessReady)) {
            MysteryNetSeq_SetNext(&net->seq, MysteryNet_WirelessEnd);
        }
        break;
    case MYSTERY_NET_STATE_WIFI:
        if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_Idle)) {
            MysteryNetSeq_SetNext(&net->seq, MysteryNet_WifiDownload);
        }
        break;
    case MYSTERY_NET_STATE_WIFI_CANCEL:
        if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_WifiDownload)) {
            net->download.cancel = TRUE;
        }
        break;
    case MYSTERY_NET_STATE_WIFI_END:
        MysteryNetSeq_SetNext(&net->seq, MysteryNet_WifiEnd);
        break;
    case MYSTERY_NET_STATE_BEACON_START:
        MysteryNetSeq_SetNext(&net->seq, MysteryNet_BeaconStart);
        break;
    case MYSTERY_NET_STATE_BEACON_END:
        MysteryNetSeq_SetNext(&net->seq, MysteryNet_BeaconEnd);
        break;
    case MYSTERY_NET_STATE_IRC_START:
        MysteryNetSeq_SetNext(&net->seq, MysteryNet_IrcStart);
        break;
    case MYSTERY_NET_STATE_IRC_END:
        MysteryNetSeq_SetNext(&net->seq, MysteryNet_IrcEnd);
        break;
    case MYSTERY_NET_STATE_IDLE:
        MysteryNetSeq_SetNext(&net->seq, MysteryNet_Idle);
        break;
    }
}

u32 MysteryNet_GetState(MysteryNet *net) {
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_Idle)) {
        return MYSTERY_NET_STATE_IDLE;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_WirelessStart)) {
        return MYSTERY_NET_STATE_WIRELESS_START;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_WirelessReady)) {
        return MYSTERY_NET_STATE_WIRELESS_READY;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_WirelessEnd)) {
        return MYSTERY_NET_STATE_WIRELESS_END;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_WifiDownload)) {
        return MYSTERY_NET_STATE_WIFI;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_WifiEnd)) {
        return MYSTERY_NET_STATE_WIFI_END;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_BeaconStart)) {
        return MYSTERY_NET_STATE_BEACON_START;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_BeaconWait)) {
        return MYSTERY_NET_STATE_BEACON_WAIT;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_BeaconEnd)) {
        return MYSTERY_NET_STATE_BEACON_END;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_IrcStart)) {
        return MYSTERY_NET_STATE_IRC_START;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_IrcWait)) {
        return MYSTERY_NET_STATE_IRC_WAIT;
    }
    if (MysteryNetSeq_IsCurrent(&net->seq, MysteryNet_IrcEnd)) {
        return MYSTERY_NET_STATE_IRC_END;
    }
    return MYSTERY_NET_STATE_IDLE;
}

u32 MysteryNet_GetBeaconFlags(MysteryNet *net) {
    return net->beaconFlags;
}

u32 MysteryNet_GetRecvData(MysteryNet *net, void *buffer, u32 size) {
    if (net->result == MYSTERY_NET_RECV_OK) {
        sys_memcpy(net->buffer, buffer, size);
        return MYSTERY_NET_RECV_OK;
    }
    return net->result;
}

u32 MysteryNet_GetError(MysteryNet *net) {
    u32 error = MYSTERY_NET_ERROR_NONE;

    if (func_02042788()) {
        switch (func_02042e84()->bNetType) {
        case 0:
        case 5:
            error = MysteryNet_CheckErrorWireless(net);
            break;
        case 1:
            error = MysteryNet_CheckErrorWifi(net);
            break;
        case 3:
            error = MysteryNet_CheckErrorIrc(net);
            break;
        }
    }
    return error;
}

void MysteryNet_ClearError(MysteryNet *net) {
    if (net->wifiWork != NULL) {
        func_02012994(net->wifiWork);
        net->wifiWork = NULL;
    }
    MysteryNet_ChangeState(net, MYSTERY_NET_STATE_IDLE);
    func_02012144();
}

static u32 MysteryNet_CheckErrorWifi(MysteryNet *net) {
    u32 error = MYSTERY_NET_ERROR_NONE;

    if (GFL_NetErrCheck()) {
        switch (func_02042540()->unk4) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            if (net->download.initialized == TRUE) {
                return error;
            }
            func_020424e4();
            func_02058490();
            func_02012154();
            func_02011de0();
            func_02012144();
            error = MYSTERY_NET_ERROR_FATAL;
            break;
        case 7:
            func_020424e4();
            GFL_NetErrAbort();
            error = 3;
            break;
        }
    }
    return error;
}

static u32 MysteryNet_CheckErrorWireless(MysteryNet *net) {
    u32 error = MYSTERY_NET_ERROR_NONE;

    if (GFL_NetErrCheck()) {
        func_02012154();
        func_02011de0();
        func_02012144();
        error = MYSTERY_NET_ERROR_FATAL;
    }
    return error;
}

static u32 MysteryNet_CheckErrorIrc(MysteryNet *net) {
    u32 error = MYSTERY_NET_ERROR_NONE;

    if (GFL_NetErrCheck()) {
        func_02012154();
        func_02011de0();
        func_02012144();
        error = MYSTERY_NET_ERROR_FATAL;
    }
    return error;
}

static void MysteryNet_Idle(MysteryNetSeq *seq, int *state, void *work) {
}

static void MysteryNet_WirelessStart(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;

    switch (*state) {
    case 0:
        func_020425ec(&sNetInit, NULL, net);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
        *state = 1;
        break;
    case 1:
        if (func_02042788()) {
            func_02042ba8(TRUE, net->heapId);
            net->wifiWork = func_02012908(4, net->heapId);
            func_02042968();
            *state = 2;
        }
        break;
    case 2:
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
        MysteryNetSeq_SetNext(seq, MysteryNet_WirelessReady);
        break;
    }
}

static void MysteryNet_WirelessEnd(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;

    switch (*state) {
    case 0:
        func_02012994(net->wifiWork);
        net->wifiWork = NULL;
        *state = 1;
        break;
    case 1:
        if (func_02042860(MysteryNet_OnExit)) {
            *state = 2;
        } else {
            *state = 3;
        }
        break;
    case 2:
        if (net->exited) {
            net->exited = FALSE;
            *state = 3;
        }
        break;
    case 3:
        MysteryNetSeq_SetNext(seq, MysteryNet_Idle);
        break;
    }
}

static void MysteryNet_WirelessReady(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;

    func_02012a4c();
    net->beaconFlags = func_02012be4(NULL);
}

static void MysteryNet_BeaconStart(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;
    DeliveryInit init;

    switch (*state) {
    case 0:
        sys_memset(&init, 0, sizeof(DeliveryInit));
        init.code = 9;
        init.data[0].datasize = sizeof(MysteryGiftRecvData);
        init.data[0].pData = net->buffer;
        init.data[0].region = region;
        init.data[0].mask = 1 << GAME_VERSION;
        init.dataNum = 1;
        init.flag4 = 0;
        init.heapId = net->heapId;
        net->beacon = func_ov012_02152990(&init);
        func_ov012_02152b64(net->beacon);
        GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, FALSE);
        net->result = MYSTERY_NET_RECV_NONE;
        *state = 1;
        break;
    case 1:
        if (func_02042788()) {
            func_02042ba8(TRUE, net->heapId);
            GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
            *state = 2;
        }
        break;
    case 2:
        MysteryNetSeq_SetNext(seq, MysteryNet_BeaconWait);
        break;
    }
}

static void MysteryNet_BeaconEnd(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;

    switch (*state) {
    case 0:
        func_ov012_02152bfc(net->beacon);
        *state = 1;
        break;
    case 1:
        if (func_02042ab8()) {
            *state = 2;
        }
        break;
    case 2:
        MysteryNetSeq_SetNext(seq, MysteryNet_Idle);
        break;
    }
}

static void MysteryNet_BeaconWait(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;

    func_ov012_02152bec(net->beacon);
    if (func_ov012_02152bd4(net->beacon)) {
        net->result = MYSTERY_NET_RECV_OK;
        MysteryNetSeq_SetNext(seq, MysteryNet_BeaconEnd);
    }
}

static void MysteryNet_IrcStart(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;
    DeliveryIrcInit init;

    switch (*state) {
    case 0:
        sys_memset(&init, 0, sizeof(DeliveryIrcInit));
        init.code = 9;
        init.data[0].datasize = sizeof(MysteryGiftRecvData);
        init.data[0].pData = net->buffer;
        init.unk04 = 0;
        init.heapId = net->heapId;
        init.data[0].region = region;
        init.dataNum = 1;
        init.data[0].mask = 1 << GAME_VERSION;
        net->irc = func_ov012_0215309c(&init);
        func_ov012_021530f8(net->irc);
        net->result = MYSTERY_NET_RECV_NONE;
        *state = 1;
        break;
    case 1:
        if (func_02042788()) {
            func_02042ba8(TRUE, net->heapId);
            *state = 2;
        }
        break;
    case 2:
        MysteryNetSeq_SetNext(seq, MysteryNet_IrcWait);
        break;
    }
}

static void MysteryNet_IrcEnd(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;

    switch (*state) {
    case 0:
        func_ov012_02153150(net->irc);
        *state = 1;
        break;
    case 1:
        if (func_02042ab8()) {
            *state = 2;
        }
        break;
    case 2:
        MysteryNetSeq_SetNext(seq, MysteryNet_Idle);
        break;
    }
}

static void MysteryNet_IrcWait(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;
    u8 ret;

    func_ov012_0215313c(net->irc);
    ret = func_ov012_02153130(net->irc);
    if (ret != 0) {
        switch (ret) {
        case 1:
            net->result = MYSTERY_NET_RECV_OK;
            break;
        case 2:
        case 3:
            net->result = MYSTERY_NET_RECV_ERROR;
            break;
        }
        MysteryNetSeq_SetNext(seq, MysteryNet_IrcEnd);
    }
}

static void MysteryNet_WifiDownload(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;
    MysteryNetDownload *download = &net->download;
    int i;

    switch (*state) {
    case 0:
        DWCRap_SetEventFunc(MysteryNet_OnDwcEvent, download);
        sys_memset(download, 0, sizeof(MysteryNetDownload));
        download->cancel = FALSE;
        download->cancelled = FALSE;
        download->selected = 0;
        download->buffer = net->buffer;
        net->result = MYSTERY_NET_RECV_NONE;
        if (!func_ov189_021a5674(MysteryNet_OnNdEvent, "IRAO", "WX9x7Zh6J3aBC4zQ")) {
            net->result = MYSTERY_NET_RECV_ERROR;
            *state = 9;
            return;
        }
        download->initialized = TRUE;
        MysteryNetDownload_Wait(download, state, 1);
        break;
    case 1:
        if (download->cancelled) {
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 3, state, 7, 7);
            return;
        }
        if (!func_ov189_021a5830("MYSTERY_E", "", "")) {
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 4, state, 9, 9);
            return;
        }
        *state = 3;
        break;
    case 3:
        if (!func_ov189_021a5850(download->files, 0, MYSTERY_NET_FILE_MAX)) {
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 4, state, 9, 9);
            return;
        }
        MysteryNetDownload_Wait(download, state, 2);
        break;
    case 2:
        for (i = 0; i < MYSTERY_NET_FILE_MAX; i++) {
            if (!Mystery_IsZero((u32 *)&download->files[i], sizeof(DWCNdFileInfo))) {
                if (Mystery_ParseHex(download->files[i].param2) & (1 << GAME_VERSION)) {
                    download->available[i] = TRUE;
                } else {
                    download->available[i] = FALSE;
                }
            } else {
                download->available[i] = FALSE;
            }
        }
        download->count = 0;
        for (i = 0; i < MYSTERY_NET_FILE_MAX; i++) {
            if (download->available[i]) {
                download->count++;
            }
        }
        *state = 4;
        break;
    case 4:
        if (download->count == 1) {
            for (i = 0; i < MYSTERY_NET_FILE_MAX; i++) {
                if (download->available[i]) {
                    download->selected = i;
                    break;
                }
            }
        } else if (download->count > 1) {
            u32 n = 0;
            u32 pick = GFL_RandomMTRange(download->count);

            for (i = 0; i < MYSTERY_NET_FILE_MAX; i++) {
                if (download->available[i]) {
                    if (n == pick) {
                        download->selected = i;
                        break;
                    }
                    n++;
                }
            }
            download->selected = i;
        } else {
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 1, state, 8, 8);
            return;
        }
        *state = 5;
        break;
    case 5:
        if (!func_ov189_021a58c8(&download->files[download->selected], download->buffer, MYSTERY_NET_BUFFER_SIZE)) {
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 4, state, 9, 9);
            return;
        }
        download->percent = 0;
        sNdState.ready = FALSE;
        sNdState.error = 0;
        download->timeout = 0;
        *state = 6;
        break;
    case 6:
        func_ov189_021a5768();
        if (download->timeout++ > 7200) {
            func_020424ac(0, 0, 0, 1013);
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 3, state, 7, 7);
        }
        if (!sNdState.ready) {
            if (download->cancel) {
                net->result = MYSTERY_NET_RECV_ERROR;
                MysteryNetDownload_Cleanup(download, 3, state, 7, 7);
                return;
            }
            if (func_ov189_021a5980(&download->received, &download->contentLength) == TRUE) {
                u32 percent = download->received * 100 / download->contentLength;

                if (download->percent != percent) {
                    download->percent = percent;
                }
            }
        } else if (sNdState.error != 0) {
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 4, state, 9, 9);
        } else if (!download->cancelled) {
            net->result = MYSTERY_NET_RECV_OK;
            MysteryNetDownload_Cleanup(download, 1, state, 8, 8);
        } else {
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 3, state, 8, 8);
        }
        break;
    case 7:
        func_ov189_021a5938();
        *state = 8;
        break;
    case 8:
        *state = 9;
        break;
    case 9:
        download->initialized = FALSE;
        DWCRap_SetEventFunc(NULL, NULL);
        MysteryNetSeq_SetNext(seq, MysteryNet_WifiEnd);
        break;
    case DOWNLOAD_WAIT:
        func_ov189_021a5768();
        if (download->timeout++ > 7200) {
            func_020424ac(0, 0, 0, 1013);
            net->result = MYSTERY_NET_RECV_ERROR;
            MysteryNetDownload_Cleanup(download, 3, state, 9, 9);
        }
        if (sNdState.ready) {
            sNdState.ready = FALSE;
            if (sNdState.error != 0) {
                net->result = MYSTERY_NET_RECV_ERROR;
                MysteryNetDownload_Cleanup(download, 3, state, 9, 9);
                return;
            }
            *state = download->nextState;
            return;
        }
        if (download->cancel) {
            download->cancelled = TRUE;
        }
        break;
    case DOWNLOAD_WAIT_CLEANUP:
        func_ov189_021a5768();
        if (download->timeout++ > 7200) {
            func_020424ac(0, 0, 0, 1013);
            *state = download->nextState;
        }
        if (sNdState.done) {
            sNdState.done = FALSE;
            *state = download->nextState;
            return;
        }
        if (download->cancel) {
            download->cancelled = TRUE;
        }
        break;
    }
}

static void MysteryNet_WifiEnd(MysteryNetSeq *seq, int *state, void *work) {
    MysteryNet *net = work;

    switch (*state) {
    case 0:
        if (func_02042860(NULL)) {
            *state = 1;
        } else {
            *state = 2;
        }
        break;
    case 1:
        if (!func_02042788()) {
            net->exited = FALSE;
            *state = 2;
        }
        break;
    case 2:
        MysteryNetSeq_SetNext(seq, MysteryNet_Idle);
        break;
    }
}

static void MysteryNetDownload_Wait(MysteryNetDownload *download, int *state, u32 nextState) {
    sNdState.ready = FALSE;
    sNdState.error = 0;
    download->nextState = nextState;
    download->timeout = 0;
    *state = DOWNLOAD_WAIT;
}

// Ends the download library, then goes on to nextState, or to failState at once if it can't
static void MysteryNetDownload_Cleanup(MysteryNetDownload *download, u32 error, int *state, u32 nextState,
                                       int failState) {
    sNdState.done = FALSE;
    sNdState.error = error;
    download->nextState = nextState;
    download->timeout = 0;
    *state = DOWNLOAD_WAIT_CLEANUP;
    if (!func_ov189_021a57dc()) {
        *state = failState;
    }
}

static void MysteryNetSeq_Init(MysteryNetSeq *seq, void *work, MysteryNetSeqFunc func) {
    sys_memset(seq, 0, sizeof(MysteryNetSeq));
    seq->work = work;
    MysteryNetSeq_SetNext(seq, func);
}

static void MysteryNetSeq_Exit(MysteryNetSeq *seq) {
    sys_memset(seq, 0, sizeof(MysteryNetSeq));
}

static void MysteryNetSeq_Main(MysteryNetSeq *seq) {
    if (!seq->end) {
        seq->func(seq, &seq->state, seq->work);
    }
}

static void MysteryNetSeq_SetNext(MysteryNetSeq *seq, MysteryNetSeqFunc func) {
    seq->func = func;
    seq->state = 0;
}

static BOOL MysteryNetSeq_IsCurrent(MysteryNetSeq *seq, MysteryNetSeqFunc func) {
    if (seq->func == func) {
        return TRUE;
    }
    return FALSE;
}

static void *MysteryNet_GetInfo(void *work) {
    MysteryNet *net = work;

    return net->info;
}

static int MysteryNet_GetInfoSize(void *work) {
    return sizeof(((MysteryNet *)NULL)->info);
}

static BOOL MysteryNet_IsSameInfo(u32 a, u32 b) {
    if (a == b) {
        return TRUE;
    }
    return FALSE;
}

static void MysteryNet_OnExit(void *work) {
    MysteryNet *net = work;

    net->exited = TRUE;
}

static void MysteryNet_OnNdEvent(u32 reason, u32 error) {
    sNdState.ready = TRUE;
    sNdState.error = error;
    MysteryNet_SetNdDone();
}

static void MysteryNet_SetNdDone(void) {
    sNdState.done = TRUE;
}

static u32 Mystery_ParseHex(const char *str) {
    u32 i;
    s32 digit;
    u32 value = 0;

    while (TRUE) {
        digit = -1;
        for (i = 0; i < NELEMS(sHexDigits); i++) {
            if (str[0] == sHexDigits[i].c) {
                digit = sHexDigits[i].value;
                break;
            }
        }
        if (digit == -1) {
            break;
        }
        value <<= 4;
        value += digit;
        str++;
    }
    return value;
}

static BOOL Mystery_IsZero(const u32 *data, u32 size) {
    u32 i;

    for (i = 0; i < size / 4; i++) {
        if (data[i] != 0) {
            return FALSE;
        }
    }
    return TRUE;
}

static u32 MysteryNet_OnDwcEvent(void *work, int a1, int event, int a3) {
    MysteryNetDownload *download = work;

    switch (event) {
    case 3:
    case 5:
    case 6:
        return download->initialized;
    }
    return 0;
}
