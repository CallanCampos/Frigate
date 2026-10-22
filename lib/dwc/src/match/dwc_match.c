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
#include "dwc/memfunc.h"
#include "dwc/natneg.h"
#include "dwc/qr2.h"
#include "dwc/sb_server.h"
#include "dwc/sb_serverbrowsing.h"
#include "dwc/transport.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// DWC's matchmaking (dwc_match.c, a guessed name; the ROM names none of it, and the file is Thumb code in overlay 11
// like the rest of the library's online part): the friends match control that the game starts, logs in and runs every
// frame, and the matchmaking under it. A match is made with anybody, through the server browser's list of the games
// that GameSpy's master server has, as a server that friends connect to, or with a friend as the client of that
// friend's server. The consoles find each other's public address through query and reporting (qr2) and NAT negotiation,
// send their commands as GameSpy Presence buddy messages ("GPCM90vMAT", the command and its data in Base64) or as
// packets with the magic "\xbbI\xccM" on their GT2 socket, and once connected they talk through GT2 connections. Its
// strings, such as "dwc_pid" and "Game server is fully occupied.", are its own. Its functions are named after the
// SDK's dwc_main.c and dwc_match.c (see dwc/match.h), whose statics DWCi_HandleGPError and DWCi_HandleGT2Error are
// DWCi_HandleMatchGPError and DWCi_HandleMatchGT2Error for the second's here.

// The byte order of the network
#define MATCH_SWAP16(x) ((u16)((u8)((x) >> 8) | ((x) << 8 & 0xff00)))

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
typedef int (*MatchCheckFunc)(void *userData, u32 param);

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
    u8 mode;
    u8 qr2MatchType;
    u8 hostState;
    u8 qr2NumEntry;
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
    s32 profileID;
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
    u32 checkParam;
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

// The library's one friends match control (sMatchRoot.fm, which DWC_InitFriendsMatch makes), and the socket error that
// closed its GT2 socket
typedef struct MatchRoot {
    int socketError;
    DWCFriendsMatchControl *fm;
} MatchRoot;

// What DWCi_GetMatchCnt returns (sMatchState.control), the query text of the match, and the clock of the last
// state change; and, as the SDK's file statics of those names, whether NAT negotiation is being run and its list is to
// be freed once it returns, and whether the server browser is to be freed once its callbacks, counted, return
typedef struct MatchState {
    u32 inNNThink;
    DWCMatchControl *control;
    u32 nnFreeNegotiateList;
    char *query;
    u32 needSbFree;
    u32 sbCallbackLevel;
    u64 clock;
} MatchState;

// A key that is added to the match (key IDs 100 to 253): its ID (0 when the slot is free), whether its value is a
// text, and the key's name and value (the SDK's DWCGameMatchKeyData)
typedef struct MatchKey {
    u8 id;
    u8 isText;
    u16 pad;
    char *name;
    const void *value;
} MatchKey;

// A peer as the other console sends it
typedef struct MatchPeerWire {
    u32 packed;
    u32 id;
    u32 ip;
    u32 localIp;
    u32 ports;
    u8 userData[4];
} MatchPeerWire;

// A packet of the matchmaking: the magic "\xbbI\xccM", the version, the command, the size of its data, the public
// port and address that the sender's qr2 reports and the sender's profile ID; and the data, up to 32 words
typedef struct MatchPacketHeader {
    char magic[4];
    s32 version;
    u8 command;
    u8 size;
    u16 port;
    u32 ip;
    s32 profileId;
} MatchPacketHeader;

typedef struct MatchPacket {
    MatchPacketHeader header;
    u32 data[0x20];
} MatchPacket;

static MatchRoot sMatchRoot;
static u8 sNetIds[0x20];
static GT2Connection sConnections[0x20];
static MatchConnectionInfo sConnectionInfos[0x20];
static MatchState sMatchState;
static MatchKey sKeys[0x9a];
// The chance in percent that a random match looks for a server first (-2 never, -1 always), and how often a machine
// may fail before it is left out
static s32 sMatchChance[2] = { -2, 5 };

static int DWC_GetState(void);
static u8 func_ov011_021603b8(void);
static int DWCi_GT2GetConnectionListIdx(void);
static void DWCi_ClearGT2ConnectionList(void);
static GT2Connection *DWCi_GetGT2ConnectionByIdx(int index);
static GT2Connection *DWCi_GetGT2ConnectionByProfileID(s32 profileId, int a1);
static MatchConnectionInfo *DWCi_GetConnectionInfoByIdx(int index);
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
static BOOL func_ov011_02160fd8(void);
static void DWCi_MatchInit(DWCMatchControl *cnt, GPConnection *gp, DWCFriendsMatchControl *fm,
                           GT2ConnectionCallbacks *callbacks, char *gameName, char *secretKey,
                           DWCFriendData *friendList, int friendListLen);
static int DWCi_QR2Startup(s32 profileId);
static void DWCi_ConnectToAnybodyAsync(int type, u8 numEntry, const char *query, DWCMatchedSCCallback matched,
                                       void *matchedParam, DWCNewClientCallback newClient, void *newClientParam,
                                       DWCEvalPlayerCallback eval, void *evalParam, DWCAttemptFunc attempt,
                                       int *attemptParam, void *param);
static void DWCi_SetupGameServer(int type, u8 numEntry, DWCMatchedSCCallback matched, void *matchedParam,
                                 DWCNewClientCallback newClient, void *newClientParam, DWCAttemptFunc attempt,
                                 int *attemptParam, void *param);
static void DWCi_ConnectToGameServerAsync(int type, s32 profileId, DWCMatchedSCCallback matched, void *matchedParam,
                                          DWCNewClientCallback newClient, void *newClientParam, DWCAttemptFunc attempt,
                                          int *attemptParam, void *param);
static void DWCi_SendStateChanged(qr2_t server);
static void DWCi_MatchProcess(int a0);
static GT2Bool DWCi_GT2UnrecognizedMessageCallback(GT2Socket socket, unsigned int ip, unsigned short port,
                                                   GT2Byte *message, int length);
static void DWCi_GT2ConnectAttemptCallback(GT2Socket socket, GT2Connection connection, unsigned int ip,
                                           unsigned short port, int latency, GT2Byte *message, int length);
static void DWCi_FormatProfileID(char *dest, u32 size);
static void DWCi_GT2ConnectedCallback(GT2Connection connection, GT2Result result, GT2Byte *message, int length);
static void DWCi_MatchGPRecvBuddyMsgCallback(void *unused, int id, u8 *str);
static void DWCi_StopResendingMatchCommand(void);
static void DWCi_StopMatching(int type, int code);
static void DWCi_ClearQR2Key(void);
static void func_ov011_02162958(u8 conn, int kind, u32 *msg);
static void func_ov011_02162bf8(u8 conn, int kind, u32 *msg);
static void DWCi_CloseConnectionsNotInBitmap(u32 mask);
static void DWCi_ProcessServerAIDBitmapCommand(const u32 *msg);
static BOOL DWCi_ProcessMatchClosing(int type, u32 code, s32 profileId);
static void DWCi_ProcessMatchSCClosing(u32 id);
static void DWCi_ShutdownMatch(void);
static BOOL DWCi_IsShutdownMatch(void);
static void DWCi_ResetMatchParam(int mode);
static void DWCi_SetMatchCommonParam(u8 mode, u8 unk17, DWCMatchedSCCallback callback, void *param);
static void DWCi_CloseMatching(void);
static int DWCi_SBUpdateAsync(u32 pid);
static int DWCi_GetDefaultMatchFilter(char *out, u32 a, u32 b, u32 c, u8 kind);
static BOOL IsPrivateAddr(u32 ip);
static int DWCi_NNStartupAsync(int reset, u32 token, DWCNodeInfo *peer);
static int DWCi_DoNatNegotiationAsync(MatchTarget *target);
static BOOL DWCi_CanSendMatchCommandByGT2(int type, int id);
static int DWCi_SendMatchCommand(u8 type, int id, u32 ip, u16 port, const u32 *data, int count);
static void DWCi_MakeMatchCommandPacket(MatchPacket *packet, int type, const u32 *data, int count);
static int DWCi_SendSBMsgCommand(int type, u32 ip, u16 port, const u32 *data, int count);
static int DWCi_SendGPBuddyMsgCommand(void *gp, u8 command, int id, const char *text);
static int DWCi_ProcessRecvMatchCommand(int type, s32 profileId, u32 ip, u32 port, const u32 *data, int count);
static u8 DWCi_CheckResvCommand(int profileId, int ip, u16 port, int nodeType, int skipChecks, int cookie,
                                void *userData, int blocked);
static u8 func_ov011_0216484c(int profileId, int ip, int port, int nodeType, int skipChecks, int cookie, void *userData,
                              int blocked);
static int DWCi_ProcessResvOK(int profileId, u32 ip, u32 port);
static int DWCi_HandleMatchCommandError(int error);
static int DWCi_SendResvCommand(int profileId, int flag);
static int DWCi_SendResvCommandToFriend(int a0, int a1, int a2);
static int DWCi_RetryReserving(int a0);
static int DWCi_CancelReservation(int unused);
static int func_ov011_02165094(int profileId, int a1);
static int DWCi_CancelPreConnectedServerProcess(int profileId);
static int DWCi_CancelPreConnectedClientProcess(int unused);
static void DWCi_PostProcessConnection(int a0);
static void func_ov011_0216549c(void);
static void func_ov011_021654c0(void);
static void func_ov011_02165540(void);
static int DWCi_StartMatchSyn(void);
static int DWCi_SendClientLinkRequest(void);
static void DWCi_DoCancelMatching(void);
static void DWCi_FinishCancelMatching(void);
static int DWCi_InvalidateReservation(void);
static void DWCi_RestartFromCancel(int a0);
static void DWCi_RestartFromTimeout(void);
static int DWCi_ResumeMatching(void);
static int DWCi_CloseCancelHostAsync(int profileId);
static void DWCi_CloseAllConnectionsByTimeout(void);
static int DWCi_CloseShutdownClientSC(u32 mask);
static void DWCi_SendMatchSynPacket(u8 aid, u16 type);
static int DWCi_ProcessMatchSynTimeout(void);
static int DWCi_CloseSynTimeoutClients(void);
static void DWCi_SendMatchAckToClients(void);
static int DWCi_SendCancelMatchSynCommand(int profileId, u8 message);
static int DWCi_ProcessCancelMatchSynCommand(int profileId, int message, int value);
static int DWCi_ProcessCancelMatchSynTimeout(void);
static u8 DWCi_GetNewMatchKey(void);
static void DWCi_ClearGameMatchKeys(void);
static BOOL DWCi_IsFriendByIdxList(int profileId);
static int DWCi_CheckDWCServer(SBServer server);
static u8 DWCi_GetAIDFromProfileID(int index, int a1);
static u32 DWCi_GetAIDBitmask(int excludeSelf);
static void DWCi_InitClWaitTimeout(void);
static int DWCi_HandleMatchGPError(int result);
static int DWCi_HandleSBError(int result);
static int DWCi_HandleQR2Error(int result);
static int DWCi_HandleNNError(int result);
static int DWCi_HandleNNResult(int result);
static int DWCi_HandleMatchGT2Error(int result);
static void DWCi_SBCallback(ServerBrowser sb, SBCallbackReason reason, SBServer server, void *instance);
static void DWCi_SBPrintServerData(SBServer server);
static BOOL DWCi_EvaluateServers(int sortAfter);
static void DWCi_RandomizeServers(void);
static void DWCi_QR2ServerKeyCallback(int keyId, qr2_buffer_t outBuf, void *userData);
static void DWCi_QR2PlayerKeyCallback(int keyId, int index, qr2_buffer_t outBuf, void *userData);
static void DWCi_QR2TeamKeyCallback(int keyId, int index, qr2_buffer_t outBuf, void *userData);
static void DWCi_QR2KeyListCallback(qr2_key_type keyType, qr2_keybuffer_t keyBuffer, void *userData);
static int DWCi_QR2CountCallback(qr2_key_type keyType, void *userData);
static void DWCi_QR2AddErrorCallback(qr2_error_t error, gsi_char *errorMessage, void *userData);
static void DWCi_QR2PublicAddrCallback(gsi_u32 ip, gsi_u16 port, void *userData);
static void DWCi_QR2NatnegCallback(int cookie, void *userData);
static void DWCi_QR2ClientMsgCallback(gsi_char *data, int size, void *userData);
static void DWCi_NNProgressCallback(NegotiateState state, void *userData);
static void DWCi_NNCompletedCallback(NegotiateResult result, SOCKET gameSocket, SOCKADDR_IN *address, void *userData);
static BOOL DWCi_ProcessNNFailure(int skip);
static BOOL DWCi_SendSvDownCommands(void);
static u32 DWCi_MakeGroupID(void);
static void DWCi_SetReservationCallback(MatchCheckFunc a, const void *userData, u32 c);
static void DWCi_ChangeToServer(int isHost);
static void DWCi_StartReservingServer(int profileId);
static BOOL func_ov011_021679c0(void);
static BOOL DWCi_SetSuspendAndNotify(u8 index, int set, int mode);
static BOOL DWCi_StartMatchingAfterSearchOwn(void);
static void DWCi_SetSuspendRequest(int a, MatchCallback b, int c);
static BOOL DWCi_ProcessSuspendRequest(void);
static BOOL DWCi_SendSuspendRequest(int value);
static BOOL func_ov011_02167f0c(void);
static void DWCi_SetMatchCnt(DWCMatchControl *match);
static DWCMatchControl *DWCi_GetMatchCnt(void);
static void DWCi_SetMatchStatus(int state);
static void DWCi_SetHostState(int mode);
static u8 DWCi_GetHostState(void);
static BOOL func_ov011_02167fd0(void);
static void DWCi_UpdateAllNodesRecvTime(void);
static void DWCi_InitNodeInfoList(void);
static BOOL DWCi_PushFrontNodeInfo(DWCNodeInfo *peer);
static BOOL DWCi_PushBackNodeInfo(DWCNodeInfo *peer);
static BOOL DWCi_DeleteNodeInfoByAID(int index);
static BOOL DWCi_DeleteNodeInfoByProfileID(int id);
static DWCNodeInfo *DWCi_GetOwnNodeInfo(void);
static DWCNodeInfo *DWCi_GetServerNodeInfo(void);
static DWCNodeInfo *DWCi_GetNodeInfoByProfileID(int id);
static DWCNodeInfo *DWCi_GetNodeInfoByAID(u8 index);
static u8 DWCi_GetFreeAID(void);
static BOOL DWCi_IsNodeNotConnected(int id);
static void DWCi_ClearTargetNodeInfo(void);
static BOOL DWCi_IsTargetNodeInfoValid(void);
static void DWCi_ClearNodeInfo(void *address);
static BOOL DWCi_IsValidNodeInfo(DWCNodeInfo *peer);
static u64 func_ov011_021683b8(void);
static BOOL func_ov011_02168468(void);
static int func_ov011_02168590(void);
static BOOL DWCi_IsMatchLocked(void);
static int DWCi_GetTopologyType(void);
static void func_ov011_021685fc(void);
static void DWCi_ApplyPendingSuspend(void);
static void DWCi_FinishSuspendRequest(int cancelled);
static BOOL func_ov011_021686a0(void);
static int func_ov011_021686b8(void);
static const char *DWCi_GetResvDenyReasonString(int code);
static void disp_match_state(int oldState, int newState);
static void DWCi_NNFreeNegotiateList(void);

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
            func_ov011_0217d158(sMatchRoot.fm->gt2Socket);
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

