#include "field/wbt_makematch.h"
#include "types.h"
#include "battle/trainer_data.h"
#include "constants/types.h"
#include "field/battle_facility.h"
#include "field/wbt.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/math.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "save/save_control.h"
#include "save/wbt_save.h"
#include "system/game_data.h"

// wbt_makematch.c, named after the string the ROM keeps for its asserts (overlay 135, whose only file it is): the
// Pokémon World Tournament's entrance, which script plugin 6 drives. It picks the eight Trainers of a tournament,
// puts them into the bracket, and gives the party the player enters with and the screens that pick it

// What picks the Trainers: the table of all Trainers, and the tournament's rules
typedef struct {
    HeapID heapId;
    u32 tournament;
    u8 type;
    u16 trainerCount;
    WbtTrainer *trainers;
    u64 seed;
    // The seven Trainers of a downloaded tournament
    const WbtUnk18A *downloadSlots;
} WbtMatchParam;

typedef struct {
    const WbtMatchParam *param;
    MATHRandContext32 rand;
    // Per Trainer of the table: 0 when it isn't up for the pick, 1 when it is, 2 when it was picked
    u8 *status;
    // Per Trainer of the table: 0 when the player has beaten it, 1 otherwise
    u8 *encountered;
} WbtMakeMatch;

// One of the eight Trainers picked: the number of its Trainer, which is in the table unless the kind says otherwise
typedef union {
    struct {
        u8 index;
        u8 kind;
    };
    u16 value;
} WbtBracketEntry;

// The slots of the save's downloaded tournaments; slot 0 is none
typedef enum {
    WBT_DOWNLOAD_SLOT_FIRST = 1,
    WBT_DOWNLOAD_SLOT_END = 4
} WbtDownloadSlot;

#define WBT_STATUS_UNUSED 0
#define WBT_STATUS_CANDIDATE 1
#define WBT_STATUS_PICKED 2

// Any, for the count of Trainers the player has beaten
#define WBT_ANY 2

#define WBT_KIND_TABLE 0
// The first of four kinds that stand for a Trainer of the downloaded tournament
#define WBT_KIND_CAST_FIRST 0xf1
#define WBT_KIND_CAST_LAST 0xf4
#define WBT_KIND_PLAYER 0xff

static WbtMakeMatch *WbtMakeMatch_Create(HeapID heapId, void *record, const WbtMatchParam *param);
static void WbtMakeMatch_Free(WbtMakeMatch *m);
static void WbtMakeMatch_GetPicks(WbtMakeMatch *m, WbtBracketEntry *result);
static u32 WbtMakeMatch_CountCandidates(WbtMakeMatch *m, u32 status, u32 rank, u32 encountered);
static u32 WbtMakeMatch_FindCandidate(WbtMakeMatch *m, u32 rank, u32 encountered, u8 n);
static int WbtMakeMatch_PickRank(WbtMakeMatch *m, u32 rank, u8 count);
static BOOL WbtMakeMatch_IsEligible(const WbtMatchParam *param, WbtTrainer *trainer);
static void WbtMakeMatch_InitStatus(WbtMakeMatch *m, void *record);
static void WbtMakeMatch_PickDownloaded(const WbtUnk18A *slots, WbtMakeMatch *m, WbtBracketEntry *result);
static void WbtMakeMatch_PickTournament1(WbtMakeMatch *m, WbtBracketEntry *result);
static void WbtMakeMatch_PickTournament10(WbtMakeMatch *m, WbtBracketEntry *result);
static void WbtMakeMatch_PickTournaments5To9(WbtMakeMatch *m, WbtBracketEntry *result);
static void WbtMakeMatch_PickDefault(WbtMakeMatch *m, WbtBracketEntry *result);
static void WbtMakeMatch_PickTournament11(WbtMakeMatch *m, WbtBracketEntry *result);
static void WbtMakeMatch_PickTournament12(WbtMakeMatch *m, WbtBracketEntry *result);
static void WbtMakeMatch_PickTournament14(WbtMakeMatch *m, WbtBracketEntry *result);
static void WbtMakeMatch_PickTournament13(WbtMakeMatch *m, WbtBracketEntry *result);
static void WbtMakeMatch_PickTournament15(WbtMakeMatch *m, WbtBracketEntry *result);
static void WbtMakeMatch_PickTypeExpert(WbtMakeMatch *m, WbtBracketEntry *result);
static void WbtMakeMatch_PickBracket(GameData *gameData, WbtMatchParam *param, WbtBracketEntry *result);
static void WbtMakeMatch_ShuffleEntrants(MATHRandContext32 *rand, WbtEntrant **items, u32 count);
static void WbtMakeMatch_PlaceEntrants(WbtEntrant *entrants, u64 seed, HeapID heapId);
static void WbtEntrant_Set(WbtEntrant *entrant, u32 unk0_0, u32 unk2, u8 unk0_3, u8 rank, u8 unk3_0, u8 unk3_6,
                                BOOL unk0_7, u8 type, u16 objCode, u16 trainerClass, u8 unk6_12);
