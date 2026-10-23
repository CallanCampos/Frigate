#include "types.h"
#include "app/mb_parent.h"
#include "app/unova_link.h"
#include "dwc/dwc.h"
#include "gfl/dwc_rap.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/net_state.h"
#include "gfl/net_system.h"
#include "gfl/nhttp_rap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "save/player_info.h"
#include "system/game_data.h"

// Unova Link's connections: the local wireless one of the Key System, the link of overlay 181 and Wi-Fi, each run as a
// sequence of steps that the other files request

#define NET_BUFFER_SIZE 0x100
#define NET_COMMAND_BASE 0x3700
// Frames before the wireless connection gives up
#define NET_TIMEOUT 3600

struct KeySystemNet {
    void *buffer;
    // KEY_SYSTEM_NET_MODE_*
    u32 mode;
    GameData *gameData;
    u8 received[NET_BUFFER_SIZE];
    KeySystemNetRequest request;
    BOOL hasReceived;
    KeySystemSeq *seq;
    u32 count;
    u32 timer;
    NHttpRap *http;
    MBDataConv *ov181Work;
    u16 heapId;
    BOOL error;
    u32 errorCode;
    BOOL cancel;
    BOOL noErrorCheck;
    void (*errorCallback)(void *work);
    void *errorCallbackWork;
};

static void KeySystemNet_SeqIdle(KeySystemSeq *seq, int *state, void *work);
static void KeySystemNet_SeqConnect(KeySystemSeq *seq, int *state, void *work);
static void KeySystemNet_SeqDisconnect(KeySystemSeq *seq, int *state, void *work);
static void KeySystemNet_SeqConnected(KeySystemSeq *seq, int *state, void *work);
static void KeySystemNet_SeqSend(KeySystemSeq *seq, int *state, void *work);
static void KeySystemNet_SeqCancel(KeySystemSeq *seq, int *state, void *work);
static void KeySystemNet_SeqSync(KeySystemSeq *seq, int *state, void *work);
static void KeySystemNet_SeqOv181Start(KeySystemSeq *seq, int *state, void *work);
static void KeySystemNet_SeqOv181End(KeySystemSeq *seq, int *state, void *work);
static void KeySystemNet_SeqWifiPost(KeySystemSeq *seq, int *state, void *work);
static void KeySystemNet_SeqWifiGet(KeySystemSeq *seq, int *state, void *work);
static void *KeySystemNet_GetBeacon(void *work);
static int KeySystemNet_GetBeaconSize(void *work);
static BOOL KeySystemNet_CheckBeacon(u32 gameId, u32 value);
static void KeySystemNet_OnConnect(void *work);
static void KeySystemNet_OnStart(void *work, BOOL a1);
static void KeySystemNet_OnDisconnect(void *work);
static void KeySystemNet_Receive(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL KeySystemNet_CanRequest(KeySystemNet *net, unsigned int request);
static void KeySystemNet_Stop(KeySystemNet *net);
static void KeySystemNet_OnError(KeySystemNet *net);
static void KeySystemNet_OnWifiError(void *work, int a1, int code, int error);

static const NetCommand sKeySystemNetCommands[] = {
    { KeySystemNet_Receive, NULL },
};

// The requests that each mode allows
static const u8 sModeRequests[KEY_SYSTEM_NET_MODE_COUNT][KEY_SYSTEM_NET_REQUEST_COUNT] = {
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE },
    { TRUE, TRUE, TRUE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE },
    { FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, FALSE, FALSE },
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE },
};

// The requests that each step allows, in KeySystemNet_GetState's order
static const u8 sStateRequests[KEY_SYSTEM_NET_STATE_COUNT][KEY_SYSTEM_NET_REQUEST_COUNT] = {
    { TRUE, FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, TRUE, TRUE },
    { FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE },
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE },
    { FALSE, TRUE, FALSE, TRUE, TRUE, FALSE, FALSE, FALSE, FALSE },
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE },
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE },
    // Canceling allows a second cancel
    { FALSE, FALSE, TRUE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE },
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE },
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE },
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE },
    { FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, FALSE },
};