static int DWC_GetState(void) {
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
    result =
        func_ov011_0217d144(&sMatchRoot.fm->gt2Socket, func_ov011_0217ed14(0, port, NULL),
                            sMatchRoot.fm->gt2SendBufSize, sMatchRoot.fm->gt2RecvBufSize, DWCi_GT2SocketErrorCallback);
    if (DWCi_HandleGT2Error(result)) {
        return result;
    }
    func_ov011_0217d188(sMatchRoot.fm->gt2Socket, DWCi_GT2ConnectAttemptCallback);
    func_ov011_0217d3a0(sMatchRoot.fm->gt2Socket, DWCi_GT2UnrecognizedMessageCallback);
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

static int DWCi_GT2GetConnectionListIdx(void) {
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

static GT2Connection *DWCi_GetGT2ConnectionByIdx(int index) {
    return &sConnections[index];
}

static GT2Connection *DWCi_GetGT2ConnectionByProfileID(s32 profileId, int a1) {
    int i;

    for (i = 0; i < 0x20; i++) {
        if (sConnections[i] != NULL &&
            profileId == ((MatchConnectionInfo *)gt2GetConnectionData(sConnections[i]))->profileId) {
            return DWCi_GetGT2ConnectionByIdx(i);
        }
    }
    return NULL;
}

static MatchConnectionInfo *DWCi_GetConnectionInfoByIdx(int index) {
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

u8 DWC_AddMatchKeyInt(u8 keyId, const char *key, const int *valueSrc) {
    int index;

    if (key == NULL || valueSrc == NULL) {
        return 0;
    }
    if (keyId >= 100 && sKeys[keyId - 100].id != 0) {
        if (sKeys[keyId - 100].name != NULL && strcmp(sKeys[keyId - 100].name, key) != 0) {
            return 0;
        }
    } else {
        keyId = DWCi_GetNewMatchKey();
        if (keyId == 0) {
            return 0;
        }
    }
    index = keyId - 100;
    sKeys[index].id = keyId;
    sKeys[index].isText = 0;
    sKeys[index].pad = 0;
    if (sKeys[index].name != NULL) {
        DWC_Free(4, sKeys[index].name, 0);
    }
    sKeys[index].name = DWC_Alloc(4, strlen(key) + 1);
    if (sKeys[index].name == NULL) {
        DWCi_StopMatching(9, -80001);
        return 0;
    }
    strcpy(sKeys[index].name, key);
    sKeys[index].value = valueSrc;
    qr2_register_keyA(keyId, sKeys[index].name);
    return keyId;
}

u8 DWC_AddMatchKeyString(u8 keyId, const char *key, const char *value) {
    int index;

    if (key == NULL || value == NULL) {
        return 0;
    }
    if (keyId >= 100 && sKeys[keyId - 100].id != 0) {
        if (sKeys[keyId - 100].name != NULL && strcmp(sKeys[keyId - 100].name, key) != 0) {
            return 0;
        }
    } else {
        keyId = DWCi_GetNewMatchKey();
        if (keyId == 0) {
            return 0;
        }
    }
    index = keyId - 100;
    sKeys[index].id = keyId;
    sKeys[index].isText = 1;
    sKeys[index].pad = 0;
    if (sKeys[index].name != NULL) {
        DWC_Free(4, sKeys[index].name, 0);
    }
    sKeys[index].name = DWC_Alloc(4, strlen(key) + 1);
    if (sKeys[index].name == NULL) {
        DWCi_StopMatching(9, -80001);
        return 0;
    }
    strcpy(sKeys[index].name, key);
    sKeys[index].value = value;
    qr2_register_keyA(keyId, sKeys[index].name);
    return keyId;
}

int DWC_GetMatchIntValue(int index, const char *key, int defaultValue) {
    void *host;

    if (DWCi_GetMatchCnt() == NULL || DWCi_IsError()) {
        return defaultValue;
    }
    host = ServerBrowserGetServer(DWCi_GetMatchCnt()->sbObj, index);
    if (host != NULL) {
        defaultValue = SBServerGetIntValueA(host, key, defaultValue);
    }
    return defaultValue;
}

static BOOL func_ov011_02160fd8(void) {
    int result = -1;
    u32 mask;

    if (DWCi_GetMatchCnt() == NULL) {
        return FALSE;
    }
    if (DWCi_GetHostState() == 0) {
        return FALSE;
    }
    mask = DWCi_GetMatchCnt()->suspendAidBitmap;
    mask &= DWC_GetAIDBitmap();
    if (func_ov011_02167f0c() == 0) {
        mask &= ~(1 << DWC_GetMyAID());
    }
    if ((DWC_GetAIDBitmap() ^ mask) == 0) {
        result = 1;
    } else if ((DWC_GetAIDBitmap() & mask) == 0) {
        result = 0;
    }
    if (result != -1) {
        DWCi_GetMatchCnt()->suspend = result == 1;
    }
    return DWCi_GetMatchCnt()->suspend;
}

static void DWCi_MatchInit(DWCMatchControl *cnt, GPConnection *gp, DWCFriendsMatchControl *fm,
                           GT2ConnectionCallbacks *callbacks, char *gameName, char *secretKey,
                           DWCFriendData *friendList, int friendListLen) {
    DWCi_SetMatchCnt(cnt);
    DWCi_GetMatchCnt()->pGpObj = gp;
    DWCi_GetMatchCnt()->fm = fm;
    DWCi_GetMatchCnt()->gt2Callbacks = callbacks;
    DWCi_GetMatchCnt()->qr2Obj = NULL;
    DWCi_GetMatchCnt()->qr2IP = 0;
    DWCi_GetMatchCnt()->qr2Port = 0;
    DWCi_GetMatchCnt()->sbObj = NULL;
    DWCi_SetMatchStatus(0);
    DWCi_GetMatchCnt()->unk_19 = 0;
    DWCi_GetMatchCnt()->profileID = 0;
    DWCi_GetMatchCnt()->gameName = gameName;
    DWCi_GetMatchCnt()->secretKey = secretKey;
    DWCi_GetMatchCnt()->friendList = friendList;
    DWCi_GetMatchCnt()->friendListLen = friendListLen;
    MI_CpuFill8(DWCi_GetMatchCnt()->friendIdxList, 0, 0x40);
    DWCi_GetMatchCnt()->friendIdxListLen = 0;
    DWCi_GetMatchCnt()->notifyFunc = 0;
    DWCi_GetMatchCnt()->notifyParam = 0;
    DWCi_GetMatchCnt()->evalCallback = NULL;
    DWCi_GetMatchCnt()->evalParam = NULL;
    DWCi_GetMatchCnt()->newHostAckBit = 0;
    DWCi_GetMatchCnt()->connectedAidBit = 0;
    DWCi_GetMatchCnt()->groupId = 0;
    DWCi_GetMatchCnt()->groupSearchCount = 0;
    DWCi_GetMatchCnt()->unk_798 = 0;
    DWCi_GetMatchCnt()->qr2MatchType = 0;
    DWCi_GetMatchCnt()->hostState = 0;
    DWCi_GetMatchCnt()->sbUpdateRequestTick = 0;
    DWCi_GetMatchCnt()->synId = 0;
    DWCi_GetMatchCnt()->lockEnabled = 1;
    DWCi_GetMatchCnt()->suppressMatchedCallback = 0;
    DWCi_GetMatchCnt()->scResvRetryMax = 0;
    DWCi_GetMatchCnt()->topology = 0;
    DWCi_InitNodeInfoList();
    DWCi_ClearTargetNodeInfo();
    DWCi_ClearGameMatchKeys();
    DWCi_ResetMatchParam(0);
}

static int DWCi_QR2Startup(s32 profileId) {
    int result;
    int retries;

    if (DWCi_GetMatchCnt()->qr2Obj != NULL) {
        return 0;
    }
    retries = 0;
    do {
        result = qr2_init_socketA(&DWCi_GetMatchCnt()->qr2Obj, gt2GetSocketSOCKET(DWCi_GetMatchCnt()->fm->gt2Socket),
                                  func_ov011_0217d38c(DWCi_GetMatchCnt()->fm->gt2Socket), DWCi_GetMatchCnt()->gameName,
                                  DWCi_GetMatchCnt()->secretKey, 1, 1, DWCi_QR2ServerKeyCallback,
                                  DWCi_QR2PlayerKeyCallback, DWCi_QR2TeamKeyCallback, DWCi_QR2KeyListCallback,
                                  DWCi_QR2CountCallback, DWCi_QR2AddErrorCallback, NULL);
        if (result == 0) {
            break;
        }
        qr2_shutdown(DWCi_GetMatchCnt()->qr2Obj);
        DWCi_GetMatchCnt()->qr2Obj = NULL;
        if (result != 3 || retries == 0) {
            DWCi_HandleQR2Error(result);
            return result;
        }
        retries++;
    } while (retries < 1);
    DWCi_GetMatchCnt()->qr2IP = 0;
    DWCi_GetMatchCnt()->qr2Port = 0;
    qr2_register_publicaddress_callback(DWCi_GetMatchCnt()->qr2Obj, DWCi_QR2PublicAddrCallback);
    qr2_register_natneg_callback(DWCi_GetMatchCnt()->qr2Obj, DWCi_QR2NatnegCallback);
    qr2_register_clientmessage_callback(DWCi_GetMatchCnt()->qr2Obj, DWCi_QR2ClientMsgCallback);
    DWCi_RegisterMatchQR2Keys();
    qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
    return result;
}

static void DWCi_ConnectToAnybodyAsync(int type, u8 numEntry, const char *query, DWCMatchedSCCallback matched,
                                       void *matchedParam, DWCNewClientCallback newClient, void *newClientParam,
                                       DWCEvalPlayerCallback eval, void *evalParam, DWCAttemptFunc attempt,
                                       int *attemptParam, void *param) {
    char buf[0x200];
    int size;

    if (sMatchState.query != NULL) {
        DWC_Free(4, sMatchState.query, 0);
        sMatchState.query = NULL;
    }
    if (query != NULL) {
        size = 0x1ff - DWCi_GetDefaultMatchFilter(buf, -1, 0x20, 3, 3) - strlen(" and ()");
        sMatchState.query = DWC_Alloc(4, size);
        if (sMatchState.query == NULL) {
            DWCi_StopMatching(9, -80001);
            return;
        }
        MI_CpuCopy8(query, sMatchState.query, size);
        *(sMatchState.query + size - 1) = 0;
    }
    DWCi_SetMatchCommonParam(0, numEntry, matched, matchedParam);
    DWCi_GetMatchCnt()->newClientCallback = newClient;
    DWCi_GetMatchCnt()->newClientParam = newClientParam;
    DWCi_GetMatchCnt()->evalCallback = eval;
    DWCi_GetMatchCnt()->evalParam = evalParam;
    DWCi_GetMatchCnt()->topology = type;
    DWCi_SetReservationCallback((MatchCheckFunc)attempt, attemptParam, (u32)param);
    DWCi_SetMatchStatus(0x16);
    if (DWCi_GetMatchCnt()->sbObj == NULL) {
        DWCi_GetMatchCnt()->sbObj = ServerBrowserNewA(DWCi_GetMatchCnt()->gameName, DWCi_GetMatchCnt()->gameName,
                                                      DWCi_GetMatchCnt()->secretKey, 0, 0x14, 1, 0, DWCi_SBCallback, 0);
    }
    if (DWCi_GetMatchCnt()->sbObj != NULL || DWCi_HandleSBError(5) == 0) {
        DWCi_GPSetServerStatus(3);
        if (DWCi_GetMatchCnt()->qr2Obj == NULL) {
            if (DWCi_QR2Startup(DWCi_GetMatchCnt()->profileID) != 0) {
                return;
            }
        } else {
            qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
        }
        if (DWCi_HandleSBError(DWCi_SBUpdateAsync(DWCi_GetMatchCnt()->profileID)) != 0) {
            return;
        }
    }
}

static void DWCi_SetupGameServer(int type, u8 numEntry, DWCMatchedSCCallback matched, void *matchedParam,
                                 DWCNewClientCallback newClient, void *newClientParam, DWCAttemptFunc attempt,
                                 int *attemptParam, void *param) {
    DWCi_SetMatchCommonParam(2, numEntry, matched, matchedParam);
    DWCi_GetMatchCnt()->newClientCallback = newClient;
    DWCi_GetMatchCnt()->newClientParam = newClientParam;
    DWCi_GetMatchCnt()->topology = type;
    DWCi_SetReservationCallback((MatchCheckFunc)attempt, attemptParam, (u32)param);
    DWCi_SetMatchStatus(0x16);
    if (DWCi_HandleMatchGPError(DWCi_GPSetServerStatus(6)) == 0) {
        DWCi_SetHostState(2);
        DWCi_GetMatchCnt()->unlocked = 1;
        if (DWCi_GetMatchCnt()->qr2Obj == NULL) {
            if (DWCi_QR2Startup(DWCi_GetMatchCnt()->profileID) != 0) {
                return;
            }
        }
        DWCi_GetMatchCnt()->groupId = (u32)DWCi_MakeGroupID();
        if (DWCi_GetMatchCnt()->sbObj == NULL) {
            DWCi_GetMatchCnt()->sbObj =
                ServerBrowserNewA(DWCi_GetMatchCnt()->gameName, DWCi_GetMatchCnt()->gameName,
                                  DWCi_GetMatchCnt()->secretKey, 0, 0x14, 1, 0, DWCi_SBCallback, 0);
        }
        qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
        if (DWCi_HandleSBError(DWCi_SBUpdateAsync(DWCi_GetMatchCnt()->profileID)) != 0) {
            return;
        }
    }
}

static void DWCi_ConnectToGameServerAsync(int type, s32 profileId, DWCMatchedSCCallback matched, void *matchedParam,
                                          DWCNewClientCallback newClient, void *newClientParam, DWCAttemptFunc attempt,
                                          int *attemptParam, void *param) {
    DWCNodeInfo saved;
    DWCNodeInfo savedHost;
    DWCNodeInfo zero;
    u8 scResvRetryCount;

    if (DWCi_GetMatchCnt()->unk_798 != 0) {
        saved = DWCi_GetMatchCnt()->self;
        savedHost = *DWCi_GetOwnNodeInfo();
        scResvRetryCount = DWCi_GetMatchCnt()->scResvRetryCount;
    }
    DWCi_SetMatchCommonParam(3, 0, matched, matchedParam);
    DWCi_GetMatchCnt()->topology = type;
    if (DWCi_GetMatchCnt()->unk_798 == 0) {
        MI_CpuFill8(&zero, 0, sizeof(zero));
        DWCi_ClearTargetNodeInfo();
        DWCi_GetMatchCnt()->self.aid = 0xff;
        DWCi_GetMatchCnt()->self.profileId = profileId;
    } else {
        DWCi_GetMatchCnt()->self = saved;
        DWCi_PushFrontNodeInfo(&savedHost);
        MI_CpuCopy8(&savedHost.userData, &DWCi_GetMatchCnt()->userData, sizeof(int));
        DWCi_GetMatchCnt()->scResvRetryCount = scResvRetryCount;
    }
    DWCi_GetMatchCnt()->newClientCallback = newClient;
    DWCi_GetMatchCnt()->newClientParam = newClientParam;
    DWCi_SetReservationCallback((MatchCheckFunc)attempt, attemptParam, (u32)param);
    DWCi_GetMatchCnt()->qr2IsReserved = 1;
    DWCi_GetMatchCnt()->qr2Reservation = DWCi_GetMatchCnt()->profileID;
    DWCi_SetHostState(1);
    if (DWCi_GetMatchCnt()->unk_798 == 0) {
        DWCi_SetMatchStatus(0x16);
    }
    if (DWCi_GetMatchCnt()->sbObj == NULL) {
        DWCi_GetMatchCnt()->sbObj = ServerBrowserNewA(DWCi_GetMatchCnt()->gameName, DWCi_GetMatchCnt()->gameName,
                                                      DWCi_GetMatchCnt()->secretKey, 0, 0x14, 1, 0, DWCi_SBCallback, 0);
    }
    if (DWCi_GetMatchCnt()->sbObj != NULL || DWCi_HandleSBError(5) == 0) {
        if (DWCi_HandleMatchGPError(DWCi_GPSetServerStatus(5)) == 0) {
            if (DWCi_GetMatchCnt()->qr2Obj == NULL) {
                if (DWCi_QR2Startup(DWCi_GetMatchCnt()->profileID) != 0) {
                    return;
                }
            } else {
                qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
            }
            if (DWCi_GetMatchCnt()->unk_798 == 0) {
                if (DWCi_HandleSBError(DWCi_SBUpdateAsync(DWCi_GetMatchCnt()->profileID)) != 0) {
                    return;
                }
                return;
            }
            qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
            DWCi_SetMatchStatus(3);
            if (DWCi_HandleMatchCommandError(DWCi_SendResvCommand(profileId, 0)) != 0) {
                return;
            }
        }
    }
}

static void DWCi_SendStateChanged(qr2_t server) {
    if (server != NULL) {
        qr2_think(server);
        if (server->userStateChangeRequested == 0) {
            switch (DWCi_GetMatchCnt()->mode) {
            case 0:
            case 2:
            case 3:
                DWCi_GetMatchCnt();
                break;
            }
        }
    }
}

// The state machine of the match, run every frame (a0 is 0 when it only has to serve the sockets)
static void DWCi_MatchProcess(int a0) {
    int i;
    int j;
    u32 limit;
    u32 mask;
    DWCMatchControl *c;

    if (DWCi_GetMatchCnt() == NULL || DWCi_IsError() != 0) {
        return;
    }
    if (a0 == 0) {
        if (DWCi_GetMatchCnt()->qr2Obj != NULL) {
            qr2_think(DWCi_GetMatchCnt()->qr2Obj);
        }
        if (DWCi_GetMatchCnt()->fm != NULL) {
            func_ov011_0217d168(DWCi_GetMatchCnt()->fm->gt2Socket);
        }
        return;
    }
    if (DWCi_GetMatchCnt()->state == 0) {
        return;
    }
    switch (DWCi_GetMatchCnt()->state) {
    case 3:
        if (DWCi_GetMatchCnt()->cmdTimeoutTime != 0 &&
            OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->cmdTimeoutStartTick) >
                DWCi_GetMatchCnt()->cmdTimeoutTime) {
            DWCi_GetMatchCnt()->cmdTimeoutTime = 0;
            if (DWCi_GetMatchCnt()->unk_798 != 0) {
                DWCi_GetMatchCnt();
                DWCi_GetMatchCnt()->scResvRetryCount++;
                if (DWCi_GetMatchCnt()->scResvRetryCount > DWCi_GetMatchCnt()->scResvRetryMax) {
                    DWCi_StopMatching(0x16, -80432);
                    return;
                }
                DWCi_RetryReserving(0);
                return;
            }
            if (func_ov011_02168468() == 1) {
                DWCi_SetMatchStatus(1);
                break;
            }
            if (DWCi_GetMatchCnt()->qr2MatchType == 3) {
                DWCi_GetMatchCnt();
                DWCi_GetMatchCnt()->scResvRetryCount++;
                if (DWCi_GetMatchCnt()->scResvRetryCount > DWCi_GetMatchCnt()->scResvRetryMax) {
                    DWCi_StopMatching(0x16, -80430);
                    return;
                }
                if (DWCi_HandleMatchCommandError(DWCi_SendResvCommand(DWCi_GetMatchCnt()->self.profileId, 0)) != 0) {
                    return;
                }
            } else if (DWCi_GetMatchCnt()->qr2MatchType == 1) {
                DWCi_ClearTargetNodeInfo();
                if (DWCi_RetryReserving(0) == 0) {
                    return;
                }
            } else {
                DWCi_ClearTargetNodeInfo();
                if (DWCi_RetryReserving(0) == 0) {
                    return;
                }
            }
        }
        if (DWCi_GetMatchCnt()->cmdResendFlag != 0 &&
            OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->cmdResendTick) >= 3000) {
            if (DWCi_HandleMatchCommandError(DWCi_SendResvCommand(DWCi_GetMatchCnt()->self.profileId, 0)) != 0) {
                return;
            }
        }
        break;
    case 2:
    case 4:
    case 22:
        if (DWCi_GetMatchCnt()->sbUpdateFlag > 0) {
            if (DWCi_GetMatchCnt()->state == 2) {
                if (DWCi_GetMatchCnt()->sbUpdateFlag == 1 || DWCi_GetMatchCnt()->sbSearchCount <= 1) {
                    limit = 4000;
                } else {
                    limit = 12000;
                }
            } else if (DWCi_GetMatchCnt()->state == 22) {
                limit = 4000;
            } else if (DWCi_GetMatchCnt()->sbUpdateFlag == 1) {
                limit = 4000;
            } else {
                limit = 12000;
            }
            if (OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->sbUpdateTick) > limit) {
                if (DWCi_HandleSBError(DWCi_SBUpdateAsync(DWCi_GetMatchCnt()->reqProfileID)) != 0) {
                    return;
                }
                DWCi_GetMatchCnt()->sbUpdateFlag = 0;
            }
        }
        break;
    case 6:
        if (DWCi_GetMatchCnt()->nnFinishTime != 0) {
            if (OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->nnFinishTime) > 25000) {
                DWCi_GetMatchCnt()->nnFinishTime = 0;
                if (func_ov011_02168468() == 1) {
                    DWCi_SetMatchStatus(1);
                } else if (DWCi_CancelPreConnectedClientProcess(DWCi_GetMatchCnt()->self.profileId) == 0) {
                    DWCi_ClearTargetNodeInfo();
                    return;
                }
            }
        } else if (DWCi_GetMatchCnt()->cmdCommand == 6 &&
                   OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->cmdSendTime) > 6000) {
            if (func_ov011_02168468() == 1) {
                DWCi_StopResendingMatchCommand();
                DWCi_SetMatchStatus(1);
            } else {
                DWCi_GetMatchCnt()->cmdCount++;
                if (DWCi_GetMatchCnt()->cmdCount > 5) {
                    DWCi_StopResendingMatchCommand();
                    if (DWCi_CancelPreConnectedClientProcess(DWCi_GetMatchCnt()->self.profileId) == 0) {
                        DWCi_ClearTargetNodeInfo();
                        return;
                    }
                } else if (DWCi_HandleMatchCommandError(
                               DWCi_SendMatchCommand(6, DWCi_GetMatchCnt()->cmdProfileId, DWCi_GetMatchCnt()->cmdIp,
                                                     DWCi_GetMatchCnt()->cmdPort, DWCi_GetMatchCnt()->cmdData,
                                                     DWCi_GetMatchCnt()->cmdLen)) != 0) {
                    return;
                }
            }
        }
        break;
    case 14:
        if (DWCi_GetMatchCnt()->cmdCommand == 2 &&
            ((DWCi_GetMatchCnt()->qr2MatchType == 0 &&
              OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->cmdSendTime) > 6000) ||
             (DWCi_GetMatchCnt()->qr2MatchType != 0 &&
              OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->cmdSendTime) > 19000))) {
            DWCi_StopResendingMatchCommand();
            if (func_ov011_02168468() == 1) {
                DWCi_SetMatchStatus(1);
            } else if (DWCi_CancelPreConnectedServerProcess(DWCi_GetMatchCnt()->self.profileId) == 0) {
                return;
            }
        }
        break;
    case 16:
        if (DWCi_GetMatchCnt()->cmdCommand == 8 &&
            OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->cmdSendTime) > 30000) {
            DWCi_GetMatchCnt()->cmdCount++;
            if (DWCi_GetMatchCnt()->cmdCount != 0) {
                DWCi_StopResendingMatchCommand();
                if (DWCi_CancelPreConnectedServerProcess(DWCi_GetMatchCnt()->self.profileId) == 0) {
                    return;
                }
            }
        }
        break;
    case 1:
        if (DWC_GetState() == 4 && DWCi_GetMatchCnt()->unk_62f != 0 &&
            OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->unk_670) > 30000) {
            DWCNodeInfo *self = &DWCi_GetMatchCnt()->self;

            if (DWCi_GetMatchCnt()->clWaitTimeoutCount >= 5) {
                if (DWCi_CancelPreConnectedClientProcess(self->profileId) == 0) {
                    return;
                }
            } else {
                u64 now;

                if (DWCi_HandleMatchCommandError(
                        DWCi_SendMatchCommand(0x40, self->profileId, self->publicIp, self->publicPort, NULL, 0)) != 0) {
                    return;
                }
                DWCi_GetMatchCnt()->clWaitTimeoutCount++;
                now = OS_GetTick();
                now -= OS_MilliSecondsToTicks(24000);
                DWCi_GetMatchCnt()->unk_670 = now;
            }
        }
        break;
    case 9:
        func_ov011_02168468();
        c = DWCi_GetMatchCnt();
        c->newHostAckBit = 0;
        c->newHostResendCount = 0;
        c->newHostNakCount = 0;
        if (DWCi_IsServerNodeMyself() == 0 && DWCi_GetGT2Connection(DWCi_GetNodeInfoByIndex(0)->aid) == NULL) {
            DWCi_StopMatching(0x16, -80431);
            return;
        }
        c->newHostAckBit |= 1 << DWCi_GetAIDFromProfileID(c->profileID, 1);
        if (DWCi_IsServerNodeMyself() != 0) {
            if (func_ov011_02167fd0() == 0) {
                DWCi_StopMatching(0x16, -80431);
                break;
            }
            DWCi_SendSvDownCommands();
            DWCi_SetMatchStatus(10);
            qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
            DWCi_InitClWaitTimeout();
        } else {
            qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
            DWCi_SetMatchStatus(10);
            DWCi_InitClWaitTimeout();
        }
        break;
    case 10:
        if (DWCi_IsServerNodeMyself() != 0) {
            if (OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->unk_670) > 5000) {
                DWCi_SendSvDownCommands();
                if (++DWCi_GetMatchCnt()->newHostResendCount > 5) {
                    mask = ~(DWCi_GetMatchCnt()->newHostAckBit & DWCi_GetMatchCnt()->connectedAidBit);
                    for (i = 0; i < DWCi_GetNumNodeInfo(); i++) {
                        u8 aid = DWCi_GetNodeInfoByIndex(i)->aid;

                        if ((1 << aid) & mask) {
                            DWCi_CloseConnectionHard(aid);
                        }
                    }
                    DWCi_GetMatchCnt()->newHostAckBit = DWCi_GetMatchCnt()->connectedAidBit;
                }
            }
            if ((DWCi_GetMatchCnt()->newHostAckBit ^ DWCi_GetMatchCnt()->connectedAidBit) == 0) {
                DWCi_SetMatchStatus(11);
            }
        } else if (OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->unk_670) > 60000) {
            DWCi_StopMatching(0x16, -80431);
        }
        break;
    case 11:
        if (DWCi_IsServerNodeMyself() != 0) {
            if (func_ov011_02167fd0() == 0) {
                DWCi_StopMatching(0x16, -80431);
                break;
            }
            DWCi_ChangeToServer(0);
            DWCi_GetMatchCnt()->aidListSendTick = 0;
            DWCi_UpdateAllNodesRecvTime();
            DWCi_ApplyPendingSuspend();
            qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
        } else if (OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->unk_670) > 20000) {
            DWCi_SetMatchStatus(1);
        }
        break;
    case 12:
        if (DWCi_GetMatchCnt()->sbUpdateFlag > 0) {
            if (OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->sbUpdateTick) > 8000) {
                if (++DWCi_GetMatchCnt()->groupSearchCount > 0xf) {
                    DWCi_StopMatching(0x16, -80432);
                    break;
                }
                if (DWCi_HandleSBError(DWCi_SBUpdateAsync(0)) != 0) {
                    return;
                }
                DWCi_GetMatchCnt()->sbUpdateFlag = 0;
            }
        }
        break;
    }
    if (DWCi_GetMatchCnt()->state == 0xe || DWCi_GetMatchCnt()->state == 5) {
        if (DWCi_GetMatchCnt()->nnFailedTime != 0 &&
            OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->nnFailedTime) > 10000) {
            DWCi_NNCompletedCallback(1, 0, NULL, &DWCi_GetMatchCnt()->target);
        }
    }
    if (DWCi_GetTopologyType() == 0 && DWCi_GetMatchCnt()->memberConnecting == 0 && DWCi_GetMatchCnt()->state == 1 &&
        DWCi_GetMatchCnt()->pending.profileId == 0 && DWCi_GetNumNodeInfo() > 2) {
        for (j = 1; j < DWCi_GetNumNodeInfo(); j++) {
            DWCNodeInfo *node = DWCi_GetNodeInfoByIndex(j);

            if (DWCi_GetOwnNodeInfo() != node && DWCi_GetGT2Connection(node->aid) == NULL) {
                if (node->nextConnectTick == 0) {
                    node->nextConnectTick = func_ov011_021683b8();
                }
                if (node->nextConnectTick < OS_GetTick() && node->connectFailureCount <= sMatchChance[1]) {
                    node->nextConnectTick = func_ov011_021683b8();
                    DWCi_GetMatchCnt()->self = *node;
                    DWCi_GetMatchCnt()->memberConnecting = 1;
                    if (DWCi_HandleMatchCommandError(DWCi_SendResvCommand(node->profileId, 0)) != 0) {
                        DWCi_GetMatchCnt()->memberConnecting = 0;
                        DWCi_ClearTargetNodeInfo();
                    } else {
                        func_ov011_021685fc();
                        DWCi_SetMatchStatus(3);
                    }
                    break;
                }
            }
        }
    }
    if (DWCi_GetMatchCnt()->sbObj != NULL) {
        sMatchState.sbCallbackLevel = 0;
        sMatchState.needSbFree = 0;
        sMatchState.inNNThink = 1;
        ServerBrowserThink(DWCi_GetMatchCnt()->sbObj);
        if (sMatchState.needSbFree != 0) {
            ServerBrowserFree(DWCi_GetMatchCnt()->sbObj);
            DWCi_GetMatchCnt()->sbObj = NULL;
        }
        if (DWCi_GetMatchCnt()->sbObj != NULL && ServerBrowserState(DWCi_GetMatchCnt()->sbObj) != 0 &&
            DWCi_GetMatchCnt()->sbUpdateRequestTick != 0 && OS_GetTick() > DWCi_GetMatchCnt()->sbUpdateRequestTick) {
            DWCi_StopMatching(6, -85010);
        }
    }
    sMatchState.nnFreeNegotiateList = 0;
    sMatchState.inNNThink = 1;
    DWCi_SendStateChanged(DWCi_GetMatchCnt()->qr2Obj);
    NNThink();
    sMatchState.inNNThink = 0;
    if (sMatchState.nnFreeNegotiateList == 1) {
        sMatchState.nnFreeNegotiateList = 0;
        NNFreeNegotiateList();
    }
    if (DWCi_GetMatchCnt()->fm != NULL) {
        func_ov011_0217d168(DWCi_GetMatchCnt()->fm->gt2Socket);
    }
    if (DWCi_GetMatchCnt()->state == 0x15 &&
        OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->closedTime) > 3000) {
        if (DWCi_ResumeMatching() != 0) {
            return;
        }
    }
    DWCi_ProcessSuspendRequest();
    DWCi_SendServerPoll(0);
    if (DWCi_ProcessMatchSynTimeout() == 0) {
        return;
    }
    if (DWCi_ProcessCancelMatchSynTimeout() == 0) {
        return;
    }
}

static GT2Bool DWCi_GT2UnrecognizedMessageCallback(GT2Socket socket, unsigned int ip, unsigned short port,
                                                   GT2Byte *message, int length) {
    const u8 *data = message;
    SOCSockAddrIn addr;

    if (length == 0 || data == NULL) {
        return gsi_false;
    }
    MIi_CpuClear32(0, &addr, sizeof(addr));
    addr.family = 2;
    addr.addr.addr = ip;
    addr.port = MATCH_SWAP16(port);
    if ((data[0] == 0xfe && data[1] == 0xfd) || data[0] == 0x5c) {
        if (DWCi_GetMatchCnt()->qr2Obj != NULL) {
            qr2_parse_queryA(DWCi_GetMatchCnt()->qr2Obj, (char *)data, length, (SOCKADDR *)&addr);
        }
    } else if (memcmp(data, NNMagicData, NATNEG_MAGIC_LEN) == 0) {
        NNProcessData((char *)data, length, &addr);
    } else {
        switch (data[0]) {
        case 0xfe:
            return gsi_false;
        default:
            return gsi_false;
        }
    }
    return gsi_true;
}

static void DWCi_GT2ConnectAttemptCallback(GT2Socket socket, GT2Connection connection, unsigned int ip,
                                           unsigned short port, int latency, GT2Byte *message, int length) {
    int slot;
    u32 gameId = 0;
    int half;
    GT2Connection *slotConn;
    MatchConnectionInfo *info;

    if (DWCi_GetMatchCnt() != NULL && DWCi_GetMatchCnt()->state == 5) {
        return;
    }
    if (DWCi_GetMatchCnt() == NULL || DWCi_GetMatchCnt()->state != 6 || DWCi_GetMatchCnt()->cancelState != 0) {
        func_ov011_0217d198(connection, "Init state", -1);
        return;
    }
    if (message != NULL && length != 0) {
        gameId = strtoul(message, NULL, 10);
    }
    slot = DWCi_GT2GetConnectionListIdx();
    if (slot == -1) {
        func_ov011_0217d198(connection, "Server full", -1);
        if (func_ov011_02168468() == 1) {
            DWCi_StopResendingMatchCommand();
            DWCi_SetMatchStatus(1);
            return;
        }
        DWCi_StopMatching(6, -87100);
        return;
    }
    if (ip != DWCi_GetMatchCnt()->self.hostIp || MATCH_SWAP16(port) != DWCi_GetMatchCnt()->self.hostPort) {
        if (gameId == DWCi_GetMatchCnt()->self.profileId) {
            DWCi_GetMatchCnt()->self.hostIp = ip;
            DWCi_GetMatchCnt()->self.hostPort = MATCH_SWAP16(port);
        } else {
            func_ov011_0217d198(connection, "Unknown connect attempt", -1);
            return;
        }
    }
    DWCi_GetMatchCnt()->nnFinishTime = 0;
    if (!func_ov011_0217d190(connection, DWCi_GetMatchCnt()->gt2Callbacks)) {
        DWCi_StopMatching(6, -80410);
        return;
    }
    DWCi_StopResendingMatchCommand();
    if (DWCi_GetMatchCnt()->memberConnecting == 0 && DWCi_GetNumNodeInfo() == 1) {
        half = latency >> 1;
        if (half >= 0xffff) {
            half = 0xffff;
        }
        DWCi_GetMatchCnt()->baseLatency = half;
    }
    slotConn = DWCi_GetGT2ConnectionByIdx(slot);
    info = DWCi_GetConnectionInfoByIdx(slot);
    *slotConn = connection;
    info->index = slot;
    info->aid = DWCi_GetMatchCnt()->self.aid;
    info->profileId = DWCi_GetMatchCnt()->self.profileId;
    DWCi_ResetLastSomeDataRecvTimeByAid(info->aid);
    gt2SetConnectionData(connection, info);
    DWCi_PostProcessConnection(2);
}

static void DWCi_FormatProfileID(char *dest, u32 size) {
    OS_SNPrintf(dest, size, "%u", DWCi_GetMatchCnt()->profileID);
}

