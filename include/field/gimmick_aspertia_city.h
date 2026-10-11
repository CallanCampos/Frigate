#ifndef POKEBW2_FIELD_GIMMICK_ASPERTIA_CITY_H
#define POKEBW2_FIELD_GIMMICK_ASPERTIA_CITY_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of zone 427 (overlay 112): four models, one for each season, and a picture on BG 3 that script plugin 14
// (overlay 66) drives. The name is a guess from the zone's constant: the ROM has no string for it. Overlay 36's gimmick
// table calls the first three
void func_ov112_021eec80(Field *field);
void func_ov112_021eecb8(Field *field);
void func_ov112_021eece0(Field *field);
// Starts the animations of the model of the current season
void func_ov112_021eed04(GameSystem *gsys);
// Asks for the picture on BG 3 to be loaded on the next update
void func_ov112_021eed44(GameSystem *gsys);

#endif // POKEBW2_FIELD_GIMMICK_ASPERTIA_CITY_H
