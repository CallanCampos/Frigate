#ifndef POKEBW2_FIELD_FIELD_PLAYER_NOGRID_H
#define POKEBW2_FIELD_FIELD_PLAYER_NOGRID_H

// The field player's rail movement, overlay 36's field_player_nogrid.c. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

FieldPlayerNoGrid *FieldPlayerRail_Create(FieldPlayerCore *core, HeapID heapId);
void FieldPlayerRail_Delete(FieldPlayerNoGrid *nogrid);
void FieldPlayerRail_AttachActor(FieldPlayerNoGrid *nogrid, const RailPosition *pos);
void FieldPlayerRail_ClearRailWork(FieldPlayerNoGrid *nogrid);
void FieldPlayerRail_UpdateMovement(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys);
BOOL FieldPlayerRail_IsBlocked(FieldPlayerNoGrid *nogrid);
BOOL FieldPlayerRail_IsForcedMove(FieldPlayerNoGrid *nogrid);
void FieldPlayerRail_SetRailPos(FieldPlayerNoGrid *nogrid, const RailPosition *pos);
// swan's name, though it takes the rail player
void FieldPlayerCore_GetRailPos(FieldPlayerNoGrid *nogrid, RailPosition *pos);
void FieldPlayerRail_GetRailLocationByDir(FieldPlayerNoGrid *nogrid, u16 dir, RailPosition *pos);
void FieldPlayerRail_GetWorldPos(FieldPlayerNoGrid *nogrid, VecFx32 *pos);
RailUnit *FieldPlayerRail_GetRailWork(FieldPlayerNoGrid *nogrid);
void FieldPlayerRail_EndBlockedWalk(FieldPlayerNoGrid *nogrid);

#endif // POKEBW2_FIELD_FIELD_PLAYER_NOGRID_H