static void WbtEntrant_SetFromTable(WbtSystem *sys, HeapID heapId, WbtEntrant *entrant, u32 index);
static void WbtEntrant_SetPlayer(WbtEntrant *entrant, WbtSystem *sys, GameData *gameData);
static void WbtEntrant_SetDownloaded(WbtEntrant *entrant, const WbtDownloadedTrainer *trainer, u32 unk3_6, u8 rank);
static void WbtEntrant_SetTableForDownloaded(WbtEntrant *entrant, WbtSystem *sys, u8 index, WbtDownloadedTrainer *trainer,
                                u32 unk3_6, u32 rank);
static void WbtMakeMatch_SetEntrants(WbtSystem *sys, GameData *gameData, WbtBracketEntry *result);
static BOOL WbtMakeMatch_IsDownloadValid(GameData *gameData, u32 slot);
static void WbtMakeMatch_ClearDownload(WbtSystem *sys);

static WbtMakeMatch *WbtMakeMatch_Create(HeapID heapId, void *record, const WbtMatchParam *param) {
    WbtMakeMatch *m = GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(WbtMakeMatch), FALSE, "wbt_makematch.c", 71);

    m->param = param;
    m->status = GFL_HeapAllocate(HEAPID_TAIL(heapId), param->trainerCount, FALSE, "wbt_makematch.c", 74);
    m->encountered = GFL_HeapAllocate(HEAPID_TAIL(heapId), param->trainerCount, FALSE, "wbt_makematch.c", 75);
    WbtMakeMatch_InitStatus(m, record);
    MATH_InitRand32(&m->rand, param->seed);
    return m;
}

static void WbtMakeMatch_Free(WbtMakeMatch *m) {
    GFL_HeapFree(m->status);
    GFL_HeapFree(m->encountered);
    GFL_HeapFree(m);
}

// Puts the picked Trainers after the player in the order of the table
static void WbtMakeMatch_GetPicks(WbtMakeMatch *m, WbtBracketEntry *result) {
    u32 count;
    u32 i;

    count = 0;
    result[count++].value = 0xff00;
    for (i = 0; i < m->param->trainerCount; i++) {
        if (m->status[i] == WBT_STATUS_PICKED && count < 8) {
            result[count].kind = WBT_KIND_TABLE;
            result[count].index = i;
            count++;
        }
    }
}

// How many Trainers of the rank have the status, and have or haven't been beaten by the player
static u32 WbtMakeMatch_CountCandidates(WbtMakeMatch *m, u32 status, u32 rank, u32 encountered) {
    u32 count = 0;
    int i;

    for (i = 0; i < m->param->trainerCount; i++) {
        if (m->status[i] == status && (rank == 0 || rank == m->param->trainers[i].unk8_0) &&
            (encountered == WBT_ANY || encountered == m->encountered[i])) {
            count++;
        }
    }
    return count;
}

// The nth candidate of the rank that has or hasn't been beaten by the player, or the number of Trainers
static u32 WbtMakeMatch_FindCandidate(WbtMakeMatch *m, u32 rank, u32 encountered, u8 n) {
    u32 left = n;
    u32 i;

    for (i = 0; i < m->param->trainerCount; i++) {
        if (m->status[i] == WBT_STATUS_CANDIDATE && (rank == 0 || rank == m->param->trainers[i].unk8_0) &&
            (encountered == WBT_ANY || encountered == m->encountered[i])) {
            left--;
            if (left == 0) {
                return i;
            }
        }
    }
    return i;
}

