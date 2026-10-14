// dwc_rap.c, in overlay 11: the GFL net's Wi-Fi connection, over Nintendo's DWC library. The name is the ROM's own,
// from the file name of its allocations. The functions have no names in swan's symbols; the names in the asserts
// (_dWork, randommatch_query) are the original's

#include "gfl/dwc_rap.h"
#include "types.h"
#include "dwc/dwc.h"
#include "dwc/gdb.h"
#include "dwc/nd.h"
#include "gfl/dwc_rapcommon.h"
#include "gfl/dwc_vchat.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_state.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "stdio.h"

#define _MATCHSTRINGNUM 0x80
#define MYDWC_STATUS_DATA_SIZE_MAX 0x20
#define DWCRAP_FRIEND_MAX 32
#define DWCRAP_NETID_MAX 2

// The states of the connection, in the work's state
enum {
    DWCRAP_STATE_INIT,
    DWCRAP_STATE_INET,
    DWCRAP_STATE_LOGIN_WAIT,
    DWCRAP_STATE_LOGIN,
    DWCRAP_STATE_SERVERS,
    DWCRAP_STATE_FRIENDS,
    DWCRAP_STATE_READY,
    DWCRAP_STATE_MATCHING,
    DWCRAP_STATE_MATCH_CANCEL,
    DWCRAP_STATE_CONNECTED,
    DWCRAP_STATE_CONNECTED_IDLE,
    DWCRAP_STATE_MATCH_ENDED,
    DWCRAP_STATE_MATCH_FAILED,
    DWCRAP_STATE_MATCH_FAILED_END,
    DWCRAP_STATE_ERROR,
    DWCRAP_STATE_15,
    DWCRAP_STATE_SERVER_ERROR,
    DWCRAP_STATE_KICKED,
    DWCRAP_STATE_CLOSED,
    DWCRAP_STATE_ENDING,
    DWCRAP_STATE_ENDED,
};

// The work of the connection, 0x7ac bytes
typedef struct {
    // The packet that is being sent: its header, and what follows
    union {
        u32 word;
        u8 bytes[4];
    } header;
    u8 data[0xfc];
    DWCFriendData *friendList;
    void *unk104;
    void *userData;
    u8 inetControl[0x68];
    void *recvBuf[2];
    // Errors that came up in the library, until they are given to the game
    int pendingCode[DWCRAP_FRIEND_MAX];
    int pendingParam[DWCRAP_FRIEND_MAX];
    GFLNetRecvFunc parentRecvFunc;
    GFLNetRecvFunc childRecvFunc;
    DWCRapDisconnectFunc disconnectFunc;
    void *disconnectWork;
    DWCRapErrorFunc errorFunc;
    void *errorWork;
    DWCRapEventFunc eventFunc;
    void *eventWork;
    DWCRapConnectFunc connectFunc;
    void *connectWork;
    DWCRapRequestFunc requestFunc;
    void (*kickFunc)(int code);
    char randommatch_query[_MATCHSTRINGNUM];
    // What the library says of each friend, and the data they share about where they are
    u8 friendStatus[DWCRAP_FRIEND_MAX];
    char friendStatusData[DWCRAP_FRIEND_MAX][MYDWC_STATUS_DATA_SIZE_MAX];
    u32 friendCheckIndex;
    int connectResult;
    int state;
    int errorCode;
    int matchType;
    int sending;
    int maxPlayers;
    u32 lastConnectBits;
    int vctActive;
    int friendIndex;
    int lastNetId;
    int matchingBusy;
    int vctMode;
    s16 lastIndex;
    s16 vctEnabled;
    int idleFrames[DWCRAP_NETID_MAX];
    int retryCount;
    int unk790;
    u16 unk794;
    u16 unk796;
    u32 connectBits;
    u8 sendSeq;
    u8 recvSeq;
    u8 unk79E;
    u8 micOn;
    u8 unk7A0;
    u8 unk7A1;
    u8 userDataDirty;
    u8 reportError;
    u8 unk7A4;
    u8 unk7A5;
    u8 sendReady;
    u8 ackPending;
    u8 connected;
    u8 friendsMatchActive;
    u8 unk7AA[2];
} DWCRapWork;

static DWCRapWork *_dWork;

static const char sQueryFormat[] = "%s = '%s'";

static void DWCRap_SetState(int state);
static void DWCRap_Delete(void);
static u32 DWCRap_UpdateLogin(void);
static void DWCRap_NewClientCallback(int index, void *param);
static BOOL DWCRap_AttemptCallback(void *param);
static void DWCRap_MatchFailed(void);
static BOOL DWCRap_SendAck(const void *data, int size);
static void DWCRap_FlushAck(void);
static void DWCRap_LoginCallback(int error);
static void DWCRap_TimeoutCallback(u8 aid);
static void DWCRap_UpdateServersCallback(int error);
static void DWCRap_FriendStatusCallback(int index, u8 status, const char *location, void *param);
static void DWCRap_DeleteFriendCallback(int deleteIndex, int srcIndex, void *param);
static void DWCRap_BuddyCallback(int index, void *param);
static void DWCRap_ResetIdleFrames(void);
static void DWCRap_SetRecvTimeouts(void);
static void DWCRap_OnMatched(int index);
static void DWCRap_MatchedCallback(int error, BOOL cancel, BOOL a2, BOOL a3, int index, void *param);
static int DWCRap_EvalCallback(int index, void *param);
static void DWCRap_SendCallback(int size, u8 aid);
static void DWCRap_SetUnk790(u32 header);
static void DWCRap_RecvCallback(u8 aid, u8 *data, int size);
static void DWCRap_ClosedCallback(int error, BOOL a1, BOOL a2, u8 aid, int index, void *param);
static u32 DWCRap_HandleError(void);
static BOOL DWCRap_CanPing(void);
static BOOL DWCRap_SendIdlePing(int index);
static u32 DWCRap_Update(void);
static void DWCRap_VctEndCallback(void);
static void DWCRap_StartVct(HeapID heapId);
static void DWCRap_UpdateFriendStatus(void);
static void DWCRap_AnybodyMatchedCallback(int error, BOOL cancel, BOOL a2, BOOL a3, int index, void *param);
static void DWCRap_FriendMatchedCallback(int error, BOOL cancel, BOOL a2, BOOL a3, int index, void *param);
static void DWCRap_ConnectedCallback(int index, void *param);
static void DWCRap_FreeRecvBuffer(int aid);
static void DWCRap_AllocRecvBuffer(int aid);
static void DWCRap_FreeRecvBuffers(void);
static void DWCRap_FlushPendingErrors(void);

