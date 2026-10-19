#ifndef POKEBW2_FIELD_GYM_DRIFTVEIL_LIFT_H
#define POKEBW2_FIELD_GYM_DRIFTVEIL_LIFT_H

#include "types.h"
#include "struct_decls.h"

// The lift of the Driftveil City gym's lower floor (zone 98, gimmick 8), overlay 97. The file name is a guess: the
// overlay has no string. Overlay 36's gimmick table calls these
void DriftveilLift_Init(Field *field);
void DriftveilLift_End(Field *field);
void DriftveilLift_Update(Field *field);
// Overlay 36's gym script commands. Rides the lift up or down, the actor standing at (7, 4) riding along when it is
// the second lift
GameEvent *DriftveilLift_CreateMoveEvent(GameSystem *gsys, BOOL down, BOOL withRider);
// Puts the lift and the player at the bottom, and the rider when it is the second one
void DriftveilLift_PlaceAtBottom(GameSystem *gsys, u16 withRider);

#endif // POKEBW2_FIELD_GYM_DRIFTVEIL_LIFT_H
