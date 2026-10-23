#include <string.h>
// Before dwc/ext.h: MWCC builds the copies of the packets' magic and of the machines' connection user data in place, as
// the ROM does, only when <string.h> declares strcpy and memcpy
#include "dwc/common.h"
#include "dwc/dwc.h"
#include "dwc/ext.h"
#include "dwc/friend.h"
#include "dwc/gp.h"
#include "dwc/gsAvailable.h"
#include "dwc/gsPlatformSocket.h"
#include "dwc/gstats.h"
#include "dwc/gt2Main.h"
#include "dwc/gt2Utility.h"
#include "dwc/match.h"
#include "dwc/match_internal.h"
#include "dwc/memfunc.h"
#include "dwc/natneg.h"
#include "dwc/qr2.h"
#include "dwc/sb_server.h"
#include "dwc/sb_serverbrowsing.h"
#include "dwc/transport.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// DWC's friends-match front end (dwc_main.c, a guessed SDK name; the ROM names none of it), in overlay 11. It owns
// the login/friend/match control, the GT2 socket and connections, and the callbacks that report to the game. Its
// original source ends before DWC_AddMatchKeyInt: its four strings (SCM, SCN, GPCM and MAT) precede the matching core's
// data, which has its own copies of all four, and its connection statics precede the core's state and key table.

// The library's one friends match control (sMatchRoot.fm, which DWC_InitFriendsMatch makes), and the socket error that
// closed its GT2 socket
typedef struct MatchRoot {
    int socketError;
    DWCFriendsMatchControl *fm;
} MatchRoot;

static MatchConnectionInfo sConnectionInfos[0x20];
static GT2Connection sConnections[0x20];
static u8 sNetIds[0x20];
static MatchRoot sMatchRoot;

static u8 func_ov011_021603b8(void);
static void DWCi_ClearGT2ConnectionList(void);
static void DWCi_SetState(int state);
static s32 DWCi_DeleteAID(u8 netId);
static u32 DWCi_GetAIDBitmapFromList(const u8 *netIds, int count);
static int DWCi_HandleGPError(int result);
static GT2Result DWCi_HandleGT2Error(GT2Result result);
static void DWCi_LoginCallback(DWCError error, int profileId, void *param);
static void DWCi_UpdateServersCallback(DWCError error, BOOL isChanged, void *param);
static void DWCi_MatchedCallback(DWCError error, BOOL cancel, BOOL a2, BOOL a3, int index, void *param);
static void DWCi_GPErrorCallback(GPConnection *connection, void *arg, void *param);
static void DWCi_GPRecvBuddyMessageCallback(GPConnection *connection, void *arg, void *param);
static void DWCi_GT2ReceivedCallback(GT2Connection connection, GT2Byte *message, int length, GT2Bool reliable);
static void DWCi_HandleClosedConnection(GT2Connection conn, int closeType, int isNetId, u8 netId);
static void DWCi_GT2ClosedCallback(GT2Connection conn, GT2CloseReason closeType);
static void DWCi_GT2PingCallback(GT2Connection connection, int latency);
static void DWCi_GT2SocketErrorCallback(GT2Socket socket);
static void DWCi_SetMatchType(u8 mode);
static BOOL func_ov011_02160d88(void);
static BOOL func_ov011_02160dac(void);
static BOOL DWC_IsValidCancelMatching(void);

void DWC_InitFriendsMatch(DWCUserData *userData, int productId, const char *secretKey, int sendBufSize, int recvBufSize,
                          DWCFriendData *friendList, int friendListLen) {
    u32 len;

    sMatchRoot.fm = DWC_Alloc(4, sizeof(DWCFriendsMatchControl));
    MI_CpuFill8(sMatchRoot.fm, 0, sizeof(DWCFriendsMatchControl));
    DWC_ClearError();
    sMatchRoot.fm->gt2Socket = NULL;
    sMatchRoot.fm->gt2Callbacks.connected = DWCi_GT2ConnectedCallback;
    sMatchRoot.fm->gt2Callbacks.received = DWCi_GT2ReceivedCallback;
    sMatchRoot.fm->gt2Callbacks.closed = DWCi_GT2ClosedCallback;
    sMatchRoot.fm->gt2Callbacks.ping = DWCi_GT2PingCallback;
    sMatchRoot.fm->gt2SendBufSize = sendBufSize ? sendBufSize : 0x2000;
    sMatchRoot.fm->gt2RecvBufSize = recvBufSize ? recvBufSize : 0x2000;
    sMatchRoot.fm->gpObj = NULL;
    sMatchRoot.fm->userdata = userData;
    sMatchRoot.fm->state = 0;
    sMatchRoot.fm->lastState = 0;
    sMatchRoot.fm->ownCloseFlag = 0;
    sMatchRoot.fm->profileID = 0;
    sMatchRoot.fm->gameName = gcd_gamename;
    sMatchRoot.fm->secretKey = gcd_secret_key;
    sMatchRoot.fm->loginCallback = NULL;
    sMatchRoot.fm->loginParam = NULL;
    sMatchRoot.fm->updateServersCallback = NULL;
    sMatchRoot.fm->updateServersParam = NULL;
    sMatchRoot.fm->matchedCallback = NULL;
    sMatchRoot.fm->matchedParam = NULL;
    sMatchRoot.fm->closedCallback = NULL;
    sMatchRoot.fm->closedParam = NULL;
    DWCi_ClearGT2ConnectionList();
    DWCi_LoginInit(&sMatchRoot.fm->logcnt, userData, &sMatchRoot.fm->gpObj, productId, userData->gamecode,
                   sMatchRoot.fm->playerName, DWCi_LoginCallback, NULL);
    DWCi_FriendInit(&sMatchRoot.fm->friendcnt, &sMatchRoot.fm->gpObj, sMatchRoot.fm->playerName, friendList,
                    friendListLen);
    DWCi_MatchInit(&sMatchRoot.fm->matchcnt, &sMatchRoot.fm->gpObj, sMatchRoot.fm, &sMatchRoot.fm->gt2Callbacks,
                   gcd_gamename, gcd_secret_key, friendList, friendListLen);
    DWCi_InitTransport(&sMatchRoot.fm->matchcnt.transport);
    len = strlen(secretKey);
    if (len < 0x100) {
        len = strlen(secretKey);
    } else {
        len = 0xff;
    }
    MI_CpuCopy8(secretKey, gcd_secret_key, len);
    gcd_secret_key[len] = 0;
}