static void DWCRap_SetState(int state) {
    _dWork->state = state;
}

BOOL DWCRap_IsInitialized(void) {
    if (_dWork) {
        return TRUE;
    }
    return FALSE;
}

int DWCRap_Init(void *userData, DWCFriendData *friendList) {
    GFLNetInitData *netInit = func_02042e84();
    int i;

    GFL_ASSERT(_dWork == NULL);
    _dWork = allocConfigDSSoftwareFeature(netInit->heapId, sizeof(DWCRapWork), "dwc_rap.c", 302);
    for (i = 0; i < DWCRAP_FRIEND_MAX; i++) {
        _dWork->pendingCode[i] = -1;
    }
    DWCRap_SetState(DWCRAP_STATE_INIT);
    _dWork->vctMode = 0;
    _dWork->friendIndex = -1;
    _dWork->maxPlayers = netInit->maxConnectNum;
    _dWork->lastNetId = -1;
    _dWork->vctEnabled = 0;
    _dWork->unk79E = 0;
    _dWork->unk794 = 1;
    _dWork->unk790 = 1;
    _dWork->unk796 = 1;
    _dWork->userData = userData;
    _dWork->friendList = friendList;
    _dWork->unk7A1 = 1;
    for (i = 0; i < DWCRAP_FRIEND_MAX; i++) {
        _dWork->friendStatus[i] = 0;
    }
    if (!DWC_CheckHasProfile(_dWork->userData)) {
        return 1;
    }
    if (!DWC_CheckValidConsole(_dWork->userData)) {
        return 2;
    }
    return 0;
}

static void DWCRap_Delete(void) {
    if (_dWork != NULL) {
        func_02042e84();
        DWCRap_FreeRecvBuffers();
        func_02042ed0(_dWork);
        _dWork = NULL;
    }
    func_02058490();
}

// Connects to the internet, logs in, and updates the friend list
static u32 DWCRap_UpdateLogin(void) {
    u16 name[2];
    OSOwnerInfo owner;
    DWCFriendData *friendList;
    int friendListLen;
    u32 result;

    switch (_dWork->state) {
    case DWCRAP_STATE_INIT:
        func_02042e84();
        func_0205ac24(&_dWork->inetControl, 2, 1, 20);
        func_0205acd0();
        DWCRap_SetState(DWCRAP_STATE_INET);
        _dWork->vctActive = 0;
        // fall through
    case DWCRAP_STATE_INET:
        if (func_0205ae04()) {
            if (func_0205af98() == 4) {
                DWCRap_SetState(DWCRAP_STATE_LOGIN_WAIT);
            } else {
                DWCRap_SetState(DWCRAP_STATE_ERROR);
            }
        } else {
            func_0205ae58();
            break;
        }
        // fall through
    case DWCRAP_STATE_LOGIN_WAIT:
        result = DWCRap_HandleError();
        if (result != 0) {
            return result;
        }
        friendList = _dWork->friendList;
        friendListLen = 0;
        if (friendList != NULL) {
            friendListLen = 32;
        }
        func_ov011_0215fa0c(_dWork->userData, 0x2fc6, "tXH2sN", 0, 0, friendList, friendListLen);
        _dWork->friendsMatchActive = 1;
        name[0] = 0;
        name[1] = 0;
        OS_GetOwnerInfo(&owner);
        func_ov011_0215fdf0(name, NULL, DWCRap_LoginCallback, NULL);
        DWCRap_SetState(DWCRAP_STATE_LOGIN);
        // fall through
    case DWCRAP_STATE_LOGIN:
        func_ov011_0215fc50();
        break;
    case DWCRAP_STATE_SERVERS:
        if (_dWork->friendList != NULL) {
            if (!func_ov011_0215fe94(NULL, DWCRap_UpdateServersCallback, _dWork->userData, DWCRap_FriendStatusCallback,
                                     &_dWork, DWCRap_DeleteFriendCallback, &_dWork)) {
                func_020424ac(0, 0, 0, 0x3ee);
            } else {
                func_ov011_0215e4d4(DWCRap_BuddyCallback, NULL);
                func_ov011_0215fc50();
            }
        }
        DWCRap_SetState(DWCRAP_STATE_FRIENDS);
        break;
    case DWCRAP_STATE_FRIENDS:
        func_ov011_0215fc50();
        if (_dWork->friendList == NULL) {
            DWCRap_SetState(DWCRAP_STATE_READY);
        }
        break;
    case DWCRAP_STATE_READY:
        func_ov011_0215fc50();
        _dWork->errorCode = 0x3ef;
        return 0x3ef;
    }
    return DWCRap_HandleError();
}

void DWCRap_SetRecvFuncs(GFLNetRecvFunc parentFunc, GFLNetRecvFunc childFunc) {
    _dWork->parentRecvFunc = parentFunc;
    _dWork->childRecvFunc = childFunc;
}

void DWCRap_SetRequestFunc(DWCRapRequestFunc func) {
    _dWork->requestFunc = func;
}