// Picks count Trainers of the rank at random, the ones the player hasn't beaten first. Returns how many it
// couldn't pick
static int WbtMakeMatch_PickRank(WbtMakeMatch *m, u32 rank, u8 count) {
    u8 *status = m->status;
    u32 candidates = WbtMakeMatch_CountCandidates(m, WBT_STATUS_CANDIDATE, rank, WBT_ANY);
    u32 unbeaten = WbtMakeMatch_CountCandidates(m, WBT_STATUS_CANDIDATE, rank, 0);
    u32 remaining = count;
    int i;

    while (remaining != 0) {
        u32 pick;

        if (candidates == 1) {
            pick = WbtMakeMatch_FindCandidate(m, rank, WBT_ANY, 1);
        } else if (unbeaten == 0) {
            pick = WbtMakeMatch_FindCandidate(m, rank, WBT_ANY, MATH_Rand32(&m->rand, candidates) + 1);
        } else if (unbeaten == 1) {
            pick = WbtMakeMatch_FindCandidate(m, rank, 0, 1);
        } else {
            pick = WbtMakeMatch_FindCandidate(m, rank, 0, MATH_Rand32(&m->rand, unbeaten) + 1);
        }
        status[pick] = WBT_STATUS_PICKED;
        candidates--;
        unbeaten = WbtMakeMatch_CountCandidates(m, WBT_STATUS_CANDIDATE, rank, 0);
        remaining--;
        if (candidates == 0 && remaining != 0) {
            return remaining;
        }
    }
    for (i = 0; i < m->param->trainerCount; i++) {
        if (status[i] == WBT_STATUS_CANDIDATE && (rank == 0 || rank == m->param->trainers[i].unk8_0)) {
            status[i] = WBT_STATUS_UNUSED;
        }
    }
    return 0;
}

// Whether the tournament takes the Trainer
static BOOL WbtMakeMatch_IsEligible(const WbtMatchParam *param, WbtTrainer *trainer) {
    switch (param->tournament) {
    case 14:
        return trainer->inTournament14;
    case 15:
        return trainer->inTournament15;
    case 12:
        return trainer->inTournament12;
    case 13:
        return trainer->inTournament13;
    case 1:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        return trainer->tournament == param->tournament;
    case 2:
        return trainer->expertType == param->type || trainer->expertType == TYPE_NULL;
    case 4:
        return trainer->inTournament4;
    case 11:
        return trainer->inTournament11;
    case 10:
        return trainer->inTournament10;
    case 3:
        return TRUE;
    }
    return FALSE;
}

static void WbtMakeMatch_InitStatus(WbtMakeMatch *m, void *record) {
    int i;

    sys_memset(m->status, WBT_STATUS_UNUSED, m->param->trainerCount);
    sys_memset(m->encountered, 1, m->param->trainerCount);
    for (i = 0; i < m->param->trainerCount; i++) {
        if (WbtMakeMatch_IsEligible(m->param, &m->param->trainers[i]) == TRUE) {
            m->status[i] = WBT_STATUS_CANDIDATE;
            if (i < 0x40 && !func_0200fec8(record, i)) {
                m->encountered[i] = 0;
            }
        }
    }
}

// A downloaded tournament picks from the table ten Trainers at random and has seven slots, each a Trainer of the
// table, of the first or the second ten, or of the downloaded tournament, or by its number
static void WbtMakeMatch_PickDownloaded(const WbtUnk18A *slots, WbtMakeMatch *m, WbtBracketEntry *result) {
    u8 order[10];
    int i = 0;
    int next = 0;
    int n;

    for (n = 0; n < 10; n++) {
        order[n] = n;
    }
    for (n = 10; n > 0; n--) {
        u8 j = GFL_RandomLCAlt(n);
        u8 tmp = order[n - 1];

        order[n - 1] = order[j];
        order[j] = tmp;
    }
    sys_memset(result, 0, 8 * sizeof(WbtBracketEntry));
    result[i++].value = 0xff00;
    for (; i < 8; i++) {
        int slot = i - 1;

        switch (slots[slot].type) {
        case 0:
        default:
            result[i].kind = WBT_KIND_TABLE;
            result[i].index = 0x44 + order[next++];
            break;
        case 1:
            result[i].kind = WBT_KIND_TABLE;
            result[i].index = 0x4e + order[next++];
            break;
        case 3:
            result[i].kind = WBT_KIND_TABLE;
            result[i].index = slots[slot].unk2;
            break;
        case 2:
            result[i].kind = WBT_KIND_CAST_FIRST + slot;
            result[i].index = slots[slot].unk2;
            break;
        }
    }
}

