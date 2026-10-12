#include "types.h"
#include "battle/btl_main.h"
#include "battle/btl_net.h"
#include "battle/btl_setup.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "pml/poke_party.h"
#include "save/chatter.h"
#include "save/player_info.h"
#include "system/str_tool.h"

// What a command that arrives whole is received into
typedef struct {
    u8 data[0xbb8];
    u16 size;
    // 1 until data arrives
    u16 empty;
} BtlNetRecvBuffer;

// Command 0x100: each machine's version, the highest of which becomes the server
typedef struct {
    u16 version;
    u16 clientId;
} BtlNetVersionPacket;

typedef struct {
    u8 version;
    u8 received;
} BtlNetClientVersion;

// The versions received, by net id
typedef struct {
    BtlNetClientVersion clients[4];
    u8 count;
    u8 max;
} BtlNetVersions;

// Command 0x104: a party sent for one client, the header then the data
typedef struct {
    // 4 for none
    u32 clientId : 8;
    u32 size : 24;
    u8 data[];
} BtlNetPartyPacket;

typedef struct {
    NetHandle *netHandle;
    HeapID heapId;
    u8 myNetId;
    // 40 until decided
    u8 serverNetId;
    u16 clientMask;
    u8 syncDataReceived;
    u8 numClients;
    u8 unkC;
    u8 serverCmdReceived;
    u8 unkE;
    u8 timingNo;
    u8 timingPending;
    // By net id, 4 for none
    u8 clientIds[4];
    BtlMainSyncData syncData;
    BtlNetRecvBuffer serverCmd;
    // By net id
    BtlNetRecvBuffer clientReplies[4];
    // By net id
    void *recvBuffers[4];
    u32 recvSizes[4];
    // By client id
    PokeParty *parties[4];
    void *tempBuffer;
    u32 tempSize;
    union {
        BtlNetVersionPacket version;
        u32 noChatter;
    } sendBuf;
    BtlNetVersions versions;
} BtlNetWork;

static BtlNetWork *sWork;

