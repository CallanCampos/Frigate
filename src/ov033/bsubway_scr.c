#include "types.h"
#include "battle/btl_setup.h"
#include "constants/pokemon.h"
#include "field/battle_facility.h"
#include "field/bsubway_scr.h"
#include "field/player_state.h"
#include "gfl/heap.h"
#include "gfl/random.h"
#include "gfl/std.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "save/bsubway_save.h"
#include "save/player_info.h"
#include "save/records.h"
#include "save/save_control.h"
#include "struct_decls.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/version.h"

// The losses in a row that lower a Wi-Fi rank, by rank
static const u8 sWifiRankLoseLimits[12] = { 0, 5, 4, 4, 3, 3, 2, 2, 1, 1, 0, 0 };

// The Battle Points a streak of seven gives, by kind of train and by streaks done; the last row is the Wi-Fi train's,
// by rank
static const u8 sBattlePointRewards[7][10] = {
    { 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
    { 5, 6, 7, 8, 9, 10, 10, 10, 10, 10 },
    { 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
    { 5, 6, 7, 8, 9, 10, 10, 10, 10, 10 },
    { 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
    { 5, 6, 7, 8, 9, 10, 10, 10, 10, 10 },
    { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10 },
};

void func_ov033_0217b468(GameSystem *gsys) {
    func_02017954(GSYS_GetGameData(gsys), NULL);
}

BSubwayScrWork *func_ov033_0217b478(GameSystem *gsys, u16 a1, u16 a2) {
    GameData *gameData;
    PlayerInfo *playerInfo;
    SaveControl *save;
    BSubwayScrWork *bsw;
    s32 i;
    u8 value;
    u32 count;
    u32 score;
    u32 level;
    u32 teamIndex;
    u8 mode;

    gameData = GSYS_GetGameData(gsys);
    playerInfo = GetGameDataPlayerInfo(gameData);
    save = GameData_GetSaveControl(gameData);
    bsw = GFL_HeapAllocate(4, 0x7f0, 1, "bsubway_scr.c", 0x5f);
    bsw->heapId = 4;
    bsw->magic = 0x12345678;
    bsw->gameData = gameData;
    bsw->gender = getTrainerGender(playerInfo);
    bsw->unk70 = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_PLAY);
    bsw->unk74 = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_SCORE);
    bsw->unk78 = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_3A);
    BSubwayPlayData_SetSaveFlag(bsw->unk70, 0);
    func_02017954(gameData, bsw);
    if (a1 == 0) {
        bsw->playMode = a2;
        bsw->memberCount = func_ov033_0217bdc0(bsw->playMode);
        for (i = 0; i < 4; i++) bsw->memberSlots[i] = 0xff;
        for (i = 0; i < 14; i++) bsw->unk32[i] = 0xffff;
        BSubwayPlayData_Init(bsw->unk70);
        BSubwayPlayData_ResetRound(bsw->unk70);
        if (BSubwayScoreData_HasStage(bsw->unk74, bsw->playMode) == 1) {
            score = BSubwayScoreData_GetStreak(bsw->unk74, bsw->playMode);
            func_ov033_0217bd88(bsw, score);
        }
        value = bsw->playMode;
        BSubwayPlayData_SetData(bsw->unk70, 0, &value);
    } else {
        bsw->playMode = BSubwayPlayData_GetData(bsw->unk70, 0, NULL);
        bsw->memberCount = func_ov033_0217bdc0(bsw->playMode);
        if (BSubwayPlayData_GetData(bsw->unk70, 10, NULL) != 0) {
            func_ov033_0217bd34(bsw);
        }
        BSubwayPlayData_GetData(bsw->unk70, 5, bsw->memberSlots);
        BSubwayPlayData_GetData(bsw->unk70, 8, bsw->unk32);
        if (bsw->playMode == 2 || bsw->playMode == 7) {
            bsw->unkC_5 = (u8)BSubwayPlayData_GetData(bsw->unk70, 9, NULL);
            BSubwayPlayData_GetData(bsw->unk70, 6, &bsw->teamConfigs[bsw->unkC_5]);
            level = 303;
            if (getTrainerGender(&GameData_GetPlayerState(gameData)->playerInfo) != 0) {
                level -= 3;
            }
            teamIndex = bsw->unkC_5;
            count = BSubwayPlayData_GetData(bsw->unk70, 7, NULL);
            func_ov033_0217c2c4(bsw, &bsw->unk2C8[teamIndex], level + teamIndex, count,
                                &bsw->teamConfigs[teamIndex], bsw->heapId);
        }
        mode = bsw->playMode;
        if (BSubwayScoreData_HasStage(bsw->unk74, mode) == 1) {
            count = BSubwayPlayData_GetRound(bsw->unk70);
            score = BSubwayScoreData_GetStage(bsw->unk74, mode);
            func_ov033_0217bda8(bsw, score, count);
        }
    }
    return bsw;
}

