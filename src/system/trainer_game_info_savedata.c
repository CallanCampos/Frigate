#include "types.h"
#include "save/trainer_game_info.h"
#include "app/name_entry.h"
#include "constants/arc.h"
#include "constants/script_text_banks.h"
#include "field/survey.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "text/script/castelia_city_13.h"

// The save block of the trainer's game info: the money, the badges, the favorite Pokémon, the greeting and thanks,
// the survey's answers, the Gym parties and the table of the gifts given once. The ROM has no string for the file,
// so the name is a guess. Block 0x34.

#define SAVE_BLOCK_TRAINER_GAME_INFO 0x34

static void TrainerGameInfo_ClearKeyboardTypes(TrainerGameInfoSave *info);

u32 TrainerGameInfo_GetSize(void) {
    return sizeof(TrainerGameInfoSave);
}

void TrainerGameInfo_Init(TrainerGameInfoSave *info) {
    MsgData *msgData;
    StrBuf *str;

    sys_memset(info, 0, sizeof(TrainerGameInfoSave));
    TrainerGameInfo_ClearKeyboardTypes(info);
    info->palParkHighScore = 0;

    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SCRIPT_MESSAGE, SCRIPT_TEXT_CASTELIA_CITY_13, HEAPID_TAIL(HEAPID_SAVEDATA));
    str = GFL_MsgDataLoadStrbufNew(msgData, CasteliaCity13_Text_Hello);
    GFL_StrBufStoreString(str, info->greeting, TRAINER_GAME_INFO_MESSAGE_LENGTH);
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(msgData, CasteliaCity13_Text_Thanks);
    GFL_StrBufStoreString(str, info->thanks, TRAINER_GAME_INFO_MESSAGE_LENGTH);
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);

    info->surveyRank = 0;
    info->surveyQuestion = 0;
    info->surveyAnswers[0] = 0xff;
    info->surveyAnswers[1] = 0xff;
    info->surveyAnswers[2] = 0xff;
}

TrainerCardSave *getTrainerCardData_wrapper(SaveControl *save) {
    return getTrainerGameInfoAddress(save);
}

TrainerGameInfoSave *getTrainerGameInfoAddress(SaveControl *save) {
    return SaveControl_GetBlockPtr(save, SAVE_BLOCK_TRAINER_GAME_INFO);
}

void func_0200c844(TrainerGameInfoSave *info, u32 index, u8 value) {
    info->keyboardTypes[index] = value;
}

u8 func_0200c84c(TrainerGameInfoSave *info, u32 index) {
    return info->keyboardTypes[index];
}

static void TrainerGameInfo_ClearKeyboardTypes(TrainerGameInfoSave *info) {
    sys_memset(info->keyboardTypes, 0, TRAINER_GAME_INFO_KEYBOARD_COUNT);
}

void SetFavePokemonData(TrainerGameInfoSave *info, u32 species, u32 form, u32 a3, int a4) {
    info->faveSpecies = species;
    info->faveForm = form;
    info->unk0a_6 = a3;
    info->unk0a_7 = a4 == 1;
}

void GetFavePokemonData(TrainerGameInfoSave *info, int *species, int *form, int *a3, int *a4) {
    *species = info->faveSpecies;
    *form = info->faveForm;
    *a3 = info->unk0a_6;
    *a4 = info->unk0a_7;
}

u32 TrainerGameInfo_GetPalParkHighScore(TrainerGameInfoSave *info) {
    return info->palParkHighScore;
}

void TrainerGameInfo_SetPalParkHighScore(TrainerGameInfoSave *info, u32 score) {
    info->palParkHighScore = score;
}

u8 TrainerGameInfo_GetPalParkResult(TrainerGameInfoSave *info) {
    return info->palParkResult;
}

void TrainerGameInfo_SetPalParkResult(TrainerGameInfoSave *info, u8 result) {
    info->palParkResult = result;
}

u32 func_0200c90c(TrainerCardSave *trainerCard) {
    return trainerCard->unk1c;
}

u32 func_0200c910(TrainerGameInfoSave *info) {
    if (info->unk1c < TRAINER_GAME_INFO_COUNT_MAX) {
        info->unk1c++;
    }
    return info->unk1c;
}

u32 func_0200c924(TrainerCardSave *trainerCard) {
    return trainerCard->unk20;
}

u32 func_0200c928(TrainerGameInfoSave *info) {
    if (info->unk20 < TRAINER_GAME_INFO_COUNT_MAX) {
        info->unk20++;
    }
    return info->unk20;
}

const u16 *TrainerGameInfo_GetGreeting(TrainerGameInfoSave *info) {
    return info->greeting;
}

void TrainerGameInfo_SetGreeting(TrainerGameInfoSave *info, StrBuf *name) {
    GFL_StrBufStoreString(name, info->greeting, TRAINER_GAME_INFO_MESSAGE_LENGTH);
}

const u16 *TrainerGameInfo_GetThanks(TrainerGameInfoSave *info) {
    return info->thanks;
}

void TrainerGameInfo_SetThanks(TrainerGameInfoSave *info, StrBuf *name) {
    GFL_StrBufStoreString(name, info->thanks, TRAINER_GAME_INFO_MESSAGE_LENGTH);
}

