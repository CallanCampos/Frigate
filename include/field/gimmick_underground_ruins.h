#ifndef POKEBW2_FIELD_GIMMICK_UNDERGROUND_RUINS_H
#define POKEBW2_FIELD_GIMMICK_UNDERGROUND_RUINS_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of the Underground Ruins (zone 565), overlay 114. Overlay 36's gimmick table calls the first three, and
// script plugin 16 (overlay 68) the others. The file's name is a guess

void UndergroundRuins_Init(Field *field);
void UndergroundRuins_End(Field *field);
void UndergroundRuins_Update(Field *field);
// Shows one of the four models that follow the first, from 0, hiding the others
void UndergroundRuins_ShowModel(GameSystem *gsys, u8 model);
// Puts the first model's animation at its end when atEnd is 1, else at its start
void UndergroundRuins_SetAnmEnd(GameSystem *gsys, u16 atEnd);
// Plays the first model's animation forward when forward is 1, else backward
void UndergroundRuins_PlayAnm(GameSystem *gsys, u16 forward);

#endif // POKEBW2_FIELD_GIMMICK_UNDERGROUND_RUINS_H
