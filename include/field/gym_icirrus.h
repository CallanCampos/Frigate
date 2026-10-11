#ifndef POKEBW2_FIELD_GYM_ICIRRUS_H
#define POKEBW2_FIELD_GYM_ICIRRUS_H

#include "types.h"
#include "struct_decls.h"

// The Icirrus City gym (zone 114, gimmick 9), overlay 98: three switches, each with two models that animate between
// its two states, which the gym's scripts flip with the commands of overlay 36's scrcmd_gym.c. The name is a guess:
// the overlay has no string, and the only scripts that use the commands are the zone's

// Overlay 36's gimmick table calls these
void func_ov098_021eec80(Field *field);
void func_ov098_021eedb8(Field *field);
void func_ov098_021eeddc(Field *field);

// The gym's script commands
// The event that plays a switch's animation, or NULL if there are fewer switches
GameEvent *func_ov098_021eee0c(GameSystem *gsys, int switchIndex);
// The same event with the sound, and the switch's state flipped when it ends
GameEvent *func_ov098_021eee48(GameSystem *gsys, int switchIndex);
// Switches the player between grid and rail movement if the switch is in the state that the place calls for
void func_ov098_021eee84(GameSystem *gsys, int place);

#endif // POKEBW2_FIELD_GYM_ICIRRUS_H
