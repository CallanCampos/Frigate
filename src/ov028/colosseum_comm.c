// The colosseum's net commands from 0x1500, over which up to four players trade their info, trainer cards and parties,
// with the senders for them. The file's name is a guess: the ROM only shows that it is a file of its own, before
// colosseum.c, by the assert file name ("") of its assert
#include "types.h"
#include "app/pms_select.h"
#include "field/colosseum.h"
#include "field/event_colosseum_battle.h"
#include "field/field_event.h"
#include "field/union_comm.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/net_command.h"
#include "gfl/net_handle.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "system/game_data.h"
#include "system/game_system.h"

// The net commands, from the base 0x1500
enum {
    COLOSSEUM_CMD_BASE = 0x1500,
    COLOSSEUM_CMD_MEMBER,
    COLOSSEUM_CMD_CARD,
    COLOSSEUM_CMD_CHOICE,
    COLOSSEUM_CMD_VOTE,
    COLOSSEUM_CMD_VOTE_REPLY,
    COLOSSEUM_CMD_PARTY,
    COLOSSEUM_CMD_RESULT,
    COLOSSEUM_CMD_READY,
    COLOSSEUM_CMD_READY_CANCEL,
    COLOSSEUM_CMD_CANCEL_ACK,
    COLOSSEUM_CMD_ALL_READY,
    COLOSSEUM_CMD_UNKD = COLOSSEUM_CMD_BASE + 13,
};

static void *Colosseum_GetRecvBuffer(int netId, void *work, int size);
static void Colosseum_RecvNone(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvMember(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvCard(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvChoice(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvVote(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvVoteReply(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvParty(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvResult(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvReady(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvReadyCancel(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvCancelAck(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvAllReady(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvUnkC(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvUnkD(int netId, int size, const void *data, void *work, NetHandle *handle);

// clang-format off
static const NetCommand sColosseumCommands[] = {
    {Colosseum_RecvNone, NULL},
    {Colosseum_RecvMember, Colosseum_GetRecvBuffer},
    {Colosseum_RecvCard, Colosseum_GetRecvBuffer},
    {Colosseum_RecvChoice, Colosseum_GetRecvBuffer},
    {Colosseum_RecvVote, NULL},
    {Colosseum_RecvVoteReply, NULL},
    {Colosseum_RecvParty, Colosseum_GetRecvBuffer},
    {Colosseum_RecvResult, NULL},
    {Colosseum_RecvReady, NULL},
    {Colosseum_RecvReadyCancel, NULL},
    {Colosseum_RecvCancelAck, NULL},
    {Colosseum_RecvAllReady, NULL},
    {Colosseum_RecvUnkC, NULL},
    {Colosseum_RecvUnkD, NULL},
};

// clang-format on

void Colosseum_RegisterCommands(UnionSystem *unisys) {
    NetCommand_AddTable(COLOSSEUM_CMD_BASE, sColosseumCommands, NELEMS(sColosseumCommands), unisys);
}

static void *Colosseum_GetRecvBuffer(int netId, void *work, int size) {
    UnionSystem *unisys = work;

    GFL_ASSERT_MSG(size <= 0x800, "size=%x, recv_size=%x\n", size, 0x800);
    return unisys->recvBuf[netId];
}

static void Colosseum_RecvNone(int netId, int size, const void *data, void *work, NetHandle *handle) {
}

static void Colosseum_RecvMember(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (netId != GFL_NetGetHandleNetId(NetHandle_GetOwn()) && colosseum != NULL && colosseum->commPlayer != NULL) {
        MI_CpuCopy8(data, &colosseum->members[netId], size);
    }
}

BOOL Colosseum_SendMember(const ColosseumMember *member, BOOL toParent) {
    u32 sendId = 0;

    if (toParent != TRUE) {
        sendId = GFL_NET_NETID_SERVER;
    }
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), sendId, COLOSSEUM_CMD_MEMBER, 0x2c, member, TRUE, FALSE,
                                       TRUE);
}

static void Colosseum_RecvCard(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (netId != GFL_NetGetHandleNetId(NetHandle_GetOwn()) && colosseum != NULL && colosseum->commPlayer != NULL &&
        colosseum->cardCheck[netId] != NULL) {
        MI_CpuCopy8(data, colosseum->card[netId], size);
        colosseum->cardReady[netId] = TRUE;
    }
}

BOOL Colosseum_SendCard(const TrainerCardData *card) {
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), GFL_NET_NETID_SERVER, COLOSSEUM_CMD_CARD, 0x6c4, card, TRUE,
                                       FALSE, TRUE);
}

BOOL Colosseum_SendCardHead(const TrainerCardData *card) {
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), GFL_NET_NETID_SERVER, COLOSSEUM_CMD_CARD, 0x678, card, TRUE,
                                       FALSE, TRUE);
}

static void Colosseum_RecvChoice(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (netId != GFL_NetGetHandleNetId(NetHandle_GetOwn()) && colosseum != NULL && colosseum->commPlayer != NULL) {
        Colosseum_StoreChoice(colosseum, netId, data);
    }
}

BOOL Colosseum_SendChoice(const ColosseumChoice *choice) {
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), GFL_NET_NETID_SERVER, COLOSSEUM_CMD_CHOICE,
                                       sizeof(ColosseumChoice), choice, FALSE, TRUE, TRUE);
}

static void Colosseum_RecvVote(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL && colosseum->commPlayer != NULL && GFL_NetIsParent()) {
        Colosseum_Vote(colosseum, netId, *(const u8 *)data);
    }
}

BOOL Colosseum_SendUnk4(UnionColosseum *colosseum) {
    u8 value = Colosseum_GetUnkDC(colosseum);

    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), 0, COLOSSEUM_CMD_VOTE, 1, &value, TRUE, FALSE, FALSE);
}

static void Colosseum_RecvVoteReply(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL && colosseum->commPlayer != NULL) {
        Colosseum_SetUnkDD(colosseum, *(const u32 *)data);
    }
}

BOOL Colosseum_SendVoteReply(UnionColosseum *colosseum, u32 netId, u32 value) {
    Colosseum_GetUnkDC(colosseum);
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), netId, COLOSSEUM_CMD_VOTE_REPLY, 4, &value, TRUE, FALSE,
                                       FALSE);
}

