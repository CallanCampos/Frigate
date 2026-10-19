#ifndef POKEBW2_FIELD_GYM_ICIRRUS_H
#define POKEBW2_FIELD_GYM_ICIRRUS_H

#include "types.h"
#include "struct_decls.h"

// The Icirrus City gym (zone 114, gimmick 9), overlay 98: three switches, each with two models that animate between
// its two states, which the gym's scripts flip with the commands of overlay 36's scrcmd_gym.c. The name is a guess:
// the overlay has no string, and the only scripts that use the commands are the zone's

// Overlay 36's gimmick table calls these
void GymIcirrus_Init(Field *field);
void GymIcirrus_End(Field *field);
void GymIcirrus_Update(Field *field);

// The gym's script commands
// The event that plays a switch's animation, or NULL if there are fewer switches
GameEvent *GymIcirrus_CreateSwitchAnimEvent(GameSystem *gsys, int switchIndex);
// The same event with the sound, and the switch's state flipped when it ends
GameEvent *GymIcirrus_CreateFlipSwitchEvent(GameSystem *gsys, int switchIndex);
// Switches the player between grid and rail movement if the switch is in the state that the place calls for
void GymIcirrus_ToggleMovementAtPlace(GameSystem *gsys, int place);

#endif // POKEBW2_FIELD_GYM_ICIRRUS_H
