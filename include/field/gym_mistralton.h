#ifndef POKEBW2_FIELD_GYM_MISTRALTON_H
#define POKEBW2_FIELD_GYM_MISTRALTON_H

#include "types.h"
#include "struct_decls.h"

// gym_mistralton.c, overlay 94: the gimmick of Mistralton City's gym, Skyla's cannons and fans. The ROM doesn't name
// the file; the name is a guess, from the gym 0601 fan ambience that the script command in scrcmd_gym.c starts and
// that this overlay stops, and from the cannons. Overlay 36's gimmick table calls the first three
void GymMistralton_Init(Field *field);
void GymMistralton_End(Field *field);
void GymMistralton_Update(Field *field);
// Called by overlay 36's grid event check after each step; returns the event that the step starts, or NULL
GameEvent *GymMistralton_CheckStep(GameSystem *gsys);
// The script command that fires the cannon at the actor with this ID
GameEvent *GymMistralton_StartCannon(GameSystem *gsys, u16 actorId);

#endif // POKEBW2_FIELD_GYM_MISTRALTON_H
