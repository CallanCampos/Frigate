// The colosseum's net commands from 0x1500, over which up to four players trade their info, trainer cards and parties,
// with the senders for them. The file's name is a guess: the ROM only shows that it is a file of its own, before
// colosseum.c, by the assert file name ("") of its assert
#include "field/colosseum.h"
#include "types.h"
#include "app/pms_select.h"
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
#define COLOSSEUM_CMD_BASE 0x1500
#define COLOSSEUM_CMD_MEMBER (COLOSSEUM_CMD_BASE + 1)
#define COLOSSEUM_CMD_CARD (COLOSSEUM_CMD_BASE + 2)
#define COLOSSEUM_CMD_CHOICE (COLOSSEUM_CMD_BASE + 3)
#define COLOSSEUM_CMD_UNK4 (COLOSSEUM_CMD_BASE + 4)
#define COLOSSEUM_CMD_UNK5 (COLOSSEUM_CMD_BASE + 5)
#define COLOSSEUM_CMD_PARTY (COLOSSEUM_CMD_BASE + 6)
#define COLOSSEUM_CMD_RESULT (COLOSSEUM_CMD_BASE + 7)
#define COLOSSEUM_CMD_UNK8 (COLOSSEUM_CMD_BASE + 8)
#define COLOSSEUM_CMD_UNK9 (COLOSSEUM_CMD_BASE + 9)
#define COLOSSEUM_CMD_UNKA (COLOSSEUM_CMD_BASE + 10)
#define COLOSSEUM_CMD_UNKB (COLOSSEUM_CMD_BASE + 11)
#define COLOSSEUM_CMD_UNKD (COLOSSEUM_CMD_BASE + 13)

