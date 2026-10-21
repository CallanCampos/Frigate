#ifndef POKEBW2_FIELD_FIELD_SLOPE_SLIP_H
#define POKEBW2_FIELD_FIELD_SLOPE_SLIP_H

#include "types.h"
#include "struct_decls.h"

// field_slope_slip.c, overlay 36: the event that slides the player down a slope tile, which fieldmap_event.c starts
// after a step onto one. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
GameEvent *EventSlopeSlip_Create(GameEvent *parent, GameSystem *gsys, Field *field);
BOOL IsTileSlopeSlip(u32 tileClass);

#endif // POKEBW2_FIELD_FIELD_SLOPE_SLIP_H
