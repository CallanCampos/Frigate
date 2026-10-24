#ifndef POKEBW2_FIELD_RAIL_SLIPDOWN_H
#define POKEBW2_FIELD_RAIL_SLIPDOWN_H

#include "types.h"
#include "struct_decls.h"

// Sliding down a slope off a rail onto the rail below, overlay 131 (rail_slipdown.c, named from the ROM's string). The
// event that loads the overlay starts the slide, polls it until it is done, then deletes it.
// Starts the actor's slide in a task. stopPlayer is TRUE when the actor is the player, who is braked first and whom the
// camera follows down
RailSlipdown *RailSlipdown_Create(GameSystem *gsys, Field *field, FieldActor *actor, BOOL stopPlayer);
void RailSlipdown_Delete(RailSlipdown *work);
BOOL RailSlipdown_IsDone(RailSlipdown *work);

#endif // POKEBW2_FIELD_RAIL_SLIPDOWN_H