static GFLNetInitData sNetInitData = {
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    KeySystemNet_GetBeacon,
    KeySystemNet_GetBeaconSize,
    KeySystemNet_CheckBeacon,
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
    0x10,
    0xf0,
    0,
    2,
    0x64,
    0x10,
    1,
    0,
    GFL_NET_TYPE_IRC_WIRELESS,
    1,
    NET_COMMAND_BASE >> 8,
    { 0xfe, 0xff, 0, 0 },
    0,
    0,
};

static u32 sBeacon;

KeySystemNet *KeySystemNet_Create(GameData **gameData, HeapID heapId) {
    KeySystemNet *net = GFL_HeapAllocate(heapId, sizeof(KeySystemNet), TRUE, "key_system_net.c", 240);

    net->gameData = *gameData;
    net->heapId = heapId;
    net->seq = KeySystemSeq_Create(1, net, KeySystemNet_SeqIdle, heapId);
    return net;
}

void KeySystemNet_Free(KeySystemNet *net) {
    KeySystemNet_Stop(net);
    KeySystemSeq_Free(net->seq);
    GFL_HeapFree(net);
}

void KeySystemNet_Update(KeySystemNet *net) {
    KeySystemSeq_Run(net->seq);
    if (net->mode == KEY_SYSTEM_NET_MODE_OV181 && net->ov181Work != NULL) {
        MBDataConv_Update(net->ov181Work);
    }
}

void KeySystemNet_SetMode(KeySystemNet *net, u32 mode) {
    if (net->mode != mode) {
        KeySystemNet_Stop(net);
        net->mode = mode;
        switch (mode) {
        case KEY_SYSTEM_NET_MODE_WIRELESS:
            break;
        case KEY_SYSTEM_NET_MODE_OV181:
            GFL_OvlLoad(OVERLAY_ID(181));
            net->ov181Work = MBDataConv_Create(1);
            break;
        case KEY_SYSTEM_NET_MODE_WIFI:
            GFL_OvlLoad(OVERLAY_ID(189));
            DWCRap_SetErrorFunc(KeySystemNet_OnWifiError, net);
            break;
        }
    }
}

void KeySystemNet_Request(KeySystemNet *net, u32 request, const void *params) {
    KeySystemSeqFunc func;

    KeySystemNet_CanRequest(net, request);
    if (params != NULL) {
        // BUG: The parameters are smaller than the union, so this reads past them on the caller's stack
        MI_CpuCopy8(params, &net->request, sizeof(KeySystemNetRequest));
    }
    switch (request) {
    case KEY_SYSTEM_NET_REQUEST_CONNECT:
        func = KeySystemNet_SeqConnect;
        break;
    case KEY_SYSTEM_NET_REQUEST_DISCONNECT:
        func = KeySystemNet_SeqDisconnect;
        break;
    case KEY_SYSTEM_NET_REQUEST_CANCEL:
        net->cancel = TRUE;
        return;
    case KEY_SYSTEM_NET_REQUEST_SEND:
        func = KeySystemNet_SeqSend;
        break;
    case KEY_SYSTEM_NET_REQUEST_SYNC:
        func = KeySystemNet_SeqSync;
        break;
    case KEY_SYSTEM_NET_REQUEST_OV181_START:
        func = KeySystemNet_SeqOv181Start;
        break;
    case KEY_SYSTEM_NET_REQUEST_OV181_END:
        func = KeySystemNet_SeqOv181End;
        break;
    case KEY_SYSTEM_NET_REQUEST_WIFI_POST:
        func = KeySystemNet_SeqWifiPost;
        break;
    case KEY_SYSTEM_NET_REQUEST_WIFI_GET:
        func = KeySystemNet_SeqWifiGet;
        break;
    default:
        return;
    }
    KeySystemSeq_Set(net->seq, func);
}

