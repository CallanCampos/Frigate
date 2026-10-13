#include "system/season.h"
#include "types.h"
#include "field/intrude_work.h"
#include "nitro/rtc.h"
#include "system/game_data.h"
#include "system/game_system.h"

// The seasons, which change with the month. The file's name is a guess. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

u8 GameSystem_GetSeason(GameSystem *gsys) {
    return getSeasonFromPlayerData(GSYS_GetGameCommSystem(gsys));
}

BOOL GameData_GetSeasons(GameData *gameData, u16 *prevSeason, u16 *season) {
    if (GameData_IsForceSeasonSync(gameData)) {
        *prevSeason = GameData_GetSeason(gameData);
        *season = *prevSeason;
        return FALSE;
    }
    *prevSeason = GameData_GetSeason(gameData);
    *season = Season_GetRealTime();
    if (*prevSeason != *season) {
        return TRUE;
    }
    return FALSE;
}

u8 Season_GetRealTime(void) {
    RTCDate date;
    u8 season;

    RTC_GetCachedDate(&date);
    season = (date.month - 1) % SEASON_COUNT;
    return season;
}

u8 Season_GetNext(u8 season) {
    return (season + 1) % SEASON_COUNT;
}

u8 Season_GetPrevious(u8 season) {
    return (season + SEASON_COUNT - 1) % SEASON_COUNT;
}

void Season_Set(GameData *gameData, u16 season) {
    GameData_SetSeason(gameData, season);
}
