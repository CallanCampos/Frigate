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
// strings, such as "dwc_pid" and "Game server is fully occupied.", are its own.

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

// The console that a NAT negotiation goes to
typedef struct MatchTarget {
    u8 kind;
    u8 unk_01;
    u16 port;
    u32 ip;
    u32 cookie;
} MatchTarget;

// Called when a suspension of the match ends, with whether it was canceled
typedef void (*MatchCallback)(int, int, int);
typedef void (*MatchNotifyFunc)(int error, BOOL cancel, BOOL self, BOOL isServer, int index, void *param);
typedef int (*MatchCheckFunc)(void *userData, u32 param);

typedef struct FriendsMatchControl FriendsMatchControl;

// The match control (func_ov011_02167f6c returns it; it is FriendsMatchControl.match)
typedef struct MatchControl {
    GPConnection *gp;
    FriendsMatchControl *fm;
    GTI2ConnectionCallbacks *callbacks;
    u8 retries;
    u8 unk_0d;
    u8 unk_0e[0x2];
    QR2Implementation *qr2;
    u8 mode;
    u8 matchType;
    u8 hostState;
    u8 unk_17;
    u8 unk_18;
    u8 unk_19;
    u8 unk_1a[0x2];
    u32 unk_1c;
    u64 unk_20;
    u32 suspend;
    s32 unk_2c;
    u8 unk_30;
    u8 unk_31[0x3];
    MatchCallback unk_34;
    u32 unk_38;
    u64 unk_3c;
    s32 unk_44;
    s32 unk_48;
    u8 unk_4c;
    u8 unk_4d[0x3];
    s32 unk_50;
    DWCNodeInfo unk_54[0x20];
    DWCNodeInfo self;
    DWCNodeInfo pending;
    s32 unk_5a4;
    s32 unk_5a8;
    DWCNodeInfo unk_5ac;
    u16 unk_5d4;
    u8 unk_5d6[0x2];
    s32 unk_5d8;
    s32 unk_5dc;
    ServerBrowser *sb;
    s32 unk_5e4;
    u64 unk_5e8;
    u64 unk_5f0;
    u8 unk_5f8;
    u8 unk_5f9;
    u16 unk_5fa;
    s32 unk_5fc;
    u64 unk_600;
    u64 unk_608;
    MatchTarget target;
    s32 state;
    u8 unk_620;
    u8 unk_621;
    u8 unk_622[0x6];
    u8 unk_628;
    u8 unk_629;
    u8 unk_62a;
    u8 unk_62b;
    u8 unk_62c;
    u8 unk_62d;
    u8 unk_62e;
    u8 unk_62f;
    u16 unk_630;
    u16 unk_632;
    s32 unk_634;
    s32 unk_638;
    u64 unk_63c;
    u32 unk_644;
    u64 unk_648;
    u32 unk_650;
    u32 unk_654;
    u64 unk_658;
    u64 unk_660;
    u64 unk_668;
    u64 unk_670;
    s32 profileId;
    s32 unk_67c;
    s32 unk_680;
    char *gameName;
    char *secretKey;
    DWCFriendData *friendList;
    s32 friendListLen;
    u8 friendOrder[0x40];
    s32 friendCount;
    u8 unk_6d8;
    u8 unk_6d9;
    u16 unk_6da;
    u32 unk_6dc;
    u32 unk_6e0[0x20];
    u32 unk_760;
    u32 unk_764;
    u64 unk_768;
    MatchNotifyFunc notifyFunc;
    void *notifyParam;
    DWCNewClientFunc newClientCallback;
    void *newClientParam;
    DWCEvalFunc evalCallback;
    void *evalParam;
    u32 unk_788;
    u32 unk_78c;
    s32 groupId;
    u8 unk_794;
    u8 unk_795[0x3];
    s32 unk_798;
    u8 unk_79c;
    u8 unk_79d;
    u8 unk_79e[0x2];
    u64 unk_7a0;
    u8 unk_7a8;
    u8 unk_7a9[0x3];
    u32 unk_7ac;
    s32 unk_7b0;
    u8 error;
    u8 unk_7b5;
    u8 unk_7b6;
    u8 unk_7b7;
    MatchCheckFunc checkFunc;
    int userData;
    u32 unk_7c0;
    u32 unk_7c4;
    u32 unk_7c8;
    u64 unk_7cc;
    s32 unk_7d4;
    DWCTransportInfo transport;
} MatchControl;

// The friends match control, which func_ov011_0215fa0c allocates: the GT2 socket that the consoles talk through and
// the callbacks of its connections, the GameSpy Presence connection, the user data, the state (0 before the login, 1
// checking that the services are up, 2 logging in, 3 logged in, 4 matching, 5 matched), the player's name and profile
// ID, the game's callbacks and the login, friend list and match parts
struct FriendsMatchControl {
    GTI2Socket *socket;
    GTI2ConnectionCallbacks callbacks;
    int sendBufSize;
    int recvBufSize;
    GPConnection gp;
    DWCUserData *userData;
    int state;
    int prevState;
    u8 unk_2c;
    u8 unk_2d;
    u8 unk_2e[0x2];
    u16 name[0x1a];
    s32 profileId;
    char *gameName;
    char *secretKey;
    DWCLoginCallback loginCallback;
    void *loginParam;
    DWCUpdateServersCallback updateCallback;
    void *updateParam;
    DWCMatchedFunc matchedCallback;
    void *matchedParam;
    DWCClosedFunc closedCallback;
    void *closedParam;
    DWCLoginControl login;
    DWCFriendControl friendControl;
    MatchControl match;
};

// The library's one friends match control (sMatchRoot.fm, which func_ov011_0215fa0c makes), and the socket error that
// closed its GT2 socket
typedef struct MatchRoot {
    int socketError;
    FriendsMatchControl *fm;
} MatchRoot;

// What func_ov011_02167f6c returns (sMatchState.control), the query text of the match, and the clock of the last
// state change
typedef struct MatchState {
    u32 unk_00;
    MatchControl *control;
    u32 unk_08;
    char *query;
    u32 unk_10;
    u32 unk_14;
    u64 clock;
} MatchState;

// A key that is added to the match (key IDs 100 to 253): its ID (0 when the slot is free), whether its value is a
// text, and the key's name and value
typedef struct MatchKey {
    u8 id;
    u8 isText;
    u16 unk_02;
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
    u8 unk_14[4];
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
static GTI2Connection *sConnections[0x20];
static MatchConnectionInfo sConnectionInfos[0x20];
static MatchState sMatchState;
static MatchKey sKeys[0x9a];
// The chance in percent that a random match looks for a server first (-2 never, -1 always), and how often a machine
// may fail before it is left out
static s32 sMatchChance[2] = { -2, 5 };

static int func_ov011_021603a4(void);
static u8 func_ov011_021603b8(void);
static int func_ov011_02160500(void);
static void func_ov011_0216051c(void);
static GTI2Connection **func_ov011_02160540(int index);
static GTI2Connection **func_ov011_0216054c(s32 profileId, int a1);
static MatchConnectionInfo *func_ov011_0216057c(int index);
static void func_ov011_021605b4(int state);
static s32 func_ov011_021605c8(int netId);
static u32 func_ov011_021605e4(const u8 *netIds, int count);
static int func_ov011_02160604(int result);
static int func_ov011_021606a4(int result);
static void func_ov011_0216076c(int error, int profileId, void *param);
static void func_ov011_021607a8(int error, int count, void *param);
static void func_ov011_021607bc(int error, BOOL cancel, BOOL a2, BOOL a3, int index, void *param);
static void func_ov011_02160844(GPConnection *connection, void *arg, void *param);
static void func_ov011_02160870(GPConnection *connection, void *arg, void *param);
static void func_ov011_02160934(GTI2Connection *connection, const void *message, int length, BOOL reliable);
static void func_ov011_0216093c(GTI2Connection *conn, int closeType, int isNetId, u8 netId);
static void func_ov011_02160d10(GTI2Connection *conn, int closeType);
static void func_ov011_02160d1c(GTI2Connection *connection, u32 latency);
static void func_ov011_02160d24(GTI2Socket *socket);
static void func_ov011_02160d4c(u8 mode);
static BOOL func_ov011_02160d88(void);
static BOOL func_ov011_02160dac(void);
static BOOL func_ov011_02160dd8(void);
static BOOL func_ov011_02160fd8(void);
static void func_ov011_02161048(MatchControl *cnt, GPConnection *gp, FriendsMatchControl *fm,
                                GTI2ConnectionCallbacks *callbacks, char *gameName, char *secretKey,
                                DWCFriendData *friendList, int friendListLen);
static int func_ov011_021611a8(s32 profileId);
static void func_ov011_021612d0(int type, u8 numEntry, const char *query, DWCMatchedFunc matched, void *matchedParam,
                                DWCNewClientFunc newClient, void *newClientParam, DWCEvalFunc eval, void *evalParam,
                                DWCAttemptFunc attempt, int *attemptParam, void *param);
static void func_ov011_02161468(int type, u8 numEntry, DWCMatchedFunc matched, void *matchedParam,
                                DWCNewClientFunc newClient, void *newClientParam, DWCAttemptFunc attempt,
                                int *attemptParam, void *param);
static void func_ov011_0216156c(int type, s32 profileId, DWCMatchedFunc matched, void *matchedParam,
                                DWCNewClientFunc newClient, void *newClientParam, DWCAttemptFunc attempt,
                                int *attemptParam, void *param);
static void func_ov011_02161788(QR2Implementation *server);
static void func_ov011_021617b4(int a0);
static BOOL func_ov011_021623a0(GTI2Socket *socket, u32 ip, u16 port, const void *message, int length);
static void func_ov011_02162440(GTI2Socket *socket, GTI2Connection *connection, u32 ip, u16 port, int latency,
                                const void *message, int length);
static void func_ov011_0216261c(char *dest, u32 size);
static void func_ov011_02162640(GTI2Connection *connection, int result, const void *message, int length);
static void func_ov011_02162804(void *unused, int id, const u8 *str);
static void func_ov011_02162864(void);
static void func_ov011_02162880(int type, int code);
static void func_ov011_02162908(void);
static void func_ov011_02162958(int conn, int kind, const u32 *msg);
static void func_ov011_02162bf8(int conn, int kind, const u32 *msg);
static void func_ov011_02162c00(u32 mask);
static void func_ov011_02162c30(const u32 *msg);
static BOOL func_ov011_02162c68(int type, u32 code, s32 profileId);
static void func_ov011_02162d64(u32 id);
static void func_ov011_02162e44(void);
static BOOL func_ov011_02162e6c(void);
static void func_ov011_02162e80(int mode);
static void func_ov011_021630f8(u8 mode, u8 unk17, DWCMatchedFunc callback, void *param);
static void func_ov011_021631a8(void);
static int func_ov011_0216322c(u32 pid);
static int func_ov011_021634a0(char *out, u32 a, u32 b, u32 c, u8 kind);
static BOOL func_ov011_02163534(u32 ip);
static int func_ov011_021635a0(int reset, u32 token, DWCNodeInfo *peer);
static int func_ov011_0216373c(MatchTarget *target);
static BOOL func_ov011_021637b0(int type, int id);
static int func_ov011_021637d8(int type, int id, u32 ip, u16 port, const u32 *data, int count);
static void func_ov011_0216390c(MatchPacket *packet, int type, const u32 *data, int count);
static int func_ov011_02163974(int type, u32 ip, u16 port, const u32 *data, int count);
static int func_ov011_021639c8(void *gp, u8 command, int id, const char *text);
static int func_ov011_02163a30(int type, s32 profileId, u32 ip, u32 port, const u32 *data, int count);
static int func_ov011_0216475c(int profileId, int ip, int port, int nodeType, int skipChecks, int cookie,
                               void *userData, int blocked);
static int func_ov011_0216484c(int profileId, int ip, int port, int nodeType, int skipChecks, int cookie,
                               void *userData, int blocked);
static int func_ov011_02164aa0(int profileId, u32 ip, u32 port);
static int func_ov011_02164b78(int error);
static int func_ov011_02164b98(int profileId, int flag);
static int func_ov011_02164d64(int a0, int a1, int a2);
static int func_ov011_02164f54(int a0);
static int func_ov011_02165040(int unused);
static int func_ov011_02165094(int profileId, int a1);
static int func_ov011_02165164(int profileId);
static int func_ov011_02165194(int unused);
static void func_ov011_02165280(int a0);
static void func_ov011_0216549c(void);
static void func_ov011_021654c0(void);
static void func_ov011_02165540(void);
static int func_ov011_02165558(void);
static int func_ov011_02165654(void);
static void func_ov011_0216574c(void);
static void func_ov011_021658a4(void);
static int func_ov011_0216592c(void);
static void func_ov011_021659ac(int a0);
static void func_ov011_02165b30(void);
static int func_ov011_02165b54(void);
static int func_ov011_02165b7c(int profileId);
static void func_ov011_02165bc8(void);
static int func_ov011_02165bf0(u32 mask);
static void func_ov011_02165d2c(u32 aid, int type);
static int func_ov011_02165ef4(void);
static int func_ov011_02166098(void);
static void func_ov011_021660f8(void);
static int func_ov011_02166148(int profileId, int message);
static int func_ov011_021661b4(int profileId, int message, int value);
static int func_ov011_02166364(void);
static u8 func_ov011_021664c4(void);
static void func_ov011_021664f0(void);
static BOOL func_ov011_02166528(int profileId);
static int func_ov011_02166580(SBServer *server);
static u8 func_ov011_021665f0(int index, int a1);
static u32 func_ov011_02166604(int excludeSelf);
static void func_ov011_02166648(void);
static int func_ov011_02166670(int result);
static int func_ov011_021666c4(int result);
static int func_ov011_0216672c(int result);
static int func_ov011_021667b8(int result);
static int func_ov011_021667f8(int result);
static int func_ov011_02166830(int result);
static void func_ov011_0216689c(ServerBrowser *sb, SBCallbackReason reason, SBServer *server, void *instance);
static void func_ov011_02166e48(SBServer *server);
static BOOL func_ov011_02166e4c(int sortAfter);
static void func_ov011_02166f8c(void);
static void func_ov011_021670a4(int keyId, QR2Buffer *outBuf, void *userData);
static void func_ov011_021671b8(int keyId, int index, QR2Buffer *outBuf, void *userData);
static void func_ov011_021671bc(int keyId, int index, QR2Buffer *outBuf, void *userData);
static void func_ov011_021671c0(int keyType, QR2KeyBuffer *keyBuffer, void *userData);
static int func_ov011_0216723c(int keyType, void *userData);
static void func_ov011_02167240(int error, const char *errorMessage, void *userData);
static void func_ov011_02167248(u32 ip, u16 port, void *userData);
static void func_ov011_02167264(int cookie, void *userData);
static void func_ov011_0216738c(const char *data, int size, void *userData);
static void func_ov011_02167424(int state, void *userData);
static void func_ov011_02167428(int result, int gameSocket, SocAddrIn *address, void *userData);
static BOOL func_ov011_02167718(int skip);
static BOOL func_ov011_02167778(void);
static u32 func_ov011_02167810(void);
static void func_ov011_02167838(MatchCheckFunc a, const void *userData, u32 c);
static void func_ov011_0216788c(int isHost);
static void func_ov011_02167924(int profileId);
static BOOL func_ov011_021679c0(void);
static BOOL func_ov011_02167ac4(u32 index, int set, int mode);
static BOOL func_ov011_02167c28(void);
static void func_ov011_02167d24(int a, MatchCallback b, int c);
static BOOL func_ov011_02167d6c(void);
static BOOL func_ov011_02167de0(int value);
static BOOL func_ov011_02167f0c(void);
static void func_ov011_02167f58(MatchControl *match);
static MatchControl *func_ov011_02167f6c(void);
static void func_ov011_02167f78(int state);
static void func_ov011_02167fa0(int mode);
static u8 func_ov011_02167fc4(void);
static BOOL func_ov011_02167fd0(void);
static void func_ov011_02167fe8(void);
static void func_ov011_0216800c(void);
static BOOL func_ov011_02168030(DWCNodeInfo *peer);
static BOOL func_ov011_021680a4(DWCNodeInfo *peer);
static BOOL func_ov011_021680e4(int index);
static BOOL func_ov011_02168184(int id);
static DWCNodeInfo *func_ov011_02168224(void);
static DWCNodeInfo *func_ov011_02168238(void);
static DWCNodeInfo *func_ov011_02168270(int id);
static DWCNodeInfo *func_ov011_021682ac(int index);
static u8 func_ov011_021682ec(void);
static BOOL func_ov011_02168348(int id);
static void func_ov011_02168374(void);
static BOOL func_ov011_02168388(void);
static void func_ov011_0216839c(void *address);
static BOOL func_ov011_021683a8(DWCNodeInfo *peer);
static u64 func_ov011_021683b8(void);
static BOOL func_ov011_02168468(void);
static int func_ov011_02168590(void);
static BOOL func_ov011_021685a0(void);
static int func_ov011_021685ec(void);
static void func_ov011_021685fc(void);
static void func_ov011_02168620(void);
static void func_ov011_02168654(int cancelled);
static BOOL func_ov011_021686a0(void);
static int func_ov011_021686b8(void);
static const char *func_ov011_021686f8(int code);
static void func_ov011_0216876c(int oldState, int newState);
static void func_ov011_021687ec(void);

void func_ov011_0215fa0c(DWCUserData *userData, int productId, const char *secretKey, int sendBufSize, int recvBufSize,
                         DWCFriendData *friendList, int friendListLen) {
    u32 len;

    sMatchRoot.fm = DWC_Alloc(4, sizeof(FriendsMatchControl));
    MI_CpuFill8(sMatchRoot.fm, 0, sizeof(FriendsMatchControl));
    DWC_ClearError();
    sMatchRoot.fm->socket = NULL;
    sMatchRoot.fm->callbacks.onConnected = func_ov011_02162640;
    sMatchRoot.fm->callbacks.onReceived = func_ov011_02160934;
    sMatchRoot.fm->callbacks.onClosed = func_ov011_02160d10;
    sMatchRoot.fm->callbacks.onPing = func_ov011_02160d1c;
    if (sendBufSize == 0) {
        sendBufSize = 0x2000;
    }
    sMatchRoot.fm->sendBufSize = sendBufSize;
    if (recvBufSize == 0) {
        recvBufSize = 0x2000;
    }
    sMatchRoot.fm->recvBufSize = recvBufSize;
    sMatchRoot.fm->gp = NULL;
    sMatchRoot.fm->userData = userData;
    sMatchRoot.fm->state = 0;
    sMatchRoot.fm->prevState = 0;
    sMatchRoot.fm->unk_2d = 0;
    sMatchRoot.fm->profileId = 0;
    sMatchRoot.fm->gameName = gcd_gamename;
    sMatchRoot.fm->secretKey = gcd_secret_key;
    sMatchRoot.fm->loginCallback = NULL;
    sMatchRoot.fm->loginParam = NULL;
    sMatchRoot.fm->updateCallback = NULL;
    sMatchRoot.fm->updateParam = NULL;
    sMatchRoot.fm->matchedCallback = NULL;
    sMatchRoot.fm->matchedParam = NULL;
    sMatchRoot.fm->closedCallback = NULL;
    sMatchRoot.fm->closedParam = NULL;
    func_ov011_0216051c();
    DWCi_LoginInit(&sMatchRoot.fm->login, userData, &sMatchRoot.fm->gp, productId, userData->gameCode,
                   sMatchRoot.fm->name, func_ov011_0216076c, NULL);
    DWCi_FriendInit(&sMatchRoot.fm->friendControl, &sMatchRoot.fm->gp, sMatchRoot.fm->name, friendList, friendListLen);
    func_ov011_02161048(&sMatchRoot.fm->match, &sMatchRoot.fm->gp, sMatchRoot.fm, &sMatchRoot.fm->callbacks,
                        gcd_gamename, gcd_secret_key, friendList, friendListLen);
    func_ov011_02168d0c(&sMatchRoot.fm->match.transport);
    len = strlen(secretKey);
    if (len < 0x100) {
        len = strlen(secretKey);
    } else {
        len = 0xff;
    }
    MI_CpuCopy8(secretKey, gcd_secret_key, len);
    gcd_secret_key[len] = 0;
}

void func_ov011_0215fb78(void) {
    if (sMatchRoot.fm != NULL) {
        if (sMatchRoot.fm->match.qr2 != NULL) {
            qr2_shutdown(sMatchRoot.fm->match.qr2);
            sMatchRoot.fm->match.qr2 = NULL;
        }
        sMatchRoot.fm->match.unk_4c = 0;
        if (sMatchRoot.fm->match.sb != NULL) {
            ServerBrowserFree(sMatchRoot.fm->match.sb);
            sMatchRoot.fm->match.sb = NULL;
        }
        func_ov011_021687ec();
        func_ov011_0215f2c0();
        CloseStatsConnection();
        func_ov011_0215f2cc();
        if (sMatchRoot.fm->gp != NULL) {
            gpSetCallback(&sMatchRoot.fm->gp, 0, NULL, 0);
            gpSetCallback(&sMatchRoot.fm->gp, 3, NULL, 0);
            gpSetCallback(&sMatchRoot.fm->gp, 1, NULL, 0);
            gpSetCallback(&sMatchRoot.fm->gp, 2, NULL, 0);
            gpProcess(&sMatchRoot.fm->gp);
            gpDestroy(&sMatchRoot.fm->gp);
            sMatchRoot.fm->gp = NULL;
        }
        DWCi_ShutdownLogin();
        DWCi_ShutdownFriend();
        func_ov011_02162e44();
        func_ov011_02168fec();
        if (sMatchRoot.fm->socket != NULL) {
            func_ov011_0217d158(sMatchRoot.fm->socket);
            sMatchRoot.fm->socket = NULL;
        }
        DWC_Free(4, sMatchRoot.fm, 0);
        sMatchRoot.fm = NULL;
    }
}

void func_ov011_0215fc50(void) {
    DWC_UpdateConnection();
    if (sMatchRoot.fm == NULL || sMatchRoot.fm->state == 0 || DWCi_IsError()) {
        return;
    }
    switch (sMatchRoot.fm->state) {
    case 1:
        switch (GSIAvailableCheckThink()) {
        case 1:
            if (func_ov011_02160604(gpInitialize(&sMatchRoot.fm->gp, sMatchRoot.fm->login.productId, 0x10, 0xb))) {
                return;
            }
            if (func_ov011_02160604(gpSetCallback(&sMatchRoot.fm->gp, 0, func_ov011_02160844, 0))) {
                return;
            }
            if (func_ov011_02160604(gpSetCallback(&sMatchRoot.fm->gp, 3, func_ov011_02160870, 0))) {
                return;
            }
            if (func_ov011_02160604(
                    gpSetCallback(&sMatchRoot.fm->gp, 7, (GPCallback)DWCi_GPRecvBuddyAuthCallback, 0))) {
                return;
            }
            if (func_ov011_02160604(
                    gpSetCallback(&sMatchRoot.fm->gp, 1, (GPCallback)DWCi_GPRecvBuddyRequestCallback, 0))) {
                return;
            }
            if (func_ov011_02160604(
                    gpSetCallback(&sMatchRoot.fm->gp, 2, (GPCallback)DWCi_GPRecvBuddyStatusCallback, 0))) {
                return;
            }
            func_ov011_021605b4(2);
            if (DWCi_LoginAsync() == 0) {
                DWCi_StopLogin(2, -20100);
            }
            break;
        case 2:
            DWCi_StopLogin(3, -20110);
            return;
        case 3:
            DWCi_StopLogin(4, -20101);
            return;
        }
        break;
    case 2:
        DWCi_LoginProcess();
        break;
    case 3:
        DWCi_FriendProcess();
        func_ov011_021617b4(0);
        break;
    case 4:
        func_ov011_021617b4(1);
        DWCi_FriendProcess();
        break;
    case 5:
        func_ov011_02168db0();
        DWCi_FriendProcess();
        if (sMatchRoot.fm->match.mode == 2 || sMatchRoot.fm->match.mode == 3) {
            func_ov011_021617b4(1);
        } else if (sMatchRoot.fm->socket != NULL) {
            func_ov011_021617b4(0);
        }
        break;
    }
    if (sMatchRoot.fm->match.unk_4c == 1) {
        if (sMatchRoot.fm->match.qr2 != NULL) {
            qr2_shutdown(sMatchRoot.fm->match.qr2);
            sMatchRoot.fm->match.qr2 = NULL;
        }
        sMatchRoot.fm->match.unk_4c = 0;
    }
}

BOOL func_ov011_0215fdf0(const u16 *name, const char *reserved, void (*callback)(int error), void *param) {
    int len;

    if (name == NULL) {
        return FALSE;
    }
    if (DWCi_IsError() || sMatchRoot.fm->state != 0) {
        return FALSE;
    }
    sMatchRoot.fm->loginCallback = (DWCLoginCallback)callback;
    sMatchRoot.fm->loginParam = param;
    if (name == NULL || name[0] == 0) {
        len = 0;
    } else {
        MIi_CpuClear16(0, sMatchRoot.fm->name, 0x34);
        if (DWCi_WStrLen(name) <= 0x19) {
            len = DWCi_WStrLen(name);
        } else {
            len = 0x19;
        }
        MIi_CpuCopy16(name, sMatchRoot.fm->name, len * 2);
    }
    sMatchRoot.fm->name[len] = 0;
    if (DWC_GetInetStatus() != 4) {
        DWCi_StopLogin(2, -60010);
        return FALSE;
    }
    func_ov011_021605b4(1);
    GSIStartAvailableCheckA(sMatchRoot.fm->gameName);
    return TRUE;
}

BOOL func_ov011_0215fe94(const char *name, void (*callback)(int error), void *userData,
                         void (*statusCallback)(int index, u8 status, const char *location, void *param),
                         void *statusParam, void (*deleteCallback)(int deleteIndex, int srcIndex, void *param),
                         void *deleteParam) {
    if (DWCi_IsError() || sMatchRoot.fm->state < 3 || DWCi_IsUpdatingServers()) {
        return FALSE;
    }
    sMatchRoot.fm->updateCallback = (DWCUpdateServersCallback)callback;
    sMatchRoot.fm->updateParam = userData;
    DWCi_UpdateServersAsync(sMatchRoot.fm->login.authToken, sMatchRoot.fm->login.partnerChallenge, func_ov011_021607a8,
                            0, statusCallback, statusParam, deleteCallback, deleteParam);
    return TRUE;
}

BOOL func_ov011_0215fef8(int type, u8 numEntry, const char *query, DWCMatchedFunc matched, void *matchedParam,
                         DWCNewClientFunc newClient, void *newClientParam, DWCEvalFunc eval, void *evalParam,
                         DWCAttemptFunc attempt, int *attemptParam, void *param) {
    if (DWCi_IsError() || sMatchRoot.fm->state != 3) {
        return FALSE;
    }
    func_ov011_0216051c();
    sMatchRoot.fm->matchedCallback = matched;
    sMatchRoot.fm->matchedParam = matchedParam;
    func_ov011_021605b4(4);
    func_ov011_02160d4c(0);
    func_ov011_021612d0(type, numEntry - 1, query, func_ov011_021607bc, NULL, newClient, newClientParam, eval,
                        evalParam, attempt, attemptParam, param);
    return TRUE;
}

BOOL func_ov011_0215ff7c(int type, u8 numEntry, DWCMatchedFunc matched, void *matchedParam, DWCNewClientFunc newClient,
                         void *newClientParam, DWCAttemptFunc attempt, int *attemptParam, void *param) {
    if (DWCi_IsError() || sMatchRoot.fm->state != 3) {
        return FALSE;
    }
    func_ov011_0216051c();
    sMatchRoot.fm->matchedCallback = matched;
    sMatchRoot.fm->matchedParam = matchedParam;
    func_ov011_021605b4(4);
    func_ov011_02160d4c(2);
    func_ov011_02161468(type, numEntry - 1, func_ov011_021607bc, NULL, newClient, newClientParam, attempt, attemptParam,
                        param);
    return TRUE;
}

BOOL func_ov011_0215fff4(int type, int friendIndex, DWCMatchedFunc matched, void *matchedParam,
                         DWCNewClientFunc newClient, void *newClientParam, DWCAttemptFunc attempt, int *attemptParam,
                         void *param) {
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
    func_ov011_0216051c();
    sMatchRoot.fm->matchedCallback = matched;
    sMatchRoot.fm->matchedParam = matchedParam;
    func_ov011_021605b4(4);
    profileId = DWCi_GetProfileIDFromList(friendIndex);
    if (profileId == 0 || gpIsBuddy(&sMatchRoot.fm->gp, profileId) == 0) {
        error = 0xb;
        goto fail;
    }
    gpGetBuddyIndex(&sMatchRoot.fm->gp, profileId, &statusIndex);
    gpGetBuddyStatus(&sMatchRoot.fm->gp, statusIndex, &status);
    if (status.status != 6) {
        error = 0xb;
        goto fail;
    }
    value[0] = '0';
    if (func_ov011_021686c0(&status) != 0) {
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
    func_ov011_02160d4c(3);
    func_ov011_0216156c(type, profileId, func_ov011_021607bc, NULL, newClient, newClientParam, attempt, attemptParam,
                        param);
    return TRUE;
fail:
    DWCi_SetError(error, 0);
    sMatchRoot.fm->matchedCallback(error, FALSE, TRUE, FALSE, 0, sMatchRoot.fm->matchedParam);
    if (sMatchRoot.fm != NULL && sMatchRoot.fm->state == 4) {
        func_ov011_021605b4(3);
        func_ov011_02162d80(1);
    }
    return TRUE;
}

BOOL func_ov011_02160150(DWCClosedFunc func, void *param) {
    if (sMatchRoot.fm == NULL) {
        return FALSE;
    }
    sMatchRoot.fm->closedCallback = func;
    sMatchRoot.fm->closedParam = param;
    return TRUE;
}

int func_ov011_02160170(void) {
    BOOL found = FALSE;
    int i;

    sMatchRoot.fm->match.unk_7b5 = TRUE;
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
        func_ov011_02168024();
        func_ov011_02162d80(1);
        func_ov011_021687ec();
        func_ov011_021605b4(3);
        func_ov011_02162908();
        func_ov011_0216800c();
        func_ov011_02167f78(0);
        func_ov011_021631a8();
        return 1;
    }
    sMatchRoot.fm->unk_2d = 1;
    gt2CloseAllConnectionsHard(sMatchRoot.fm->socket);
    while (func_ov011_02168024() > 1) {
        for (i = 0; i < func_ov011_02168024(); i++) {
            DWCNodeInfo *host = func_ov011_02168250(i);

            if (host->profileId != sMatchRoot.fm->profileId) {
                func_ov011_0216093c(NULL, 0, 1, host->aid);
            }
        }
    }
    sMatchRoot.fm->unk_2d = 0;
    func_ov011_0216800c();
    func_ov011_02167f78(0);
    func_ov011_021631a8();
    if (func_ov011_021603a4() != 3) {
        func_ov011_021687ec();
        func_ov011_021605b4(3);
        func_ov011_02162908();
    }
    return 0;
}

int func_ov011_02160284(u8 netId) {
    if (func_ov011_02160408()) {
        return func_ov011_02160710(netId);
    }
    return -3;
}

BOOL func_ov011_021602a0(void) {
    if (sMatchRoot.fm == NULL || !func_ov011_02160d88()) {
        return 0;
    }
    return func_ov011_02168024();
}

u8 func_ov011_021602c0(void) {
    DWCNodeInfo *host;

    if (sMatchRoot.fm == NULL || !func_ov011_02160d88()) {
        return 0xff;
    }
    host = func_ov011_02168270(sMatchRoot.fm->profileId);
    if (host == NULL) {
        return 0xff;
    }
    return host->aid;
}

int func_ov011_021602f0(u8 **netIds) {
    int i;

    if (sMatchRoot.fm == NULL || !func_ov011_02160d88()) {
        return 0;
    }
    MI_CpuFill8(sNetIds, 0, sizeof(sNetIds));
    for (i = 0; i < func_ov011_02168024(); i++) {
        sNetIds[i] = func_ov011_02168250(i)->aid;
    }
    *netIds = sNetIds;
    return func_ov011_02168024();
}

int func_ov011_02160344(void) {
    u8 *netIds;

    if (sMatchRoot.fm == NULL || !func_ov011_02160d88()) {
        return 0;
    }
    return func_ov011_021605e4(netIds, func_ov011_021602f0(&netIds));
}

BOOL func_ov011_02160370(u8 netId) {
    if (sMatchRoot.fm == NULL) {
        return FALSE;
    }
    if (netId == func_ov011_021602c0()) {
        return FALSE;
    }
    if (func_ov011_021682ac(netId) != NULL) {
        return TRUE;
    }
    return FALSE;
}

static int func_ov011_021603a4(void) {
    if (sMatchRoot.fm != NULL) {
        return sMatchRoot.fm->state;
    }
    return 0;
}

static u8 func_ov011_021603b8(void) {
    DWCNodeInfo *host;

    if (sMatchRoot.fm == NULL || sMatchRoot.fm->match.state == 0 || sMatchRoot.fm->match.state == 9 ||
        sMatchRoot.fm->match.state == 10 || sMatchRoot.fm->match.state == 11 || sMatchRoot.fm->match.unk_0d == 0 ||
        sMatchRoot.fm->match.hostState == 0) {
        return 0xff;
    }
    host = func_ov011_02168250(0);
    if (host == NULL) {
        return 0xff;
    }
    return host->aid;
}

BOOL func_ov011_02160408(void) {
    if (sMatchRoot.fm == NULL) {
        return FALSE;
    }
    if (func_ov011_021603b8() == 0xff) {
        return FALSE;
    }
    if (sMatchRoot.fm->match.hostState == 2) {
        return TRUE;
    }
    return FALSE;
}

int func_ov011_0216043c(void) {
    int result;

    if (sMatchRoot.fm->socket != NULL) {
        return 0;
    }
    result =
        func_ov011_0217d144(&sMatchRoot.fm->socket, func_ov011_0217ed14(0, DWCi_GetMathRand32(0x4000) + 0xc000, NULL),
                            sMatchRoot.fm->sendBufSize, sMatchRoot.fm->recvBufSize, func_ov011_02160d24);
    if (func_ov011_021606a4(result)) {
        return result;
    }
    func_ov011_0217d188(sMatchRoot.fm->socket, func_ov011_02162440);
    func_ov011_0217d3a0(sMatchRoot.fm->socket, func_ov011_021623a0);
    return result;
}

GTI2Connection *func_ov011_021604b0(u8 aid) {
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

u8 func_ov011_021604ec(GTI2Connection *connection) {
    MatchConnectionInfo *info = gt2GetConnectionData(connection);

    if (info != NULL) {
        return info->aid;
    }
    return 0xff;
}

static int func_ov011_02160500(void) {
    int i;

    for (i = 0; i < 0x20; i++) {
        if (sConnections[i] == NULL) {
            return i;
        }
    }
    return -1;
}

static void func_ov011_0216051c(void) {
    MIi_CpuClear32(0, sConnections, sizeof(sConnections));
    MIi_CpuClear32(0, sConnectionInfos, sizeof(sConnectionInfos));
}

static GTI2Connection **func_ov011_02160540(int index) {
    return &sConnections[index];
}

static GTI2Connection **func_ov011_0216054c(s32 profileId, int a1) {
    int i;

    for (i = 0; i < 0x20; i++) {
        if (sConnections[i] != NULL &&
            profileId == ((MatchConnectionInfo *)gt2GetConnectionData(sConnections[i]))->profileId) {
            return func_ov011_02160540(i);
        }
    }
    return NULL;
}

static MatchConnectionInfo *func_ov011_0216057c(int index) {
    return &sConnectionInfos[index];
}

BOOL func_ov011_02160588(u8 netId) {
    int i;

    for (i = 0; i < 0x20; i++) {
        if (sConnections[i] != NULL && netId == ((MatchConnectionInfo *)gt2GetConnectionData(sConnections[i]))->aid) {
            return TRUE;
        }
    }
    return FALSE;
}

static void func_ov011_021605b4(int state) {
    sMatchRoot.fm->prevState = sMatchRoot.fm->state;
    sMatchRoot.fm->state = state;
}

static s32 func_ov011_021605c8(int netId) {
    DWCNodeInfo *host = func_ov011_021682ac(netId);
    s32 profileId;

    if (host == NULL) {
        return 0;
    }
    profileId = host->profileId;
    func_ov011_021680e4(netId);
    return profileId;
}

static u32 func_ov011_021605e4(const u8 *netIds, int count) {
    u32 mask = 0;
    int i = 0;

    for (; i < count; i++) {
        mask |= 1 << netIds[i];
    }
    return mask;
}

static int func_ov011_02160604(int result) {
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
        if (sMatchRoot.fm->login.state < 1) {
            DWCi_StopLogin(type, code);
        }
        break;
    case 4:
        code -= 81000;
        func_ov011_02162880(type, code);
        break;
    default:
        code -= 91000;
        break;
    }
    DWCi_StopFriendProcess(type, code);
    return result;
}

static int func_ov011_021606a4(int result) {
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

int func_ov011_02160710(u8 netId) {
    DWCNodeInfo *host;
    void *conn;

    if (sMatchRoot.fm == NULL || DWCi_IsError() || (sMatchRoot.fm->state != 4 && sMatchRoot.fm->state != 5)) {
        return -1;
    }
    conn = func_ov011_021604b0(netId);
    if (conn == NULL) {
        host = func_ov011_021682ac(netId);
        if (host == NULL) {
            return -2;
        }
        func_ov011_0216093c(NULL, 0, 1, host->aid);
        return 0;
    }
    gt2CloseConnectionHard(conn);
    return 0;
}

static void func_ov011_0216076c(int error, int profileId, void *param) {
    if (error == 0) {
        sMatchRoot.fm->profileId = profileId;
        func_ov011_021605b4(3);
        DWCi_InitGPProcessCount();
    } else {
        func_ov011_021605b4(0);
    }
    if (sMatchRoot.fm->loginCallback != NULL) {
        sMatchRoot.fm->loginCallback(error, profileId, sMatchRoot.fm->loginParam);
    }
}

static void func_ov011_021607a8(int error, int count, void *param) {
    sMatchRoot.fm->updateCallback(error, count, sMatchRoot.fm->updateParam);
}

static void func_ov011_021607bc(int error, BOOL cancel, BOOL a2, BOOL a3, int index, void *param) {
    if (error == 0 && cancel) {
        if (sMatchRoot.fm->match.state == 0) {
            func_ov011_021605b4(3);
            func_ov011_02162908();
        }
    } else if (error == 0) {
        func_ov011_021605b4(5);
    }
    if (sMatchRoot.fm->match.unk_7b5 == 0) {
        sMatchRoot.fm->matchedCallback(error, cancel, a2, a3, index, sMatchRoot.fm->matchedParam);
    }
    if (error != 0 && sMatchRoot.fm != NULL && (sMatchRoot.fm->state == 4 || sMatchRoot.fm->state == 5)) {
        func_ov011_021605b4(3);
    }
}

static void func_ov011_02160844(GPConnection *connection, void *arg, void *param) {
    GPErrorArg *error = arg;

    if (error->errorCode != 0x603 && error->errorCode != 0x901 && error->errorCode != 0xb01) {
        func_ov011_02160604(3);
    }
}

static void func_ov011_02160870(GPConnection *connection, void *arg, void *param) {
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
                if (sMatchRoot.fm->state == 4 ||
                    (sMatchRoot.fm->state == 5 && (sMatchRoot.fm->match.mode == 2 || sMatchRoot.fm->match.mode == 3))) {
                    func_ov011_02162804(connection, message->profile, text + strlen("MAT"));
                }
            }
        }
    }
}