u32 KeySystemNet_GetState(KeySystemNet *net) {
    if (KeySystemSeq_IsCurrent(net->seq, KeySystemNet_SeqIdle)) {
        return KEY_SYSTEM_NET_STATE_IDLE;
    }
    if (KeySystemSeq_IsCurrent(net->seq, KeySystemNet_SeqConnect)) {
        return KEY_SYSTEM_NET_STATE_CONNECTING;
    }
    if (KeySystemSeq_IsCurrent(net->seq, KeySystemNet_SeqDisconnect)) {
        return KEY_SYSTEM_NET_STATE_DISCONNECTING;
    }
    if (KeySystemSeq_IsCurrent(net->seq, KeySystemNet_SeqConnected)) {
        return KEY_SYSTEM_NET_STATE_CONNECTED;
    }
    if (KeySystemSeq_IsCurrent(net->seq, KeySystemNet_SeqSend)) {
        return KEY_SYSTEM_NET_STATE_SENDING;
    }
    if (KeySystemSeq_IsCurrent(net->seq, KeySystemNet_SeqSync)) {
        return KEY_SYSTEM_NET_STATE_SYNCING;
    }
    if (KeySystemSeq_IsCurrent(net->seq, KeySystemNet_SeqCancel)) {
        return KEY_SYSTEM_NET_STATE_CANCELING;
    }
    if (KeySystemSeq_IsCurrent(net->seq, KeySystemNet_SeqOv181Start)) {
        return KEY_SYSTEM_NET_STATE_OV181;
    }
    if (KeySystemSeq_IsCurrent(net->seq, KeySystemNet_SeqOv181End)) {
        return KEY_SYSTEM_NET_STATE_OV181_END;
    }
    if (KeySystemSeq_IsCurrent(net->seq, KeySystemNet_SeqWifiPost)) {
        return KEY_SYSTEM_NET_STATE_WIFI_POST;
    }
    if (KeySystemSeq_IsCurrent(net->seq, KeySystemNet_SeqWifiGet)) {
        return KEY_SYSTEM_NET_STATE_WIFI_GET;
    }
    return KEY_SYSTEM_NET_STATE_IDLE;
}

BOOL KeySystemNet_GetReceived(KeySystemNet *net, void *dest, u32 size) {
    if (net->hasReceived) {
        if (dest != NULL) {
            MI_CpuCopy8(net->received, dest, size);
        }
        return TRUE;
    }
    return FALSE;
}

u32 KeySystemNet_CheckError(KeySystemNet *net) {
    if (GameData_CheckEventsPaused(net->gameData)) {
        return KEY_SYSTEM_NET_ERROR_NONE;
    }
    if (net->noErrorCheck) {
        return KEY_SYSTEM_NET_ERROR_NONE;
    }
    switch (net->mode) {
    case KEY_SYSTEM_NET_MODE_WIRELESS:
    case KEY_SYSTEM_NET_MODE_OV181:
        if (net->error) {
            GFL_NetErrRecordWifi(14);
            KeySystemNet_OnError(net);
            GFL_NetErrShowScreen();
            return KEY_SYSTEM_NET_ERROR;
        }
        if (GFL_NetErrCheck()) {
            KeySystemNet_OnError(net);
            GFL_NetErrShowScreen();
            return KEY_SYSTEM_NET_ERROR;
        }
        break;
    case KEY_SYSTEM_NET_MODE_WIFI:
        if (net->error) {
            GFL_NetErrRecordWifi(net->errorCode);
            KeySystemNet_OnError(net);
            GFL_NetErrShowScreen();
            return KEY_SYSTEM_NET_ERROR;
        }
        if (GFL_NetErrCheck()) {
            switch (NetState_GetErrorInfo()->errorType) {
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
                KeySystemNet_OnError(net);
                GFL_NetErrShowScreen();
                return KEY_SYSTEM_NET_ERROR;
            case 7:
                GFL_NetErrAbort();
                return KEY_SYSTEM_NET_ERROR;
            }
        }
        break;
    }
    return KEY_SYSTEM_NET_ERROR_NONE;
}

void KeySystemNet_Reset(KeySystemNet *net) {
    KeySystemNet_Stop(net);
    switch (net->mode) {
    case KEY_SYSTEM_NET_MODE_WIRELESS:
    case KEY_SYSTEM_NET_MODE_OV181:
        GFL_NetIwFree();
        GFL_NetErrShutdown();
        GFL_NetErrClear();
        KeySystemSeq_Set(net->seq, KeySystemNet_SeqIdle);
        break;
    case KEY_SYSTEM_NET_MODE_WIFI:
        GFL_NetErrShutdown();
        GFL_NetErrClear();
        NetState_ClearWifiError();
        DWC_ClearError();
        KeySystemSeq_Set(net->seq, KeySystemNet_SeqIdle);
        break;
    }
    net->error = FALSE;
    net->errorCode = 0;
    net->mode = KEY_SYSTEM_NET_MODE_NONE;
}