static void DWCRap_NewClientCallback(int index, void *param) {
}

static BOOL DWCRap_AttemptCallback(void *param) {
    return _dWork->unk7A5;
}

// clang-format off
BOOL DWCRap_StartMatch(const char *key, int numEntry, BOOL a2, int a3) {
    int i;
    int ret;

    GFL_ASSERT(_dWork != NULL);
    if (_dWork->state != DWCRAP_STATE_READY) {
        return FALSE;
    }
    DWCRap_FreeRecvBuffers();
    ret = func_ov011_02160ed4(0, "a", key);
    GFL_ASSERT(ret!=0);
    sys_memset(_dWork->randommatch_query, 0, _MATCHSTRINGNUM);
    sprintf(_dWork->randommatch_query, sQueryFormat, "a", key);
    GFL_ASSERT(GFL_STD_StrLen((const char*)_dWork->randommatch_query) < _MATCHSTRINGNUM);
    if (a2) {
        func_ov011_02160ed4(1, _dWork->randommatch_query, _dWork->randommatch_query);
    }
    for (i = 0; i < numEntry; i++) {
        DWCRap_AllocRecvBuffer(i);
    }
    DWCRap_SetState(DWCRAP_STATE_MATCHING);
    _dWork->maxPlayers = numEntry;
    if (!func_ov011_0215fef8(2, numEntry, _dWork->randommatch_query, DWCRap_MatchedCallback, func_02042d94(),
                             DWCRap_NewClientCallback, func_02042d94(), DWCRap_EvalCallback, func_02042d94(),
                             DWCRap_AttemptCallback, &_dWork->connectResult, func_02042d94())) {
        return FALSE;
    }
    _dWork->matchType = 0;
    func_ov011_02168bb0(DWCRap_SendCallback, 0);
    func_ov011_02168bd8(DWCRap_RecvCallback, 0);
    func_ov011_02160150(DWCRap_ClosedCallback, 0);
    func_ov011_02168c00(DWCRap_TimeoutCallback, 0);
    _dWork->sending = 0;
    _dWork->unk7A1 = 1;
    return TRUE;
}

BOOL DWCRap_StartMatchQuery(const char *query, int numEntry, DWCEvalFunc evalFunc, void *evalWork) {
    int i;

    GFL_ASSERT(_dWork != NULL);
    if (_dWork->state != DWCRAP_STATE_READY) {
        return FALSE;
    }
    DWCRap_FreeRecvBuffers();
    for (i = 0; i < numEntry; i++) {
        DWCRap_AllocRecvBuffer(i);
    }
    DWCRap_SetState(DWCRAP_STATE_MATCHING);
    _dWork->maxPlayers = numEntry;
    if (!func_ov011_0215fef8(1, numEntry, query, DWCRap_MatchedCallback, NULL, DWCRap_NewClientCallback, NULL,
                             evalFunc, evalWork, NULL, NULL, NULL)) {
        return FALSE;
    }
    _dWork->matchType = 0;
    func_ov011_02168bb0(DWCRap_SendCallback, 0);
    func_ov011_02168bd8(DWCRap_RecvCallback, 0);
    func_ov011_02160150(DWCRap_ClosedCallback, 0);
    func_ov011_02168c00(DWCRap_TimeoutCallback, 0);
    _dWork->sending = 0;
    _dWork->unk7A1 = 1;
    _dWork->errorCode = 0x3ef;
    return TRUE;
}
// clang-format on

static void DWCRap_MatchFailed(void) {
    if (_dWork->state == DWCRAP_STATE_MATCH_FAILED) {
        DWCRap_SetState(DWCRAP_STATE_MATCH_FAILED_END);
    } else {
        DWCRap_SetState(DWCRAP_STATE_MATCH_ENDED);
    }
}

int DWCRap_Process(int a0) {
    u32 result;

    DWCRap_FlushAck();
    switch (_dWork->state) {
    case DWCRAP_STATE_INIT:
    case DWCRAP_STATE_INET:
    case DWCRAP_STATE_LOGIN_WAIT:
    case DWCRAP_STATE_LOGIN:
    case DWCRAP_STATE_SERVERS:
    case DWCRAP_STATE_FRIENDS:
        return DWCRap_UpdateLogin();
    case DWCRAP_STATE_MATCHING:
        if (a0 != 0) {
            DWCRap_SetState(DWCRAP_STATE_MATCH_CANCEL);
        }
        if (_dWork->matchType == 2 && _dWork->friendIndex >= 0 && _dWork->friendStatus[_dWork->friendIndex] != 6) {
            DWCRap_SetState(DWCRAP_STATE_MATCH_FAILED);
        }
        break;
    case DWCRAP_STATE_MATCH_CANCEL:
    case DWCRAP_STATE_MATCH_FAILED:
        func_ov011_02160170();
        DWCRap_MatchFailed();
        break;
    case DWCRAP_STATE_CONNECTED: {
        GFLNetInitData *netInit = func_02042e84();

        if (_dWork->vctEnabled != 0) {
            DWCRap_StartVct(netInit->heapId);
        }
        _dWork->unk794 = 0;
        DWCRap_SetState(DWCRAP_STATE_CONNECTED_IDLE);
        _dWork->errorCode = 1000;
        return 1000;
    }
    case DWCRAP_STATE_MATCH_ENDED:
        DWCRap_SetState(DWCRAP_STATE_READY);
        _dWork->sending = 0;
        _dWork->lastNetId = -1;
        _dWork->errorCode = 0x3e9;
        return 0x3e9;
    case DWCRAP_STATE_MATCH_FAILED_END:
        DWCRap_SetState(DWCRAP_STATE_READY);
        _dWork->sending = 0;
        _dWork->lastNetId = -1;
        _dWork->errorCode = 0x3ea;
        return 0x3ea;
    case DWCRAP_STATE_ERROR:
        return DWCRap_HandleError();
    case DWCRAP_STATE_CLOSED:
        if (_dWork->vctActive == 0) {
            func_ov011_02160170();
            DWCRap_SetState(DWCRAP_STATE_ENDING);
            break;
        }
        // fall through
    case DWCRAP_STATE_READY:
        result = DWCRap_Update();
        if (result == 0) {
            _dWork->errorCode = 0x3ef;
            return 0x3ef;
        }
        return result;
    }
    return DWCRap_Update();
}