static void DWCi_GT2ConnectedCallback(GT2Connection connection, GT2Result result, GT2Byte *message, int length) {
    char text[0x18];
    DWCMatchControl *work;
    const char *address;
    int status;
    int slot;
    GT2Connection *slotConn;
    MatchConnectionInfo *info;

    if (!(((DWCi_GetMatchCnt() != NULL && (DWCi_GetMatchCnt()->state == 6 || DWCi_GetMatchCnt()->state == 0xf)) ||
           DWCi_GetMatchCnt()->abortedMemberState == 0xf))) {
        return;
    }
    if (result != 0) {
        if (func_ov011_02168468() == 1) {
            DWCi_SetMatchStatus(1);
            return;
        }
        if (DWCi_GetMatchCnt()->abortedMemberState == 0xf) {
            func_ov011_021685fc();
            return;
        }
        if (result == 5) {
            return;
        }
        if (result == 6) {
            DWCi_GetMatchCnt()->retries++;
            if (DWCi_GetMatchCnt()->retries > 5) {
                DWCi_GetMatchCnt()->retries = 0;
                DWCi_CancelPreConnectedServerProcess(DWCi_GetMatchCnt()->self.profileId);
                return;
            }
            DWCi_FormatProfileID(text, 0x18);
            work = DWCi_GetMatchCnt();
            address = func_ov011_0217ed14(DWCi_GetMatchCnt()->self.hostIp, DWCi_GetMatchCnt()->self.hostPort, NULL);
            status = func_ov011_0217d1a0(work->fm->gt2Socket, NULL, address, text, -1, 5000,
                                         DWCi_GetMatchCnt()->gt2Callbacks, FALSE);
            if (status == 1) {
                DWCi_HandleMatchGT2Error(status);
                return;
            }
            if (status != 0 && DWCi_CancelPreConnectedServerProcess(DWCi_GetMatchCnt()->self.profileId) == 0) {
                return;
            }
        } else {
            if (DWCi_CancelPreConnectedServerProcess(DWCi_GetMatchCnt()->self.profileId) == 0) {
                return;
            }
        }
        return;
    }
    slot = DWCi_GT2GetConnectionListIdx();
    if (slot == -1) {
        DWCi_StopMatching(6, -87100);
        return;
    }
    slotConn = DWCi_GetGT2ConnectionByIdx(slot);
    info = DWCi_GetConnectionInfoByIdx(slot);
    *slotConn = connection;
    info->index = slot;
    if (DWCi_GetMatchCnt()->abortedMemberState != 0xf) {
        info->aid = DWCi_GetMatchCnt()->self.aid;
        info->profileId = DWCi_GetMatchCnt()->self.profileId;
    } else {
        info->aid = DWCi_GetMatchCnt()->abortedMember.aid;
        info->profileId = DWCi_GetMatchCnt()->abortedMember.profileId;
    }
    DWCi_ResetLastSomeDataRecvTimeByAid(info->aid);
    gt2SetConnectionData(connection, info);
    if (DWCi_GetMatchCnt()->state == 0xf || DWCi_GetMatchCnt()->abortedMemberState == 0xf) {
        DWCi_PostProcessConnection(0);
    } else {
        DWCi_PostProcessConnection(1);
    }
}

static void DWCi_MatchGPRecvBuddyMsgCallback(void *unused, int id, u8 *str) {
    u8 buf[0x200];
    int len = strlen(str) - 1;
    int count;
    int n;

    if (len == 0) {
        count = 0;
    } else {
        n = DWC_Base64Decode(str + 1, len, buf, 0x200);
        if (n == -1 || n % 4 != 0 || n > 0x200) {
            return;
        }
        count = n / 4;
    }
    DWCi_ProcessRecvMatchCommand(str[0], id, 0, 0, (const u32 *)buf, count);
}

static void DWCi_StopResendingMatchCommand(void) {
    DWCi_GetMatchCnt()->cmdCommand = 0xff;
    DWCi_GetMatchCnt()->cmdCount = 0;
}

static void DWCi_StopMatching(int type, int code) {
    int flag;
    DWCMatchControl *work;
    BOOL isHost;

    if (DWCi_GetMatchCnt() != NULL && type != 0) {
        DWCi_CloseAllConnectionsByTimeout();
        DWCi_SetError(type, code);
        flag = 1;
        DWCi_GPSetServerStatus(1);
        if (DWCi_GetMatchCnt()->cbEventPid != 0) {
            flag = 0;
        }
        work = DWCi_GetMatchCnt();
        isHost = DWCi_GetMatchCnt()->mode == 2;
        work->notifyFunc(type, 0, flag, isHost, DWCi_GetFriendListIndex(DWCi_GetMatchCnt()->cbEventPid),
                         DWCi_GetMatchCnt()->notifyParam);
        DWCi_CloseMatching();
    }
}

static void DWCi_ClearQR2Key(void) {
    if (DWC_GetState() == 3) {
        DWCi_GetMatchCnt()->qr2NumEntry = 0;
        DWCi_GetMatchCnt()->groupId = 0;
        qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
    }
}

void DWCi_ProcessMatchSynPacket(u8 aid, u16 type, const void *data) {
    if (func_ov011_021686a0()) {
        func_ov011_02162bf8(aid, type, data);
    } else {
        func_ov011_02162958(aid, type, data);
    }
}

static inline void MatchPeer_Read(DWCNodeInfo *peer, MatchPeerWire *wire) {
    MI_CpuFill8(peer, 0, sizeof(DWCNodeInfo));
    peer->aid = wire->packed;
    peer->hasPrivateAddress = wire->packed >> 16;
    peer->profileId = wire->id;
    peer->publicIp = wire->ip;
    peer->localIp = wire->localIp;
    peer->publicPort = wire->ports >> 16;
    peer->localPort = wire->ports;
    memcpy(&peer->userData, wire->userData, sizeof(u32));
}

static void func_ov011_02162958(u8 conn, int kind, u32 *msg) {
    int i;
    int count;
    int duration;
    u32 value;
    u16 latency;
    u64 now;

    switch (kind) {
    case 2:
        if (DWCi_GetMatchCnt()->state == 1) {
            if (msg[0] == 1) {
                DWCi_GetMatchCnt()->cbEventPid = 0;
                DWCi_GetMatchCnt()->pending.profileId = 0;
                value = msg[1];
                DWCi_GetMatchCnt()->synId = value;
                if (func_ov011_021686a0()) {
                    DWCi_PushFrontNodeInfo(&DWCi_GetMatchCnt()->self);
                }
            } else {
                DWCNodeInfo client;

                MatchPeer_Read(&client, (MatchPeerWire *)&msg[1]);
                DWCi_GetMatchCnt()->pending = client;
                value = msg[7];
                DWCi_GetMatchCnt()->synId = value;
                DWCi_GetMatchCnt()->cbEventPid = client.profileId;
            }
            if (DWCi_GetNumNodeInfo() == 1) {
                duration = 90000;
            } else {
                duration = DWCi_GetNumNodeInfo() * 12000;
            }
            DWCi_GetMatchCnt()->unk_670 = duration;
            DWCi_GetMatchCnt()->clWaitTimeoutCount = 0;
            DWCi_SetMatchStatus(8);
            DWCi_SendMatchSynPacket(conn, 3);
        }
        break;
    case 3:
        if (DWCi_GetMatchCnt()->state == 0x13) {
            latency = ((const u8 *)msg)[0] | (((const u8 *)msg)[1] << 8);
            DWCi_GetMatchCnt()->synAckBit |= 1 << conn;
            if (latency > DWCi_GetMatchCnt()->baseLatency) {
                DWCi_GetMatchCnt()->baseLatency = latency;
            }
            if (DWCi_GetAIDBitmask(0) == DWCi_GetMatchCnt()->synAckBit) {
                DWCi_GetMatchCnt()->synId = DWCi_GetMathRand32(0xfffffffe) + 1;
                DWCi_SendMatchAckToClients();
                DWCi_GetMatchCnt()->synResendCount = 0;
                now = OS_GetTick();
                DWCi_GetMatchCnt()->lastSynSent = now;
            }
        } else {
            DWCi_SendMatchSynPacket(conn, 4);
        }
        break;
    case 4:
        if (DWCi_GetMatchCnt()->state == 8) {
            count = msg[0];
            if (count != 0) {
                for (i = 0; i < count; i++) {
                    DWCNodeInfo peer;
                    MatchPeerWire *wire = (MatchPeerWire *)(msg + 1 + (count - 1 - i) * 6);

                    MatchPeer_Read(&peer, wire);
                    DWCi_PushFrontNodeInfo(&peer);
                }
                value = *(msg + count * 6 + 1);
                DWCi_GetMatchCnt()->synId = value;
                DWCi_PushFrontNodeInfo(&DWCi_GetMatchCnt()->self);
                DWCi_ClearTargetNodeInfo();
            } else {
                value = msg[1];
                DWCi_GetMatchCnt()->synId = value;
            }
            DWCi_PostProcessConnection(3);
        }
        break;
    }
}

static void func_ov011_02162bf8(u8 conn, int kind, u32 *msg) {
    func_ov011_02162958(conn, kind, msg);
}

static void DWCi_CloseConnectionsNotInBitmap(u32 mask) {
    u32 current = DWC_GetAIDBitmap();
    u32 diff = current & (current ^ mask);
    u8 i;

    if (diff != 0) {
        for (i = 0; i < 32; i++) {
            if (diff & (1 << i)) {
                DWCi_CloseConnectionHard(i);
            }
        }
    }
}

static void DWCi_ProcessServerAIDBitmapCommand(const u32 *msg) {
    u32 first = msg[0];
    u32 mask = msg[1];
    u32 id = msg[2];
    DWCNodeInfo *peer;

    if (DWC_IsServerMyself() == 0 && id == DWCi_GetMatchCnt()->synId) {
        peer = DWCi_GetServerNodeInfo();
        if (peer != NULL && peer->profileId == first) {
            DWCi_CloseConnectionsNotInBitmap(mask);
        }
    }
}

static BOOL DWCi_ProcessMatchClosing(int type, u32 code, s32 profileId) {
    u64 now;

    if (DWC_GetState() != 4) {
        return FALSE;
    }
    if (DWCi_GetMatchCnt()->mode == 2) {
        return TRUE;
    }
    if (type != 0) {
        DWCi_StopMatching(type, code - 80000);
        return TRUE;
    }
    if (DWCi_GetMatchCnt()->cancelState == 1 || DWCi_GetMatchCnt()->closeState == 1) {
        return TRUE;
    }
    if (DWCi_GetMatchCnt()->target.cookie != 0) {
        NNCancel(DWCi_GetMatchCnt()->target.cookie);
        DWCi_GetMatchCnt()->target.cookie = 0;
    }
    if (DWCi_GetNumNodeInfo() > 1) {
        if (DWCi_GetMatchCnt()->closeState == 0) {
            DWCi_GetMatchCnt()->closeState = 2;
            gt2CloseAllConnectionsHard(DWCi_GetMatchCnt()->fm->gt2Socket);
        }
    } else if (DWCi_GetMatchCnt()->qr2MatchType == 3) {
        DWCi_StopMatching(0x16, -80430);
    } else if (DWCi_GetMatchCnt()->state == 1) {
        DWCi_SetMatchStatus(0x15);
        now = OS_GetTick();
        DWCi_GetMatchCnt()->closedTime = now;
    } else {
        DWCi_RestartFromCancel(1);
    }
    return TRUE;
}

static void DWCi_ProcessMatchSCClosing(u32 id) {
    if (DWCi_GetMatchCnt()->closeState != 1) {
        DWCi_CancelPreConnectedServerProcess(id);
    }
}

int DWCi_GPSetServerStatus(int status) {
    char number[12];
    char text[0x27];
    int players;

    OS_SNPrintf(number, 12, "%u", DWCi_GetMatchCnt()->qr2NumEntry + 1);
    DWC_SetCommonKeyValueString("SCM", number, text, '/');
    players = DWCi_GetNumNodeInfo();
    if (DWCi_GetMatchCnt()->qr2MatchType == 2 && players == 0) {
        players = 1;
    }
    OS_SNPrintf(number, 12, "%u", players);
    DWC_AddCommonKeyValueString("SCN", number, text, '/');
    OS_SNPrintf(number, 12, "%u", 0x5a);
    DWC_AddCommonKeyValueString("VER", number, text, '/');
    OS_SNPrintf(number, 12, "%u", DWCi_IsMatchLocked() != 0);
    DWC_AddCommonKeyValueString("LCK", number, text, '/');
    return DWCi_SetGPStatus(status, text, NULL);
}

static void DWCi_ShutdownMatch(void) {
    DWCi_SetMatchCnt(0);
    if (sMatchState.query != NULL) {
        DWC_Free(4, sMatchState.query, 0);
        sMatchState.query = NULL;
    }
    DWCi_ClearGameMatchKeys();
}

static BOOL DWCi_IsShutdownMatch(void) {
    if (DWCi_GetMatchCnt() == NULL) {
        return TRUE;
    }
    return FALSE;
}

static void DWCi_ResetMatchParam(int mode) {
    DWCMatchControl *work;

    DWCi_GetMatchCnt()->retries = 0;
    DWCi_GetMatchCnt()->nnRecvCount = 0;
    work = DWCi_GetMatchCnt();
    work->nnCookieRand = DWCi_GetMathRand32(0x10000);
    DWCi_GetMatchCnt()->nnLastCookie = 0;
    DWCi_GetMatchCnt()->nnFailedTime = 0;
    DWCi_GetMatchCnt()->nnFinishTime = 0;
    DWCi_GetMatchCnt()->clLinkProgress = 0;
    DWCi_GetMatchCnt()->cancelState = 0;
    DWCi_GetMatchCnt()->scResvRetryCount = 0;
    DWCi_GetMatchCnt()->synResendCount = 0;
    DWCi_GetMatchCnt()->cancelSynResendCount = 0;
    DWCi_GetMatchCnt()->unk_628 = 0;
    DWCi_GetMatchCnt()->closeState = 0;
    DWCi_GetMatchCnt()->cancelBaseLatency = 0;
    DWCi_GetMatchCnt()->lastSynSent = 0;
    DWCi_GetMatchCnt()->closedTime = 0;
    MIi_CpuClear32(0, &DWCi_GetMatchCnt()->cmdCommand, 0x98);
    DWCi_GetMatchCnt()->memberConnecting = 0;
    func_ov011_021685fc();
    if (mode == 2) {
        if (DWCi_GetHostState() == 2) {
            DWCi_SetMatchStatus(0xd);
        } else if (DWCi_GetHostState() == 1) {
            DWCi_SetMatchStatus(1);
        }
        return;
    }
    DWCi_GetMatchCnt()->qr2IsReserved = 0;
    DWCi_GetMatchCnt()->qr2Reservation = 0;
    DWCi_GetMatchCnt()->sbUpdateFlag = 0;
    DWCi_GetMatchCnt()->baseLatency = 0;
    DWCi_GetMatchCnt()->friendCount = 0;
    DWCi_GetMatchCnt()->cmdResendFlag = 0;
    DWCi_GetMatchCnt()->cmdResendTick = 0;
    DWCi_GetMatchCnt()->cmdTimeoutTime = 0;
    DWCi_GetMatchCnt()->cmdTimeoutStartTick = 0;
    DWCi_GetMatchCnt()->synAckBit = 0;
    DWCi_GetMatchCnt()->reqProfileID = 0;
    MIi_CpuClear32(0, &DWCi_GetMatchCnt()->target, 0xc);
    DWCi_GetMatchCnt()->newHostAckBit = 0;
    DWCi_GetMatchCnt()->connectedAidBit = 0;
    DWCi_GetMatchCnt()->groupSearchCount = 0;
    DWCi_GetMatchCnt()->sbSearchHostsOnly = 1;
    DWCi_GetMatchCnt()->sbSearchCount = 0;
    DWCi_GetMatchCnt()->hostState = 0;
    DWCi_GetMatchCnt()->matched = 0;
    DWCi_GetMatchCnt()->checkFunc = 0;
    DWCi_GetMatchCnt()->checkParam = 0;
    DWCi_InitNodeInfoList();
    DWCi_ClearTargetNodeInfo();
    DWCi_GetMatchCnt()->pending.profileId = 0;
    if (mode == 1) {
        if (DWCi_GetMatchCnt()->qr2MatchType == 0 || DWCi_GetMatchCnt()->qr2MatchType == 1) {
            DWCi_SetMatchStatus(0x16);
        }
        return;
    }
    DWCi_GetMatchCnt()->mode = 0;
    DWCi_GetMatchCnt()->qr2NumEntry = 0;
    DWCi_GetMatchCnt()->qr2ShutdownFlag = 0;
    DWCi_GetMatchCnt()->cbEventPid = 0;
    DWCi_GetMatchCnt()->clWaitTimeoutCount = 0;
    DWCi_GetMatchCnt()->unk_670 = 0;
    DWCi_GetMatchCnt()->newClientCallback = 0;
    DWCi_GetMatchCnt()->newClientParam = 0;
    MI_CpuFill8(&DWCi_GetMatchCnt()->userData, 0, sizeof(int));
}

static void DWCi_SetMatchCommonParam(u8 mode, u8 unk17, DWCMatchedSCCallback callback, void *param) {
    DWCi_ResetMatchParam(0);
    DWCi_GetMatchCnt()->mode = mode;
    DWCi_GetMatchCnt()->qr2NumEntry = unk17;
    DWCi_GetMatchCnt()->notifyFunc = callback;
    DWCi_GetMatchCnt()->notifyParam = param;
    DWCi_GetMatchCnt()->nnFailureCount = 0;
    DWCi_GetMatchCnt()->suspendRequest = 0xff;
    DWCi_GetMatchCnt()->unk_30 = 0;
    DWCi_GetMatchCnt()->hostState = 0;
    DWCi_GetMatchCnt()->sbSearchHostsOnly = 1;
    DWCi_GetMatchCnt()->sbSearchCount = 0;
    DWCi_GetMatchCnt()->qr2NumEntry = unk17;
    DWCi_GetMatchCnt()->suspendAidBitmap = 0;
    DWCi_GetMatchCnt()->suspend = 0;
    DWCi_GetMatchCnt()->suspendSendTick = 0;
    DWCi_GetMatchCnt()->suspendPending = 0;
    DWCi_GetMatchCnt()->unlocked = 1;
    DWCi_GetMatchCnt()->suspendWaitTime = 0;
    DWCi_GetMatchCnt()->resvToken = 0;
}

static void DWCi_CloseMatching(void) {
    if (DWCi_GetMatchCnt() != NULL) {
        if (DWCi_GetMatchCnt()->sbObj != 0) {
            if (sMatchState.sbCallbackLevel == 0) {
                ServerBrowserFree(DWCi_GetMatchCnt()->sbObj);
                DWCi_GetMatchCnt()->sbObj = 0;
            } else {
                sMatchState.needSbFree = 1;
            }
        }
        DWCi_NNFreeNegotiateList();
        DWCi_SetMatchStatus(0);
        if (sMatchState.query != NULL) {
            DWC_Free(4, sMatchState.query, 0);
            sMatchState.query = NULL;
        }
        DWCi_ClearGameMatchKeys();
        DWCi_GetMatchCnt()->qr2ShutdownFlag = 1;
        DWCi_InitNodeInfoList();
        DWCi_ClearTargetNodeInfo();
        DWCi_GetMatchCnt()->unlocked = 1;
        DWCi_GPSetServerStatus(1);
    }
}

static int DWCi_SBUpdateAsync(u32 pid) {
    int flag = 1;
    char text[0x200];
    u8 list[0xcc];
    int i;
    int count;
    u8 roll;
    u64 now;
    int result;
    int len;

    count = 6;
    list[0] = 8;
    list[1] = 10;
    list[2] = 50;
    list[3] = 51;
    list[4] = 52;
    list[5] = 53;
    list[count++] = 54;
    list[count++] = 55;
    list[count++] = 56;
    if (DWCi_GetMatchCnt()->qr2MatchType == 0 || DWCi_GetMatchCnt()->qr2MatchType == 1) {
        for (i = 0; i < 0x9a; i++) {
            if (sKeys[i].id != 0) {
                list[count] = sKeys[i].id;
                count++;
            }
        }
    }
    switch (DWCi_GetMatchCnt()->state) {
    case 2:
        if (sMatchChance[0] == -2) {
            roll = DWCi_GetMatchCnt()->sbSearchHostsOnly == 1 ? 2 : 3;
            DWCi_GetMatchCnt()->sbSearchHostsOnly = 0;
        } else if (sMatchChance[0] == -1) {
            roll = DWCi_GetMatchCnt()->sbSearchHostsOnly != 0 ? 2 : 0;
            DWCi_GetMatchCnt()->sbSearchHostsOnly ^= 1;
        } else {
            roll = (s32)DWCi_GetMathRand32(100) < sMatchChance[0] ? 2 : 0;
        }
        len = DWCi_GetDefaultMatchFilter(text, DWCi_GetMatchCnt()->profileID, DWCi_GetMatchCnt()->qr2NumEntry,
                                         DWCi_GetMatchCnt()->qr2MatchType, roll);
        if (sMatchState.query != NULL) {
            OS_SNPrintf(text + len, 0x1ff - len, " and (%s)", sMatchState.query);
            text[0x1fe] = 0;
        }
        DWCi_GetMatchCnt()->sbSearchCount++;
        break;
    case 3:
    case 4:
    case 0x16:
        OS_SNPrintf(text, 0x1ff, "%s = %u", "dwc_pid", pid);
        DWCi_GetMatchCnt()->reqProfileID = pid;
        break;
    case 9:
    case 10:
    case 11:
        OS_SNPrintf(text, 0x1ff, "%s = %u", "dwc_pid", pid);
        flag = 0;
        break;
    case 12:
        OS_SNPrintf(text, 0x1ff, "(%s = %u) and (%s = %u) and (%s != %u)", "dwc_groupid", DWCi_GetMatchCnt()->groupId,
                    "dwc_hoststate", 2, "dwc_pid", DWCi_GetMatchCnt()->profileID);
        break;
    }
    ServerBrowserClear(DWCi_GetMatchCnt()->sbObj);
    for (i = 0; i < 1; i++) {
        result = ServerBrowserLimitUpdateA(DWCi_GetMatchCnt()->sbObj, flag, 0, list, count, text,
                                           DWCi_GetMatchCnt()->state == 0x16 ? 1 : 6);
        if (result == 0 || result != 2) {
            break;
        }
    }
    if (result == 0) {
        now = OS_GetTick();
        DWCi_GetMatchCnt()->sbUpdateRequestTick = now + OS_MilliSecondsToTicks(30000);
    }
    return result;
}

static int DWCi_GetDefaultMatchFilter(char *out, u32 a, u32 b, u32 c, u8 kind) {
    char condition[0x200];

    if (kind == 3) {
        OS_SNPrintf(condition, 0x1ff, "(%s = %u or %s = %u)", "dwc_hoststate", 0, "dwc_hoststate", 2);
    } else {
        OS_SNPrintf(condition, 0x1ff, "%s = %u", "dwc_hoststate", kind);
    }
    OS_SNPrintf(out, 0x1ff,
                "%s = %d and %s != %u and maxplayers = %d and numplayers < %d and %s = %d and %s = %u and %s",
                "dwc_mver", 0x5a, "dwc_pid", a, b, b, "dwc_mtype", c, "dwc_suspend", 0, condition);
    return strlen(out);
}

static BOOL IsPrivateAddr(u32 ip) {
    u32 swapped = ((ip >> 24) & 0xff) | ((ip >> 8) & 0xff00) | ((ip << 8) & 0x00ff0000) | ((ip << 24) & 0xff000000);
    u8 first = (swapped & 0xff000000) >> 24;
    u8 second = (swapped & 0x00ff0000) >> 16;

    if (first == 10) {
        return TRUE;
    }
    if (first == 172 && second >= 16 && second <= 31) {
        return TRUE;
    }
    if (first == 192 && second == 168) {
        return TRUE;
    }
    return FALSE;
}

static int DWCi_NNStartupAsync(int reset, u32 token, DWCNodeInfo *peer) {
    int mismatch;
    int result = 0;
    u32 hostInfo[2];

    NNCancel(DWCi_GetMatchCnt()->target.cookie);
    if (reset == 0) {
        token = (u16)DWCi_GetMatchCnt()->profileID | (DWCi_GetMatchCnt()->nnCookieRand << 16);
        if (peer->hasPrivateAddress != 0) {
            if (peer->publicIp == ServerBrowserGetMyPublicIPAddr(DWCi_GetMatchCnt()->sbObj)) {
                peer->hostIp = peer->localIp;
                peer->hostPort = peer->localPort;
                mismatch = 0;
            } else {
                mismatch = 1;
            }
        } else if (IsPrivateAddr(SOC_GetHostID()) != 0) {
            mismatch = 1;
        } else {
            peer->hostIp = peer->publicIp;
            peer->hostPort = peer->publicPort;
            mismatch = 0;
        }
        if (mismatch) {
            DWCi_GetMatchCnt()->nnCookieRand = DWCi_GetMathRand32(0x10000);
            DWCi_GetMatchCnt()->target.cookie = token;
        } else {
            hostInfo[0] = SOC_GetHostID();
            hostInfo[1] = func_ov011_0217d38c(DWCi_GetMatchCnt()->fm->gt2Socket);
            int sent = DWCi_SendMatchCommand(6, peer->profileId, peer->publicIp, peer->publicPort, hostInfo, 2);
            DWCi_GetMatchCnt()->cmdCount = 0;
            if (sent != 0) {
                return 2;
            }
            DWCi_GetMatchCnt()->target.cookie = 0;
        }
        DWCi_GetMatchCnt()->target.kind = 0;
        DWCi_GetMatchCnt()->target.retryCount = 0;
        DWCi_GetMatchCnt()->target.port = peer->publicPort;
        DWCi_GetMatchCnt()->target.ip = peer->publicIp;
    } else {
        DWCi_GetMatchCnt()->target.kind = 1;
        mismatch = 1;
        DWCi_GetMatchCnt()->target.retryCount = 0;
        DWCi_GetMatchCnt()->target.port = 0;
        DWCi_GetMatchCnt()->target.ip = 0;
        DWCi_GetMatchCnt()->target.cookie = token;
    }
    if (mismatch) {
        result = DWCi_DoNatNegotiationAsync(&DWCi_GetMatchCnt()->target);
    } else {
        u32 sock = gt2GetSocketSOCKET(DWCi_GetMatchCnt()->fm->gt2Socket);

        DWCi_NNCompletedCallback(0, sock, 0, &DWCi_GetMatchCnt()->target);
        DWCi_GetMatchCnt()->nnFinishTime = 0;
    }
    return result;
}

