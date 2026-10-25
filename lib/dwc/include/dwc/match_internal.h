#ifndef POKEBW2_DWC_MATCH_INTERNAL_H
#define POKEBW2_DWC_MATCH_INTERNAL_H

#include "dwc/login.h"
#include "dwc/match.h"
#include "dwc/qr2.h"
#include "dwc/sb_serverbrowsing.h"
#include "dwc/transport.h"

// The control shared by DWC's friends-match front end (dwc_main.c) and matching core (dwc_match.c), and the internal
// calls between them. Their separate string pools and groups of file statics identify two original source files.

// What a GT2 connection to another machine keeps as its data: its slot in the table of connections, the machine's aid
// and its profile ID
typedef struct MatchConnectionInfo {
    u8 index;
    u8 aid;
    u8 unk_02[2];
    s32 profileId;
} MatchConnectionInfo;

// The console that a NAT negotiation goes to, and how often it was tried again (the SDK's DWCNNInfo)
typedef struct MatchTarget {
    u8 kind;
    u8 retryCount;
    u16 port;
    u32 ip;
    u32 cookie;
} MatchTarget;

// Called when a suspension of the match ends, with whether it was canceled
typedef void (*MatchCallback)(int, int, int);
typedef void (*MatchNotifyFunc)(DWCError error, BOOL cancel, BOOL self, BOOL isServer, int index, void *param);
#ifdef PLATFORM_PC
// The public API supplies DWCAttemptFunc (one argument); call through that exact type on a host.
typedef DWCAttemptFunc MatchCheckFunc;
#else
typedef int (*MatchCheckFunc)(void *userData, u32 param);
#endif

typedef struct DWCFriendsMatchControl DWCFriendsMatchControl;

// The match control (DWCi_GetMatchCnt returns it; it is DWCFriendsMatchControl.match). The fields that the SDK's
// DWCMatchControl also has take their names from it, and its command control (cmdCnt) is the cmd fields here
typedef struct DWCMatchControl {
    GPConnection *pGpObj;
    DWCFriendsMatchControl *fm;
    GT2ConnectionCallbacks *gt2Callbacks;
    u8 retries;
    u8 matched;
    u8 unk_0e[0x2];
    qr2_t qr2Obj;
    // The API, the match state machine and the GameSpy callbacks (GP buddy messages, GT2 closes, qr2 keys) share
    // these, and the original loads them again at every test
    volatile u8 mode;
    volatile u8 qr2MatchType;
    u8 hostState;
    // Read in source order beside mode and qr2MatchType, as the GameSpy callbacks change it
    volatile u8 qr2NumEntry;
    u8 qr2IsReserved;
    u8 unk_19;
    u8 unk_1a[0x2];
    u32 suspendAidBitmap;
    u64 suspendSendTick;
    u32 suspend;
    s32 suspendRequest;
    u8 unk_30;
    u8 unk_31[0x3];
    MatchCallback suspendCallback;
    u32 suspendParam;
    u64 suspendStartTick;
    s32 suspendResend;
    s32 suspendPending;
    u8 qr2ShutdownFlag;
    u8 unk_4d[0x3];
    s32 nodeCount;
    DWCNodeInfo nodes[0x20];
    DWCNodeInfo self;
    DWCNodeInfo pending;
    s32 memberConnecting;
    s32 abortedMemberState;
    DWCNodeInfo abortedMember;
    u16 qr2Port;
    u8 unk_5d6[0x2];
    s32 qr2IP;
    s32 qr2Reservation;
    ServerBrowser sbObj;
    s32 sbUpdateFlag;
    u64 sbUpdateTick;
    u64 sbUpdateRequestTick;
    u8 nnRecvCount;
    u8 nnFailureCount;
    u16 nnCookieRand;
    s32 nnLastCookie;
    u64 nnFailedTime;
    u64 nnFinishTime;
    MatchTarget target;
    s32 state;
    u8 clLinkProgress;
    u8 friendCount;
    u8 unk_622[0x6];
    u8 unk_628;
    u8 closeState;
    u8 cancelState;
    u8 scResvRetryCount;
    u8 synResendCount;
    u8 cancelSynResendCount;
    u8 clWaitTimeoutCount;
    u8 unk_62f;
    u16 baseLatency;
    u16 cancelBaseLatency;
    s32 resvToken;
    s32 cmdResendFlag;
    u64 cmdResendTick;
    u32 cmdTimeoutTime;
    u64 cmdTimeoutStartTick;
    u32 synAckBit;
    u32 cancelSynAckBit;
    u64 lastSynSent;
    u64 lastCancelSynSent;
    u64 closedTime;
    u64 unk_670;
    // Read in source order beside mode and qr2MatchType, as the GameSpy callbacks change it
    volatile s32 profileID;
    s32 reqProfileID;
    s32 cbEventPid;
    char *gameName;
    char *secretKey;
    DWCFriendData *friendList;
    s32 friendListLen;
    u8 friendIdxList[0x40];
    s32 friendIdxListLen;
    u8 cmdCommand;
    u8 cmdCount;
    u16 cmdPort;
    u32 cmdIp;
    u32 cmdData[0x20];
    u32 cmdProfileId;
    u32 cmdLen;
    u64 cmdSendTime;
    MatchNotifyFunc notifyFunc;
    void *notifyParam;
    DWCNewClientCallback newClientCallback;
    void *newClientParam;
    DWCEvalPlayerCallback evalCallback;
    void *evalParam;
    u32 newHostAckBit;
    u32 connectedAidBit;
    s32 groupId;
    u8 groupSearchCount;
    u8 unk_795[0x3];
    s32 unk_798;
    u8 newHostResendCount;
    u8 newHostNakCount;
    u8 unk_79e[0x2];
    u64 aidListSendTick;
    u8 sbSearchHostsOnly;
    u8 unk_7a9[0x3];
    u32 sbSearchCount;
    s32 topology;
    u8 error;
    u8 suppressMatchedCallback;
    u8 scResvRetryMax;
    u8 unk_7b7;
    MatchCheckFunc checkFunc;
    int userData;
    uptr checkParam; // Opaque local callback pointer; never part of a match packet.
    u32 unlocked;
    u32 synId;
    u64 suspendWaitTime;
    s32 lockEnabled;
    DWCTransportInfo transport;
} DWCMatchControl;

