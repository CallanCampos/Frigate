#ifndef POKEBW2_FIELD_GIMMICK_UNDERGROUND_RUINS_H
#define POKEBW2_FIELD_GIMMICK_UNDERGROUND_RUINS_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of the Underground Ruins (zone 565), overlay 114. Overlay 36's gimmick table calls the first three, and
// script plugin 16 (overlay 68) the others. The file's name is a guess

void UndergroundRuins_Init(Field *field);
void UndergroundRuins_End(Field *field);
void UndergroundRuins_Update(Field *field);
// Shows model a1 + 1 of the four that follow the first, hiding the others
void UndergroundRuins_ShowModel(GameSystem *gsys, u8 a1);
// Puts the first model's animation at its end when a1 is 1, else at its start
void UndergroundRuins_SetAnmEnd(GameSystem *gsys, u16 a1);
// Plays the first model's animation forward when a1 is 1, else backward
void UndergroundRuins_PlayAnm(GameSystem *gsys, u16 a1);

#endif // POKEBW2_FIELD_GIMMICK_UNDERGROUND_RUINS_H