BOOL DWCRap_SendPacket(const void *data, int size, int type) {
    u16 netIds;

    if (size >= 0x100) {
        return FALSE;
    }
    if (_dWork->sending != 0) {
        return FALSE;
    }
    _dWork->header.word = type | (_dWork->unk794 << 8);
    _dWork->sendSeq++;
    _dWork->header.bytes[2] = _dWork->sendSeq;
    sys_memcpy(data, _dWork->data, size);
    _dWork->sending = 1;
    netIds = func_ov011_02160344();
    if (netIds != func_ov011_02168ad4(netIds, &_dWork->header, size + 4)) {
        _dWork->sending = 0;
        return FALSE;
    }
    return TRUE;
}

static BOOL DWCRap_SendAck(const void *data, int size) {
    if (DWCRap_SendPacket(data, size, 3)) {
        return TRUE;
    }
    return FALSE;
}

BOOL DWCRap_Send(const void *data, int size) {
    if (_dWork->sendReady == 0) {
        return FALSE;
    }
    if (!DWCRap_SendPacket(data, size, 1)) {
        return FALSE;
    }
    _dWork->sendReady = 0;
    if (_dWork->childRecvFunc != NULL) {
        _dWork->childRecvFunc(func_ov011_021602c0(), (u8 *)data, size);
    }
    return TRUE;
}

static void DWCRap_FlushAck(void) {
    if (_dWork->ackPending != 0) {
        u32 data = 0;

        if (DWCRap_SendAck(&data, 4)) {
            _dWork->ackPending = 0;
        }
    }
}

static void DWCRap_LoginCallback(int error) {
    if (func_02057dc4(_dWork->userData)) {
        func_02057de8(_dWork->userData);
        _dWork->userDataDirty = 1;
    }
    if (error == 0) {
        DWCRap_SetState(DWCRAP_STATE_SERVERS);
    } else {
        DWCRap_SetState(DWCRAP_STATE_ERROR);
    }
}

static void DWCRap_TimeoutCallback(u8 aid) {
    if (aid < DWCRAP_NETID_MAX) {
        if (_dWork->connected != 0) {
            func_ov011_02160170();
            _dWork->lastNetId = -1;
            DWCRap_SetState(DWCRAP_STATE_ENDED);
        }
    }
}

static void DWCRap_UpdateServersCallback(int error) {
    if (error == 0) {
        DWCRap_SetState(DWCRAP_STATE_READY);
    } else {
        DWCRap_SetState(DWCRAP_STATE_ERROR);
    }
}

static void DWCRap_FriendStatusCallback(int index, u8 status, const char *location, void *param) {
}

static void DWCRap_DeleteFriendCallback(int deleteIndex, int srcIndex, void *param) {
    int i;

    func_02042e84();
    for (i = 0; i < DWCRAP_FRIEND_MAX; i++) {
        if (_dWork->pendingCode[i] == -1) {
            _dWork->pendingCode[i] = deleteIndex;
            _dWork->pendingParam[i] = srcIndex;
        }
    }
}

static void DWCRap_BuddyCallback(int index, void *param) {
}

static void DWCRap_ResetIdleFrames(void) {
    sys_memset(_dWork->idleFrames, 0, sizeof(_dWork->idleFrames));
}

// clang-format off
static void DWCRap_SetRecvTimeouts(void) {
    int i;

    for (i = 0; i < DWCRAP_NETID_MAX; i++) {
        func_ov011_02168ca4(i, 0);
    }
    if (func_ov011_021602c0() == 0) {
        for (i = 0; i < _dWork->maxPlayers; i++) {
            if (i != func_ov011_021602c0() && (func_ov011_02160344() & (1 << i))) {
                GFL_ASSERT_MSG(func_ov011_02168ca4(i, 10000), "DWC_SetRecvTimeoutTime\n");
            }
        }
    } else {
        GFL_ASSERT_MSG(func_ov011_02168ca4(0, 10000), "DWC_SetRecvTimeoutTime\n");
    }
    _dWork->connected = 1;
    DWCRap_ResetIdleFrames();
}
// clang-format on

static void DWCRap_OnMatched(int index) {
    DWCRap_SetState(DWCRAP_STATE_CONNECTED);
    DWCRap_SetRecvTimeouts();
}

static void DWCRap_MatchedCallback(int error, BOOL cancel, BOOL a2, BOOL a3, int index, void *param) {
    if (error == 0) {
        if (cancel == 0) {
            DWCRap_OnMatched(index);
        } else {
            DWCRap_MatchFailed();
        }
    } else {
        DWCRap_SetState(DWCRAP_STATE_ERROR);
    }
    if (_dWork->connectFunc != NULL) {
        _dWork->connectFunc(func_ov011_021602c0(), _dWork->connectWork);
    }
}

static int DWCRap_EvalCallback(int index, void *param) {
    return 1;
}

static void DWCRap_SendCallback(int size, u8 aid) {
    if (aid < DWCRAP_NETID_MAX) {
        _dWork->sending = 0;
        _dWork->idleFrames[aid] = 0;
    }
}

static void DWCRap_SetUnk790(u32 header) {
    if (header & 0x100) {
        _dWork->unk790 = 1;
    } else {
        _dWork->unk790 = 0;
    }
}