static int DWCi_DoNatNegotiationAsync(MatchTarget *target) {
    int result;
    int i;

    if (target->kind == 0) {
        DWCMatchControl *work = DWCi_GetMatchCnt();
        if (DWCi_HandleSBError(ServerBrowserSendNatNegotiateCookieToServerA(
                work->sbObj, func_ov011_0217ed14(target->ip, 0, 0), target->port, target->cookie)) != 0) {
            return 2;
        }
    }
    for (i = 0; i < 1; i++) {
        result = NNBeginNegotiationWithSocket(gt2GetSocketSOCKET(DWCi_GetMatchCnt()->fm->gt2Socket), target->cookie,
                                              target->kind, DWCi_NNProgressCallback, DWCi_NNCompletedCallback, target);
        if (result == 0 || result != 3) {
            break;
        }
    }
    return result;
}

static BOOL DWCi_CanSendMatchCommandByGT2(int type, int id) {
    BOOL result = TRUE;

    if (DWCi_GetGT2ConnectionByProfileID(id, 1) == 0) {
        return FALSE;
    }
    if (DWCi_GetNodeInfoByProfileID(id) == NULL) {
        result = FALSE;
    }
    return result;
}

static int DWCi_SendMatchCommand(u8 type, int id, u32 ip, u16 port, const u32 *data, int count) {
    int result;
    MatchPacket packet;
    char text[0x200];
    DWCNodeInfo *peer;
    int length;
    u64 now;

    if (DWCi_CanSendMatchCommandByGT2(type, id)) {
        peer = DWCi_GetNodeInfoByProfileID(id);
        DWCi_MakeMatchCommandPacket(&packet, type, data, count);
        DWCi_SendUnreliable(DWC_GetMyAID(), peer->aid, &packet, packet.header.size + sizeof(MatchPacketHeader), FALSE);
        result = 0;
    } else if (gpIsBuddy(DWCi_GetMatchCnt()->pGpObj, id)) {
        length = 0;
        if (data != NULL && count != 0) {
            length = DWC_Base64Encode((const char *)data, count * 4, text, 0x200);
        }
        text[length] = 0;
        result = DWCi_SendGPBuddyMsgCommand(DWCi_GetMatchCnt()->pGpObj, type, id, text);
    } else {
        if (ip == 0) {
            peer = DWCi_GetNodeInfoByProfileID(id);
            ip = peer->publicIp;
            port = peer->publicPort;
        }
        result = DWCi_SendSBMsgCommand(type, ip, port, data, count);
    }
    if (type == 2 || type == 6 || type == 8 || type == 9) {
        DWCi_GetMatchCnt()->cmdCommand = type;
        DWCi_GetMatchCnt()->cmdPort = port;
        DWCi_GetMatchCnt()->cmdIp = ip;
        DWCi_GetMatchCnt()->cmdProfileId = id;
        DWCi_GetMatchCnt()->cmdLen = count;
        now = OS_GetTick();
        DWCi_GetMatchCnt()->cmdSendTime = now;
        if (data != NULL && count != 0) {
            MIi_CpuCopy32(data, DWCi_GetMatchCnt()->cmdData, count * 4);
        }
    }
    return result;
}

static void DWCi_MakeMatchCommandPacket(MatchPacket *packet, int type, const u32 *data, int count) {
    if (data != NULL && count != 0) {
        MIi_CpuCopy32(data, packet->data, count * 4);
    } else {
        count = 0;
    }
    strcpy(packet->header.magic, "\xbbI\xccM");
    packet->header.version = 0x5a;
    packet->header.command = type;
    packet->header.size = count * 4;
    packet->header.port = DWCi_GetMatchCnt()->qr2Port;
    packet->header.ip = DWCi_GetMatchCnt()->qr2IP;
    packet->header.profileId = DWCi_GetMatchCnt()->profileID;
}

static int DWCi_SendSBMsgCommand(int type, u32 ip, u16 port, const u32 *data, int count) {
    MatchPacket packet;
    int result;
    int i;

    DWCi_MakeMatchCommandPacket(&packet, type, data, count);
    for (i = 0; i < 1; i++) {
        result =
            ServerBrowserSendMessageToServerA(DWCi_GetMatchCnt()->sbObj, func_ov011_0217ed14(ip, 0, NULL), port,
                                              (const char *)&packet, packet.header.size + sizeof(MatchPacketHeader));
        if (result == 0 || result != 2) {
            break;
        }
    }
    return result;
}

static int DWCi_SendGPBuddyMsgCommand(void *gp, u8 command, int id, const char *text) {
    char buffer[0x200];
    int length;
    char *end;

    length = OS_SNPrintf(buffer, 0x200, "%s%dv%s", "GPCM", 0x5a, "MAT");
    buffer[length] = command;
    end = buffer + 1 + length;
    *end = 0;
    if (text != NULL) {
        int textLength = strlen(text);

        MI_CpuCopy8(text, end, textLength);
        end[textLength] = 0;
    }
    return gpSendBuddyMessage(gp, id, buffer);
}

static int DWCi_ProcessRecvMatchCommand(int type, s32 profileId, u32 ip, u32 port, const u32 *data, int count) {
    int n = 0;
    u32 reply[0x40];
    DWCNodeInfo request;
    DWCNodeInfo accepted;
    SOCSockAddrIn address;
    u32 sendId[1];
    u8 result;
    u8 saved;
    u32 requestType;
    u32 requestExtra;
    u32 acceptedExtra;
    u32 acceptedUnk24;
    u32 joinId;
    u8 joinAid;
    int blocked;
    u32 requestSkip;
    u32 requestCookie;
    DWCNodeInfo *host;
    u8 i;
    u32 code;
    u64 now;
    u32 hostIp;
    u16 hostPort;
    int conn;
    u32 id;
    u8 aid;
    DWCNodeInfo *node;
    DWCNodeInfo *self;
    int k;
    u8 index;
    int inMask;
    int members;
    u8 mode;
    u8 value;
    u8 flag;

    if (DWCi_GetMatchCnt() == NULL || DWCi_GetMatchCnt()->state == 0) {
        return 1;
    }
    if (DWC_GetState() == 4 && DWCi_GetMatchCnt()->state == 1 && DWCi_GetNumNodeInfo() > 0 &&
        profileId == DWCi_GetNodeInfoByIndex(0)->profileId) {
        DWCi_InitClWaitTimeout();
    }
    switch (type) {
    case 1: {
        blocked = 0;
        requestType = data[0];
        MI_CpuFill8(&request, 0, sizeof(DWCNodeInfo));
        request.profileId = profileId;
        request.aid = 0xff;
        request.publicIp = data[1];
        request.publicPort = data[2];
        request.localIp = data[3];
        request.localPort = data[4];
        request.hasPrivateAddress = data[5];
        requestSkip = data[6];
        requestCookie = data[7];
        memcpy(&request.userData, &data[8], sizeof(u32));
        requestExtra = data[9];
        if (count >= 11 && data[10] == 1) {
            blocked = 1;
        }
        result = DWCi_CheckResvCommand(request.profileId, request.publicIp, request.publicPort, requestType,
                                       requestSkip != 0, requestCookie, &request.userData, blocked);
        if (result == 2) {
            DWCi_ClearTargetNodeInfo();
            DWCi_GetMatchCnt()->self = request;
            if (DWCi_HandleMatchCommandError(DWCi_ProcessResvOK(profileId, ip, port)) != 0) {
                return 0;
            }
            if (DWCi_GetMatchCnt()->memberConnecting != 0) {
                node = DWCi_GetNodeInfoByProfileID(DWCi_GetMatchCnt()->self.profileId);
                DWCi_GetMatchCnt()->self.aid = node->aid;
            }
            if (DWCi_GetMatchCnt()->memberConnecting == 0 && DWCi_GetMatchCnt()->mode == 2 &&
                DWCi_GetMatchCnt()->state != 0xe &&
                ((DWCi_GetMatchCnt()->qr2MatchType != 0 && DWCi_GetMatchCnt()->qr2MatchType != 1) ||
                 DWCi_GetMatchCnt()->matched != 0) &&
                DWCi_GetMatchCnt()->newClientCallback != NULL) {
                DWCi_GetMatchCnt()->newClientCallback(DWCi_GetFriendListIndex(profileId),
                                                      DWCi_GetMatchCnt()->newClientParam);
            }
            host = DWCi_GetOwnNodeInfo();
            n = 0;
            reply[n++] = DWCi_GetMatchCnt()->qr2NumEntry;
            reply[n++] = host->aid;
            reply[n++] = host->profileId;
            reply[n++] = host->publicIp;
            reply[n++] = host->publicPort;
            reply[n++] = host->localIp;
            reply[n++] = host->localPort;
            reply[n++] = host->hasPrivateAddress;
            reply[n++] = host->userData;
            reply[n++] = DWCi_GetMatchCnt()->groupId;
            reply[n++] = DWCi_GetMatchCnt()->self.aid;
            reply[n++] = requestExtra;
            reply[n++] = DWCi_GetNumNodeInfo();
            reply[n++] = DWCi_GetMatchCnt()->memberConnecting != 0 ? 1 : 0;
            DWCi_GetMatchCnt();
            DWCi_SetMatchStatus(0xe);
        } else if (result == 3) {
            DWCi_GetResvDenyReasonString(DWCi_GetMatchCnt()->error);
            reply[0] = DWCi_GetMatchCnt()->error;
            n = 1;
        }
        if (result != 0xff) {
            if (DWCi_HandleMatchCommandError(DWCi_SendMatchCommand(result, request.profileId, request.publicIp,
                                                                   request.publicPort, reply, n)) != 0) {
                return 0;
            }
        }
        break;
    }
    case 2: {
        acceptedExtra = data[0];
        if (DWCi_GetMatchCnt()->state != 3 || profileId != DWCi_GetMatchCnt()->reqProfileID || profileId != data[2] ||
            DWCi_GetMatchCnt()->resvToken != data[11]) {
            break;
        }
        if (count >= 14 && (data[13] == 1) != DWCi_GetMatchCnt()->memberConnecting) {
            break;
        }
        DWCi_GetMatchCnt()->unk_628 = 0;
        DWCi_GetMatchCnt()->cmdTimeoutTime = 0;
        DWCi_GetMatchCnt()->cmdResendFlag = 0;
        i = 1;
        MI_CpuFill8(&accepted, 0, sizeof(DWCNodeInfo));
        accepted.aid = data[i++];
        accepted.profileId = data[i++];
        accepted.publicIp = data[i++];
        accepted.publicPort = data[i++];
        accepted.localIp = data[i++];
        accepted.localPort = data[i++];
        accepted.hasPrivateAddress = data[i++];
        memcpy(&accepted.userData, &data[i++], sizeof(u32));
        acceptedUnk24 = data[i++];
        if (DWCi_GetMatchCnt()->memberConnecting == 0) {
            DWCi_GetOwnNodeInfo()->aid = data[i++];
        }
        DWCi_GetMatchCnt()->self = accepted;
        if (DWCi_GetMatchCnt()->mode == 3) {
            DWCi_GetMatchCnt()->groupId = acceptedUnk24;
            DWCi_GetMatchCnt()->qr2NumEntry = acceptedExtra;
        }
        if (DWCi_GetMatchCnt()->qr2MatchType == 1 && DWCi_GetMatchCnt()->evalCallback != NULL &&
            DWCi_GetMatchCnt()->memberConnecting == 0 && DWCi_GetMatchCnt()->unk_798 == 0) {
            DWCi_SetMatchStatus(4);
            if (DWCi_HandleSBError(DWCi_SBUpdateAsync(profileId)) != 0) {
                return 0;
            }
        } else {
            DWCi_SetMatchStatus(5);
            if (DWCi_HandleNNError(DWCi_NNStartupAsync(0, 0, &DWCi_GetMatchCnt()->self)) != 0) {
                return 0;
            }
        }
        break;
    }
    case 3: {
        if (DWCi_GetMatchCnt()->state != 3 || profileId != DWCi_GetMatchCnt()->reqProfileID) {
            break;
        }
        code = data[0];
        if (func_ov011_02168468() == 1) {
            DWCi_SetMatchStatus(1);
            break;
        }
        if (DWCi_GetMatchCnt()->unk_798 != 0 || DWCi_GetMatchCnt()->qr2MatchType == 3) {
            DWCi_StopMatching(code != 0x10 ? 0x15 : 0xc, 0);
            return 0;
        }
        return DWCi_RetryReserving(DWCi_GetMatchCnt()->reqProfileID);
    }
    case 4: {
        if (DWCi_GetMatchCnt()->state != 3) {
            break;
        }
        if (func_ov011_02168468() == 1) {
            DWCi_SetMatchStatus(1);
            break;
        }
        if (DWCi_GetMatchCnt()->unk_798 != 0) {
            if (profileId != DWCi_GetMatchCnt()->reqProfileID ||
                gpIsBuddy(DWCi_GetMatchCnt()->pGpObj, profileId) == 0) {
                break;
            }
            DWCi_GetMatchCnt()->scResvRetryMax = 1;
            DWCi_GetMatchCnt()->scResvRetryCount = 0;
            DWCi_GetMatchCnt()->cmdTimeoutTime = 12000;
            now = OS_GetTick();
            DWCi_GetMatchCnt()->cmdTimeoutStartTick = now;
        } else {
            if (profileId != DWCi_GetMatchCnt()->reqProfileID) {
                break;
            }
            now = OS_GetTick();
            DWCi_GetMatchCnt()->cmdTimeoutStartTick = now;
            DWCi_GetMatchCnt()->unk_628 = 0;
            if (DWCi_GetMatchCnt()->qr2MatchType == 0) {
                DWCi_ClearTargetNodeInfo();
                DWCi_SetMatchStatus(2);
                DWCi_GetMatchCnt()->sbUpdateFlag = 1;
                now = OS_GetTick();
                DWCi_GetMatchCnt()->sbUpdateTick = now;
            } else if (DWCi_GetMatchCnt()->qr2MatchType == 1) {
                DWCi_ClearTargetNodeInfo();
                DWCi_SendResvCommandToFriend(1, profileId, profileId);
            } else if (DWCi_GetMatchCnt()->qr2MatchType == 3) {
                DWCi_GetMatchCnt()->scResvRetryMax = 1;
                DWCi_GetMatchCnt()->scResvRetryCount = 0;
                DWCi_GetMatchCnt()->cmdTimeoutTime = 12000;
                now = OS_GetTick();
                DWCi_GetMatchCnt()->cmdTimeoutStartTick = now;
            }
        }
        break;
    }
    case 5:
        if (DWCi_GetMatchCnt()->qr2IsReserved == 0 || profileId != DWCi_GetMatchCnt()->qr2Reservation) {
            break;
        }
        if (func_ov011_02168468() == 1) {
            DWCi_SetMatchStatus(1);
            break;
        }
        if (DWCi_GetMatchCnt()->mode == 2 && DWCi_IsTargetNodeInfoValid() == 1 &&
            DWCi_GetGT2ConnectionByProfileID(DWCi_GetMatchCnt()->self.profileId, 0) != NULL &&
            profileId == DWCi_GetMatchCnt()->self.profileId) {
            gt2CloseAllConnectionsHard(DWCi_GetMatchCnt()->fm->gt2Socket);
        }
        if (DWCi_CancelPreConnectedServerProcess(profileId) == 0) {
            return 0;
        }
        break;
    case 6: {
        hostIp = data[0];
        hostPort = data[1];
        if (DWCi_IsTargetNodeInfoValid() == 0) {
            break;
        }
        if (func_ov011_021686a0() == 0 && DWCi_GetMatchCnt()->mode == 3 && DWCi_GetMatchCnt()->memberConnecting == 0) {
            break;
        }
        if (DWCi_GetMatchCnt()->state == 1) {
            DWCi_SetMatchStatus(5);
        } else if (!((DWCi_GetMatchCnt()->state == 5 || DWCi_GetMatchCnt()->state == 0xe) &&
                     profileId == DWCi_GetMatchCnt()->qr2Reservation)) {
            break;
        }
        DWCi_GetMatchCnt()->cmdCommand = 0xff;
        address.port = MATCH_SWAP16(hostPort);
        address.addr.addr = hostIp;
        DWCi_GetMatchCnt()->target.kind = 1;
        conn = gt2GetSocketSOCKET(DWCi_GetMatchCnt()->fm->gt2Socket);
        DWCi_NNCompletedCallback(0, conn, &address, &DWCi_GetMatchCnt()->target);
        DWCi_GetMatchCnt()->nnFinishTime = 0;
        break;
    }
    case 7: {
        if (func_ov011_02168468() == 1) {
            DWCi_SetMatchStatus(1);
        }
        if (DWCi_GetMatchCnt()->state != 1 || profileId != DWCi_GetNodeInfoByIndex(0)->profileId) {
            break;
        }
        id = data[0];
        aid = data[1];
        if (DWCi_GetMatchCnt()->pending.profileId != 0) {
            break;
        }
        DWCi_GetMatchCnt()->newHostAckBit = 0;
        if (func_ov011_021686a0() != 0) {
            DWCi_GetMatchCnt()->self.profileId = id;
            DWCi_GetMatchCnt()->self.aid = aid;
        } else {
            DWCi_GetMatchCnt()->pending.profileId = id;
        }
        if (DWCi_GetMatchCnt()->newClientCallback != NULL) {
            DWCi_GetMatchCnt()->newClientCallback(DWCi_GetFriendListIndex(id), DWCi_GetMatchCnt()->newClientParam);
        }
        break;
    }
    case 9: {
        node = DWCi_GetNodeInfoByIndex(func_ov011_021686b8() - 1 - DWCi_GetMatchCnt()->clLinkProgress);
        if (DWCi_GetMatchCnt()->state != 0x10 || node == NULL || node->profileId != data[0]) {
            break;
        }
        DWCi_GetMatchCnt()->clLinkProgress++;
        DWCi_PostProcessConnection(0);
        break;
    }
    case 8: {
        if (DWCi_GetMatchCnt()->state != 1 || profileId != DWCi_GetMatchCnt()->self.profileId) {
            break;
        }
        joinId = data[0];
        self = &DWCi_GetMatchCnt()->self;
        joinAid = data[2];
        if (DWCi_GetGT2Connection(joinAid) != NULL) {
            sendId[0] = joinId;
            if (DWCi_HandleMatchCommandError(
                    DWCi_SendMatchCommand(9, profileId, self->publicIp, self->publicPort, sendId, 1)) != 0) {
                return 0;
            }
            break;
        }
        DWCi_GetMatchCnt()->pending = DWCi_GetMatchCnt()->self;
        self->profileId = joinId;
        self->aid = joinAid;
        self->publicIp = data[3];
        self->publicPort = data[4];
        self->localIp = data[5];
        self->localPort = data[6];
        self->hasPrivateAddress = data[7];
        memcpy(&self->userData, &data[8], sizeof(u32));
        DWCi_SetMatchStatus(5);
        DWCi_GetMatchCnt()->cmdTimeoutTime = 0;
        DWCi_GetMatchCnt()->cmdResendFlag = 0;
        DWCi_NNStartupAsync(0, 0, self);
        break;
    }
    case 10: {
        if (DWCi_GetMatchCnt()->state != 1 || profileId != DWCi_GetServerNodeInfo()->profileId) {
            break;
        }
        id = data[0];
        aid = data[1];
        if (id == DWCi_GetMatchCnt()->self.profileId && aid == DWCi_GetMatchCnt()->self.aid) {
            break;
        }
        DWCi_GetMatchCnt()->self.aid = aid;
        DWCi_GetMatchCnt()->self.profileId = id;
        break;
    }
    case 12:
        if (profileId != DWCi_GetMatchCnt()->self.profileId) {
            break;
        }
        if (DWCi_GetMatchCnt()->qr2MatchType == 0 || DWCi_GetMatchCnt()->qr2MatchType == 1) {
            if (DWCi_CancelPreConnectedClientProcess(profileId) == 0) {
                return 0;
            }
            break;
        }
        if (DWCi_GetMatchCnt()->mode != 3) {
            break;
        }
        DWCi_GetMatchCnt()->cbEventPid = profileId;
        DWCi_CloseAllConnectionsByTimeout();
        DWCi_RestartFromCancel(0);
        break;
    case 13:
    case 14:
    case 15:
        if (DWCi_ProcessCancelMatchSynCommand(profileId, type, data[0]) == 0) {
            return 0;
        }
        break;
    case 16: {
        node = DWCi_GetServerNodeInfo();
        if (node == NULL) {
            return 1;
        }
        if (profileId != node->profileId) {
            return 1;
        }
        saved = DWCi_GetMatchCnt()->closeState;
        DWCi_GetMatchCnt()->closeState = 1;
        for (k = 0; k < count; k++) {
            node = DWCi_GetNodeInfoByProfileID(data[k]);
            if (node == NULL) {
                if (DWCi_GetMatchCnt()->pending.profileId == data[k]) {
                    DWCi_CloseConnectionHard(DWCi_GetMatchCnt()->pending.aid);
                    DWCi_GetMatchCnt()->pending.profileId = 0;
                }
            } else {
                DWCi_CloseConnectionHard(node->aid);
            }
        }
        DWCi_GetMatchCnt()->closeState = saved;
        break;
    }
    case 0x40:
        if (DWCi_IsTargetNodeInfoValid() == 0) {
            break;
        }
        if (profileId == DWCi_GetMatchCnt()->self.profileId) {
            if (DWCi_HandleMatchCommandError(DWCi_SendMatchCommand(0x41, profileId, ip, port, NULL, 0)) != 0) {
                return 0;
            }
        }
        break;
    case 0x41:
        break;
    case 0x52: {
        inMask = 0;
        if (DWCi_GetMatchCnt()->matched == 0) {
            break;
        }
        if (DWCi_IsServerNodeMyself() != 0) {
            return 0;
        }
        index = DWCi_GetAIDFromProfileID(DWCi_GetMatchCnt()->profileID, 1);
        if (index == 0xff) {
            return 0;
        }
        if (DWCi_GetAIDFromProfileID(profileId, 1) == 0xff) {
            return 0;
        }
        if (profileId == DWCi_GetNodeInfoByIndex(0)->profileId) {
            inMask = (DWCi_GetMatchCnt()->newHostAckBit & (1 << index)) != 0;
        }
        if (DWCi_HandleMatchCommandError(
                DWCi_SendMatchCommand(inMask != 0 ? 0x53 : 0x54, profileId, ip, port, NULL, 0)) != 0) {
            return 0;
        }
        now = OS_GetTick();
        DWCi_GetMatchCnt()->unk_670 = now;
        if (inMask != 0 && DWCi_GetMatchCnt()->state == 0xa) {
            DWCi_SetMatchStatus(1);
            DWCi_UpdateAllNodesRecvTime();
        }
        break;
    }
    case 0x53: {
        if (DWCi_GetMatchCnt()->matched == 0) {
            break;
        }
        if (DWCi_IsServerNodeMyself() == 0) {
            break;
        }
        index = DWCi_GetAIDFromProfileID(profileId, 1);
        if (index == 0xff) {
            return 0;
        }
        DWCi_GetMatchCnt()->newHostAckBit |= 1 << index;
        if (DWCi_HandleMatchCommandError(DWCi_SendMatchCommand(0x53, profileId, ip, port, NULL, 0)) != 0) {
            return 0;
        }
        break;
    }
    case 0x54: {
        if (DWCi_GetMatchCnt()->matched == 0) {
            break;
        }
        members = DWCi_GetNumNodeInfo();
        if (++DWCi_GetMatchCnt()->newHostNakCount > members * 5) {
            DWC_CloseAllConnectionsHard();
        } else {
            now = OS_GetTick();
            DWCi_GetMatchCnt()->unk_670 = now;
        }
        break;
    }
    case 0x55:
        DWCi_GetMatchCnt();
        break;
    case 0x70:
        DWCi_SBUpdateAsync(0);
        DWCi_InitClWaitTimeout();
        DWCi_SetMatchStatus(0xc);
        break;
    case 0x82: {
        mode = data[1];
        value = data[2];
        if (DWCi_GetNodeInfoByProfileID(data[0]) == NULL) {
            break;
        }
        if (DWCi_GetHostState() == 2) {
            flag = data[3];
            if (mode == 1) {
                break;
            }
            DWCi_SetSuspendAndNotify(flag, value, 1);
        } else if (DWCi_GetHostState() == 1) {
            if (mode == 0) {
                break;
            }
            if (count == 2) {
                if (DWCi_GetMatchCnt()->unk_30 == 1) {
                    break;
                }
                DWCi_GetMatchCnt()->unk_30 = 1;
                DWCi_GetMatchCnt()->suspendWaitTime = 8000;
                now = OS_GetTick();
                DWCi_GetMatchCnt()->suspendSendTick = now;
            } else {
                DWCi_GetMatchCnt()->unk_30 = 1;
                DWCi_GetMatchCnt()->suspendAidBitmap = data[3];
            }
        }
        break;
    }
    case 0x83:
        DWCi_ProcessServerAIDBitmapCommand(data);
        break;
    }
    return 1;
}