void func_ov033_0217b664(GameSystem *gsys, BSubwayScrWork *bsw) {
    if (bsw != NULL) {
        if (bsw->allocatedBuffer != NULL) {
            GFL_HeapFree(bsw->allocatedBuffer);
            bsw->allocatedBuffer = NULL;
        }
        if (bsw->btlSetup != NULL) {
            BtlSetup_Free(bsw->btlSetup);
            bsw->btlSetup = NULL;
        }
        sys_memset(bsw, 0, sizeof(BSubwayScrWork));
        GFL_HeapFree(bsw);
    }
    func_02017954(GSYS_GetGameData(gsys), NULL);
}

void func_ov033_0217b6b4(BSubwayScrWork *bsw) {
    u8 mode;
    u32 score;

    if (bsw->playMode == 2) {
        mode = 3;
    } else if (bsw->playMode == 7) {
        mode = 8;
    }
    bsw->playMode = mode;
    BSubwayPlayData_SetData(bsw->unk70, 0, &mode);
    if (BSubwayScoreData_HasStage(bsw->unk74, bsw->playMode) == 1) {
        score = BSubwayScoreData_GetStreak(bsw->unk74, bsw->playMode);
        func_ov033_0217bd88(bsw, score);
    } else {
        func_ov033_0217bda0(bsw);
    }
}

void func_ov033_0217b708(BSubwayScrWork *bsw) {
    u8 value;

    value = bsw->playMode;
    BSubwayPlayData_SetData(bsw->unk70, 0, &value);
    BSubwayPlayData_SetData(bsw->unk70, 5, bsw->memberSlots);
    BSubwayPlayData_SetData(bsw->unk70, 8, bsw->unk32);
    BSubwayPlayData_SetSaveFlag(bsw->unk70, 1);
    if (bsw->playMode == 2 || bsw->playMode == 7) {
        value = bsw->unkC_5;
        BSubwayPlayData_SetData(bsw->unk70, 9, &value);
        BSubwayPlayData_SetData(bsw->unk70, 6, &bsw->teamConfigs[bsw->unkC_5]);
        BSubwayPlayData_SetData(bsw->unk70, 7, bsw->unk664 + bsw->unkC_5);
    }
}

void func_ov033_0217b790(BSubwayScrWork *bsw, GameSystem *gsys) {
    PokeParty *party;
    PartyPkm *pkm;
    s32 i;

    bsw->memberCount = func_ov033_0217bdc0(bsw->playMode);
    BSubwayPlayData_GetData(bsw->unk70, 5, bsw->memberSlots);
    party = func_ov033_0217bd60(bsw);
    for (i = 0; i < bsw->memberCount; i++) {
        pkm = PokeParty_GetPkm(party, bsw->memberSlots[i]);
        bsw->memberSpecies[i] = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        bsw->memberItems[i] = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
    }
}

void func_ov033_0217b7e8(BSubwayScrWork *bsw) {
    u8 value;

    value = bsw->playMode;
    BSubwayPlayData_SetData(bsw->unk70, 0, &value);
    BSubwayPlayData_ResetRound(bsw->unk70);
    BSubwayPlayData_SetData(bsw->unk70, 5, bsw->memberSlots);
    BSubwayPlayData_SetSaveFlag(bsw->unk70, 1);
    if (bsw->playMode == 2 || bsw->playMode == 7) {
        value = bsw->unkC_5;
        BSubwayPlayData_SetData(bsw->unk70, 9, &value);
        BSubwayPlayData_SetData(bsw->unk70, 6, &bsw->teamConfigs[bsw->unkC_5]);
        BSubwayPlayData_SetData(bsw->unk70, 7, bsw->unk664 + bsw->unkC_5);
    }
}