// clang-format off
static void DWCRap_RecvCallback(u8 aid, u8 *data, int size) {
    u32 header = (data[3] << 24) | (data[2] << 16) | (data[1] << 8) | data[0];

    GFL_ASSERT(aid < 2);
    if (aid < DWCRAP_NETID_MAX) {
        _dWork->connected = 1;
        if ((u8)header == 1) {
            DWCRap_SetUnk790(header);
            _dWork->recvSeq = data[2];
            _dWork->ackPending = 1;
        } else if ((u8)header == 3) {
            _dWork->sendReady = 1;
            return;
        } else {
            if (DWCRapVChat_ReceivePacket(aid, data, size)) {
                return;
            }
            return;
        }
        if (func_ov011_021602c0() == 0) {
            if (_dWork->parentRecvFunc != NULL) {
                _dWork->parentRecvFunc(aid, data + 4, size - 4);
            }
        } else {
            if (_dWork->childRecvFunc != NULL) {
                _dWork->childRecvFunc(aid, data + 4, size - 4);
            }
        }
    }
}
// clang-format on

static void DWCRap_ClosedCallback(int error, BOOL a1, BOOL a2, u8 aid, int index, void *param) {
    if (aid < DWCRAP_NETID_MAX) {
        _dWork->sending = 0;
        _dWork->retryCount = 0;
        _dWork->matchingBusy = 0;
        if (error == 0) {
            if (_dWork->unk7A1 == 1 && func_ov011_021602a0() == 1) {
                if (_dWork->state != DWCRAP_STATE_MATCH_CANCEL) {
                    DWCRap_SetState(DWCRAP_STATE_CLOSED);
                }
                if (_dWork->vctActive != 0) {
                    DWCRap_ResetVoiceChat();
                }
            }
            if (a1 == 0) {
                DWCRap_SetState(DWCRAP_STATE_CLOSED);
            }
        }
        if (_dWork->reportError != 0) {
            GFLNetErrorInfo *info = func_02042540();

            if (info->type == 0) {
                func_020424ac(info->code, info->unk4, info->unk8, 0x3f1);
            }
        }
        func_02040a9c(aid);
        if (_dWork->disconnectFunc != NULL) {
            _dWork->disconnectFunc(aid, _dWork->disconnectWork);
        }
    }
}

// Asks the library about its error and the game's functions what to do about it, and returns the error's kind
static u32 DWCRap_HandleError(void) {
    int code;
    int type;
    int error;
    u32 result = 0;

    error = func_020583b0(&code, &type);
    if (error != 0) {
        if (_dWork->eventFunc != NULL && _dWork->eventFunc(_dWork->eventWork, code, type, error) == 1) {
            return 0;
        }
        if (_dWork->errorFunc != NULL) {
            _dWork->errorFunc(_dWork->errorWork, code, type, error);
        }
        result = code;
        if (code == 0 || type == 1) {
            result = error;
        }
        switch (type) {
        case 1:
            func_02058490();
            if (_dWork != NULL && _dWork->state == DWCRAP_STATE_ERROR) {
                DWCRap_SetState(DWCRAP_STATE_READY);
            }
            break;
        case 2:
            if (code <= -40000 && code >= -41999) {
                Gdb_Shutdown();
            }
            if (_dWork != NULL && _dWork->state == DWCRAP_STATE_ERROR) {
                DWCRap_SetState(DWCRAP_STATE_READY);
            }
            break;
        case 4:
            if (code <= -40000 && code >= -41999) {
                Gdb_Shutdown();
            }
            break;
        case 5:
            DWC_NdCleanupAsync();
            break;
        case 3:
        case 6:
            if (_dWork != NULL) {
                switch (_dWork->state) {
                case DWCRAP_STATE_LOGIN:
                case DWCRAP_STATE_READY:
                case DWCRAP_STATE_MATCHING:
                case DWCRAP_STATE_MATCH_CANCEL:
                case DWCRAP_STATE_CONNECTED:
                case DWCRAP_STATE_CONNECTED_IDLE:
                case DWCRAP_STATE_MATCH_ENDED:
                case DWCRAP_STATE_ERROR:
                case DWCRAP_STATE_CLOSED:
                case DWCRAP_STATE_ENDING:
                case DWCRAP_STATE_ENDED:
                    if (_dWork->friendsMatchActive == 1) {
                        func_ov011_0215fb78();
                        _dWork->friendsMatchActive = 0;
                    }
                    // fall through
                case DWCRAP_STATE_INIT:
                case DWCRAP_STATE_INET:
                case DWCRAP_STATE_LOGIN_WAIT:
                    func_0205b198();
                    break;
                default:
                    break;
                }
                if (_dWork != NULL) {
                    DWCRap_SetState(DWCRAP_STATE_SERVER_ERROR);
                }
            }
            break;
        case 7:
            if (_dWork != NULL) {
                DWCRap_SetState(DWCRAP_STATE_KICKED);
                if (_dWork->kickFunc != NULL) {
                    _dWork->kickFunc(-code);
                }
            }
            break;
        }
    }
    if (_dWork->unk79E != 0) {
        result = 0x3ee;
        func_020424ac(code, type, error, result);
    } else if (_dWork->state == DWCRAP_STATE_ENDED) {
        result = 0x3f0;
        func_020424ac(code, type, error, result);
    } else if (error != 0) {
        func_020424ac(code, type, error, 0);
    }
    return result;
}

static BOOL DWCRap_CanPing(void) {
    int i;
    BOOL found = FALSE;

    for (i = 0; i < _dWork->maxPlayers; i++) {
        if (i != func_ov011_021602c0() && func_ov011_02160370(i)) {
            found = TRUE;
            if (!func_ov011_02168998(i)) {
                return FALSE;
            }
        }
    }
    return found;
}