// Tournament 1 takes its Trainers in the order of the table
static void WbtMakeMatch_PickTournament1(WbtMakeMatch *m, WbtBracketEntry *result) {
    int i;
    int count;

    sys_memset(result, 0, 8 * sizeof(WbtBracketEntry));
    count = 0;
    result[count++].value = 0xff00;
    for (i = 0; i < m->param->trainerCount; i++) {
        if (m->param->trainers[i].tournament == 1) {
            result[count].kind = WBT_KIND_TABLE;
            result[count].index = i;
            count++;
            if (count >= 8) {
                break;
            }
        }
    }
}

// Tournament 10
static void WbtMakeMatch_PickTournament10(WbtMakeMatch *m, WbtBracketEntry *result) {
    WbtMakeMatch_PickRank(m, 3, 7);
    WbtMakeMatch_GetPicks(m, result);
}

// Tournaments 5 to 9
static void WbtMakeMatch_PickTournaments5To9(WbtMakeMatch *m, WbtBracketEntry *result) {
    WbtMakeMatch_PickRank(m, 3, 7);
    WbtMakeMatch_GetPicks(m, result);
}

// Tournaments 0 and 4
static void WbtMakeMatch_PickDefault(WbtMakeMatch *m, WbtBracketEntry *result) {
    WbtMakeMatch_PickRank(m, 3, 2);
    WbtMakeMatch_PickRank(m, 2, 1);
    WbtMakeMatch_PickRank(m, 1, 4);
    WbtMakeMatch_GetPicks(m, result);
}

// Tournament 11
static void WbtMakeMatch_PickTournament11(WbtMakeMatch *m, WbtBracketEntry *result) {
    WbtMakeMatch_PickRank(m, 5, 1);
    WbtMakeMatch_PickRank(m, 4, 1);
    WbtMakeMatch_PickRank(m, 3, 1);
    WbtMakeMatch_PickRank(m, 1, 4);
    WbtMakeMatch_GetPicks(m, result);
}

// Tournament 12
static void WbtMakeMatch_PickTournament12(WbtMakeMatch *m, WbtBracketEntry *result) {
    WbtMakeMatch_PickRank(m, 3, 2);
    WbtMakeMatch_PickRank(m, 2, 2);
    WbtMakeMatch_PickRank(m, 1, 3);
    WbtMakeMatch_GetPicks(m, result);
}

// Tournament 14
static void WbtMakeMatch_PickTournament14(WbtMakeMatch *m, WbtBracketEntry *result) {
    WbtMakeMatch_PickRank(m, 3, 2);
    WbtMakeMatch_PickRank(m, 2, 2);
    WbtMakeMatch_PickRank(m, 1, 3);
    WbtMakeMatch_GetPicks(m, result);
}

// Tournament 13
static void WbtMakeMatch_PickTournament13(WbtMakeMatch *m, WbtBracketEntry *result) {
    WbtMakeMatch_PickRank(m, 3, 7);
    WbtMakeMatch_GetPicks(m, result);
}

// Tournament 15
static void WbtMakeMatch_PickTournament15(WbtMakeMatch *m, WbtBracketEntry *result) {
    WbtMakeMatch_PickRank(m, 3, 7);
    WbtMakeMatch_GetPicks(m, result);
}

// The Type Expert Tournament takes as many Trainers of each rank as there are
static void WbtMakeMatch_PickTypeExpert(WbtMakeMatch *m, WbtBracketEntry *result) {
    u32 rank3 = WbtMakeMatch_CountCandidates(m, WBT_STATUS_CANDIDATE, 3, WBT_ANY);
    u32 rank2;

    WbtMakeMatch_PickRank(m, 3, rank3);
    rank2 = WbtMakeMatch_CountCandidates(m, WBT_STATUS_CANDIDATE, 2, WBT_ANY);
    WbtMakeMatch_PickRank(m, 2, rank2);
    WbtMakeMatch_PickRank(m, 1, 7 - rank3 - rank2);
    WbtMakeMatch_GetPicks(m, result);
}

