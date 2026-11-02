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
// clang-format off
static const u8 sBattlePointRewards[7][10] = {
    { 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
    { 5, 6, 7, 8, 9, 10, 10, 10, 10, 10 },
    { 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
    { 5, 6, 7, 8, 9, 10, 10, 10, 10, 10 },
    { 3, 3, 3, 0, 0, 0, 0, 0, 0, 0 },
    { 5, 6, 7, 8, 9, 10, 10, 10, 10, 10 },
    { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10 },
};

// clang-format on

void BsubwayScr_ClearWorkPtr(GameSystem *gsys) {
    GameData_SetBSubwayScrWork(GSYS_GetGameData(gsys), NULL);
}

BSubwayScrWork *BsubwayScr_CreateWork(GameSystem *gsys, u16 resume, u16 playMode) {
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
    bsw->playData = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_PLAY);
    bsw->scoreData = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_SCORE);
    bsw->wifiData = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_3A);
    BSubwayPlayData_SetSaveFlag(bsw->playData, 0);
    GameData_SetBSubwayScrWork(gameData, bsw);
    if (resume == 0) {
        bsw->playMode = playMode;
        bsw->memberCount = BsubwayScr_GetMemberNum(bsw->playMode);
        for (i = 0; i < 4; i++)
            bsw->memberSlots[i] = 0xff;
        for (i = 0; i < 14; i++)
            bsw->trainerNos[i] = 0xffff;
        BSubwayPlayData_Init(bsw->playData);
        BSubwayPlayData_ResetRound(bsw->playData);
        if (BSubwayScoreData_HasStage(bsw->scoreData, bsw->playMode) == 1) {
            score = BSubwayScoreData_GetStreak(bsw->scoreData, bsw->playMode);
            BsubwayScr_SetWinCount(bsw, score);
        }
        value = bsw->playMode;
        BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_MODE, &value);
    } else {
        bsw->playMode = BSubwayPlayData_GetData(bsw->playData, BSUBWAY_PLAY_MODE, NULL);
        bsw->memberCount = BsubwayScr_GetMemberNum(bsw->playMode);
        if (BSubwayPlayData_GetData(bsw->playData, BSUBWAY_PLAY_USE_BATTLE_BOX, NULL) != 0) {
            BsubwayScr_LoadPartyCopy(bsw);
        }
        BSubwayPlayData_GetData(bsw->playData, BSUBWAY_PLAY_MEMBER_SLOTS, bsw->memberSlots);
        BSubwayPlayData_GetData(bsw->playData, BSUBWAY_PLAY_TRAINER_NOS, bsw->trainerNos);
        if (bsw->playMode == BSUBWAY_MODE_MULTI || bsw->playMode == BSUBWAY_MODE_SUPER_MULTI) {
            bsw->partnerIndex = (u8)BSubwayPlayData_GetData(bsw->playData, BSUBWAY_PLAY_PARTNER_INDEX, NULL);
            BSubwayPlayData_GetData(bsw->playData, BSUBWAY_PLAY_PARTNER_CONFIG, &bsw->teamConfigs[bsw->partnerIndex]);
            level = 303;
            if (getTrainerGender(&GameData_GetPlayerState(gameData)->playerInfo) != 0) {
                level -= 3;
            }
            teamIndex = bsw->partnerIndex;
            count = BSubwayPlayData_GetData(bsw->playData, BSUBWAY_PLAY_PARTNER_RENTAL_ITEM, NULL);
            BsubwayScr_CreatePartnerMons(bsw, &bsw->partners[teamIndex], level + teamIndex, count,
                                         &bsw->teamConfigs[teamIndex], bsw->heapId);
        }
        mode = bsw->playMode;
        if (BSubwayScoreData_HasStage(bsw->scoreData, mode) == 1) {
            count = BSubwayPlayData_GetRound(bsw->playData);
            score = BSubwayScoreData_GetStage(bsw->scoreData, mode);
            BsubwayScr_SetWinCountFromRound(bsw, score, count);
        }
    }
    return bsw;
}