void DWC_ShutdownFriendsMatch(void) {
    if (sMatchRoot.fm != NULL) {
        if (sMatchRoot.fm->matchcnt.qr2Obj != NULL) {
            qr2_shutdown(sMatchRoot.fm->matchcnt.qr2Obj);
            sMatchRoot.fm->matchcnt.qr2Obj = NULL;
        }
        sMatchRoot.fm->matchcnt.qr2ShutdownFlag = 0;
        if (sMatchRoot.fm->matchcnt.sbObj != NULL) {
            ServerBrowserFree(sMatchRoot.fm->matchcnt.sbObj);
            sMatchRoot.fm->matchcnt.sbObj = NULL;
        }
        DWCi_NNFreeNegotiateList();
        func_ov011_0215f2c0();
        CloseStatsConnection();
        func_ov011_0215f2cc();
        if (sMatchRoot.fm->gpObj != NULL) {
            gpSetCallback(&sMatchRoot.fm->gpObj, 0, NULL, 0);
            gpSetCallback(&sMatchRoot.fm->gpObj, 3, NULL, 0);
            gpSetCallback(&sMatchRoot.fm->gpObj, 1, NULL, 0);
            gpSetCallback(&sMatchRoot.fm->gpObj, 2, NULL, 0);
            gpProcess(&sMatchRoot.fm->gpObj);
            gpDestroy(&sMatchRoot.fm->gpObj);
            sMatchRoot.fm->gpObj = NULL;
        }
        DWCi_ShutdownLogin();
        DWCi_ShutdownFriend();
        DWCi_ShutdownMatch();
        DWCi_ShutdownTransport();
        if (sMatchRoot.fm->gt2Socket != NULL) {
            gt2CloseSocket(sMatchRoot.fm->gt2Socket);
            sMatchRoot.fm->gt2Socket = NULL;
        }
        DWC_Free(4, sMatchRoot.fm, 0);
        sMatchRoot.fm = NULL;
    }
}

void DWC_ProcessFriendsMatch(void) {
    DWC_UpdateConnection();
    if (sMatchRoot.fm == NULL || sMatchRoot.fm->state == 0 || DWCi_IsError()) {
        return;
    }
    switch (sMatchRoot.fm->state) {
    case 1:
        switch (GSIAvailableCheckThink()) {
        case GSIACAvailable:
            if (DWCi_HandleGPError(gpInitialize(&sMatchRoot.fm->gpObj, sMatchRoot.fm->logcnt.productID, 0x10, 0xb))) {
                return;
            }
            if (DWCi_HandleGPError(gpSetCallback(&sMatchRoot.fm->gpObj, 0, DWCi_GPErrorCallback, 0))) {
                return;
            }
            if (DWCi_HandleGPError(gpSetCallback(&sMatchRoot.fm->gpObj, 3, DWCi_GPRecvBuddyMessageCallback, 0))) {
                return;
            }
            if (DWCi_HandleGPError(
                    gpSetCallback(&sMatchRoot.fm->gpObj, 7, (GPCallback)DWCi_GPRecvBuddyAuthCallback, 0))) {
                return;
            }
            if (DWCi_HandleGPError(
                    gpSetCallback(&sMatchRoot.fm->gpObj, 1, (GPCallback)DWCi_GPRecvBuddyRequestCallback, 0))) {
                return;
            }
            if (DWCi_HandleGPError(
                    gpSetCallback(&sMatchRoot.fm->gpObj, 2, (GPCallback)DWCi_GPRecvBuddyStatusCallback, 0))) {
                return;
            }
            DWCi_SetState(2);
            if (DWCi_LoginAsync() == 0) {
                DWCi_StopLogin(2, -20100);
            }
            break;
        case GSIACUnavailable:
            DWCi_StopLogin(3, -20110);
            return;
        case GSIACTemporarilyUnavailable:
            DWCi_StopLogin(4, -20101);
            return;
        }
        break;
    case 2:
        DWCi_LoginProcess();
        break;
    case 3:
        DWCi_FriendProcess();
        DWCi_MatchProcess(0);
        break;
    case 4:
        DWCi_MatchProcess(1);
        DWCi_FriendProcess();
        break;
    case 5:
        DWCi_TransportProcess();
        DWCi_FriendProcess();
        if (sMatchRoot.fm->matchcnt.mode == 2 || sMatchRoot.fm->matchcnt.mode == 3) {
            DWCi_MatchProcess(1);
        } else if (sMatchRoot.fm->gt2Socket != NULL) {
            DWCi_MatchProcess(0);
        }
        break;
    }
    if (sMatchRoot.fm->matchcnt.qr2ShutdownFlag == 1) {
        if (sMatchRoot.fm->matchcnt.qr2Obj != NULL) {
            qr2_shutdown(sMatchRoot.fm->matchcnt.qr2Obj);
            sMatchRoot.fm->matchcnt.qr2Obj = NULL;
        }
        sMatchRoot.fm->matchcnt.qr2ShutdownFlag = 0;
    }
}