// Tells the game about the match, passing on the match's player and the game's own parameter
static inline void MatchNotify(DWCError error, BOOL cancel, BOOL self, BOOL isServer) {
    DWCi_GetMatchCnt()->notifyFunc(error, cancel, self, isServer,
                                   DWCi_GetFriendListIndex(DWCi_GetMatchCnt()->cbEventPid),
                                   DWCi_GetMatchCnt()->notifyParam);
}

static u8 DWCi_CheckResvCommand(int profileId, int ip, u16 port, int nodeType, int skipChecks, int cookie,
                                void *userData, int blocked) {
    u64 now;
    u8 result;

    DWCi_GetMatchCnt()->error = 1;
    switch (DWCi_GetHostState()) {
    case 1:
        if (DWCi_IsNodeNotConnected(profileId) != 0) {
            if (DWCi_GetMatchCnt()->memberConnecting == 0) {
                if (DWCi_GetMatchCnt()->state == 1) {
                    if (DWCi_GetMatchCnt()->pending.profileId == 0) {
                        DWCi_GetMatchCnt()->memberConnecting = 1;
                        func_ov011_021685fc();
                        result = 2;
                        now = OS_GetTick();
                        DWCi_GetMatchCnt()->cmdSendTime = now;
                    } else {
                        result = 4;
                    }
                } else {
                    result = 4;
                }
            } else if (DWCi_GetMatchCnt()->state == 3) {
                DWCNodeInfo *self = DWCi_GetOwnNodeInfo();

                if (profileId == DWCi_GetMatchCnt()->self.profileId) {
                    if (self->profileId < profileId) {
                        result = 2;
                        now = OS_GetTick();
                        DWCi_GetMatchCnt()->cmdSendTime = now;
                    } else {
                        result = 0xff;
                    }
                } else {
                    result = 4;
                }
            } else {
                result = 4;
            }
        } else {
            result = 3;
            DWCi_GetMatchCnt()->error = 0x14;
        }
        break;
    case 0:
    case 2:
        result = func_ov011_0216484c(profileId, ip, port, nodeType, skipChecks, cookie, userData, blocked);
        break;
    }
    return result;
}

static u8 func_ov011_0216484c(int profileId, int ip, int port, int nodeType, int skipChecks, int cookie, void *userData,
                              int blocked) {
    u8 result;

    if (DWCi_IsMatchLocked() != 0) {
        DWCi_GetMatchCnt()->error = 0x13;
        return 3;
    }
    if (blocked != 0) {
        DWCi_GetMatchCnt()->error = 0x18;
        return 3;
    }
    if (skipChecks == 0) {
        if ((nodeType == 0 && DWCi_GetMatchCnt()->qr2MatchType != 0) ||
            (nodeType == 1 && DWCi_GetMatchCnt()->qr2MatchType != 1) ||
            (nodeType == 3 && DWCi_GetMatchCnt()->qr2MatchType != 2)) {
            DWCi_GetMatchCnt()->error = 0x12;
            return 3;
        }
        if (DWCi_GetMatchCnt()->qr2MatchType == 1) {
            if (gpIsBuddy(DWCi_GetMatchCnt()->pGpObj, profileId) == 0 || DWCi_GetFriendListIndex(profileId) == -1) {
                return 0xff;
            }
            if (DWCi_IsFriendByIdxList(profileId) == 0) {
                DWCi_GetMatchCnt()->error = 0x15;
                return 3;
            }
        } else if (DWCi_GetMatchCnt()->qr2MatchType == 2) {
            if (gpIsBuddy(DWCi_GetMatchCnt()->pGpObj, profileId) == 0 || DWCi_GetFriendListIndex(profileId) == -1) {
                return 0xff;
            }
        }
    }
    if (DWCi_GetMatchCnt()->qr2NumEntry + 1 == func_ov011_021679c0()) {
        result = 3;
        DWCi_GetMatchCnt()->error = 0x10;
    } else if (DWCi_GetMatchCnt()->state == 0x16) {
        result = 4;
    } else if (func_ov011_02160fd8() != 0 || DWCi_GetMatchCnt()->suspendPending != 0) {
        result = 3;
        DWCi_GetMatchCnt()->error = 0x11;
    } else if (cookie != DWCi_GetMatchCnt()->topology) {
        result = 3;
        DWCi_GetMatchCnt()->error = 0x12;
    } else if (DWCi_GetNodeInfoByProfileID(profileId) != NULL) {
        result = 0xff;
    } else if (!((DWCi_GetMatchCnt()->state == 0xd || DWCi_GetMatchCnt()->state == 2 ||
                  DWCi_GetMatchCnt()->state == 3) &&
                 (DWCi_GetMatchCnt()->qr2IP != 0 || DWCi_GetMatchCnt()->qr2Port != 0) && (ip != 0 || port != 0))) {
        if (DWCi_GetMatchCnt()->state == 0xe && profileId == DWCi_GetMatchCnt()->qr2Reservation) {
            result = 0xff;
        } else {
            result = 4;
        }
    } else if (DWCi_GetMatchCnt()->mode != 2 && DWCi_GetMatchCnt()->reqProfileID != 0) {
        if (profileId == DWCi_GetMatchCnt()->reqProfileID) {
            if (DWCi_GetMatchCnt()->profileID < profileId) {
                result = 2;
            } else {
                result = 0xff;
            }
        } else {
            result = 3;
            DWCi_GetMatchCnt()->error = 0x17;
        }
    } else {
        result = 2;
    }
    if (result == 2) {
        if (DWCi_GetMatchCnt()->checkFunc != NULL &&
            DWCi_GetMatchCnt()->checkFunc(userData, DWCi_GetMatchCnt()->checkParam) == 0) {
            DWCi_GetMatchCnt()->error = 0x16;
            return 3;
        }
        if (DWCi_GetHostState() == 0) {
            DWCi_ChangeToServer(1);
        }
    }
    return result;
}

static int DWCi_ProcessResvOK(int profileId, u32 ip, u32 port) {
    u32 data[16];
    DWCNodeInfo *self;
    u8 aid;
    int i;

    if (DWCi_GetMatchCnt()->qr2IsReserved != 0 && profileId == DWCi_GetMatchCnt()->qr2Reservation) {
        return 0;
    }
    DWCi_GetMatchCnt()->qr2IsReserved = 1;
    DWCi_GetMatchCnt()->qr2Reservation = profileId;
    DWCi_GetMatchCnt()->cmdResendFlag = 0;
    DWCi_GetMatchCnt()->cmdTimeoutTime = 0;
    qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
    DWCi_GetMatchCnt()->reqProfileID = 0;
    aid = DWCi_GetFreeAID();
    DWCi_GetMatchCnt()->self.aid = aid;
    if (DWCi_GetMatchCnt()->memberConnecting == 0) {
        data[0] = DWCi_GetMatchCnt()->self.profileId;
        data[1] = DWCi_GetMatchCnt()->self.aid;
        self = DWCi_GetOwnNodeInfo();
        for (i = 0; i < DWCi_GetNumNodeInfo(); i++) {
            DWCNodeInfo *node = DWCi_GetNodeInfoByIndex(i);

            if (self->profileId != node->profileId) {
                int result = DWCi_SendMatchCommand(7, node->profileId, node->publicIp, node->publicPort, data, 2);

                if (result != 0) {
                    return result;
                }
            }
        }
    }
    return 0;
}

static int DWCi_HandleMatchCommandError(int error) {
    if (DWCi_GetMatchCnt()->qr2MatchType == 0) {
        return DWCi_HandleSBError(error);
    }
    return DWCi_HandleMatchGPError(error);
}

static int DWCi_SendResvCommand(int profileId, int flag) {
    u64 now;
    u32 data[16];
    DWCNodeInfo *self;
    u32 ip = 0;
    u16 port = 0;

    if (flag != 0 || (DWCi_GetMatchCnt()->qr2IP == 0 && DWCi_GetMatchCnt()->qr2Port == 0)) {
        DWCi_GetMatchCnt()->cmdResendFlag = 1;
        now = OS_GetTick();
        DWCi_GetMatchCnt()->cmdResendTick = now;
        DWCi_GetMatchCnt()->reqProfileID = 0;
        return 0;
    }
    if (DWCi_GetMatchCnt()->qr2MatchType == 0) {
        ip = DWCi_GetMatchCnt()->self.publicIp;
        port = DWCi_GetMatchCnt()->self.publicPort;
        DWCi_GetMatchCnt()->reqProfileID = profileId;
    } else {
        DWCi_GetMatchCnt()->reqProfileID = profileId;
        if (gpIsBuddy(DWCi_GetMatchCnt()->pGpObj, profileId) == 0) {
            ip = DWCi_GetMatchCnt()->self.publicIp;
            port = DWCi_GetMatchCnt()->self.publicPort;
        }
    }
    if (DWCi_GetMatchCnt()->qr2MatchType == 3 ||
        (DWCi_GetMatchCnt()->unk_798 != 0 && gpIsBuddy(DWCi_GetMatchCnt()->pGpObj, profileId) != 0)) {
        DWCi_GetMatchCnt()->scResvRetryMax = 0;
        DWCi_GetMatchCnt()->cmdTimeoutTime = 30000;
    } else {
        DWCi_GetMatchCnt()->scResvRetryMax = 5;
        DWCi_GetMatchCnt()->cmdTimeoutTime = 12000;
    }
    now = OS_GetTick();
    DWCi_GetMatchCnt()->cmdTimeoutStartTick = now;
    DWCi_GetMatchCnt()->cmdResendFlag = 0;
    self = DWCi_GetOwnNodeInfo();
    data[0] = DWCi_GetMatchCnt()->qr2MatchType;
    data[1] = self->publicIp;
    data[2] = self->publicPort;
    data[3] = self->localIp;
    data[4] = self->localPort;
    data[5] = self->hasPrivateAddress;
    data[6] = DWCi_GetMatchCnt()->unk_798;
    data[7] = DWCi_GetMatchCnt()->topology;
    data[8] = DWCi_GetMatchCnt()->userData;
    DWCi_GetMatchCnt()->resvToken = DWCi_GetMathRand32(-2) + 1;
    data[9] = DWCi_GetMatchCnt()->resvToken;
    data[10] = DWCi_GetMatchCnt()->memberConnecting != 0;
    return DWCi_SendMatchCommand(1, profileId, ip, port, data, 11);
}

static int DWCi_SendResvCommandToFriend(int a0, int a1, int a2) {
    u64 now;
    GPBuddyStatus status;
    int index;
    int found = 0;
    int profileId;
    int i;

    if (a1 == 0) {
        DWCi_GetMatchCnt()->friendCount++;
        DWCi_GetMatchCnt()->friendCount = DWCi_GetMatchCnt()->friendCount % DWCi_GetMatchCnt()->friendIdxListLen;
    }
    DWCi_GetMatchCnt()->mode = 3;
    for (i = 0; i < DWCi_GetMatchCnt()->friendIdxListLen;) {
        profileId = DWC_GetGsProfileId(
            DWCi_GetUserData(),
            &DWCi_GetMatchCnt()->friendList[DWCi_GetMatchCnt()->friendIdxList[DWCi_GetMatchCnt()->friendCount]]);
        if (profileId != 0 && profileId != -1 &&
            DWCi_Acc_IsValidFriendData(
                &DWCi_GetMatchCnt()->friendList[DWCi_GetMatchCnt()->friendIdxList[DWCi_GetMatchCnt()->friendCount]]) &&
            DWCi_GetNodeInfoByProfileID(profileId) == NULL) {
            int err1 = gpGetBuddyIndex(DWCi_GetMatchCnt()->pGpObj, profileId, &index);
            int err2 = gpGetBuddyStatus(DWCi_GetMatchCnt()->pGpObj, index, &status);

            err1 |= err2;
            if (DWCi_GetBuddyStatusLCK(&status) == 0 && err1 == 0 && status.status == 4) {
                found = 1;
                break;
            }
        }
        i++;
        DWCi_GetMatchCnt()->friendCount = (i + DWCi_GetMatchCnt()->friendCount) % DWCi_GetMatchCnt()->friendIdxListLen;
    }
    if (found != 0) {
        if (profileId == a2) {
            a0 = 1;
        }
        DWCi_ClearTargetNodeInfo();
        DWCi_GetMatchCnt()->self.aid = 0xff;
        DWCi_GetMatchCnt()->self.profileId = profileId;
        return DWCi_SendResvCommand(profileId, a0);
    }
    DWCi_GetMatchCnt()->cmdTimeoutTime = 3000;
    now = OS_GetTick();
    DWCi_GetMatchCnt()->cmdTimeoutStartTick = now;
    DWCi_GetMatchCnt()->cmdResendFlag = 0;
    return 0;
}

static int DWCi_RetryReserving(int a0) {
    u64 now;
    DWCi_GetMatchCnt()->reqProfileID = 0;
    DWCi_GetMatchCnt()->unk_628 = 0;
    now = OS_GetTick();
    DWCi_GetMatchCnt()->cmdTimeoutStartTick = now;
    DWCi_ClearTargetNodeInfo();
    if (DWCi_GetMatchCnt()->unk_798 != 0) {
        DWCi_SetMatchStatus(0xc);
        if (DWCi_HandleSBError(DWCi_SBUpdateAsync(0)) != 0) {
            return 0;
        }
    } else if (DWCi_GetMatchCnt()->qr2MatchType == 0) {
        DWCi_SetMatchStatus(2);
        if (a0 != 0) {
            DWCi_GetMatchCnt()->sbUpdateFlag = 1;
            now = OS_GetTick();
            DWCi_GetMatchCnt()->sbUpdateTick = now;
        } else {
            if (DWCi_HandleSBError(DWCi_SBUpdateAsync(0)) != 0) {
                return 0;
            }
        }
    } else if (DWCi_GetMatchCnt()->qr2MatchType == 1) {
        if (DWCi_HandleMatchCommandError(DWCi_SendResvCommandToFriend(1, 0, a0)) != 0) {
            return 0;
        }
    } else if (DWCi_GetMatchCnt()->qr2MatchType == 3) {
        DWCi_StopMatching(6, -80410);
        return 0;
    }
    return 1;
}

static int DWCi_CancelReservation(int unused) {
    int result;

    if (DWCi_IsTargetNodeInfoValid() == 0) {
        return 0;
    }
    result = DWCi_SendMatchCommand(5, DWCi_GetMatchCnt()->self.profileId, DWCi_GetMatchCnt()->self.publicIp,
                                   DWCi_GetMatchCnt()->self.publicPort, NULL, 0);
    DWCi_GetMatchCnt()->reqProfileID = 0;
    return result;
}

static int func_ov011_02165094(int profileId, int a1) {
    int i;

    if (DWCi_IsTargetNodeInfoValid() != 0) {
        DWCi_ClearTargetNodeInfo();
    }
    DWCi_GetMatchCnt()->cmdCommand = 0xff;
    if (DWCi_GetMatchCnt()->target.cookie != 0) {
        NNCancel(DWCi_GetMatchCnt()->target.cookie);
        DWCi_GetMatchCnt()->target.cookie = 0;
    }
    DWCi_GetMatchCnt()->reqProfileID = 0;
    if (DWCi_GetHostState() == 2) {
        DWCi_GetMatchCnt()->qr2IsReserved = 0;
        DWCi_GetMatchCnt()->qr2Reservation = 0;
        DWCi_SetMatchStatus(0x11);
        DWCi_GetMatchCnt()->cancelSynAckBit = 0;
        DWCi_GetMatchCnt()->cancelBaseLatency = 0;
        DWCi_CloseCancelHostAsync(profileId);
        for (i = 1; i < DWCi_GetNumNodeInfo(); i++) {
            if (DWCi_SendCancelMatchSynCommand(DWCi_GetNodeInfoByIndex(i)->profileId, 0xd) == 0) {
                return 0;
            }
        }
        if (DWCi_GetNumNodeInfo() == 1) {
            DWCi_RestartFromCancel(a1);
        }
    } else {
        func_ov011_02168468();
    }
    return 1;
}

static int DWCi_CancelPreConnectedServerProcess(int profileId) {
    int a1 = 2;

    if (DWCi_GetMatchCnt()->qr2MatchType == 0 || DWCi_GetMatchCnt()->qr2MatchType == 1) {
        if (DWCi_GetMatchCnt()->matched == 0) {
            a1 = 1;
        }
    }
    return func_ov011_02165094(profileId, a1);
}

static int DWCi_CancelPreConnectedClientProcess(int unused) {
    int result = 1;

    if (DWCi_GetMatchCnt()->unk_798 != 0 || DWCi_GetMatchCnt()->qr2MatchType == 3) {
        if (DWCi_GetNumNodeInfo() > 1) {
            DWCi_CloseAllConnectionsByTimeout();
        }
        if (DWCi_GetMatchCnt()->unk_798 != 0) {
            DWCi_StopMatching(0x16, -80432);
        } else {
            DWCi_StopMatching(0x16, -80430);
        }
        return 0;
    }
    if (func_ov011_02168468() == 1) {
        return result;
    }
    if (DWCi_GetMatchCnt()->pending.profileId != 0) {
        DWCi_GetMatchCnt()->pending.profileId = 0;
    }
    if (DWCi_GetMatchCnt()->target.cookie != 0) {
        NNCancel(DWCi_GetMatchCnt()->target.cookie);
        DWCi_GetMatchCnt()->target.cookie = 0;
    }
    if ((func_ov011_021686a0() != 0 && DWCi_GetMatchCnt()->unk_62f != 0) ||
        (DWCi_IsTargetNodeInfoValid() != 0 &&
         DWCi_GetGT2ConnectionByProfileID(DWCi_GetMatchCnt()->self.profileId, 1) != NULL) ||
        DWCi_GetNumNodeInfo() > 1) {
        DWCi_RestartFromTimeout();
    } else {
        DWCi_SetMatchStatus(3);
        result = DWCi_RetryReserving(0);
    }
    return result;
}

static void DWCi_PostProcessConnection(int a0) {
    switch (a0) {
    case 0:
        if (func_ov011_021686a0() != 0) {
            DWCi_SendClientLinkRequest();
        } else {
            DWCi_StartMatchSyn();
        }
        break;
    case 1:
        DWCi_SetMatchStatus(1);
        if (DWCi_GetMatchCnt()->mode == 3) {
            DWCi_GetMatchCnt()->cbEventPid = DWCi_GetMatchCnt()->self.profileId;
        }
        break;
    case 2:
        DWCi_SetMatchStatus(1);
        if (func_ov011_02168468() == 1) {
            func_ov011_021685fc();
        } else {
            DWCi_SetHostState(1);
            DWCi_GetMatchCnt()->mode = 3;
            if (DWCi_GetMatchCnt()->qr2MatchType == 0 || DWCi_GetMatchCnt()->qr2MatchType == 1) {
                DWCi_GetMatchCnt()->qr2IsReserved = 1;
                DWCi_GetMatchCnt()->qr2Reservation = DWCi_GetMatchCnt()->profileID;
            }
            DWCi_InitClWaitTimeout();
            if (func_ov011_021686a0() != 0) {
                if (DWCi_IsValidNodeInfo(&DWCi_GetMatchCnt()->pending) != 0) {
                    u32 data;
                    DWCNodeInfo *node;
                    int result;

                    node = &DWCi_GetMatchCnt()->self;
                    data = node->profileId;
                    DWCi_PushFrontNodeInfo(node);
                    DWCi_GetMatchCnt()->self = DWCi_GetMatchCnt()->pending;
                    node = &DWCi_GetMatchCnt()->self;
                    result = DWCi_SendMatchCommand(9, node->profileId, node->publicIp, node->publicPort, &data, 1);
                    DWCi_ClearNodeInfo(&DWCi_GetMatchCnt()->pending);
                    if (DWCi_HandleMatchCommandError(result) != 0) {
                        return;
                    }
                }
                DWCi_GetMatchCnt()->unk_62f = 1;
            }
        }
        break;
    case 3:
        DWCi_GetMatchCnt()->matched = 1;
        func_ov011_0216549c();
        DWCi_GetMatchCnt()->unk_798 = 0;
        qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
        MatchNotify(0, 0, DWCi_GetMatchCnt()->cbEventPid == 0, 0);
        if (DWCi_GetMatchCnt()->mode == 2) {
            if (DWCi_HandleMatchGPError(DWCi_GPSetServerStatus(-1)) != 0) {
                return;
            }
            DWCi_SetMatchStatus(0xd);
        } else {
            DWCi_GPSetServerStatus(2);
            DWCi_SetMatchStatus(1);
        }
        DWCi_GetMatchCnt()->cbEventPid = 0;
        DWCi_GetMatchCnt()->cancelState = 0;
        DWCi_UpdateAllNodesRecvTime();
        break;
    }
    if (DWCi_GetMatchCnt()->sbObj != NULL) {
        DWCMatchControl *ctrl;

        ServerBrowserClear(DWCi_GetMatchCnt()->sbObj);
        ctrl = DWCi_GetMatchCnt();
        ctrl->sbUpdateRequestTick = 0;
    }
}

static void func_ov011_0216549c(void) {
    DWCi_GetMatchCnt()->nnFailureCount = 0;
    if (func_ov011_021686a0() != 0) {
        func_ov011_02165540();
        return;
    }
    func_ov011_021654c0();
}

static void func_ov011_021654c0(void) {
    DWCNodeInfo *pending;
    DWCNodeInfo *node;

    if (DWCi_GetMatchCnt()->mode == 2) {
        if (DWCi_IsTargetNodeInfoValid() != 0) {
            DWCi_PushBackNodeInfo(&DWCi_GetMatchCnt()->self);
            DWCi_ClearTargetNodeInfo();
        }
    } else if (DWCi_GetNumNodeInfo() == 1) {
        DWCi_PushFrontNodeInfo(&DWCi_GetMatchCnt()->self);
        DWCi_ClearTargetNodeInfo();
    } else {
        pending = &DWCi_GetMatchCnt()->pending;
        if (pending->profileId != 0) {
            node = DWCi_GetNodeInfoByProfileID(pending->profileId);
            if (node != NULL) {
                DWCi_CloseConnectionHard(node->aid);
            }
            node = DWCi_GetNodeInfoByAID(pending->aid);
            if (node != NULL) {
                DWCi_CloseConnectionHard(node->aid);
            }
            DWCi_PushBackNodeInfo(pending);
            pending->profileId = 0;
        }
    }
}

static void func_ov011_02165540(void) {
    func_ov011_021654c0();
    DWCi_GetMatchCnt()->unk_62f = 0;
}

static int DWCi_StartMatchSyn(void) {
    int i;
    int result = 0;
    u64 now;

    if (DWCi_GetMatchCnt()->memberConnecting != 0 || DWCi_GetMatchCnt()->abortedMemberState == 0xf) {
        DWCi_GetMatchCnt()->qr2IsReserved = 0;
        DWCi_GetMatchCnt()->qr2Reservation = 0;
        DWCi_SetMatchStatus(1);
        func_ov011_02168468();
        return 0;
    }
    DWCi_GetMatchCnt()->qr2IsReserved = 0;
    DWCi_GetMatchCnt()->qr2Reservation = 0;
    if (DWCi_GetMatchCnt()->matched == 0 && DWCi_GetNumNodeInfo() == 1) {
        now = OS_GetTick();
        DWCi_GetMatchCnt()->aidListSendTick = now;
    }
    qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
    if (DWCi_GetMatchCnt()->mode == 2) {
        DWCi_GetMatchCnt()->cbEventPid = DWCi_GetMatchCnt()->self.profileId;
    }
    DWCi_SetMatchStatus(0x13);
    DWCi_GetMatchCnt()->synAckBit = 0;
    for (i = 1; i < DWCi_GetNumNodeInfo(); i++) {
        DWCi_SendMatchSynPacket(DWCi_GetNodeInfoByIndex(i)->aid, 2);
    }
    DWCi_SendMatchSynPacket(DWCi_GetMatchCnt()->self.aid, 2);
    if (DWCi_GetMatchCnt()->mode != 2) {
        result = 1;
    }
    return result;
}

static int DWCi_SendClientLinkRequest(void) {
    u32 data[16];
    DWCNodeInfo saved;
    DWCNodeInfo *me;
    DWCNodeInfo *node;
    u32 index;

    saved = DWCi_GetMatchCnt()->self;
    me = &saved;
    if (DWCi_GetMatchCnt()->clLinkProgress < func_ov011_021686b8() - 1) {
        index = (func_ov011_021686b8() - 1 - DWCi_GetMatchCnt()->clLinkProgress) & 0xff;
        DWCi_SetMatchStatus(0x10);
        node = DWCi_GetNodeInfoByIndex(index);
        data[0] = node->profileId;
        data[1] = index;
        data[2] = node->aid;
        data[3] = node->publicIp;
        data[4] = node->publicPort;
        data[5] = node->localIp;
        data[6] = node->localPort;
        data[7] = node->hasPrivateAddress;
        data[8] = node->userData;
    } else {
        DWCi_GetMatchCnt()->clLinkProgress = 0;
        return DWCi_StartMatchSyn();
    }
    if (DWCi_GetMatchCnt()->state != 0x13) {
        if (DWCi_HandleMatchCommandError(
                DWCi_SendMatchCommand(8, me->profileId, me->publicIp, me->publicPort, data, 9)) != 0) {
            return 0;
        }
        node = DWCi_GetNodeInfoByIndex(index);
        data[0] = me->profileId;
        data[1] = me->aid;
        if (DWCi_HandleMatchCommandError(
                DWCi_SendMatchCommand(10, node->profileId, node->publicIp, node->publicPort, data, 2)) != 0) {
            return 0;
        }
        DWCi_GetMatchCnt()->cmdCount = 0;
    }
    return 0;
}

