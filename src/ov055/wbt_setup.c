#include "types.h"
#include "constants/arc.h"
#include "field/field_actor.h"
#include "field/wbt.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "gfl/str.h"
#include "nitro/math.h"
#include "save/player_info.h"
#include "save/wbt_save.h"
#include "system/game_data.h"
#include "system/game_system.h"

static const u16 sScoreUnk[] = { 8, 13, 18, 23, 28 };

static void WbtSetup_SetEntrant(HeapID heapId, GameData *gameData, WbtSetupEntrant *dest, const WbtEntrant *entrant);
static void WbtSetup_SetMatches(WbtSystem *sys, WbtSetup *setup);
static u8 func_ov055_021e6a14(u32 index);
static void WbtSetup_LoadActorConfig(ArcTool *handle, u16 objCode, u8 *unk6, s16 *unk4);

WbtSetup *WbtSetup_Create(HeapID heapId, GameSystem *gsys) {
    WbtSetup *setup = GFL_HeapAllocate(heapId, sizeof(WbtSetup), TRUE, "wbt_setup.c", 57);
    int i;

    setup->gsys = gsys;
    setup->unk4 = 0;
    setup->tournamentName = GFL_StrBufCreate(40, heapId);
    setup->unk9C = GFL_StrBufCreate(40, heapId);
    for (i = 0; i < 8; i++) {
        WbtSetupEntrant *entrant = &setup->entrants[i];

        entrant->name = GFL_StrBufCreate(16, heapId);
        entrant->unk4 = 36;
        entrant->unk6 = 2;
        entrant->unk7 = 0;
        entrant->rank = 1;
        entrant->type = 0;
    }
    setup->unkA0 = 0;
    return setup;
}

void WbtSetup_Free(WbtSetup *setup) {
    int i;

    for (i = 0; i < 8; i++) {
        GFL_StrBufFree(setup->entrants[i].name);
    }
    GFL_StrBufFree(setup->tournamentName);
    GFL_StrBufFree(setup->unk9C);
    GFL_HeapFree(setup);
}

void WbtSetup_Load(GameSystem *gsys, WbtSystem *sys, WbtSetup *setup) {
    GameData *gameData = GSYS_GetGameData(gsys);
    MATHRandContext32 rand;
    int i;
    u32 round;

    for (i = 0; i < 8; i++) {
        WbtSetup_SetEntrant(HEAPID_FIELDMAP, gameData, &setup->entrants[i], WbtSystem_GetEntrant(sys, i));
    }
    MATH_InitRand32(&rand, WbtSystem_GetSeed(sys));
    WbtSystem_PlayFirstRounds(sys, &rand);
    WbtSetup_SetMatches(sys, setup);
    switch (WbtSystem_GetRound(sys)) {
    default:
    case 1:
        round = 0;
        break;
    case 2:
        round = 1;
        break;
    case 3:
        round = 2;
        break;
    }
    setup->unk4 = round;
    WbtSystem_LoadTournamentName(sys, WbtSystem_GetTournament(sys), setup->tournamentName);
    WbtStyle_LoadName(WbtSystem_GetStyle(sys), setup->unk9C);
}

static void WbtSetup_SetEntrant(HeapID heapId, GameData *gameData, WbtSetupEntrant *dest, const WbtEntrant *entrant) {
    ArcTool *handle;
    u16 objCode;

    dest->isPlayer = entrant->unk0_0 == 3 ? TRUE : FALSE;
    dest->rank = entrant->unk0_4;
    dest->type = entrant->type;
    dest->unk7 = entrant->unk0_3;
    WbtEntrant_GetName(entrant, dest->name);
    objCode = entrant->objCode;
    handle = GFL_ArcSysCreateFileHandle(ARCID_MMODEL_TBL, heapId);
    WbtSetup_LoadActorConfig(handle, objCode, &dest->unk6, &dest->unk4);
    GFL_ArcToolFree(handle);
}

static void WbtSetup_SetMatches(WbtSystem *sys, WbtSetup *setup) {
    WbtMatch *match;
    WbtSetupMatch *dest;
    int i;

    for (i = 0; i < 4; i++) {
        dest = &setup->firstRound[i];
        match = &sys->matches[i];
        dest->unk0 = match->firstWon;
        dest->unk4 = sScoreUnk[match->score];
    }
    for (i = 0; i < 2; i++) {
        dest = &setup->secondRound[i];
        match = &sys->matches[i + 4];
        dest->unk0 = match->firstWon;
        dest->unk4 = sScoreUnk[match->score];
    }
}

static u8 func_ov055_021e6a14(u32 index) {
    return data_ov036_021cf1c8[index].unk0[1]->face;
}

static void WbtSetup_LoadActorConfig(ArcTool *handle, u16 objCode, u8 *unk6, s16 *unk4) {
    FieldActorConfig record;
    u32 offset = GetIndexOfObjID(objCode) * sizeof(FieldActorConfig) + 4;

    GFL_ArcToolReadRange(handle, 0, offset, sizeof(record), &record);
    *unk6 = func_ov055_021e6a14(record.spriteControllerType);
    *unk4 = record.rscIndices.res1;
}

WbtOv326Param *WbtSetup_CreateWinRecordParam(HeapID heapId, GameSystem *gsys, u16 *var) {
    GameData *gameData = GSYS_GetGameData(gsys);
    WbtOv326Param *param = GFL_HeapAllocate(heapId, sizeof(WbtOv326Param), TRUE, "wbt_setup.c", 318);
    int i;

    param->unk0 = 0;
    param->save = func_020179f8(gameData);
    param->playerInfo = GetGameDataPlayerInfo(gameData);
    for (i = 0; i < 29; i++) {
        param->open[i] = WbtTournament_IsOpen(gameData, WbtTournament_FromRecord(i));
    }
    param->unk4 = var;
    return param;
}

void WbtSetup_FreeWinRecordParam(WbtOv326Param *param) {
    GFL_HeapFree(param);
}

WbtOv326Param2 *WbtSetup_CreateDownloadParam(HeapID heapId, GameSystem *gsys, u32 a2, u16 *var1, u16 *var2) {
    GameData *gameData = GSYS_GetGameData(gsys);
    WbtOv326Param2 *param = GFL_HeapAllocate(heapId, sizeof(WbtOv326Param2), TRUE, "wbt_setup.c", 390);

    param->gsys = gsys;
    param->unk8 = a2;
    param->unkC = var1;
    param->unk10 = var2;
    return param;
}

void WbtSetup_FreeDownloadParam(WbtOv326Param2 *param) {
    GFL_HeapFree(param);
}