// Picks the eight Trainers of the tournament, the player first
static void WbtMakeMatch_PickBracket(GameData *gameData, WbtMatchParam *param, WbtBracketEntry *result) {
    WbtMakeMatch *m = WbtMakeMatch_Create(param->heapId, func_020179f8(gameData), param);

    switch (param->tournament) {
    default:
        WbtMakeMatch_PickDefault(m, result);
        break;
    case 1:
        WbtMakeMatch_PickTournament1(m, result);
        break;
    case 2:
        WbtMakeMatch_PickTypeExpert(m, result);
        break;
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
        WbtMakeMatch_PickTournaments5To9(m, result);
        break;
    case 10:
        WbtMakeMatch_PickTournament10(m, result);
        break;
    case 11:
        WbtMakeMatch_PickTournament11(m, result);
        break;
    case 14:
        WbtMakeMatch_PickTournament14(m, result);
        break;
    case 15:
        WbtMakeMatch_PickTournament15(m, result);
        break;
    case 12:
        WbtMakeMatch_PickTournament12(m, result);
        break;
    case 13:
        WbtMakeMatch_PickTournament13(m, result);
        break;
    case 3:
        WbtMakeMatch_PickDownloaded(param->downloadSlots, m, result);
        break;
    }
    WbtMakeMatch_Free(m);
}

// Shuffles the items
static void WbtMakeMatch_ShuffleEntrants(MATHRandContext32 *rand, WbtEntrant **items, u32 count) {
    while (count != 0) {
        u32 j = MATH_Rand32(rand, count);
        WbtEntrant *tmp;

        count--;
        tmp = items[count];
        items[count] = items[j];
        items[j] = tmp;
    }
}

// Where the highest ranks of the eight sit, by the player's place in the bracket
static const u8 sFirstPlace[8] = { 4, 4, 4, 4, 0, 0, 0, 0 };
static const u8 sSecondPlace[8] = { 2, 2, 0, 0, 6, 6, 4, 4 };
static const u8 sThirdPlace[8] = { 1, 0, 3, 2, 5, 4, 7, 6 };

static inline void WbtMakeMatch_SwapEntrants(WbtEntrant **items, u32 a, u32 b) {
    WbtEntrant *tmp = items[a];

    items[a] = items[b];
    items[b] = tmp;
}

// Puts the eight Trainers into the bracket's places: the player at random, the best two of the others as far from
// the player as they can be, and the rest at random
static void WbtMakeMatch_PlaceEntrants(WbtEntrant *entrants, u64 seed, HeapID heapId) {
    WbtEntrant *others[8];
    WbtEntrant *places[8];
    MATHRandContext32 rand;
    WbtEntrant *copy;
    int playerPlace;
    int i;
    int j;
    int next;

    MATH_InitRand32(&rand, seed);
    sys_memset(others, 0, sizeof(others));
    sys_memset(places, 0, sizeof(places));
    for (i = 0; i < 8; i++) {
        others[i] = &entrants[i];
    }
    WbtMakeMatch_ShuffleEntrants(&rand, others, 8);
    playerPlace = 8 - 9;
    for (i = 0; i < 8; i++) {
        if (func_ov055_021e5e34(others[i]) == 3) {
            places[i] = others[i];
            playerPlace = i;
            for (; i < 7; i++) {
                others[i] = others[i + 1];
            }
            break;
        }
    }
    others[7] = NULL;
    for (j = 0; j < 6; j++) {
        for (i = j + 1; i < 7; i++) {
            if (func_ov055_021e5e10(others[j]) < func_ov055_021e5e10(others[i])) {
                WbtMakeMatch_SwapEntrants(others, j, i);
            }
        }
    }
    places[(u8)(sFirstPlace[playerPlace] + MATH_Rand32(&rand, 4))] = others[0];
    places[(u8)(sSecondPlace[playerPlace] + MATH_Rand32(&rand, 2))] = others[1];
    places[sThirdPlace[playerPlace]] = others[2];
    WbtMakeMatch_ShuffleEntrants(&rand, &others[3], 4);
    for (next = 3, i = 0; i < 8; i++) {
        if (places[i] == NULL) {
            places[i] = others[next];
            next++;
            if (next >= 7) {
                break;
            }
        }
    }
    copy = GFL_HeapAllocate(HEAPID_TAIL(heapId), 8 * sizeof(WbtEntrant), FALSE, "wbt_makematch.c", 738);
    for (i = 0; i < 8; i++) {
        copy[i] = *places[i];
    }
    sys_memcpy32(copy, entrants, 8 * sizeof(WbtEntrant));
    GFL_HeapFree(copy);
}