static void DWCi_DoCancelMatching(void) {
    int hasConn;

    DWCi_GetMatchCnt()->cbEventPid = 0;
    hasConn = 0;
    if (DWCi_GetMatchCnt()->state == 0x16) {
        DWCi_FinishCancelMatching();
        return;
    }
    if (DWCi_GT2GetConnectionListIdx() != 0) {
        hasConn = 1;
    }
    DWCi_GetMatchCnt()->cancelState = 1;
    if (DWCi_GetMatchCnt()->mode == 3) {
        if (DWCi_GetMatchCnt()->target.cookie != 0) {
            NNCancel(DWCi_GetMatchCnt()->target.cookie);
            DWCi_GetMatchCnt()->target.cookie = 0;
        }
        DWCi_GetMatchCnt()->cancelState = 1;
        if (hasConn != 0) {
            gt2CloseAllConnectionsHard(DWCi_GetMatchCnt()->fm->gt2Socket);
        } else {
            if (DWCi_HandleMatchCommandError(DWCi_CancelReservation(0)) != 0) {
                return;
            }
        }
        DWCi_FinishCancelMatching();
        return;
    }
    if (DWCi_GetMatchCnt()->state == 3 || DWCi_GetMatchCnt()->state == 4 || DWCi_GetMatchCnt()->state == 5 ||
        DWCi_GetMatchCnt()->state == 6 || DWCi_GetMatchCnt()->state == 0xe || DWCi_GetMatchCnt()->state == 0xf) {
        if (DWCi_GetMatchCnt()->reqProfileID != 0) {
            if (DWCi_HandleMatchCommandError(DWCi_CancelReservation(0)) != 0) {
                return;
            }
        }
        if (DWCi_HandleMatchCommandError(DWCi_InvalidateReservation()) != 0) {
            return;
        }
    }
    if (DWCi_GetMatchCnt()->target.cookie != 0) {
        NNCancel(DWCi_GetMatchCnt()->target.cookie);
        DWCi_GetMatchCnt()->target.cookie = 0;
    }
    if (hasConn != 0) {
        DWCi_GetMatchCnt()->cancelState = 1;
        gt2CloseAllConnectionsHard(DWCi_GetMatchCnt()->fm->gt2Socket);
    }
    DWCi_FinishCancelMatching();
}

static void DWCi_FinishCancelMatching(void) {
    int flag = 1;

    if (DWCi_HandleMatchGPError(DWCi_GPSetServerStatus(1)) == 0) {
        DWCi_CloseMatching();
        if (DWCi_GetMatchCnt()->cbEventPid == 0 && DWCi_GetMatchCnt()->mode != 2) {
            flag = 0;
        }
        MatchNotify(0, 1, DWCi_GetMatchCnt()->cbEventPid == 0, flag);
        DWCi_GetMatchCnt()->cancelState = 0;
    }
}

static int DWCi_InvalidateReservation(void) {
    u32 data = 0;
    int result = 0;

    if (DWCi_GetMatchCnt()->qr2IsReserved != 0 && DWCi_GetMatchCnt()->qr2Reservation != 0 &&
        DWCi_GetMatchCnt()->qr2Reservation != DWCi_GetMatchCnt()->profileID) {
        result = DWCi_SendMatchCommand(0xc, DWCi_GetMatchCnt()->qr2Reservation, DWCi_GetMatchCnt()->self.publicIp,
                                       DWCi_GetMatchCnt()->self.publicPort, &data, 1);
        DWCi_GetMatchCnt()->qr2IsReserved = 0;
        DWCi_GetMatchCnt()->qr2Reservation = 0;
        DWCi_ClearTargetNodeInfo();
    }
    return result;
}

static void DWCi_RestartFromCancel(int a0) {
    if (a0 == 0) {
        DWCi_FinishCancelMatching();
        return;
    }
    DWCi_ResetMatchParam(a0);
    if (DWCi_GetMatchCnt()->qr2MatchType == 2 || DWCi_GetMatchCnt()->qr2MatchType == 3) {
        MatchNotify(0, 1, DWCi_GetMatchCnt()->cbEventPid == 0, 0);
        return;
    }
    if (DWCi_GetMatchCnt()->qr2MatchType == 0) {
        if (DWCi_GetMatchCnt()->matched != 0) {
            MatchNotify(0, 1, DWCi_GetMatchCnt()->cbEventPid == 0, 0);
        }
        if (a0 == 1) {
            DWCi_SetHostState(0);
            DWCi_SetMatchStatus(0x16);
            if (DWCi_HandleSBError(DWCi_SBUpdateAsync(DWCi_GetMatchCnt()->profileID)) != 0) {
                return;
            }
        }
    } else if (DWCi_GetMatchCnt()->qr2MatchType == 1) {
        if (DWCi_GetMatchCnt()->matched != 0) {
            MatchNotify(0, 1, DWCi_GetMatchCnt()->cbEventPid == 0, 0);
        }
        if (a0 == 1) {
            DWCi_SetHostState(0);
            DWCi_GetMatchCnt()->unk_798 = 0;
            DWCi_SetMatchStatus(0x16);
            if (DWCi_HandleSBError(DWCi_SBUpdateAsync(DWCi_GetMatchCnt()->profileID)) != 0) {
                return;
            }
        }
    }
}

static void DWCi_RestartFromTimeout(void) {
    if (DWCi_GetMatchCnt()->qr2MatchType != 2 && DWCi_GetMatchCnt()->qr2MatchType != 3) {
        DWCi_CloseAllConnectionsByTimeout();
        DWCi_RestartFromCancel(1);
    }
}

static int DWCi_ResumeMatching(void) {
    DWCi_GetMatchCnt()->qr2IsReserved = 0;
    DWCi_GetMatchCnt()->qr2Reservation = 0;
    DWCi_GetMatchCnt()->closeState = 0;
    DWCi_RestartFromCancel(1);
    return 0;
}

static int DWCi_CloseCancelHostAsync(int profileId) {
    GT2Connection *conn;
    int result;

    DWCi_GetMatchCnt()->cbEventPid = profileId;
    conn = DWCi_GetGT2ConnectionByProfileID(profileId, 0);
    if (conn != NULL) {
        DWCi_GetMatchCnt()->closeState = 1;
        result = 1;
        gt2CloseConnectionHard(*conn);
        DWCi_GetMatchCnt()->closeState = 0;
        DWCi_DeleteNodeInfoByProfileID(profileId);
    } else {
        DWCi_DeleteNodeInfoByProfileID(profileId);
        result = 0;
    }
    return result;
}

static void DWCi_CloseAllConnectionsByTimeout(void) {
    DWCi_GetMatchCnt()->closeState = 1;
    gt2CloseAllConnectionsHard(DWCi_GetMatchCnt()->fm->gt2Socket);
    DWCi_GetMatchCnt()->closeState = 0;
}

static int DWCi_CloseShutdownClientSC(u32 mask) {
    int outIds[32];
    int inIds[32];
    int addSelf = 0;
    int i;
    int outCount = 0;
    int inCount = 0;
    int result = 1;

    for (i = 1; i < DWCi_GetNumNodeInfo(); i++) {
        DWCNodeInfo *node = DWCi_GetNodeInfoByIndex(i);

        if (node != NULL) {
            if (((1 << node->aid) & mask) != 0) {
                inIds[inCount] = node->profileId;
                inCount++;
            } else {
                outIds[outCount] = node->profileId;
                outCount++;
            }
        }
    }
    if (DWCi_IsTargetNodeInfoValid() != 0) {
        DWCNodeInfo *self = &DWCi_GetMatchCnt()->self;

        if ((mask & (1 << self->aid)) != 0) {
            inIds[inCount] = self->profileId;
            inCount++;
        } else {
            outIds[outCount] = self->profileId;
            outCount++;
            addSelf = 1;
        }
    }
    for (i = 0; i < inCount; i++) {
        int profileId = inIds[i];
        DWCNodeInfo *node = DWCi_GetNodeInfoByProfileID(profileId);

        if (DWCi_IsTargetNodeInfoValid() != 0 && node == NULL && profileId == DWCi_GetMatchCnt()->self.profileId) {
            node = &DWCi_GetMatchCnt()->self;
        }
        if (node != NULL) {
            if (DWCi_HandleMatchCommandError(
                    DWCi_SendMatchCommand(0x10, profileId, node->publicIp, node->publicPort, outIds, outCount)) != 0) {
                result = 0;
            }
        }
    }
    DWCi_GetMatchCnt()->closeState = 1;
    if (addSelf != 0) {
        DWCi_CloseConnectionHard(DWCi_GetMatchCnt()->self.aid);
        DWCi_ClearTargetNodeInfo();
    }
    for (i = 0; i < outCount; i++) {
        u8 aid = DWCi_GetAIDFromProfileID(outIds[i], 0);

        if (aid != 0xff) {
            DWCi_CloseConnectionHard(aid);
        }
    }
    DWCi_GetMatchCnt()->closeState = 0;
    return result;
}

static void DWCi_SendMatchSynPacket(u8 aid, u16 type) {
    int size = 4;
    u32 local[2];
    u32 *data = local;
    u64 now;

    switch (type) {
    case 2:
        if (DWCi_IsTargetNodeInfoValid() != 0 && aid == DWCi_GetMatchCnt()->self.aid) {
            data[0] = 1;
        } else {
            DWCNodeInfo *self;

            size = 0x1c;
            data = DWC_Alloc(4, 0x1c);
            if (data == NULL) {
                return;
            }
            data[0] = 0;
            self = &DWCi_GetMatchCnt()->self;
            data[1] = ((self->hasPrivateAddress << 16) & 0xffff0000) | self->aid;
            data[2] = self->profileId;
            data[3] = self->publicIp;
            data[4] = self->localIp;
            data[5] = ((self->publicPort << 16) & 0xffff0000) | self->localPort;
            data[6] = self->userData;
        }
        break;
    case 3:
        ((u8 *)data)[0] = DWCi_GetMatchCnt()->baseLatency;
        ((u8 *)data)[1] = DWCi_GetMatchCnt()->baseLatency >> 8;
        break;
    case 4:
        if (DWCi_IsTargetNodeInfoValid() != 0 && aid == DWCi_GetMatchCnt()->self.aid) {
            int count;

            if (func_ov011_021686a0() != 0) {
                count = 0;
            } else {
                count = DWCi_GetNumNodeInfo() - 1;
            }
            if (count == 0) {
                ((u8 *)data)[0] = 0;
                ((u8 *)data)[1] = 0;
                ((u8 *)data)[2] = 0;
                ((u8 *)data)[3] = 0;
                data[1] = DWCi_GetMatchCnt()->synId;
                size = 8;
            } else {
                u8 n;
                int pos;

                size = count * 0x18 + 8;
                data = DWC_Alloc(4, size);
                if (data == NULL) {
                    return;
                }
                data[0] = count;
                pos = 1;
                for (n = 0; n < count; n++) {
                    DWCNodeInfo *node = DWCi_GetNodeInfoByIndex(n + 1);

                    data[pos++] = ((node->hasPrivateAddress << 16) & 0xffff0000) | node->aid;
                    data[pos++] = node->profileId;
                    data[pos++] = node->publicIp;
                    data[pos++] = node->localIp;
                    data[pos++] = ((node->publicPort << 16) & 0xffff0000) | node->localPort;
                    data[pos++] = node->userData;
                }
                data[pos] = DWCi_GetMatchCnt()->synId;
            }
        } else {
            size = 8;
            ((u8 *)data)[0] = 0;
            ((u8 *)data)[1] = 0;
            ((u8 *)data)[2] = 0;
            ((u8 *)data)[3] = 0;
            data[1] = DWCi_GetMatchCnt()->synId;
        }
        break;
    }
    DWCi_SendReliable(type, aid, data, size);
    if (data != local) {
        DWC_Free(4, data, 0);
    }
    now = OS_GetTick();
    DWCi_GetMatchCnt()->lastSynSent = now;
}

static int DWCi_ProcessMatchSynTimeout(void) {
    u64 ms;

    if (DWCi_GetMatchCnt()->state == 8 || DWCi_GetMatchCnt()->state == 0x13 || DWCi_GetMatchCnt()->state == 0x17 ||
        DWCi_GetMatchCnt()->state == 0x14) {
        ms = OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->lastSynSent);
    } else {
        return 1;
    }
    switch (DWCi_GetMatchCnt()->state) {
    case 8:
        if (ms > DWCi_GetMatchCnt()->unk_670) {
            DWCi_GetNumNodeInfo();
            DWCi_GetMatchCnt()->pending.profileId = 0;
            if (DWCi_CancelPreConnectedClientProcess(DWCi_GetServerNodeInfo()->profileId) == 0) {
                return 0;
            }
        }
        break;
    case 0x13:
        if (ms > 6000) {
            DWCi_GetMatchCnt()->synResendCount++;
            if (DWCi_GetMatchCnt()->synResendCount > 5) {
                u64 now;

                DWCi_GetMatchCnt()->synResendCount = 0;
                now = OS_GetTick();
                DWCi_GetMatchCnt()->lastSynSent = now;
                DWCi_CloseSynTimeoutClients();
            } else {
                int i;

                for (i = 1; i < DWCi_GetNumNodeInfo(); i++) {
                    u8 aid = DWCi_GetNodeInfoByIndex(i)->aid;

                    if ((DWCi_GetMatchCnt()->synAckBit & (1 << aid)) == 0) {
                        DWCi_SendMatchSynPacket(aid, 2);
                    }
                }
                {
                    DWCNodeInfo *self = &DWCi_GetMatchCnt()->self;
                    u8 aid = self->aid;

                    if ((DWCi_GetMatchCnt()->synAckBit & (1 << aid)) == 0) {
                        DWCi_SendMatchSynPacket(aid, 2);
                    }
                }
            }
        }
        break;
    case 0x17:
        if (ms > 3000) {
            DWCi_SendMatchAckToClients();
        }
        break;
    case 0x14:
        if (ms > DWCi_GetMatchCnt()->baseLatency) {
            DWCi_PostProcessConnection(3);
        }
        break;
    }
    return 1;
}

static int DWCi_CloseSynTimeoutClients(void) {
    u64 now;

    if (DWCi_CloseShutdownClientSC(DWCi_GetMatchCnt()->synAckBit) == 0) {
        return 0;
    }
    if (DWCi_GetNumNodeInfo() > 1 || DWCi_IsTargetNodeInfoValid() != 0) {
        now = OS_GetTick();
        DWCi_GetMatchCnt()->lastSynSent = now;
        DWCi_SetMatchStatus(0x17);
    } else {
        if (DWCi_CancelPreConnectedServerProcess(DWCi_GetMatchCnt()->cbEventPid) == 0) {
            return 0;
        }
    }
    return 1;
}

static void DWCi_SendMatchAckToClients(void) {
    int i;

    if (DWCi_GetHostState() == 2) {
        for (i = 1; i < DWCi_GetNumNodeInfo(); i++) {
            DWCi_SendMatchSynPacket(DWCi_GetNodeInfoByIndex(i)->aid, 4);
        }
        if (DWCi_IsTargetNodeInfoValid() != 0) {
            DWCi_SendMatchSynPacket(DWCi_GetMatchCnt()->self.aid, 4);
        }
        DWCi_SetMatchStatus(0x14);
    }
}

static int DWCi_SendCancelMatchSynCommand(int profileId, u8 message) {
    u32 data;
    DWCNodeInfo *node;
    u64 now;
    int count;

    if (message == 0xd) {
        data = DWCi_GetMatchCnt()->cbEventPid;
        count = 1;
    } else {
        count = 0;
    }
    node = DWCi_GetNodeInfoByProfileID(profileId);
    if (node == NULL) {
        return 0;
    }
    if (DWCi_HandleMatchCommandError(
            DWCi_SendMatchCommand(message, profileId, node->publicIp, node->publicPort, &data, count)) != 0) {
        return 0;
    }
    now = OS_GetTick();
    DWCi_GetMatchCnt()->lastCancelSynSent = now;
    return 1;
}

static int DWCi_ProcessCancelMatchSynCommand(int profileId, int message, int value) {
    u64 ms;
    u64 now;
    int i;
    int aid;

    if (DWC_GetState() != 5) {
        return 1;
    }
    switch (message) {
    case 0xd:
        if (DWCi_GetMatchCnt()->state != 7) {
            DWCi_SetMatchStatus(7);
            DWCi_GetMatchCnt()->cbEventPid = value;
            if (DWCi_GetMatchCnt()->pending.profileId != 0) {
                DWCi_GetMatchCnt()->pending.profileId = 0;
            }
            DWCi_ClearTargetNodeInfo();
        }
        if (DWCi_SendCancelMatchSynCommand(profileId, 0xe) == 0) {
            return 0;
        }
        break;
    case 0xe:
        if (DWCi_GetMatchCnt()->state == 0x11) {
            now = OS_GetTick();
            ms = OS_TicksToMilliSeconds(now - DWCi_GetMatchCnt()->lastCancelSynSent) / 2;
            if (ms > 300) {
                ms = OS_TicksToMilliSeconds(now - DWCi_GetMatchCnt()->lastCancelSynSent) / 2 - 300;
                if (DWCi_GetMatchCnt()->cancelBaseLatency < ms) {
                    DWCi_GetMatchCnt()->cancelBaseLatency = ms;
                }
            }
            aid = DWCi_GetAIDFromProfileID(profileId, 0);
            if (aid != 0xff) {
                DWCi_GetMatchCnt()->cancelSynAckBit |= 1 << aid;
            }
            if (DWCi_GetAIDBitmask(1) == DWCi_GetMatchCnt()->cancelSynAckBit) {
                for (i = 1; i < DWCi_GetNumNodeInfo(); i++) {
                    if (DWCi_SendCancelMatchSynCommand(DWCi_GetNodeInfoByIndex(i)->profileId, 0xf) == 0) {
                        return 0;
                    }
                }
                DWCi_SetMatchStatus(0x12);
            }
        } else {
            if (DWCi_SendCancelMatchSynCommand(profileId, 0xf) == 0) {
                return 0;
            }
        }
        break;
    case 0xf:
        if (DWCi_GetMatchCnt()->state == 7) {
            DWCi_RestartFromCancel(2);
        }
        break;
    }
    return 1;
}

static int DWCi_ProcessCancelMatchSynTimeout(void) {
    u64 ms;
    u64 now;
    int i;

    if (DWCi_GetMatchCnt()->state == 7 || DWCi_GetMatchCnt()->state == 0x11 || DWCi_GetMatchCnt()->state == 0x12) {
        ms = OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->lastCancelSynSent);
    } else {
        return 1;
    }
    switch (DWCi_GetMatchCnt()->state) {
    case 7:
        if (ms > 6000) {
            if (DWCi_SendCancelMatchSynCommand(DWCi_GetNodeInfoByIndex(0)->profileId, 0xe) == 0) {
                return 0;
            }
        }
        break;
    case 0x11:
        if (ms > 6000) {
            DWCi_GetMatchCnt()->cancelSynResendCount++;
            if (DWCi_GetMatchCnt()->cancelSynResendCount > 5) {
                if (DWCi_CloseShutdownClientSC(DWCi_GetMatchCnt()->cancelSynAckBit) == 0) {
                    return 0;
                }
                if (DWCi_GetNumNodeInfo() > 1) {
                    DWCi_GetMatchCnt()->cancelSynResendCount = 0;
                    now = OS_GetTick();
                    DWCi_GetMatchCnt()->lastCancelSynSent = now;
                } else {
                    DWCi_RestartFromCancel(2);
                }
            } else {
                for (i = 1; i < DWCi_GetNumNodeInfo(); i++) {
                    DWCNodeInfo *node = DWCi_GetNodeInfoByIndex(i);

                    if ((DWCi_GetMatchCnt()->cancelSynAckBit & (1 << node->aid)) == 0 &&
                        DWCi_SendCancelMatchSynCommand(node->profileId, 0xd) == 0) {
                        return 0;
                    }
                }
            }
        }
        break;
    case 0x12:
        if (ms > DWCi_GetMatchCnt()->cancelBaseLatency) {
            DWCi_RestartFromCancel(2);
        }
        break;
    }
    return 1;
}

static u8 DWCi_GetNewMatchKey(void) {
    u8 i = 0;

    do {
        if (sKeys[i].id == 0) {
            return i + 100;
        }
        i++;
    } while (i < 0x9a);
    return 0;
}

static void DWCi_ClearGameMatchKeys(void) {
    int i;

    for (i = 0; i < 0x9a; i++) {
        if (sKeys[i].name != NULL) {
            DWC_Free(4, sKeys[i].name, 0);
        }
    }
    MIi_CpuClear32(0, sKeys, sizeof(sKeys));
}

static BOOL DWCi_IsFriendByIdxList(int profileId) {
    int i;

    if (DWCi_GetMatchCnt()->friendList == 0) {
        return FALSE;
    }
    for (i = 0; i < DWCi_GetMatchCnt()->friendIdxListLen; i++) {
        int friendIndex = DWCi_GetProfileIDFromList(DWCi_GetMatchCnt()->friendIdxList[i]);

        if (friendIndex > 0 && friendIndex == profileId) {
            return TRUE;
        }
    }
    return FALSE;
}

static int DWCi_CheckDWCServer(SBServer server) {
    if (SBServerGetIntValueA(server, "numplayers", -1) == -1) {
        return 0;
    }
    if (SBServerGetIntValueA(server, "maxplayers", -1) == -1) {
        return 0;
    }
    if (SBServerGetIntValueA(server, "dwc_mtype", -1) == -1) {
        return 0;
    }
    if (SBServerGetIntValueA(server, "dwc_mver", -1) == -1) {
        return 0;
    }
    return SBServerGetIntValueA(server, "dwc_pid", 0);
}

static u8 DWCi_GetAIDFromProfileID(int index, int a1) {
    DWCNodeInfo *peer = DWCi_GetNodeInfoByProfileID(index);

    if (peer == NULL) {
        return 0xff;
    }
    return peer->aid;
}

static u32 DWCi_GetAIDBitmask(int excludeSelf) {
    u32 mask = 0;
    int i;

    for (i = 1; i < DWCi_GetNumNodeInfo(); i++) {
        mask |= 1 << DWCi_GetNodeInfoByIndex(i)->aid;
    }
    if (excludeSelf == 0) {
        mask |= 1 << DWCi_GetMatchCnt()->self.aid;
    }
    return mask;
}

static void DWCi_InitClWaitTimeout(void) {
    u64 now;

    DWCi_GetMatchCnt()->clWaitTimeoutCount = 0;
    now = OS_GetTick();
    DWCi_GetMatchCnt()->unk_670 = now;
}

static int DWCi_HandleMatchGPError(int result) {
    int type = result;
    int code;

    if (result == 0) {
        return 0;
    }
    switch (result) {
    case 1:
        type = 9;
        code = type - 10;
        break;
    case 2:
        type = 9;
        code = type - 11;
        break;
    case 3:
        type = 6;
        code = type - 16;
        break;
    case 4:
        type = 6;
        code = type - 26;
        break;
    }
    DWCi_StopMatching(type, code - 81000);
    return result;
}

static int DWCi_HandleSBError(int result) {
    int type = result;
    int code;

    if (result == 0) {
        return 0;
    }
    switch (result) {
    case 1:
        type = 6;
        code = type - 56;
        break;
    case 2:
        type = 6;
        code = type - 36;
        break;
    case 3:
        type = 6;
        code = type - 26;
        break;
    case 4:
        type = 6;
        code = type - 46;
        break;
    case 5:
        type = 9;
        code = type - 10;
        break;
    case 6:
        type = 9;
        code = type - 11;
        break;
    }
    DWCi_StopMatching(type, code - 85000);
    return result;
}

static int DWCi_HandleQR2Error(int result) {
    int type;
    int code;

    if (result == 0) {
        return 0;
    }
    switch (result) {
    case 1:
        type = 6;
        code = type - 56;
        break;
    case 2:
        type = 6;
        code = type - 66;
        break;
    case 3:
        type = 6;
        code = type - 36;
        break;
    case 4:
        type = 6;
        code = type - 86;
        break;
    case 5:
        type = 6;
        code = type - 26;
        break;
    }
    switch (DWC_GetState()) {
    case 2:
        DWCi_StopLogin(type, code - 64000);
        break;
    case 4:
        DWCi_StopMatching(type, code - 84000);
        break;
    default:
        DWCi_SetError(type, code - 94000);
        break;
    }
    return result;
}

static int DWCi_HandleNNError(int result) {
    int type = result;
    int code;

    if (result == 0) {
        return 0;
    }
    switch (result) {
    case 1:
        type = 9;
        code = type - 10;
        break;
    case 2:
        type = 6;
        code = type - 56;
        break;
    case 3:
        type = 6;
        code = type - 36;
        break;
    }
    DWCi_StopMatching(type, code - 86000);
    return result;
}

static int DWCi_HandleNNResult(int result) {
    int type;
    int code;

    if (result == 0) {
        return 0;
    }
    switch (result) {
    case 1:
        return 1;
    case 2:
        return 2;
    case 3:
        return 3;
    default:
        type = 6;
        code = -86009;
        break;
    }
    if (type != 0) {
        DWCi_StopMatching(type, code);
    }
    return result;
}

