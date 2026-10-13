#ifndef POKEBW2_SYSTEM_SEASON_H
#define POKEBW2_SYSTEM_SEASON_H

#include "types.h"
#include "struct_decls.h"

#define SEASON_SPRING 0
#define SEASON_SUMMER 1
#define SEASON_AUTUMN 2
#define SEASON_WINTER 3
#define SEASON_COUNT 4

// season.c (the name is a guess). Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

// The season of the field, from the game data that the communication system takes as the base player's
u8 GameSystem_GetSeason(GameSystem *gsys);
// The season the game data has and the season of the date, which are the same while the season is kept in sync with
// another player; returns whether they differ
BOOL GameData_GetSeasons(GameData *gameData, u16 *prevSeason, u16 *season);
u8 Season_GetNext(u8 season);
u8 Season_GetPrevious(u8 season);
// The season of the date: spring in January, May and September, and so on
u8 Season_GetRealTime(void);
void Season_Set(GameData *gameData, u16 season);

#endif // POKEBW2_SYSTEM_SEASON_H