u16 func_ov033_0217b86c(GameSystem *gsys) {
    SaveControl *save;
    BSubwayPlayData *play;
    BSubwayScoreData *score;
    u8 mode;

    save = GameData_GetSaveControl(GSYS_GetGameData(gsys));
    play = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_PLAY);
    score = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_SCORE);
    mode = BSubwayPlayData_GetData(play, 0, NULL);
    BSubwayPlayData_ResetRound(play);
    BSubwayScoreData_ResetStage(score, mode);
    return mode;
}

void func_ov033_0217b8ac(GameSystem *gsys, BSubwayScrWork *bsw) {
    u16 value;
    u8 mode;
    BSubwayScoreData *score;

    value = bsw->unkE;
    mode = bsw->playMode;
    score = bsw->unk74;
    BSubwayScoreData_SetStreak(score, mode, value);
    BSubwayScoreData_UpdateMaxStreak(score, mode, value);
    func_ov033_0217be2c(bsw, GameData_GetSaveControl(bsw->gameData), 1, value);
    BSubwayScoreData_ResetStage(bsw->unk74, mode);
    BSubwayPlayData_ResetRound(bsw->unk70);
}

u16 func_ov033_0217b8ec(BSubwayScrWork *bsw) {
    u16 count;
    u16 countIndex;
    s32 index;
    u16 category;
    u16 reward;
    SaveControl *save;
    const u8 *row;

    if (bsw->playMode == 4) {
        reward = 0;
        if ((u16)BSubwayPlayData_GetData(bsw->unk70, 11, NULL) == 1) {
            index = (s8)BSubwayScoreData_GetWifiRank(bsw->unk74);
            if (index < 0) {
                index = 0;
            } else if (index >= 10) {
                index = 9;
            }
            reward = sBattlePointRewards[6][index];
        } else {
            reward = 5;
        }
    } else {
        reward = 0;
        count = BSubwayScoreData_GetStage(bsw->unk74, bsw->playMode);
        switch (bsw->playMode) {
        case 0:
            category = 0;
            break;
        case 5:
            category = 1;
            reward = 1;
            break;
        case 1:
            category = 2;
            break;
        case 6:
            category = 3;
            reward = 1;
            break;
        case 2:
            category = 4;
            break;
        case 3:
            category = 4;
            break;
        case 7:
            category = 5;
            reward = 1;
            break;
        case 8:
            category = 5;
            reward = 1;
            break;
        case 4:
            category = 6;
            break;
        default:
            category = 0;
            break;
        }
        countIndex = count - 1;
        if ((s16)countIndex < 0) {
            countIndex = 0;
        } else if (countIndex >= 10) {
            countIndex = 9;
        }
        if (bsw->unkC_1 != 0) {
            if (reward == 1) {
                reward = 30;
            } else {
                reward = 10;
            }
        } else {
            row = sBattlePointRewards[category];
            reward = row[countIndex];
        }
    }
    if (reward == 0) {
        reward = 1;
    }
    BSubwayScoreData_AddBattlePoints(bsw->unk74, reward);
    if (reward != 0) {
        save = GameData_GetSaveControl(bsw->gameData);
        RecordAdd(getTrainerCardInfoBlkAddress(save), 0x21, reward);
    }
    return reward;
}

void func_ov033_0217b9dc(BSubwayScrWork *bsw) {
    u8 mode;
    u16 level;
    u16 choice;
    s32 i;

    mode = bsw->playMode;
    level = func_ov033_0217be1c(func_ov033_0217bd84(bsw));
    if (mode == 2 || mode == 3 || mode == 7 || mode == 8) {
        if (level < bsw->unk18) {
            level = bsw->unk18;
        }
        for (i = 0; i < 14; i++) {
            do {
                choice = func_ov033_0217c11c(bsw, level, (u8)(i / 2), mode, (u8)(i & 1));
            } while (func_ov033_0217bdf4(bsw->unk32, choice, i));
            bsw->unk32[i] = choice;
        }
    } else {
        for (i = 0; i < 7; i++) {
            do {
                choice = func_ov033_0217c11c(bsw, level, (u8)i, bsw->playMode, 0);
            } while (func_ov033_0217bdf4(bsw->unk32, choice, i));
            bsw->unk32[i] = choice;
        }
    }
}