static void WbtEntrant_Set(WbtEntrant *entrant, u32 unk0_0, u32 unk2, u8 unk0_3, u8 rank, u8 unk3_0, u8 unk3_6,
                                BOOL unk0_7, u8 type, u16 objCode, u16 trainerClass, u8 unk6_12) {
    entrant->unk0_0 = unk0_0;
    entrant->unk2 = unk2;
    entrant->unk0_3 = unk0_3;
    entrant->unk0_4 = rank;
    entrant->unk3_0 = unk3_0;
    entrant->unk3_6 = unk3_6;
    entrant->unk0_7 = unk0_7;
    entrant->type = type;
    entrant->objCode = objCode;
    entrant->unk6_0 = trainerClass;
    entrant->unk6_12 = unk6_12;
}

// A Trainer of the table
static void WbtEntrant_SetFromTable(WbtSystem *sys, HeapID heapId, WbtEntrant *entrant, u32 index) {
    WbtTrainer trainer;

    func_ov055_021e6440(sys->trainers, index, &trainer);
    WbtEntrant_Set(entrant, 0, index, trainer.gender, trainer.unk8_0, trainer.unk8_3, 0, trainer.unkE != 0,
                        trainer.type, trainer.unk2, trainer.trainerClass, 15);
    func_ov055_021e6388(heapId, index, entrant->name);
}

static void WbtEntrant_SetPlayer(WbtEntrant *entrant, WbtSystem *sys, GameData *gameData) {
    PlayerInfo *playerInfo = GetGameDataPlayerInfo(gameData);
    void *wbtSave = func_020179f8(gameData);
    u16 objCode = 0xe7;
    u8 isFemale = 0;
    u8 gender = getTrainerGender(playerInfo);
    u8 rank = 3;

    if (func_0200feac(wbtSave, 0x11)) {
        rank = 4;
    }
    if (gender == 1) {
        objCode = 0xf0;
        isFemale = 1;
    }
    WbtEntrant_Set(entrant, 3, 0, gender, rank, 0, 0, FALSE, func_ov055_021e64f0(sys->partyBC, sys->seed), objCode,
                        isFemale, 1);
    textCopy(playerInfo->name, entrant->name);
}

// A Trainer of a downloaded tournament
static void WbtEntrant_SetDownloaded(WbtEntrant *entrant, const WbtDownloadedTrainer *trainer, u32 unk3_6, u8 rank) {
    u32 trainerClass = func_ov055_021e67e4(trainer->unk3D4);
    u32 objCode = func_ov012_02162b38(trainerClass);

    WbtEntrant_Set(entrant, 1, 0, TrainerClass_GetSex(trainerClass), rank, 0, unk3_6, FALSE, TYPE_NULL, objCode,
                        trainerClass, trainer->unk3DC);
    GFL_StrBufLoadFixedString(entrant->name, trainer->name, 14);
    GFL_StrBufLoadFixedString(entrant->unkC, trainer->className, 22);
}

// A Trainer of the table that a downloaded tournament uses in place of one of its own; the original takes the
// downloaded Trainer and a rank too, and does nothing with them
static void WbtEntrant_SetTableForDownloaded(WbtEntrant *entrant, WbtSystem *sys, u8 index, WbtDownloadedTrainer *trainer,
                                u32 unk3_6, u32 rank) {
    WbtTrainer table;

    func_ov055_021e6440(sys->trainers, index, &table);
    WbtEntrant_Set(entrant, 2, index, table.gender, table.unk8_0, table.unk8_3, unk3_6, FALSE, table.type,
                        table.unk2, table.trainerClass, 15);
    func_ov055_021e6388(sys->heapId, index, entrant->name);
}