static void func_ov011_02160934(GTI2Connection *connection, const void *message, int length, BOOL reliable) {
    func_ov011_02168d40(connection, message, length, reliable);
}

static void func_ov011_0216093c(GTI2Connection *conn, int closeType, int isNetId, u8 netId) {
    int id;
    int profileId = 0;
    int errorCode;
    BOOL isHost = FALSE;
    BOOL hadHost;
    int error;
    MatchConnectionInfo *connection;
    int i;
    DWCNodeInfo *host;

    if (func_ov011_02162e6c() != 0) {
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
        hadHost = func_ov011_021682ac(id) != NULL;
        if (!isNetId) {
            func_ov011_02168fa4(id);
        }
        if (func_ov011_02168238() != NULL && id == func_ov011_02168238()->aid) {
            isHost = TRUE;
        }
        profileId = func_ov011_021605c8(id);
        sMatchRoot.fm->match.unk_1c &= ~(1 << id);
        if (sMatchRoot.fm->match.unk_48 != 0 && isHost && func_ov011_021677e8() == 0) {
            sMatchRoot.fm->match.unk_44 = 1;
        }
        if (profileId == 0) {
            profileId = ((MatchConnectionInfo *)gt2GetConnectionData(func_ov011_021604b0(id)))->profileId;
        }
        if (!isNetId) {
            sConnections[connection->index] = NULL;
        }
    }
    if (sMatchRoot.fm->match.unk_7c4 == 1 && func_ov011_02160408()) {
        sMatchRoot.fm->match.unk_7c4 = 1;
    } else if (sMatchRoot.fm->match.matchType == 3 || sMatchRoot.fm->match.matchType == 2) {
        sMatchRoot.fm->match.unk_7c4 = 0;
    } else {
        sMatchRoot.fm->match.unk_7c4 = func_ov011_021602a0() == 1;
    }
    if (func_ov011_021677e8() != 0) {
        sMatchRoot.fm->match.unk_78c = 0;
        for (i = 0; i < func_ov011_02168024();) {
            host = func_ov011_02168250(i);
            if (host->profileId != sMatchRoot.fm->match.profileId && func_ov011_021604b0(host->aid) == NULL) {
                if (sMatchRoot.fm->closedCallback != NULL) {
                    sMatchRoot.fm->closedCallback(error, closeType == 0, isHost, host->aid,
                                                  DWCi_GetFriendListIndex(host->profileId), sMatchRoot.fm->closedParam);
                }
                func_ov011_021605c8(host->aid);
            } else {
                sMatchRoot.fm->match.unk_78c |= 1 << func_ov011_02168250(i)->aid;
                i++;
            }
        }
    }
    if (closeType != 0 && func_ov011_02160408() == 1) {
        sMatchRoot.fm->match.unk_7a0 = 0;
        if (func_ov011_021686a0() != 0 && sMatchRoot.fm->match.state == 0x10 && sMatchRoot.fm->match.unk_6d8 == 8) {
            sMatchRoot.fm->match.unk_768 = 0;
        }
    }
    if (sMatchRoot.fm->unk_2d == 0 && sMatchRoot.fm->state == 5 && !hadHost) {
        if (sMatchRoot.fm->match.mode != 2) {
            return;
        }
        if (error != 0) {
            return;
        }
        func_ov011_02162d80(-1);
        func_ov011_02162d64(profileId);
        return;
    }
    if (func_ov011_02162c68(error, errorCode, profileId) != 0) {
        return;
    }
    if (error != 0) {
        DWCi_SetError(error, errorCode - 80000);
        return;
    }
    if (sMatchRoot.fm->match.mode == 2) {
        if (sMatchRoot.fm->unk_2d == 0) {
            func_ov011_02162d80(-1);
        } else if (func_ov011_02168024() == 1) {
            func_ov011_02162d80(1);
        }
    } else if (func_ov011_02168024() == 1) {
        func_ov011_02162d80(1);
    }
    if (sMatchRoot.fm->match.hostState == 2) {
        qr2_send_statechanged(sMatchRoot.fm->match.qr2);
    }
    if (sMatchRoot.fm->closedCallback != NULL && hadHost) {
        sMatchRoot.fm->closedCallback(error, closeType == 0, isHost, id, DWCi_GetFriendListIndex(profileId),
                                      sMatchRoot.fm->closedParam);
    }
    if (sMatchRoot.fm->unk_2d == 0 && sMatchRoot.fm->match.mode == 2) {
        return;
    }
    if (func_ov011_02168468() == 1) {
        func_ov011_02167f78(1);
    }
    if (func_ov011_02168024() == 1) {
        if (sMatchRoot.fm->unk_2d == 0) {
            func_ov011_02167f78(0xb);
            return;
        }
        func_ov011_021687ec();
        func_ov011_021605b4(3);
        func_ov011_02162908();
        sMatchRoot.fm->match.hostState = 0;
        return;
    }
    if (sMatchRoot.fm->match.mode == 3 && isHost && func_ov011_02168024() > 0) {
        if (func_ov011_02168024() == 1 && func_ov011_02167fd0() != 0) {
            func_ov011_02167f78(0xb);
            return;
        }
        func_ov011_02167f78(9);
    }
}

static void func_ov011_02160d10(GTI2Connection *conn, int closeType) {
    func_ov011_0216093c(conn, closeType, 0, 0xff);
}

static void func_ov011_02160d1c(GTI2Connection *connection, u32 latency) {
    func_ov011_02168d80(connection, latency);
}

static void func_ov011_02160d24(GTI2Socket *socket) {
    sMatchRoot.socketError = GOAGetLastError(gt2GetSocketSOCKET(socket));
    DWCi_SetError(9, -97003);
    sMatchRoot.fm->socket = NULL;
}

static void func_ov011_02160d4c(u8 mode) {
    sMatchRoot.fm->match.matchType = mode;
    sMatchRoot.fm->match.unk_798 = 0;
}

u8 func_ov011_02160d6c(void) {
    u8 netId = func_ov011_021603b8();
    DWCNodeInfo *host;

    if (netId == 0xff) {
        host = func_ov011_02168250(0);
        if (host != NULL) {
            netId = host->aid;
        }
    }
    return netId;
}

static BOOL func_ov011_02160d88(void) {
    BOOL result = TRUE;

    if (sMatchRoot.fm->match.matchType != 2 && sMatchRoot.fm->match.unk_0d == 0) {
        result = FALSE;
    }
    return result;
}

static BOOL func_ov011_02160dac(void) {
    BOOL result;

    if (func_ov011_02160dd8()) {
        func_ov011_0216574c();
        result = TRUE;
    } else {
        func_ov011_02167f6c();
        result = FALSE;
    }
    func_ov011_02167f6c()->unk_7b5 = 0;
    return result;
}

static BOOL func_ov011_02160dd8(void) {
    if (DWCi_IsError()) {
        return FALSE;
    }
    if (func_ov011_02167f6c() == NULL || func_ov011_021603a4() != 4 || func_ov011_02167f6c()->unk_62a != 0) {
        return FALSE;
    }
    return TRUE;
}

u8 func_ov011_02160e10(u8 keyId, const char *key, const int *valueSrc) {
    if (key == NULL || valueSrc == NULL) {
        return 0;
    }
    if (keyId >= 100 && sKeys[keyId - 100].id != 0) {
        if (sKeys[keyId - 100].name != NULL && strcmp(sKeys[keyId - 100].name, key) != 0) {
            return 0;
        }
    } else {
        keyId = func_ov011_021664c4();
        if (keyId == 0) {
            return 0;
        }
    }
    sKeys[keyId - 100].id = keyId;
    sKeys[keyId - 100].isText = 0;
    sKeys[keyId - 100].unk_02 = 0;
    if (sKeys[keyId - 100].name != NULL) {
        DWC_Free(4, sKeys[keyId - 100].name, 0);
    }
    sKeys[keyId - 100].name = DWC_Alloc(4, strlen(key) + 1);
    if (sKeys[keyId - 100].name == NULL) {
        func_ov011_02162880(9, -80001);
        return 0;
    }
    strcpy(sKeys[keyId - 100].name, key);
    sKeys[keyId - 100].value = valueSrc;
    qr2_register_keyA(keyId, sKeys[keyId - 100].name);
    return keyId;
}

u8 func_ov011_02160ed4(u8 keyId, const char *key, const char *value) {
    if (key == NULL || value == NULL) {
        return 0;
    }
    if (keyId >= 100 && sKeys[keyId - 100].id != 0) {
        if (sKeys[keyId - 100].name != NULL && strcmp(sKeys[keyId - 100].name, key) != 0) {
            return 0;
        }
    } else {
        keyId = func_ov011_021664c4();
        if (keyId == 0) {
            return 0;
        }
    }
    sKeys[keyId - 100].id = keyId;
    sKeys[keyId - 100].isText = 1;
    sKeys[keyId - 100].unk_02 = 0;
    if (sKeys[keyId - 100].name != NULL) {
        DWC_Free(4, sKeys[keyId - 100].name, 0);
    }
    sKeys[keyId - 100].name = DWC_Alloc(4, strlen(key) + 1);
    if (sKeys[keyId - 100].name == NULL) {
        func_ov011_02162880(9, -80001);
        return 0;
    }
    strcpy(sKeys[keyId - 100].name, key);
    sKeys[keyId - 100].value = value;
    qr2_register_keyA(keyId, sKeys[keyId - 100].name);
    return keyId;
}

int func_ov011_02160f98(int index, const char *key, int defaultValue) {
    void *host;

    if (func_ov011_02167f6c() == NULL || DWCi_IsError()) {
        return defaultValue;
    }
    host = ServerBrowserGetServer(func_ov011_02167f6c()->sb, index);
    if (host != NULL) {
        defaultValue = SBServerGetIntValueA(host, key, defaultValue);
    }
    return defaultValue;
}

static BOOL func_ov011_02160fd8(void) {
    int result = -1;
    u32 mask;

    if (func_ov011_02167f6c() == NULL) {
        return FALSE;
    }
    if (func_ov011_02167fc4() == 0) {
        return FALSE;
    }
    mask = func_ov011_02167f6c()->unk_1c;
    mask &= func_ov011_02160344();
    if (func_ov011_02167f0c() == 0) {
        mask &= ~(1 << func_ov011_021602c0());
    }
    if ((func_ov011_02160344() ^ mask) == 0) {
        result = 1;
    } else if ((func_ov011_02160344() & mask) == 0) {
        result = 0;
    }
    if (result != -1) {
        func_ov011_02167f6c()->suspend = result == 1;
    }
    return func_ov011_02167f6c()->suspend;
}

static void func_ov011_02161048(MatchControl *cnt, GPConnection *gp, FriendsMatchControl *fm,
                                GTI2ConnectionCallbacks *callbacks, char *gameName, char *secretKey,
                                DWCFriendData *friendList, int friendListLen) {
    func_ov011_02167f58(cnt);
    func_ov011_02167f6c()->gp = gp;
    func_ov011_02167f6c()->fm = fm;
    func_ov011_02167f6c()->callbacks = callbacks;
    func_ov011_02167f6c()->qr2 = NULL;
    func_ov011_02167f6c()->unk_5d8 = 0;
    func_ov011_02167f6c()->unk_5d4 = 0;
    func_ov011_02167f6c()->sb = NULL;
    func_ov011_02167f78(0);
    func_ov011_02167f6c()->unk_19 = 0;
    func_ov011_02167f6c()->profileId = 0;
    func_ov011_02167f6c()->gameName = gameName;
    func_ov011_02167f6c()->secretKey = secretKey;
    func_ov011_02167f6c()->friendList = friendList;
    func_ov011_02167f6c()->friendListLen = friendListLen;
    MI_CpuFill8(func_ov011_02167f6c()->friendOrder, 0, 0x40);
    func_ov011_02167f6c()->friendCount = 0;
    func_ov011_02167f6c()->notifyFunc = 0;
    func_ov011_02167f6c()->notifyParam = 0;
    func_ov011_02167f6c()->evalCallback = NULL;
    func_ov011_02167f6c()->evalParam = NULL;
    func_ov011_02167f6c()->unk_788 = 0;
    func_ov011_02167f6c()->unk_78c = 0;
    func_ov011_02167f6c()->groupId = 0;
    func_ov011_02167f6c()->unk_794 = 0;
    func_ov011_02167f6c()->unk_798 = 0;
    func_ov011_02167f6c()->matchType = 0;
    func_ov011_02167f6c()->hostState = 0;
    func_ov011_02167f6c()->unk_5f0 = 0;
    func_ov011_02167f6c()->unk_7c8 = 0;
    func_ov011_02167f6c()->unk_7d4 = 1;
    func_ov011_02167f6c()->unk_7b5 = 0;
    func_ov011_02167f6c()->unk_7b6 = 0;
    func_ov011_02167f6c()->unk_7b0 = 0;
    func_ov011_0216800c();
    func_ov011_02168374();
    func_ov011_021664f0();
    func_ov011_02162e80(0);
}