static int DWCi_HandleMatchGT2Error(int result) {
    int type = result;
    int code;

    if (result == 0) {
        return 0;
    }
    switch (result) {
    case 1:
        type = 9;
        code = type - 10;
        break;
    case 2:
    case 5:
        type = 0;
        code = 0;
        result = 0;
        break;
    case 3:
        type = 6;
        code = type - 16;
        break;
    case 4:
        type = 6;
        code = type - 36;
        break;
    case 6:
        type = 6;
        code = type - 76;
        break;
    case 7:
        type = 6;
        code = type - 86;
        break;
    }
    if (type != 0) {
        DWCi_StopMatching(type, code - 87000);
    }
    return result;
}

static void DWCi_SBCallback(ServerBrowser sb, SBCallbackReason reason, SBServer server, void *instance) {
    int index;
    int count;
    SBServer chosen;
    SBServer host;
    int best;
    DWCNodeInfo peer;

    sMatchState.sbCallbackLevel++;
    if (DWCi_GetMatchCnt()->sbUpdateRequestTick != 0) {
        if (reason == sbc_updatecomplete || reason == sbc_queryerror) {
            DWCi_GetMatchCnt()->sbUpdateRequestTick = 0;
        } else {
            u64 now = OS_GetTick();

            DWCi_GetMatchCnt()->sbUpdateRequestTick = now + OS_MilliSecondsToTicks(30000);
        }
    }
    switch (reason) {
    case 0:
        DWCi_SBPrintServerData(server);
        break;
    case 4:
        DWCi_GetMatchCnt()->sbUpdateRequestTick = 0;
        for (index = 0; index < ServerBrowserCount(sb); index++) {
            server = ServerBrowserGetServer(sb, index);
            if (DWCi_CheckDWCServer(server) == 0) {
                ServerBrowserRemoveServer(sb, server);
                index--;
            }
        }
        switch (DWCi_GetMatchCnt()->state) {
        case 0x16:
            for (index = 0; index < ServerBrowserCount(sb); index++) {
                server = ServerBrowserGetServer(sb, index);
                if (DWCi_GetMatchCnt()->qr2IP != 0 &&
                    DWCi_GetMatchCnt()->qr2IP == SBServerGetPublicInetAddress(server) &&
                    DWCi_GetMatchCnt()->qr2Port != 0 &&
                    DWCi_GetMatchCnt()->qr2Port == SBServerGetPublicQueryPort(server)) {
                    break;
                }
            }
            if (index < ServerBrowserCount(sb)) {
                MI_CpuFill8(&peer, 0, 0x28);
                peer.profileId = DWCi_GetMatchCnt()->profileID;
                peer.aid = 0xff;
                peer.publicIp = DWCi_GetMatchCnt()->qr2IP;
                peer.publicPort = DWCi_GetMatchCnt()->qr2Port;
                peer.localIp = SBServerGetPrivateInetAddress(server);
                peer.localPort = SBServerGetPrivateQueryPort(server);
                peer.hasPrivateAddress = SBServerHasPrivateAddress(server);
                MI_CpuCopy8(&DWCi_GetMatchCnt()->userData, &peer.userData, sizeof(u32));
                if (DWCi_GetMatchCnt()->qr2MatchType == 0) {
                    DWCi_PushFrontNodeInfo(&peer);
                    DWCi_StartMatchingAfterSearchOwn();
                } else if (DWCi_GetMatchCnt()->qr2MatchType == 1) {
                    DWCi_PushFrontNodeInfo(&peer);
                    DWCi_StartMatchingAfterSearchOwn();
                } else if (DWCi_GetMatchCnt()->qr2MatchType == 2) {
                    peer.aid = DWCi_GetFreeAID();
                    DWCi_PushFrontNodeInfo(&peer);
                    DWCi_StartMatchingAfterSearchOwn();
                } else if (DWCi_GetMatchCnt()->qr2MatchType == 3) {
                    DWCi_PushFrontNodeInfo(&peer);
                    DWCi_StartMatchingAfterSearchOwn();
                }
                break;
            }
            DWCi_GetMatchCnt()->sbUpdateFlag = 2;
            {
                u64 now = OS_GetTick();

                DWCi_GetMatchCnt()->sbUpdateTick = now;
            }
            break;
        case 2:
            DWCi_EvaluateServers(1);
            DWCi_RandomizeServers();
            if (ServerBrowserCount(sb) != 0) {
                host = ServerBrowserGetServer(sb, 0);
                DWCi_ClearTargetNodeInfo();
                DWCi_GetMatchCnt()->self.profileId = SBServerGetIntValueA(host, "dwc_pid", 0);
                DWCi_GetMatchCnt()->self.aid = 0xff;
                DWCi_GetMatchCnt()->self.publicIp = SBServerGetPublicInetAddress(host);
                DWCi_GetMatchCnt()->self.publicPort = SBServerGetPublicQueryPort(host);
                DWCi_GetMatchCnt()->self.localIp = SBServerGetPrivateInetAddress(host);
                DWCi_GetMatchCnt()->self.localPort = SBServerGetPrivateQueryPort(host);
                DWCi_GetMatchCnt()->self.hasPrivateAddress = SBServerHasPrivateAddress(host);
                DWCi_StartReservingServer(DWCi_GetMatchCnt()->self.profileId);
            } else {
                DWCi_GetMatchCnt()->sbUpdateFlag = 2;
                {
                    u64 now = OS_GetTick();

                    DWCi_GetMatchCnt()->sbUpdateTick = now;
                }
                DWCi_ClearTargetNodeInfo();
            }
            break;
        case 4:
            while (ServerBrowserCount(sb) != 0) {
                server = ServerBrowserGetServer(sb, 0);
                if (SBServerGetPublicInetAddress(server) == DWCi_GetMatchCnt()->self.publicIp) {
                    if (SBServerGetPublicQueryPort(server) == DWCi_GetMatchCnt()->self.publicPort) {
                        break;
                    }
                }
                ServerBrowserRemoveServer(sb, server);
            }
            if (ServerBrowserCount(sb) != 0) {
                int profileId = SBServerGetIntValueA(ServerBrowserGetServer(sb, 0), "dwc_pid", 0);

                if (DWCi_GetMatchCnt()->qr2MatchType == 1) {
                    DWCi_GetMatchCnt();
                    if (profileId != DWCi_GetMatchCnt()->reqProfileID || DWCi_EvaluateServers(0) == 0) {
                        if (DWCi_HandleMatchCommandError(DWCi_CancelReservation(DWCi_GetMatchCnt()->reqProfileID)) ==
                            0) {
                            DWCi_SetMatchStatus(3);
                            DWCi_ClearTargetNodeInfo();
                            DWCi_HandleMatchCommandError(
                                DWCi_SendResvCommandToFriend(0, 0, DWCi_GetMatchCnt()->reqProfileID));
                        }
                        break;
                    }
                }
                DWCi_SetMatchStatus(5);
                ServerBrowserGetServer(sb, 0);
                DWCi_HandleNNError(DWCi_NNStartupAsync(0, 0, &DWCi_GetMatchCnt()->self));
            } else {
                DWCi_GetMatchCnt()->sbUpdateFlag = 2;
                {
                    u64 now = OS_GetTick();

                    DWCi_GetMatchCnt()->sbUpdateTick = now;
                }
            }
            break;
        case 0xc:
            count = ServerBrowserCount(sb);
            if (count != 0) {
                if (count > 1) {
                    best = -1;
                    for (index = 0; index < count; index++) {
                        host = ServerBrowserGetServer(sb, index);
                        int players = SBServerGetIntValueA(host, "numplayers", -1);

                        if (best < players) {
                            chosen = host;
                            best = players;
                        }
                    }
                } else {
                    chosen = ServerBrowserGetServer(sb, 0);
                }
                DWCi_ClearTargetNodeInfo();
                DWCi_GetMatchCnt()->self.profileId = SBServerGetIntValueA(chosen, "dwc_pid", 0);
                DWCi_GetMatchCnt()->self.aid = 0xff;
                DWCi_GetMatchCnt()->self.publicIp = SBServerGetPublicInetAddress(chosen);
                DWCi_GetMatchCnt()->self.publicPort = SBServerGetPublicQueryPort(chosen);
                DWCi_GetMatchCnt()->self.localIp = SBServerGetPrivateInetAddress(chosen);
                DWCi_GetMatchCnt()->self.localPort = SBServerGetPrivateQueryPort(chosen);
                DWCi_GetMatchCnt()->self.hasPrivateAddress = SBServerHasPrivateAddress(chosen);
                DWCi_GetMatchCnt()->qr2MatchType = SBServerGetIntValueA(chosen, "dwc_mtype", -1);
                DWCi_ConnectToGameServerAsync(DWCi_GetTopologyType(), SBServerGetIntValueA(chosen, "dwc_pid", 0),
                                              DWCi_GetMatchCnt()->notifyFunc, DWCi_GetMatchCnt()->notifyParam,
                                              DWCi_GetMatchCnt()->newClientCallback, DWCi_GetMatchCnt()->newClientParam,
                                              (DWCAttemptFunc)DWCi_GetMatchCnt()->checkFunc,
                                              &DWCi_GetMatchCnt()->userData, (void *)DWCi_GetMatchCnt()->checkParam);
            } else {
                DWCi_GetMatchCnt()->sbUpdateFlag = 2;
                {
                    u64 now = OS_GetTick();

                    DWCi_GetMatchCnt()->sbUpdateTick = now;
                }
            }
            break;
        }
        break;
    case 5:
        break;
    }
    sMatchState.sbCallbackLevel--;
}

static void DWCi_SBPrintServerData(SBServer server) {
}

// How often each of the first servers is picked
static const int sMatchWeights[6] = { 3, 3, 2, 2, 1, 1 };

static BOOL DWCi_EvaluateServers(int sortAfter) {
    BOOL removed = FALSE;
    int i;

    for (i = 0; i < ServerBrowserCount(DWCi_GetMatchCnt()->sbObj); i++) {
        void *server = ServerBrowserGetServer(DWCi_GetMatchCnt()->sbObj, i);

        if (DWCi_GetMatchCnt()->qr2MatchType == 0) {
            int profileId = SBServerGetIntValueA(server, "dwc_pid", 0);
            BOOL skip = FALSE;

            if (DWCi_GetNodeInfoByProfileID(profileId) != 0) {
                ServerBrowserRemoveServer(DWCi_GetMatchCnt()->sbObj, server);
                i--;
                skip = TRUE;
            }
            if (skip) {
                continue;
            }
        }
        if (DWCi_GetMatchCnt()->evalCallback != 0) {
            int rank = DWCi_GetMatchCnt()->evalCallback(i, DWCi_GetMatchCnt()->evalParam);

            if (rank > 0) {
                if (rank > 0x7fffff) {
                    rank = 0x7fffff;
                }
                SBServerAddIntKeyValue(server, "dwc_eval", DWCi_GetMathRand32(0x100) | (rank << 8));
            } else {
                ServerBrowserRemoveServer(DWCi_GetMatchCnt()->sbObj, server);
                i--;
                removed = TRUE;
                continue;
            }
        } else {
            SBServerAddIntKeyValue(server, "dwc_eval", DWCi_GetMathRand32(0x80));
        }
    }
    if (sortAfter != 0 && ServerBrowserCount(DWCi_GetMatchCnt()->sbObj) != 0) {
        ServerBrowserSortA(DWCi_GetMatchCnt()->sbObj, 0, "dwc_eval", 0);
    }
    if (removed && ServerBrowserCount(DWCi_GetMatchCnt()->sbObj) == 0) {
        return FALSE;
    }
    return TRUE;
}

static void DWCi_RandomizeServers(void) {
    int best = 0;
    int total = 0;
    int i;
    u32 pick;
    u32 cumulative[6];

    if (ServerBrowserCount(DWCi_GetMatchCnt()->sbObj) > 1) {
        for (i = 0; i < ServerBrowserCount(DWCi_GetMatchCnt()->sbObj); i++) {
            int eval = SBServerGetIntValueA(ServerBrowserGetServer(DWCi_GetMatchCnt()->sbObj, i), "dwc_eval", -1);

            if (eval > best) {
                best = eval;
            }
            total += sMatchWeights[i];
        }
        pick = DWCi_GetMathRand32(100);
        for (i = 0; i < ServerBrowserCount(DWCi_GetMatchCnt()->sbObj); i++) {
            int previous;

            if (i == ServerBrowserCount(DWCi_GetMatchCnt()->sbObj) - 1) {
                break;
            }
            if (i > 0) {
                previous = cumulative[i - 1];
            } else {
                previous = 0;
            }
            cumulative[i] = sMatchWeights[i] * 100 / total;
            cumulative[i] = previous + cumulative[i];
            if (pick < cumulative[i]) {
                break;
            }
        }
        if (best < 0x7fffffff) {
            best++;
        }
        SBServerAddIntKeyValue(ServerBrowserGetServer(DWCi_GetMatchCnt()->sbObj, i), "dwc_eval", best);
        ServerBrowserSortA(DWCi_GetMatchCnt()->sbObj, 0, "dwc_eval", 0);
    }
}

static void DWCi_QR2ServerKeyCallback(int keyId, qr2_buffer_t outBuf, void *userData) {
    int count;

    switch (keyId) {
    case 8:
        count = DWCi_GetNumNodeInfo() - 1;
        if (count < 0) {
            count = 0;
        }
        qr2_buffer_add_int(outBuf, count);
        return;
    case 10:
        qr2_buffer_add_int(outBuf, DWCi_GetMatchCnt()->qr2NumEntry);
        return;
    case 0x32:
        qr2_buffer_add_int(outBuf, DWCi_GetMatchCnt()->profileID);
        return;
    case 0x33:
        qr2_buffer_add_int(outBuf, DWCi_GetMatchCnt()->qr2MatchType);
        return;
    case 0x34:
        qr2_buffer_add_int(outBuf, 0x5a);
        return;
    case 0x35:
        qr2_buffer_add_int(outBuf, 1);
        return;
    case 0x36:
        qr2_buffer_add_int(outBuf, DWCi_GetHostState() == 2 ? DWCi_GetMatchCnt()->groupId : 0);
        return;
    case 0x37:
        qr2_buffer_add_int(outBuf, DWCi_GetMatchCnt()->hostState);
        return;
    case 0x38:
        qr2_buffer_add_int(outBuf, DWCi_GetMatchCnt()->suspend);
        return;
    }
    keyId -= 100;
    if (keyId >= 0 && keyId < 154 && sKeys[keyId].id != 0) {
        if (sKeys[keyId].isText != 0) {
            qr2_buffer_addA(outBuf, (const char *)sKeys[keyId].value);
        } else {
            qr2_buffer_add_int(outBuf, *(int *)sKeys[keyId].value);
        }
    }
}

static void DWCi_QR2PlayerKeyCallback(int keyId, int index, qr2_buffer_t outBuf, void *userData) {
}

static void DWCi_QR2TeamKeyCallback(int keyId, int index, qr2_buffer_t outBuf, void *userData) {
}

static void DWCi_QR2KeyListCallback(qr2_key_type keyType, qr2_keybuffer_t keyBuffer, void *userData) {
    int i;

    switch (keyType) {
    case key_server:
        qr2_keybuffer_add(keyBuffer, 8);
        qr2_keybuffer_add(keyBuffer, 10);
        qr2_keybuffer_add(keyBuffer, 0x32);
        qr2_keybuffer_add(keyBuffer, 0x33);
        qr2_keybuffer_add(keyBuffer, 0x34);
        qr2_keybuffer_add(keyBuffer, 0x35);
        qr2_keybuffer_add(keyBuffer, 0x36);
        qr2_keybuffer_add(keyBuffer, 0x37);
        qr2_keybuffer_add(keyBuffer, 0x38);
        for (i = 0; i < 154; i++) {
            if (sKeys[i].id != 0) {
                qr2_keybuffer_add(keyBuffer, sKeys[i].id);
            }
        }
        break;
    case key_player:
        break;
    case key_team:
        break;
    }
}

static int DWCi_QR2CountCallback(qr2_key_type keyType, void *userData) {
    return 0;
}

static void DWCi_QR2AddErrorCallback(qr2_error_t error, gsi_char *errorMessage, void *userData) {
    DWCi_HandleQR2Error(error);
}

static void DWCi_QR2PublicAddrCallback(gsi_u32 ip, gsi_u16 port, void *userData) {
    DWCi_GetMatchCnt()->qr2IP = ip;
    DWCi_GetMatchCnt()->qr2Port = port;
}

static void DWCi_QR2NatnegCallback(int cookie, void *userData) {
    if (DWC_IsServerMyself() == 0 && DWCi_GetMatchCnt()->pending.profileId != 0) {
        return;
    }
    if (DWCi_GetMatchCnt()->state == 1) {
        if (func_ov011_021686a0() == 0) {
            return;
        }
        DWCi_SetMatchStatus(5);
    } else if (DWCi_GetMatchCnt()->state != 5 && DWCi_GetMatchCnt()->state != 0xe) {
        return;
    }
    if (cookie == DWCi_GetMatchCnt()->nnLastCookie) {
        DWCi_GetMatchCnt()->nnRecvCount++;
    } else {
        DWCi_GetMatchCnt()->nnRecvCount = 0;
        DWCi_GetMatchCnt()->nnLastCookie = cookie;
    }
    DWCi_GetMatchCnt()->nnFailedTime = 0;
    if (DWCi_HandleNNError(DWCi_NNStartupAsync(1, cookie, &DWCi_GetMatchCnt()->self)) == 0) {
        DWCi_GetMatchCnt()->cmdCommand = 0xff;
    }
}

void DWCi_ProcessGT2MatchCommand(const u8 *data, int size) {
    MatchPacketHeader header;
    u32 payload[0x20];

    if (DWC_GetState() != 4) {
        if (DWC_GetState() != 5) {
            return;
        }
        if (DWCi_GetMatchCnt()->mode != 2 && DWCi_GetMatchCnt()->mode != 3) {
            return;
        }
    }
    MI_CpuCopy8(data, &header, sizeof(MatchPacketHeader));
    if (header.version == 0x5a && size == header.size + sizeof(MatchPacketHeader)) {
        MI_CpuCopy8(data + sizeof(MatchPacketHeader), payload, header.size);
        DWCi_ProcessRecvMatchCommand(header.command, header.profileId, header.ip, header.port, payload,
                                     header.size >> 2);
    }
}

static void DWCi_QR2ClientMsgCallback(gsi_char *data, int size, void *userData) {
    MatchPacketHeader header;
    u32 payload[0x20];
    int offset = 0;

    if (DWC_GetState() != 4) {
        if (DWC_GetState() != 5) {
            return;
        }
        if (DWCi_GetMatchCnt()->mode != 2 && DWCi_GetMatchCnt()->mode != 3) {
            return;
        }
    }
    if (size < 0x14) {
        return;
    }
    do {
        MI_CpuCopy8(data, &header, 0x14);
        if (strncmp(header.magic, "\xbbI\xccM", 4) != 0 || header.version != 0x5a) {
            return;
        }
        MI_CpuCopy8(data + 0x14, payload, header.size);
        if (DWCi_ProcessRecvMatchCommand(header.command, header.profileId, header.ip, header.port, payload,
                                         header.size >> 2) == 0) {
            return;
        }
        offset += header.size + 0x14;
    } while (offset + 0x14 <= size);
}

static void DWCi_NNProgressCallback(NegotiateState state, void *userData) {
}

static void DWCi_NNCompletedCallback(NegotiateResult result, SOCKET gameSocket, SOCKADDR_IN *address, void *userData) {
    MatchTarget *connection = userData;
    u8 buffer[0x18];
    int status;
    int mapped;

    if ((DWCi_GetMatchCnt()->state == 5 || DWCi_GetMatchCnt()->state == 0xe) && connection != NULL) {
        if (result == 0) {
            if (connection->kind != 0) {
                DWCi_GetMatchCnt()->self.hostIp = address->addr.addr;
                DWCi_GetMatchCnt()->self.hostPort = MATCH_SWAP16(address->port);
                DWCi_GetMatchCnt()->nnRecvCount = 0;
                DWCi_GetMatchCnt()->nnLastCookie = 0;
                DWCi_GetMatchCnt()->nnFailedTime = 0;
                DWCi_SetMatchStatus(DWCi_GetMatchCnt()->state == 0xe ? 0xf : 6);
                DWCi_GetMatchCnt()->retries = 0;
                DWCi_FormatProfileID(buffer, 0x18);
                status = func_ov011_0217d1a0(
                    DWCi_GetMatchCnt()->fm->gt2Socket, 0,
                    func_ov011_0217ed14(DWCi_GetMatchCnt()->self.hostIp, DWCi_GetMatchCnt()->self.hostPort, 0), buffer,
                    -1, 5000, DWCi_GetMatchCnt()->gt2Callbacks, 0);
                if (DWCi_GetMatchCnt()->memberConnecting != 0 && status != 0) {
                    DWCi_GetNodeInfoByProfileID(DWCi_GetMatchCnt()->self.profileId)->connectFailureCount++;
                    func_ov011_02168468();
                    DWCi_SetMatchStatus(1);
                    return;
                }
                if (status == 1) {
                    DWCi_HandleMatchGT2Error(status);
                    return;
                }
                if (status == 0) {
                    return;
                }
                if (DWCi_CancelPreConnectedServerProcess(DWCi_GetMatchCnt()->self.profileId) != 0) {
                    return;
                }
            } else {
                if (address != NULL) {
                    DWCi_GetMatchCnt()->self.hostIp = address->addr.addr;
                    DWCi_GetMatchCnt()->self.hostPort = MATCH_SWAP16(address->port);
                }
                {
                    u64 now = OS_GetTick();

                    DWCi_GetMatchCnt()->nnFinishTime = now;
                }
                DWCi_SetMatchStatus(6);
            }
        } else if (connection->cookie != 0) {
            if (DWCi_GetMatchCnt()->memberConnecting != 0) {
                DWCi_GetNodeInfoByProfileID(DWCi_GetMatchCnt()->self.profileId)->connectFailureCount++;
                func_ov011_02168468();
                DWCi_SetMatchStatus(1);
                return;
            }
            mapped = DWCi_HandleNNResult(result);
            if (mapped != 2 && mapped != 1 && mapped != 3) {
                return;
            }
            {
                if (connection->kind == 0) {
                    if (mapped == 1 || mapped == 3 || (mapped == 2 && connection->retryCount >= 1)) {
                        NNCancel(DWCi_GetMatchCnt()->target.cookie);
                        connection->cookie = 0;
                        if (DWCi_ProcessNNFailure(0) != 0) {
                            if (DWCi_CancelPreConnectedClientProcess(DWCi_GetMatchCnt()->self.profileId) != 0) {
                                return;
                            }
                        }
                    } else {
                        connection->retryCount++;
                        if (DWCi_HandleNNError(DWCi_DoNatNegotiationAsync(connection)) != 0) {
                            return;
                        }
                    }
                } else {
                    u64 now = OS_GetTick();

                    DWCi_GetMatchCnt()->nnFailedTime = now;
                    if (mapped == 1 || mapped == 3 || (mapped == 2 && DWCi_GetMatchCnt()->nnRecvCount >= 1)) {
                        NNCancel(DWCi_GetMatchCnt()->target.cookie);
                        connection->cookie = 0;
                        if (DWCi_GetMatchCnt()->qr2MatchType == 3 || DWCi_GetMatchCnt()->qr2MatchType == 2) {
                            if (DWCi_ProcessNNFailure(1) == 0) {
                                return;
                            }
                        } else if (DWCi_ProcessNNFailure(0) == 0) {
                            return;
                        }
                        DWCi_GetMatchCnt()->nnRecvCount = 0;
                        DWCi_GetMatchCnt()->nnLastCookie = 0;
                        DWCi_GetMatchCnt()->nnFailedTime = 0;
                        if (DWCi_CancelPreConnectedServerProcess(DWCi_GetMatchCnt()->self.profileId) != 0) {
                            return;
                        }
                    }
                }
            }
        }
    }
}

static BOOL DWCi_ProcessNNFailure(int skip) {
    if (skip != 0) {
        return TRUE;
    }
    if (DWCi_GetMatchCnt()->qr2MatchType != 3) {
        DWCi_GetMatchCnt()->nnFailureCount++;
    }
    if (DWCi_GetMatchCnt()->qr2MatchType == 3 || DWCi_GetMatchCnt()->unk_798 != 0 ||
        DWCi_GetMatchCnt()->nnFailureCount >= 5) {
        DWCi_StopMatching(6, -86420);
        return FALSE;
    }
    return TRUE;
}

static BOOL DWCi_SendSvDownCommands(void) {
    int count;
    int i;

    DWCi_GetMatchCnt();
    count = DWCi_GetNumNodeInfo();
    for (i = 1; i < count; i++) {
        DWCNodeInfo *peer = DWCi_GetNodeInfoByIndex(i);
        u32 data = DWC_GetAIDBitmap();
        u8 type;

        if (DWCi_GetMatchCnt()->newHostAckBit & (1 << peer->aid)) {
            type = 0x55;
        } else {
            type = 0x52;
        }
        if (DWCi_HandleMatchCommandError(
                DWCi_SendMatchCommand(type, peer->profileId, peer->publicIp, peer->publicPort, &data, 1)) != 0) {
            return FALSE;
        }
    }
    DWCi_InitClWaitTimeout();
    return TRUE;
}

