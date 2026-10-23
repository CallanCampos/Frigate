#include "system/zone_weather.h"
#include "types.h"
#include "constants/version.h"
#include "field/calender.h"
#include "field/encounter.h"
#include "field/intrude_work.h"
#include "field/zone.h"
#include "nitro/rtc.h"
#include "save/event_work.h"
#include "system/dsi.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/season.h"
#include "system/version.h"

// Which weather a zone has. The ROM doesn't name the file; the name is a guess. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0), except EntralinkWeatherFlag and its fields

#define ZONE_ROUTE_7 0x151
// Set by the Route 7 event that changes its weather
#define FLAG_ROUTE_7_WEATHER 0x96d

// An Entralink zone, and the flag that picks between its two weathers
typedef struct {
    u16 zoneId;
    u16 flag;
} EntralinkWeatherFlag;

static BOOL IsSeasonCurrent(GameData *gameData);
static u32 GetEntralinkWeatherIfApplicable(GameSystem *gsys, GameData *gameData, u32 zoneId);
static u32 GetRoute7WeatherIfApplicable(GameSystem *gsys, GameData *gameData, u32 zoneId);
static u32 GetBirthdayZoneWeather(GameSystem *gsys, GameData *gameData, u32 zoneId);

static const EntralinkWeatherFlag ENTRALINK_WEATHER_FLAG_ZONE_LOOKUP[25] = {
    { 0x121, 0x9ad }, { 0x122, 0x9ae }, { 0x123, 0x9af }, { 0x124, 0x9b0 }, { 0x125, 0x9b1 },
    { 0x126, 0x9aa }, { 0x127, 0x9aa }, { 0x12d, 0x9ae }, { 0x12e, 0x9b1 }, { 0x12f, 0x9b4 },
    { 0x130, 0x9ad }, { 0x131, 0x9ae }, { 0x132, 0x9af }, { 0x133, 0x9b0 }, { 0x134, 0x9b1 },
    { 0x135, 0x9b1 }, { 0x136, 0x9b2 }, { 0x137, 0x9b2 }, { 0x138, 0x9b5 }, { 0x139, 0x9b4 },
    { 0x13a, 0x9ad }, { 0x13b, 0x9b2 }, { 0x13c, 0x9b5 }, { 0x129, 0x9ae }, { 0x12a, 0x9af },
};

u8 GetWeatherAll(GameSystem *gsys, u32 zoneId) {
    GameData *gameData = GSYS_GetGameData(gsys);
    Calendar *calendar = GetCalendar(gameData);
    u32 weather;

    weather = GetDefaultWeatherValue(gameData, zoneId);
    if (weather != WEATHER_NONE) {
        return weather;
    }
    weather = GetEntralinkWeatherIfApplicable(gsys, gameData, zoneId);
    if (weather != WEATHER_NONE) {
        return weather;
    }
    weather = GetRoute7WeatherIfApplicable(gsys, gameData, zoneId);
    if (weather != WEATHER_NONE) {
        return weather;
    }
    weather = GetBirthdayZoneWeather(gsys, gameData, zoneId);
    if (weather != WEATHER_NONE) {
        return weather;
    }
    if (IsSeasonCurrent(gameData)) {
        weather = Calendar_GetWeather(calendar, zoneId);
        return weather;
    }
    weather = GetZoneEnvFlagsWeather(zoneId);
    return weather;
}

static BOOL IsSeasonCurrent(GameData *gameData) {
    if (GameData_GetSeason(gameData) == Season_GetRealTime()) {
        return TRUE;
    }
    return FALSE;
}

void ResetWeather(GameSystem *gsys, u32 zoneId) {
    GameData *gameData = GSYS_GetGameData(gsys);
    u32 weather = GetWeatherAll(gsys, zoneId);

    SetNowWeather(gameData, weather);
}

void UpdateWeatherToDefault(GameData *gameData, u32 zoneId) {
    u32 weather = GetDefaultWeatherValue(gameData, zoneId);
    // The original narrows the weather to a u16 but tests it as an int: a u16 local tests it in u16 arithmetic
    int nowWeather = (u16)GetNowWeather(gameData);

    if (weather != WEATHER_NONE) {
        SetNowWeather(gameData, weather);
    } else if ((nowWeather == 6 || nowWeather == 7) && EncountSave_IsAnyEntryActive(gameData) == TRUE) {
        weather = GetZoneEnvFlagsWeather(zoneId);
        SetNowWeather(gameData, weather);
    }
}

// The Entralink's forest zones take the weather of the connected game, White's or Black's, in one of two kinds
static u32 GetEntralinkWeatherIfApplicable(GameSystem *gsys, GameData *gameData, u32 zoneId) {
    EventWork *eventWork = GameData_GetEventWork(gameData);
    GameCommSys *commSys = GSYS_GetGameCommSystem(gsys);
    u32 i;

    if (GetZoneIsEntralinkAny(zoneId) && commSys != NULL) {
        for (i = 0; i < 25; i++) {
            if (zoneId == ENTRALINK_WEATHER_FLAG_ZONE_LOOKUP[i].zoneId) {
                if (!EventWork_FlagGet(eventWork, ENTRALINK_WEATHER_FLAG_ZONE_LOOKUP[i].flag)) {
                    if (getGameOrigin(commSys) == VERSION_WHITE || getGameOrigin(commSys) == VERSION_WHITE2) {
                        return 13;
                    }
                    return 14;
                }
                if (getGameOrigin(commSys) == VERSION_WHITE || getGameOrigin(commSys) == VERSION_WHITE2) {
                    return 10;
                }
                return 11;
            }
        }
    }
    return WEATHER_NONE;
}

static u32 GetRoute7WeatherIfApplicable(GameSystem *gsys, GameData *gameData, u32 zoneId) {
    EventWork *eventWork = GameData_GetEventWork(gameData);

    if (zoneId == ZONE_ROUTE_7 && EventWork_FlagGet(eventWork, FLAG_ROUTE_7_WEATHER)) {
        if (getGameVersion() == VERSION_BLACK2) {
            return 7;
        }
        return 6;
    }
    return WEATHER_NONE;
}

// Weather 0 on Routes 14 and 15 on the player's birthday, from the DS's owner settings
static u32 GetBirthdayZoneWeather(GameSystem *gsys, GameData *gameData, u32 zoneId) {
    u8 month;
    u8 day;
    RTCDate date;

    if (GetIsZoneRoute14Or15(zoneId)) {
        RTC_GetCachedDate(&date);
        getBirthdayMonthDay(&month, &day);
        if (date.month == month && date.day == day) {
            return 0;
        }
    }
    return WEATHER_NONE;
}