// Fills in the eight Trainers from the picks
static void WbtMakeMatch_SetEntrants(WbtSystem *sys, GameData *gameData, WbtBracketEntry *result) {
    int i;
    int rankIndex = 0;

    for (i = 0; i < 8; i++) {
        WbtEntrant *entrant = &sys->entrants[i];
        WbtDownloadedTrainer *trainer;
        u8 slot;

        switch (result[i].kind) {
        case WBT_KIND_PLAYER:
            WbtEntrant_SetPlayer(entrant, sys, gameData);
            func_ov055_021e63b4(sys->heapId, entrant->unk2, entrant->unkC);
            break;
        case WBT_KIND_CAST_FIRST:
        case WBT_KIND_CAST_FIRST + 1:
        case WBT_KIND_CAST_FIRST + 2:
        case WBT_KIND_CAST_LAST:
            slot = result[i].kind - WBT_KIND_CAST_FIRST;
            trainer = &sys->download.trainers[slot];
            if (result[i].index == 0) {
                WbtEntrant_SetDownloaded(entrant, trainer, result[i].kind - WBT_KIND_CAST_FIRST, 3);
            } else {
                u8 index = result[i].index - 1;

                WbtEntrant_SetTableForDownloaded(entrant, sys, index, trainer, result[i].kind - WBT_KIND_CAST_FIRST, 3);
                func_ov055_021e63b4(sys->heapId, index, entrant->unkC);
            }
            break;
        default:
            WbtEntrant_SetFromTable(sys, sys->heapId, entrant, result[i].index);
            // The rival's number in the table, whose name the player chose
            if (result[i].index == 0x37) {
                GFL_StrBufLoadString(entrant->name,
                                     getPtrToRivalName(getHollow_RivalData(GameData_GetSaveControl(gameData))));
            }
            func_ov055_021e63b4(sys->heapId, entrant->unk2, entrant->unkC);
            break;
        }
        if (sys->tournament == 3 && entrant->unk0_0 != 3) {
            entrant->unk0_4 = sys->downloadSlots[rankIndex].unk1;
            rankIndex++;
        }
        switch (sys->tournament) {
        case 12:
        case 13:
        case 14:
        case 15:
            entrant->type = TYPE_NULL;
            break;
        }
    }
}

void WbtMakeMatch_Make(WbtSystem *sys, GameData *gameData) {
    WbtBracketEntry result[8];
    WbtMatchParam param;

    sys_memset(&param, 0, sizeof(WbtMatchParam));
    param.heapId = sys->heapId;
    param.tournament = sys->tournament;
    param.type = sys->type;
    param.trainerCount = sys->trainers->count;
    param.trainers = sys->trainers->trainers;
    param.seed = sys->seed;
    param.downloadSlots = NULL;
    if (sys->downloadValid != 0) {
        param.downloadSlots = sys->downloadSlots;
    }
    WbtMakeMatch_PickBracket(gameData, &param, result);
    WbtMakeMatch_SetEntrants(sys, gameData, result);
    WbtMakeMatch_PlaceEntrants(sys->entrants, sys->seed, sys->heapId);
}

u8 WbtMakeMatch_GetBattlePoints(WbtSystem *sys) {
    if (sys->tournament == 1 || sys->tournament == 10) {
        u8 total;
        int i;

        for (i = 0, total = 0; i < 3; i++) {
            total += sys->unk13DC[i];
        }
        return total;
    }
    return func_ov055_021e67cc(sys->tournament, sys->style);
}

// Whether the downloaded tournament of the slot is in the save and its checksum holds
static BOOL WbtMakeMatch_IsDownloadValid(GameData *gameData, u32 slot) {
    SaveControl *save;
    u32 block;
    BOOL valid;

    block = slot + 0x10;
    valid = FALSE;
    save = GameData_GetSaveControl(gameData);

    if (func_020074ec(save, block, HEAPID_GAMEEVENT) == TRUE) {
        valid = func_02010428(getAddressOfExtraSaveBlk(save, block, 0));
    }
    freeIntermediateSaveExtraBlksAfterLoad(save, block);
    return valid;
}

