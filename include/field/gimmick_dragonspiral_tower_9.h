#ifndef POKEBW2_FIELD_GIMMICK_DRAGONSPIRAL_TOWER_9_H
#define POKEBW2_FIELD_GIMMICK_DRAGONSPIRAL_TOWER_9_H

#include "types.h"
#include "nitro/fx.h"
#include "struct_decls.h"

// The gimmick of the top of Dragonspiral Tower, zone 213 (overlay 132): the scene that releases the Dark Stone's (or
// Light Stone's) power, and a model that is thrown. The name is a guess: the ROM has no string for it. Script plugin 14
// (overlay 66) drives it

// Overlay 36's gimmick table calls these
void GimmickDT9_Setup(Field *field);
void GimmickDT9_End(Field *field);
void GimmickDT9_Update(Field *field);

// Script plugin 14 calls these
// The event of the stone's release at the player's position, with its sounds and a flash of the screen
GameEvent *GimmickDT9_CreateReleaseEvent(GameSystem *gsys);
// The event that throws the second model from one position to another, its flight rising by height in the middle and
// lasting frames
GameEvent *GimmickDT9_CreateThrowEvent(GameSystem *gsys, const VecFx32 *from, const VecFx32 *to, fx32 height, u32 frames);
// Plays the second model's animations at the position
void GimmickDT9_PlayThrown(GameSystem *gsys, const VecFx32 *pos);
// The event that waits for the second model's first animation to pass its fifth frame
GameEvent *GimmickDT9_CreateWaitFifthFrameEvent(GameSystem *gsys);
// The event that waits for the second model's first animation to finish
GameEvent *GimmickDT9_CreateWaitThrownDoneEvent(GameSystem *gsys);

#endif // POKEBW2_FIELD_GIMMICK_DRAGONSPIRAL_TOWER_9_H