static BOOL DWCRap_SendIdlePing(int index) {
    if (_dWork->sending == 0 && DWCRap_CanPing() && (func_ov011_02160344() & 0xfffe)) {
        _dWork->sending = 1;
        _dWork->header.word = 2 | (_dWork->unk794 << 8);
        func_ov011_02168ad4(func_ov011_02160344(), &_dWork->header, 4);
        _dWork->idleFrames[index] = 0;
        return TRUE;
    }
    return FALSE;
}

static u32 DWCRap_Update(void) {
    int i;

    DWCRap_FlushPendingErrors();
    func_ov011_0215fc50();
    DWCRap_UpdateFriendStatus();
    if (_dWork->vctActive != 0) {
        if (_dWork->unk794 == 1 && _dWork->unk790 == 1 && _dWork->unk796 == 1) {
            DWCRapVChat_Update(FALSE);
        } else {
            DWCRapVChat_Update(TRUE);
        }
        if (_dWork->lastConnectBits != func_ov011_02160344() && _dWork->micOn == 0 && _dWork->vctEnabled != 0) {
            if (DWCRapVChat_AddHeard(func_ov011_02160344(), func_ov011_021602c0())) {
                _dWork->lastConnectBits = func_ov011_02160344();
            }
        }
    }
    if (_dWork->state == DWCRAP_STATE_ENDED) {
        u32 result = DWCRap_HandleError();

        if (result != 0) {
            return result;
        }
        return 0x3eb;
    } else if (_dWork->state == DWCRAP_STATE_ENDING) {
        return 0x3ec;
    } else if (_dWork->state == DWCRAP_STATE_CONNECTED || _dWork->state == DWCRAP_STATE_CONNECTED_IDLE) {
        for (i = 0; i < _dWork->maxPlayers; i++) {
            if (_dWork->idleFrames[i]++ >= 240 && _dWork->sending == 0 && DWCRap_SendIdlePing(i)) {
                DWCRap_ResetIdleFrames();
                break;
            }
        }
    }
    return DWCRap_HandleError();
}

int DWCRap_GetNetId(void) {
    if (_dWork != NULL) {
        if (_dWork->state == DWCRAP_STATE_CONNECTED || _dWork->state == DWCRAP_STATE_CONNECTED_IDLE ||
            _dWork->state == DWCRAP_STATE_CLOSED) {
            return func_ov011_021602c0();
        }
    }
    return -1;
}

static void DWCRap_VctEndCallback(void) {
    _dWork->vctActive = 0;
}

void func_ov011_021515a4(void) {
    _dWork->unk794 = 1;
}

static void DWCRap_StartVct(HeapID heapId) {
    int mode;

    _dWork->unk794 = 1;
    _dWork->unk790 = 1;
    _dWork->unk796 = 1;
    if (_dWork->vctActive == 0) {
        switch (_dWork->vctMode) {
        case 2:
            mode = 1;
            break;
        case 3:
            mode = 2;
            break;
        case 4:
            mode = 3;
            break;
        case 5:
            mode = 4;
            break;
        default:
            mode = 4;
            break;
        }
        DWCRapVChat_Start(heapId, mode, 1);
        DWCRapVChat_SetEndCallback(DWCRap_VctEndCallback);
        _dWork->vctActive = 1;
    }
}

void DWCRap_StartVoiceChat(void) {
    DWCRap_StartVct(func_02042e84()->heapId);
}

void DWCRap_StopVoiceChat(void) {
    if (_dWork->vctActive != 0) {
        DWCRapVChat_RequestEnd();
    }
}

int DWCRap_IsVoiceChatActive(void) {
    return _dWork->vctActive;
}

void DWCRap_ResetVoiceChat(void) {
    DWCRapVChat_End();
    if (_dWork != NULL) {
        _dWork->vctActive = 0;
        _dWork->lastConnectBits = 0;
        _dWork->unk794 = 0;
    }
}

void DWCRap_SetMic(BOOL on) {
    if (on) {
        DWCRapVChat_RemoveHeard(func_ov011_021602c0());
        _dWork->micOn = TRUE;
    } else {
        _dWork->micOn = FALSE;
    }
    DWCRapVChat_SetMuted(_dWork->micOn);
}

// clang-format off
int DWCRap_GetErrorKind(int code, int type) {
    int hundreds = code / 100;
    int thousands = code / 1000;

    if (code == 20101) {
        return 1;
    }
    if (thousands == 23) {
        return 1;
    }
    if (code == 20108) {
        return 2;
    }
    if (code == 20110) {
        return 3;
    }
    if (hundreds == 512) {
        return 4;
    }
    if (hundreds == 500) {
        return 5;
    }
    if (code == 51103) {
        return 6;
    }
    if (hundreds == 510) {
        return 6;
    }
    if (hundreds == 511) {
        return 6;
    }
    if (hundreds == 513) {
        return 6;
    }
    if (code >= 52000 && code <= 52003) {
        return 8;
    }
    if (code >= 52010 && code <= 52012) {
        return 8;
    }
    if (code >= 52100 && code <= 52103) {
        return 8;
    }
    if (code >= 52110 && code <= 52112) {
        return 8;
    }
    if (code >= 52200 && code <= 52203) {
        return 8;
    }
    if (code >= 52210 && code <= 52212) {
        return 8;
    }
    if (code >= 52400 && code <= 52403) {
        return 8;
    }
    if (code >= 52410 && code <= 52412) {
        return 8;
    }
    if (code >= 52500 && code <= 52503) {
        return 8;
    }
    if (code >= 52510 && code <= 52512) {
        return 8;
    }
    if (code >= 52700 && code <= 52703) {
        return 8;
    }
    if (code >= 52710 && code <= 52712) {
        return 8;
    }
    if (code == 80430) {
        return 9;
    }
    if (thousands == 20) {
        return 0;
    }
    if (hundreds == 520) {
        return 0;
    }
    if (hundreds == 521) {
        return 0;
    }
    if (hundreds == 522) {
        return 0;
    }
    if (hundreds == 523) {
        return 0;
    }
    if (hundreds == 530) {
        return 0;
    }
    if (hundreds == 531) {
        return 0;
    }
    if (hundreds == 532) {
        return 0;
    }
    if (code < 10000) {
        return 14;
    }
    if (thousands == 31) {
        return 12;
    }
    switch (type) {
    case 0:
    case 1:
        return 14;
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
        return 11;
    default:
        GFL_ASSERT(0);
        break;
    case 7:
        return 15;
    }
    return -1;
}
// clang-format on