u16 func_ov033_0217ba94(BSubwayScrWork *bsw, GameSystem *gsys) {
    PokeParty *party;
    PartyPkm *pkm;
    u16 i;

    if (bsw->unk84 != 0 || bsw->unk82 == 7 || bsw->unk82 == 8) {
        return 0;
    }
    party = func_ov033_0217bd60(bsw);
    for (i = 0; i < bsw->memberCount; i++) {
        if (bsw->memberChoices[i] - 1 >= 6) {
            bsw->memberChoices[i] = 1;
        }
        bsw->memberSlots[i] = bsw->memberChoices[i] - 1;
        pkm = PokeParty_GetPkm(party, bsw->memberSlots[i]);
        bsw->memberSpecies[i] = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        bsw->memberItems[i] = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
    }
    return 1;
}

BOOL func_ov033_0217bb20(BSubwayScrWork *bsw) {
    if (!bsw->unkC_0) {
        if (BSubwayPlayData_GetRound(bsw->unk70) < 7) {
            return FALSE;
        }
        bsw->unkC_0 = 1;
    }
    return TRUE;
}

void func_ov033_0217bb4c(BSubwayScrWork *bsw, GameSystem *gsys) {
    u8 mode;
    u16 value;

    mode = bsw->playMode;
    value = func_ov033_0217bd84(bsw);
    BSubwayScoreData_SetStreak(bsw->unk74, mode, value);
    BSubwayScoreData_UpdateMaxStreak(bsw->unk74, mode, BSubwayScoreData_GetStreak(bsw->unk74, mode));
    BSubwayScoreData_IncStage(bsw->unk74, mode);
    func_ov033_0217be2c(bsw, GameData_GetSaveControl(bsw->gameData), 1, value);
    BSubwayPlayData_ResetRound(bsw->unk70);
}

void func_ov033_0217bb98(BSubwayScrWork *bsw, GameSystem *gsys) {
    bsw->unkC_0 = 0;
    bsw->unkC_1 = 0;
}

void func_ov033_0217bbac(BSubwayScrWork *bsw) {
    u16 ids[2];
    u16 values[2];
    s32 index;
    u32 round;
    u32 slot;

    round = BSubwayPlayData_GetRound(bsw->unk70);
    switch (bsw->playMode) {
    case 4:
        BSubwayWifiData_GetTrainer(bsw->unk78, bsw->trainers, round, bsw->heapId);
        break;
    case 2:
    case 3:
    case 7:
    case 8:
        slot = round * 2;
        func_ov033_0217c264(bsw, &bsw->trainers[0], bsw->unk32[slot], bsw->memberCount, NULL, NULL, NULL,
                            bsw->heapId);
        for (index = 0; index < bsw->memberCount; index++) {
            ids[index] = bsw->trainers[0].pokemon[index].species;
            values[index] = bsw->trainers[0].pokemon[index].item;
        }
        func_ov033_0217c264(bsw, &bsw->trainers[1], bsw->unk32[slot + 1], bsw->memberCount, ids, values, NULL,
                            bsw->heapId);
        break;
    default:
        func_ov033_0217c264(bsw, &bsw->trainers[0], bsw->unk32[round], bsw->memberCount, NULL, NULL, NULL,
                            bsw->heapId);
        break;
    }
}

u32 func_ov033_0217bca0(BSubwayScrWork *bsw, u16 index) {
    return func_ov012_02162b38(bsw->trainers[index].trainerId);
}

