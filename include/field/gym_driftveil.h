#ifndef POKEBW2_FIELD_GYM_DRIFTVEIL_H
#define POKEBW2_FIELD_GYM_DRIFTVEIL_H

#include "types.h"
#include "struct_decls.h"

// The Driftveil City gym's upper floor (zone 99, gimmick 7), overlay 96: pads that slide the player to another part of
// the floor, switches in the floor that stay pressed, four elevators and a freight elevator, which the gym's scripts
// ride with the commands of overlay 36's scrcmd_gym.c. The name is a guess: the overlay has no string, and the only
// scripts that use its commands are the zone's (the lower floor, overlay 97, is gym_driftveil_lift.h)

// Overlay 36's gimmick table calls these
void DriftveilGym_Init(Field *field);
void DriftveilGym_End(Field *field);
void DriftveilGym_Update(Field *field);

// Overlay 36's tile events call this: the event that slides the player when he stands on a pad that faces the way he
// does, or NULL
GameEvent *DriftveilGym_CheckPadEvent(GameSystem *gsys);

// The gym's script commands
// The event that rides one of the four elevators, up or down
GameEvent *DriftveilGym_CreateElevatorEvent(GameSystem *gsys, u16 elevator, u16 down);
// The event that rides the freight elevator up or down, carrying the actor standing at (46, 18) if there is one
GameEvent *DriftveilGym_CreateFreightEvent(GameSystem *gsys, BOOL up, BOOL withRider);
// Puts the camera where the freight elevator's ride calls for
void DriftveilGym_PrepareFreight(GameSystem *gsys);

#endif // POKEBW2_FIELD_GYM_DRIFTVEIL_H
