// Overlay 31, named descriptively (no string names the file; its asserts name the infrared system NetIrcSys, and its
// table is a GFLNetDevTable like net_devwireless.c's): the GFL net device for infrared. It is the table of wrappers
// over NetIrcSys (net_irc.c), which the other files of the overlay carry down to the card's infrared chip. Overlay 31
// is four original files, probably in this order: this one, net_irc.c, irc_link.c and irc_spi.c; MWCC sorts a file's
// .bss by size, and the original's .bss order is only possible split so.
#include "field/net_devirc.h"
#include "field/net_irc.h"
#include "irc/irc_link.h"

static BOOL NetDevIrc_Init(HeapID heapId, void *sys, BOOL (*callback)(BOOL success), void *work);
static BOOL NetDevIrc_Begin(void (*callback)(BOOL ok));
static int NetDevIrc_Update(u16 connectBits);
static BOOL NetDevIrc_ReturnTrue10(int a0, int a1);
static BOOL NetDevIrc_End(void (*callback)(BOOL ok));
static BOOL NetDevIrc_IsRunning(void);
static BOOL NetDevIrc_IsIdle(void);
static BOOL NetDevIrc_Send(u8 *data, int size, int a2, GFLNetSendDoneFunc callback);
static BOOL NetDevIrc_SetRecvCallback(GFLNetRecvFunc callback);
static int NetDevIrc_GetNetId(void);
static int NetDevIrc_IsError(void);
static void NetDevIrc_Poll(void);
static BOOL NetDevIrc_CanSend(void);
static BOOL NetDevIrc_GetSendTurn(void);
static BOOL NetDevIrc_GetPeerSeen(void);
static BOOL NetDevIrc_IsLinked(void);
static BOOL NetDevIrc_IsConnectedSlot(void);
static BOOL NetDevIrc_IsInitialized(void);
static int NetDevIrc_GetConnectBits(void);
static BOOL NetDevIrc_GetUnkCF0(void);
static void NetDevIrc_StartLink(void);
static void NetDevIrc_SetTimeoutSlot(int frames);
static void NetDevIrc_SetPeerFilterSlot(void *filter);

static BOOL NetDevIrc_Init(HeapID heapId, void *sys, BOOL (*callback)(BOOL success), void *work) {
    GFLNetInitData *init = func_02042e84();

    NetIrc_Init(func_02042e84()->unk68.word, init->gameCommandBase);
    NetIrc_SetInitialized();
    return TRUE;
}

static BOOL NetDevIrc_Begin(void (*callback)(BOOL ok)) {
    IrcLink_Ping();
    return TRUE;
}

static int NetDevIrc_Update(u16 connectBits) {
    NetIrc_SendPending();
    return 0;
}

static BOOL NetDevIrc_ReturnTrue10(int a0, int a1) {
    return TRUE;
}

static BOOL NetDevIrc_End(void (*callback)(BOOL ok)) {
    NetIrc_Exit();
    return TRUE;
}

static BOOL NetDevIrc_IsRunning(void) {
    return NetIrc_IsInitialized();
}

static BOOL NetDevIrc_IsIdle(void) {
    if (!NetIrc_IsInitialized()) {
        return TRUE;
    }
    return FALSE;
}

static BOOL NetDevIrc_Send(u8 *data, int size, int a2, GFLNetSendDoneFunc callback) {
    NetIrc_Send(data, size, 0);
    if (callback != NULL) {
        callback(TRUE);
    }
    return TRUE;
}

static BOOL NetDevIrc_SetRecvCallback(GFLNetRecvFunc callback) {
    NetIrc_SetRecvCallback(callback);
    return TRUE;
}

static int NetDevIrc_GetNetId(void) {
    if (NetDevIrc_IsConnected()) {
        return NetIrc_GetNetId();
    }
    return 0;
}

static int NetDevIrc_IsError(void) {
    return NetIrc_IsError();
}

static void NetDevIrc_Poll(void) {
    NetIrc_Update();
}

static BOOL NetDevIrc_CanSend(void) {
    return NetIrc_CanSend();
}

static BOOL NetDevIrc_GetSendTurn(void) {
    return NetIrc_GetSendTurn();
}

static BOOL NetDevIrc_GetPeerSeen(void) {
    return NetIrc_GetPeerSeen();
}

static BOOL NetDevIrc_IsLinked(void) {
    return NetIrc_IsLinked();
}

static BOOL NetDevIrc_IsConnectedSlot(void) {
    return NetDevIrc_IsConnected();
}

static BOOL NetDevIrc_IsInitialized(void) {
    return NetIrc_IsInitialized();
}

static int NetDevIrc_GetConnectBits(void) {
    if (NetDevIrc_IsConnected()) {
        return 3;
    }
    return 0;
}

static BOOL NetDevIrc_GetUnkCF0(void) {
    return NetIrc_GetUnkCF0();
}

static void NetDevIrc_StartLink(void) {
    NetIrc_StartLink();
}

static void NetDevIrc_SetTimeoutSlot(int frames) {
    NetDevIrc_SetTimeout(frames);
}

static void NetDevIrc_SetPeerFilterSlot(void *filter) {
    NetDevIrc_SetPeerFilter(filter);
}

static GFLNetDevTable NetDevIrcTable = {
    .init = NetDevIrc_Init,
    .unk08 = NetDevIrc_Begin,
    .update = NetDevIrc_Update,
    .unk10 = NetDevIrc_ReturnTrue10,
    .unk14 = NetDevIrc_End,
    .send = NetDevIrc_Send,
    .setRecvCallback = NetDevIrc_SetRecvCallback,
    .unk6C = NetDevIrc_IsRunning,
    .isConnected = NetDevIrc_IsConnectedSlot,
    .unk74 = NetDevIrc_IsIdle,
    .unk78 = NetDevIrc_IsInitialized,
    .getConnectBits = NetDevIrc_GetConnectBits,
    .getNetId = NetDevIrc_GetNetId,
    .isError = NetDevIrc_IsError,
    .unk9C = NetDevIrc_Poll,
    .unkA0 = NetDevIrc_CanSend,
    .unkA4 = NetDevIrc_GetSendTurn,
    .unkA8 = NetDevIrc_GetPeerSeen,
    .unkAC = NetDevIrc_IsLinked,
    .unkB0 = NetDevIrc_GetUnkCF0,
    .unkB4 = NetDevIrc_StartLink,
    .unkC4 = NetDevIrc_SetTimeoutSlot,
    .unkC8 = NetDevIrc_SetPeerFilterSlot,
};

const GFLNetDevTable *NetDevIrc_GetTable(void) {
    return &NetDevIrcTable;
}