void BsubwayScr_DeleteWork(GameSystem *gsys, BSubwayScrWork *bsw) {
    if (bsw != NULL) {
        if (bsw->allocatedBuffer != NULL) {
            GFL_HeapFree(bsw->allocatedBuffer);
            bsw->allocatedBuffer = NULL;
        }
        if (bsw->btlSetup != NULL) {
            BtlSetup_Free(bsw->btlSetup);
            bsw->btlSetup = NULL;
        }
        MI_CpuFill8(bsw, 0, sizeof(BSubwayScrWork));
        GFL_HeapFree(bsw);
    }
    GameData_SetBSubwayScrWork(GSYS_GetGameData(gsys), NULL);
}

void BsubwayScr_ChangeCommMultiMode(BSubwayScrWork *bsw) {
    u8 mode;
    u32 score;

    if (bsw->playMode == BSUBWAY_MODE_MULTI) {
        mode = BSUBWAY_MODE_COMM_MULTI;
    } else if (bsw->playMode == BSUBWAY_MODE_SUPER_MULTI) {
        mode = BSUBWAY_MODE_SUPER_COMM_MULTI;
    }
    bsw->playMode = mode;
    BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_MODE, &mode);
    if (BSubwayScoreData_HasStage(bsw->scoreData, bsw->playMode) == 1) {
        score = BSubwayScoreData_GetStreak(bsw->scoreData, bsw->playMode);
        BsubwayScr_SetWinCount(bsw, score);
    } else {
        BsubwayScr_ClearWinCount(bsw);
    }
}

void BsubwayScr_SaveRestPlayData(BSubwayScrWork *bsw) {
    u8 value;

    value = bsw->playMode;
    BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_MODE, &value);
    BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_MEMBER_SLOTS, bsw->memberSlots);
    BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_TRAINER_NOS, bsw->trainerNos);
    BSubwayPlayData_SetSaveFlag(bsw->playData, 1);
    if (bsw->playMode == BSUBWAY_MODE_MULTI || bsw->playMode == BSUBWAY_MODE_SUPER_MULTI) {
        value = bsw->partnerIndex;
        BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_PARTNER_INDEX, &value);
        BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_PARTNER_CONFIG, &bsw->teamConfigs[bsw->partnerIndex]);
        BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_PARTNER_RENTAL_ITEM,
                                bsw->partnerRentalItems + bsw->partnerIndex);
    }
}

void BsubwayScr_LoadMembers(BSubwayScrWork *bsw, GameSystem *gsys) {
    PokeParty *party;
    PartyPkm *pkm;
    s32 i;

    bsw->memberCount = BsubwayScr_GetMemberNum(bsw->playMode);
    BSubwayPlayData_GetData(bsw->playData, BSUBWAY_PLAY_MEMBER_SLOTS, bsw->memberSlots);
    party = BsubwayScr_GetParty(bsw);
    for (i = 0; i < bsw->memberCount; i++) {
        pkm = PokeParty_GetPkm(party, bsw->memberSlots[i]);
        bsw->memberSpecies[i] = PokeParty_GetParam(pkm, PKM_PARAM_SPECIES, NULL);
        bsw->memberItems[i] = PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL);
    }
}

void BsubwayScr_SavePlayDataMembers(BSubwayScrWork *bsw) {
    u8 value;

    value = bsw->playMode;
    BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_MODE, &value);
    BSubwayPlayData_ResetRound(bsw->playData);
    BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_MEMBER_SLOTS, bsw->memberSlots);
    BSubwayPlayData_SetSaveFlag(bsw->playData, 1);
    if (bsw->playMode == BSUBWAY_MODE_MULTI || bsw->playMode == BSUBWAY_MODE_SUPER_MULTI) {
        value = bsw->partnerIndex;
        BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_PARTNER_INDEX, &value);
        BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_PARTNER_CONFIG, &bsw->teamConfigs[bsw->partnerIndex]);
        BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_PARTNER_RENTAL_ITEM,
                                bsw->partnerRentalItems + bsw->partnerIndex);
    }
}

u16 BsubwayScr_ResetSavedPlay(GameSystem *gsys) {
    SaveControl *save;
    BSubwayPlayData *play;
    BSubwayScoreData *score;
    u8 mode;

    save = GameData_GetSaveControl(GSYS_GetGameData(gsys));
    play = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_PLAY);
    score = SaveControl_GetBlockPtr(save, SAVE_BLOCK_BSUBWAY_SCORE);
    mode = BSubwayPlayData_GetData(play, BSUBWAY_PLAY_MODE, NULL);
    BSubwayPlayData_ResetRound(play);
    BSubwayScoreData_ResetStage(score, mode);
    return mode;
}

