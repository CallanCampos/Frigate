// The game's clock: what changes as minutes and days pass, and the date and time of the save. Function names from swan;
// the file's name is a guess after Platinum's ev_time.c
#include "types.h"
#include "field/ev_time.h"
#include "field/field.h"
#include "field/shaymin_form.h"
#include "field/survey.h"
#include "field/unity_tower.h"
#include "gfl/rtc_cache.h"
#include "nitro/rtc.h"
#include "pml/poke_party.h"
#include "save/adventure.h"
#include "save/encounter.h"
#include "save/event_work.h"
#include "save/join_avenue.h"
#include "save/musical_savedata.h"
#include "save/records.h"
#include "save/save_control.h"
#include "save/trainer_card.h"
#include "save/trainer_log.h"
#include "system/game_data.h"
#include "system/rtc.h"

static BOOL calcPokerusDecay(AdventureTime *adventureTime, const RTCDate *date, const RTCTime *time, s32 *days,
                             s32 *minutes);
static void midnight(GameData *gameData, s32 days);
static void func_ov012_021630e8(GameData *gameData, s32 minutes, const RTCTime *time);

void func_ov012_02162f44(GameData *gameData) {
    AdventureTime *adventureTime = getSaveAdventureTimeBlock(GameData_GetSaveControl(gameData));
    RTCDate date;
    RTCTime time;
    s32 days;
    s32 minutes;

    if (adventureTime->unk0 == 0) {
        return;
    }
    RTC_GetCachedDateTime(&date, &time);
    if (!calcPokerusDecay(adventureTime, &date, &time, &days, &minutes)) {
        return;
    }
    // A new day can't come without a minute passing
    if (minutes == 0 && days != 0) {
        return;
    }
    if (minutes != 0) {
        dayCountdownTest(adventureTime, minutes);
        func_ov012_021630e8(gameData, minutes, &time);
    }
    if (days != 0) {
        midnight(gameData, days);
    }
}

static BOOL calcPokerusDecay(AdventureTime *adventureTime, const RTCDate *date, const RTCTime *time, s32 *days,
                             s32 *minutes) {
    s64 last = func_0207d12c(&adventureTime->date, &adventureTime->time);
    s64 now = func_0207d12c(date, time);
    s32 day = func_0207d0b4(date);
    s32 passed;

    *days = 0;
    *minutes = 0;
    if (now < last) {
        adventureTime->date = *date;
        adventureTime->time = *time;
        adventureTime->day = day;
        return FALSE;
    }
    passed = (now - last) / 60;
    if (passed > 0) {
        *minutes = passed;
        adventureTime->date = *date;
        adventureTime->time = *time;
        if (day - adventureTime->day != 0) {
            *days = day - adventureTime->day;
            adventureTime->day = day;
        }
    }
    return TRUE;
}

static void midnight(GameData *gameData, s32 days) {
    BOOL fullDay = checkForMidnight(gameData);
    SaveControl *save = GameData_GetSaveControl(gameData);
    JoinAvenueSave *joinAvenue;
    JoinAvenueInfo *info;
    JoinAvenueOccupants *occupants;

    EventWork_ResetDailyFlags(GameData_GetEventWork(gameData));
    EncountSave_RerollSwarmLocation(save);
    pokerusDecay(GameData_GetParty(gameData), days);
    func_020095e0(GameData_GetRecords(gameData));
    func_02009c48(getUnityTower_SurveySaveBlkAddrress(save));
    func_0200edb0(func_0200ec2c(save));
    func_0200b220(func_0200afbc(save));
    func_0200ff78(getHollow_RivalBlk(save), 0);
    joinAvenue = SaveControl_GetJoinAvenue(save);
    info = JoinAvenue_GetInfo(joinAvenue);
    occupants = getAddressOfBeginningOfOccupants(joinAvenue);
    func_020392d4(info, fullDay);
    func_020389a0(occupants, fullDay);
}

static void func_ov012_021630e8(GameData *gameData, s32 minutes, const RTCTime *time) {
    PokeParty *party = SaveControl_GetPokePartySave(GameData_GetSaveControl(gameData));

    func_ov012_02164384(gameData, party, minutes, time, GameData_GetSeason(gameData));
    func_0200cb10(getTrainerGameInfoAddress(GameData_GetSaveControl(gameData)), minutes);
}

RTCDate *getAddressAdventureTimeBlk(GameData *gameData) {
    return &getSaveAdventureTimeBlock(GameData_GetSaveControl(gameData))->date;
}

u16 GameData_GetDayPeriod(GameData *gameData) {
    AdventureTime *adventureTime = getSaveAdventureTimeBlock(GameData_GetSaveControl(gameData));

    return GetDayPeriod(GameData_GetSeason(gameData), adventureTime->time.hour);
}

u32 GameData_GetMonth(GameData *gameData) {
    return getSaveAdventureTimeBlock(GameData_GetSaveControl(gameData))->date.month;
}

u32 GameData_GetDay(GameData *gameData) {
    return getSaveAdventureTimeBlock(GameData_GetSaveControl(gameData))->date.day;
}

u32 getCurrentDayOfWeek(GameData *gameData) {
    return getSaveAdventureTimeBlock(GameData_GetSaveControl(gameData))->date.week;
}

u32 getCurrentHour(GameData *gameData) {
    return getSaveAdventureTimeBlock(GameData_GetSaveControl(gameData))->time.hour;
}

u32 getCurrentMinute(GameData *gameData) {
    return getSaveAdventureTimeBlock(GameData_GetSaveControl(gameData))->time.minute;
}

void setCurrentSeconds(GameData *gameData) {
    AdventureTime *adventureTime = getSaveAdventureTimeBlock(GameData_GetSaveControl(gameData));

    adventureTime->seconds = RTC_ConvertSecondsCached();
}

BOOL checkForMidnight(GameData *gameData) {
    return hasFullDayMinutesPassed(getSaveAdventureTimeBlock(GameData_GetSaveControl(gameData)));
}
