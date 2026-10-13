#include "types.h"
#include "app/fmission_log_accessor_fapp.h"
#include "constants/flags.h"
#include "constants/vars.h"
#include "constants/version.h"
#include "field/fest_mission_data.h"
#include "field/festival.h"
#include "gfl/heap.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nitro/fx.h"
#include "save/event_work.h"
#include "save/funfest_savedata.h"
#include "save/high_link.h"
#include "save/medal_box.h"
#include "save/save_control.h"
#include "system/fest_mission.h"
#include "system/fest_mission_beacon.h"
#include "system/game_data.h"

// What the Funfest mission screen takes from the festival: the last lines of the mission's log and the mission's
// result, named after the ROM's "fmission_log_accessor_fapp.c"

#define BONUS_OVER 0
#define BONUS_RATE 1

// The game a mission belongs to, FestMissionHeader's unk01. The values from 3 are missions no medal counts
typedef enum {
    FEST_MISSION_VERSION_BOTH,
    FEST_MISSION_VERSION_WHITE2,
    FEST_MISSION_VERSION_BLACK2,
} FestMissionVersion;

static void FMissionLogFAppEntry_Clear(FMissionLogFAppEntry *entry);
static void FMissionLogFApp_GiveMedals(LinkFestival *festival, FMissionAppResult *result, MedalBox *medalBox,
                                       FunfestSave *funfest);

// The part of a total above the first value adds to the reward at the rate of the second
static const fx32 sFMissionLogFAppBonuses[][2] = {
    { 100, FX32_CONST(0.05) }, { 30, FX32_CONST(0.1) }, { 20, FX32_CONST(0.2) },
    { 10, FX32_CONST(0.5) },   { 0, FX32_CONST(1) },
};

FMissionLogFApp *FMissionLogFApp_Create(HeapID heapId) {
    FMissionLogFApp *accessor =
        GFL_HeapAllocate(heapId, sizeof(FMissionLogFApp), TRUE, "fmission_log_accessor_fapp.c", 55);
    int i;

    for (i = 0; i < FMISSION_LOG_FAPP_COUNT; i++) {
        accessor->entries[i].name = GFL_StrBufCreate(8, heapId);
    }
    accessor->count = 0;
    return accessor;
}

void FMissionLogFApp_Free(FMissionLogFApp *accessor) {
    int i;

    for (i = 0; i < FMISSION_LOG_FAPP_COUNT; i++) {
        GFL_StrBufFree(accessor->entries[i].name);
        accessor->entries[i].name = NULL;
    }
    accessor->count = 0;
    GFL_HeapFree(accessor);
}

BOOL FMissionLogFApp_Read(const FestMissionLog *log, FMissionLogFApp *accessor) {
    int i;

    accessor->count = 0;
    for (i = 0; i < FMISSION_LOG_FAPP_COUNT; i++) {
        FMissionLogFAppEntry_Clear(&accessor->entries[i]);
    }
    if (log->count == 0) {
        return FALSE;
    }
    for (i = log->count - 1; i >= 0; i--) {
        FMissionLogFAppEntry *entry = &accessor->entries[accessor->count];

        entry->trainerId = log->entries[i].trainerId;
        entry->score = log->entries[i].score;
        func_02015018(&log->entries[i], entry->name);
        accessor->count++;
        if (accessor->count >= FMISSION_LOG_FAPP_COUNT) {
            break;
        }
    }
    return TRUE;
}

static void FMissionLogFAppEntry_Clear(FMissionLogFAppEntry *entry) {
    GFL_StrBufClear(entry->name);
    entry->trainerId = 0;
    entry->score = 0;
}

