#ifndef POKEBW2_FIELD_FIELD_PLAYER_NOGRID_H
#define POKEBW2_FIELD_FIELD_PLAYER_NOGRID_H

// The field player's rail movement, overlay 36's field_player_nogrid.c. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "gfl/heap.h"
#include "nitro/fx.h"
#include "struct_decls.h"

FieldPlayerNoGrid *func_ov036_0219cea0(FieldPlayerCore *core, HeapID heapId);
void func_ov036_0219cf40(FieldPlayerNoGrid *nogrid);
void func_ov036_0219cf48(FieldPlayerNoGrid *nogrid, const RailPosition *pos);
void func_ov036_0219cf84(FieldPlayerNoGrid *nogrid);
void FieldPlayerRail_UpdateMovement(FieldPlayerNoGrid *nogrid, u32 pressedKeys, u32 heldKeys);
BOOL func_ov036_0219d008(FieldPlayerNoGrid *nogrid);
BOOL func_ov036_0219d018(FieldPlayerNoGrid *nogrid);
void FieldPlayerRail_SetRailPos(FieldPlayerNoGrid *nogrid, const RailPosition *pos);
// swan's name, though it takes the rail player
void FieldPlayerCore_GetRailPos(FieldPlayerNoGrid *nogrid, RailPosition *pos);
void func_ov036_0219d044(FieldPlayerNoGrid *nogrid, u16 dir, RailPosition *pos);
void FieldPlayerRail_GetWorldPos(FieldPlayerNoGrid *nogrid, VecFx32 *pos);
RailUnit *func_ov036_0219d05c(FieldPlayerNoGrid *nogrid);
void func_ov036_0219d060(FieldPlayerNoGrid *nogrid);

#endif // POKEBW2_FIELD_FIELD_PLAYER_NOGRID_H
