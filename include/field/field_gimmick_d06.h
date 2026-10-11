#ifndef POKEBW2_FIELD_FIELD_GIMMICK_D06_H
#define POKEBW2_FIELD_FIELD_GIMMICK_D06_H

#include "types.h"
#include "struct_decls.h"

// The gimmick 13 of Chargestone Cave (overlay 107, zones 195 to 197): stones that the player pushes along the cave's
// electric rock. The name is the ROM's string for the file, field_gimmick_d06.c

// Overlay 36's gimmick table calls these
void GimmickD06_Setup(Field *field);
void GimmickD06_End(Field *field);
void GimmickD06_Update(Field *field);

// Overlay 36's tile events call this with the player's direction: the event that pushes the stone in front of the
// player, or NULL
GameEvent *GimmickD06_CheckPush(Field *field, u16 dir);

#endif // POKEBW2_FIELD_FIELD_GIMMICK_D06_H