void KeySystemNet_SetBuffer(KeySystemNet *net, void *buffer) {
    net->buffer = buffer;
}

void KeySystemNet_SetNoErrorCheck(KeySystemNet *net, BOOL noErrorCheck) {
    net->noErrorCheck = noErrorCheck;
}

void KeySystemNet_SetErrorCallback(KeySystemNet *net, void (*callback)(void *work), void *work) {
    net->errorCallback = callback;
    net->errorCallbackWork = work;
}

static void KeySystemNet_SeqIdle(KeySystemSeq *seq, int *state, void *work) {
}

static void KeySystemNet_SeqConnect(KeySystemSeq *seq, int *state, void *work) {
    KeySystemNet *net = work;

    KeySystemNet_GetRequest(net);
    switch (*state) {
    case 0:
        net->count = 0;
        net->cancel = FALSE;
        GFL_NetInit(&sNetInitData, NULL, net);
        net->timer = 0;
        (*state)++;
        break;
    case 1:
        if (net->cancel) {
            KeySystemSeq_Set(seq, KeySystemNet_SeqCancel);
            return;
        }
        if (GFL_NetIsInitialized()) {
            GFL_NetSetIconScreen(TRUE, HEAPID_KEY_SYSTEM);
            GFL_NetIwChangeoverConnect(KeySystemNet_OnConnect, KeySystemNet_OnStart, KeySystemNet_OnDisconnect);
            (*state)++;
        }
        break;
    case 2:
        if (net->cancel) {
            KeySystemSeq_Set(seq, KeySystemNet_SeqCancel);
            return;
        }
        if (net->timer++ >= NET_TIMEOUT) {
            net->timer = 0;
            GFL_NetIwFree();
            GFL_NetExit(NULL);
            *state = 6;
        }
        break;
    case 3:
        net->timer = 0;
        NetCommand_AddTable(NET_COMMAND_BASE, sKeySystemNetCommands, 1, net);
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 10, NET_COMMAND_BASE >> 8);
        GflNet_SetAutoExit(TRUE);
        GFL_NetSetDisconnectError(TRUE);
        (*state)++;
        break;
    case 4:
        if (net->timer++ >= NET_TIMEOUT) {
            net->timer = 0;
            NetCommand_RemoveTable(NET_COMMAND_BASE);
            GFL_NetIwFree();
            GFL_NetExit(NULL);
            *state = 6;
        } else if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 10, NET_COMMAND_BASE >> 8)) {
            (*state)++;
        }
        break;
    case 5:
        KeySystemSeq_Set(seq, KeySystemNet_SeqConnected);
        break;
    case 6:
        if (GFL_NetIsEnded()) {
            KeySystemSeq_Set(seq, KeySystemNet_SeqIdle);
        }
        break;
    }
}

static void KeySystemNet_SeqDisconnect(KeySystemSeq *seq, int *state, void *work) {
    switch (*state) {
    case 0:
        if (GFL_NetGetNegotiatedCount() > 0) {
            NetHandle_SendTimingPair(NetHandle_GetOwn(), 13, NET_COMMAND_BASE >> 8);
            (*state)++;
        } else {
            *state = 2;
        }
        break;
    case 1:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 13, NET_COMMAND_BASE >> 8)) {
            GflNet_SetAutoExit(FALSE);
            GFL_NetSetDisconnectError(FALSE);
            (*state)++;
        }
        break;
    case 2:
        NetCommand_RemoveTable(NET_COMMAND_BASE);
        GFL_NetIwFree();
        GFL_NetExit(NULL);
        (*state)++;
        break;
    case 3:
        if (GFL_NetIsEnded()) {
            (*state)++;
        }
        break;
    case 4:
        KeySystemSeq_Set(seq, KeySystemNet_SeqIdle);
        break;
    }
}