u16 func_ov033_0217bcb4(BSubwayScoreData *score, GameSystem *gsys, u32 op) {
    u8 value;
    u32 limit;

    value = BSubwayScoreData_GetWifiRank(score);
    switch (op) {
    case 0:
        return value;
    case 3:
        BSubwayScoreData_SetFlag(score, 0, 2);
        if (value == 10) {
            return 0;
        }
        BSubwayScoreData_IncWifiRank(score);
        return 1;
    case 4:
        limit = BSubwayScoreData_SetWifiLoseCount(score, 3);
        if (value == 1) {
            return 0;
        }
        if (limit >= sWifiRankLoseLimits[value - 1]) {
            BSubwayScoreData_DecWifiRank(score);
            BSubwayScoreData_SetWifiLoseCount(score, 2);
            BSubwayScoreData_SetFlag(score, 0, 2);
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

void func_ov033_0217bd34(BSubwayScrWork *bsw) {
    if (bsw->allocatedBuffer != NULL) {
        GFL_HeapFree(bsw->allocatedBuffer);
    }
    bsw->allocatedBuffer =
        convertBoxedPokeSetToParty(getBattleBox(GameData_GetSaveControl(bsw->gameData)), HEAPID_GAMEEVENT);
}

PokeParty *func_ov033_0217bd60(BSubwayScrWork *bsw) {
    if (BSubwayPlayData_GetData(bsw->unk70, 10, NULL) == 0) {
        return GameData_GetParty(bsw->gameData);
    }
    return bsw->allocatedBuffer;
}

u16 func_ov033_0217bd84(BSubwayScrWork *bsw) {
    return bsw->unkE;
}

void func_ov033_0217bd88(BSubwayScrWork *bsw, u32 value) {
    bsw->unkE = value;
}

void func_ov033_0217bd8c(BSubwayScrWork *bsw) {
    if (bsw->unkE < 0xffff) {
        bsw->unkE++;
    }
}

void func_ov033_0217bda0(BSubwayScrWork *bsw) {
    bsw->unkE = 0;
}

void func_ov033_0217bda8(BSubwayScrWork *bsw, u32 count, u32 extra) {
    u32 value;

    value = count * 7;
    value += extra;

    if (value > 0xffff) {
        value = 0xffff;
    }
    bsw->unkE = value;
}

u16 func_ov033_0217bdc0(u16 mode) {
    switch (mode) {
    case 0:
    case 4:
    case 5:
        return 3;
    case 1:
    case 6:
        return 4;
    case 2:
    case 3:
    case 7:
    case 8:
        return 2;
    default:
        return 0;
    }
}

BOOL func_ov033_0217bdf4(const u16 *list, u16 value, u16 count) {
    u16 i;

    for (i = 0; i < count; i++) {
        if (list[i] == value) {
            return TRUE;
        }
    }
    return FALSE;
}

u16 func_ov033_0217be1c(s32 value) {
    return value / 7;
}

void func_ov033_0217be2c(BSubwayScrWork *bsw, SaveControl *save, u32 a2, u32 a3) {
    u8 mode;

    switch (bsw->playMode) {
    case 0:
        func_ov033_0217c010(bsw, save, 0);
        return;
    case 4:
        func_ov033_0217c010(bsw, save, 1);
        mode = bsw->playMode;
        BSubwayPlayData_SetData(bsw->unk70, 0, &mode);
        mode = BSubwayPlayData_GetRound(bsw->unk70) + 1;
        BSubwayPlayData_SetData(bsw->unk70, 1, &mode);
        BSubwayScoreData_CalcWifiScore(bsw->unk74, bsw->unk70);
        return;
    case 1:
        return;
    }
}

void func_ov033_0217be88(BSubwayScrWork *bsw, u8 variant) {
    u32 base;
    s32 index;

    base = 0x12c;
    if (variant != 0) {
        base += 3;
    }
    for (index = 0; index < 3; index++) {
        bsw->unk664[index] = func_ov033_0217c264(bsw, &bsw->unk2C8[index], base + index, bsw->memberCount,
                                                 bsw->memberSpecies, bsw->memberItems, &bsw->teamConfigs[index],
                                                 bsw->heapId);
    }
}

void func_ov033_0217bf04(BSubwayPokemon *dst, PartyPkm *pkm) {
    s32 i;

    dst->species = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
    dst->form = PokeParty_GetParam(pkm, PKM_PARAM_FORM, NULL);
    dst->item = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
    for (i = 0; i < 4; i++) {
        dst->moves[i] = PokeParty_GetParam(pkm, PKM_PARAM_MOVE1 + i, NULL);
        dst->ppUps |= PokeParty_GetParam(pkm, PKM_PARAM_MOVE1_PP_UP + i, NULL) << (2 * i);
    }
    dst->region = PokeParty_GetParam(pkm, PKM_PARAM_REGION, NULL);
    dst->id = PokeParty_GetParam(pkm, PKM_PARAM_ID, NULL);
    dst->personality = PokeParty_GetParam(pkm, PKM_PARAM_PID, NULL);
    dst->ivs.all = PokeParty_GetParam(pkm, PKM_PARAM_IVS_ALL, NULL);
    for (i = 0; i < 6; i++) {
        dst->evs[i] = PokeParty_GetParam(pkm, PKM_PARAM_EV_HP + i, NULL);
    }
    dst->ability = PokeParty_GetParam(pkm, PKM_PARAM_ABILITY, NULL);
    dst->happiness = PokeParty_GetParam(pkm, PKM_PARAM_HAPPINESS, NULL);
    PokeParty_GetParam(pkm, PKM_PARAM_NICKNAME_RAW, dst->nickname);
}

void func_ov033_0217c010(BSubwayScrWork *bsw, SaveControl *save, u32 flag) {
    BSubwayPokemon *team;
    PokeParty *party;
    s32 i;
    HeapID heapId = bsw->heapId;

    team = GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(BSubwayPokemon) * 3, FALSE, "bsubway_scr.c", 0x8a1);
    sys_memset(team, 0, sizeof(BSubwayPokemon) * 3);
    party = func_ov033_0217bd60(bsw);
    for (i = 0; i < 3; i++) {
        func_ov033_0217bf04(&team[i], PokeParty_GetPkm(party, bsw->memberSlots[i]));
    }
    BSubwayScoreData_SetTeam(bsw->unk74, flag, team);
    sys_memset(team, 0, sizeof(BSubwayPokemon) * 3);
    GFL_HeapFree(team);
}

BtlSetup *func_ov033_0217c094(BSubwayScrWork *bsw, GameSystem *gsys) {
    PokeParty *party;
    PokeParty *source;
    PartyPkm *pkm;
    BtlSetup *result;
    s32 i;

    party = PokeParty_Create(0x8004);
    source = func_ov033_0217bd60(bsw);
    PokeParty_InitCore(party, bsw->memberCount);
    for (i = 0; i < bsw->memberCount; i++) {
        pkm = PokeParty_GetPkm(source, bsw->memberSlots[i]);
        PokeParty_AddPkm(party, pkm);
    }
    result =
        SetupTrialHouseBattle(gsys, party, bsw->playMode, bsw->trainers, &bsw->unk2C8[bsw->unkC_5], bsw->memberCount);
    GFL_HeapFree(party);
    return result;
}

void *func_ov033_0217c110(BSubwayScrWork *bsw) {
    return bsw->unk74C;
}

// The tables from here on lie after the ones above in the ROM, each group sorted by size on its own, so this file is
// two original files. The second starts at func_ov033_0217c094, func_ov033_0217c110 or func_ov033_0217c11c.

// The trainers of the super trains from the fifth streak on
static const BSubwayTrainerRange sSuperTopTrainerRange[1] = {
    { 200, 299 },
};

// The trainers of a normal train's last car, by streak
static const BSubwayTrainerRange sNormalLastTrainerRanges[2] = {
    { 50, 69 },
    { 70, 89 },
};

// The trainers of a normal train's first six cars, by streak
static const BSubwayTrainerRange sNormalTrainerRanges[3] = {
    { 0, 49 },
    { 30, 69 },
    { 50, 109 },
};

// The trainers of a super train's first six cars, by streak
static const BSubwayTrainerRange sSuperTrainerRanges[4] = {
    { 110, 159 },
    { 140, 179 },
    { 160, 199 },
    { 180, 219 },
};

// The trainers of a super train's last car, by streak
static const BSubwayTrainerRange sSuperLastTrainerRanges[4] = {
    { 160, 179 },
    { 180, 199 },
    { 200, 219 },
    { 220, 239 },
};

u16 func_ov033_0217c11c(BSubwayScrWork *bsw, u16 level, u8 index, u32 mode, u8 side) {
    BOOL super;
    s32 boss;
    u16 trainerId;
    const BSubwayTrainerRange *ranges;
    u16 count;

    trainerId = 0;
    super = FALSE;
    boss = -1;
    switch (mode) {
    case 5:
    case 6:
    case 7:
    case 8:
        super = TRUE;
        break;
    }
    if (index == 6) {
        if (!super) {
            if (level == 2) {
                boss = 0;
            }
        } else if (level == 6) {
            boss = 1;
        }
    }
    if (boss != -1) {
        switch (mode) {
        case 0:
        case 5:
            if (boss == 0) {
                trainerId = 0x132;
            } else if (boss == 1) {
                trainerId = 0x133;
            }
            break;
        case 1:
        case 6:
            if (boss == 0) {
                trainerId = 0x134;
            } else if (boss == 1) {
                trainerId = 0x135;
            }
            break;
        case 2:
        case 3:
        case 7:
        case 8:
            if (boss == 0) {
                if (side == 0) {
                    trainerId = 0x137;
                } else {
                    trainerId = 0x136;
                }
            } else if (boss == 1) {
                if (side == 0) {
                    trainerId = 0x139;
                } else {
                    trainerId = 0x138;
                }
            }
            break;
        }
    } else if (!super) {
        if (level < 3) {
            if (index < 6) {
                ranges = sNormalTrainerRanges;
            } else if (level < 2) {
                ranges = sNormalLastTrainerRanges;
            } else {
                return trainerId;
            }
            count = ranges[level].max - ranges[level].min + 1;
            trainerId = ranges[level].min + randFFFFFFFFdivFFFF(bsw) % count;
        }
    } else if (level < 4) {
        if (index < 6) {
            ranges = sSuperTrainerRanges;
        } else {
            ranges = sSuperLastTrainerRanges;
        }
        count = ranges[level].max - ranges[level].min + 1;
        trainerId = ranges[level].min + randFFFFFFFFdivFFFF(bsw) % count;
    } else {
        count = sSuperTopTrainerRange[0].max - sSuperTopTrainerRange[0].min + 1;
        trainerId = sSuperTopTrainerRange[0].min + randFFFFFFFFdivFFFF(bsw) % count;
    }
    return trainerId;
}

BOOL func_ov033_0217c264(BSubwayScrWork *bsw, BSubwayTrainer *trainer, u16 trainerId, u32 count, const u16 *species,
                         const u16 *items, BSubwayTeamConfig *config, HeapID heapId) {
    return func_ov012_02162864(trainer, trainerId, count, species, items, config, heapId);
}

u8 func_ov033_0217c288(u32 value) {
    if (value < 100) {
        return 3;
    }
    if (value < 120) {
        return 6;
    }
    if (value < 140) {
        return 9;
    }
    if (value < 160) {
        return 12;
    }
    if (value < 180) {
        return 15;
    }
    if (value < 200) {
        return 18;
    }
    if (value < 220) {
        return 21;
    }
    return 31;
}

void func_ov033_0217c2c4(BSubwayScrWork *bsw, BSubwayTrainer *trainer, u16 trainerId, u32 count,
                         const BSubwayTeamConfig *config, HeapID heapId) {
    u8 iv;
    void *temp;
    s32 i;

    temp = func_ov012_021628c0(trainer, 0xd4, trainerId, 15, heapId);
    iv = func_ov033_0217c288(trainerId);
    for (i = 0; i < 2; i++) {
        func_ov012_02162490(&trainer->pokemon[i], 0xd3, config->files[i], config->id, config->pids[i], iv, i,
                            count, heapId);
    }
    GFL_HeapFree(temp);
}

u16 randFFFFFFFFdivFFFF(BSubwayScrWork *bsw) {
    return GFL_RandomLC(0xffffffff) / 0xffff;
}