BOOL DWCi_IsServerNodeMyself(void) {
    BOOL result = FALSE;
    DWCNodeInfo *peer = DWCi_GetNodeInfoByIndex(0);

    if (peer != NULL && peer->profileId == DWCi_GetMatchCnt()->profileID) {
        result = TRUE;
    }
    return result;
}

static u32 DWCi_MakeGroupID(void) {
    DWCMatchControl *match = DWCi_GetMatchCnt();

    return match->profileID + DWCi_GetMathRand32(10000000) + 100000000;
}

static void DWCi_SetReservationCallback(MatchCheckFunc a, const void *userData, u32 c) {
    DWCi_GetMatchCnt()->checkFunc = a;
    if (userData != NULL) {
        memcpy(&DWCi_GetMatchCnt()->userData, userData, sizeof(int));
    } else {
        memset(&DWCi_GetMatchCnt()->userData, 0, sizeof(int));
    }
    DWCi_GetMatchCnt()->checkParam = c;
}

static void DWCi_ChangeToServer(int isHost) {
    int mode;

    if (isHost != 0) {
        DWCi_GetMatchCnt()->groupId = DWCi_MakeGroupID();
        DWCi_GetOwnNodeInfo()->aid = 0;
        DWCi_GetMatchCnt()->unlocked = 1;
    }
    DWCi_GetMatchCnt()->mode = 2;
    DWCi_SetMatchStatus(0xd);
    DWCi_SetHostState(2);
    DWCi_GetMatchCnt()->qr2Reservation = 0;
    DWCi_GetMatchCnt()->qr2IsReserved = 0;
    if (DWCi_GetMatchCnt()->qr2MatchType == 0) {
        mode = 3;
    } else {
        DWCMatchControl *match = DWCi_GetMatchCnt();

        mode = 4;
        if (match->qr2MatchType != 1) {
            mode = 6;
        }
    }
    if (DWCi_GetMatchCnt()->qr2MatchType == 3) {
        DWCi_GetMatchCnt()->qr2MatchType = 2;
    }
    DWCi_HandleMatchGPError(DWCi_GPSetServerStatus(mode));
    DWCi_GetMatchCnt()->reqProfileID = 0;
}

static void DWCi_StartReservingServer(int profileId) {
    DWCi_GetMatchCnt()->mode = 3;
    if (DWCi_GetMatchCnt()->sbObj == NULL) {
        DWCi_GetMatchCnt()->sbObj = ServerBrowserNewA(DWCi_GetMatchCnt()->gameName, DWCi_GetMatchCnt()->gameName,
                                                      DWCi_GetMatchCnt()->secretKey, 0, 0x14, 1, 0, DWCi_SBCallback, 0);
    }
    if (DWCi_GetMatchCnt()->sbObj != NULL || DWCi_HandleSBError(5) == 0) {
        DWCi_SetMatchStatus(3);
        if (DWCi_HandleMatchCommandError(DWCi_SendResvCommand(DWCi_GetMatchCnt()->self.profileId, 0)) != 0) {
            return;
        }
    }
}

static BOOL func_ov011_021679c0(void) {
    return DWC_GetNumConnectionHost();
}

int DWCi_SendServerPoll(BOOL force) {
    u32 packet[3];
    int i;

    if (DWC_IsServerMyself() == 0) {
        return 1;
    }
    if (DWCi_GetMatchCnt()->state == 0x13 || DWCi_GetMatchCnt()->state == 0x14) {
        return 1;
    }
    if (force != 0 || 15000 < OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->aidListSendTick)) {
        u64 now;

        packet[0] = DWCi_GetMatchCnt()->profileID;
        packet[1] = DWC_GetAIDBitmap();
        packet[2] = DWCi_GetMatchCnt()->synId;
        for (i = 0; i < DWCi_GetNumNodeInfo(); i++) {
            DWCNodeInfo *peer = DWCi_GetNodeInfoByIndex(i);

            if (peer != DWCi_GetOwnNodeInfo()) {
                if (DWCi_HandleMatchCommandError(DWCi_SendMatchCommand(0x83, peer->profileId, peer->publicIp,
                                                                       peer->publicPort, packet, 3)) != 0) {
                    return -1;
                }
            }
        }
        now = OS_GetTick();
        DWCi_GetMatchCnt()->aidListSendTick = now;
    }
    return 1;
}

static BOOL DWCi_SetSuspendAndNotify(u8 index, int set, int mode) {
    int count = 0;
    DWCNodeInfo *host = DWCi_GetNodeInfoByIndex(0);
    u32 oldFlags = DWCi_GetMatchCnt()->suspendAidBitmap;
    u32 flags;
    u32 packet[4];
    DWCNodeInfo *target;
    BOOL notify;
    int i;
    int hasSet;

    if (set != 0) {
        DWCMatchControl *match = DWCi_GetMatchCnt();

        match->suspendAidBitmap = match->suspendAidBitmap | (1 << index);
    } else {
        DWCMatchControl *match = DWCi_GetMatchCnt();

        match->suspendAidBitmap = match->suspendAidBitmap & ~(1 << index);
    }
    if (oldFlags != DWCi_GetMatchCnt()->suspendAidBitmap) {
        DWCi_GPSetServerStatus(-1);
    }
    flags = DWCi_GetMatchCnt()->suspendAidBitmap;
    if (func_ov011_02167f0c() == 0) {
        flags &= ~(1 << DWC_GetMyAID());
    }
    hasSet = func_ov011_02160fd8();
    notify = TRUE;
    if ((set == 0 || hasSet == 0) && (set != 0 || hasSet != 0)) {
        notify = FALSE;
    }
    if (mode == 1) {
        target = DWCi_GetNodeInfoByAID(index);

        packet[0] = DWCi_GetMatchCnt()->profileID;
        packet[1] = 1;
        count += 2;
        if (notify && DWCi_GetMatchCnt()->unk_30 != 0) {
            packet[count++] = set;
            packet[count++] = flags;
        }
        if (DWCi_HandleMatchCommandError(DWCi_SendMatchCommand(0x82, target->profileId, target->publicIp,
                                                               target->publicPort, packet, count)) != 0) {
            return FALSE;
        }
    }
    if (notify && DWCi_GetMatchCnt()->unk_30 != 1) {
        DWCi_GetMatchCnt()->unk_30 = 1;
        packet[0] = DWCi_GetMatchCnt()->profileID;
        packet[1] = 1;
        packet[2] = set;
        packet[3] = flags;
        for (i = 0; i < DWCi_GetNumNodeInfo(); i++) {
            DWCNodeInfo *peer = DWCi_GetNodeInfoByIndex(i);

            if (peer != host) {
                if (DWCi_HandleMatchCommandError(DWCi_SendMatchCommand(0x82, peer->profileId, peer->publicIp,
                                                                       peer->publicPort, packet, 4)) != 0) {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

static BOOL DWCi_StartMatchingAfterSearchOwn(void) {
    if (DWCi_GetMatchCnt()->sbObj != NULL) {
        ServerBrowserClear(DWCi_GetMatchCnt()->sbObj);
    }
    if (DWCi_GetMatchCnt()->unk_798 != 0) {
        DWCi_SetMatchStatus(0xc);
    } else if (DWCi_GetMatchCnt()->qr2MatchType == 0) {
        DWCi_SetMatchStatus(2);
    } else if (DWCi_GetMatchCnt()->qr2MatchType == 1) {
        DWCi_SetMatchStatus(3);
        return DWCi_HandleMatchCommandError(DWCi_SendResvCommandToFriend(0, 1, 0)) == 0;
    } else if (DWCi_GetMatchCnt()->qr2MatchType == 2) {
        DWCi_SetMatchStatus(0xd);
        DWCi_GetMatchCnt()->matched = 1;
        return TRUE;
    } else if (DWCi_GetMatchCnt()->qr2MatchType == 3) {
        DWCMatchControl *match = DWCi_GetMatchCnt();

        if (gpIsBuddy(match->pGpObj, DWCi_GetMatchCnt()->self.profileId) != 0) {
            DWCi_SetMatchStatus(3);
            if (DWCi_HandleMatchCommandError(DWCi_SendResvCommand(DWCi_GetMatchCnt()->self.profileId, 0)) != 0) {
                return FALSE;
            }
        }
        return TRUE;
    }
    DWCi_GetMatchCnt()->reqProfileID = 0;
    return DWCi_HandleSBError(DWCi_SBUpdateAsync(DWCi_GetMatchCnt()->reqProfileID)) == 0;
}

static void DWCi_SetSuspendRequest(int a, MatchCallback b, int c) {
    u64 now;

    DWCi_GetMatchCnt()->unk_30 = 0;
    DWCi_GetMatchCnt()->suspendRequest = a;
    DWCi_GetMatchCnt()->suspendCallback = b;
    DWCi_GetMatchCnt()->suspendParam = c;
    DWCi_GetMatchCnt()->suspendSendTick = 0;
    now = OS_GetTick();
    DWCi_GetMatchCnt()->suspendStartTick = now;
    DWCi_GetMatchCnt()->suspendPending = 1;
}

static BOOL DWCi_ProcessSuspendRequest(void) {
    int value;
    int hasSet;

    if (DWCi_GetMatchCnt()->suspendPending == 0) {
        return FALSE;
    }
    if (DWCi_GetMatchCnt()->suspendResend != 0) {
        DWCi_SetSuspendRequest(DWCi_GetMatchCnt()->suspendRequest, DWCi_GetMatchCnt()->suspendCallback,
                               DWCi_GetMatchCnt()->suspendParam);
        DWCi_GetMatchCnt()->suspendResend = 0;
    }
    value = DWCi_GetMatchCnt()->suspendRequest;
    if (value == 0xff) {
        return FALSE;
    }
    hasSet = func_ov011_02160fd8();
    if ((value != 0 && hasSet != 0) || (value == 0 && hasSet == 0)) {
        DWCi_FinishSuspendRequest(0);
        return TRUE;
    }
    DWCi_SendSuspendRequest(value);
    return TRUE;
}

static BOOL DWCi_SendSuspendRequest(int value) {
    u32 packet[4];
    DWCNodeInfo *self;
    DWCNodeInfo *host;

    if (DWCi_GetHostState() == 2) {
        return DWCi_SetSuspendAndNotify(DWCi_GetOwnNodeInfo()->aid, value, 0);
    }
    if (DWCi_GetMatchCnt()->state != 1) {
        return TRUE;
    }
    if (DWCi_GetMatchCnt()->unk_30 != 0) {
        if (OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->suspendSendTick) <
            DWCi_GetMatchCnt()->suspendWaitTime) {
            return TRUE;
        }
    }
    if (3000 < OS_TicksToMilliSeconds(OS_GetTick() - DWCi_GetMatchCnt()->suspendSendTick)) {
        u64 now;

        self = DWCi_GetOwnNodeInfo();
        host = DWCi_GetNodeInfoByIndex(0);
        packet[0] = DWCi_GetMatchCnt()->profileID;
        packet[1] = 0;
        packet[2] = value;
        packet[3] = self->aid;
        if (DWCi_HandleMatchCommandError(
                DWCi_SendMatchCommand(0x82, host->profileId, host->publicIp, host->publicPort, packet, 4)) != 0) {
            return FALSE;
        }
        now = OS_GetTick();
        DWCi_GetMatchCnt()->suspendSendTick = now;
        DWCi_GetMatchCnt()->suspendWaitTime = 3000;
    }
    return TRUE;
}

static BOOL func_ov011_02167f0c(void) {
    if (DWC_IsServerMyself() != 0) {
        if (DWCi_GetMatchCnt()->state == 0xd || DWCi_GetMatchCnt()->state == 0x16) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL DWCi_IsSvDownState(void) {
    int state = DWCi_GetMatchCnt()->state;

    if (state == 9 || state == 10 || state == 11) {
        return TRUE;
    }
    return FALSE;
}

static void DWCi_SetMatchCnt(DWCMatchControl *match) {
    sMatchState.control = match;
    sMatchState.clock = OS_GetTick();
}

static DWCMatchControl *DWCi_GetMatchCnt(void) {
    return sMatchState.control;
}

static void DWCi_SetMatchStatus(int state) {
    if (sMatchState.control->state != state) {
        disp_match_state(sMatchState.control->state, state);
    }
    sMatchState.control->state = state;
}

static void DWCi_SetHostState(int mode) {
    sMatchState.control->hostState = mode;
    if (DWCi_GetMatchCnt()->qr2Obj != NULL) {
        qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
    }
}

static u8 DWCi_GetHostState(void) {
    return DWCi_GetMatchCnt()->hostState;
}

static BOOL func_ov011_02167fd0(void) {
    if (DWCi_GetMatchCnt()->topology != 1) {
        return TRUE;
    }
    return FALSE;
}

static void DWCi_UpdateAllNodesRecvTime(void) {
    int i;

    for (i = 0; i < DWCi_GetNumNodeInfo(); i++) {
        DWCi_ResetLastSomeDataRecvTimeByAid(DWCi_GetNodeInfoByIndex(i)->aid);
    }
}

static void DWCi_InitNodeInfoList(void) {
    MI_CpuFill8(&DWCi_GetMatchCnt()->nodeCount, 0, 0x504);
}

int DWCi_GetNumNodeInfo(void) {
    return DWCi_GetMatchCnt()->nodeCount;
}

static BOOL DWCi_PushFrontNodeInfo(DWCNodeInfo *peer) {
    int i;

    if (DWCi_GetMatchCnt()->nodeCount == 0x20) {
        return FALSE;
    }
    for (i = DWCi_GetMatchCnt()->nodeCount; i > 0; i--) {
        DWCi_GetMatchCnt()->nodes[i] = DWCi_GetMatchCnt()->nodes[i - 1];
    }
    DWCi_GetMatchCnt()->nodes[0] = *peer;
    DWCi_GetMatchCnt()->nodeCount++;
    return TRUE;
}

static BOOL DWCi_PushBackNodeInfo(DWCNodeInfo *peer) {
    if (DWCi_GetMatchCnt()->nodeCount == 0x20) {
        return FALSE;
    }
    DWCi_GetMatchCnt()->nodes[DWCi_GetMatchCnt()->nodeCount] = *peer;
    DWCi_GetMatchCnt()->nodeCount++;
    return TRUE;
}

static BOOL DWCi_DeleteNodeInfoByAID(int index) {
    int i;

    if (DWCi_GetMatchCnt()->nodeCount == 0) {
        return FALSE;
    }
    for (i = 0; i < DWCi_GetMatchCnt()->nodeCount; i++) {
        if (index == DWCi_GetMatchCnt()->nodes[i].aid) {
            break;
        }
    }
    if (i == DWCi_GetMatchCnt()->nodeCount) {
        return FALSE;
    }
    for (; i < DWCi_GetMatchCnt()->nodeCount - 1; i++) {
        DWCi_GetMatchCnt()->nodes[i] = DWCi_GetMatchCnt()->nodes[i + 1];
    }
    DWCi_GetMatchCnt()->nodeCount--;
    return TRUE;
}

static BOOL DWCi_DeleteNodeInfoByProfileID(int id) {
    int i;

    if (DWCi_GetMatchCnt()->nodeCount == 0) {
        return FALSE;
    }
    for (i = 0; i < DWCi_GetMatchCnt()->nodeCount; i++) {
        if (id == DWCi_GetMatchCnt()->nodes[i].profileId) {
            break;
        }
    }
    if (i == DWCi_GetMatchCnt()->nodeCount) {
        return FALSE;
    }
    for (; i < DWCi_GetMatchCnt()->nodeCount - 1; i++) {
        DWCi_GetMatchCnt()->nodes[i] = DWCi_GetMatchCnt()->nodes[i + 1];
    }
    DWCi_GetMatchCnt()->nodeCount--;
    return TRUE;
}

static DWCNodeInfo *DWCi_GetOwnNodeInfo(void) {
    return DWCi_GetNodeInfoByProfileID(DWCi_GetMatchCnt()->profileID);
}

static DWCNodeInfo *DWCi_GetServerNodeInfo(void) {
    if (DWCi_GetMatchCnt()->nodeCount >= 1) {
        return DWCi_GetMatchCnt()->nodes;
    }
    return NULL;
}

DWCNodeInfo *DWCi_GetNodeInfoByIndex(int index) {
    if (index < DWCi_GetMatchCnt()->nodeCount) {
        return &DWCi_GetMatchCnt()->nodes[index];
    }
    return NULL;
}

static DWCNodeInfo *DWCi_GetNodeInfoByProfileID(int id) {
    int i;

    for (i = 0; i < DWCi_GetMatchCnt()->nodeCount; i++) {
        if (id == DWCi_GetMatchCnt()->nodes[i].profileId) {
            return &DWCi_GetMatchCnt()->nodes[i];
        }
    }
    return NULL;
}

static DWCNodeInfo *DWCi_GetNodeInfoByAID(u8 index) {
    int i;

    for (i = 0; i < DWCi_GetMatchCnt()->nodeCount; i++) {
        if (index == DWCi_GetMatchCnt()->nodes[i].aid) {
            return &DWCi_GetMatchCnt()->nodes[i];
        }
    }
    return NULL;
}

static u8 DWCi_GetFreeAID(void) {
    u8 index;
    int i;

    if (DWCi_GetMatchCnt()->nodeCount == 0x20) {
        return 0xff;
    }
    for (index = 0; index < 0x20; index++) {
        for (i = 0; i < DWCi_GetMatchCnt()->nodeCount; i++) {
            if (index == DWCi_GetMatchCnt()->nodes[i].aid) {
                break;
            }
        }
        if (i == DWCi_GetMatchCnt()->nodeCount) {
            return index;
        }
    }
    return 0xff;
}

static BOOL DWCi_IsNodeNotConnected(int id) {
    DWCNodeInfo *peer = DWCi_GetNodeInfoByProfileID(id);

    if (peer == NULL) {
        return FALSE;
    }
    if (peer == DWCi_GetOwnNodeInfo()) {
        return FALSE;
    }
    if (DWCi_GetGT2Connection(peer->aid) == 0) {
        return TRUE;
    }
    return FALSE;
}

static void DWCi_ClearTargetNodeInfo(void) {
    DWCi_ClearNodeInfo(&DWCi_GetMatchCnt()->self);
}

static BOOL DWCi_IsTargetNodeInfoValid(void) {
    return DWCi_IsValidNodeInfo(&DWCi_GetMatchCnt()->self);
}

static void DWCi_ClearNodeInfo(void *address) {
    MI_CpuFill8(address, 0, 0x28);
}

static BOOL DWCi_IsValidNodeInfo(DWCNodeInfo *peer) {
    if (peer->profileId != 0) {
        return TRUE;
    }
    return FALSE;
}

static u64 func_ov011_021683b8(void) {
    DWCNodeInfo *self = DWCi_GetOwnNodeInfo();
    u64 time = OS_GetTick() + OS_MilliSecondsToTicks(3000);
    int selfIndex = 0;
    int active = 0;
    int i;

    if (DWCi_GetNumNodeInfo() > 2) {
        for (i = 1; i < DWCi_GetNumNodeInfo(); i++) {
            DWCNodeInfo *peer = DWCi_GetNodeInfoByIndex(i);

            if (self == peer) {
                selfIndex = i;
            } else {
                DWCi_GetGT2Connection(peer->aid);
                if (DWCi_GetGT2Connection(peer->aid) != 0) {
                    active++;
                }
            }
        }
    }
    {
        u64 activeTicks = (u64)active * OS_MilliSecondsToTicks(3000);
        u64 waitTicks;

        time += activeTicks;
        waitTicks = OS_MilliSecondsToTicks((DWCi_GetNumNodeInfo() - 1 - selfIndex) * 6000);
        time += waitTicks;
    }
    return time;
}

static BOOL func_ov011_02168468(void) {
    if (DWCi_GetMatchCnt()->memberConnecting == 0) {
        return FALSE;
    }
    DWCi_GetMatchCnt()->memberConnecting = 0;
    DWCi_GetMatchCnt()->abortedMemberState = DWCi_GetMatchCnt()->state;
    if (DWCi_IsTargetNodeInfoValid() != 0) {
        DWCi_GetMatchCnt()->abortedMember = DWCi_GetMatchCnt()->self;
    } else {
        MI_CpuFill8(&DWCi_GetMatchCnt()->abortedMember, 0, 0x28);
    }
    if (DWCi_GetMatchCnt()->target.cookie != 0) {
        NNCancel(DWCi_GetMatchCnt()->target.cookie);
        DWCi_GetMatchCnt()->target.cookie = 0;
        DWCi_GetMatchCnt()->nnRecvCount = 0;
        DWCi_GetMatchCnt()->nnLastCookie = 0;
        DWCi_GetMatchCnt()->nnFailedTime = 0;
    }
    if (DWCi_GetMatchCnt()->state == 3 || DWCi_GetMatchCnt()->state == 5 || DWCi_GetMatchCnt()->state == 6 ||
        DWCi_GetMatchCnt()->state == 0xe || DWCi_GetMatchCnt()->state == 0xf) {
        if (DWCi_GetMatchCnt()->reqProfileID != 0) {
            if (DWCi_HandleMatchCommandError(DWCi_CancelReservation(0)) != 0) {
                DWCi_ClearTargetNodeInfo();
                return TRUE;
            }
        }
        DWCi_GetMatchCnt()->qr2IsReserved = 0;
        DWCi_GetMatchCnt()->qr2Reservation = 0;
    }
    DWCi_ClearTargetNodeInfo();
    return TRUE;
}

static int func_ov011_02168590(void) {
    return DWCi_GetMatchCnt()->unlocked;
}

static BOOL DWCi_IsMatchLocked(void) {
    if (DWCi_GetMatchCnt()->lockEnabled == 0) {
        return FALSE;
    }
    if (DWC_IsServerMyself() != 0) {
        if (DWCi_GetMatchCnt()->qr2MatchType == 1 || DWCi_GetMatchCnt()->qr2MatchType == 3 ||
            DWCi_GetMatchCnt()->qr2MatchType == 2) {
            if (func_ov011_02168590() == 0) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static int DWCi_GetTopologyType(void) {
    return DWCi_GetMatchCnt()->topology;
}

static void func_ov011_021685fc(void) {
    DWCi_GetMatchCnt()->abortedMemberState = 0;
    MI_CpuFill8(&DWCi_GetMatchCnt()->abortedMember, 0, 0x28);
}

static void DWCi_ApplyPendingSuspend(void) {
    if (DWCi_GetMatchCnt()->suspendRequest != 0xff) {
        if (DWCi_GetMatchCnt()->suspendRequest == 1) {
            DWCMatchControl *match = DWCi_GetMatchCnt();

            match->suspendAidBitmap = DWC_GetAIDBitmap();
        } else {
            DWCi_GetMatchCnt()->suspendAidBitmap = 0;
        }
        DWCi_FinishSuspendRequest(0);
    }
}

static void DWCi_FinishSuspendRequest(int cancelled) {
    if (DWCi_GetMatchCnt()->suspendCallback != 0) {
        DWCi_GetMatchCnt()->suspendCallback(cancelled, DWCi_GetMatchCnt()->suspendRequest,
                                            DWCi_GetMatchCnt()->suspendParam);
    }
    if (cancelled == 0) {
        qr2_send_statechanged(DWCi_GetMatchCnt()->qr2Obj);
    }
    DWCi_GetMatchCnt()->suspendRequest = 0xff;
    DWCi_GetMatchCnt()->suspendPending = 0;
}

static BOOL func_ov011_021686a0(void) {
    if (DWCi_GetMatchCnt()->topology == 2) {
        return TRUE;
    }
    return FALSE;
}

static int func_ov011_021686b8(void) {
    return DWCi_GetNumNodeInfo();
}

u8 DWCi_GetBuddyStatusLCK(const GPBuddyStatus *status) {
    u8 result = 0;
    char text[4] = { 0, 0, 0, 0 };

    if (DWC_GetCommonValueString("LCK", text, status->statusString, '/') > 0) {
        result = strtoul(text, NULL, 10);
    }
    return result;
}

static const char *DWCi_GetResvDenyReasonString(int code) {
    switch (code) {
    case 0x10:
        return "Game server is fully occupied.";
    case 0x11:
        return "This Domain is already closed.";
    case 0x12:
        return "The condition was not satisfied.";
    case 0x13:
        return "This Domain is already locked.";
    case 0x14:
        return "It tried to go to the client for the reservation.";
    case 0x15:
        return "It is a reservation to the friend who doesn't exist in the list.";
    case 0x16:
        return "It was rejected by the attempt callback.";
    case 0x17:
        return "The reservation came from a different other host.";
    case 0x18:
        return "Illegal mesh reservation.";
    default:
        return "";
    }
}

static void disp_match_state(int oldState, int newState) {
    sMatchState.clock = OS_GetTick();
}

void DWCi_SetMatchProfileID(int profileId) {
    DWCi_GetMatchCnt()->profileID = profileId;
}

void DWCi_RegisterMatchQR2Keys(void) {
    qr2_register_keyA(0x32, "dwc_pid");
    qr2_register_keyA(0x33, "dwc_mtype");
    qr2_register_keyA(0x34, "dwc_mver");
    qr2_register_keyA(0x35, "dwc_eval");
    qr2_register_keyA(0x36, "dwc_groupid");
    qr2_register_keyA(0x37, "dwc_hoststate");
    qr2_register_keyA(0x38, "dwc_suspend");
}

static void DWCi_NNFreeNegotiateList(void) {
    NNCancel(DWCi_GetMatchCnt()->target.cookie);
    if (sMatchState.inNNThink == 0) {
        NNFreeNegotiateList();
    } else {
        sMatchState.nnFreeNegotiateList = 1;
    }
}