BOOL DWCRap_RequestClose(BOOL a0) {
    int code;
    int type;

    if (func_020583b0(&code, &type)) {
        func_02058490();
    }
    if (a0 == 0) {
        switch (_dWork->state) {
        case DWCRAP_STATE_MATCHING:
        case DWCRAP_STATE_CONNECTED:
        case DWCRAP_STATE_CONNECTED_IDLE:
            if (_dWork->vctActive != 0) {
                DWCRapVChat_RequestEnd();
            }
            DWCRap_SetState(DWCRAP_STATE_CLOSED);
            break;
        case DWCRAP_STATE_READY:
        case DWCRAP_STATE_SERVER_ERROR:
        case DWCRAP_STATE_ENDING:
        case DWCRAP_STATE_ENDED:
            return TRUE;
        }
    } else {
        switch (_dWork->state) {
        case DWCRAP_STATE_READY:
        case DWCRAP_STATE_ENDING:
        case DWCRAP_STATE_ENDED:
            return TRUE;
        }
    }
    return FALSE;
}

BOOL DWCRap_ResetToReady(void) {
    if (_dWork->state == DWCRAP_STATE_ENDING || _dWork->state == DWCRAP_STATE_ENDED ||
        _dWork->state == DWCRAP_STATE_SERVER_ERROR || _dWork->state == DWCRAP_STATE_READY) {
        DWCRap_SetState(DWCRAP_STATE_READY);
        _dWork->errorCode = 0x3ef;
        _dWork->lastNetId = -1;
        return TRUE;
    }
    return FALSE;
}

void DWCRap_Shutdown(void) {
    if (_dWork != NULL) {
        func_02058490();
        if (_dWork->friendsMatchActive != 0) {
            func_ov011_0215fb78();
        }
        _dWork->friendsMatchActive = 0;
        func_0205b198();
        DWCRap_ResetVoiceChat();
        DWCRap_Delete();
    }
}

static void DWCRap_UpdateFriendStatus(void) {
    if (_dWork->friendList != NULL) {
        u32 index = _dWork->friendCheckIndex & 0x1f;

        if (func_020576a4(&_dWork->friendList[index])) {
            int size;
            u8 status = func_ov011_0215e410(&_dWork->friendList[index], _dWork->friendStatusData[index], &size);

            if (size < 1) {
                _dWork->friendStatus[index] = 0;
                sys_memset(_dWork->friendStatusData[index], 0, MYDWC_STATUS_DATA_SIZE_MAX);
            } else {
                _dWork->friendStatus[index] = status;
            }
        }
        _dWork->friendCheckIndex++;
    }
}

// clang-format off
void DWCRap_SetOwnStatusData(const char *data, int size) {
    GFL_ASSERT(size <= MYDWC_STATUS_DATA_SIZE_MAX);
    func_ov011_0215e47c(data, size);
}
// clang-format on

char *DWCRap_GetFriendStatusData(int index) {
    return _dWork->friendStatusData[index];
}

u8 DWCRap_GetFriendStatus(int index) {
    return _dWork->friendStatus[index];
}

int DWCRap_Connect(int friendIndex, int numEntry, BOOL a2) {
    int i;
    int entries = numEntry;
    int matchType;
    BOOL ok;

    if (DWCRap_IsUserDataDirty()) {
        return -4;
    }
    if (_dWork->state != DWCRAP_STATE_READY) {
        _dWork->retryCount++;
        if (_dWork->retryCount > 120) {
            return -3;
        }
        return -1;
    }
    DWCRap_FreeRecvBuffers();
    _dWork->connectBits = 0;
    _dWork->unk7A1 = 1;
    _dWork->friendIndex = friendIndex;
    _dWork->maxPlayers = numEntry;
    _dWork->lastIndex = -1;
    if (a2) {
        entries = 2;
    }
    _dWork->matchingBusy = 1;
    if (friendIndex < 0) {
        ok = func_ov011_0215ff7c(2, entries, DWCRap_AnybodyMatchedCallback, func_02042d94(), DWCRap_ConnectedCallback,
                                 func_02042d94(), DWCRap_AttemptCallback, &_dWork->connectResult, func_02042d94());
        matchType = 1;
    } else {
        ok = func_ov011_0215fff4(2, friendIndex, DWCRap_FriendMatchedCallback, NULL, DWCRap_ConnectedCallback,
                                 func_02042d94(), DWCRap_AttemptCallback, &_dWork->connectResult, func_02042d94());
        matchType = 2;
    }
    _dWork->matchType = matchType;
    if (!ok) {
        _dWork->retryCount++;
        if (_dWork->retryCount > 120) {
            func_020424ac(0, 0, 0, 0x3f1);
        }
        return -2;
    }
    _dWork->retryCount = 0;
    for (i = 0; i < numEntry; i++) {
        DWCRap_AllocRecvBuffer(i);
    }
    DWCRap_SetState(DWCRAP_STATE_MATCHING);
    func_ov011_02168bb0(DWCRap_SendCallback, 0);
    func_ov011_02168bd8(DWCRap_RecvCallback, 0);
    func_ov011_02160150(DWCRap_ClosedCallback, 0);
    func_ov011_02168c00(DWCRap_TimeoutCallback, 0);
    _dWork->sending = 0;
    return 0;
}