void BsubwayScr_SetLoseScore(GameSystem *gsys, BSubwayScrWork *bsw) {
    u16 value;
    u8 mode;
    BSubwayScoreData *score;

    value = bsw->winCount;
    mode = bsw->playMode;
    score = bsw->scoreData;
    BSubwayScoreData_SetStreak(score, mode, value);
    BSubwayScoreData_UpdateMaxStreak(score, mode, value);
    BsubwayScr_SaveRecordParty(bsw, GameData_GetSaveControl(bsw->gameData), 1, value);
    BSubwayScoreData_ResetStage(bsw->scoreData, mode);
    BSubwayPlayData_ResetRound(bsw->playData);
}

u16 BsubwayScr_AddBattlePoints(BSubwayScrWork *bsw) {
    u16 count;
    u16 countIndex;
    s32 index;
    u16 category;
    u16 reward;
    SaveControl *save;
    const u8 *row;

    if (bsw->playMode == BSUBWAY_MODE_WIFI) {
        reward = 0;
        if ((u16)BSubwayPlayData_GetData(bsw->playData, BSUBWAY_PLAY_UNK0_10, NULL) == 1) {
            index = (s8)BSubwayScoreData_GetWifiRank(bsw->scoreData);
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
        count = BSubwayScoreData_GetStage(bsw->scoreData, bsw->playMode);
        switch (bsw->playMode) {
        case BSUBWAY_MODE_SINGLE:
            category = 0;
            break;
        case BSUBWAY_MODE_SUPER_SINGLE:
            category = 1;
            reward = 1;
            break;
        case BSUBWAY_MODE_DOUBLE:
            category = 2;
            break;
        case BSUBWAY_MODE_SUPER_DOUBLE:
            category = 3;
            reward = 1;
            break;
        case BSUBWAY_MODE_MULTI:
            category = 4;
            break;
        case BSUBWAY_MODE_COMM_MULTI:
            category = 4;
            break;
        case BSUBWAY_MODE_SUPER_MULTI:
            category = 5;
            reward = 1;
            break;
        case BSUBWAY_MODE_SUPER_COMM_MULTI:
            category = 5;
            reward = 1;
            break;
        case BSUBWAY_MODE_WIFI:
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
    BSubwayScoreData_AddBattlePoints(bsw->scoreData, reward);
    if (reward != 0) {
        save = GameData_GetSaveControl(bsw->gameData);
        RecordAdd(getTrainerCardInfoBlkAddress(save), 0x21, reward);
    }
    return reward;
}

void BsubwayScr_DrawTrainerNos(BSubwayScrWork *bsw) {
    u8 mode;
    u16 level;
    u16 choice;
    s32 i;

    mode = bsw->playMode;
    level = BsubwayScr_GetStageNo(BsubwayScr_GetWinCount(bsw));
    if (mode == BSUBWAY_MODE_MULTI || mode == BSUBWAY_MODE_COMM_MULTI || mode == BSUBWAY_MODE_SUPER_MULTI ||
        mode == BSUBWAY_MODE_SUPER_COMM_MULTI) {
        if (level < bsw->partnerStageNo) {
            level = bsw->partnerStageNo;
        }
        for (i = 0; i < 14; i++) {
            do {
                choice = BsubwayScr_DrawTrainerNo(bsw, level, (u8)(i / 2), mode, (u8)(i & 1));
            } while (BsubwayScr_IsTrainerNoUsed(bsw->trainerNos, choice, i));
            bsw->trainerNos[i] = choice;
        }
    } else {
        for (i = 0; i < 7; i++) {
            do {
                choice = BsubwayScr_DrawTrainerNo(bsw, level, (u8)i, bsw->playMode, 0);
            } while (BsubwayScr_IsTrainerNoUsed(bsw->trainerNos, choice, i));
            bsw->trainerNos[i] = choice;
        }
    }
}

u16 BsubwayScr_SetSelectedMembers(BSubwayScrWork *bsw, GameSystem *gsys) {
    PokeParty *party;
    PartyPkm *pkm;
    u16 i;

    if (bsw->pokeSelectResult != 0 || bsw->pokeSelectIndex == 7 || bsw->pokeSelectIndex == 8) {
        return 0;
    }
    party = BsubwayScr_GetParty(bsw);
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

BOOL BsubwayScr_IsWinMax(BSubwayScrWork *bsw) {
    if (!bsw->winMax) {
        if (BSubwayPlayData_GetRound(bsw->playData) < 7) {
            return FALSE;
        }
        bsw->winMax = 1;
    }
    return TRUE;
}

void BsubwayScr_SetClearScore(BSubwayScrWork *bsw, GameSystem *gsys) {
    u8 mode;
    u16 value;

    mode = bsw->playMode;
    value = BsubwayScr_GetWinCount(bsw);
    BSubwayScoreData_SetStreak(bsw->scoreData, mode, value);
    BSubwayScoreData_UpdateMaxStreak(bsw->scoreData, mode, BSubwayScoreData_GetStreak(bsw->scoreData, mode));
    BSubwayScoreData_IncStage(bsw->scoreData, mode);
    BsubwayScr_SaveRecordParty(bsw, GameData_GetSaveControl(bsw->gameData), 1, value);
    BSubwayPlayData_ResetRound(bsw->playData);
}

void BsubwayScr_ClearStateBits(BSubwayScrWork *bsw, GameSystem *gsys) {
    bsw->winMax = 0;
    bsw->unkC_1 = 0;
}

void BsubwayScr_CreateRoundTrainers(BSubwayScrWork *bsw) {
    u16 ids[2];
    u16 values[2];
    s32 index;
    u32 round;
    unsigned int slot;

    round = BSubwayPlayData_GetRound(bsw->playData);
    switch (bsw->playMode) {
    case BSUBWAY_MODE_WIFI:
        BSubwayWifiData_GetTrainer(bsw->wifiData, bsw->trainers, round, bsw->heapId);
        break;
    case BSUBWAY_MODE_MULTI:
    case BSUBWAY_MODE_COMM_MULTI:
    case BSUBWAY_MODE_SUPER_MULTI:
    case BSUBWAY_MODE_SUPER_COMM_MULTI:
        slot = round * 2;
        BsubwayScr_CreateTrainer(bsw, &bsw->trainers[0], bsw->trainerNos[slot], bsw->memberCount, NULL, NULL, NULL,
                                 bsw->heapId);
        for (index = 0; index < bsw->memberCount; index++) {
            ids[index] = bsw->trainers[0].pokemon[index].species;
            values[index] = bsw->trainers[0].pokemon[index].item;
        }
        BsubwayScr_CreateTrainer(bsw, &bsw->trainers[1], bsw->trainerNos[slot + 1], bsw->memberCount, ids, values, NULL,
                                 bsw->heapId);
        break;
    default:
        BsubwayScr_CreateTrainer(bsw, &bsw->trainers[0], bsw->trainerNos[round], bsw->memberCount, NULL, NULL, NULL,
                                 bsw->heapId);
        break;
    }
}

u32 BsubwayScr_GetTrainerObjCode(BSubwayScrWork *bsw, u16 index) {
    return BtlInst_GetTrainerClassObjCode(bsw->trainers[index].trainerId);
}

u16 BsubwayScr_UpdateStageRank(BSubwayScoreData *score, GameSystem *gsys, u32 op) {
    u8 value;
    u32 limit;

    value = BSubwayScoreData_GetWifiRank(score);
    switch (op) {
    case 0:
        return value;
    case 3:
        BSubwayScoreData_SetFlag(score, 0, BSUBWAY_FLAG_RESET);
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
            BSubwayScoreData_SetFlag(score, 0, BSUBWAY_FLAG_RESET);
            return 1;
        }
        return 0;
    default:
        return 0;
    }
}

void BsubwayScr_LoadPartyCopy(BSubwayScrWork *bsw) {
    if (bsw->allocatedBuffer != NULL) {
        GFL_HeapFree(bsw->allocatedBuffer);
    }
    bsw->allocatedBuffer =
        convertBoxedPokeSetToParty(getBattleBox(GameData_GetSaveControl(bsw->gameData)), HEAPID_GAMEEVENT);
}

PokeParty *BsubwayScr_GetParty(BSubwayScrWork *bsw) {
    if (BSubwayPlayData_GetData(bsw->playData, BSUBWAY_PLAY_USE_BATTLE_BOX, NULL) == 0) {
        return GameData_GetParty(bsw->gameData);
    }
    return bsw->allocatedBuffer;
}

u16 BsubwayScr_GetWinCount(BSubwayScrWork *bsw) {
    return bsw->winCount;
}

void BsubwayScr_SetWinCount(BSubwayScrWork *bsw, u32 value) {
    bsw->winCount = value;
}

void BsubwayScr_IncWinCount(BSubwayScrWork *bsw) {
    if (bsw->winCount < 0xffff) {
        bsw->winCount++;
    }
}

void BsubwayScr_ClearWinCount(BSubwayScrWork *bsw) {
    bsw->winCount = 0;
}

void BsubwayScr_SetWinCountFromRound(BSubwayScrWork *bsw, u32 count, u32 extra) {
    u32 value;

    value = count * 7;
    value += extra;

    if (value > 0xffff) {
        value = 0xffff;
    }
    bsw->winCount = value;
}

u16 BsubwayScr_GetMemberNum(u16 mode) {
    switch (mode) {
    case BSUBWAY_MODE_SINGLE:
    case BSUBWAY_MODE_WIFI:
    case BSUBWAY_MODE_SUPER_SINGLE:
        return 3;
    case BSUBWAY_MODE_DOUBLE:
    case BSUBWAY_MODE_SUPER_DOUBLE:
        return 4;
    case BSUBWAY_MODE_MULTI:
    case BSUBWAY_MODE_COMM_MULTI:
    case BSUBWAY_MODE_SUPER_MULTI:
    case BSUBWAY_MODE_SUPER_COMM_MULTI:
        return 2;
    default:
        return 0;
    }
}

BOOL BsubwayScr_IsTrainerNoUsed(const u16 *list, u16 value, u16 count) {
    u16 i;

    for (i = 0; i < count; i++) {
        if (list[i] == value) {
            return TRUE;
        }
    }
    return FALSE;
}

u16 BsubwayScr_GetStageNo(s32 value) {
    return value / 7;
}

void BsubwayScr_SaveRecordParty(BSubwayScrWork *bsw, SaveControl *save, u32 a2, u32 winCount) {
    u8 mode;

    switch (bsw->playMode) {
    case BSUBWAY_MODE_SINGLE:
        BsubwayScr_SaveMemberParty(bsw, save, 0);
        return;
    case BSUBWAY_MODE_WIFI:
        BsubwayScr_SaveMemberParty(bsw, save, 1);
        mode = bsw->playMode;
        BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_MODE, &mode);
        mode = BSubwayPlayData_GetRound(bsw->playData) + 1;
        BSubwayPlayData_SetData(bsw->playData, BSUBWAY_PLAY_ROUND, &mode);
        BSubwayScoreData_CalcWifiScore(bsw->scoreData, bsw->playData);
        return;
    case BSUBWAY_MODE_DOUBLE:
        return;
    }
}

void BsubwayScr_CreatePartners(BSubwayScrWork *bsw, u8 variant) {
    u32 base;
    s32 index;

    base = 0x12c;
    if (variant != 0) {
        base += 3;
    }
    for (index = 0; index < 3; index++) {
        bsw->partnerRentalItems[index] =
            BsubwayScr_CreateTrainer(bsw, &bsw->partners[index], base + index, bsw->memberCount, bsw->memberSpecies,
                                     bsw->memberItems, &bsw->teamConfigs[index], bsw->heapId);
    }
}

void BsubwayScr_PackBattleMon(BSubwayPokemon *dst, PartyPkm *pkm) {
    int i;

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

void BsubwayScr_SaveMemberParty(BSubwayScrWork *bsw, SaveControl *save, u32 flag) {
    BSubwayPokemon *team;
    PokeParty *party;
    s32 i;
    HeapID heapId = bsw->heapId;

    team = GFL_HeapAllocate(HEAPID_TAIL(heapId), sizeof(BSubwayPokemon) * 3, FALSE, "bsubway_scr.c", 0x8a1);
    MI_CpuFill8(team, 0, sizeof(BSubwayPokemon) * 3);
    party = BsubwayScr_GetParty(bsw);
    for (i = 0; i < 3; i++) {
        BsubwayScr_PackBattleMon(&team[i], PokeParty_GetPkm(party, bsw->memberSlots[i]));
    }
    BSubwayScoreData_SetTeam(bsw->scoreData, flag, team);
    MI_CpuFill8(team, 0, sizeof(BSubwayPokemon) * 3);
    GFL_HeapFree(team);
}
