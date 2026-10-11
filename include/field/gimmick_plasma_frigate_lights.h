#ifndef POKEBW2_FIELD_GIMMICK_PLASMA_FRIGATE_LIGHTS_H
#define POKEBW2_FIELD_GIMMICK_PLASMA_FRIGATE_LIGHTS_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of zones 553 and 563 of the Plasma Frigate (overlay 119), which the frigate's script plugin drives: the
// four switches and the four lights beside them of overlay 117's gimmick (gimmick_plasma_frigate.h), from the same
// archive, without the panels. The name is a guess: the ROM has no string for it

// Overlay 36's gimmick table calls these
void func_ov119_021eec80(Field *field);
void func_ov119_021eeca8(Field *field);
void func_ov119_021eecc0(Field *field);

// The frigate's script plugin (plugin 12) calls these
// Shows the switch's light in the state that second says: with it, the end of the second animation of the light and of
// the switch, and without it, the first animation of the light looping and the end of the switch's first
void func_ov119_021eecd0(GameSystem *gsys, int index, BOOL second);
// Plays the second animation of the switch, with a sound
void func_ov119_021eed10(GameSystem *gsys, int index);
// Plays the second animation of the switch's light, with a sound
void func_ov119_021eed34(GameSystem *gsys, int index);

#endif // POKEBW2_FIELD_GIMMICK_PLASMA_FRIGATE_LIGHTS_H
