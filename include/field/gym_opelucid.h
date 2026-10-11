#ifndef POKEBW2_FIELD_GYM_OPELUCID_H
#define POKEBW2_FIELD_GYM_OPELUCID_H

#include "types.h"
#include "struct_decls.h"

// The Opelucid City gym, overlay 99: the two dragon statues that carry the player up to the Gym Leader, tier by tier,
// as the Veterans of each tier are beaten. The name is a guess: the overlay has no string, and the trainers whose
// flags the code checks are the Opelucid gym's Veterans

// Overlay 36's gimmick table calls these
void GymOpelucid_Init(Field *field);
void GymOpelucid_End(Field *field);
void GymOpelucid_Update(Field *field);

// The gym's script commands
// The event that moves the player and the statues in a direction, 0 to 3 (up, down, and to the two sides), or NULL
GameEvent *GymOpelucid_CreateMoveEvent(GameSystem *gsys, u16 direction);
// The event that presses one of the three buttons, which opens the way to the next tier
GameEvent *GymOpelucid_CreateButtonEvent(GameSystem *gsys, u16 button);
// Sets the event variables that the scripts read from the statues' state
void GymOpelucid_RefreshVars(GameSystem *gsys);
// Frees the gym's heap while a battle uses the memory, and creates it again after
void GymOpelucid_FreeHeap(GameSystem *gsys);
void GymOpelucid_CreateHeap(GameSystem *gsys);

#endif // POKEBW2_FIELD_GYM_OPELUCID_H