BOOL DWC_LoginAsync(const u16 *name, const char *reserved, DWCLoginCallback callback, void *param) {
    int len;

    if (name == NULL) {
        return FALSE;
    }
    if (DWCi_IsError() || sMatchRoot.fm->state != 0) {
        return FALSE;
    }
    sMatchRoot.fm->loginCallback = callback;
    sMatchRoot.fm->loginParam = param;
    if (name == NULL || name[0] == 0) {
        len = 0;
    } else {
        MIi_CpuClear16(0, sMatchRoot.fm->playerName, 0x34);
        if (DWCi_WStrLen(name) <= 0x19) {
            len = DWCi_WStrLen(name);
        } else {
            len = 0x19;
        }
        MIi_CpuCopy16(name, sMatchRoot.fm->playerName, len * 2);
    }
    sMatchRoot.fm->playerName[len] = 0;
    if (DWC_GetInetStatus() != 4) {
        DWCi_StopLogin(2, -60010);
        return FALSE;
    }
    DWCi_SetState(1);
    GSIStartAvailableCheckA(sMatchRoot.fm->gameName);
    return TRUE;
}

BOOL DWC_UpdateServersAsync(const char *name, DWCUpdateServersCallback callback, void *userData,
                            DWCFriendStatusCallback statusCallback, void *statusParam,
                            DWCDeleteFriendListCallback deleteCallback, void *deleteParam) {
    if (DWCi_IsError() || sMatchRoot.fm->state < 3 || DWCi_IsUpdatingServers()) {
        return FALSE;
    }
    sMatchRoot.fm->updateServersCallback = callback;
    sMatchRoot.fm->updateServersParam = userData;
    DWCi_UpdateServersAsync(sMatchRoot.fm->logcnt.authToken, sMatchRoot.fm->logcnt.partnerChallenge,
                            DWCi_UpdateServersCallback, 0, statusCallback, statusParam, deleteCallback, deleteParam);
    return TRUE;
}

BOOL DWC_ConnectToAnybodyAsync(int type, u8 numEntry, const char *query, DWCMatchedSCCallback matched,
                               void *matchedParam, DWCNewClientCallback newClient, void *newClientParam,
                               DWCEvalPlayerCallback eval, void *evalParam, DWCAttemptFunc attempt, int *attemptParam,
                               void *param) {
    if (DWCi_IsError() || sMatchRoot.fm->state != 3) {
        return FALSE;
    }
    DWCi_ClearGT2ConnectionList();
    sMatchRoot.fm->matchedCallback = matched;
    sMatchRoot.fm->matchedParam = matchedParam;
    DWCi_SetState(4);
    DWCi_SetMatchType(0);
    DWCi_ConnectToAnybodyAsync(type, numEntry - 1, query, DWCi_MatchedCallback, NULL, newClient, newClientParam, eval,
                               evalParam, attempt, attemptParam, param);
    return TRUE;
}

BOOL DWC_SetupGameServer(int type, u8 numEntry, DWCMatchedSCCallback matched, void *matchedParam,
                         DWCNewClientCallback newClient, void *newClientParam, DWCAttemptFunc attempt,
                         int *attemptParam, void *param) {
    if (DWCi_IsError() || sMatchRoot.fm->state != 3) {
        return FALSE;
    }
    DWCi_ClearGT2ConnectionList();
    sMatchRoot.fm->matchedCallback = matched;
    sMatchRoot.fm->matchedParam = matchedParam;
    DWCi_SetState(4);
    DWCi_SetMatchType(2);
    DWCi_SetupGameServer(type, numEntry - 1, DWCi_MatchedCallback, NULL, newClient, newClientParam, attempt,
                         attemptParam, param);
    return TRUE;
}

