#ifndef POKEBW2_FIELD_GYM_HUMILAU_H
#define POKEBW2_FIELD_GYM_HUMILAU_H

#include "types.h"
#include "struct_decls.h"

// gym_humilau.c, overlay 101: the gimmick of Humilau City's gym, Marlon's floating platforms, which carry the player
// across the water when stepped onto from one of the gym's ride spots. The ROM doesn't name the file; the name is a
// guess, from overlay 36's gimmick table, whose gimmick 38 is this overlay and which ARCID_GIMMICK_TBL gives only to
// ZONE_HUMILAU_CITY_GYM, and from the sounds it plays (SEQ_SE_SW_MIZUGYM_01 and _02, "water gym", and
// SEQ_SE_SW_SHIZUI, Marlon's Japanese name). The names are ours

// Overlay 36's gimmick table calls these
void GymHumilau_Init(Field *field);
void GymHumilau_End(Field *field);
void GymHumilau_Update(Field *field);
// Called by overlay 36's tile event check; returns the event that rides the platform in front of the player when the
// player stands on a ride spot, or NULL
GameEvent *GymHumilau_CheckRide(GameSystem *gsys);

// The gym's script commands: the effects around the Gym Leader, placed at the actor with ID 0. The first plays
// SEQ_SE_SW_SHIZUI
void GymHumilau_PlayLeaderSplash(GameSystem *gsys);
void GymHumilau_PlayLeaderWaves(GameSystem *gsys);

#endif // POKEBW2_FIELD_GYM_HUMILAU_H