static void KeySystemNet_SeqConnected(KeySystemSeq *seq, int *state, void *work) {
}

static void KeySystemNet_SeqSend(KeySystemSeq *seq, int *state, void *work) {
    KeySystemNet *net = work;
    KeySystemNetRequest *request = KeySystemNet_GetRequest(net);

    switch (*state) {
    case 0:
        net->hasReceived = FALSE;
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 11, NET_COMMAND_BASE >> 8);
        (*state)++;
        break;
    case 1:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 11, NET_COMMAND_BASE >> 8)) {
            (*state)++;
        }
        break;
    case 2:
        if (GFL_NetSendCommand(NetHandle_GetOwn(), NET_COMMAND_BASE, request->send.size, request->send.data)) {
            (*state)++;
        }
        break;
    case 3:
        if (net->hasReceived) {
            (*state)++;
        }
        break;
    case 4:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), 12, NET_COMMAND_BASE >> 8);
        (*state)++;
        break;
    case 5:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), 12, NET_COMMAND_BASE >> 8)) {
            (*state)++;
        }
        break;
    case 6:
        KeySystemSeq_Set(seq, KeySystemNet_SeqConnected);
        break;
    }
}

static void KeySystemNet_SeqCancel(KeySystemSeq *seq, int *state, void *work) {
    switch (*state) {
    case 0:
        if (GFL_NetIsInitialized()) {
            GFL_NetExit(NULL);
            *state = 1;
        } else {
            *state = 2;
        }
        break;
    case 1:
        if (GFL_NetIsEnded()) {
            *state = 2;
        }
        break;
    case 2:
        KeySystemSeq_Set(seq, KeySystemNet_SeqIdle);
        break;
    }
}

static void KeySystemNet_SeqSync(KeySystemSeq *seq, int *state, void *work) {
    KeySystemNet *net = work;

    switch (*state) {
    case 0:
        NetHandle_SendTimingPair(NetHandle_GetOwn(), (u8)(net->count + 20), NET_COMMAND_BASE >> 8);
        (*state)++;
        break;
    case 1:
        if (NetHandle_IsTimingPairRecv(NetHandle_GetOwn(), (u8)(net->count + 20), NET_COMMAND_BASE >> 8)) {
            (*state)++;
        }
        break;
    case 2:
        net->count++;
        (*state)++;
        break;
    case 3:
        KeySystemSeq_Set(seq, KeySystemNet_SeqConnected);
        break;
    }
}

static void KeySystemNet_SeqOv181Start(KeySystemSeq *seq, int *state, void *work) {
    KeySystemNet *net = work;
    KeySystemNetRequest *request = KeySystemNet_GetRequest(net);

    switch (*state) {
    case 0:
        request->ov181.result = 0;
        MBDataConv_SetGameInfo(net->ov181Work, request->ov181.text, request->ov181.title);
        (*state)++;
        break;
    case 1:
        MBDataConv_Request(net->ov181Work, MB_DATACONV_REQUEST_DISTRIBUTE);
        (*state)++;
        break;
    case 2:
        if (MBDataConv_IsIdle(net->ov181Work)) {
            request->ov181.result = 1;
            *state = 4;
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            MBDataConv_Request(net->ov181Work, MB_DATACONV_REQUEST_CANCEL);
            *state = 3;
        }
        break;
    case 3:
        if (MBDataConv_IsIdle(net->ov181Work)) {
            request->ov181.result = 2;
            *state = 4;
        }
        break;
    case 4:
        KeySystemSeq_Set(seq, KeySystemNet_SeqIdle);
        break;
    }
}

static void KeySystemNet_SeqOv181End(KeySystemSeq *seq, int *state, void *work) {
    KeySystemNet *net = work;
    KeySystemNetRequest *request = KeySystemNet_GetRequest(net);

    switch (*state) {
    case 0:
        request->ov181End.done = FALSE;
        MBDataConv_Request(net->ov181Work, MB_DATACONV_REQUEST_CONNECT);
        (*state)++;
        break;
    case 1:
        if (MBDataConv_IsIdle(net->ov181Work)) {
            (*state)++;
        }
        break;
    case 2:
        request->ov181End.done = TRUE;
        request->ov181End.result = MBDataConv_GetResult(net->ov181Work);
        request->ov181End.data = MBDataConv_GetReceivedData(net->ov181Work);
        KeySystemSeq_Set(seq, KeySystemNet_SeqIdle);
        break;
    }
}

