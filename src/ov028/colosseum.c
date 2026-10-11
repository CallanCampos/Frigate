// The Union Room's colosseum (the file's name is from its assert strings): up to four players trade their info, trainer
// cards and parties over the net commands from 0x1500, then pick the teams of a battle. The work is allocated by
// Colosseum_Create and kept in the UnionSystem
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

// The heap that the Union Room uses
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
static BOOL Colosseum_SendUnk5(UnionColosseum *colosseum, u32 netId, u32 value);
static void Colosseum_RecvParty(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvResult(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvUnk8(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvUnk9(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvUnkA(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL Colosseum_SendUnkA(u32 netId);
static void Colosseum_RecvUnkB(int netId, int size, const void *data, void *work, NetHandle *handle);
static BOOL Colosseum_SendUnkB(void);
static void Colosseum_RecvUnkC(int netId, int size, const void *data, void *work, NetHandle *handle);
static void Colosseum_RecvUnkD(int netId, int size, const void *data, void *work, NetHandle *handle);
static u8 Colosseum_GetUnkDC(const UnionColosseum *colosseum);
static void Colosseum_SetUnkDD(UnionColosseum *colosseum, u32 value);
static BOOL Colosseum_Vote(UnionColosseum *colosseum, u32 netId, u8 value);
static void Colosseum_StoreChoice(UnionColosseum *colosseum, u32 netId, const ColosseumChoice *choice);
static void Colosseum_SetPlayer(UnionColosseum *colosseum, BattlePlayer *player, u8 index, u32 unused);

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

BOOL Colosseum_CommandBattle(GameSystem *gsys, UnionSystem *unisys, Field *field, void *param, GameEvent *event,
                         GameEvent **next, u8 *step) {
    UnionSelf *self = &unisys->situ;

    if (*step == 0) {
        *next = func_ov012_02152704(gsys, field, self->activity, param);
    } else {
        return TRUE;
    }
    (*step)++;
    return FALSE;
}

BOOL Colosseum_CommandPmsSelect(GameSystem *gsys, UnionSystem *unisys, Field *field, void *param, GameEvent *event,
                         GameEvent **next, u8 *step) {
    if (*step == 0) {
        *next = EventFieldSubprocessTransition_Create(gsys, field, OVERLAY_ID(185), &PMS_SELECT_PROC_FUNCTIONS, param);
    } else {
        return TRUE;
    }
    (*step)++;
    return FALSE;
}

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

static BOOL Colosseum_SendUnk5(UnionColosseum *colosseum, u32 netId, u32 value) {
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

static BOOL Colosseum_SendUnkA(u32 netId) {
    return func_02042c18(func_02040440(), netId, COLOSSEUM_CMD_UNKA, 0, NULL, TRUE, FALSE, FALSE);
}

static void Colosseum_RecvUnkB(int netId, int size, const void *data, void *work, NetHandle *handle) {
    UnionSystem *unisys = work;
    UnionColosseum *colosseum = unisys->colosseum_sys;

    if (colosseum != NULL) {
        colosseum->unkCB = TRUE;
    }
}

static BOOL Colosseum_SendUnkB(void) {
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

UnionColosseum *Colosseum_Create(GameData *gameData, GameSystem *gsys, const PlayerInfo *info, u32 flag) {
    UnionColosseum *colosseum;
    int self = func_02042a6c(func_02040440());
    int i;
    u8 value;
    ColosseumMember *member;

    colosseum = GFL_HeapAllocate(HEAPID_UNION, sizeof(UnionColosseum), TRUE, "colosseum.c", 53);
    colosseum->commPlayer = func_ov012_021613d0(4, gsys, HEAPID_UNION, 0);
    for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
        colosseum->card[i] = GFL_HeapAllocate(HEAPID_UNION, sizeof(TrainerCardData), TRUE, "colosseum.c", 56);
        colosseum->cardCheck[i] = colosseum->card[i];
        colosseum->party[i] = PokeParty_Create(HEAPID_UNION);
    }
    for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
        colosseum->unkCC[i] = 0xff;
        colosseum->unkD0[i] = 0xff;
    }
    colosseum->unkDC = 0xff;
    colosseum->unkDD = 0xff;
    PokeParty_Copy(GameData_GetParty(gameData), colosseum->party[self]);
    member = &colosseum->members[self];
    func_02008b34(info, &member->info);
    func_0207c33c(member->mac);
    member->unk26 = 1;
    value = 1;
    if (flag == 1) {
        value = 0;
    }
    member->unk27 = value;
    member->unk28 = 100;
    func_ov012_02169770(colosseum->cardCheck[self], gameData, TRUE, FALSE, HEAPID_UNION);
    func_ov012_02169508(colosseum->card[self], gameData, HEAPID_UNION);
    colosseum->cardReady[self] = TRUE;
    return colosseum;
}

void Colosseum_Free(UnionColosseum *colosseum, GameSystem *gsys) {
    int i;

    for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
        GFL_HeapFree(colosseum->card[i]);
        GFL_HeapFree(colosseum->party[i]);
    }
    func_ov012_02161404(gsys, colosseum->commPlayer);
    GFL_HeapFree(colosseum);
}

void func_ov028_021729a0(UnionColosseum *colosseum, BOOL value) {
    colosseum->unkC8_0 = value;
}

void func_ov028_021729bc(UnionColosseum *colosseum, u8 value) {
    colosseum->unkDC = value;
    colosseum->unkDD = 0xff;
}

static u8 Colosseum_GetUnkDC(const UnionColosseum *colosseum) {
    return colosseum->unkDC;
}

static void Colosseum_SetUnkDD(UnionColosseum *colosseum, u32 value) {
    colosseum->unkDD = value;
}

u8 func_ov028_021729dc(const UnionColosseum *colosseum) {
    return colosseum->unkDD;
}

static BOOL Colosseum_Vote(UnionColosseum *colosseum, u32 netId, u8 value) {
    int i;
    BOOL accepted;

    if (value == 0xff) {
        colosseum->unkCC[netId] = value;
        accepted = TRUE;
    } else {
        for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
            if (value == colosseum->unkCC[i]) {
                accepted = FALSE;
                break;
            }
        }
        if (i == COLOSSEUM_MEMBER_MAX) {
            colosseum->unkCC[netId] = value;
            accepted = TRUE;
        }
    }
    colosseum->unkD0[netId] = accepted;
    return accepted;
}

void func_ov028_02172a1c(UnionColosseum *colosseum) {
    int i;

    if (func_02042bc4()) {
        for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
            u8 value = colosseum->unkD0[i];

            if (value != 0xff && Colosseum_SendUnk5(colosseum, i, value) == TRUE) {
                colosseum->unkD0[i] = 0xff;
            }
        }
    }
}

void func_ov028_02172a50(UnionColosseum *colosseum) {
    int count;
    int i;

    if (func_02042bc4()) {
        count = 0;
        for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
            if (colosseum->unkD4[i] == 1) {
                count++;
            }
        }
        if (count == func_02042a78()) {
            if (Colosseum_SendUnkB() == TRUE) {
                for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
                    colosseum->unkD4[i] = 0;
                    colosseum->unkD8[i] = 0;
                }
            }
        } else {
            for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
                if (colosseum->unkD8[i] == 1 && Colosseum_SendUnkA(i) == TRUE) {
                    colosseum->unkD8[i] = 0;
                }
            }
        }
    }
}