// How many downloaded tournaments are in the save
u32 WbtMakeMatch_CountDownloads(WbtSystem *sys, GameData *gameData) {
    u32 count;
    WbtDownloadSlot slot;

    for (slot = WBT_DOWNLOAD_SLOT_FIRST, count = 0; slot < WBT_DOWNLOAD_SLOT_END; slot++) {
        if (WbtMakeMatch_IsDownloadValid(gameData, slot) == TRUE) {
            count++;
        }
    }
    return count;
}

static void WbtMakeMatch_ClearDownload(WbtSystem *sys) {
    sys_memset32(0, &sys->download, 0x1214);
    sys->downloadValid = FALSE;
}

// Loads the downloaded tournament that the slot of the system picked
void WbtMakeMatch_LoadDownload(WbtSystem *sys, GameData *gameData) {
    u32 slot;
    u32 block;
    BOOL valid;
    SaveControl *save;
    void *data;

    slot = sys->unk1D;
    valid = FALSE;
    if (slot == 0) {
        WbtMakeMatch_ClearDownload(sys);
        return;
    }
    block = slot + 0x10;
    save = GameData_GetSaveControl(gameData);
    if (func_020074ec(save, block, HEAPID_GAMEEVENT) == TRUE) {
        data = getAddressOfExtraSaveBlk(save, block, 0);
        valid = func_02010428(data);
    }
    if (valid) {
        sys_memcpy32(data, &sys->download, 0x1214);
        sys->style = func_ov055_021e64e8(sys->download.tournament.unkA7);
    } else {
        sys_memset32(0, &sys->download, 0x1214);
    }
    freeIntermediateSaveExtraBlksAfterLoad(save, block);
    sys->downloadValid = valid;
}

void WbtMakeMatch_ReadPick(WbtSystem *sys, PokeListParam *param) {
    sys_memcpy(param->picked, sys->castSlots, sizeof(sys->castSlots));
    sys->listIndex = param->index;
    sys->listResult = param->result;
    if (sys->listResult != 0 || sys->listIndex == 7 || sys->listIndex == 8) {
        *sys->unk13E0 = 0;
    } else {
        *sys->unk13E0 = 1;
    }
}

PokeParty *WbtMakeMatch_GetParty(WbtSystem *sys, GameData *gameData) {
    if (sys->tournament == 12 || sys->tournament == 13) {
        return sys->partyC4;
    }
    if (sys->unk13A8 == 0) {
        return GameData_GetParty(gameData);
    }
    return sys->unk13AC;
}

void WbtMakeMatch_LoadBattleBoxParty(WbtSystem *sys, GameData *gameData) {
    BattleBoxSave *battleBox = getBattleBox(GameData_GetSaveControl(gameData));

    if (BattleBoxSave_HasPkm(battleBox)) {
        sys->unk13AC = convertBoxedPokeSetToParty(battleBox, sys->heapId);
    }
}

void WbtMakeMatch_SetupPokeList(WbtSystem *sys, GameData *gameData, PokeListParam *param) {
    int mode;

    PokeListParam_Setup(param, gameData, 0x16, WbtMakeMatch_GetParty(sys, gameData));
    switch (sys->style) {
    case 0:
    default:
        mode = 0;
        break;
    case 1:
        mode = 1;
        break;
    case 2:
        mode = 3;
        break;
    case 3:
        mode = 4;
        break;
    }
    param->regulation = func_ov055_021e5f78(sys);
    param->unk18 = func_ov055_021e6024(sys);
    param->unk48 = mode;
}

void WbtMakeMatch_SetupPStatus(WbtSystem *sys, GameData *gameData, PStatusParam *param) {
    PokeDexSave *pokedex = GameData_GetPokedex(gameData);
    PokeParty *party;

    sys_memset32(0, param, sizeof(PStatusParam));
    party = WbtMakeMatch_GetParty(sys, gameData);
    param->party = party;
    param->dataType = PSTATUS_DATA_PARTY;
    param->partyCount = PokeParty_GetPkmCount(party);
    param->mode = PSTATUS_MODE_NORMAL;
    param->page = 0;
    param->gameData = gameData;
    param->isNationalDex = PokeDex_IsNationalObtained(pokedex);
}
