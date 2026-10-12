#include "types.h"
#include "field/calender.h"
#include "field/zone.h"
#include "gfl/arc.h"
#include "gfl/heap.h"
#include "system/game_data.h"

#define CALENDAR_ZONE_MAX 24
// The weather of a zone with no weather by date
#define CALENDAR_NO_OFFSET 0xffff

static void Calendar_Reset(Calendar *calendar);
static void Calendar_BindArc(Calendar *calendar);
static void Calendar_ReleaseArc(Calendar *calendar);
static u16 Calendar_GetWeatherCore(Calendar *calendar, u16 zoneId);
static u16 Calendar_ReadWeatherValue(Calendar *calendar, u16 zoneId, u8 month, u8 day);
static void Calendar_Preload(Calendar *calendar);
static u16 Calendar_GetOffsetForZone(Calendar *calendar, u16 zoneId);
static u16 Calendar_GetDayOfYear(u8 month, u8 day);

Calendar *Calendar_Create(GameData *gameData, HeapID heapId) {
    Calendar *calendar = GFL_HeapAllocate(heapId, sizeof(Calendar), FALSE, "calender.c", 83);

    Calendar_Reset(calendar);
    calendar->heapId = heapId;
    calendar->gameData = gameData;
    Calendar_BindArc(calendar);
    Calendar_Preload(calendar);
    return calendar;
}

void Calendar_Free(Calendar *calendar) {
    Calendar_ReleaseArc(calendar);
    GFL_HeapFree(calendar);
}

u16 Calendar_GetWeather(Calendar *calendar, u16 zoneId) {
    return Calendar_GetWeatherCore(calendar, zoneId);
}

static void Calendar_Reset(Calendar *calendar) {
    calendar->heapId = 0;
    calendar->gameData = NULL;
    calendar->arc = NULL;
}

static void Calendar_BindArc(Calendar *calendar) {
    calendar->arc = GFL_ArcSysCreateFileHandle(96, calendar->heapId);
}

static void Calendar_ReleaseArc(Calendar *calendar) {
    GFL_ArcToolFree(calendar->arc);
    calendar->arc = NULL;
}

static u16 Calendar_GetWeatherCore(Calendar *calendar, u16 zoneId) {
    u32 month = GameData_GetMonth(calendar->gameData);
    u32 day = GameData_GetDay(calendar->gameData);

    return Calendar_ReadWeatherValue(calendar, zoneId, month, day);
}

static u16 Calendar_ReadWeatherValue(Calendar *calendar, u16 zoneId, u8 month, u8 day) {
    u16 offset = Calendar_GetOffsetForZone(calendar, zoneId);
    u8 weather;

    if (offset == CALENDAR_NO_OFFSET) {
        return GetZoneEnvFlagsWeather(zoneId);
    }
    offset += Calendar_GetDayOfYear(month, day);
    GFL_ArcToolReadRange(calendar->arc, 0, offset, sizeof(weather), &weather);
    return weather;
}

static void Calendar_Preload(Calendar *calendar) {
    GFL_ArcToolReadRange(calendar->arc, 1, 2, sizeof(calendar->zones), calendar->zones);
}

static u16 Calendar_GetOffsetForZone(Calendar *calendar, u16 zoneId) {
    int i;
    u16 offset = CALENDAR_NO_OFFSET;

    for (i = 0; i < CALENDAR_ZONE_MAX; i++) {
        if (zoneId == calendar->zones[i].zoneId) {
            offset = calendar->zones[i].offset;
            break;
        }
    }
    return offset;
}

static u16 Calendar_GetDayOfYear(u8 month, u8 day) {
    u8 days[13] = {0, 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int i;
    u16 total = 0;
    u16 dayOfMonth;

    for (i = 0; i < month; i++) {
        total += days[i];
    }
    dayOfMonth = day - 1;
    return total + dayOfMonth;
}