int DWCRap_GetFriendIndex(void) {
    if (_dWork != NULL) {
        return _dWork->friendIndex;
    }
    return -1;
}

static void DWCRap_AnybodyMatchedCallback(int error, BOOL cancel, BOOL a2, BOOL a3, int index, void *param) {
    BOOL failed = FALSE;

    _dWork->matchingBusy = 0;
    if (error == 0) {
        if (cancel == 0) {
            if (_dWork->unk7A4 != 0 && index == -1) {
                failed = TRUE;
            }
            if (_dWork->requestFunc != NULL) {
                if (!_dWork->requestFunc(index, func_02042d94())) {
                    failed = TRUE;
                }
            }
            if (_dWork->unk7A0 != 0 || failed) {
                if (index != -1) {
                    func_ov011_02160284(index);
                }
            }
            _dWork->friendIndex = index;
            _dWork->connectBits = func_ov011_02160344();
            if (_dWork->connectBits == 1) {
                DWCRap_SetState(DWCRAP_STATE_MATCH_CANCEL);
            } else {
                DWCRap_OnMatched(index);
            }
        } else if (a2 == 0) {
            _dWork->lastIndex = index;
            _dWork->lastNetId = -1;
        }
    }
}

static void DWCRap_FriendMatchedCallback(int error, BOOL cancel, BOOL a2, BOOL a3, int index, void *param) {
    _dWork->matchingBusy = 0;
    if (error == 0 && cancel == 0) {
        DWCRap_OnMatched(index);
    }
}

static void DWCRap_ConnectedCallback(int index, void *param) {
    _dWork->lastNetId = index;
    if (_dWork->connectFunc != NULL) {
        _dWork->connectFunc(index, _dWork->connectWork);
    }
}

BOOL func_ov011_02151de4(void) {
    return DWCRapVChat_GetSpeaking();
}

s16 func_ov011_02151dec(void) {
    if (_dWork != NULL) {
        return _dWork->vctEnabled;
    }
    return 0;
}

int DWCRap_GetLastNetId(void) {
    if (_dWork != NULL) {
        return _dWork->lastNetId;
    }
    return 0;
}

s16 func_ov011_02151e24(void) {
    if (_dWork != NULL) {
        return _dWork->lastIndex;
    }
    return -1;
}

void func_ov011_02151e40(s16 a0) {
    _dWork->vctEnabled = a0;
}

static void DWCRap_FreeRecvBuffer(int aid) {
    if (_dWork->recvBuf[aid] != NULL) {
        func_02042ed0(_dWork->recvBuf[aid]);
        _dWork->recvBuf[aid] = NULL;
    }
}

static void DWCRap_AllocRecvBuffer(int aid) {
    GFLNetInitData *netInit = func_02042e84();

    DWCRap_FreeRecvBuffer(aid);
    if (_dWork->recvBuf[aid] == NULL) {
        _dWork->recvBuf[aid] = allocConfigDSSoftwareFeature(netInit->heapId, 0x1000, "dwc_rap.c", 2929);
        func_ov011_02168b84(aid, _dWork->recvBuf[aid], 0x1000);
    }
}

static void DWCRap_FreeRecvBuffers(void) {
    int i;

    for (i = 0; i < DWCRAP_NETID_MAX; i++) {
        DWCRap_FreeRecvBuffer(i);
    }
    _dWork->sendReady = 1;
}

BOOL DWCRap_IsUserDataDirty(void) {
    return _dWork->userDataDirty;
}

void DWCRap_ClearUserDataDirty(void) {
    _dWork->userDataDirty = 0;
}

BOOL DWCRap_IsReady(void) {
    if (_dWork != NULL) {
        if (_dWork->state == DWCRAP_STATE_READY) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

BOOL DWCRap_IsConnected(void) {
    if (_dWork != NULL && (_dWork->state == DWCRAP_STATE_CONNECTED || _dWork->state == DWCRAP_STATE_CONNECTED_IDLE)) {
        return TRUE;
    }
    return FALSE;
}

static void DWCRap_FlushPendingErrors(void) {
    int i;

    for (i = 0; i < DWCRAP_FRIEND_MAX; i++) {
        if (_dWork->pendingCode[i] != -1) {
            GFLNetInitData *netInit = func_02042e84();

            if (netInit->unk34.wifiError.callback != NULL) {
                netInit->unk34.wifiError.callback(_dWork->pendingCode[i], _dWork->pendingParam[i], func_02042d94());
            }
            _dWork->pendingCode[i] = -1;
        }
    }
}

int DWCRap_GetStatus(void) {
    return _dWork->errorCode;
}

void func_ov011_02151fec(int a0) {
    _dWork->unk7A5 = a0;
}

BOOL DWCRap_IsEnding(void) {
    if (_dWork == NULL) {
        return TRUE;
    }
    if (_dWork->state == DWCRAP_STATE_ENDING || _dWork->state == DWCRAP_STATE_SERVER_ERROR ||
        _dWork->state == DWCRAP_STATE_ENDED) {
        return TRUE;
    }
    return FALSE;
}

void DWCRap_SetReportError(int a0) {
    _dWork->reportError = a0;
}

void DWCRap_SetErrorFunc(DWCRapErrorFunc func, void *work) {
    _dWork->errorFunc = func;
    _dWork->errorWork = work;
}

void DWCRap_SetEventFunc(DWCRapEventFunc func, void *work) {
    _dWork->eventFunc = func;
    _dWork->eventWork = work;
}