static void Colosseum_RecvParty(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL && colosseum->party[netId] != NULL) {
        MI_CpuCopy8(data, colosseum->party[netId], size);
        colosseum->partyReady[netId] = TRUE;
    }
}

BOOL Colosseum_SendParty(const PokeParty *party) {
    NetHandle *handle = NetHandle_GetOwn();

    return GFL_NetSendCommandToMachine(handle, GFL_NET_NETID_SERVER, COLOSSEUM_CMD_PARTY, PokeParty_GetSaveDataSize(),
                                       party, TRUE, FALSE, TRUE);
}

static void Colosseum_RecvResult(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        MI_CpuCopy8(data, colosseum->unk16C, size);
        colosseum->unk170 = TRUE;
    }
}

BOOL Colosseum_SendResult(const void *result) {
    return GFL_NetSendCommand(NetHandle_GetOwn(), COLOSSEUM_CMD_RESULT, 4, result);
}

static void Colosseum_RecvReady(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        colosseum->readyReceived[netId] = TRUE;
    }
}

BOOL Colosseum_SendUnk8(void) {
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), 0, COLOSSEUM_CMD_READY, 0, NULL, TRUE, FALSE, FALSE);
}

static void Colosseum_RecvReadyCancel(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        colosseum->cancelReceived[netId] = TRUE;
        colosseum->readyReceived[netId] = FALSE;
    }
}

BOOL Colosseum_SendUnk9(void) {
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), 0, COLOSSEUM_CMD_READY_CANCEL, 0, NULL, TRUE, FALSE, FALSE);
}

static void Colosseum_RecvCancelAck(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        colosseum->unkDF = FALSE;
    }
}

BOOL Colosseum_SendCancelAck(u32 netId) {
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), netId, COLOSSEUM_CMD_CANCEL_ACK, 0, NULL, TRUE, FALSE,
                                       FALSE);
}

static void Colosseum_RecvAllReady(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        colosseum->unkCB = TRUE;
    }
}

BOOL Colosseum_SendAllReady(void) {
    return GFL_NetSendCommandToMachine(NetHandle_GetOwn(), GFL_NET_NETID_SERVER, COLOSSEUM_CMD_ALL_READY, 0, NULL, TRUE,
                                       FALSE, FALSE);
}

static void Colosseum_RecvUnkC(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        colosseum->unk171++;
    }
}

static void Colosseum_RecvUnkD(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        colosseum->unk172[netId] = TRUE;
    }
}

BOOL Colosseum_SendUnkD(void) {
    return GFL_NetSendCommand(NetHandle_GetOwn(), COLOSSEUM_CMD_UNKD, 0, NULL);
}