static void Colosseum_StoreChoice(UnionColosseum *colosseum, u32 netId, const ColosseumChoice *choice) {
    colosseum->choice[netId] = *choice;
    colosseum->choiceReady[netId] = TRUE;
}

u8 func_ov028_02172ae8(UnionColosseum *colosseum, u32 netId, ColosseumChoice *choice) {
    u8 ready;

    *choice = colosseum->choice[netId];
    ready = colosseum->choiceReady[netId];
    colosseum->choiceReady[netId] = FALSE;
    return ready;
}

void func_ov028_02172b14(UnionColosseum *colosseum) {
    colosseum->unk171 = 0;
}

BOOL func_ov028_02172b20(const UnionColosseum *colosseum) {
    int count = 0;
    int i;

    for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
        if (colosseum->partyReady[i] == 1) {
            count++;
        }
    }
    if (count >= func_02042a78()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov028_02172b4c(UnionColosseum *colosseum, BOOL keepOwn) {
    int self = func_02042a6c(func_02040440());
    int i;

    for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
        if (!keepOwn || self != i) {
            PokeParty_Init(colosseum->party[i]);
        }
        colosseum->partyReady[i] = FALSE;
    }
}

void Colosseum_BuildBattlePlayers(UnionColosseum *colosseum, GameSystem *gsys, ColosseumPlayers *players, u32 unused) {
    int member_max = 0;
    int i;
    int self;
    int tr_no;
    int selfTeam;
    int slot;

    sys_memset(players, 0, sizeof(ColosseumPlayers));
    players->battle.unk44 = 3;
    for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
        if (colosseum->partyReady[i] == 0) {
            players->battle.unk44 = 1;
            break;
        }
        member_max++;
    }
    GFL_ASSERT(member_max == 2 || member_max == 4);
    self = func_02042a6c(func_02040440());
    selfTeam = colosseum->unk16C[self];
    for (i = 0; i < member_max; i++) {
        u8 team = colosseum->unk16C[i];

        if ((team & 1) == (selfTeam & 1)) {
            slot = 0;
        } else if (players->battle.unk44 == 1) {
            slot = 1;
        } else {
            slot = 2;
        }
        tr_no = slot + (team >> 1);
        GFL_ASSERT(tr_no < member_max);
        Colosseum_SetPlayer(colosseum, &players->battle.players[tr_no], i, unused);
    }
    players->records = GameData_GetRecords(GSYS_GetGameData(gsys));
}

void func_ov028_02172c60(ColosseumPlayers *players) {
    sys_memset(players, 0, sizeof(ColosseumPlayers));
}

static void Colosseum_SetPlayer(UnionColosseum *colosseum, BattlePlayer *player, u8 index, u32 unused) {
    player->party = colosseum->party[index];
    player->info = &colosseum->members[index].info;
    player->unk08[1] = colosseum->members[index].unk28;
}