static void func_ov167_021b9abc(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL func_ov167_021b9b30(int netId);
static u8 func_ov167_021b9b94(u8 clientId);
static void func_ov167_021b9be0(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *func_ov167_021b9c6c(int netId, void *work, int size);
static void func_ov167_021b9cb4(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov167_021ba4ec(void);
static void func_ov167_021ba26c(void);
static void *func_ov167_021b9d58(int netId, void *work, int size);
static void func_ov167_021b9db8(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *func_ov167_021b9eb0(int netId, void *work, int size);
static void func_ov167_021b9efc(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *func_ov167_021ba038(int netId, void *work, int size);
static void func_ov167_021ba080(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *func_ov167_021ba154(int netId, void *work, int size);
static void func_ov167_021ba19c(int netId, int size, const void *data, void *work, NetHandle *handle);
static BtlCommTrainerData *func_ov167_021ba20c(const BtlSetupTrainer *trainer, HeapID heapId);
static void func_ov167_021ba264(void *data);
static void *func_ov167_021ba3fc(int netId, void *work, int size);
static void func_ov167_021ba414(int netId, int size, const void *data, void *work, NetHandle *handle);
static void func_ov167_021ba4b8(int netId, int size, const void *data, void *work, NetHandle *handle);
static void *func_ov167_021ba4e0(int netId, void *work, int size);

// Empties the receive buffers that func_ov167_021ba26c frees, and forgets their clients
static inline void ClearRecvBuffers(void) {
    u32 i;

    for (i = 0; i < 4; i++) {
        sWork->recvBuffers[i] = NULL;
        sWork->recvSizes[i] = 0;
        sWork->clientIds[i] = 4;
    }
    for (i = 0; i < 4; i++) {
        sWork->parties[i] = NULL;
    }
    sWork->tempBuffer = NULL;
    sWork->tempSize = 0;
}

void func_ov167_021b9950(NetHandle *netHandle, u16 clientMask, HeapID heapId) {
    u32 count;
    u32 max;
    u32 i;

    if (netHandle != NULL) {
        sWork = GFL_HeapAllocate(heapId, sizeof(BtlNetWork), TRUE, "btl_net.c", 212);
        sWork->netHandle = netHandle;
        sWork->clientMask = clientMask;
        sWork->heapId = heapId;
        sWork->myNetId = func_02042a6c(netHandle);
        sWork->serverNetId = 40;
        sWork->serverCmdReceived = FALSE;
        sWork->unkE = 0;
        for (i = 0, count = 0; i < 4; i++) {
            if ((1 << i) & clientMask) {
                count++;
            }
        }
        max = func_02042a78();
        if (count > max) {
            count = max;
        }
        sWork->numClients = count;
        sWork->timingNo = 0;
        sWork->timingPending = FALSE;
        sWork->syncDataReceived = FALSE;
        ClearRecvBuffers();
        func_ov167_021ba2f4(1);
    } else {
        sWork = NULL;
    }
}

BOOL func_ov167_021b9a30(void) {
    if (sWork != NULL) {
        return func_ov167_021ba318(1) ? TRUE : FALSE;
    }
    return TRUE;
}

void func_ov167_021b9a54(void) {
    if (sWork != NULL) {
        func_ov167_021ba26c();
        GFL_HeapFree(sWork);
    }
}

BOOL func_ov167_021b9a70(void) {
    if (sWork != NULL) {
        if (GFL_NetErrCheck()) {
            return TRUE;
        }
        return FALSE;
    }
    return FALSE;
}

// Sends this machine's version and client
BOOL func_ov167_021b9a94(u8 clientId) {
    BtlNetVersionPacket *packet = &sWork->sendBuf.version;

    packet->version = 100;
    packet->clientId = clientId;
    return func_02042be8(sWork->netHandle, 0x100, sizeof(BtlNetVersionPacket), packet);
}

// Command 0x100: records a machine's version, and once every machine's is in, makes the highest the server
static void func_ov167_021b9abc(int netId, int size, const void *data, void *work, NetHandle *handle) {
    const BtlNetVersionPacket *packet = data;
    BtlNetVersions *versions = &sWork->versions;
    int i;

    if (!versions->clients[netId].received) {
        u16 version = packet->version;

        sWork->clientIds[netId] = packet->clientId;
        versions->clients[netId].version = version;
        versions->clients[netId].received = TRUE;
        if (versions->max < version) {
            versions->max = version;
        }
        versions->count++;
        if (versions->count >= sWork->numClients) {
            sWork->serverNetId = 40;
            for (i = 0; i < 4; i++) {
                if (versions->clients[i].received && versions->max == versions->clients[i].version) {
                    sWork->serverNetId = i;
                    return;
                }
            }
        }
    }
}

static BOOL func_ov167_021b9b30(int netId) {
    return sWork->versions.clients[netId].received;
}

// Whether the server is decided
BOOL func_ov167_021b9b48(void) {
    if (sWork->serverNetId != 40) {
        return TRUE;
    }
    return FALSE;
}

// Whether this machine is the server
BOOL func_ov167_021b9b60(void) {
    if (sWork->serverNetId == sWork->myNetId) {
        return TRUE;
    }
    return FALSE;
}

// The highest version, which every machine plays by
u8 func_ov167_021b9b78(void) {
    if (sWork != NULL) {
        return sWork->versions.max;
    }
    return 100;
}

// The net id of a client, or 40
static u8 func_ov167_021b9b94(u8 clientId) {
    int i;

    for (i = 0; i < 4; i++) {
        if (sWork->clientIds[i] == clientId) {
            return i;
        }
    }
    return 40;
}

// Sends the server's sync data to everyone
BOOL func_ov167_021b9bb8(BtlMainSyncData *data) {
    return func_02042c18(sWork->netHandle, 0xff, 0x101, sizeof(BtlMainSyncData), data, 0, 0, 0);
}

// Command 0x101: keeps the first sync data that arrives
static void func_ov167_021b9be0(int netId, int size, const void *data, void *work, NetHandle *handle) {
    if (!sWork->syncDataReceived) {
        sWork->syncData = *(const BtlMainSyncData *)data;
        sWork->syncDataReceived = TRUE;
    }
}

// Copies the sync data once it has arrived
BOOL func_ov167_021b9c0c(BtlMainSyncData *data) {
    if (sWork->syncDataReceived) {
        *data = sWork->syncData;
        return TRUE;
    }
    return FALSE;
}

// Sends this machine's party to everyone
u8 func_ov167_021b9c38(PokeParty *party) {
    return func_02042c18(sWork->netHandle, 0xff, 0x102, PokeParty_GetSaveDataSize(), party, 0, 1, 1);
}

// Command 0x102's buffer
static void *func_ov167_021b9c6c(int netId, void *work, int size) {
    sWork->recvBuffers[netId] = GFL_HeapAllocate(HEAPID_TAIL(sWork->heapId), size, TRUE, "btl_net.c", 462);
    return sWork->recvBuffers[netId];
}

// Command 0x102: a party has arrived
static void func_ov167_021b9cb4(int netId, int size, const void *data, void *work, NetHandle *handle) {
    sWork->recvSizes[netId] = size;
}

// Whether every machine's data has arrived
BOOL func_ov167_021b9ccc(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (func_ov167_021b9b30(i) && sWork->recvSizes[i] == 0) {
            return FALSE;
        }
    }
    return TRUE;
}

void func_ov167_021b9d00(void) {
    func_ov167_021ba4ec();
    func_ov167_021ba26c();
}

// Sends this machine's Chatot cry to everyone, or a word of zero without one
BOOL func_ov167_021b9d0c(void *chatter) {
    u32 size;

    if (doesChatotExist(chatter)) {
        size = PerapVoice_GetSize();
    } else {
        size = sizeof(u32);
        chatter = &sWork->sendBuf.noChatter;
        sWork->sendBuf.noChatter = 0;
    }
    return func_02042c18(sWork->netHandle, 0xff, 0x103, size, chatter, 0, 1, 1);
}

// Command 0x103's buffer: a word for no cry, which goes in the machine's reply buffer
static void *func_ov167_021b9d58(int netId, void *work, int size) {
    if (size == sizeof(u32)) {
        return &sWork->clientReplies[netId];
    }
    sWork->recvBuffers[netId] = GFL_HeapAllocate(HEAPID_TAIL(sWork->heapId), size, TRUE, "btl_net.c", 543);
    return sWork->recvBuffers[netId];
}

// Command 0x103: a Chatot cry has arrived
static void func_ov167_021b9db8(int netId, int size, const void *data, void *work, NetHandle *handle) {
    if (size == sizeof(u32)) {
        BtlNetRecvBuffer *buffer = &sWork->clientReplies[netId];

        buffer->size = size;
        buffer->empty = FALSE;
    } else {
        sWork->recvSizes[netId] = size;
    }
}

// Whether every machine's Chatot cry has arrived
BOOL func_ov167_021b9dfc(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (func_ov167_021b9b30(i) && sWork->clientReplies[i].empty && sWork->recvSizes[i] == 0) {
            return FALSE;
        }
    }
    return TRUE;
}

// A client's Chatot cry, or NULL
void *func_ov167_021b9e48(u8 clientId) {
    u8 netId = func_ov167_021b9b94(clientId);

    if (netId != 40 && sWork->recvSizes[netId] != 0) {
        return sWork->recvBuffers[netId];
    }
    return NULL;
}

void func_ov167_021b9e74(void) {
    func_ov167_021ba4ec();
    func_ov167_021ba26c();
}

// Sends a party for one client to everyone
BOOL func_ov167_021b9e80(void *data) {
    BtlNetPartyPacket *packet = data;

    return func_02042c18(sWork->netHandle, 0xff, 0x104, packet->size + sizeof(BtlNetPartyPacket), packet, 0, 1, 1);
}

// Command 0x104's buffer
static void *func_ov167_021b9eb0(int netId, void *work, int size) {
    if (sWork->tempBuffer == NULL) {
        sWork->tempBuffer = GFL_HeapAllocate(HEAPID_TAIL(sWork->heapId), size, TRUE, "btl_net.c", 633);
    }
    return sWork->tempBuffer;
}

// Command 0x104: keeps the party for its client
static void func_ov167_021b9efc(int netId, int size, const void *data, void *work, NetHandle *handle) {
    const BtlNetPartyPacket *packet = data;

    if (sWork->parties[packet->clientId] == NULL) {
        sWork->parties[packet->clientId] =
            GFL_HeapAllocate(HEAPID_TAIL(sWork->heapId), packet->size, TRUE, "btl_net.c", 642);
    }
    sys_memcpy(packet->data, sWork->parties[packet->clientId], packet->size);
    GFL_HeapFree(sWork->tempBuffer);
    sWork->tempBuffer = NULL;
}

// Whether a client's party has arrived
BOOL func_ov167_021b9f84(u8 clientId) {
    if (sWork->parties[clientId] != NULL) {
        return TRUE;
    }
    return FALSE;
}

// A client's party, from its machine or sent for it
PokeParty *func_ov167_021b9fa4(u8 clientId) {
    u8 netId = func_ov167_021b9b94(clientId);
    PokeParty *party;
    u32 count;
    u32 i;

    if (netId != 40) {
        return sWork->recvBuffers[netId];
    }
    party = sWork->parties[clientId];
    if (party != NULL) {
        count = PokeParty_GetPkmCount(party);
        for (i = 0; i < count; i++) {
            PokeParty_GetPkm(party, i);
        }
        return party;
    }
    return NULL;
}

void func_ov167_021ba000(void) {
    func_ov167_021ba26c();
}

// Sends this machine's player info to everyone
BOOL func_ov167_021ba008(PlayerInfo *info) {
    return func_02042c18(sWork->netHandle, 0xff, 0x105, PlayerInfo_GetSize(), info, 0, 1, 1);
}

// Command 0x105's buffer
static void *func_ov167_021ba038(int netId, void *work, int size) {
    sWork->recvBuffers[netId] = GFL_HeapAllocate(HEAPID_TAIL(sWork->heapId), size, TRUE, "btl_net.c", 724);
    return sWork->recvBuffers[netId];
}

// Command 0x105: a machine's player info has arrived
static void func_ov167_021ba080(int netId, int size, const void *data, void *work, NetHandle *handle) {
    sWork->recvSizes[netId] = size;
}

// Whether every machine's player info has arrived
BOOL func_ov167_021ba098(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (func_ov167_021b9b30(i) && sWork->recvSizes[i] == 0) {
            return FALSE;
        }
    }
    return TRUE;
}

// A client's player info, from its machine or sent for it
PlayerInfo *func_ov167_021ba0cc(u8 clientId) {
    u8 netId = func_ov167_021b9b94(clientId);

    if (netId != 40) {
        return sWork->recvBuffers[netId];
    }
    if (sWork->tempBuffer != NULL && sWork->tempSize != 0) {
        return sWork->tempBuffer;
    }
    return NULL;
}

// Sends this machine's trainer to everyone
BOOL func_ov167_021ba108(BtlSetupTrainer *trainer) {
    BtlCommTrainerData *data = func_ov167_021ba20c(trainer, HEAPID_TAIL(sWork->heapId));
    BOOL result = func_02042c18(sWork->netHandle, 0xff, 0x106, sizeof(BtlCommTrainerData), data, 0, 1, 0);

    func_ov167_021ba264(data);
    return result;
}

// Command 0x106's buffer
static void *func_ov167_021ba154(int netId, void *work, int size) {
    sWork->tempBuffer = GFL_HeapAllocate(HEAPID_TAIL(sWork->heapId), size, TRUE, "btl_net.c", 798);
    return sWork->tempBuffer;
}

// Command 0x106: a trainer has arrived
static void func_ov167_021ba19c(int netId, int size, const void *data, void *work, NetHandle *handle) {
    sWork->tempSize = size;
}

// Whether the trainer has arrived
BOOL func_ov167_021ba1b0(void) {
    if (sWork->tempSize != 0) {
        return TRUE;
    }
    return FALSE;
}

void *func_ov167_021ba1cc(void) {
    return sWork->tempBuffer;
}

void func_ov167_021ba1e0(void) {
    GFL_HeapFree(sWork->tempBuffer);
    sWork->tempBuffer = NULL;
    sWork->tempSize = 0;
}

void func_ov167_021ba204(void) {
    func_ov167_021ba26c();
}

// A trainer as a link battle sends it, with its name copied out of the string buffer
static BtlCommTrainerData *func_ov167_021ba20c(const BtlSetupTrainer *trainer, HeapID heapId) {
    BtlCommTrainerData *data = GFL_HeapAllocate(heapId, sizeof(BtlCommTrainerData), TRUE, "btl_net.c", 841);

    sys_memcpy(trainer, data, sizeof(BtlSetupTrainer));
    sys_memset16(GFL_StrBufGetTerminator(), data->name, sizeof(data->name));
    if (trainer->name != NULL) {
        data->nameLength = GFL_StrBufGetCharCount(trainer->name);
        wcharsncpy(GFL_StrBufGetStringPtr(trainer->name), data->name, NELEMS(data->name));
    }
    return data;
}

static void func_ov167_021ba264(void *data) {
    GFL_HeapFree(data);
}

static void func_ov167_021ba26c(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (sWork->recvBuffers[i] != NULL) {
            GFL_HeapFree(sWork->recvBuffers[i]);
            sWork->recvBuffers[i] = NULL;
            sWork->recvSizes[i] = 0;
        }
    }
    for (i = 0; i < 4; i++) {
        if (sWork->parties[i] != NULL) {
            GFL_HeapFree(sWork->parties[i]);
            sWork->parties[i] = NULL;
        }
    }
    if (sWork->tempBuffer != NULL) {
        GFL_HeapFree(sWork->tempBuffer);
        sWork->tempBuffer = NULL;
        sWork->tempSize = 0;
    }
}

// Starts synchronizing every machine on a timing number
void func_ov167_021ba2f4(u8 id) {
    if (sWork != NULL) {
        sWork->timingNo = id;
        sWork->timingPending = TRUE;
        func_02040b38(sWork->netHandle, id, 1, sWork->clientMask);
    }
}

// Whether every machine has reached the timing number, starting the wait if it isn't waiting
// Whether every machine has reached the timing number, starting the wait if it isn't waiting
BOOL func_ov167_021ba318(u8 id) {
    if (sWork != NULL) {
        if (sWork->timingPending) {
            BOOL done = func_02040664(sWork->netHandle, sWork->timingNo, 1);

            if (done) {
                sWork->timingNo = 0;
                sWork->timingPending = FALSE;
                return done;
            }
        } else {
            func_02040b38(sWork->netHandle, id, 1, sWork->clientMask);
        }
        return FALSE;
    }
    return TRUE;
}

// Sends a server command to a client's machine
BOOL func_ov167_021ba358(u8 clientId, void *data, u32 size) {
    u8 netId = func_ov167_021b9b94(clientId);

    if (netId != 40) {
        return func_02042c18(sWork->netHandle, netId, 0x107, size, data, 0, 0, 1);
    }
    return TRUE;
}

// Whether every machine's reply has arrived
BOOL func_ov167_021ba398(void) {
    int i;

    for (i = 0; i < 4; i++) {
        if (func_ov167_021b9b30(i) && sWork->clientReplies[i].empty) {
            return FALSE;
        }
    }
    return TRUE;
}

// A client's reply and its size
u16 func_ov167_021ba3d4(u8 clientId, void **data) {
    u8 netId = func_ov167_021b9b94(clientId);
    BtlNetRecvBuffer *buffer = &sWork->clientReplies[netId];

    *data = buffer;
    return buffer->size;
}

// Command 0x108's buffer
static void *func_ov167_021ba3fc(int netId, void *work, int size) {
    return &sWork->clientReplies[netId];
}

// Command 0x108: a client's reply has arrived
static void func_ov167_021ba414(int netId, int size, const void *data, void *work, NetHandle *handle) {
    BtlNetRecvBuffer *buffer = &sWork->clientReplies[netId];

    buffer->size = size;
    buffer->empty = FALSE;
    sWork->clientReplies[netId].size = size;
    sWork->clientReplies[netId].empty = FALSE;
}

void func_ov167_021ba458(void) {
    func_ov167_021ba4ec();
}

// Whether the server's command has arrived
BOOL func_ov167_021ba460(void) {
    return sWork->serverCmdReceived;
}

// The server's command and its size
u16 func_ov167_021ba46c(void **data) {
    BtlNetRecvBuffer *buffer = &sWork->serverCmd;

    *data = buffer;
    return buffer->size;
}

// Sends this machine's reply to the server
BOOL func_ov167_021ba484(void *data, u32 size) {
    BOOL result = func_02042c18(sWork->netHandle, sWork->serverNetId, 0x108, size, data, 0, 1, 1);

    if (result) {
        sWork->serverCmdReceived = FALSE;
    }
    return result;
}

// Command 0x107: the server's command has arrived
static void func_ov167_021ba4b8(int netId, int size, const void *data, void *work, NetHandle *handle) {
    if (!sWork->serverCmdReceived) {
        BtlNetRecvBuffer *buffer = &sWork->serverCmd;

        buffer->size = size;
        buffer->empty = FALSE;
        sWork->serverCmdReceived = TRUE;
    }
}

// Command 0x107's buffer
static void *func_ov167_021ba4e0(int netId, void *work, int size) {
    return &sWork->serverCmd;
}

static void func_ov167_021ba4ec(void) {
    int i;

    for (i = 0; i < 4; i++) {
        BtlNetRecvBuffer *buffer = &sWork->clientReplies[i];

        buffer->size = 0;
        buffer->empty = TRUE;
    }
}

// A party packet of a size, for no client yet
void *func_ov167_021ba524(u32 size, HeapID heapId) {
    BtlNetPartyPacket *packet = GFL_HeapAllocate(heapId, size + sizeof(BtlNetPartyPacket), TRUE, "btl_net.c", 1143);

    packet->size = size;
    packet->clientId = 4;
    return packet;
}

void func_ov167_021ba55c(void *data) {
    GFL_HeapFree(data);
}

// Puts a client's party in a party packet
void func_ov167_021ba564(void *data, PokeParty *party, u8 clientId) {
    BtlNetPartyPacket *packet = data;

    packet->clientId = clientId;
    sys_memcpy(party, packet->data, packet->size);
}

// The link battle's commands, from 0x100
const NetCommand data_ov167_021d7448[9] = {
    { func_ov167_021b9abc, NULL },
    { func_ov167_021b9be0, NULL },
    { func_ov167_021b9cb4, func_ov167_021b9c6c },
    { func_ov167_021b9db8, func_ov167_021b9d58 },
    { func_ov167_021b9efc, func_ov167_021b9eb0 },
    { func_ov167_021ba080, func_ov167_021ba038 },
    { func_ov167_021ba19c, func_ov167_021ba154 },
    { func_ov167_021ba4b8, func_ov167_021ba4e0 },
    { func_ov167_021ba414, func_ov167_021ba3fc },
};