static int func_ov011_021611a8(s32 profileId) {
    int result;
    int retries;

    if (func_ov011_02167f6c()->qr2 != NULL) {
        return 0;
    }
    retries = 0;
    do {
        result =
            qr2_init_socketA(&func_ov011_02167f6c()->qr2, gt2GetSocketSOCKET(func_ov011_02167f6c()->fm->socket),
                             func_ov011_0217d38c(func_ov011_02167f6c()->fm->socket), func_ov011_02167f6c()->gameName,
                             func_ov011_02167f6c()->secretKey, 1, 1, func_ov011_021670a4, func_ov011_021671b8,
                             func_ov011_021671bc, func_ov011_021671c0, func_ov011_0216723c, func_ov011_02167240, NULL);
        if (result == 0) {
            break;
        }
        qr2_shutdown(func_ov011_02167f6c()->qr2);
        func_ov011_02167f6c()->qr2 = NULL;
        if (result != 3 || retries == 0) {
            func_ov011_0216672c(result);
            return result;
        }
        retries++;
    } while (retries < 1);
    func_ov011_02167f6c()->unk_5d8 = 0;
    func_ov011_02167f6c()->unk_5d4 = 0;
    qr2_register_publicaddress_callback(func_ov011_02167f6c()->qr2, func_ov011_02167248);
    qr2_register_natneg_callback(func_ov011_02167f6c()->qr2, func_ov011_02167264);
    qr2_register_clientmessage_callback(func_ov011_02167f6c()->qr2, func_ov011_0216738c);
    func_ov011_02168794();
    qr2_send_statechanged(func_ov011_02167f6c()->qr2);
    return result;
}

static void func_ov011_021612d0(int type, u8 numEntry, const char *query, DWCMatchedFunc matched, void *matchedParam,
                                DWCNewClientFunc newClient, void *newClientParam, DWCEvalFunc eval, void *evalParam,
                                DWCAttemptFunc attempt, int *attemptParam, void *param) {
    char buf[0x200];
    int size;

    if (sMatchState.query != NULL) {
        DWC_Free(4, sMatchState.query, 0);
        sMatchState.query = NULL;
    }
    if (query != NULL) {
        size = 0x1ff - func_ov011_021634a0(buf, -1, 0x20, 3, 3) - strlen(" and ()");
        sMatchState.query = DWC_Alloc(4, size);
        if (sMatchState.query == NULL) {
            func_ov011_02162880(9, -80001);
            return;
        }
        MI_CpuCopy8(query, sMatchState.query, size);
        *(sMatchState.query + size - 1) = 0;
    }
    func_ov011_021630f8(0, numEntry, matched, matchedParam);
    func_ov011_02167f6c()->newClientCallback = newClient;
    func_ov011_02167f6c()->newClientParam = newClientParam;
    func_ov011_02167f6c()->evalCallback = eval;
    func_ov011_02167f6c()->evalParam = evalParam;
    func_ov011_02167f6c()->unk_7b0 = type;
    func_ov011_02167838((MatchCheckFunc)attempt, attemptParam, (u32)param);
    func_ov011_02167f78(0x16);
    if (func_ov011_02167f6c()->sb == NULL) {
        func_ov011_02167f6c()->sb =
            ServerBrowserNewA(func_ov011_02167f6c()->gameName, func_ov011_02167f6c()->gameName,
                              func_ov011_02167f6c()->secretKey, 0, 0x14, 1, 0, func_ov011_0216689c, 0);
    }
    if (func_ov011_02167f6c()->sb != NULL || func_ov011_021666c4(5) == 0) {
        func_ov011_02162d80(3);
        if (func_ov011_02167f6c()->qr2 == NULL) {
            if (func_ov011_021611a8(func_ov011_02167f6c()->profileId) != 0) {
                return;
            }
        } else {
            qr2_send_statechanged(func_ov011_02167f6c()->qr2);
        }
        if (func_ov011_021666c4(func_ov011_0216322c(func_ov011_02167f6c()->profileId)) != 0) {
            return;
        }
    }
}

static void func_ov011_02161468(int type, u8 numEntry, DWCMatchedFunc matched, void *matchedParam,
                                DWCNewClientFunc newClient, void *newClientParam, DWCAttemptFunc attempt,
                                int *attemptParam, void *param) {
    func_ov011_021630f8(2, numEntry, matched, matchedParam);
    func_ov011_02167f6c()->newClientCallback = newClient;
    func_ov011_02167f6c()->newClientParam = newClientParam;
    func_ov011_02167f6c()->unk_7b0 = type;
    func_ov011_02167838((MatchCheckFunc)attempt, attemptParam, (u32)param);
    func_ov011_02167f78(0x16);
    if (func_ov011_02166670(func_ov011_02162d80(6)) == 0) {
        func_ov011_02167fa0(2);
        func_ov011_02167f6c()->unk_7c4 = 1;
        if (func_ov011_02167f6c()->qr2 == NULL) {
            if (func_ov011_021611a8(func_ov011_02167f6c()->profileId) != 0) {
                return;
            }
        }
        func_ov011_02167f6c()->groupId = (u32)func_ov011_02167810();
        if (func_ov011_02167f6c()->sb == NULL) {
            func_ov011_02167f6c()->sb =
                ServerBrowserNewA(func_ov011_02167f6c()->gameName, func_ov011_02167f6c()->gameName,
                                  func_ov011_02167f6c()->secretKey, 0, 0x14, 1, 0, func_ov011_0216689c, 0);
        }
        qr2_send_statechanged(func_ov011_02167f6c()->qr2);
        if (func_ov011_021666c4(func_ov011_0216322c(func_ov011_02167f6c()->profileId)) != 0) {
            return;
        }
    }
}

static void func_ov011_0216156c(int type, s32 profileId, DWCMatchedFunc matched, void *matchedParam,
                                DWCNewClientFunc newClient, void *newClientParam, DWCAttemptFunc attempt,
                                int *attemptParam, void *param) {
    DWCNodeInfo saved;
    DWCNodeInfo savedHost;
    DWCNodeInfo zero;
    u8 unk_62b;

    if (func_ov011_02167f6c()->unk_798 != 0) {
        saved = func_ov011_02167f6c()->self;
        savedHost = *func_ov011_02168224();
        unk_62b = func_ov011_02167f6c()->unk_62b;
    }
    func_ov011_021630f8(3, 0, matched, matchedParam);
    func_ov011_02167f6c()->unk_7b0 = type;
    if (func_ov011_02167f6c()->unk_798 == 0) {
        MI_CpuFill8(&zero, 0, sizeof(zero));
        func_ov011_02168374();
        func_ov011_02167f6c()->self.aid = 0xff;
        func_ov011_02167f6c()->self.profileId = profileId;
    } else {
        func_ov011_02167f6c()->self = saved;
        func_ov011_02168030(&savedHost);
        MI_CpuCopy8(&savedHost.unk_24, &func_ov011_02167f6c()->userData, sizeof(int));
        func_ov011_02167f6c()->unk_62b = unk_62b;
    }
    func_ov011_02167f6c()->newClientCallback = newClient;
    func_ov011_02167f6c()->newClientParam = newClientParam;
    func_ov011_02167838((MatchCheckFunc)attempt, attemptParam, (u32)param);
    func_ov011_02167f6c()->unk_18 = 1;
    func_ov011_02167f6c()->unk_5dc = func_ov011_02167f6c()->profileId;
    func_ov011_02167fa0(1);
    if (func_ov011_02167f6c()->unk_798 == 0) {
        func_ov011_02167f78(0x16);
    }
    if (func_ov011_02167f6c()->sb == NULL) {
        func_ov011_02167f6c()->sb =
            ServerBrowserNewA(func_ov011_02167f6c()->gameName, func_ov011_02167f6c()->gameName,
                              func_ov011_02167f6c()->secretKey, 0, 0x14, 1, 0, func_ov011_0216689c, 0);
    }
    if (func_ov011_02167f6c()->sb != NULL || func_ov011_021666c4(5) == 0) {
        if (func_ov011_02166670(func_ov011_02162d80(5)) == 0) {
            if (func_ov011_02167f6c()->qr2 == NULL) {
                if (func_ov011_021611a8(func_ov011_02167f6c()->profileId) != 0) {
                    return;
                }
            } else {
                qr2_send_statechanged(func_ov011_02167f6c()->qr2);
            }
            if (func_ov011_02167f6c()->unk_798 == 0) {
                if (func_ov011_021666c4(func_ov011_0216322c(func_ov011_02167f6c()->profileId)) != 0) {
                    return;
                }
                return;
            }
            qr2_send_statechanged(func_ov011_02167f6c()->qr2);
            func_ov011_02167f78(3);
            if (func_ov011_02164b78(func_ov011_02164b98(profileId, 0)) != 0) {
                return;
            }
        }
    }
}

static void func_ov011_02161788(QR2Implementation *server) {
    if (server != NULL) {
        qr2_think(server);
        if (server->userStateChangeRequested == 0) {
            switch (func_ov011_02167f6c()->mode) {
            case 0:
            case 2:
            case 3:
                func_ov011_02167f6c();
                break;
            }
        }
    }
}

// The state machine of the match, run every frame (a0 is 0 when it only has to serve the sockets)
static void func_ov011_021617b4(int a0) {
    int i;
    u32 limit;
    u32 mask;
    MatchControl *c;

    if (func_ov011_02167f6c() == NULL || DWCi_IsError() != 0) {
        return;
    }
    if (a0 == 0) {
        if (func_ov011_02167f6c()->qr2 != NULL) {
            qr2_think(func_ov011_02167f6c()->qr2);
        }
        if (func_ov011_02167f6c()->fm != NULL) {
            func_ov011_0217d168(func_ov011_02167f6c()->fm->socket);
        }
        return;
    }
    if (func_ov011_02167f6c()->state == 0) {
        return;
    }
    switch (func_ov011_02167f6c()->state) {
    case 3:
        if (func_ov011_02167f6c()->unk_644 != 0 &&
            OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_648) > func_ov011_02167f6c()->unk_644) {
            func_ov011_02167f6c()->unk_644 = 0;
            if (func_ov011_02167f6c()->unk_798 != 0) {
                func_ov011_02167f6c();
                func_ov011_02167f6c()->unk_62b++;
                if (func_ov011_02167f6c()->unk_62b > func_ov011_02167f6c()->unk_7b6) {
                    func_ov011_02162880(0x16, -80432);
                    return;
                }
                func_ov011_02164f54(0);
                return;
            }
            if (func_ov011_02168468() == 1) {
                func_ov011_02167f78(1);
                break;
            }
            if (func_ov011_02167f6c()->matchType == 3) {
                func_ov011_02167f6c();
                func_ov011_02167f6c()->unk_62b++;
                if (func_ov011_02167f6c()->unk_62b > func_ov011_02167f6c()->unk_7b6) {
                    func_ov011_02162880(0x16, -80430);
                    return;
                }
                if (func_ov011_02164b78(func_ov011_02164b98(func_ov011_02167f6c()->self.profileId, 0)) != 0) {
                    return;
                }
            } else if (func_ov011_02167f6c()->matchType == 1) {
                func_ov011_02168374();
                if (func_ov011_02164f54(0) == 0) {
                    return;
                }
            } else {
                func_ov011_02168374();
                if (func_ov011_02164f54(0) == 0) {
                    return;
                }
            }
        }
        if (func_ov011_02167f6c()->unk_638 != 0 &&
            OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_63c) >= 3000) {
            if (func_ov011_02164b78(func_ov011_02164b98(func_ov011_02167f6c()->self.profileId, 0)) != 0) {
                return;
            }
        }
        break;
    case 2:
    case 4:
    case 22:
        if (func_ov011_02167f6c()->unk_5e4 > 0) {
            if (func_ov011_02167f6c()->state == 2) {
                if (func_ov011_02167f6c()->unk_5e4 == 1 || func_ov011_02167f6c()->unk_7ac <= 1) {
                    limit = 4000;
                } else {
                    limit = 12000;
                }
            } else if (func_ov011_02167f6c()->state == 22) {
                limit = 4000;
            } else if (func_ov011_02167f6c()->unk_5e4 == 1) {
                limit = 4000;
            } else {
                limit = 12000;
            }
            if (OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_5e8) > limit) {
                if (func_ov011_021666c4(func_ov011_0216322c(func_ov011_02167f6c()->unk_67c)) != 0) {
                    return;
                }
                func_ov011_02167f6c()->unk_5e4 = 0;
            }
        }
        break;
    case 6:
        if (func_ov011_02167f6c()->unk_608 != 0) {
            if (OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_608) > 25000) {
                func_ov011_02167f6c()->unk_608 = 0;
                if (func_ov011_02168468() == 1) {
                    func_ov011_02167f78(1);
                } else if (func_ov011_02165194(func_ov011_02167f6c()->self.profileId) == 0) {
                    func_ov011_02168374();
                    return;
                }
            }
        } else if (func_ov011_02167f6c()->unk_6d8 == 6 &&
                   OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_768) > 6000) {
            if (func_ov011_02168468() == 1) {
                func_ov011_02162864();
                func_ov011_02167f78(1);
            } else {
                func_ov011_02167f6c()->unk_6d9++;
                if (func_ov011_02167f6c()->unk_6d9 > 5) {
                    func_ov011_02162864();
                    if (func_ov011_02165194(func_ov011_02167f6c()->self.profileId) == 0) {
                        func_ov011_02168374();
                        return;
                    }
                } else if (func_ov011_02164b78(
                               func_ov011_021637d8(6, func_ov011_02167f6c()->unk_760, func_ov011_02167f6c()->unk_6dc,
                                                   func_ov011_02167f6c()->unk_6da, func_ov011_02167f6c()->unk_6e0,
                                                   func_ov011_02167f6c()->unk_764)) != 0) {
                    return;
                }
            }
        }
        break;
    case 14:
        if (func_ov011_02167f6c()->unk_6d8 == 2 &&
            ((func_ov011_02167f6c()->matchType == 0 &&
              OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_768) > 6000) ||
             (func_ov011_02167f6c()->matchType != 0 &&
              OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_768) > 19000))) {
            func_ov011_02162864();
            if (func_ov011_02168468() == 1) {
                func_ov011_02167f78(1);
            } else if (func_ov011_02165164(func_ov011_02167f6c()->self.profileId) == 0) {
                return;
            }
        }
        break;
    case 16:
        if (func_ov011_02167f6c()->unk_6d8 == 8 &&
            OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_768) > 30000) {
            func_ov011_02167f6c()->unk_6d9++;
            if (func_ov011_02167f6c()->unk_6d9 != 0) {
                func_ov011_02162864();
                if (func_ov011_02165164(func_ov011_02167f6c()->self.profileId) == 0) {
                    return;
                }
            }
        }
        break;
    case 1:
        if (func_ov011_021603a4() == 4 && func_ov011_02167f6c()->unk_62f != 0 &&
            OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_670) > 30000) {
            DWCNodeInfo *self = &func_ov011_02167f6c()->self;

            if (func_ov011_02167f6c()->unk_62e >= 5) {
                if (func_ov011_02165194(self->profileId) == 0) {
                    return;
                }
            } else {
                u64 now;

                if (func_ov011_02164b78(
                        func_ov011_021637d8(0x40, self->profileId, self->publicIp, self->publicPort, NULL, 0)) != 0) {
                    return;
                }
                func_ov011_02167f6c()->unk_62e++;
                now = OS_GetTick();
                now -= OS_MilliSecondsToTicks(24000);
                func_ov011_02167f6c()->unk_670 = now;
            }
        }
        break;
    case 9:
        func_ov011_02168468();
        c = func_ov011_02167f6c();
        c->unk_788 = 0;
        c->unk_79c = 0;
        c->unk_79d = 0;
        if (func_ov011_021677e8() == 0 && func_ov011_021604b0(func_ov011_02168250(0)->aid) == NULL) {
            func_ov011_02162880(0x16, -80431);
            return;
        }
        c->unk_788 |= 1 << func_ov011_021665f0(c->profileId, 1);
        if (func_ov011_021677e8() != 0) {
            if (func_ov011_02167fd0() == 0) {
                func_ov011_02162880(0x16, -80431);
                break;
            }
            func_ov011_02167778();
            func_ov011_02167f78(10);
            qr2_send_statechanged(func_ov011_02167f6c()->qr2);
            func_ov011_02166648();
        } else {
            qr2_send_statechanged(func_ov011_02167f6c()->qr2);
            func_ov011_02167f78(10);
            func_ov011_02166648();
        }
        break;
    case 10:
        if (func_ov011_021677e8() != 0) {
            if (OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_670) > 5000) {
                func_ov011_02167778();
                if (++func_ov011_02167f6c()->unk_79c > 5) {
                    mask = ~(func_ov011_02167f6c()->unk_788 & func_ov011_02167f6c()->unk_78c);
                    for (i = 0; i < func_ov011_02168024(); i++) {
                        u8 aid = func_ov011_02168250(i)->aid;

                        if ((1 << aid) & mask) {
                            func_ov011_02160710(aid);
                        }
                    }
                    func_ov011_02167f6c()->unk_788 = func_ov011_02167f6c()->unk_78c;
                }
            }
            if ((func_ov011_02167f6c()->unk_788 ^ func_ov011_02167f6c()->unk_78c) == 0) {
                func_ov011_02167f78(11);
            }
        } else if (OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_670) > 60000) {
            func_ov011_02162880(0x16, -80431);
        }
        break;
    case 11:
        if (func_ov011_021677e8() != 0) {
            if (func_ov011_02167fd0() == 0) {
                func_ov011_02162880(0x16, -80431);
                break;
            }
            func_ov011_0216788c(0);
            func_ov011_02167f6c()->unk_7a0 = 0;
            func_ov011_02167fe8();
            func_ov011_02168620();
            qr2_send_statechanged(func_ov011_02167f6c()->qr2);
        } else if (OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_670) > 20000) {
            func_ov011_02167f78(1);
        }
        break;
    case 12:
        if (func_ov011_02167f6c()->unk_5e4 > 0) {
            if (OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_5e8) > 8000) {
                if (++func_ov011_02167f6c()->unk_794 > 0xf) {
                    func_ov011_02162880(0x16, -80432);
                    break;
                }
                if (func_ov011_021666c4(func_ov011_0216322c(0)) != 0) {
                    return;
                }
                func_ov011_02167f6c()->unk_5e4 = 0;
            }
        }
        break;
    }
    if (func_ov011_02167f6c()->state == 0xe || func_ov011_02167f6c()->state == 5) {
        if (func_ov011_02167f6c()->unk_600 != 0 &&
            OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_600) > 10000) {
            func_ov011_02167428(1, 0, NULL, &func_ov011_02167f6c()->target);
        }
    }
    if (func_ov011_021685ec() == 0 && func_ov011_02167f6c()->unk_5a4 == 0 && func_ov011_02167f6c()->state == 1 &&
        func_ov011_02167f6c()->pending.profileId == 0 && func_ov011_02168024() > 2) {
        for (i = 1; i < func_ov011_02168024(); i++) {
            DWCNodeInfo *node = func_ov011_02168250(i);

            if (func_ov011_02168224() != node && func_ov011_021604b0(node->aid) == NULL) {
                if (node->unk_1c == 0) {
                    node->unk_1c = func_ov011_021683b8();
                }
                if (node->unk_1c < OS_GetTick() && node->unk_18 <= sMatchChance[1]) {
                    node->unk_1c = func_ov011_021683b8();
                    func_ov011_02167f6c()->self = *node;
                    func_ov011_02167f6c()->unk_5a4 = 1;
                    if (func_ov011_02164b78(func_ov011_02164b98(node->profileId, 0)) != 0) {
                        func_ov011_02167f6c()->unk_5a4 = 0;
                        func_ov011_02168374();
                    } else {
                        func_ov011_021685fc();
                        func_ov011_02167f78(3);
                    }
                    break;
                }
            }
        }
    }
    if (func_ov011_02167f6c()->sb != NULL) {
        sMatchState.unk_14 = 0;
        sMatchState.unk_10 = 0;
        sMatchState.unk_00 = 1;
        ServerBrowserThink(func_ov011_02167f6c()->sb);
        if (sMatchState.unk_10 != 0) {
            ServerBrowserFree(func_ov011_02167f6c()->sb);
            func_ov011_02167f6c()->sb = NULL;
        }
        if (func_ov011_02167f6c()->sb != NULL && ServerBrowserState(func_ov011_02167f6c()->sb) != 0 &&
            func_ov011_02167f6c()->unk_5f0 != 0 && OS_GetTick() > func_ov011_02167f6c()->unk_5f0) {
            func_ov011_02162880(6, -83986);
        }
    }
    sMatchState.unk_08 = 0;
    sMatchState.unk_00 = 1;
    func_ov011_02161788(func_ov011_02167f6c()->qr2);
    NNThink();
    sMatchState.unk_00 = 0;
    if (sMatchState.unk_08 == 1) {
        sMatchState.unk_08 = 0;
        NNFreeNegotiateList();
    }
    if (func_ov011_02167f6c()->fm != NULL) {
        func_ov011_0217d168(func_ov011_02167f6c()->fm->socket);
    }
    if (func_ov011_02167f6c()->state == 0x15 &&
        OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_668) > 3000) {
        if (func_ov011_02165b54() != 0) {
            return;
        }
    }
    func_ov011_02167d6c();
    func_ov011_021679c8(0);
    if (func_ov011_02165ef4() == 0) {
        return;
    }
    if (func_ov011_02166364() == 0) {
        return;
    }
}

static inline void MatchPeer_Read(DWCNodeInfo *peer, const MatchPeerWire *wire) {
    MI_CpuFill8(peer, 0, sizeof(DWCNodeInfo));
    peer->aid = wire->packed;
    peer->unk_17 = wire->packed >> 16;
    peer->profileId = wire->id;
    peer->publicIp = wire->ip;
    peer->localIp = wire->localIp;
    peer->publicPort = wire->ports >> 16;
    peer->localPort = wire->ports;
    memcpy(&peer->unk_24, wire->unk_14, sizeof(u32));
}

static BOOL func_ov011_021623a0(GTI2Socket *socket, u32 ip, u16 port, const void *message, int length) {
    const u8 *data = message;
    SocAddrIn addr;

    if (length == 0 || data == NULL) {
        return FALSE;
    }
    MIi_CpuClear32(0, &addr, sizeof(addr));
    addr.family = 2;
    addr.port = MATCH_SWAP16(port);
    addr.addr = ip;
    if ((data[0] == 0xfe && data[1] == 0xfd) || data[0] == 0x5c) {
        if (func_ov011_02167f6c()->qr2 != NULL) {
            qr2_parse_queryA(func_ov011_02167f6c()->qr2, (char *)data, length, &addr);
        }
    } else {
        if (memcmp(data, NNMagicData, NATNEG_MAGIC_LEN) != 0) {
            if (data[0] == 0xfe) {
                return FALSE;
            }
            return FALSE;
        }
        NNProcessData((char *)data, length, &addr);
    }
    return TRUE;
}

static void func_ov011_02162440(GTI2Socket *socket, GTI2Connection *connection, u32 ip, u16 port, int latency,
                                const void *message, int length) {
    int slot;
    u32 gameId = 0;
    int half;
    GTI2Connection **slotConn;
    MatchConnectionInfo *info;

    if (func_ov011_02167f6c() != NULL && func_ov011_02167f6c()->state == 5) {
        return;
    }
    if (func_ov011_02167f6c() == NULL || func_ov011_02167f6c()->state != 6 || func_ov011_02167f6c()->unk_62a != 0) {
        func_ov011_0217d198(connection, "Init state", -1);
        return;
    }
    if (message != NULL && length != 0) {
        gameId = strtoul(message, NULL, 10);
    }
    slot = func_ov011_02160500();
    if (slot == -1) {
        func_ov011_0217d198(connection, "Server full", -1);
        if (func_ov011_02168468() == 1) {
            func_ov011_02162864();
            func_ov011_02167f78(1);
            return;
        }
        func_ov011_02162880(6, -87100);
        return;
    }
    if (ip != func_ov011_02167f6c()->self.hostIp || MATCH_SWAP16(port) != func_ov011_02167f6c()->self.hostPort) {
        if (gameId == func_ov011_02167f6c()->self.profileId) {
            func_ov011_02167f6c()->self.hostIp = ip;
            func_ov011_02167f6c()->self.hostPort = MATCH_SWAP16(port);
        } else {
            func_ov011_0217d198(connection, "Unknown connect attempt", -1);
            return;
        }
    }
    func_ov011_02167f6c()->unk_608 = 0;
    if (!func_ov011_0217d190(connection, func_ov011_02167f6c()->callbacks)) {
        func_ov011_02162880(6, -80410);
        return;
    }
    func_ov011_02162864();
    if (func_ov011_02167f6c()->unk_5a4 == 0 && func_ov011_02168024() == 1) {
        half = latency >> 1;
        if (half >= 0xffff) {
            half = 0xffff;
        }
        func_ov011_02167f6c()->unk_630 = half;
    }
    slotConn = func_ov011_02160540(slot);
    info = func_ov011_0216057c(slot);
    *slotConn = connection;
    info->index = slot;
    info->aid = func_ov011_02167f6c()->self.aid;
    info->profileId = func_ov011_02167f6c()->self.profileId;
    func_ov011_02169000(info->aid);
    gt2SetConnectionData(connection, info);
    func_ov011_02165280(2);
}

static void func_ov011_0216261c(char *dest, u32 size) {
    OS_SNPrintf(dest, size, "%u", func_ov011_02167f6c()->profileId);
}