BOOL DWC_ConnectToGameServerAsync(int type, int friendIndex, DWCMatchedSCCallback matched, void *matchedParam,
                                  DWCNewClientCallback newClient, void *newClientParam, DWCAttemptFunc attempt,
                                  int *attemptParam, void *param) {
    s32 profileId;
    int error;
    int statusIndex = -1;
    char value[4];
    GPBuddyStatus status;
    u8 scm;
    u8 scn;

    if (DWCi_IsError() || sMatchRoot.fm->state != 3) {
        return FALSE;
    }
    DWCi_ClearGT2ConnectionList();
    sMatchRoot.fm->matchedCallback = matched;
    sMatchRoot.fm->matchedParam = matchedParam;
    DWCi_SetState(4);
    profileId = DWCi_GetProfileIDFromList(friendIndex);
    if (profileId == 0 || gpIsBuddy(&sMatchRoot.fm->gpObj, profileId) == 0) {
        error = 0xb;
        goto fail;
    }
    gpGetBuddyIndex(&sMatchRoot.fm->gpObj, profileId, &statusIndex);
    gpGetBuddyStatus(&sMatchRoot.fm->gpObj, statusIndex, &status);
    if (status.status != 6) {
        error = 0xb;
        goto fail;
    }
    value[0] = '0';
    if (DWCi_GetBuddyStatusLCK(&status) != 0) {
        error = 0x15;
        goto fail;
    }
    DWC_GetCommonValueString("SCM", value, status.statusString, '/');
    scm = strtoul(value, NULL, 10);
    DWC_GetCommonValueString("SCN", value, status.statusString, '/');
    scn = strtoul(value, NULL, 10);
    if (scn == scm) {
        error = 0xc;
        goto fail;
    }
    DWCi_SetMatchType(3);
    DWCi_ConnectToGameServerAsync(type, profileId, DWCi_MatchedCallback, NULL, newClient, newClientParam, attempt,
                                  attemptParam, param);
    return TRUE;
fail:
    DWCi_SetError(error, 0);
    sMatchRoot.fm->matchedCallback(error, FALSE, TRUE, FALSE, 0, sMatchRoot.fm->matchedParam);
    if (sMatchRoot.fm != NULL && sMatchRoot.fm->state == 4) {
        DWCi_SetState(3);
        DWCi_GPSetServerStatus(1);
    }
    return TRUE;
}

BOOL DWC_SetConnectionClosedCallback(DWCConnectionClosedCallback func, void *param) {
    if (sMatchRoot.fm == NULL) {
        return FALSE;
    }
    sMatchRoot.fm->closedCallback = func;
    sMatchRoot.fm->closedParam = param;
    return TRUE;
}

int DWC_CloseAllConnectionsHard(void) {
    BOOL found = FALSE;
    int i;

    sMatchRoot.fm->matchcnt.suppressMatchedCallback = TRUE;
    if (func_ov011_02160dac()) {
        return 1;
    }
    if (sMatchRoot.fm == NULL || DWCi_IsError() || (sMatchRoot.fm->state != 4 && sMatchRoot.fm->state != 5)) {
        return -1;
    }
    for (i = 0; i < 0x20; i++) {
        if (sConnections[i] != NULL) {
            found = TRUE;
            break;
        }
    }
    if (!found) {
        DWCi_GetNumNodeInfo();
        DWCi_GPSetServerStatus(1);
        DWCi_NNFreeNegotiateList();
        DWCi_SetState(3);
        DWCi_ClearQR2Key();
        DWCi_InitNodeInfoList();
        DWCi_SetMatchStatus(0);
        DWCi_CloseMatching();
        return 1;
    }
    sMatchRoot.fm->ownCloseFlag = 1;
    gt2CloseAllConnectionsHard(sMatchRoot.fm->gt2Socket);
    while (DWCi_GetNumNodeInfo() > 1) {
        for (i = 0; i < DWCi_GetNumNodeInfo(); i++) {
            DWCNodeInfo *host = DWCi_GetNodeInfoByIndex(i);

            if (host->profileId != sMatchRoot.fm->profileID) {
                DWCi_HandleClosedConnection(NULL, 0, 1, host->aid);
            }
        }
    }
    sMatchRoot.fm->ownCloseFlag = 0;
    DWCi_InitNodeInfoList();
    DWCi_SetMatchStatus(0);
    DWCi_CloseMatching();
    if (DWC_GetState() != 3) {
        DWCi_NNFreeNegotiateList();
        DWCi_SetState(3);
        DWCi_ClearQR2Key();
    }
    return 0;
}

int DWC_CloseConnectionHard(u8 netId) {
    if (DWC_IsServerMyself()) {
        return DWCi_CloseConnectionHard(netId);
    }
    return -3;
}

int DWC_GetNumConnectionHost(void) {
    if (sMatchRoot.fm == NULL || !func_ov011_02160d88()) {
        return 0;
    }
    return DWCi_GetNumNodeInfo();
}