void FMissionLogFApp_EndMission(LinkFestival *festival, FMissionAppResult *result) {
    SaveControl *save = GameData_GetSaveControl(festival->gameData);
    FunfestSave *funfest = func_02010dec(save);
    EventWork *eventWork = GameData_GetEventWork(festival->gameData);
    MedalBox *medalBox = SaveControl_GetMedalBox(save);
    int players;

    sys_memset(result, 0, sizeof(FMissionAppResult));
    LinkFestival_SetState(festival, 4);
    result->cleared = func_020148a8(festival, &result->total);
    if (!festival->info->unk0_30) {
        if (festival->info->index == 0 && !EventWork_FlagGet(eventWork, EVENT_FLAG_0x0986)) {
            EventWork_FlagSet(eventWork, EVENT_FLAG_0x0986);
            *EventWork_GetWkPtr(eventWork, EVENT_WORK_0x404d) = result->cleared + 2;
        }
        result->newCount = func_02010ed4(funfest, (u8)festival->info->index, result->total);
        result->newBest = func_02010f2c(funfest, (u8)festival->info->index, festival->config.missionId);
    }
    players = func_0201494c(festival);
    func_02010e80(funfest, (u8)func_0201494c(festival));
    if (players >= 10) {
        MedalBox_GiveMedal(medalBox, 221);
        if (players >= 30) {
            MedalBox_GiveMedal(medalBox, 222);
        }
    }
    if (players > 1 && func_02014a54(festival)) {
        func_02010e9c(funfest);
    }
    result->seconds = festival->config.unk20;
    if (result->cleared) {
        u16 score;
        BOOL second;
        fx32 sum;
        u16 gained;
        void *levels;
        fx32 rate;
        FestMissionInfo *info;
        u32 points;
        u16 level;
        int quotient;

        func_02010e58(funfest);
        if (!festival->info->unk0_30) {
            func_02010f80(funfest, (u8)festival->info->index, festival->info->level + 1);
            if (((FestMissionHeader *)festival->unk54)->unk02 != 0) {
                EventWork_FlagSet(eventWork, ((FestMissionHeader *)festival->unk54)->unk02);
            }
        }
        score = festival->config.missionId;
        second = FALSE;
        func_0201494c(festival);
        result->item = festival->info->unk4_0;
        result->itemCount = festival->info->unk4_10;
        info = festival->info;
        switch (info->unk8_30) {
        case 1: {
            fx32 perLevel = FX_Div(FX32_CONST(result->seconds), FX32_CONST(15));

            result->bonusCount = FX_Whole(FX_Div(FX_Mul(perLevel, FX32_CONST(festival->info->level + 1)), FX32_CONST(3)));
            result->bonusCount += score;
            break;
        }
        case 2: {
            int rest;
            u32 i;

            sum = FX32_CONST(score);
            rest = result->total - score;
            for (i = 0; i < NELEMS(sFMissionLogFAppBonuses); i++) {
                const fx32 *bonus = sFMissionLogFAppBonuses[i];

                if (rest > bonus[BONUS_OVER]) {
                    int over = rest - bonus[BONUS_OVER];

                    rate = bonus[BONUS_RATE];
                    sum += FX_Mul(FX32_CONST(over), rate);
                    rest -= over;
                }
            }
            result->bonusCount = FX_Whole(FX_Div(FX_Mul(sum, FX32_CONST(info->level + 1)), FX32_CONST(3)));
            second = FALSE;
            if (festival->info->level >= 2 && result->total >= festival->info->count * 2) {
                second = TRUE;
            }
            break;
        }
        }
        if (result->bonusCount > 99) {
            result->bonusCount = 99;
        }
        if (festival->info->unkC_22 != 0 && festival->info->unk10_0 != 0 && second) {
            result->item2 = festival->info->unkC_22;
            result->item2Count = festival->info->unk10_0;
        }
        levels = func_02017208(festival->gameData);
        points = func_02010fd4(funfest, festival->info->unk0_31);
        points += result->total;
        if (points > 1000) {
            points = 1000;
        }
        if (!festival->info->unk0_31) {
            level = func_0200c5dc(levels);
        } else {
            level = func_0200c5e0(levels);
        }
        gained = 0;
        do {
            int need = (level + 1) * 5;

            if (need > 50) {
                need = 50;
            }
            quotient = points / need;
            if (quotient > 0) {
                points -= need;
                gained++;
                level++;
            }
        } while (quotient != 0);
        if (gained >= 10) {
            gained = 10;
            points = 0;
        }
        result->isBlack = festival->info->unk0_31;
        if (!festival->info->unk0_31) {
            result->levels = HighLinkSave_AddWhiteLevel(levels, gained);
        } else {
            result->levels = HighLinkSave_AddBlackLevel(levels, gained);
        }
        func_02010fe4(funfest, festival->info->unk0_31, (u16)points);
        FMissionLogFApp_GiveMedals(festival, result, medalBox, funfest);
    }
}

static void FMissionLogFApp_GiveMedals(LinkFestival *festival, FMissionAppResult *result, MedalBox *medalBox,
                                       FunfestSave *funfest) {
    if (result->total >= 100) {
        MedalBox_GiveMedal(medalBox, 223);
        if (result->total >= 1000) {
            MedalBox_GiveMedal(medalBox, 224);
        }
    }
    if (result->cleared) {
        u32 completed = func_02010e50(funfest);
        // The other game's missions don't count
        FestMissionVersion excluded;
        int i;

        if (completed >= 30) {
            MedalBox_GiveMedal(medalBox, 229);
            if (completed >= 100) {
                MedalBox_GiveMedal(medalBox, 230);
            }
        }
        if (GAME_VERSION == VERSION_BLACK2) {
            excluded = FEST_MISSION_VERSION_WHITE2;
        } else {
            excluded = FEST_MISSION_VERSION_BLACK2;
        }
        for (i = 1; i < FEST_MISSION_COUNT; i++) {
            u8 kind = ((FestMissionHeader *)festival->headers)[i].unk01;

            if (kind >= 3 || excluded == kind || i == 44) {
                continue;
            }
            if (!func_02010f6c(funfest, i)) {
                break;
            }
        }
        if (i == FEST_MISSION_COUNT) {
            MedalBox_GiveMedal(medalBox, 231);
        }
    }
}