static void func_ov011_02162640(GTI2Connection *connection, int result, const void *message, int length) {
    char text[0x18];
    MatchControl *work;
    const char *address;
    int status;
    int slot;
    GTI2Connection **slotConn;
    MatchConnectionInfo *info;

    if (!(((func_ov011_02167f6c() != NULL &&
            (func_ov011_02167f6c()->state == 6 || func_ov011_02167f6c()->state == 0xf)) ||
           func_ov011_02167f6c()->unk_5a8 == 0xf))) {
        return;
    }
    if (result != 0) {
        if (func_ov011_02168468() == 1) {
            func_ov011_02167f78(1);
            return;
        }
        if (func_ov011_02167f6c()->unk_5a8 == 0xf) {
            func_ov011_021685fc();
            return;
        }
        if (result == 6) {
            func_ov011_02167f6c()->retries++;
            if (func_ov011_02167f6c()->retries > 5) {
                func_ov011_02167f6c()->retries = 0;
                func_ov011_02165164(func_ov011_02167f6c()->self.profileId);
                return;
            }
            func_ov011_0216261c(text, 0x18);
            work = func_ov011_02167f6c();
            address =
                func_ov011_0217ed14(func_ov011_02167f6c()->self.hostIp, func_ov011_02167f6c()->self.hostPort, NULL);
            status = func_ov011_0217d1a0(work->fm->socket, NULL, address, text, -1, 5000,
                                         func_ov011_02167f6c()->callbacks, FALSE);
            if (status == 1) {
                func_ov011_02166830(status);
                return;
            }
            if (status != 0) {
                func_ov011_02165164(func_ov011_02167f6c()->self.profileId);
            }
        } else if (result != 5) {
            func_ov011_02165164(func_ov011_02167f6c()->self.profileId);
        }
        return;
    }
    slot = func_ov011_02160500();
    if (slot == -1) {
        func_ov011_02162880(6, -87100);
        return;
    }
    slotConn = func_ov011_02160540(slot);
    info = func_ov011_0216057c(slot);
    *slotConn = connection;
    info->index = slot;
    if (func_ov011_02167f6c()->unk_5a8 != 0xf) {
        info->aid = func_ov011_02167f6c()->self.aid;
        info->profileId = func_ov011_02167f6c()->self.profileId;
    } else {
        info->aid = func_ov011_02167f6c()->unk_5ac.aid;
        info->profileId = func_ov011_02167f6c()->unk_5ac.profileId;
    }
    func_ov011_02169000(info->aid);
    gt2SetConnectionData(connection, info);
    if (func_ov011_02167f6c()->state == 0xf || func_ov011_02167f6c()->unk_5a8 == 0xf) {
        func_ov011_02165280(0);
    } else {
        func_ov011_02165280(1);
    }
}

static void func_ov011_02162804(void *unused, int id, const u8 *str) {
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
    func_ov011_02163a30(str[0], id, 0, 0, (const u32 *)buf, count);
}

static void func_ov011_02162864(void) {
    func_ov011_02167f6c()->unk_6d8 = 0xff;
    func_ov011_02167f6c()->unk_6d9 = 0;
}

static void func_ov011_02162880(int type, int code) {
    int flag;
    MatchControl *work;
    BOOL isHost;

    if (func_ov011_02167f6c() != NULL && type != 0) {
        func_ov011_02165bc8();
        DWCi_SetError(type, code);
        flag = 1;
        func_ov011_02162d80(1);
        if (func_ov011_02167f6c()->unk_680 != 0) {
            flag = 0;
        }
        work = func_ov011_02167f6c();
        isHost = func_ov011_02167f6c()->mode == 2;
        work->notifyFunc(type, 0, flag, isHost, DWCi_GetFriendListIndex(func_ov011_02167f6c()->unk_680),
                         func_ov011_02167f6c()->notifyParam);
        func_ov011_021631a8();
    }
}

static void func_ov011_02162908(void) {
    if (func_ov011_021603a4() == 3) {
        func_ov011_02167f6c()->unk_17 = 0;
        func_ov011_02167f6c()->groupId = 0;
        qr2_send_statechanged(func_ov011_02167f6c()->qr2);
    }
}

void func_ov011_02162930(u8 aid, int type, const void *data) {
    if (func_ov011_021686a0()) {
        func_ov011_02162bf8(aid, type, data);
    } else {
        func_ov011_02162958(aid, type, data);
    }
}

static void func_ov011_02162958(int conn, int kind, const u32 *msg) {
    int i;
    int count;
    int duration;
    u32 value;
    u32 nextId;
    u64 now;

    switch (kind) {
    case 2:
        if (func_ov011_02167f6c()->state == 1) {
            if (msg[0] == 1) {
                func_ov011_02167f6c()->unk_680 = 0;
                func_ov011_02167f6c()->pending.profileId = 0;
                value = msg[1];
                func_ov011_02167f6c()->unk_7c8 = value;
                if (func_ov011_021686a0()) {
                    func_ov011_02168030(&func_ov011_02167f6c()->self);
                }
            } else {
                DWCNodeInfo client;

                MatchPeer_Read(&client, (const MatchPeerWire *)&msg[1]);
                func_ov011_02167f6c()->pending = client;
                value = msg[7];
                func_ov011_02167f6c()->unk_7c8 = value;
                func_ov011_02167f6c()->unk_680 = client.profileId;
            }
            if (func_ov011_02168024() == 1) {
                duration = 90000;
            } else {
                duration = func_ov011_02168024() * 12000;
            }
            func_ov011_02167f6c()->unk_670 = duration;
            func_ov011_02167f6c()->unk_62e = 0;
            func_ov011_02167f78(8);
            func_ov011_02165d2c(conn, 3);
        }
        break;
    case 3:
        if (func_ov011_02167f6c()->state == 0x13) {
            value = ((const u8 *)msg)[0] | (((const u8 *)msg)[1] << 8);
            func_ov011_02167f6c()->unk_650 |= 1 << conn;
            if (value > func_ov011_02167f6c()->unk_630) {
                func_ov011_02167f6c()->unk_630 = value;
            }
            if (func_ov011_02166604(0) == func_ov011_02167f6c()->unk_650) {
                nextId = DWCi_GetMathRand32(0xfffffffe);
                func_ov011_02167f6c()->unk_7c8 = nextId + 1;
                func_ov011_021660f8();
                func_ov011_02167f6c()->unk_62c = 0;
                now = OS_GetTick();
                func_ov011_02167f6c()->unk_658 = now;
            }
        } else {
            func_ov011_02165d2c(conn, 4);
        }
        break;
    case 4:
        if (func_ov011_02167f6c()->state == 8) {
            count = msg[0];
            if (count != 0) {
                for (i = 0; i < count; i++) {
                    DWCNodeInfo peer;

                    MatchPeer_Read(&peer, (const MatchPeerWire *)&msg[1 + (count - 1 - i) * 6]);
                    func_ov011_02168030(&peer);
                }
                value = msg[1 + count * 6];
                func_ov011_02167f6c()->unk_7c8 = value;
                func_ov011_02168030(&func_ov011_02167f6c()->self);
                func_ov011_02168374();
            } else {
                value = msg[1];
                func_ov011_02167f6c()->unk_7c8 = value;
            }
            func_ov011_02165280(3);
        }
        break;
    }
}

static void func_ov011_02162bf8(int conn, int kind, const u32 *msg) {
    func_ov011_02162958(conn, kind, msg);
}

static void func_ov011_02162c00(u32 mask) {
    u32 current = func_ov011_02160344();
    u32 diff = current & (current ^ mask);
    u8 i;

    if (diff != 0) {
        for (i = 0; i < 32; i++) {
            if (diff & (1 << i)) {
                func_ov011_02160710(i);
            }
        }
    }
}

static void func_ov011_02162c30(const u32 *msg) {
    u32 first = msg[0];
    u32 mask = msg[1];
    u32 id = msg[2];
    DWCNodeInfo *peer;

    if (func_ov011_02160408() == 0 && id == func_ov011_02167f6c()->unk_7c8) {
        peer = func_ov011_02168238();
        if (peer != NULL && peer->profileId == first) {
            func_ov011_02162c00(mask);
        }
    }
}

static BOOL func_ov011_02162c68(int type, u32 code, s32 profileId) {
    u64 now;

    if (func_ov011_021603a4() != 4) {
        return FALSE;
    }
    if (func_ov011_02167f6c()->mode == 2) {
        return TRUE;
    }
    if (type != 0) {
        func_ov011_02162880(type, code - 80000);
        return TRUE;
    }
    if (func_ov011_02167f6c()->unk_62a == 1 || func_ov011_02167f6c()->unk_629 == 1) {
        return TRUE;
    }
    if (func_ov011_02167f6c()->target.cookie != 0) {
        NNCancel(func_ov011_02167f6c()->target.cookie);
        func_ov011_02167f6c()->target.cookie = 0;
    }
    if (func_ov011_02168024() > 1) {
        if (func_ov011_02167f6c()->unk_629 == 0) {
            func_ov011_02167f6c()->unk_629 = 2;
            gt2CloseAllConnectionsHard(func_ov011_02167f6c()->fm->socket);
        }
    } else if (func_ov011_02167f6c()->matchType == 3) {
        func_ov011_02162880(0x16, -80430);
    } else if (func_ov011_02167f6c()->state == 1) {
        func_ov011_02167f78(0x15);
        now = OS_GetTick();
        func_ov011_02167f6c()->unk_668 = now;
    } else {
        func_ov011_021659ac(1);
    }
    return TRUE;
}

static void func_ov011_02162d64(u32 id) {
    if (func_ov011_02167f6c()->unk_629 != 1) {
        func_ov011_02165164(id);
    }
}

int func_ov011_02162d80(int status) {
    char number[12];
    char text[0x27];
    int players;

    OS_SNPrintf(number, 12, "%u", func_ov011_02167f6c()->unk_17 + 1);
    DWC_SetCommonKeyValueString("SCM", number, text, '/');
    players = func_ov011_02168024();
    if (func_ov011_02167f6c()->matchType == 2 && players == 0) {
        players = 1;
    }
    OS_SNPrintf(number, 12, "%u", players);
    DWC_AddCommonKeyValueString("SCN", number, text, '/');
    OS_SNPrintf(number, 12, "%u", 0x5a);
    DWC_AddCommonKeyValueString("VER", number, text, '/');
    OS_SNPrintf(number, 12, "%u", func_ov011_021685a0() != 0);
    DWC_AddCommonKeyValueString("LCK", number, text, '/');
    return DWCi_SetGPStatus(status, text, NULL);
}

static void func_ov011_02162e44(void) {
    func_ov011_02167f58(0);
    if (sMatchState.query != NULL) {
        DWC_Free(4, sMatchState.query, 0);
        sMatchState.query = NULL;
    }
    func_ov011_021664f0();
}