u8 DWC_GetMyAID(void) {
    DWCNodeInfo *host;

    if (sMatchRoot.fm == NULL || !func_ov011_02160d88()) {
        return 0xff;
    }
    host = DWCi_GetNodeInfoByProfileID(sMatchRoot.fm->profileID);
    if (host == NULL) {
        return 0xff;
    }
    return host->aid;
}

int DWC_GetAIDList(u8 **netIds) {
    int i;

    if (sMatchRoot.fm == NULL || !func_ov011_02160d88()) {
        return 0;
    }
    MI_CpuFill8(sNetIds, 0, sizeof(sNetIds));
    for (i = 0; i < DWCi_GetNumNodeInfo(); i++) {
        sNetIds[i] = DWCi_GetNodeInfoByIndex(i)->aid;
    }
    *netIds = sNetIds;
    return DWCi_GetNumNodeInfo();
}

u32 DWC_GetAIDBitmap(void) {
    u8 *netIds;

    if (sMatchRoot.fm == NULL || !func_ov011_02160d88()) {
        return 0;
    }
    return DWCi_GetAIDBitmapFromList(netIds, DWC_GetAIDList(&netIds));
}

BOOL DWC_IsValidAID(u8 netId) {
    if (sMatchRoot.fm == NULL) {
        return FALSE;
    }
    if (netId == DWC_GetMyAID()) {
        return FALSE;
    }
    if (DWCi_GetNodeInfoByAID(netId) != NULL) {
        return TRUE;
    }
    return FALSE;
}

int DWC_GetState(void) {
    if (sMatchRoot.fm != NULL) {
        return sMatchRoot.fm->state;
    }
    return 0;
}

static u8 func_ov011_021603b8(void) {
    DWCNodeInfo *host;

    if (sMatchRoot.fm == NULL || sMatchRoot.fm->matchcnt.state == 0 || sMatchRoot.fm->matchcnt.state == 9 ||
        sMatchRoot.fm->matchcnt.state == 10 || sMatchRoot.fm->matchcnt.state == 11 ||
        sMatchRoot.fm->matchcnt.matched == 0 || sMatchRoot.fm->matchcnt.hostState == 0) {
        return 0xff;
    }
    host = DWCi_GetNodeInfoByIndex(0);
    if (host == NULL) {
        return 0xff;
    }
    return host->aid;
}

BOOL DWC_IsServerMyself(void) {
    if (sMatchRoot.fm == NULL) {
        return FALSE;
    }
    if (func_ov011_021603b8() == 0xff) {
        return FALSE;
    }
    if (sMatchRoot.fm->matchcnt.hostState == 2) {
        return TRUE;
    }
    return FALSE;
}

int DWCi_GT2Startup(void) {
    int result;
    u16 port;

    if (sMatchRoot.fm->gt2Socket != NULL) {
        return 0;
    }
    port = DWCi_GetMathRand32(0x4000) + 0xc000;
    result = gt2CreateSocket(&sMatchRoot.fm->gt2Socket, gt2AddressToString(0, port, NULL),
                             sMatchRoot.fm->gt2SendBufSize, sMatchRoot.fm->gt2RecvBufSize, DWCi_GT2SocketErrorCallback);
    if (DWCi_HandleGT2Error(result)) {
        return result;
    }
    gt2Listen(sMatchRoot.fm->gt2Socket, DWCi_GT2ConnectAttemptCallback);
    gt2SetUnrecognizedMessageCallback(sMatchRoot.fm->gt2Socket, DWCi_GT2UnrecognizedMessageCallback);
    return result;
}

GT2Connection DWCi_GetGT2Connection(u8 aid) {
    int i;

    if (sMatchRoot.fm == NULL) {
        return NULL;
    }
    for (i = 0; i < 0x20; i++) {
        if (sConnections[i] != NULL && aid == ((MatchConnectionInfo *)gt2GetConnectionData(sConnections[i]))->aid) {
            return sConnections[i];
        }
    }
    return NULL;
}

u8 DWCi_GetConnectionAID(GT2Connection connection) {
    MatchConnectionInfo *info = gt2GetConnectionData(connection);

    if (info != NULL) {
        return info->aid;
    }
    return 0xff;
}

int DWCi_GT2GetConnectionListIdx(void) {
    int i;

    for (i = 0; i < 0x20; i++) {
        if (sConnections[i] == NULL) {
            return i;
        }
    }
    return -1;
}

static void DWCi_ClearGT2ConnectionList(void) {
    MIi_CpuClear32(0, sConnections, sizeof(sConnections));
    MIi_CpuClear32(0, sConnectionInfos, sizeof(sConnectionInfos));
}

GT2Connection *DWCi_GetGT2ConnectionByIdx(int index) {
    return &sConnections[index];
}

GT2Connection *DWCi_GetGT2ConnectionByProfileID(s32 profileId, int a1) {
    int i;

    for (i = 0; i < 0x20; i++) {
        if (sConnections[i] != NULL &&
            profileId == ((MatchConnectionInfo *)gt2GetConnectionData(sConnections[i]))->profileId) {
            return DWCi_GetGT2ConnectionByIdx(i);
        }
    }
    return NULL;
}