u16 func_0200c96c(TrainerGameInfoSave *info) {
    return info->surveyRank;
}

void func_0200c974(TrainerGameInfoSave *info, u32 value) {
    info->surveyRank = value;
}

BOOL isBadgeObtained(TrainerCardSave *trainerCard, u32 badgeId) {
    if (trainerCard->badges & (1 << badgeId)) {
        return TRUE;
    }
    return FALSE;
}

void addBadge(TrainerCardSave *trainerCard, u32 badgeId) {
    trainerCard->badges |= 1 << badgeId;
}

u32 getBadgeCount(TrainerGameInfoSave *info) {
    u32 count = 0;
    u32 badges;

    for (badges = info->badges; badges != 0; badges >>= 1) {
        if (badges & 1) {
            count++;
        }
    }
    return count;
}

u32 getCash(TrainerGameInfoSave *info) {
    return info->cash;
}

u32 addCashToTotal(TrainerGameInfoSave *info, u32 amount) {
    if (amount > TRAINER_GAME_INFO_CASH_MAX) {
        info->cash = TRAINER_GAME_INFO_CASH_MAX;
    } else {
        info->cash += amount;
    }
    if (info->cash > TRAINER_GAME_INFO_CASH_MAX) {
        info->cash = TRAINER_GAME_INFO_CASH_MAX;
    }
    return info->cash;
}

u32 subCashFromTotal(TrainerGameInfoSave *info, u32 amount) {
    if (info->cash < amount) {
        info->cash = 0;
    } else {
        info->cash -= amount;
    }
    return info->cash;
}

void SetGymDefeatPartyData(TrainerGameInfoSave *info, u32 gym, const u16 *species) {
    int i;

    for (i = 0; i < 6; i++) {
        info->gymParties[gym][i] = species[i];
    }
}

void GetGymDefeatPartyData(TrainerGameInfoSave *info, u32 gym, u16 *species) {
    int i;

    for (i = 0; i < 6; i++) {
        species[i] = info->gymParties[gym][i];
    }
}

void func_0200ca38(TrainerGameInfoSave *info, u32 item, u32 bits) {
    info->startMenuFlags |= bits << (item * 2);
}

u8 func_0200ca50(TrainerGameInfoSave *info, u32 item) {
    return (info->startMenuFlags >> (item * 2)) & 3;
}

u32 func_0200ca64(TrainerGameInfoSave *info) {
    return info->startMenuFlags;
}

void func_0200ca6c(TrainerGameInfoSave *info, u32 value) {
    info->startMenuFlags = value;
}

u32 func_0200ca74(TrainerGameInfoSave *info) {
    return info->unk0b;
}

void func_0200ca78(TrainerGameInfoSave *info, u32 value) {
    info->unk0b = value;
}

u16 func_0200ca7c(TrainerGameInfoSave *info) {
    return info->surveyQuestion;
}

void func_0200ca84(TrainerGameInfoSave *info, u8 value) {
    info->surveyQuestion = value;
}

u16 func_0200ca8c(TrainerGameInfoSave *info, u8 index) {
    return info->surveyAnswers[index];
}

void func_0200ca94(TrainerGameInfoSave *info, u8 index, u8 value) {
    info->surveyAnswers[index] = value;
}

BOOL func_0200ca9c(TrainerGameInfoSave *info, u8 index) {
    return info->surveyCounts[index];
}

void func_0200caa8(TrainerGameInfoSave *info, u8 index, u16 value) {
    info->surveyCounts[index] = value;
}

void func_0200cab4(TrainerGameInfoSave *info, u8 index, u16 count) {
    u32 total = info->surveyCounts[index];

    total += count;
    if (total > 0xffff) {
        total = 0xffff;
    }
    info->surveyCounts[index] = total;
}

s64 getSecondsFromTrainerCardData(TrainerGameInfoSave *info) {
    return info->surveySeconds;
}

void setSecondsCurrentTimeInTrainerCard(TrainerCardSave *trainerCard, s64 seconds) {
    trainerCard->surveySeconds = seconds;
}

void setToTrainerCardDRTable(TrainerCardSave *trainerCard, int flag, u32 key) {
    if (flag < TRAINER_GAME_INFO_DR_COUNT) {
        trainerCard->drTable[flag] = key;
    }
}

u32 getOneShotDRKeyFromSave(TrainerCardSave *trainerCard, int flag) {
    if (flag >= TRAINER_GAME_INFO_DR_COUNT) {
        return 0;
    }
    return trainerCard->drTable[flag];
}

u16 func_0200cb00(TrainerCardSave *trainerCard) {
    return trainerCard->unke4;
}

void func_0200cb08(TrainerCardSave *trainerCard, u16 value) {
    trainerCard->unke4 = value;
}

void func_0200cb10(TrainerGameInfoSave *info, u16 minutes) {
    if (minutes <= info->unke4) {
        info->unke4 -= minutes;
    } else {
        info->unke4 = 0;
    }
}

u8 func_0200cb30(TrainerCardSave *trainerCard) {
    return trainerCard->battleRecorderColor;
}

void func_0200cb3c(TrainerCardSave *trainerCard, u8 color) {
    trainerCard->battleRecorderColor = color;
}
