#ifndef POKEBW2_FIELD_EV_TIME_H
#define POKEBW2_FIELD_EV_TIME_H

// Overlay 12's ev_time.c, the game's clock. EvTime_Update is in field/field.h, and the getters of the date and
// time in system/game_data.h. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "nitro/rtc.h"
#include "struct_decls.h"

// The date the clock was last checked, followed by the time
RTCDate *getAddressAdventureTimeBlk(GameData *gameData);
// Saves the time in seconds
void setCurrentSeconds(GameData *gameData);

#endif // POKEBW2_FIELD_EV_TIME_H