static void KeySystemNet_SeqWifiPost(KeySystemSeq *seq, int *state, void *work) {
    KeySystemNet *net = work;
    KeySystemNetRequest *request = KeySystemNet_GetRequest(net);
    int status;
    void *response;

    switch (*state) {
    case 0:
        net->hasReceived = FALSE;
        request->wifi.result = 0;
        net->http =
            NHttpRap_Create(net->heapId, PlayerInfo_GetProfileId(GetGameDataPlayerInfo(net->gameData)), net->buffer);
        NHttpRap_SetAnswerBuffer(net->http, request->wifi.buffer, request->wifi.size);
        if (NHttpRap_SendRequestWithId(10, request->wifi.id, net->http)) {
            *state = 1;
        }
        net->timer = 0;
        break;
    case 1:
        if (!NHttpRap_StartRequest(net->http)) {
            *state = 2;
        } else {
            GFL_ASSERT(0);
            request->wifi.result = 1;
            *state = 3;
        }
        break;
    case 2:
        status = NHttpRap_GetStatus(net->http);
        switch (status) {
        case 503:
            net->error = TRUE;
            net->errorCode = 0x3e;
            request->wifi.result = 2;
            *state = 3;
            return;
        case 502:
            net->error = TRUE;
            net->errorCode = 0x3f;
            request->wifi.result = 2;
            *state = 3;
            return;
        }
        if (status >= 400) {
            net->error = TRUE;
            net->errorCode = 0x3c;
            request->wifi.result = 2;
            *state = 3;
        } else {
            status = NHttpRap_Poll(net->http);
            if (status == 0) {
                response = NHttpRap_GetAnswer(net->http);
                switch (*(int *)response) {
                case 0:
                    *state = 3;
                    request->wifi.result = 4;
                    net->hasReceived = TRUE;
                    break;
                case 8:
                    *state = 3;
                    request->wifi.result = 6;
                    break;
                case 10:
                    *state = 3;
                    request->wifi.result = 5;
                    break;
                case 5:
                    *state = 3;
                    request->wifi.result = 7;
                    break;
                default:
                    net->error = TRUE;
                    net->errorCode = GFL_NetErrGetServerMessage(*(int *)response);
                    request->wifi.result = 3;
                    *state = 3;
                    break;
                }
            } else if (status != 15) {
                request->wifi.result = 1;
                *state = 3;
            }
            if (net->timer++ > 36000) {
                net->error = 2;
                net->errorCode = 13;
                *state = 3;
            }
        }
        break;
    case 3:
        NHttpRap_ResetAnswerBuffer(net->http);
        if (net->http != NULL) {
            NHttpRap_EndRequest(net->http);
            NHttpRap_Destroy(net->http);
            net->http = NULL;
        }
        KeySystemSeq_Set(seq, KeySystemNet_SeqIdle);
        break;
    }
}

