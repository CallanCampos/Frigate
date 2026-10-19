#ifndef POKEBW2_FIELD_GIMMICK_HUMILAU_H
#define POKEBW2_FIELD_GIMMICK_HUMILAU_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of Route 21, Humilau City and Route 22 (zones 463, 465 and 474, gimmick 54), overlay 113

// Overlay 36's gimmick table calls these
void GimmickHumilau_Init(Field *field);
void GimmickHumilau_End(Field *field);
void GimmickHumilau_Update(Field *field);

// Script plugin 15 calls these. Shows the splash at a position given in grid squares, and plays its sound
void GimmickHumilau_ShowSplash(GameSystem *gsys, u16 x, u16 y, u16 z);
// Makes an actor jump out of the water and land on a grid square. a4 is not used
GameEvent *GimmickHumilau_CreateJumpEvent(GameSystem *gsys, u16 actorId, u16 x, u16 z, u16 a4);

#endif // POKEBW2_FIELD_GIMMICK_HUMILAU_H