MatchConnectionInfo *DWCi_GetConnectionInfoByIdx(int index) {
    return &sConnectionInfos[index];
}

BOOL DWCi_IsValidAID(u8 netId) {
    int i;

    for (i = 0; i < 0x20; i++) {
        if (sConnections[i] != NULL && netId == ((MatchConnectionInfo *)gt2GetConnectionData(sConnections[i]))->aid) {
            return TRUE;
        }
    }
    return FALSE;
}

static void DWCi_SetState(int state) {
    sMatchRoot.fm->lastState = sMatchRoot.fm->state;
    sMatchRoot.fm->state = state;
}

static s32 DWCi_DeleteAID(u8 netId) {
    DWCNodeInfo *host = DWCi_GetNodeInfoByAID(netId);
    s32 profileId;

    if (host == NULL) {
        return 0;
    }
    profileId = host->profileId;
    DWCi_DeleteNodeInfoByAID(netId);
    return profileId;
}

static u32 DWCi_GetAIDBitmapFromList(const u8 *netIds, int count) {
    u32 mask = 0;
    int i = 0;

    for (; i < count; i++) {
        mask |= 1 << netIds[i];
    }
    return mask;
}

static int DWCi_HandleGPError(int result) {
    int type;
    int code;

    if (result == 0) {
        return 0;
    }
    switch (result) {
    case 1:
        type = 9;
        code = -1;
        break;
    case 2:
        type = 9;
        code = -2;
        break;
    case 3:
        type = 6;
        code = -10;
        break;
    case 4:
        type = 6;
        code = -20;
        break;
    }
    switch (sMatchRoot.fm->state) {
    case 1:
        code -= 61000;
        DWCi_StopLogin(type, code);
        break;
    case 2:
        code -= 61000;
        if (sMatchRoot.fm->logcnt.state < 1) {
            DWCi_StopLogin(type, code);
        }
        break;
    case 4:
        code -= 81000;
        DWCi_StopMatching(type, code);
        break;
    default:
        code -= 91000;
        break;
    }
    DWCi_StopFriendProcess(type, code);
    return result;
}

static GT2Result DWCi_HandleGT2Error(GT2Result result) {
    int type;
    int code;

    if (result == 0) {
        return 0;
    }
    switch (result) {
    case 1:
        type = 9;
        code = -1;
        break;
    case 2:
    case 5:
        type = 0;
        code = 0;
        result = 0;
        break;
    case 3:
        type = 6;
        code = -10;
        break;
    case 4:
        type = 6;
        code = -30;
        break;
    case 6:
        type = 6;
        code = -70;
        break;
    case 7:
        type = 6;
        code = -80;
        break;
    }
    if (type != 0) {
        DWCi_StopLogin(type, code - 67000);
    }
    return result;
}

int DWCi_CloseConnectionHard(u8 netId) {
    DWCNodeInfo *host;
    void *conn;

    if (sMatchRoot.fm == NULL || DWCi_IsError() || (sMatchRoot.fm->state != 4 && sMatchRoot.fm->state != 5)) {
        return -1;
    }
    conn = DWCi_GetGT2Connection(netId);
    if (conn == NULL) {
        host = DWCi_GetNodeInfoByAID(netId);
        if (host == NULL) {
            return -2;
        }
        DWCi_HandleClosedConnection(NULL, 0, 1, host->aid);
        return 0;
    }
    gt2CloseConnectionHard(conn);
    return 0;
}

static void DWCi_LoginCallback(DWCError error, int profileId, void *param) {
    if (error == 0) {
        sMatchRoot.fm->profileID = profileId;
        DWCi_SetState(3);
        DWCi_InitGPProcessCount();
    } else {
        DWCi_SetState(0);
    }
    if (sMatchRoot.fm->loginCallback != NULL) {
        sMatchRoot.fm->loginCallback(error, profileId, sMatchRoot.fm->loginParam);
    }
}

static void DWCi_UpdateServersCallback(DWCError error, BOOL isChanged, void *param) {
    sMatchRoot.fm->updateServersCallback(error, isChanged, sMatchRoot.fm->updateServersParam);
}

static void DWCi_MatchedCallback(DWCError error, BOOL cancel, BOOL a2, BOOL a3, int index, void *param) {
    if (error == 0 && cancel) {
        if (sMatchRoot.fm->matchcnt.state == 0) {
            DWCi_SetState(3);
            DWCi_ClearQR2Key();
        }
    } else if (error == 0) {
        DWCi_SetState(5);
    }
    if (sMatchRoot.fm->matchcnt.suppressMatchedCallback == 0) {
        sMatchRoot.fm->matchedCallback(error, cancel, a2, a3, index, sMatchRoot.fm->matchedParam);
    }
    if (error != 0 && sMatchRoot.fm != NULL && (sMatchRoot.fm->state == 4 || sMatchRoot.fm->state == 5)) {
        DWCi_SetState(3);
    }
}

