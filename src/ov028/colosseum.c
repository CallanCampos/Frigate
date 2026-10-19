// The Union Room's colosseum (the file's name is from its allocations' strings): the work of up to four players, which
// Colosseum_Create allocates and the UnionSystem keeps, and the teams of a battle. Its net commands are
// colosseum_comm.c's
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
#include "nitro/os.h"
#include "pml/poke_party.h"
#include "system/game_data.h"
#include "system/game_system.h"

static void Colosseum_SetPlayer(UnionColosseum *colosseum, BattlePlayer *player, u8 index, u32 unused);

UnionColosseum *Colosseum_Create(GameData *gameData, GameSystem *gsys, const PlayerInfo *info, u32 flag) {
    UnionColosseum *colosseum;
    int self = GFL_NetGetHandleNetId(NetHandle_GetOwn());
    int i;
    u8 value;
    ColosseumMember *member;

    colosseum = GFL_HeapAllocate(HEAPID_UNION, sizeof(UnionColosseum), TRUE, "colosseum.c", 53);
    colosseum->commPlayer = CommPlayer_Init(4, gsys, HEAPID_UNION, 0);
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
    PlayerInfo_Copy(info, &member->info);
    OS_GetMacAddress(member->mac);
    member->unk26 = 1;
    value = 1;
    if (flag == 1) {
        value = 0;
    }
    member->unk27 = value;
    member->unk28 = 100;
    TrainerCardData_Fill(colosseum->cardCheck[self], gameData, TRUE, FALSE, HEAPID_UNION);
    TrainerCardData_FillExtras(colosseum->card[self], gameData, HEAPID_UNION);
    colosseum->cardReady[self] = TRUE;
    return colosseum;
}

void Colosseum_Free(UnionColosseum *colosseum, GameSystem *gsys) {
    int i;

    for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
        GFL_HeapFree(colosseum->card[i]);
        GFL_HeapFree(colosseum->party[i]);
    }
    CommPlayer_Exit(gsys, colosseum->commPlayer);
    GFL_HeapFree(colosseum);
}

void func_ov028_021729a0(UnionColosseum *colosseum, BOOL value) {
    colosseum->unkC8_0 = value;
}

void func_ov028_021729bc(UnionColosseum *colosseum, u32 value) {
    colosseum->unkDC = value;
    colosseum->unkDD = 0xff;
}

u8 Colosseum_GetUnkDC(const UnionColosseum *colosseum) {
    return colosseum->unkDC;
}

void Colosseum_SetUnkDD(UnionColosseum *colosseum, u32 value) {
    colosseum->unkDD = value;
}

u8 func_ov028_021729dc(const UnionColosseum *colosseum) {
    return colosseum->unkDD;
}

BOOL Colosseum_Vote(UnionColosseum *colosseum, u32 netId, u8 value) {
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

    if (GFL_NetIsParent()) {
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

    if (GFL_NetIsParent()) {
        count = 0;
        for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
            if (colosseum->unkD4[i] == 1) {
                count++;
            }
        }
        if (count == GFL_NetGetNegotiatedCount()) {
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

void Colosseum_StoreChoice(UnionColosseum *colosseum, u32 netId, const ColosseumChoice *choice) {
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
    if (count >= GFL_NetGetNegotiatedCount()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov028_02172b4c(UnionColosseum *colosseum, BOOL keepOwn) {
    int self = GFL_NetGetHandleNetId(NetHandle_GetOwn());
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

    MI_CpuFill8(players, 0, sizeof(ColosseumPlayers));
    players->battle.unk44 = 3;
    for (i = 0; i < COLOSSEUM_MEMBER_MAX; i++) {
        if (colosseum->partyReady[i] == 0) {
            players->battle.unk44 = 1;
            break;
        }
        member_max++;
    }
    GFL_ASSERT(member_max == 2 || member_max == 4);
    self = GFL_NetGetHandleNetId(NetHandle_GetOwn());
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
    MI_CpuFill8(players, 0, sizeof(ColosseumPlayers));
}

static void Colosseum_SetPlayer(UnionColosseum *colosseum, BattlePlayer *player, u8 index, u32 unused) {
    player->party = colosseum->party[index];
    player->info = &colosseum->members[index].info;
    player->unk08[1] = colosseum->members[index].unk28;
}