static void KeySystemNet_SeqWifiGet(KeySystemSeq *seq, int *state, void *work) {
    KeySystemNet *net = work;
    KeySystemNetRequest *request = KeySystemNet_GetRequest(net);
    int *response;
    int status;

    switch (*state) {
    case 0:
        net->hasReceived = FALSE;
        request->wifiGet.result = 0;
        net->http =
            NHttpRap_Create(net->heapId, PlayerInfo_GetProfileId(GetGameDataPlayerInfo(net->gameData)), net->buffer);
        if (NHttpRap_SendRequest(0, net->http)) {
            *state = 1;
        }
        break;
    case 1:
        if (!NHttpRap_StartRequest(net->http)) {
            *state = 2;
        } else {
            request->wifiGet.result = 1;
            *state = 3;
        }
        break;
    case 2:
        status = NHttpRap_Poll(net->http);
        if (status == 0) {
            response = NHttpRap_GetAnswer(net->http);
            switch (NHttpRap_GetStatus(net->http)) {
            case 503:
                net->error = TRUE;
                net->errorCode = 0x3e;
                request->wifiGet.result = 2;
                *state = 3;
                return;
            case 502:
                net->error = TRUE;
                net->errorCode = 0x3f;
                request->wifiGet.result = 2;
                *state = 3;
                return;
            }
            if (*response == 0) {
                // Never read, but the original sets it
                u32 unused[2] = { 0, 0 };

                // The value is 0x80 bytes into the response
                response += 0x20;
                request->wifiGet.value = *(u16 *)response;
                *state = 3;
                request->wifiGet.result = 4;
            } else {
                net->error = TRUE;
                net->errorCode = GFL_NetErrGetServerMessage(*response);
                request->wifiGet.result = 3;
                *state = 3;
            }
        } else if (status != 15) {
            request->wifiGet.result = 1;
            *state = 3;
        }
        break;
    case 3:
        if (net->http != NULL) {
            NHttpRap_EndRequest(net->http);
            NHttpRap_Destroy(net->http);
            net->http = NULL;
        }
        KeySystemSeq_Set(seq, KeySystemNet_SeqIdle);
        break;
    }
}

static void *KeySystemNet_GetBeacon(void *work) {
    return &sBeacon;
}

static int KeySystemNet_GetBeaconSize(void *work) {
    return sizeof(sBeacon);
}

static BOOL KeySystemNet_CheckBeacon(u32 gameId, u32 value) {
    if (gameId == value) {
        return TRUE;
    }
    return FALSE;
}

static void KeySystemNet_OnConnect(void *work) {
    KeySystemNet *net = work;

    if (net->timer < NET_TIMEOUT) {
        KeySystemSeq_AddState(net->seq, 1);
    }
}

static void KeySystemNet_OnStart(void *work, BOOL a1) {
}

static void KeySystemNet_OnDisconnect(void *work) {
    KeySystemNetRequest *request = KeySystemNet_GetRequest(work);

    if (request->callback.func != NULL) {
        request->callback.func(request->callback.arg);
    }
}

static void KeySystemNet_Receive(int netId, int size, const void *data, void *work, NetHandle *handle) {
    KeySystemNet *net = work;

    if (handle == NetHandle_GetOwn() && netId != NetSystem_GetNetId() && size < NET_BUFFER_SIZE) {
        net->hasReceived = TRUE;
        MI_CpuCopy8(data, net->received, size);
    }
}

KeySystemNetRequest *KeySystemNet_GetRequest(KeySystemNet *net) {
    return &net->request;
}

static BOOL KeySystemNet_CanRequest(KeySystemNet *net, unsigned int request) {
    u32 state = KeySystemNet_GetState(net);
    BOOL allowed = FALSE;

    if (sStateRequests[state][request] && sModeRequests[net->mode][request]) {
        allowed = TRUE;
    }
    return allowed;
}

static void KeySystemNet_Stop(KeySystemNet *net) {
    KeySystemNet_SetErrorCallback(net, NULL, NULL);
    switch (net->mode) {
    case KEY_SYSTEM_NET_MODE_OV181:
        if (net->ov181Work != NULL) {
            MBDataConv_Delete(net->ov181Work);
            net->ov181Work = NULL;
            GFL_OvlUnload(OVERLAY_ID(181));
        }
        break;
    case KEY_SYSTEM_NET_MODE_WIFI:
        DWCRap_SetErrorFunc(NULL, NULL);
        if (net->http != NULL) {
            NHttpRap_EndRequest(net->http);
            NHttpRap_Destroy(net->http);
            net->http = NULL;
        }
        GFL_OvlUnload(OVERLAY_ID(189));
        break;
    }
}

static void KeySystemNet_OnError(KeySystemNet *net) {
    if (net->errorCallback != NULL) {
        net->errorCallback(net->errorCallbackWork);
    }
}

static void KeySystemNet_OnWifiError(void *work, int a1, int code, int error) {
    KeySystemNet *net = work;

    if (code == 3 || code == 6) {
        if (net->http != NULL) {
            NHttpRap_EndRequest(net->http);
            NHttpRap_Destroy(net->http);
            net->http = NULL;
        }
    }
}