// The friends match control, which DWC_InitFriendsMatch allocates: the GT2 socket that the consoles talk through and
// the callbacks of its connections, the GameSpy Presence connection, the user data, the state (0 before the login, 1
// checking that the services are up, 2 logging in, 3 logged in, 4 matching, 5 matched), the player's name and profile
// ID, the game's callbacks and the login, friend list and match parts
struct DWCFriendsMatchControl {
    GTI2Socket *gt2Socket;
    GT2ConnectionCallbacks gt2Callbacks;
    int gt2SendBufSize;
    int gt2RecvBufSize;
    GPConnection gpObj;
    DWCUserData *userdata;
    int state;
    int lastState;
    u8 unk_2c;
    u8 ownCloseFlag;
    u8 unk_2e[0x2];
    u16 playerName[0x1a];
    s32 profileID;
    char *gameName;
    char *secretKey;
    DWCLoginCallback loginCallback;
    void *loginParam;
    DWCUpdateServersCallback updateServersCallback;
    void *updateServersParam;
    DWCMatchedSCCallback matchedCallback;
    void *matchedParam;
    DWCConnectionClosedCallback closedCallback;
    void *closedParam;
    DWCLoginControl logcnt;
    DWCFriendControl friendcnt;
    DWCMatchControl matchcnt;
};

// Calls shared by the friends-match front end and matching core. The public calls remain declared in dwc/match.h.
int DWC_GetState(void);
int DWCi_GT2GetConnectionListIdx(void);
GT2Connection *DWCi_GetGT2ConnectionByIdx(int index);
GT2Connection *DWCi_GetGT2ConnectionByProfileID(s32 profileId, int a1);
MatchConnectionInfo *DWCi_GetConnectionInfoByIdx(int index);
void DWCi_MatchInit(DWCMatchControl *cnt, GPConnection *gp, DWCFriendsMatchControl *fm,
                    GT2ConnectionCallbacks *callbacks, char *gameName, char *secretKey, DWCFriendData *friendList,
                    int friendListLen);
void DWCi_ConnectToAnybodyAsync(int type, u8 numEntry, const char *query, DWCMatchedSCCallback matched,
                                void *matchedParam, DWCNewClientCallback newClient, void *newClientParam,
                                DWCEvalPlayerCallback eval, void *evalParam, DWCAttemptFunc attempt, int *attemptParam,
                                void *param);
void DWCi_SetupGameServer(int type, u8 numEntry, DWCMatchedSCCallback matched, void *matchedParam,
                          DWCNewClientCallback newClient, void *newClientParam, DWCAttemptFunc attempt,
                          int *attemptParam, void *param);
void DWCi_ConnectToGameServerAsync(int type, s32 profileId, DWCMatchedSCCallback matched, void *matchedParam,
                                   DWCNewClientCallback newClient, void *newClientParam, DWCAttemptFunc attempt,
                                   int *attemptParam, void *param);
void DWCi_MatchProcess(int a0);
GT2Bool DWCi_GT2UnrecognizedMessageCallback(GT2Socket socket, unsigned int ip, unsigned short port, GT2Byte *message,
                                            int length);
void DWCi_GT2ConnectAttemptCallback(GT2Socket socket, GT2Connection connection, unsigned int ip, unsigned short port,
                                    int latency, GT2Byte *message, int length);
void DWCi_GT2ConnectedCallback(GT2Connection connection, GT2Result result, GT2Byte *message, int length);
void DWCi_MatchGPRecvBuddyMsgCallback(void *unused, int id, u8 *str);
void DWCi_StopMatching(int type, int code);
void DWCi_ClearQR2Key(void);
BOOL DWCi_ProcessMatchClosing(int type, u32 code, s32 profileId);
void DWCi_ProcessMatchSCClosing(u32 id);
void DWCi_ShutdownMatch(void);
BOOL DWCi_IsShutdownMatch(void);
void DWCi_CloseMatching(void);
void DWCi_DoCancelMatching(void);
DWCMatchControl *DWCi_GetMatchCnt(void);
void DWCi_SetMatchStatus(int state);
BOOL DWCi_IsServerChangeEnabled(void);
void DWCi_InitNodeInfoList(void);
BOOL DWCi_DeleteNodeInfoByAID(int index);
DWCNodeInfo *DWCi_GetServerNodeInfo(void);
DWCNodeInfo *DWCi_GetNodeInfoByProfileID(int id);
DWCNodeInfo *DWCi_GetNodeInfoByAID(u8 index);
BOOL DWCi_StopMeshMaking(void);
BOOL DWCi_IsFullMesh(void);
void DWCi_NNFreeNegotiateList(void);

#endif
