#ifndef POKEBW2_FIELD_FIELD_WATER_CURRENT_H
#define POKEBW2_FIELD_FIELD_WATER_CURRENT_H

#include "types.h"
#include "struct_decls.h"

// field_water_current.c, overlay 36: the event that carries the player along a water current tile, which
// fieldmap_event.c starts after a step onto one. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
GameEvent *EventWaterCurrentMove_Create(GameEvent *parent, GameSystem *gsys, Field *field);
BOOL IsTileWaterCurrent(u32 tileClass);

#endif // POKEBW2_FIELD_FIELD_WATER_CURRENT_H