static void *Colosseum_GetRecvBuffer(int netId, void *work, int size);
static void Colosseum_RecvNone(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvMember(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvCard(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvChoice(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvUnk4(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvUnk5(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvParty(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvResult(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvUnk8(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvUnk9(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvUnkA(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvUnkB(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvUnkC(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvUnkD(int netId, int size, const void *data, void *work, NetHandle *handle);

static const NetCommand sColosseumCommands[] = {
    {Colosseum_RecvNone, NULL},
    {Colosseum_RecvMember, Colosseum_GetRecvBuffer},
    {Colosseum_RecvCard, Colosseum_GetRecvBuffer},
    {Colosseum_RecvChoice, Colosseum_GetRecvBuffer},
    {Colosseum_RecvUnk4, NULL},
    {Colosseum_RecvUnk5, NULL},
    {Colosseum_RecvParty, Colosseum_GetRecvBuffer},
    {Colosseum_RecvResult, NULL},
    {Colosseum_RecvUnk8, NULL},
    {Colosseum_RecvUnk9, NULL},
    {Colosseum_RecvUnkA, NULL},
    {Colosseum_RecvUnkB, NULL},
    {Colosseum_RecvUnkC, NULL},
    {Colosseum_RecvUnkD, NULL},
};

void Colosseum_RegisterCommands(UnionSystem *unisys) {
    func_02040c20(COLOSSEUM_CMD_BASE, sColosseumCommands, NELEMS(sColosseumCommands), unisys);
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

    if (netId != func_02042a6c(func_02040440()) && colosseum != NULL && colosseum->commPlayer != NULL) {
        sys_memcpy(data, &colosseum->members[netId], size);
    }
}

BOOL Colosseum_SendMember(const void *member, BOOL toParent) {
    u32 sendId = 0;

    if (toParent != TRUE) {
        sendId = GFL_NET_NETID_SERVER;
    }
    return func_02042c18(func_02040440(), sendId, COLOSSEUM_CMD_MEMBER, 0x2c, member, TRUE, FALSE, TRUE);
}

static void Colosseum_RecvCard(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (netId != func_02042a6c(func_02040440()) && colosseum != NULL && colosseum->commPlayer != NULL
        && colosseum->cardCheck[netId] != NULL) {
        sys_memcpy(data, colosseum->card[netId], size);
        colosseum->cardReady[netId] = TRUE;
    }
}

BOOL Colosseum_SendCard(const TrainerCardData *card) {
    return func_02042c18(func_02040440(), GFL_NET_NETID_SERVER, COLOSSEUM_CMD_CARD, 0x6c4, card, TRUE, FALSE, TRUE);
}

BOOL Colosseum_SendCardHead(const TrainerCardData *card) {
    return func_02042c18(func_02040440(), GFL_NET_NETID_SERVER, COLOSSEUM_CMD_CARD, 0x678, card, TRUE, FALSE, TRUE);
}

static void Colosseum_RecvChoice(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (netId != func_02042a6c(func_02040440()) && colosseum != NULL && colosseum->commPlayer != NULL) {
        Colosseum_StoreChoice(colosseum, netId, data);
    }
}

BOOL Colosseum_SendChoice(const ColosseumChoice *choice) {
    return func_02042c18(func_02040440(), GFL_NET_NETID_SERVER, COLOSSEUM_CMD_CHOICE, sizeof(ColosseumChoice), choice, FALSE,
                  TRUE, TRUE);
}

static void Colosseum_RecvUnk4(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL && colosseum->commPlayer != NULL && func_02042bc4()) {
        Colosseum_Vote(colosseum, netId, *(const u8 *)data);
    }
}

BOOL func_ov028_021725c8(UnionColosseum *colosseum) {
    u8 value = Colosseum_GetUnkDC(colosseum);

    return func_02042c18(func_02040440(), 0, COLOSSEUM_CMD_UNK4, 1, &value, TRUE, FALSE, FALSE);
}

static void Colosseum_RecvUnk5(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL && colosseum->commPlayer != NULL) {
        Colosseum_SetUnkDD(colosseum, *(const u32 *)data);
    }
}

BOOL Colosseum_SendUnk5(UnionColosseum *colosseum, u32 netId, u32 value) {
    Colosseum_GetUnkDC(colosseum);
    return func_02042c18(func_02040440(), netId, COLOSSEUM_CMD_UNK5, 4, &value, TRUE, FALSE, FALSE);
}

static void Colosseum_RecvParty(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL && colosseum->party[netId] != NULL) {
        sys_memcpy(data, colosseum->party[netId], size);
        colosseum->partyReady[netId] = TRUE;
    }
}

BOOL Colosseum_SendParty(const PokeParty *party) {
    NetHandle *handle = func_02040440();

    return func_02042c18(handle, GFL_NET_NETID_SERVER, COLOSSEUM_CMD_PARTY, PokeParty_GetSaveDataSize(), party, TRUE, FALSE,
                  TRUE);
}

static void Colosseum_RecvResult(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        sys_memcpy(data, colosseum->unk16C, size);
        colosseum->unk170 = TRUE;
    }
}

BOOL Colosseum_SendResult(const void *result) {
    return func_02042be8(func_02040440(), COLOSSEUM_CMD_RESULT, 4, result);
}

static void Colosseum_RecvUnk8(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        colosseum->unkD4[netId] = TRUE;
    }
}

BOOL func_ov028_02172704(void) {
    return func_02042c18(func_02040440(), 0, COLOSSEUM_CMD_UNK8, 0, NULL, TRUE, FALSE, FALSE);
}

static void Colosseum_RecvUnk9(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        colosseum->unkD8[netId] = TRUE;
        colosseum->unkD4[netId] = FALSE;
    }
}

BOOL func_ov028_02172748(void) {
    return func_02042c18(func_02040440(), 0, COLOSSEUM_CMD_UNK9, 0, NULL, TRUE, FALSE, FALSE);
}

static void Colosseum_RecvUnkA(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        colosseum->unkDF = FALSE;
    }
}

BOOL Colosseum_SendUnkA(u32 netId) {
    return func_02042c18(func_02040440(), netId, COLOSSEUM_CMD_UNKA, 0, NULL, TRUE, FALSE, FALSE);
}

static void Colosseum_RecvUnkB(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        colosseum->unkCB = TRUE;
    }
}

BOOL Colosseum_SendUnkB(void) {
    return func_02042c18(func_02040440(), GFL_NET_NETID_SERVER, COLOSSEUM_CMD_UNKB, 0, NULL, TRUE, FALSE, FALSE);
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

BOOL func_ov028_02172818(void) {
    return func_02042be8(func_02040440(), COLOSSEUM_CMD_UNKD, 0, NULL);
}