static void DWCi_GPErrorCallback(GPConnection *connection, void *arg, void *param) {
    GPErrorArg *error = arg;

    if (error->errorCode != 0x603 && error->errorCode != 0x901 && error->errorCode != 0xb01) {
        DWCi_HandleGPError(3);
    }
}

static void DWCi_GPRecvBuddyMessageCallback(GPConnection *connection, void *arg, void *param) {
    GPRecvBuddyMessageArg *message = arg;
    char version[12] = { 0 };
    const char *text = message->message;
    u32 len;
    char *end;

    if (memcmp(text, "GPCM", strlen("GPCM")) == 0) {
        text += strlen("GPCM");
        end = strchr(text, 'v');
        len = end - text;
        strncpy(version, text, len);
        if (len <= 10 && strtoul(version, NULL, 10) == 0x5a) {
            text += len + 1;
            if (memcmp(text, "MAT", strlen("MAT")) == 0) {
                if (sMatchRoot.fm->state == 4 || (sMatchRoot.fm->state == 5 && (sMatchRoot.fm->matchcnt.mode == 2 ||
                                                                                sMatchRoot.fm->matchcnt.mode == 3))) {
                    DWCi_MatchGPRecvBuddyMsgCallback(connection, message->profile, text + strlen("MAT"));
                }
            }
        }
    }
}

static void DWCi_GT2ReceivedCallback(GT2Connection connection, GT2Byte *message, int length, GT2Bool reliable) {
    DWCi_RecvCallback(connection, message, length, reliable);
}

static void DWCi_HandleClosedConnection(GT2Connection conn, int closeType, int isNetId, u8 netId) {
    u8 id;
    int profileId = 0;
    int errorCode;
    BOOL isHost = FALSE;
    BOOL hadHost;
    int error;
    MatchConnectionInfo *connection;
    int i;
    DWCNodeInfo *host;

    if (DWCi_IsShutdownMatch() != 0) {
        return;
    }
    switch (closeType) {
    case 0:
    case 1:
        error = 0;
        errorCode = 0;
        break;
    case 2:
    case 3:
        error = 6;
        errorCode = -7600;
        break;
    case 4:
        error = 9;
        errorCode = -7601;
        break;
    }
    if (error == 0) {
        if (!isNetId) {
            connection = gt2GetConnectionData(conn);
            if (connection == NULL) {
                return;
            }
            id = connection->aid;
        } else {
            id = netId;
        }
        if (DWCi_GetNodeInfoByAID(id) != NULL) {
            hadHost = TRUE;
        } else {
            hadHost = FALSE;
        }
        if (!isNetId) {
            DWCi_ClearTransConnection(id);
        }
        if (DWCi_GetServerNodeInfo() != NULL && id == DWCi_GetServerNodeInfo()->aid) {
            isHost = TRUE;
        }
        profileId = DWCi_DeleteAID(id);
        sMatchRoot.fm->matchcnt.suspendAidBitmap &= ~(1 << id);
        if (sMatchRoot.fm->matchcnt.suspendPending != 0 && isHost && DWCi_IsServerNodeMyself() == 0) {
            sMatchRoot.fm->matchcnt.suspendResend = 1;
        }
        if (profileId == 0) {
            profileId = ((MatchConnectionInfo *)gt2GetConnectionData(DWCi_GetGT2Connection(id)))->profileId;
        }
        if (!isNetId) {
            sConnections[connection->index] = NULL;
        }
    }
    if (sMatchRoot.fm->matchcnt.unlocked == 1 && DWC_IsServerMyself()) {
        sMatchRoot.fm->matchcnt.unlocked = 1;
    } else if (sMatchRoot.fm->matchcnt.qr2MatchType == 3 || sMatchRoot.fm->matchcnt.qr2MatchType == 2) {
        sMatchRoot.fm->matchcnt.unlocked = 0;
    } else if (DWC_GetNumConnectionHost() == 1) {
        sMatchRoot.fm->matchcnt.unlocked = 1;
    } else {
        sMatchRoot.fm->matchcnt.unlocked = 0;
    }
    if (DWCi_IsServerNodeMyself() != 0) {
        sMatchRoot.fm->matchcnt.connectedAidBit = 0;
        for (i = 0; i < DWCi_GetNumNodeInfo();) {
            host = DWCi_GetNodeInfoByIndex(i);
            if (host->profileId != sMatchRoot.fm->matchcnt.profileID && DWCi_GetGT2Connection(host->aid) == NULL) {
                if (sMatchRoot.fm->closedCallback != NULL) {
                    sMatchRoot.fm->closedCallback(error, closeType == 0, isHost, host->aid,
                                                  DWCi_GetFriendListIndex(host->profileId), sMatchRoot.fm->closedParam);
                }
                DWCi_DeleteAID(host->aid);
            } else {
                sMatchRoot.fm->matchcnt.connectedAidBit |= 1 << DWCi_GetNodeInfoByIndex(i)->aid;
                i++;
            }
        }
    }
    if (closeType != 0 && DWC_IsServerMyself() == 1) {
        sMatchRoot.fm->matchcnt.aidListSendTick = 0;
        if (func_ov011_021686a0() != 0 && sMatchRoot.fm->matchcnt.state == 0x10 &&
            sMatchRoot.fm->matchcnt.cmdCommand == 8) {
            sMatchRoot.fm->matchcnt.cmdSendTime = 0;
        }
    }
    if (sMatchRoot.fm->ownCloseFlag == 0 && sMatchRoot.fm->state == 5 && !hadHost) {
        if (sMatchRoot.fm->matchcnt.mode != 2) {
            return;
        }
        if (error != 0) {
            return;
        }
        DWCi_GPSetServerStatus(-1);
        DWCi_ProcessMatchSCClosing(profileId);
        return;
    }
    if (DWCi_ProcessMatchClosing(error, errorCode, profileId) != 0) {
        return;
    }
    if (error != 0) {
        DWCi_SetError(error, errorCode - 80000);
        return;
    }
    if (sMatchRoot.fm->matchcnt.mode == 2) {
        if (sMatchRoot.fm->ownCloseFlag == 0) {
            DWCi_GPSetServerStatus(-1);
        } else if (DWCi_GetNumNodeInfo() == 1) {
            DWCi_GPSetServerStatus(1);
        }
    } else if (DWCi_GetNumNodeInfo() == 1) {
        DWCi_GPSetServerStatus(1);
    }
    if (sMatchRoot.fm->matchcnt.hostState == 2) {
        qr2_send_statechanged(sMatchRoot.fm->matchcnt.qr2Obj);
    }
    if (sMatchRoot.fm->closedCallback != NULL && hadHost) {
        sMatchRoot.fm->closedCallback(error, closeType == 0, isHost, id, DWCi_GetFriendListIndex(profileId),
                                      sMatchRoot.fm->closedParam);
    }
    if (sMatchRoot.fm->ownCloseFlag == 0 && sMatchRoot.fm->matchcnt.mode == 2) {
        return;
    }
    if (func_ov011_02168468() == 1) {
        DWCi_SetMatchStatus(1);
    }
    if (DWCi_GetNumNodeInfo() == 1) {
        if (sMatchRoot.fm->ownCloseFlag != 0) {
            DWCi_NNFreeNegotiateList();
            DWCi_SetState(3);
            DWCi_ClearQR2Key();
            sMatchRoot.fm->matchcnt.hostState = 0;
            return;
        }
        DWCi_SetMatchStatus(0xb);
        return;
    }
    if (sMatchRoot.fm->matchcnt.mode == 3 && isHost && DWCi_GetNumNodeInfo() >= 1) {
        if (DWCi_GetNumNodeInfo() == 1 && func_ov011_02167fd0() != 0) {
            DWCi_SetMatchStatus(0xb);
            return;
        }
        DWCi_SetMatchStatus(9);
    }
}

