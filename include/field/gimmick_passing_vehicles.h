#ifndef POKEBW2_FIELD_GIMMICK_PASSING_VEHICLES_H
#define POKEBW2_FIELD_GIMMICK_PASSING_VEHICLES_H

#include "types.h"
#include "struct_decls.h"

// A gimmick (overlay 128) of models that cross the zone along two lanes in opposite directions, with a sound when one
// passes the player. The name is a guess: the ROM has no string for it and the zone isn't known. Overlay 36's gimmick
// table calls these
void func_ov128_021eec80(Field *field);
void func_ov128_021eecd4(Field *field);
void func_ov128_021eed48(Field *field);

#endif // POKEBW2_FIELD_GIMMICK_PASSING_VEHICLES_H