static BOOL func_ov011_02162e6c(void) {
    if (func_ov011_02167f6c() == NULL) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov011_02162e80(int mode) {
    MatchControl *work;

    func_ov011_02167f6c()->retries = 0;
    func_ov011_02167f6c()->unk_5f8 = 0;
    work = func_ov011_02167f6c();
    work->unk_5fa = DWCi_GetMathRand32(0x10000);
    func_ov011_02167f6c()->unk_5fc = 0;
    func_ov011_02167f6c()->unk_600 = 0;
    func_ov011_02167f6c()->unk_608 = 0;
    func_ov011_02167f6c()->unk_620 = 0;
    func_ov011_02167f6c()->unk_62a = 0;
    func_ov011_02167f6c()->unk_62b = 0;
    func_ov011_02167f6c()->unk_62c = 0;
    func_ov011_02167f6c()->unk_62d = 0;
    func_ov011_02167f6c()->unk_628 = 0;
    func_ov011_02167f6c()->unk_629 = 0;
    func_ov011_02167f6c()->unk_632 = 0;
    func_ov011_02167f6c()->unk_658 = 0;
    func_ov011_02167f6c()->unk_668 = 0;
    MIi_CpuClear32(0, &func_ov011_02167f6c()->unk_6d8, 0x98);
    func_ov011_02167f6c()->unk_5a4 = 0;
    func_ov011_021685fc();
    if (mode == 2) {
        if (func_ov011_02167fc4() == 2) {
            func_ov011_02167f78(0xd);
        } else if (func_ov011_02167fc4() == 1) {
            func_ov011_02167f78(1);
        }
        return;
    }
    func_ov011_02167f6c()->unk_18 = 0;
    func_ov011_02167f6c()->unk_5dc = 0;
    func_ov011_02167f6c()->unk_5e4 = 0;
    func_ov011_02167f6c()->unk_630 = 0;
    func_ov011_02167f6c()->unk_621 = 0;
    func_ov011_02167f6c()->unk_638 = 0;
    func_ov011_02167f6c()->unk_63c = 0;
    func_ov011_02167f6c()->unk_644 = 0;
    func_ov011_02167f6c()->unk_648 = 0;
    func_ov011_02167f6c()->unk_650 = 0;
    func_ov011_02167f6c()->unk_67c = 0;
    MIi_CpuClear32(0, &func_ov011_02167f6c()->target, 0xc);
    func_ov011_02167f6c()->unk_788 = 0;
    func_ov011_02167f6c()->unk_78c = 0;
    func_ov011_02167f6c()->unk_794 = 0;
    func_ov011_02167f6c()->unk_7a8 = 1;
    func_ov011_02167f6c()->unk_7ac = 0;
    func_ov011_02167f6c()->hostState = 0;
    func_ov011_02167f6c()->unk_0d = 0;
    func_ov011_02167f6c()->checkFunc = 0;
    func_ov011_02167f6c()->unk_7c0 = 0;
    func_ov011_0216800c();
    func_ov011_02168374();
    func_ov011_02167f6c()->pending.profileId = 0;
    if (mode == 1) {
        if (func_ov011_02167f6c()->matchType == 0 || func_ov011_02167f6c()->matchType == 1) {
            func_ov011_02167f78(0x16);
        }
        return;
    }
    func_ov011_02167f6c()->mode = 0;
    func_ov011_02167f6c()->unk_17 = 0;
    func_ov011_02167f6c()->unk_4c = 0;
    func_ov011_02167f6c()->unk_680 = 0;
    func_ov011_02167f6c()->unk_62e = 0;
    func_ov011_02167f6c()->unk_670 = 0;
    func_ov011_02167f6c()->newClientCallback = 0;
    func_ov011_02167f6c()->newClientParam = 0;
    MI_CpuFill8(&func_ov011_02167f6c()->userData, 0, sizeof(int));
}

static void func_ov011_021630f8(u8 mode, u8 unk17, DWCMatchedFunc callback, void *param) {
    func_ov011_02162e80(0);
    func_ov011_02167f6c()->mode = mode;
    func_ov011_02167f6c()->unk_17 = unk17;
    func_ov011_02167f6c()->notifyFunc = callback;
    func_ov011_02167f6c()->notifyParam = param;
    func_ov011_02167f6c()->unk_5f9 = 0;
    func_ov011_02167f6c()->unk_2c = 0xff;
    func_ov011_02167f6c()->unk_30 = 0;
    func_ov011_02167f6c()->hostState = 0;
    func_ov011_02167f6c()->unk_7a8 = 1;
    func_ov011_02167f6c()->unk_7ac = 0;
    func_ov011_02167f6c()->unk_17 = unk17;
    func_ov011_02167f6c()->unk_1c = 0;
    func_ov011_02167f6c()->suspend = 0;
    func_ov011_02167f6c()->unk_20 = 0;
    func_ov011_02167f6c()->unk_48 = 0;
    func_ov011_02167f6c()->unk_7c4 = 1;
    func_ov011_02167f6c()->unk_7cc = 0;
    func_ov011_02167f6c()->unk_634 = 0;
}

static void func_ov011_021631a8(void) {
    if (func_ov011_02167f6c() != NULL) {
        if (func_ov011_02167f6c()->sb != 0) {
            if (sMatchState.unk_14 == 0) {
                ServerBrowserFree(func_ov011_02167f6c()->sb);
                func_ov011_02167f6c()->sb = 0;
            } else {
                sMatchState.unk_10 = 1;
            }
        }
        func_ov011_021687ec();
        func_ov011_02167f78(0);
        if (sMatchState.query != NULL) {
            DWC_Free(4, sMatchState.query, 0);
            sMatchState.query = NULL;
        }
        func_ov011_021664f0();
        func_ov011_02167f6c()->unk_4c = 1;
        func_ov011_0216800c();
        func_ov011_02168374();
        func_ov011_02167f6c()->unk_7c4 = 1;
        func_ov011_02162d80(1);
    }
}

static int func_ov011_0216322c(u32 pid) {
    int flag = 1;
    char text[0x200];
    u8 list[0xcc];
    int i;
    int count;
    u8 roll;
    u64 now;
    int result;
    int len;

    list[0] = 8;
    list[1] = 10;
    list[2] = 50;
    list[3] = 51;
    list[4] = 52;
    list[5] = 53;
    list[6] = 54;
    list[7] = 55;
    list[8] = 56;
    count = 9;
    if (func_ov011_02167f6c()->matchType == 0 || func_ov011_02167f6c()->matchType == 1) {
        for (i = 0; i < 0x9a; i++) {
            if (sKeys[i].id != 0) {
                list[count] = sKeys[i].id;
                count++;
            }
        }
    }
    switch (func_ov011_02167f6c()->state) {
    case 2:
        if (sMatchChance[0] == -2) {
            roll = func_ov011_02167f6c()->unk_7a8 == 1 ? 2 : 3;
            func_ov011_02167f6c()->unk_7a8 = 0;
        } else if (sMatchChance[0] == -1) {
            roll = func_ov011_02167f6c()->unk_7a8 != 0 ? 2 : 0;
            func_ov011_02167f6c()->unk_7a8 ^= 1;
        } else {
            roll = DWCi_GetMathRand32(100) < sMatchChance[0] ? 2 : 0;
        }
        len = func_ov011_021634a0(text, func_ov011_02167f6c()->profileId, func_ov011_02167f6c()->unk_17,
                                  func_ov011_02167f6c()->matchType, roll);
        if (sMatchState.query != NULL) {
            OS_SNPrintf(text + len, 0x1ff - len, " and (%s)", sMatchState.query);
            text[0x1fe] = 0;
        }
        func_ov011_02167f6c()->unk_7ac++;
        break;
    case 3:
    case 4:
    case 0x16:
        OS_SNPrintf(text, 0x1ff, "%s = %u", "dwc_pid", pid);
        func_ov011_02167f6c()->unk_67c = pid;
        break;
    case 9:
    case 10:
    case 11:
        OS_SNPrintf(text, 0x1ff, "%s = %u", "dwc_pid", pid);
        flag = 0;
        break;
    case 12:
        OS_SNPrintf(text, 0x1ff, "(%s = %u) and (%s = %u) and (%s != %u)", "dwc_groupid",
                    func_ov011_02167f6c()->groupId, "dwc_hoststate", 2, "dwc_pid", func_ov011_02167f6c()->profileId);
        break;
    }
    ServerBrowserClear(func_ov011_02167f6c()->sb);
    for (i = 0; i < 1; i++) {
        result = ServerBrowserLimitUpdateA(func_ov011_02167f6c()->sb, flag, 0, list, count, text,
                                           func_ov011_02167f6c()->state == 0x16 ? 1 : 6);
        if (result == 0 || result != 2) {
            break;
        }
    }
    if (result == 0) {
        now = OS_GetTick();
        func_ov011_02167f6c()->unk_5f0 = now + OS_MilliSecondsToTicks(30000);
    }
    return result;
}

static int func_ov011_021634a0(char *out, u32 a, u32 b, u32 c, u8 kind) {
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

static BOOL func_ov011_02163534(u32 ip) {
    u32 swapped =
        ((ip & 0xff000000) >> 24) | ((ip & 0x00ff0000) >> 8) | ((ip & 0x0000ff00) << 8) | ((ip & 0x000000ff) << 24);
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

static int func_ov011_021635a0(int reset, u32 token, DWCNodeInfo *peer) {
    int mismatch;
    int result = 0;
    u32 hostInfo[2];

    NNCancel(func_ov011_02167f6c()->target.cookie);
    if (reset == 0) {
        token = (func_ov011_02167f6c()->unk_5fa << 16) | (u16)func_ov011_02167f6c()->profileId;
        if (peer->unk_17 != 0) {
            if (peer->publicIp != ServerBrowserGetMyPublicIPAddr(func_ov011_02167f6c()->sb)) {
                mismatch = 1;
            } else {
                peer->hostIp = peer->localIp;
                peer->hostPort = peer->localPort;
                mismatch = 0;
            }
        } else if (func_ov011_02163534(SOC_GetHostID()) != 0) {
            mismatch = 1;
        } else {
            peer->hostIp = peer->publicIp;
            peer->hostPort = peer->publicPort;
            mismatch = 0;
        }
        if (mismatch) {
            func_ov011_02167f6c()->unk_5fa = DWCi_GetMathRand32(0x10000);
            func_ov011_02167f6c()->target.cookie = token;
        } else {
            hostInfo[0] = SOC_GetHostID();
            hostInfo[1] = func_ov011_0217d38c(func_ov011_02167f6c()->fm->socket);
            int sent = func_ov011_021637d8(6, peer->profileId, peer->publicIp, peer->publicPort, hostInfo, 2);
            func_ov011_02167f6c()->unk_6d9 = 0;
            if (sent != 0) {
                return 2;
            }
            func_ov011_02167f6c()->target.cookie = 0;
        }
        func_ov011_02167f6c()->target.kind = 0;
        func_ov011_02167f6c()->target.unk_01 = 0;
        func_ov011_02167f6c()->target.port = peer->publicPort;
        func_ov011_02167f6c()->target.ip = peer->publicIp;
    } else {
        mismatch = 1;
        func_ov011_02167f6c()->target.kind = 1;
        func_ov011_02167f6c()->target.unk_01 = 0;
        func_ov011_02167f6c()->target.port = 0;
        func_ov011_02167f6c()->target.ip = 0;
        func_ov011_02167f6c()->target.cookie = token;
    }
    if (mismatch) {
        result = func_ov011_0216373c(&func_ov011_02167f6c()->target);
    } else {
        u32 sock = gt2GetSocketSOCKET(func_ov011_02167f6c()->fm->socket);

        func_ov011_02167428(0, sock, 0, &func_ov011_02167f6c()->target);
        func_ov011_02167f6c()->unk_608 = 0;
    }
    return result;
}

static int func_ov011_0216373c(MatchTarget *target) {
    int result;
    int i;

    if (target->kind == 0) {
        MatchControl *work = func_ov011_02167f6c();
        if (func_ov011_021666c4(ServerBrowserSendNatNegotiateCookieToServerA(
                work->sb, func_ov011_0217ed14(target->ip, 0, 0), target->port, target->cookie)) != 0) {
            return 2;
        }
    }
    for (i = 0; i < 1; i++) {
        result = NNBeginNegotiationWithSocket(gt2GetSocketSOCKET(func_ov011_02167f6c()->fm->socket), target->cookie,
                                              target->kind, func_ov011_02167424, func_ov011_02167428, target);
        if (result == 0 || result != 3) {
            break;
        }
    }
    return result;
}

static BOOL func_ov011_021637b0(int type, int id) {
    BOOL result = TRUE;

    if (func_ov011_0216054c(id, 1) == 0) {
        return FALSE;
    }
    if (func_ov011_02168270(id) == NULL) {
        result = FALSE;
    }
    return result;
}

static int func_ov011_021637d8(int type, int id, u32 ip, u16 port, const u32 *data, int count) {
    int result = 0;
    MatchPacket packet;
    char text[0x200];
    DWCNodeInfo *peer;
    int length;
    u64 now;

    if (func_ov011_021637b0(type, id)) {
        peer = func_ov011_02168270(id);
        func_ov011_0216390c(&packet, type, data, count);
        func_ov011_021690a4(func_ov011_021602c0(), peer->aid, &packet, packet.header.size + sizeof(MatchPacketHeader),
                            FALSE);
    } else if (gpIsBuddy(func_ov011_02167f6c()->gp, id)) {
        length = 0;
        if (data != NULL && count != 0) {
            length = DWC_Base64Encode((const char *)data, count * 4, text, 0x200);
        }
        text[length] = 0;
        result = func_ov011_021639c8(func_ov011_02167f6c()->gp, type, id, text);
    } else {
        if (ip == 0) {
            peer = func_ov011_02168270(id);
            ip = peer->publicIp;
            port = peer->publicPort;
        }
        result = func_ov011_02163974(type, ip, port, data, count);
    }
    if (type == 2 || type == 6 || (u8)(type - 8) <= 1) {
        func_ov011_02167f6c()->unk_6d8 = type;
        func_ov011_02167f6c()->unk_6da = port;
        func_ov011_02167f6c()->unk_6dc = ip;
        func_ov011_02167f6c()->unk_760 = id;
        func_ov011_02167f6c()->unk_764 = count;
        now = OS_GetTick();
        func_ov011_02167f6c()->unk_768 = now;
        if (data != NULL && count != 0) {
            MIi_CpuCopy32(data, func_ov011_02167f6c()->unk_6e0, count * 4);
        }
    }
    return result;
}

static void func_ov011_0216390c(MatchPacket *packet, int type, const u32 *data, int count) {
    if (data != NULL && count != 0) {
        MIi_CpuCopy32(data, packet->data, count * 4);
    } else {
        count = 0;
    }
    strcpy(packet->header.magic, "\xbbI\xccM");
    packet->header.version = 0x5a;
    packet->header.command = type;
    packet->header.size = count * 4;
    packet->header.port = func_ov011_02167f6c()->unk_5d4;
    packet->header.ip = func_ov011_02167f6c()->unk_5d8;
    packet->header.profileId = func_ov011_02167f6c()->profileId;
}

static int func_ov011_02163974(int type, u32 ip, u16 port, const u32 *data, int count) {
    MatchPacket packet;
    int result;
    int i;

    func_ov011_0216390c(&packet, type, data, count);
    for (i = 0; i < 1; i++) {
        result =
            ServerBrowserSendMessageToServerA(func_ov011_02167f6c()->sb, func_ov011_0217ed14(ip, 0, NULL), port,
                                              (const char *)&packet, packet.header.size + sizeof(MatchPacketHeader));
        if (result == 0 || result != 2) {
            break;
        }
    }
    return result;
}

static int func_ov011_021639c8(void *gp, u8 command, int id, const char *text) {
    char buffer[0x200];
    int length;
    char *end;

    length = OS_SNPrintf(buffer, 0x200, "%s%dv%s", "GPCM", 0x5a, "MAT", id);
    buffer[length++] = command;
    end = buffer + length;
    *end = 0;
    if (text != NULL) {
        int textLength = strlen(text);

        MI_CpuCopy8(text, end, textLength);
        end[textLength] = 0;
    }
    return gpSendBuddyMessage(gp, id, buffer);
}

static int func_ov011_02163a30(int type, s32 profileId, u32 ip, u32 port, const u32 *data, int count) {
    int n = 0;
    u32 reply[0x40];
    DWCNodeInfo request;
    DWCNodeInfo accepted;
    SocAddrIn address;
    u32 sendId[1];
    int result;
    u8 saved;
    u32 requestType;
    u32 requestExtra;
    u32 acceptedExtra;
    u32 acceptedUnk24;
    u32 joinId;
    u8 joinAid;
    int blocked;
    DWCNodeInfo *peer;
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

    if (func_ov011_02167f6c() == NULL || func_ov011_02167f6c()->state == 0) {
        return 1;
    }
    if (func_ov011_021603a4() == 4 && func_ov011_02167f6c()->state == 1 && func_ov011_02168024() > 0 &&
        profileId == func_ov011_02168250(0)->profileId) {
        func_ov011_02166648();
    }
    switch (type) {
    case 1: {
        blocked = 0;
        requestType = data[0];
        peer = &request;
        MI_CpuFill8(peer, 0, sizeof(DWCNodeInfo));
        peer->profileId = profileId;
        peer->aid = 0xff;
        peer->publicIp = data[1];
        peer->publicPort = data[2];
        peer->localIp = data[3];
        peer->localPort = data[4];
        peer->unk_17 = data[5];
        requestSkip = data[6];
        requestCookie = data[7];
        memcpy(&peer->unk_24, &data[8], sizeof(u32));
        requestExtra = data[9];
        if (count >= 11 && data[10] == 1) {
            blocked = 1;
        }
        result = func_ov011_0216475c(peer->profileId, peer->publicIp, peer->publicPort, requestType, requestSkip != 0,
                                     requestCookie, &peer->unk_24, blocked);
        if (result == 2) {
            func_ov011_02168374();
            func_ov011_02167f6c()->self = *peer;
            if (func_ov011_02164b78(func_ov011_02164aa0(profileId, ip, port)) != 0) {
                return 0;
            }
            if (func_ov011_02167f6c()->unk_5a4 != 0) {
                func_ov011_02167f6c()->self.aid = func_ov011_02168270(func_ov011_02167f6c()->self.profileId)->aid;
            }
            if (func_ov011_02167f6c()->unk_5a4 == 0 && func_ov011_02167f6c()->mode == 2 &&
                func_ov011_02167f6c()->state != 0xe &&
                ((func_ov011_02167f6c()->matchType != 0 && func_ov011_02167f6c()->matchType != 1) ||
                 func_ov011_02167f6c()->unk_0d != 0) &&
                func_ov011_02167f6c()->newClientCallback != NULL) {
                func_ov011_02167f6c()->newClientCallback(DWCi_GetFriendListIndex(profileId),
                                                         func_ov011_02167f6c()->newClientParam);
            }
            host = func_ov011_02168224();
            n = 0;
            reply[n++] = func_ov011_02167f6c()->unk_17;
            reply[n++] = host->aid;
            reply[n++] = host->profileId;
            reply[n++] = host->publicIp;
            reply[n++] = host->publicPort;
            reply[n++] = host->localIp;
            reply[n++] = host->localPort;
            reply[n++] = host->unk_17;
            reply[n++] = host->unk_24;
            reply[n++] = func_ov011_02167f6c()->groupId;
            reply[n++] = func_ov011_02167f6c()->self.aid;
            reply[n++] = requestExtra;
            reply[n++] = func_ov011_02168024();
            reply[n++] = func_ov011_02167f6c()->unk_5a4 != 0 ? 1 : 0;
            func_ov011_02167f6c();
            func_ov011_02167f78(0xe);
        } else if (result == 3) {
            func_ov011_021686f8(func_ov011_02167f6c()->error);
            reply[0] = func_ov011_02167f6c()->error;
            n = 1;
        }
        if (result != 0xff) {
            if (func_ov011_02164b78(
                    func_ov011_021637d8(result, peer->profileId, peer->publicIp, peer->publicPort, reply, n)) != 0) {
                return 0;
            }
        }
        break;
    }
    case 2: {
        acceptedExtra = data[0];
        if (func_ov011_02167f6c()->state != 3 || profileId != func_ov011_02167f6c()->unk_67c || profileId != data[2] ||
            func_ov011_02167f6c()->unk_634 != data[11]) {
            break;
        }
        if (count >= 14 && (data[13] == 1) != func_ov011_02167f6c()->unk_5a4) {
            break;
        }
        func_ov011_02167f6c()->unk_628 = 0;
        func_ov011_02167f6c()->unk_644 = 0;
        func_ov011_02167f6c()->unk_638 = 0;
        i = 1;
        MI_CpuFill8(&accepted, 0, sizeof(DWCNodeInfo));
        accepted.aid = data[i++];
        accepted.profileId = data[i++];
        accepted.publicIp = data[i++];
        accepted.publicPort = data[i++];
        accepted.localIp = data[i++];
        accepted.localPort = data[i++];
        accepted.unk_17 = data[i++];
        memcpy(&accepted.unk_24, &data[i++], sizeof(u32));
        acceptedUnk24 = data[i++];
        if (func_ov011_02167f6c()->unk_5a4 == 0) {
            func_ov011_02168224()->aid = data[i++];
        }
        func_ov011_02167f6c()->self = accepted;
        if (func_ov011_02167f6c()->mode == 3) {
            func_ov011_02167f6c()->groupId = acceptedUnk24;
            func_ov011_02167f6c()->unk_17 = acceptedExtra;
        }
        if (func_ov011_02167f6c()->matchType == 1 && func_ov011_02167f6c()->evalCallback != NULL &&
            func_ov011_02167f6c()->unk_5a4 == 0 && func_ov011_02167f6c()->unk_798 == 0) {
            func_ov011_02167f78(4);
            if (func_ov011_021666c4(func_ov011_0216322c(profileId)) != 0) {
                return 0;
            }
        } else {
            func_ov011_02167f78(5);
            if (func_ov011_021667b8(func_ov011_021635a0(0, 0, &func_ov011_02167f6c()->self)) != 0) {
                return 0;
            }
        }
        break;
    }
    case 3: {
        if (func_ov011_02167f6c()->state != 3 || profileId != func_ov011_02167f6c()->unk_67c) {
            break;
        }
        code = data[0];
        if (func_ov011_02168468() == 1) {
            func_ov011_02167f78(1);
            break;
        }
        if (func_ov011_02167f6c()->unk_798 != 0 || func_ov011_02167f6c()->matchType == 3) {
            func_ov011_02162880(code == 0x10 ? 0xc : 0x15, 0);
            return 0;
        }
        return func_ov011_02164f54(func_ov011_02167f6c()->unk_67c);
    }
    case 4: {
        if (func_ov011_02167f6c()->state != 3) {
            break;
        }
        if (func_ov011_02168468() == 1) {
            func_ov011_02167f78(1);
            break;
        }
        if (func_ov011_02167f6c()->unk_798 != 0) {
            if (profileId != func_ov011_02167f6c()->unk_67c || gpIsBuddy(func_ov011_02167f6c()->gp, profileId) == 0) {
                break;
            }
            func_ov011_02167f6c()->unk_7b6 = 1;
            func_ov011_02167f6c()->unk_62b = 0;
            func_ov011_02167f6c()->unk_644 = 12000;
            now = OS_GetTick();
            func_ov011_02167f6c()->unk_648 = now;
        } else {
            if (profileId != func_ov011_02167f6c()->unk_67c) {
                break;
            }
            now = OS_GetTick();
            func_ov011_02167f6c()->unk_648 = now;
            func_ov011_02167f6c()->unk_628 = 0;
            if (func_ov011_02167f6c()->matchType == 0) {
                func_ov011_02168374();
                func_ov011_02167f78(2);
                func_ov011_02167f6c()->unk_5e4 = 1;
                now = OS_GetTick();
                func_ov011_02167f6c()->unk_5e8 = now;
            } else if (func_ov011_02167f6c()->matchType == 1) {
                func_ov011_02168374();
                func_ov011_02164d64(1, profileId, profileId);
            } else if (func_ov011_02167f6c()->matchType == 3) {
                func_ov011_02167f6c()->unk_7b6 = 1;
                func_ov011_02167f6c()->unk_62b = 0;
                func_ov011_02167f6c()->unk_644 = 12000;
                now = OS_GetTick();
                func_ov011_02167f6c()->unk_648 = now;
            }
        }
        break;
    }
    case 5:
        if (func_ov011_02167f6c()->unk_18 == 0 || profileId != func_ov011_02167f6c()->unk_5dc) {
            break;
        }
        if (func_ov011_02168468() == 1) {
            func_ov011_02167f78(1);
            break;
        }
        if (func_ov011_02167f6c()->mode == 2 && func_ov011_02168388() == 1 &&
            func_ov011_0216054c(func_ov011_02167f6c()->self.profileId, 0) != NULL &&
            profileId == func_ov011_02167f6c()->self.profileId) {
            gt2CloseAllConnectionsHard(func_ov011_02167f6c()->fm->socket);
        }
        if (func_ov011_02165164(profileId) == 0) {
            return 0;
        }
        break;
    case 6: {
        hostIp = data[0];
        hostPort = data[1];
        if (func_ov011_02168388() == 0) {
            break;
        }
        if (func_ov011_021686a0() == 0 && func_ov011_02167f6c()->mode == 3 && func_ov011_02167f6c()->unk_5a4 == 0) {
            break;
        }
        if (func_ov011_02167f6c()->state == 1) {
            func_ov011_02167f78(5);
        } else if (!((func_ov011_02167f6c()->state == 5 || func_ov011_02167f6c()->state == 0xe) &&
                     profileId == func_ov011_02167f6c()->unk_5dc)) {
            break;
        }
        func_ov011_02167f6c()->unk_6d8 = 0xff;
        address.port = MATCH_SWAP16(hostPort);
        address.addr = hostIp;
        func_ov011_02167f6c()->target.kind = 1;
        conn = gt2GetSocketSOCKET(func_ov011_02167f6c()->fm->socket);
        func_ov011_02167428(0, conn, &address, &func_ov011_02167f6c()->target);
        func_ov011_02167f6c()->unk_608 = 0;
        break;
    }
    case 7: {
        if (func_ov011_02168468() == 1) {
            func_ov011_02167f78(1);
        }
        if (func_ov011_02167f6c()->state != 1 || profileId != func_ov011_02168250(0)->profileId) {
            break;
        }
        id = data[0];
        aid = data[1];
        if (func_ov011_02167f6c()->pending.profileId != 0) {
            break;
        }
        func_ov011_02167f6c()->unk_788 = 0;
        if (func_ov011_021686a0() != 0) {
            func_ov011_02167f6c()->self.profileId = id;
            func_ov011_02167f6c()->self.aid = aid;
        } else {
            func_ov011_02167f6c()->pending.profileId = id;
        }
        if (func_ov011_02167f6c()->newClientCallback != NULL) {
            func_ov011_02167f6c()->newClientCallback(DWCi_GetFriendListIndex(id),
                                                     func_ov011_02167f6c()->newClientParam);
        }
        break;
    }
    case 9: {
        node = func_ov011_02168250(func_ov011_021686b8() - 1 - func_ov011_02167f6c()->unk_620);
        if (func_ov011_02167f6c()->state != 0x10 || node == NULL || node->profileId != data[0]) {
            break;
        }
        func_ov011_02167f6c()->unk_620++;
        func_ov011_02165280(0);
        break;
    }
    case 8: {
        if (func_ov011_02167f6c()->state != 1 || profileId != func_ov011_02167f6c()->self.profileId) {
            break;
        }
        joinId = data[0];
        self = &func_ov011_02167f6c()->self;
        joinAid = data[2];
        if (func_ov011_021604b0(joinAid) != NULL) {
            sendId[0] = joinId;
            if (func_ov011_02164b78(func_ov011_021637d8(9, profileId, self->publicIp, self->publicPort, sendId, 1)) !=
                0) {
                return 0;
            }
            break;
        }
        func_ov011_02167f6c()->pending = func_ov011_02167f6c()->self;
        self->profileId = joinId;
        self->aid = joinAid;
        self->publicIp = data[3];
        self->publicPort = data[4];
        self->localIp = data[5];
        self->localPort = data[6];
        self->unk_17 = data[7];
        memcpy(&self->unk_24, &data[8], sizeof(u32));
        func_ov011_02167f78(5);
        func_ov011_02167f6c()->unk_644 = 0;
        func_ov011_02167f6c()->unk_638 = 0;
        func_ov011_021635a0(0, 0, self);
        break;
    }
    case 10: {
        if (func_ov011_02167f6c()->state != 1 || profileId != func_ov011_02168238()->profileId) {
            break;
        }
        id = data[0];
        aid = data[1];
        if (id == func_ov011_02167f6c()->self.profileId && aid == func_ov011_02167f6c()->self.aid) {
            break;
        }
        func_ov011_02167f6c()->self.aid = aid;
        func_ov011_02167f6c()->self.profileId = id;
        break;
    }
    case 12:
        if (profileId != func_ov011_02167f6c()->self.profileId) {
            break;
        }
        if (func_ov011_02167f6c()->matchType == 0 || func_ov011_02167f6c()->matchType == 1) {
            if (func_ov011_02165194(profileId) == 0) {
                return 0;
            }
            break;
        }
        if (func_ov011_02167f6c()->mode != 3) {
            break;
        }
        func_ov011_02167f6c()->unk_680 = profileId;
        func_ov011_02165bc8();
        func_ov011_021659ac(0);
        break;
    case 13:
    case 14:
    case 15:
        if (func_ov011_021661b4(profileId, type, data[0]) == 0) {
            return 0;
        }
        break;
    case 16: {
        node = func_ov011_02168238();
        if (node == NULL) {
            return 1;
        }
        if (profileId != node->profileId) {
            return 1;
        }
        saved = func_ov011_02167f6c()->unk_629;
        func_ov011_02167f6c()->unk_629 = 1;
        for (k = 0; k < count; k++) {
            node = func_ov011_02168270(data[k]);
            if (node == NULL) {
                if (func_ov011_02167f6c()->pending.profileId == data[k]) {
                    func_ov011_02160710(func_ov011_02167f6c()->pending.aid);
                    func_ov011_02167f6c()->pending.profileId = 0;
                }
            } else {
                func_ov011_02160710(node->aid);
            }
        }
        func_ov011_02167f6c()->unk_629 = saved;
        break;
    }
    case 0x40:
        if (func_ov011_02168388() == 0) {
            break;
        }
        if (profileId == func_ov011_02167f6c()->self.profileId) {
            if (func_ov011_02164b78(func_ov011_021637d8(0x41, profileId, ip, port, NULL, 0)) != 0) {
                return 0;
            }
        }
        break;
    case 0x41:
        break;
    case 0x52: {
        inMask = 0;
        if (func_ov011_02167f6c()->unk_0d == 0) {
            break;
        }
        if (func_ov011_021677e8() != 0) {
            return 0;
        }
        index = func_ov011_021665f0(func_ov011_02167f6c()->profileId, 1);
        if (index == 0xff) {
            return 0;
        }
        if (func_ov011_021665f0(profileId, 1) == 0xff) {
            return 0;
        }
        if (profileId == func_ov011_02168250(0)->profileId) {
            inMask = (func_ov011_02167f6c()->unk_788 & (1 << index)) != 0;
        }
        if (func_ov011_02164b78(func_ov011_021637d8(inMask != 0 ? 0x53 : 0x54, profileId, ip, port, NULL, 0)) != 0) {
            return 0;
        }
        now = OS_GetTick();
        func_ov011_02167f6c()->unk_670 = now;
        if (inMask != 0 && func_ov011_02167f6c()->state == 0xa) {
            func_ov011_02167f78(1);
            func_ov011_02167fe8();
        }
        break;
    }
    case 0x53: {
        if (func_ov011_02167f6c()->unk_0d == 0) {
            break;
        }
        if (func_ov011_021677e8() == 0) {
            break;
        }
        index = func_ov011_021665f0(profileId, 1);
        if (index == 0xff) {
            return 0;
        }
        func_ov011_02167f6c()->unk_788 |= 1 << index;
        if (func_ov011_02164b78(func_ov011_021637d8(0x53, profileId, ip, port, NULL, 0)) != 0) {
            return 0;
        }
        break;
    }
    case 0x54: {
        if (func_ov011_02167f6c()->unk_0d == 0) {
            break;
        }
        members = func_ov011_02168024();
        if (++func_ov011_02167f6c()->unk_79d > members * 5) {
            func_ov011_02160170();
        } else {
            now = OS_GetTick();
            func_ov011_02167f6c()->unk_670 = now;
        }
        break;
    }
    case 0x55:
        func_ov011_02167f6c();
        break;
    case 0x70:
        func_ov011_0216322c(0);
        func_ov011_02166648();
        func_ov011_02167f78(0xc);
        break;
    case 0x82: {
        mode = data[1];
        value = data[2];
        if (func_ov011_02168270(data[0]) == NULL) {
            break;
        }
        if (func_ov011_02167fc4() == 2) {
            flag = data[3];
            if (mode == 1) {
                break;
            }
            func_ov011_02167ac4(flag, value, 1);
        } else if (func_ov011_02167fc4() == 1) {
            if (mode == 0) {
                break;
            }
            if (count == 2) {
                if (func_ov011_02167f6c()->unk_30 == 1) {
                    break;
                }
                func_ov011_02167f6c()->unk_30 = 1;
                func_ov011_02167f6c()->unk_7cc = 8000;
                now = OS_GetTick();
                func_ov011_02167f6c()->unk_20 = now;
            } else {
                func_ov011_02167f6c()->unk_30 = 1;
                func_ov011_02167f6c()->unk_1c = data[3];
            }
        }
        break;
    }
    case 0x83:
        func_ov011_02162c30(data);
        break;
    }
    return 1;
}

// Tells the game about the match, passing on the match's player and the game's own parameter
static inline void MatchNotify(int error, BOOL cancel, BOOL self, BOOL isServer) {
    func_ov011_02167f6c()->notifyFunc(error, cancel, self, isServer,
                                      DWCi_GetFriendListIndex(func_ov011_02167f6c()->unk_680),
                                      func_ov011_02167f6c()->notifyParam);
}

static int func_ov011_0216475c(int profileId, int ip, int port, int nodeType, int skipChecks, int cookie,
                               void *userData, int blocked) {
    u64 now;
    int result;

    func_ov011_02167f6c()->error = 1;
    switch (func_ov011_02167fc4()) {
    case 1:
        if (func_ov011_02168348(profileId) != 0) {
            if (func_ov011_02167f6c()->unk_5a4 == 0) {
                if (func_ov011_02167f6c()->state == 1 && func_ov011_02167f6c()->pending.profileId == 0) {
                    func_ov011_02167f6c()->unk_5a4 = 1;
                    func_ov011_021685fc();
                    result = 2;
                    now = OS_GetTick();
                    func_ov011_02167f6c()->unk_768 = now;
                } else {
                    result = 4;
                }
            } else if (func_ov011_02167f6c()->state == 3) {
                DWCNodeInfo *self = func_ov011_02168224();

                if (profileId == func_ov011_02167f6c()->self.profileId) {
                    if (self->profileId < profileId) {
                        result = 2;
                        now = OS_GetTick();
                        func_ov011_02167f6c()->unk_768 = now;
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
            func_ov011_02167f6c()->error = 0x14;
        }
        break;
    case 0:
    case 2:
        result = func_ov011_0216484c(profileId, ip, port, nodeType, skipChecks, cookie, userData, blocked);
        break;
    }
    return result;
}

static int func_ov011_0216484c(int profileId, int ip, int port, int nodeType, int skipChecks, int cookie,
                               void *userData, int blocked) {
    int result;

    if (func_ov011_021685a0() != 0) {
        func_ov011_02167f6c()->error = 0x13;
        return 3;
    }
    if (blocked != 0) {
        func_ov011_02167f6c()->error = 0x18;
        return 3;
    }
    if (skipChecks == 0) {
        if ((nodeType == 0 && func_ov011_02167f6c()->matchType != 0) ||
            (nodeType == 1 && func_ov011_02167f6c()->matchType != 1) ||
            (nodeType == 3 && func_ov011_02167f6c()->matchType != 2)) {
            func_ov011_02167f6c()->error = 0x12;
            return 3;
        }
        if (func_ov011_02167f6c()->matchType == 1) {
            if (gpIsBuddy(func_ov011_02167f6c()->gp, profileId) == 0 || DWCi_GetFriendListIndex(profileId) == -1) {
                return 0xff;
            }
            if (func_ov011_02166528(profileId) == 0) {
                func_ov011_02167f6c()->error = 0x15;
                return 3;
            }
        } else if (func_ov011_02167f6c()->matchType == 2) {
            if (gpIsBuddy(func_ov011_02167f6c()->gp, profileId) == 0 || DWCi_GetFriendListIndex(profileId) == -1) {
                return 0xff;
            }
        }
    }
    if (func_ov011_02167f6c()->unk_17 + 1 == func_ov011_021679c0()) {
        result = 3;
        func_ov011_02167f6c()->error = 0x10;
    } else if (func_ov011_02167f6c()->state == 0x16) {
        result = 4;
    } else if (func_ov011_02160fd8() != 0 || func_ov011_02167f6c()->unk_48 != 0) {
        result = 3;
        func_ov011_02167f6c()->error = 0x11;
    } else if (cookie != func_ov011_02167f6c()->unk_7b0) {
        result = 3;
        func_ov011_02167f6c()->error = 0x12;
    } else if (func_ov011_02168270(profileId) != NULL) {
        result = 0xff;
    } else if (!((func_ov011_02167f6c()->state == 0xd || func_ov011_02167f6c()->state == 2 ||
                  func_ov011_02167f6c()->state == 3) &&
                 (func_ov011_02167f6c()->unk_5d8 != 0 || func_ov011_02167f6c()->unk_5d4 != 0) &&
                 (ip != 0 || port != 0))) {
        if (func_ov011_02167f6c()->state == 0xe && profileId == func_ov011_02167f6c()->unk_5dc) {
            result = 0xff;
        } else {
            result = 4;
        }
    } else if (func_ov011_02167f6c()->mode != 2 && func_ov011_02167f6c()->unk_67c != 0) {
        if (profileId == func_ov011_02167f6c()->unk_67c) {
            if (func_ov011_02167f6c()->profileId < profileId) {
                result = 2;
            } else {
                result = 0xff;
            }
        } else {
            result = 3;
            func_ov011_02167f6c()->error = 0x17;
        }
    } else {
        result = 2;
    }
    if (result == 2) {
        if (func_ov011_02167f6c()->checkFunc != NULL &&
            func_ov011_02167f6c()->checkFunc(userData, func_ov011_02167f6c()->unk_7c0) == 0) {
            func_ov011_02167f6c()->error = 0x16;
            return 3;
        }
        if (func_ov011_02167fc4() == 0) {
            func_ov011_0216788c(1);
        }
    }
    return result;
}

static int func_ov011_02164aa0(int profileId, u32 ip, u32 port) {
    u32 data[16];
    DWCNodeInfo *self;
    u8 aid;
    int i;

    if (func_ov011_02167f6c()->unk_18 != 0 && profileId == func_ov011_02167f6c()->unk_5dc) {
        return 0;
    }
    func_ov011_02167f6c()->unk_18 = 1;
    func_ov011_02167f6c()->unk_5dc = profileId;
    func_ov011_02167f6c()->unk_638 = 0;
    func_ov011_02167f6c()->unk_644 = 0;
    qr2_send_statechanged(func_ov011_02167f6c()->qr2);
    func_ov011_02167f6c()->unk_67c = 0;
    aid = func_ov011_021682ec();
    func_ov011_02167f6c()->self.aid = aid;
    if (func_ov011_02167f6c()->unk_5a4 == 0) {
        data[0] = func_ov011_02167f6c()->self.profileId;
        data[1] = func_ov011_02167f6c()->self.aid;
        self = func_ov011_02168224();
        for (i = 0; i < func_ov011_02168024(); i++) {
            DWCNodeInfo *node = func_ov011_02168250(i);

            if (self->profileId != node->profileId) {
                int result = func_ov011_021637d8(7, node->profileId, node->publicIp, node->publicPort, data, 2);

                if (result != 0) {
                    return result;
                }
            }
        }
    }
    return 0;
}

static int func_ov011_02164b78(int error) {
    if (func_ov011_02167f6c()->matchType == 0) {
        return func_ov011_021666c4(error);
    }
    return func_ov011_02166670(error);
}

static int func_ov011_02164b98(int profileId, int flag) {
    u64 now;
    u32 data[16];
    DWCNodeInfo *self;
    u32 ip = 0;
    u16 port = 0;

    if (flag != 0 || (func_ov011_02167f6c()->unk_5d8 == 0 && func_ov011_02167f6c()->unk_5d4 == 0)) {
        func_ov011_02167f6c()->unk_638 = 1;
        now = OS_GetTick();
        func_ov011_02167f6c()->unk_63c = now;
        func_ov011_02167f6c()->unk_67c = 0;
        return 0;
    }
    if (func_ov011_02167f6c()->matchType == 0) {
        ip = func_ov011_02167f6c()->self.publicIp;
        port = func_ov011_02167f6c()->self.publicPort;
        func_ov011_02167f6c()->unk_67c = profileId;
    } else {
        func_ov011_02167f6c()->unk_67c = profileId;
        if (gpIsBuddy(func_ov011_02167f6c()->gp, profileId) == 0) {
            ip = func_ov011_02167f6c()->self.publicIp;
            port = func_ov011_02167f6c()->self.publicPort;
        }
    }
    if (func_ov011_02167f6c()->matchType == 3 ||
        (func_ov011_02167f6c()->unk_798 != 0 && gpIsBuddy(func_ov011_02167f6c()->gp, profileId) != 0)) {
        func_ov011_02167f6c()->unk_7b6 = 0;
        func_ov011_02167f6c()->unk_644 = 30000;
    } else {
        func_ov011_02167f6c()->unk_7b6 = 5;
        func_ov011_02167f6c()->unk_644 = 12000;
    }
    now = OS_GetTick();
    func_ov011_02167f6c()->unk_648 = now;
    func_ov011_02167f6c()->unk_638 = 0;
    self = func_ov011_02168224();
    data[0] = func_ov011_02167f6c()->matchType;
    data[1] = self->publicIp;
    data[2] = self->publicPort;
    data[3] = self->localIp;
    data[4] = self->localPort;
    data[5] = self->unk_17;
    data[6] = func_ov011_02167f6c()->unk_798;
    data[7] = func_ov011_02167f6c()->unk_7b0;
    data[8] = func_ov011_02167f6c()->userData;
    func_ov011_02167f6c()->unk_634 = DWCi_GetMathRand32(-2) + 1;
    data[9] = func_ov011_02167f6c()->unk_634;
    data[10] = func_ov011_02167f6c()->unk_5a4 != 0;
    return func_ov011_021637d8(1, profileId, ip, port, data, 11);
}

static int func_ov011_02164d64(int a0, int a1, int a2) {
    u64 now;
    GPBuddyStatus status;
    int index;
    int found = 0;
    int profileId;
    int i;

    if (a1 == 0) {
        func_ov011_02167f6c()->unk_621++;
        func_ov011_02167f6c()->unk_621 = func_ov011_02167f6c()->unk_621 % func_ov011_02167f6c()->friendCount;
    }
    func_ov011_02167f6c()->mode = 3;
    for (i = 0; i < func_ov011_02167f6c()->friendCount;) {
        profileId = DWC_GetGsProfileId(
            DWCi_GetUserData(),
            &func_ov011_02167f6c()->friendList[func_ov011_02167f6c()->friendOrder[func_ov011_02167f6c()->unk_621]]);
        if (profileId != 0 && profileId != -1 &&
            DWCi_IsValidFriendData(
                &func_ov011_02167f6c()
                     ->friendList[func_ov011_02167f6c()->friendOrder[func_ov011_02167f6c()->unk_621]]) &&
            func_ov011_02168270(profileId) == NULL) {
            int err1 = gpGetBuddyIndex(func_ov011_02167f6c()->gp, profileId, &index);
            int err2 = gpGetBuddyStatus(func_ov011_02167f6c()->gp, index, &status);

            err1 |= err2;
            if (func_ov011_021686c0(&status) == 0 && err1 == 0 && status.status == 4) {
                found = 1;
                break;
            }
        }
        i++;
        func_ov011_02167f6c()->unk_621 = (i + func_ov011_02167f6c()->unk_621) % func_ov011_02167f6c()->friendCount;
    }
    if (found != 0) {
        if (profileId == a2) {
            a0 = 1;
        }
        func_ov011_02168374();
        func_ov011_02167f6c()->self.aid = 0xff;
        func_ov011_02167f6c()->self.profileId = profileId;
        return func_ov011_02164b98(profileId, a0);
    }
    func_ov011_02167f6c()->unk_644 = 3000;
    now = OS_GetTick();
    func_ov011_02167f6c()->unk_648 = now;
    func_ov011_02167f6c()->unk_638 = 0;
    return 0;
}

static int func_ov011_02164f54(int a0) {
    u64 now;
    func_ov011_02167f6c()->unk_67c = 0;
    func_ov011_02167f6c()->unk_628 = 0;
    now = OS_GetTick();
    func_ov011_02167f6c()->unk_648 = now;
    func_ov011_02168374();
    if (func_ov011_02167f6c()->unk_798 != 0) {
        func_ov011_02167f78(0xc);
        if (func_ov011_021666c4(func_ov011_0216322c(0)) != 0) {
            return 0;
        }
    } else if (func_ov011_02167f6c()->matchType == 0) {
        func_ov011_02167f78(2);
        if (a0 != 0) {
            func_ov011_02167f6c()->unk_5e4 = 1;
            now = OS_GetTick();
            func_ov011_02167f6c()->unk_5e8 = now;
        } else {
            if (func_ov011_021666c4(func_ov011_0216322c(0)) != 0) {
                return 0;
            }
        }
    } else if (func_ov011_02167f6c()->matchType == 1) {
        if (func_ov011_02164b78(func_ov011_02164d64(1, 0, a0)) != 0) {
            return 0;
        }
    } else if (func_ov011_02167f6c()->matchType == 3) {
        func_ov011_02162880(6, -80410);
        return 0;
    }
    return 1;
}

static int func_ov011_02165040(int unused) {
    int result;

    if (func_ov011_02168388() == 0) {
        return 0;
    }
    result = func_ov011_021637d8(5, func_ov011_02167f6c()->self.profileId, func_ov011_02167f6c()->self.publicIp,
                                 func_ov011_02167f6c()->self.publicPort, NULL, 0);
    func_ov011_02167f6c()->unk_67c = 0;
    return result;
}

static int func_ov011_02165094(int profileId, int a1) {
    int i;

    if (func_ov011_02168388() != 0) {
        func_ov011_02168374();
    }
    func_ov011_02167f6c()->unk_6d8 = 0xff;
    if (func_ov011_02167f6c()->target.cookie != 0) {
        NNCancel(func_ov011_02167f6c()->target.cookie);
        func_ov011_02167f6c()->target.cookie = 0;
    }
    func_ov011_02167f6c()->unk_67c = 0;
    if (func_ov011_02167fc4() == 2) {
        func_ov011_02167f6c()->unk_18 = 0;
        func_ov011_02167f6c()->unk_5dc = 0;
        func_ov011_02167f78(0x11);
        func_ov011_02167f6c()->unk_654 = 0;
        func_ov011_02167f6c()->unk_632 = 0;
        func_ov011_02165b7c(profileId);
        for (i = 1; i < func_ov011_02168024(); i++) {
            if (func_ov011_02166148(func_ov011_02168250(i)->profileId, 0xd) == 0) {
                return 0;
            }
        }
        if (func_ov011_02168024() == 1) {
            func_ov011_021659ac(a1);
        }
    } else {
        func_ov011_02168468();
    }
    return 1;
}

static int func_ov011_02165164(int profileId) {
    int a1 = 2;

    if (func_ov011_02167f6c()->matchType == 0 || func_ov011_02167f6c()->matchType == 1) {
        if (func_ov011_02167f6c()->unk_0d == 0) {
            a1 = 1;
        }
    }
    return func_ov011_02165094(profileId, a1);
}

static int func_ov011_02165194(int unused) {
    int result = 1;

    if (func_ov011_02167f6c()->unk_798 != 0 || func_ov011_02167f6c()->matchType == 3) {
        if (func_ov011_02168024() > 1) {
            func_ov011_02165bc8();
        }
        if (func_ov011_02167f6c()->unk_798 != 0) {
            func_ov011_02162880(0x16, -80432);
        } else {
            func_ov011_02162880(0x16, -80430);
        }
        return 0;
    }
    if (func_ov011_02168468() == 1) {
        return result;
    }
    if (func_ov011_02167f6c()->pending.profileId != 0) {
        func_ov011_02167f6c()->pending.profileId = 0;
    }
    if (func_ov011_02167f6c()->target.cookie != 0) {
        NNCancel(func_ov011_02167f6c()->target.cookie);
        func_ov011_02167f6c()->target.cookie = 0;
    }
    if ((func_ov011_021686a0() != 0 && func_ov011_02167f6c()->unk_62f != 0) ||
        (func_ov011_02168388() != 0 && func_ov011_0216054c(func_ov011_02167f6c()->self.profileId, 1) != NULL) ||
        func_ov011_02168024() > 1) {
        func_ov011_02165b30();
    } else {
        func_ov011_02167f78(3);
        result = func_ov011_02164f54(0);
    }
    return result;
}

static void func_ov011_02165280(int a0) {
    switch (a0) {
    case 0:
        if (func_ov011_021686a0() != 0) {
            func_ov011_02165654();
        } else {
            func_ov011_02165558();
        }
        break;
    case 1:
        func_ov011_02167f78(1);
        if (func_ov011_02167f6c()->mode == 3) {
            func_ov011_02167f6c()->unk_680 = func_ov011_02167f6c()->self.profileId;
        }
        break;
    case 2:
        func_ov011_02167f78(1);
        if (func_ov011_02168468() == 1) {
            func_ov011_021685fc();
        } else {
            func_ov011_02167fa0(1);
            func_ov011_02167f6c()->mode = 3;
            if (func_ov011_02167f6c()->matchType == 0 || func_ov011_02167f6c()->matchType == 1) {
                func_ov011_02167f6c()->unk_18 = 1;
                func_ov011_02167f6c()->unk_5dc = func_ov011_02167f6c()->profileId;
            }
            func_ov011_02166648();
            if (func_ov011_021686a0() != 0) {
                if (func_ov011_021683a8(&func_ov011_02167f6c()->pending) != 0) {
                    u32 data;
                    DWCNodeInfo *node;
                    int result;

                    node = &func_ov011_02167f6c()->self;
                    data = node->profileId;
                    func_ov011_02168030(node);
                    func_ov011_02167f6c()->self = func_ov011_02167f6c()->pending;
                    node = &func_ov011_02167f6c()->self;
                    result = func_ov011_021637d8(9, node->profileId, node->publicIp, node->publicPort, &data, 1);
                    func_ov011_0216839c(&func_ov011_02167f6c()->pending);
                    if (func_ov011_02164b78(result) != 0) {
                        return;
                    }
                }
                func_ov011_02167f6c()->unk_62f = 1;
            }
        }
        break;
    case 3:
        func_ov011_02167f6c()->unk_0d = 1;
        func_ov011_0216549c();
        func_ov011_02167f6c()->unk_798 = 0;
        qr2_send_statechanged(func_ov011_02167f6c()->qr2);
        MatchNotify(0, 0, func_ov011_02167f6c()->unk_680 == 0, 0);
        if (func_ov011_02167f6c()->mode == 2) {
            if (func_ov011_02166670(func_ov011_02162d80(-1)) != 0) {
                return;
            }
            func_ov011_02167f78(0xd);
        } else {
            func_ov011_02162d80(2);
            func_ov011_02167f78(1);
        }
        func_ov011_02167f6c()->unk_680 = 0;
        func_ov011_02167f6c()->unk_62a = 0;
        func_ov011_02167fe8();
        break;
    }
    if (func_ov011_02167f6c()->sb != NULL) {
        MatchControl *ctrl;

        ServerBrowserClear(func_ov011_02167f6c()->sb);
        ctrl = func_ov011_02167f6c();
        ctrl->unk_5f0 = 0;
    }
}

static void func_ov011_0216549c(void) {
    func_ov011_02167f6c()->unk_5f9 = 0;
    if (func_ov011_021686a0() != 0) {
        func_ov011_02165540();
        return;
    }
    func_ov011_021654c0();
}

static void func_ov011_021654c0(void) {
    DWCNodeInfo *pending;
    DWCNodeInfo *node;

    if (func_ov011_02167f6c()->mode == 2) {
        if (func_ov011_02168388() != 0) {
            func_ov011_021680a4(&func_ov011_02167f6c()->self);
            func_ov011_02168374();
        }
    } else if (func_ov011_02168024() == 1) {
        func_ov011_02168030(&func_ov011_02167f6c()->self);
        func_ov011_02168374();
    } else {
        pending = &func_ov011_02167f6c()->pending;
        if (pending->profileId != 0) {
            node = func_ov011_02168270(pending->profileId);
            if (node != NULL) {
                func_ov011_02160710(node->aid);
            }
            node = func_ov011_021682ac(pending->aid);
            if (node != NULL) {
                func_ov011_02160710(node->aid);
            }
            func_ov011_021680a4(pending);
            pending->profileId = 0;
        }
    }
}

static void func_ov011_02165540(void) {
    func_ov011_021654c0();
    func_ov011_02167f6c()->unk_62f = 0;
}

static int func_ov011_02165558(void) {
    int i;
    int result = 0;
    u64 now;

    if (func_ov011_02167f6c()->unk_5a4 != 0 || func_ov011_02167f6c()->unk_5a8 == 0xf) {
        func_ov011_02167f6c()->unk_18 = 0;
        func_ov011_02167f6c()->unk_5dc = 0;
        func_ov011_02167f78(1);
        func_ov011_02168468();
        return 0;
    }
    func_ov011_02167f6c()->unk_18 = 0;
    func_ov011_02167f6c()->unk_5dc = 0;
    if (func_ov011_02167f6c()->unk_0d == 0 && func_ov011_02168024() == 1) {
        now = OS_GetTick();
        func_ov011_02167f6c()->unk_7a0 = now;
    }
    qr2_send_statechanged(func_ov011_02167f6c()->qr2);
    if (func_ov011_02167f6c()->mode == 2) {
        func_ov011_02167f6c()->unk_680 = func_ov011_02167f6c()->self.profileId;
    }
    func_ov011_02167f78(0x13);
    func_ov011_02167f6c()->unk_650 = 0;
    for (i = 1; i < func_ov011_02168024(); i++) {
        func_ov011_02165d2c(func_ov011_02168250(i)->aid, 2);
    }
    func_ov011_02165d2c(func_ov011_02167f6c()->self.aid, 2);
    if (func_ov011_02167f6c()->mode != 2) {
        result = 1;
    }
    return result;
}

static int func_ov011_02165654(void) {
    u32 data[16];
    DWCNodeInfo saved;
    DWCNodeInfo *me;
    DWCNodeInfo *node;
    u32 index;

    saved = func_ov011_02167f6c()->self;
    me = &saved;
    if (func_ov011_02167f6c()->unk_620 < func_ov011_021686b8() - 1) {
        index = (func_ov011_021686b8() - 1 - func_ov011_02167f6c()->unk_620) & 0xff;
        func_ov011_02167f78(0x10);
        node = func_ov011_02168250(index);
        data[0] = node->profileId;
        data[1] = index;
        data[2] = node->aid;
        data[3] = node->publicIp;
        data[4] = node->publicPort;
        data[5] = node->localIp;
        data[6] = node->localPort;
        data[7] = node->unk_17;
        data[8] = node->unk_24;
    } else {
        func_ov011_02167f6c()->unk_620 = 0;
        return func_ov011_02165558();
    }
    if (func_ov011_02167f6c()->state != 0x13) {
        if (func_ov011_02164b78(func_ov011_021637d8(8, me->profileId, me->publicIp, me->publicPort, data, 9)) != 0) {
            return 0;
        }
        node = func_ov011_02168250(index);
        data[0] = me->profileId;
        data[1] = me->aid;
        if (func_ov011_02164b78(func_ov011_021637d8(10, node->profileId, node->publicIp, node->publicPort, data, 2)) !=
            0) {
            return 0;
        }
        func_ov011_02167f6c()->unk_6d9 = 0;
    }
    return 0;
}

static void func_ov011_0216574c(void) {
    int hasConn;

    func_ov011_02167f6c()->unk_680 = 0;
    hasConn = 0;
    if (func_ov011_02167f6c()->state == 0x16) {
        func_ov011_021658a4();
        return;
    }
    if (func_ov011_02160500() != 0) {
        hasConn = 1;
    }
    func_ov011_02167f6c()->unk_62a = 1;
    if (func_ov011_02167f6c()->mode == 3) {
        if (func_ov011_02167f6c()->target.cookie != 0) {
            NNCancel(func_ov011_02167f6c()->target.cookie);
            func_ov011_02167f6c()->target.cookie = 0;
        }
        func_ov011_02167f6c()->unk_62a = 1;
        if (hasConn != 0) {
            gt2CloseAllConnectionsHard(func_ov011_02167f6c()->fm->socket);
        } else {
            if (func_ov011_02164b78(func_ov011_02165040(0)) != 0) {
                return;
            }
        }
        func_ov011_021658a4();
        return;
    }
    if (func_ov011_02167f6c()->state == 3 || func_ov011_02167f6c()->state == 4 || func_ov011_02167f6c()->state == 5 ||
        func_ov011_02167f6c()->state == 6 || func_ov011_02167f6c()->state == 0xe ||
        func_ov011_02167f6c()->state == 0xf) {
        if (func_ov011_02167f6c()->unk_67c != 0) {
            if (func_ov011_02164b78(func_ov011_02165040(0)) != 0) {
                return;
            }
        }
        if (func_ov011_02164b78(func_ov011_0216592c()) != 0) {
            return;
        }
    }
    if (func_ov011_02167f6c()->target.cookie != 0) {
        NNCancel(func_ov011_02167f6c()->target.cookie);
        func_ov011_02167f6c()->target.cookie = 0;
    }
    if (hasConn != 0) {
        func_ov011_02167f6c()->unk_62a = 1;
        gt2CloseAllConnectionsHard(func_ov011_02167f6c()->fm->socket);
    }
    func_ov011_021658a4();
}

static void func_ov011_021658a4(void) {
    int flag = 1;

    if (func_ov011_02166670(func_ov011_02162d80(1)) == 0) {
        func_ov011_021631a8();
        if (func_ov011_02167f6c()->unk_680 == 0 && func_ov011_02167f6c()->mode != 2) {
            flag = 0;
        }
        MatchNotify(0, 1, func_ov011_02167f6c()->unk_680 == 0, flag);
        func_ov011_02167f6c()->unk_62a = 0;
    }
}

static int func_ov011_0216592c(void) {
    u32 data = 0;
    int result = 0;

    if (func_ov011_02167f6c()->unk_18 != 0 && func_ov011_02167f6c()->unk_5dc != 0 &&
        func_ov011_02167f6c()->unk_5dc != func_ov011_02167f6c()->profileId) {
        result = func_ov011_021637d8(0xc, func_ov011_02167f6c()->unk_5dc, func_ov011_02167f6c()->self.publicIp,
                                     func_ov011_02167f6c()->self.publicPort, &data, 1);
        func_ov011_02167f6c()->unk_18 = 0;
        func_ov011_02167f6c()->unk_5dc = 0;
        func_ov011_02168374();
    }
    return result;
}

static void func_ov011_021659ac(int a0) {
    if (a0 == 0) {
        func_ov011_021658a4();
        return;
    }
    func_ov011_02162e80(a0);
    if (func_ov011_02167f6c()->matchType == 2 || func_ov011_02167f6c()->matchType == 3) {
        MatchNotify(0, 1, func_ov011_02167f6c()->unk_680 == 0, 0);
        return;
    }
    if (func_ov011_02167f6c()->matchType == 0) {
        if (func_ov011_02167f6c()->unk_0d != 0) {
            MatchNotify(0, 1, func_ov011_02167f6c()->unk_680 == 0, 0);
        }
        if (a0 == 1) {
            func_ov011_02167fa0(0);
            func_ov011_02167f78(0x16);
            if (func_ov011_021666c4(func_ov011_0216322c(func_ov011_02167f6c()->profileId)) != 0) {
                return;
            }
        }
    } else if (func_ov011_02167f6c()->matchType == 1) {
        if (func_ov011_02167f6c()->unk_0d != 0) {
            MatchNotify(0, 1, func_ov011_02167f6c()->unk_680 == 0, 0);
        }
        if (a0 == 1) {
            func_ov011_02167fa0(0);
            func_ov011_02167f6c()->unk_798 = 0;
            func_ov011_02167f78(0x16);
            if (func_ov011_021666c4(func_ov011_0216322c(func_ov011_02167f6c()->profileId)) != 0) {
                return;
            }
        }
    }
}

static void func_ov011_02165b30(void) {
    if (func_ov011_02167f6c()->matchType != 2 && func_ov011_02167f6c()->matchType != 3) {
        func_ov011_02165bc8();
        func_ov011_021659ac(1);
    }
}

static int func_ov011_02165b54(void) {
    func_ov011_02167f6c()->unk_18 = 0;
    func_ov011_02167f6c()->unk_5dc = 0;
    func_ov011_02167f6c()->unk_629 = 0;
    func_ov011_021659ac(1);
    return 0;
}

static int func_ov011_02165b7c(int profileId) {
    GTI2Connection **conn;
    int result;

    func_ov011_02167f6c()->unk_680 = profileId;
    conn = func_ov011_0216054c(profileId, 0);
    if (conn != NULL) {
        func_ov011_02167f6c()->unk_629 = 1;
        result = 1;
        gt2CloseConnectionHard(*conn);
        func_ov011_02167f6c()->unk_629 = 0;
        func_ov011_02168184(profileId);
    } else {
        func_ov011_02168184(profileId);
        result = 0;
    }
    return result;
}

static void func_ov011_02165bc8(void) {
    func_ov011_02167f6c()->unk_629 = 1;
    gt2CloseAllConnectionsHard(func_ov011_02167f6c()->fm->socket);
    func_ov011_02167f6c()->unk_629 = 0;
}

static int func_ov011_02165bf0(u32 mask) {
    int outIds[32];
    int inIds[32];
    int addSelf = 0;
    int result = 1;
    int i;
    int inCount = 0;
    int outCount = 0;

    for (i = 1; i < func_ov011_02168024(); i++) {
        DWCNodeInfo *node = func_ov011_02168250(i);

        if (node != NULL) {
            if (((1 << node->aid) & mask) != 0) {
                inIds[inCount++] = node->profileId;
            } else {
                outIds[outCount++] = node->profileId;
            }
        }
    }
    if (func_ov011_02168388() != 0) {
        MatchControl *ctrl = func_ov011_02167f6c();

        if ((mask & (1 << ctrl->self.aid)) != 0) {
            inIds[inCount++] = ctrl->self.profileId;
        } else {
            outIds[outCount++] = ctrl->self.profileId;
            addSelf = 1;
        }
    }
    for (i = 0; i < inCount; i++) {
        int profileId = inIds[i];
        DWCNodeInfo *node = func_ov011_02168270(profileId);

        if (func_ov011_02168388() != 0 && node == NULL && profileId == func_ov011_02167f6c()->self.profileId) {
            node = &func_ov011_02167f6c()->self;
        }
        if (node != NULL) {
            if (func_ov011_02164b78(
                    func_ov011_021637d8(0x10, profileId, node->publicIp, node->publicPort, outIds, outCount)) != 0) {
                result = 0;
            }
        }
    }
    func_ov011_02167f6c()->unk_629 = 1;
    if (addSelf != 0) {
        func_ov011_02160710(func_ov011_02167f6c()->self.aid);
        func_ov011_02168374();
    }
    for (i = 0; i < outCount; i++) {
        int aid = func_ov011_021665f0(outIds[i], 0);

        if (aid != 0xff) {
            func_ov011_02160710(aid);
        }
    }
    func_ov011_02167f6c()->unk_629 = 0;
    return result;
}

static void func_ov011_02165d2c(u32 aid, int type) {
    int size = 4;
    u32 local[2];
    u32 *data = local;
    u64 now;

    switch (type) {
    case 2:
        if (func_ov011_02168388() != 0 && aid == func_ov011_02167f6c()->self.aid) {
            data[0] = 1;
        } else {
            DWCNodeInfo *self;

            size = 0x1c;
            data = DWC_Alloc(4, 0x1c);
            if (data == NULL) {
                return;
            }
            data[0] = 0;
            self = &func_ov011_02167f6c()->self;
            data[1] = ((self->unk_17 << 16) & 0xffff0000) | self->aid;
            data[2] = self->profileId;
            data[3] = self->publicIp;
            data[4] = self->localIp;
            data[5] = ((self->publicPort << 16) & 0xffff0000) | self->localPort;
            data[6] = self->unk_24;
        }
        break;
    case 3:
        ((u8 *)data)[0] = func_ov011_02167f6c()->unk_630;
        ((u8 *)data)[1] = func_ov011_02167f6c()->unk_630 >> 8;
        break;
    case 4:
        if (func_ov011_02168388() != 0 && aid == func_ov011_02167f6c()->self.aid) {
            int count;

            if (func_ov011_021686a0() != 0) {
                count = 0;
            } else {
                count = func_ov011_02168024() - 1;
            }
            if (count == 0) {
                ((u8 *)data)[0] = 0;
                ((u8 *)data)[1] = 0;
                ((u8 *)data)[2] = 0;
                ((u8 *)data)[3] = 0;
                data[1] = func_ov011_02167f6c()->unk_7c8;
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
                    DWCNodeInfo *node = func_ov011_02168250(n + 1);

                    data[pos++] = ((node->unk_17 << 16) & 0xffff0000) | node->aid;
                    data[pos++] = node->profileId;
                    data[pos++] = node->publicIp;
                    data[pos++] = node->localIp;
                    data[pos++] = ((node->publicPort << 16) & 0xffff0000) | node->localPort;
                    data[pos++] = node->unk_24;
                }
                data[pos] = func_ov011_02167f6c()->unk_7c8;
            }
        } else {
            size = 8;
            ((u8 *)data)[0] = 0;
            ((u8 *)data)[1] = 0;
            ((u8 *)data)[2] = 0;
            ((u8 *)data)[3] = 0;
            data[1] = func_ov011_02167f6c()->unk_7c8;
        }
        break;
    }
    func_ov011_02168a1c(type, aid, data, size);
    if (data != local) {
        DWC_Free(4, data, 0);
    }
    now = OS_GetTick();
    func_ov011_02167f6c()->unk_658 = now;
}

static int func_ov011_02165ef4(void) {
    u64 ms;

    if (func_ov011_02167f6c()->state == 8 || func_ov011_02167f6c()->state == 0x13 ||
        func_ov011_02167f6c()->state == 0x17 || func_ov011_02167f6c()->state == 0x14) {
        ms = OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_658);
    } else {
        return 1;
    }
    switch (func_ov011_02167f6c()->state) {
    case 8:
        if (ms > func_ov011_02167f6c()->unk_670) {
            func_ov011_02168024();
            func_ov011_02167f6c()->pending.profileId = 0;
            if (func_ov011_02165194(func_ov011_02168238()->profileId) == 0) {
                return 0;
            }
        }
        break;
    case 0x13:
        if (ms > 6000) {
            func_ov011_02167f6c()->unk_62c++;
            if (func_ov011_02167f6c()->unk_62c > 5) {
                u64 now;

                func_ov011_02167f6c()->unk_62c = 0;
                now = OS_GetTick();
                func_ov011_02167f6c()->unk_658 = now;
                func_ov011_02166098();
            } else {
                int i;

                for (i = 1; i < func_ov011_02168024(); i++) {
                    u8 aid = func_ov011_02168250(i)->aid;

                    if ((func_ov011_02167f6c()->unk_650 & (1 << aid)) == 0) {
                        func_ov011_02165d2c(aid, 2);
                    }
                }
                {
                    DWCNodeInfo *self = &func_ov011_02167f6c()->self;
                    u8 aid = self->aid;

                    if ((func_ov011_02167f6c()->unk_650 & (1 << aid)) == 0) {
                        func_ov011_02165d2c(aid, 2);
                    }
                }
            }
        }
        break;
    case 0x17:
        if (ms > 3000) {
            func_ov011_021660f8();
        }
        break;
    case 0x14:
        if (ms > func_ov011_02167f6c()->unk_630) {
            func_ov011_02165280(3);
        }
        break;
    }
    return 1;
}

static int func_ov011_02166098(void) {
    u64 now;

    if (func_ov011_02165bf0(func_ov011_02167f6c()->unk_650) == 0) {
        return 0;
    }
    if (func_ov011_02168024() > 1 || func_ov011_02168388() != 0) {
        now = OS_GetTick();
        func_ov011_02167f6c()->unk_658 = now;
        func_ov011_02167f78(0x17);
    } else {
        if (func_ov011_02165164(func_ov011_02167f6c()->unk_680) == 0) {
            return 0;
        }
    }
    return 1;
}

static void func_ov011_021660f8(void) {
    int i;

    if (func_ov011_02167fc4() == 2) {
        for (i = 1; i < func_ov011_02168024(); i++) {
            func_ov011_02165d2c(func_ov011_02168250(i)->aid, 4);
        }
        if (func_ov011_02168388() != 0) {
            func_ov011_02165d2c(func_ov011_02167f6c()->self.aid, 4);
        }
        func_ov011_02167f78(0x14);
    }
}

static int func_ov011_02166148(int profileId, int message) {
    u32 data;
    DWCNodeInfo *node;
    u64 now;
    int count;

    if (message == 0xd) {
        data = func_ov011_02167f6c()->unk_680;
        count = 1;
    } else {
        count = 0;
    }
    node = func_ov011_02168270(profileId);
    if (node == NULL) {
        return 0;
    }
    if (func_ov011_02164b78(func_ov011_021637d8(message, profileId, node->publicIp, node->publicPort, &data, count)) !=
        0) {
        return 0;
    }
    now = OS_GetTick();
    func_ov011_02167f6c()->unk_660 = now;
    return 1;
}

static int func_ov011_021661b4(int profileId, int message, int value) {
    u64 ms;
    u64 now;
    int i;
    int aid;

    if (func_ov011_021603a4() != 5) {
        return 1;
    }
    switch (message) {
    case 0xd:
        if (func_ov011_02167f6c()->state != 7) {
            func_ov011_02167f78(7);
            func_ov011_02167f6c()->unk_680 = value;
            if (func_ov011_02167f6c()->pending.profileId != 0) {
                func_ov011_02167f6c()->pending.profileId = 0;
            }
            func_ov011_02168374();
        }
        if (func_ov011_02166148(profileId, 0xe) == 0) {
            return 0;
        }
        break;
    case 0xe:
        if (func_ov011_02167f6c()->state == 0x11) {
            now = OS_GetTick();
            ms = OS_TicksToMilliSeconds(now - func_ov011_02167f6c()->unk_660) / 2;
            if (ms > 300) {
                ms = OS_TicksToMilliSeconds(now - func_ov011_02167f6c()->unk_660) / 2 - 300;
                if (func_ov011_02167f6c()->unk_632 < ms) {
                    func_ov011_02167f6c()->unk_632 = ms;
                }
            }
            aid = func_ov011_021665f0(profileId, 0);
            if (aid != 0xff) {
                func_ov011_02167f6c()->unk_654 |= 1 << aid;
            }
            if (func_ov011_02166604(1) == func_ov011_02167f6c()->unk_654) {
                for (i = 1; i < func_ov011_02168024(); i++) {
                    if (func_ov011_02166148(func_ov011_02168250(i)->profileId, 0xf) == 0) {
                        return 0;
                    }
                }
                func_ov011_02167f78(0x12);
            }
        } else {
            if (func_ov011_02166148(profileId, 0xf) == 0) {
                return 0;
            }
        }
        break;
    case 0xf:
        if (func_ov011_02167f6c()->state == 7) {
            func_ov011_021659ac(2);
        }
        break;
    }
    return 1;
}

static int func_ov011_02166364(void) {
    u64 ms;
    u64 now;
    int i;

    if (func_ov011_02167f6c()->state == 7 || func_ov011_02167f6c()->state == 0x11 ||
        func_ov011_02167f6c()->state == 0x12) {
        ms = OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_660);
    } else {
        return 1;
    }
    switch (func_ov011_02167f6c()->state) {
    case 7:
        if (ms > 6000) {
            if (func_ov011_02166148(func_ov011_02168250(0)->profileId, 0xe) == 0) {
                return 0;
            }
        }
        break;
    case 0x11:
        if (ms > 6000) {
            func_ov011_02167f6c()->unk_62d++;
            if (func_ov011_02167f6c()->unk_62d > 5) {
                if (func_ov011_02165bf0(func_ov011_02167f6c()->unk_654) == 0) {
                    return 0;
                }
                if (func_ov011_02168024() > 1) {
                    func_ov011_02167f6c()->unk_62d = 0;
                    now = OS_GetTick();
                    func_ov011_02167f6c()->unk_660 = now;
                } else {
                    func_ov011_021659ac(2);
                }
            } else {
                for (i = 1; i < func_ov011_02168024(); i++) {
                    DWCNodeInfo *node = func_ov011_02168250(i);

                    if ((func_ov011_02167f6c()->unk_654 & (1 << node->aid)) == 0 &&
                        func_ov011_02166148(node->profileId, 0xd) == 0) {
                        return 0;
                    }
                }
            }
        }
        break;
    case 0x12:
        if (ms > func_ov011_02167f6c()->unk_632) {
            func_ov011_021659ac(2);
        }
        break;
    }
    return 1;
}

static u8 func_ov011_021664c4(void) {
    u8 i = 0;

    do {
        if (sKeys[i].id == 0) {
            return i + 100;
        }
        i++;
    } while (i < 0x9a);
    return 0;
}

static void func_ov011_021664f0(void) {
    int i;

    for (i = 0; i < 0x9a; i++) {
        if (sKeys[i].name != NULL) {
            DWC_Free(4, sKeys[i].name, 0);
        }
    }
    MIi_CpuClear32(0, sKeys, sizeof(sKeys));
}

static BOOL func_ov011_02166528(int profileId) {
    int i;

    if (func_ov011_02167f6c()->friendList == 0) {
        return FALSE;
    }
    for (i = 0; i < func_ov011_02167f6c()->friendCount; i++) {
        int friendIndex = DWCi_GetProfileIDFromList(func_ov011_02167f6c()->friendOrder[i]);

        if (friendIndex > 0 && friendIndex == profileId) {
            return TRUE;
        }
    }
    return FALSE;
}

static int func_ov011_02166580(SBServer *server) {
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

static u8 func_ov011_021665f0(int index, int a1) {
    DWCNodeInfo *peer = func_ov011_02168270(index);

    if (peer == NULL) {
        return 0xff;
    }
    return peer->aid;
}

static u32 func_ov011_02166604(int excludeSelf) {
    u32 mask = 0;
    int i;

    for (i = 1; i < func_ov011_02168024(); i++) {
        mask |= 1 << func_ov011_02168250(i)->aid;
    }
    if (excludeSelf == 0) {
        mask |= 1 << func_ov011_02167f6c()->self.aid;
    }
    return mask;
}

static void func_ov011_02166648(void) {
    u64 now;

    func_ov011_02167f6c()->unk_62e = 0;
    now = OS_GetTick();
    func_ov011_02167f6c()->unk_670 = now;
}

static int func_ov011_02166670(int result) {
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
    func_ov011_02162880(type, code - 81000);
    return result;
}

static int func_ov011_021666c4(int result) {
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
    func_ov011_02162880(type, code - 85000);
    return result;
}

static int func_ov011_0216672c(int result) {
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
    switch (func_ov011_021603a4()) {
    case 2:
        DWCi_StopLogin(type, code - 64000);
        break;
    case 4:
        func_ov011_02162880(type, code - 84000);
        break;
    default:
        DWCi_SetError(type, code - 94000);
        break;
    }
    return result;
}

static int func_ov011_021667b8(int result) {
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
    func_ov011_02162880(type, code - 86000);
    return result;
}

static int func_ov011_021667f8(int result) {
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
        func_ov011_02162880(type, code);
    }
    return result;
}

static int func_ov011_02166830(int result) {
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
        func_ov011_02162880(type, code - 87000);
    }
    return result;
}

static void func_ov011_0216689c(ServerBrowser *sb, SBCallbackReason reason, SBServer *server, void *instance) {
    int index;
    int count;
    SBServer *chosen;
    SBServer *host;
    int best;
    DWCNodeInfo peer;

    sMatchState.unk_14++;
    if (func_ov011_02167f6c()->unk_5f0 != 0) {
        if (reason == sbc_updatecomplete || reason == sbc_queryerror) {
            func_ov011_02167f6c()->unk_5f0 = 0;
        } else {
            u64 now = OS_GetTick();

            func_ov011_02167f6c()->unk_5f0 = now + OS_MilliSecondsToTicks(30000);
        }
    }
    switch (reason) {
    case 0:
        func_ov011_02166e48(server);
        break;
    case 4:
        func_ov011_02167f6c()->unk_5f0 = 0;
        for (index = 0; index < ServerBrowserCount(sb); index++) {
            server = ServerBrowserGetServer(sb, index);
            if (func_ov011_02166580(server) == 0) {
                ServerBrowserRemoveServer(sb, server);
                index--;
            }
        }
        switch (func_ov011_02167f6c()->state) {
        case 0x16:
            for (index = 0; index < ServerBrowserCount(sb); index++) {
                server = ServerBrowserGetServer(sb, index);
                if (func_ov011_02167f6c()->unk_5d8 != 0 &&
                    func_ov011_02167f6c()->unk_5d8 == SBServerGetPublicInetAddress(server) &&
                    func_ov011_02167f6c()->unk_5d4 != 0 &&
                    func_ov011_02167f6c()->unk_5d4 == SBServerGetPublicQueryPort(server)) {
                    break;
                }
            }
            if (index < ServerBrowserCount(sb)) {
                MI_CpuFill8(&peer, 0, 0x28);
                peer.profileId = func_ov011_02167f6c()->profileId;
                peer.aid = 0xff;
                peer.publicIp = func_ov011_02167f6c()->unk_5d8;
                peer.publicPort = func_ov011_02167f6c()->unk_5d4;
                peer.localIp = SBServerGetPrivateInetAddress(server);
                peer.localPort = SBServerGetPrivateQueryPort(server);
                peer.unk_17 = SBServerHasPrivateAddress(server);
                MI_CpuCopy8(&func_ov011_02167f6c()->userData, &peer.unk_24, sizeof(u32));
                if (func_ov011_02167f6c()->matchType != 0 && func_ov011_02167f6c()->matchType != 1) {
                    if (func_ov011_02167f6c()->matchType == 2) {
                        peer.aid = func_ov011_021682ec();
                    } else if (func_ov011_02167f6c()->matchType != 3) {
                        break;
                    }
                }
                func_ov011_02168030(&peer);
                func_ov011_02167c28();
                break;
            }
            func_ov011_02167f6c()->unk_5e4 = 2;
            {
                u64 now = OS_GetTick();

                func_ov011_02167f6c()->unk_5e8 = now;
            }
            break;
        case 2:
            func_ov011_02166e4c(1);
            func_ov011_02166f8c();
            if (ServerBrowserCount(sb) != 0) {
                host = ServerBrowserGetServer(sb, 0);
                func_ov011_02168374();
                func_ov011_02167f6c()->self.profileId = SBServerGetIntValueA(host, "dwc_pid", 0);
                func_ov011_02167f6c()->self.aid = 0xff;
                func_ov011_02167f6c()->self.publicIp = SBServerGetPublicInetAddress(host);
                func_ov011_02167f6c()->self.publicPort = SBServerGetPublicQueryPort(host);
                func_ov011_02167f6c()->self.localIp = SBServerGetPrivateInetAddress(host);
                func_ov011_02167f6c()->self.localPort = SBServerGetPrivateQueryPort(host);
                func_ov011_02167f6c()->self.unk_17 = SBServerHasPrivateAddress(host);
                func_ov011_02167924(func_ov011_02167f6c()->self.profileId);
            } else {
                func_ov011_02167f6c()->unk_5e4 = 2;
                {
                    u64 now = OS_GetTick();

                    func_ov011_02167f6c()->unk_5e8 = now;
                }
                func_ov011_02168374();
            }
            break;
        case 4:
            while (ServerBrowserCount(sb) != 0) {
                server = ServerBrowserGetServer(sb, 0);
                if (SBServerGetPublicInetAddress(server) == func_ov011_02167f6c()->self.publicIp) {
                    if (SBServerGetPublicQueryPort(server) == func_ov011_02167f6c()->self.publicPort) {
                        break;
                    }
                }
                ServerBrowserRemoveServer(sb, server);
            }
            if (ServerBrowserCount(sb) != 0) {
                int profileId = SBServerGetIntValueA(ServerBrowserGetServer(sb, 0), "dwc_pid", 0);

                if (func_ov011_02167f6c()->matchType == 1) {
                    func_ov011_02167f6c();
                    if (profileId != func_ov011_02167f6c()->unk_67c || func_ov011_02166e4c(0) == 0) {
                        if (func_ov011_02164b78(func_ov011_02165040(func_ov011_02167f6c()->unk_67c)) == 0) {
                            func_ov011_02167f78(3);
                            func_ov011_02168374();
                            func_ov011_02164b78(func_ov011_02164d64(0, 0, func_ov011_02167f6c()->unk_67c));
                        }
                        break;
                    }
                }
                func_ov011_02167f78(5);
                ServerBrowserGetServer(sb, 0);
                func_ov011_021667b8(func_ov011_021635a0(0, 0, &func_ov011_02167f6c()->self));
            } else {
                func_ov011_02167f6c()->unk_5e4 = 2;
                {
                    u64 now = OS_GetTick();

                    func_ov011_02167f6c()->unk_5e8 = now;
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
                func_ov011_02168374();
                func_ov011_02167f6c()->self.profileId = SBServerGetIntValueA(chosen, "dwc_pid", 0);
                func_ov011_02167f6c()->self.aid = 0xff;
                func_ov011_02167f6c()->self.publicIp = SBServerGetPublicInetAddress(chosen);
                func_ov011_02167f6c()->self.publicPort = SBServerGetPublicQueryPort(chosen);
                func_ov011_02167f6c()->self.localIp = SBServerGetPrivateInetAddress(chosen);
                func_ov011_02167f6c()->self.localPort = SBServerGetPrivateQueryPort(chosen);
                func_ov011_02167f6c()->self.unk_17 = SBServerHasPrivateAddress(chosen);
                func_ov011_02167f6c()->matchType = SBServerGetIntValueA(chosen, "dwc_mtype", -1);
                func_ov011_0216156c(func_ov011_021685ec(), SBServerGetIntValueA(chosen, "dwc_pid", 0),
                                    func_ov011_02167f6c()->notifyFunc, func_ov011_02167f6c()->notifyParam,
                                    func_ov011_02167f6c()->newClientCallback, func_ov011_02167f6c()->newClientParam,
                                    (DWCAttemptFunc)func_ov011_02167f6c()->checkFunc, &func_ov011_02167f6c()->userData,
                                    (void *)func_ov011_02167f6c()->unk_7c0);
            } else {
                func_ov011_02167f6c()->unk_5e4 = 2;
                {
                    u64 now = OS_GetTick();

                    func_ov011_02167f6c()->unk_5e8 = now;
                }
            }
            break;
        }
        break;
    case 5:
        break;
    }
    sMatchState.unk_14--;
}

static void func_ov011_02166e48(SBServer *server) {
}

// How often each of the first servers is picked
static const int sMatchWeights[6] = { 3, 3, 2, 2, 1, 1 };

static BOOL func_ov011_02166e4c(int sortAfter) {
    BOOL removed = FALSE;
    int i;

    for (i = 0; i < ServerBrowserCount(func_ov011_02167f6c()->sb); i++) {
        void *server = ServerBrowserGetServer(func_ov011_02167f6c()->sb, i);

        if (func_ov011_02167f6c()->matchType == 0) {
            int profileId = SBServerGetIntValueA(server, "dwc_pid", 0);
            BOOL skip = FALSE;

            if (func_ov011_02168270(profileId) != 0) {
                ServerBrowserRemoveServer(func_ov011_02167f6c()->sb, server);
                i--;
                skip = TRUE;
            }
            if (skip) {
                continue;
            }
        }
        if (func_ov011_02167f6c()->evalCallback != 0) {
            int rank = func_ov011_02167f6c()->evalCallback(i, func_ov011_02167f6c()->evalParam);

            if (rank > 0) {
                if (rank > 0x7fffff) {
                    rank = 0x7fffff;
                }
                SBServerAddIntKeyValue(server, "dwc_eval", DWCi_GetMathRand32(0x100) | (rank << 8));
            } else {
                ServerBrowserRemoveServer(func_ov011_02167f6c()->sb, server);
                i--;
                removed = TRUE;
                continue;
            }
        } else {
            SBServerAddIntKeyValue(server, "dwc_eval", DWCi_GetMathRand32(0x80));
        }
    }
    if (sortAfter != 0 && ServerBrowserCount(func_ov011_02167f6c()->sb) != 0) {
        ServerBrowserSortA(func_ov011_02167f6c()->sb, 0, "dwc_eval", 0);
    }
    if (removed && ServerBrowserCount(func_ov011_02167f6c()->sb) == 0) {
        return FALSE;
    }
    return TRUE;
}

static void func_ov011_02166f8c(void) {
    int best = 0;
    int total = 0;
    int i;
    u32 pick;
    u32 cumulative[6];

    if (ServerBrowserCount(func_ov011_02167f6c()->sb) > 1) {
        for (i = 0; i < ServerBrowserCount(func_ov011_02167f6c()->sb); i++) {
            int eval = SBServerGetIntValueA(ServerBrowserGetServer(func_ov011_02167f6c()->sb, i), "dwc_eval", -1);

            if (eval > best) {
                best = eval;
            }
            total += sMatchWeights[i];
        }
        pick = DWCi_GetMathRand32(100);
        for (i = 0; i < ServerBrowserCount(func_ov011_02167f6c()->sb); i++) {
            int previous;

            if (i == ServerBrowserCount(func_ov011_02167f6c()->sb) - 1) {
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
        SBServerAddIntKeyValue(ServerBrowserGetServer(func_ov011_02167f6c()->sb, i), "dwc_eval", best);
        ServerBrowserSortA(func_ov011_02167f6c()->sb, 0, "dwc_eval", 0);
    }
}

static void func_ov011_021670a4(int keyId, QR2Buffer *outBuf, void *userData) {
    int count;

    switch (keyId) {
    case 8:
        count = func_ov011_02168024() - 1;
        if (count < 0) {
            count = 0;
        }
        qr2_buffer_add_int(outBuf, count);
        return;
    case 10:
        qr2_buffer_add_int(outBuf, func_ov011_02167f6c()->unk_17);
        return;
    case 0x32:
        qr2_buffer_add_int(outBuf, func_ov011_02167f6c()->profileId);
        return;
    case 0x33:
        qr2_buffer_add_int(outBuf, func_ov011_02167f6c()->matchType);
        return;
    case 0x34:
        qr2_buffer_add_int(outBuf, 0x5a);
        return;
    case 0x35:
        qr2_buffer_add_int(outBuf, 1);
        return;
    case 0x36:
        qr2_buffer_add_int(outBuf, func_ov011_02167fc4() == 2 ? func_ov011_02167f6c()->groupId : 0);
        return;
    case 0x37:
        qr2_buffer_add_int(outBuf, func_ov011_02167f6c()->hostState);
        return;
    case 0x38:
        qr2_buffer_add_int(outBuf, func_ov011_02167f6c()->suspend);
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

static void func_ov011_021671b8(int keyId, int index, QR2Buffer *outBuf, void *userData) {
}

static void func_ov011_021671bc(int keyId, int index, QR2Buffer *outBuf, void *userData) {
}

static void func_ov011_021671c0(int keyType, QR2KeyBuffer *keyBuffer, void *userData) {
    int i;

    switch (keyType) {
    case 0:
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
    case 1:
        break;
    case 2:
        break;
    }
}

static int func_ov011_0216723c(int keyType, void *userData) {
    return 0;
}

static void func_ov011_02167240(int error, const char *errorMessage, void *userData) {
    func_ov011_0216672c(error);
}

static void func_ov011_02167248(u32 ip, u16 port, void *userData) {
    func_ov011_02167f6c()->unk_5d8 = ip;
    func_ov011_02167f6c()->unk_5d4 = port;
}

static void func_ov011_02167264(int cookie, void *userData) {
    if (func_ov011_02160408() == 0 && func_ov011_02167f6c()->pending.profileId != 0) {
        return;
    }
    if (func_ov011_02167f6c()->state == 1) {
        if (func_ov011_021686a0() == 0) {
            return;
        }
        func_ov011_02167f78(5);
    } else if (func_ov011_02167f6c()->state != 5 && func_ov011_02167f6c()->state != 0xe) {
        return;
    }
    if (cookie == func_ov011_02167f6c()->unk_5fc) {
        func_ov011_02167f6c()->unk_5f8++;
    } else {
        func_ov011_02167f6c()->unk_5f8 = 0;
        func_ov011_02167f6c()->unk_5fc = cookie;
    }
    func_ov011_02167f6c()->unk_600 = 0;
    if (func_ov011_021667b8(func_ov011_021635a0(1, cookie, &func_ov011_02167f6c()->self)) == 0) {
        func_ov011_02167f6c()->unk_6d8 = 0xff;
    }
}

void func_ov011_0216731c(const u8 *data, int size) {
    MatchPacketHeader header;
    u32 payload[0x20];

    if (func_ov011_021603a4() != 4) {
        if (func_ov011_021603a4() != 5) {
            return;
        }
        if (func_ov011_02167f6c()->mode != 2 && func_ov011_02167f6c()->mode != 3) {
            return;
        }
    }
    MI_CpuCopy8(data, &header, sizeof(MatchPacketHeader));
    if (header.version == 0x5a && size == header.size + sizeof(MatchPacketHeader)) {
        MI_CpuCopy8(data + sizeof(MatchPacketHeader), payload, header.size);
        func_ov011_02163a30(header.command, header.profileId, header.ip, header.port, payload, header.size >> 2);
    }
}

static void func_ov011_0216738c(const char *data, int size, void *userData) {
    MatchPacketHeader header;
    u32 payload[0x20];
    int offset = 0;

    if (func_ov011_021603a4() != 4) {
        if (func_ov011_021603a4() != 5) {
            return;
        }
        if (func_ov011_02167f6c()->mode != 2 && func_ov011_02167f6c()->mode != 3) {
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
        if (func_ov011_02163a30(header.command, header.profileId, header.ip, header.port, payload, header.size >> 2) ==
            0) {
            return;
        }
        offset += header.size + 0x14;
    } while (offset + 0x14 <= size);
}

static void func_ov011_02167424(int state, void *userData) {
}

static void func_ov011_02167428(int result, int gameSocket, SocAddrIn *address, void *userData) {
    MatchTarget *connection = userData;
    u8 buffer[0x18];
    int status;
    int mapped;

    if ((func_ov011_02167f6c()->state == 5 || func_ov011_02167f6c()->state == 0xe) && connection != NULL) {
        if (result == 0) {
            if (connection->kind != 0) {
                func_ov011_02167f6c()->self.hostIp = address->addr;
                func_ov011_02167f6c()->self.hostPort = MATCH_SWAP16(address->port);
                func_ov011_02167f6c()->unk_5f8 = 0;
                func_ov011_02167f6c()->unk_5fc = 0;
                func_ov011_02167f6c()->unk_600 = 0;
                func_ov011_02167f78(func_ov011_02167f6c()->state == 0xe ? 0xf : 6);
                func_ov011_02167f6c()->retries = 0;
                func_ov011_0216261c(buffer, 0x18);
                status = func_ov011_0217d1a0(
                    func_ov011_02167f6c()->fm->socket, 0,
                    func_ov011_0217ed14(func_ov011_02167f6c()->self.hostIp, func_ov011_02167f6c()->self.hostPort, 0),
                    buffer, -1, 5000, func_ov011_02167f6c()->callbacks, 0);
                if (func_ov011_02167f6c()->unk_5a4 != 0 && status != 0) {
                    func_ov011_02168270(func_ov011_02167f6c()->self.profileId)->unk_18++;
                    func_ov011_02168468();
                    func_ov011_02167f78(1);
                    return;
                }
                if (status == 1) {
                    func_ov011_02166830(status);
                    return;
                }
                if (status == 0) {
                    return;
                }
                if (func_ov011_02165164(func_ov011_02167f6c()->self.profileId) != 0) {
                    return;
                }
            } else {
                if (address != NULL) {
                    func_ov011_02167f6c()->self.hostIp = address->addr;
                    func_ov011_02167f6c()->self.hostPort = MATCH_SWAP16(address->port);
                }
                {
                    u64 now = OS_GetTick();

                    func_ov011_02167f6c()->unk_608 = now;
                }
                func_ov011_02167f78(6);
            }
        } else if (connection->cookie != 0) {
            if (func_ov011_02167f6c()->unk_5a4 != 0) {
                func_ov011_02168270(func_ov011_02167f6c()->self.profileId)->unk_18++;
                func_ov011_02168468();
                func_ov011_02167f78(1);
                return;
            }
            mapped = func_ov011_021667f8(result);
            if (mapped != 2 && mapped != 1 && mapped != 3) {
                return;
            }
            {
                if (connection->kind == 0) {
                    if (mapped == 1 || mapped == 3 || (mapped == 2 && connection->unk_01 >= 1)) {
                        NNCancel(func_ov011_02167f6c()->target.cookie);
                        connection->cookie = 0;
                        if (func_ov011_02167718(0) != 0) {
                            if (func_ov011_02165194(func_ov011_02167f6c()->self.profileId) != 0) {
                                return;
                            }
                        }
                    } else {
                        connection->unk_01++;
                        if (func_ov011_021667b8(func_ov011_0216373c(connection)) != 0) {
                            return;
                        }
                    }
                } else {
                    u64 now = OS_GetTick();

                    func_ov011_02167f6c()->unk_600 = now;
                    if (mapped == 1 || mapped == 3 || (mapped == 2 && func_ov011_02167f6c()->unk_5f8 >= 1)) {
                        NNCancel(func_ov011_02167f6c()->target.cookie);
                        connection->cookie = 0;
                        if (func_ov011_02167f6c()->matchType == 3 || func_ov011_02167f6c()->matchType == 2) {
                            if (func_ov011_02167718(1) == 0) {
                                return;
                            }
                        } else if (func_ov011_02167718(0) == 0) {
                            return;
                        }
                        func_ov011_02167f6c()->unk_5f8 = 0;
                        func_ov011_02167f6c()->unk_5fc = 0;
                        func_ov011_02167f6c()->unk_600 = 0;
                        if (func_ov011_02165164(func_ov011_02167f6c()->self.profileId) != 0) {
                            return;
                        }
                    }
                }
            }
        }
    }
}

static BOOL func_ov011_02167718(int skip) {
    if (skip != 0) {
        return TRUE;
    }
    if (func_ov011_02167f6c()->matchType != 3) {
        func_ov011_02167f6c()->unk_5f9++;
    }
    if (func_ov011_02167f6c()->matchType == 3 || func_ov011_02167f6c()->unk_798 != 0 ||
        func_ov011_02167f6c()->unk_5f9 >= 5) {
        func_ov011_02162880(6, -86420);
        return FALSE;
    }
    return TRUE;
}

static BOOL func_ov011_02167778(void) {
    int count;
    int i;

    func_ov011_02167f6c();
    count = func_ov011_02168024();
    for (i = 1; i < count; i++) {
        DWCNodeInfo *peer = func_ov011_02168250(i);
        u32 data = func_ov011_02160344();
        u32 type;

        if (func_ov011_02167f6c()->unk_788 & (1 << peer->aid)) {
            type = 0x55;
        } else {
            type = 0x52;
        }
        if (func_ov011_02164b78(
                func_ov011_021637d8(type, peer->profileId, peer->publicIp, peer->publicPort, &data, 1)) != 0) {
            return FALSE;
        }
    }
    func_ov011_02166648();
    return TRUE;
}

BOOL func_ov011_021677e8(void) {
    BOOL result = FALSE;
    DWCNodeInfo *peer = func_ov011_02168250(0);

    if (peer != NULL && peer->profileId == func_ov011_02167f6c()->profileId) {
        result = TRUE;
    }
    return result;
}

static u32 func_ov011_02167810(void) {
    MatchControl *match = func_ov011_02167f6c();

    return match->profileId + DWCi_GetMathRand32(10000000) + 100000000;
}

static void func_ov011_02167838(MatchCheckFunc a, const void *userData, u32 c) {
    func_ov011_02167f6c()->checkFunc = a;
    if (userData != NULL) {
        memcpy(&func_ov011_02167f6c()->userData, userData, sizeof(int));
    } else {
        memset(&func_ov011_02167f6c()->userData, 0, sizeof(int));
    }
    func_ov011_02167f6c()->unk_7c0 = c;
}

static void func_ov011_0216788c(int isHost) {
    int mode;

    if (isHost != 0) {
        func_ov011_02167f6c()->groupId = func_ov011_02167810();
        func_ov011_02168224()->aid = 0;
        func_ov011_02167f6c()->unk_7c4 = 1;
    }
    func_ov011_02167f6c()->mode = 2;
    func_ov011_02167f78(0xd);
    func_ov011_02167fa0(2);
    func_ov011_02167f6c()->unk_5dc = 0;
    func_ov011_02167f6c()->unk_18 = 0;
    if (func_ov011_02167f6c()->matchType == 0) {
        mode = 3;
    } else {
        MatchControl *match = func_ov011_02167f6c();

        mode = 4;
        if (match->matchType != 1) {
            mode = 6;
        }
    }
    if (func_ov011_02167f6c()->matchType == 3) {
        func_ov011_02167f6c()->matchType = 2;
    }
    func_ov011_02166670(func_ov011_02162d80(mode));
    func_ov011_02167f6c()->unk_67c = 0;
}

static void func_ov011_02167924(int profileId) {
    func_ov011_02167f6c()->mode = 3;
    if (func_ov011_02167f6c()->sb == NULL) {
        func_ov011_02167f6c()->sb =
            ServerBrowserNewA(func_ov011_02167f6c()->gameName, func_ov011_02167f6c()->gameName,
                              func_ov011_02167f6c()->secretKey, 0, 0x14, 1, 0, func_ov011_0216689c, 0);
    }
    if (func_ov011_02167f6c()->sb != NULL || func_ov011_021666c4(5) == 0) {
        func_ov011_02167f78(3);
        if (func_ov011_02164b78(func_ov011_02164b98(func_ov011_02167f6c()->self.profileId, 0)) != 0) {
            return;
        }
    }
}

static BOOL func_ov011_021679c0(void) {
    return func_ov011_021602a0();
}

int func_ov011_021679c8(BOOL force) {
    u32 packet[3];
    int i;

    if (func_ov011_02160408() == 0) {
        return 1;
    }
    if (func_ov011_02167f6c()->state == 0x13 || func_ov011_02167f6c()->state == 0x14) {
        return 1;
    }
    if (force != 0 || 15000 < OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_7a0)) {
        u64 now;

        packet[0] = func_ov011_02167f6c()->profileId;
        packet[1] = func_ov011_02160344();
        packet[2] = func_ov011_02167f6c()->unk_7c8;
        for (i = 0; i < func_ov011_02168024(); i++) {
            DWCNodeInfo *peer = func_ov011_02168250(i);

            if (peer != func_ov011_02168224()) {
                if (func_ov011_02164b78(
                        func_ov011_021637d8(0x83, peer->profileId, peer->publicIp, peer->publicPort, packet, 3)) != 0) {
                    return -1;
                }
            }
        }
        now = OS_GetTick();
        func_ov011_02167f6c()->unk_7a0 = now;
    }
    return 1;
}

static BOOL func_ov011_02167ac4(u32 index, int set, int mode) {
    int count = 0;
    DWCNodeInfo *host = func_ov011_02168250(0);
    u32 oldFlags = func_ov011_02167f6c()->unk_1c;
    u32 flags;
    u32 packet[4];
    DWCNodeInfo *target;
    BOOL notify;
    int i;
    int hasSet;

    if (set != 0) {
        MatchControl *match = func_ov011_02167f6c();

        match->unk_1c = match->unk_1c | (1 << index);
    } else {
        MatchControl *match = func_ov011_02167f6c();

        match->unk_1c = match->unk_1c & ~(1 << index);
    }
    if (oldFlags != func_ov011_02167f6c()->unk_1c) {
        func_ov011_02162d80(-1);
    }
    flags = func_ov011_02167f6c()->unk_1c;
    if (func_ov011_02167f0c() == 0) {
        flags &= ~(1 << func_ov011_021602c0());
    }
    hasSet = func_ov011_02160fd8();
    notify = TRUE;
    if ((set == 0 || hasSet == 0) && (set != 0 || hasSet != 0)) {
        notify = FALSE;
    }
    if (mode == 1) {
        target = func_ov011_021682ac(index);

        packet[0] = func_ov011_02167f6c()->profileId;
        packet[1] = 1;
        count += 2;
        if (notify && func_ov011_02167f6c()->unk_30 != 0) {
            packet[count++] = set;
            packet[count++] = flags;
        }
        if (func_ov011_02164b78(func_ov011_021637d8(0x82, target->profileId, target->publicIp, target->publicPort,
                                                    packet, count)) != 0) {
            return FALSE;
        }
    }
    if (notify && func_ov011_02167f6c()->unk_30 != 1) {
        func_ov011_02167f6c()->unk_30 = 1;
        packet[0] = func_ov011_02167f6c()->profileId;
        packet[1] = 1;
        packet[2] = set;
        packet[3] = flags;
        for (i = 0; i < func_ov011_02168024(); i++) {
            DWCNodeInfo *peer = func_ov011_02168250(i);

            if (peer != host) {
                if (func_ov011_02164b78(
                        func_ov011_021637d8(0x82, peer->profileId, peer->publicIp, peer->publicPort, packet, 4)) != 0) {
                    return FALSE;
                }
            }
        }
    }
    return TRUE;
}

static BOOL func_ov011_02167c28(void) {
    if (func_ov011_02167f6c()->sb != NULL) {
        ServerBrowserClear(func_ov011_02167f6c()->sb);
    }
    if (func_ov011_02167f6c()->unk_798 != 0) {
        func_ov011_02167f78(0xc);
    } else if (func_ov011_02167f6c()->matchType == 0) {
        func_ov011_02167f78(2);
    } else if (func_ov011_02167f6c()->matchType == 1) {
        func_ov011_02167f78(3);
        return func_ov011_02164b78(func_ov011_02164d64(0, 1, 0)) == 0;
    } else if (func_ov011_02167f6c()->matchType == 2) {
        func_ov011_02167f78(0xd);
        func_ov011_02167f6c()->unk_0d = 1;
        return TRUE;
    } else if (func_ov011_02167f6c()->matchType == 3) {
        MatchControl *match = func_ov011_02167f6c();

        if (gpIsBuddy(match->gp, func_ov011_02167f6c()->self.profileId) != 0) {
            func_ov011_02167f78(3);
            if (func_ov011_02164b78(func_ov011_02164b98(func_ov011_02167f6c()->self.profileId, 0)) != 0) {
                return FALSE;
            }
        }
        return TRUE;
    }
    func_ov011_02167f6c()->unk_67c = 0;
    return func_ov011_021666c4(func_ov011_0216322c(func_ov011_02167f6c()->unk_67c)) == 0;
}

static void func_ov011_02167d24(int a, MatchCallback b, int c) {
    u64 now;

    func_ov011_02167f6c()->unk_30 = 0;
    func_ov011_02167f6c()->unk_2c = a;
    func_ov011_02167f6c()->unk_34 = b;
    func_ov011_02167f6c()->unk_38 = c;
    func_ov011_02167f6c()->unk_20 = 0;
    now = OS_GetTick();
    func_ov011_02167f6c()->unk_3c = now;
    func_ov011_02167f6c()->unk_48 = 1;
}

static BOOL func_ov011_02167d6c(void) {
    int value;
    int hasSet;

    if (func_ov011_02167f6c()->unk_48 == 0) {
        return FALSE;
    }
    if (func_ov011_02167f6c()->unk_44 != 0) {
        func_ov011_02167d24(func_ov011_02167f6c()->unk_2c, func_ov011_02167f6c()->unk_34,
                            func_ov011_02167f6c()->unk_38);
        func_ov011_02167f6c()->unk_44 = 0;
    }
    value = func_ov011_02167f6c()->unk_2c;
    if (value == 0xff) {
        return FALSE;
    }
    hasSet = func_ov011_02160fd8();
    if ((value != 0 && hasSet != 0) || (value == 0 && hasSet == 0)) {
        func_ov011_02168654(0);
        return TRUE;
    }
    func_ov011_02167de0(value);
    return TRUE;
}

static BOOL func_ov011_02167de0(int value) {
    u32 packet[4];
    DWCNodeInfo *self;
    DWCNodeInfo *host;

    if (func_ov011_02167fc4() == 2) {
        return func_ov011_02167ac4(func_ov011_02168224()->aid, value, 0);
    }
    if (func_ov011_02167f6c()->state != 1) {
        return TRUE;
    }
    if (func_ov011_02167f6c()->unk_30 != 0) {
        if (OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_20) < func_ov011_02167f6c()->unk_7cc) {
            return TRUE;
        }
    }
    if (3000 < OS_TicksToMilliSeconds(OS_GetTick() - func_ov011_02167f6c()->unk_20)) {
        u64 now;

        self = func_ov011_02168224();
        host = func_ov011_02168250(0);
        packet[0] = func_ov011_02167f6c()->profileId;
        packet[1] = 0;
        packet[2] = value;
        packet[3] = self->aid;
        if (func_ov011_02164b78(
                func_ov011_021637d8(0x82, host->profileId, host->publicIp, host->publicPort, packet, 4)) != 0) {
            return FALSE;
        }
        now = OS_GetTick();
        func_ov011_02167f6c()->unk_20 = now;
        func_ov011_02167f6c()->unk_7cc = 3000;
    }
    return TRUE;
}

static BOOL func_ov011_02167f0c(void) {
    if (func_ov011_02160408() != 0) {
        if (func_ov011_02167f6c()->state == 0xd || func_ov011_02167f6c()->state == 0x16) {
            return TRUE;
        }
        return FALSE;
    }
    return TRUE;
}

BOOL func_ov011_02167f3c(void) {
    int state = func_ov011_02167f6c()->state;

    if (state == 9 || state == 10 || state == 11) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov011_02167f58(MatchControl *match) {
    sMatchState.control = match;
    sMatchState.clock = OS_GetTick();
}

static MatchControl *func_ov011_02167f6c(void) {
    return sMatchState.control;
}

static void func_ov011_02167f78(int state) {
    if (sMatchState.control->state != state) {
        func_ov011_0216876c(sMatchState.control->state, state);
    }
    sMatchState.control->state = state;
}

static void func_ov011_02167fa0(int mode) {
    sMatchState.control->hostState = mode;
    if (func_ov011_02167f6c()->qr2 != NULL) {
        qr2_send_statechanged(func_ov011_02167f6c()->qr2);
    }
}

static u8 func_ov011_02167fc4(void) {
    return func_ov011_02167f6c()->hostState;
}

static BOOL func_ov011_02167fd0(void) {
    if (func_ov011_02167f6c()->unk_7b0 != 1) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov011_02167fe8(void) {
    int i;

    for (i = 0; i < func_ov011_02168024(); i++) {
        func_ov011_02169000(func_ov011_02168250(i)->aid);
    }
}

static void func_ov011_0216800c(void) {
    MI_CpuFill8(&func_ov011_02167f6c()->unk_50, 0, 0x504);
}

int func_ov011_02168024(void) {
    return func_ov011_02167f6c()->unk_50;
}

static BOOL func_ov011_02168030(DWCNodeInfo *peer) {
    int i;

    if (func_ov011_02167f6c()->unk_50 == 0x20) {
        return FALSE;
    }
    for (i = func_ov011_02167f6c()->unk_50; i > 0; i--) {
        func_ov011_02167f6c()->unk_54[i] = func_ov011_02167f6c()->unk_54[i - 1];
    }
    func_ov011_02167f6c()->unk_54[0] = *peer;
    func_ov011_02167f6c()->unk_50++;
    return TRUE;
}

static BOOL func_ov011_021680a4(DWCNodeInfo *peer) {
    if (func_ov011_02167f6c()->unk_50 == 0x20) {
        return FALSE;
    }
    func_ov011_02167f6c()->unk_54[func_ov011_02167f6c()->unk_50] = *peer;
    func_ov011_02167f6c()->unk_50++;
    return TRUE;
}

static BOOL func_ov011_021680e4(int index) {
    int i;

    if (func_ov011_02167f6c()->unk_50 == 0) {
        return FALSE;
    }
    for (i = 0; i < func_ov011_02167f6c()->unk_50; i++) {
        if (index == func_ov011_02167f6c()->unk_54[i].aid) {
            break;
        }
    }
    if (i == func_ov011_02167f6c()->unk_50) {
        return FALSE;
    }
    for (; i < func_ov011_02167f6c()->unk_50 - 1; i++) {
        func_ov011_02167f6c()->unk_54[i] = func_ov011_02167f6c()->unk_54[i + 1];
    }
    func_ov011_02167f6c()->unk_50--;
    return TRUE;
}

static BOOL func_ov011_02168184(int id) {
    int i;

    if (func_ov011_02167f6c()->unk_50 == 0) {
        return FALSE;
    }
    for (i = 0; i < func_ov011_02167f6c()->unk_50; i++) {
        if (id == func_ov011_02167f6c()->unk_54[i].profileId) {
            break;
        }
    }
    if (i == func_ov011_02167f6c()->unk_50) {
        return FALSE;
    }
    for (; i < func_ov011_02167f6c()->unk_50 - 1; i++) {
        func_ov011_02167f6c()->unk_54[i] = func_ov011_02167f6c()->unk_54[i + 1];
    }
    func_ov011_02167f6c()->unk_50--;
    return TRUE;
}

static DWCNodeInfo *func_ov011_02168224(void) {
    return func_ov011_02168270(func_ov011_02167f6c()->profileId);
}

static DWCNodeInfo *func_ov011_02168238(void) {
    if (func_ov011_02167f6c()->unk_50 >= 1) {
        return func_ov011_02167f6c()->unk_54;
    }
    return NULL;
}

DWCNodeInfo *func_ov011_02168250(int index) {
    if (index < func_ov011_02167f6c()->unk_50) {
        return &func_ov011_02167f6c()->unk_54[index];
    }
    return NULL;
}

static DWCNodeInfo *func_ov011_02168270(int id) {
    int i;

    for (i = 0; i < func_ov011_02167f6c()->unk_50; i++) {
        if (id == func_ov011_02167f6c()->unk_54[i].profileId) {
            return &func_ov011_02167f6c()->unk_54[i];
        }
    }
    return NULL;
}

static DWCNodeInfo *func_ov011_021682ac(int index) {
    int i;

    for (i = 0; i < func_ov011_02167f6c()->unk_50; i++) {
        if (index == func_ov011_02167f6c()->unk_54[i].aid) {
            return &func_ov011_02167f6c()->unk_54[i];
        }
    }
    return NULL;
}

static u8 func_ov011_021682ec(void) {
    u8 index;
    int i;

    if (func_ov011_02167f6c()->unk_50 == 0x20) {
        return 0xff;
    }
    for (index = 0; index < 0x20; index++) {
        for (i = 0; i < func_ov011_02167f6c()->unk_50; i++) {
            if (index == func_ov011_02167f6c()->unk_54[i].aid) {
                break;
            }
        }
        if (i == func_ov011_02167f6c()->unk_50) {
            return index;
        }
    }
    return 0xff;
}

static BOOL func_ov011_02168348(int id) {
    DWCNodeInfo *peer = func_ov011_02168270(id);

    if (peer == NULL) {
        return FALSE;
    }
    if (peer == func_ov011_02168224()) {
        return FALSE;
    }
    if (func_ov011_021604b0(peer->aid) == 0) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov011_02168374(void) {
    func_ov011_0216839c(&func_ov011_02167f6c()->self);
}

static BOOL func_ov011_02168388(void) {
    return func_ov011_021683a8(&func_ov011_02167f6c()->self);
}

static void func_ov011_0216839c(void *address) {
    MI_CpuFill8(address, 0, 0x28);
}

static BOOL func_ov011_021683a8(DWCNodeInfo *peer) {
    if (peer->profileId != 0) {
        return TRUE;
    }
    return FALSE;
}

static u64 func_ov011_021683b8(void) {
    DWCNodeInfo *self = func_ov011_02168224();
    u64 time = OS_GetTick() + OS_MilliSecondsToTicks(3000);
    int selfIndex = 0;
    int active = 0;
    int i;

    if (func_ov011_02168024() > 2) {
        for (i = 1; i < func_ov011_02168024(); i++) {
            DWCNodeInfo *peer = func_ov011_02168250(i);

            if (self == peer) {
                selfIndex = i;
            } else {
                func_ov011_021604b0(peer->aid);
                if (func_ov011_021604b0(peer->aid) != 0) {
                    active++;
                }
            }
        }
    }
    {
        u64 activeTicks = (u64)active * OS_MilliSecondsToTicks(3000);
        u64 waitTicks;

        time += activeTicks;
        waitTicks = OS_MilliSecondsToTicks((func_ov011_02168024() - 1 - selfIndex) * 6000);
        time += waitTicks;
    }
    return time;
}

static BOOL func_ov011_02168468(void) {
    if (func_ov011_02167f6c()->unk_5a4 == 0) {
        return FALSE;
    }
    func_ov011_02167f6c()->unk_5a4 = 0;
    func_ov011_02167f6c()->unk_5a8 = func_ov011_02167f6c()->state;
    if (func_ov011_02168388() != 0) {
        func_ov011_02167f6c()->unk_5ac = func_ov011_02167f6c()->self;
    } else {
        MI_CpuFill8(&func_ov011_02167f6c()->unk_5ac, 0, 0x28);
    }
    if (func_ov011_02167f6c()->target.cookie != 0) {
        NNCancel(func_ov011_02167f6c()->target.cookie);
        func_ov011_02167f6c()->target.cookie = 0;
        func_ov011_02167f6c()->unk_5f8 = 0;
        func_ov011_02167f6c()->unk_5fc = 0;
        func_ov011_02167f6c()->unk_600 = 0;
    }
    if (func_ov011_02167f6c()->state == 3 || func_ov011_02167f6c()->state == 5 || func_ov011_02167f6c()->state == 6 ||
        func_ov011_02167f6c()->state == 0xe || func_ov011_02167f6c()->state == 0xf) {
        if (func_ov011_02167f6c()->unk_67c != 0) {
            if (func_ov011_02164b78(func_ov011_02165040(0)) != 0) {
                func_ov011_02168374();
                return TRUE;
            }
        }
        func_ov011_02167f6c()->unk_18 = 0;
        func_ov011_02167f6c()->unk_5dc = 0;
    }
    func_ov011_02168374();
    return TRUE;
}

static int func_ov011_02168590(void) {
    return func_ov011_02167f6c()->unk_7c4;
}

static BOOL func_ov011_021685a0(void) {
    if (func_ov011_02167f6c()->unk_7d4 == 0) {
        return FALSE;
    }
    if (func_ov011_02160408() != 0) {
        if (func_ov011_02167f6c()->matchType == 1 || func_ov011_02167f6c()->matchType == 3 ||
            func_ov011_02167f6c()->matchType == 2) {
            if (func_ov011_02168590() == 0) {
                return TRUE;
            }
        }
    }
    return FALSE;
}

static int func_ov011_021685ec(void) {
    return func_ov011_02167f6c()->unk_7b0;
}

static void func_ov011_021685fc(void) {
    func_ov011_02167f6c()->unk_5a8 = 0;
    MI_CpuFill8(&func_ov011_02167f6c()->unk_5ac, 0, 0x28);
}

static void func_ov011_02168620(void) {
    if (func_ov011_02167f6c()->unk_2c != 0xff) {
        if (func_ov011_02167f6c()->unk_2c == 1) {
            MatchControl *match = func_ov011_02167f6c();

            match->unk_1c = func_ov011_02160344();
        } else {
            func_ov011_02167f6c()->unk_1c = 0;
        }
        func_ov011_02168654(0);
    }
}

static void func_ov011_02168654(int cancelled) {
    if (func_ov011_02167f6c()->unk_34 != 0) {
        func_ov011_02167f6c()->unk_34(cancelled, func_ov011_02167f6c()->unk_2c, func_ov011_02167f6c()->unk_38);
    }
    if (cancelled == 0) {
        qr2_send_statechanged(func_ov011_02167f6c()->qr2);
    }
    func_ov011_02167f6c()->unk_2c = 0xff;
    func_ov011_02167f6c()->unk_48 = 0;
}

static BOOL func_ov011_021686a0(void) {
    if (func_ov011_02167f6c()->unk_7b0 == 2) {
        return TRUE;
    }
    return FALSE;
}

static int func_ov011_021686b8(void) {
    return func_ov011_02168024();
}

u8 func_ov011_021686c0(const GPBuddyStatus *status) {
    u8 result = 0;
    char text[4] = { 0, 0, 0, 0 };

    if (DWC_GetCommonValueString("LCK", text, status->statusString, '/') > 0) {
        result = strtoul(text, NULL, 10);
    }
    return result;
}

static const char *func_ov011_021686f8(int code) {
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

static void func_ov011_0216876c(int oldState, int newState) {
    sMatchState.clock = OS_GetTick();
}

void func_ov011_02168780(int profileId) {
    func_ov011_02167f6c()->profileId = profileId;
}

void func_ov011_02168794(void) {
    qr2_register_keyA(0x32, "dwc_pid");
    qr2_register_keyA(0x33, "dwc_mtype");
    qr2_register_keyA(0x34, "dwc_mver");
    qr2_register_keyA(0x35, "dwc_eval");
    qr2_register_keyA(0x36, "dwc_groupid");
    qr2_register_keyA(0x37, "dwc_hoststate");
    qr2_register_keyA(0x38, "dwc_suspend");
}

static void func_ov011_021687ec(void) {
    NNCancel(func_ov011_02167f6c()->target.cookie);
    if (sMatchState.unk_00 == 0) {
        NNFreeNegotiateList();
    } else {
        sMatchState.unk_08 = 1;
    }
}