static void DWCi_GT2ClosedCallback(GT2Connection conn, GT2CloseReason closeType) {
    DWCi_HandleClosedConnection(conn, closeType, 0, 0xff);
}

static void DWCi_GT2PingCallback(GT2Connection connection, int latency) {
    DWCi_PingCallback(connection, latency);
}

static void DWCi_GT2SocketErrorCallback(GT2Socket socket) {
    sMatchRoot.socketError = GOAGetLastError(gt2GetSocketSOCKET(socket));
    DWCi_SetError(9, -97003);
    sMatchRoot.fm->gt2Socket = NULL;
}

static void DWCi_SetMatchType(u8 mode) {
    sMatchRoot.fm->matchcnt.qr2MatchType = mode;
    sMatchRoot.fm->matchcnt.unk_798 = 0;
}

u8 DWCi_GetServerOrSubServerAID(void) {
    u8 netId = func_ov011_021603b8();
    DWCNodeInfo *host;

    if (netId == 0xff) {
        host = DWCi_GetNodeInfoByIndex(0);
        if (host != NULL) {
            netId = host->aid;
        }
    }
    return netId;
}

static BOOL func_ov011_02160d88(void) {
    BOOL result = TRUE;

    if (sMatchRoot.fm->matchcnt.qr2MatchType != 2 && sMatchRoot.fm->matchcnt.matched == 0) {
        result = FALSE;
    }
    return result;
}

static BOOL func_ov011_02160dac(void) {
    BOOL result;

    if (DWC_IsValidCancelMatching()) {
        DWCi_DoCancelMatching();
        result = TRUE;
    } else {
        DWCi_GetMatchCnt();
        result = FALSE;
    }
    DWCi_GetMatchCnt()->suppressMatchedCallback = 0;
    return result;
}

static BOOL DWC_IsValidCancelMatching(void) {
    if (DWCi_IsError()) {
        return FALSE;
    }
    if (DWCi_GetMatchCnt() == NULL || DWC_GetState() != 4 || DWCi_GetMatchCnt()->cancelState != 0) {
        return FALSE;
    }
    return TRUE;
}
