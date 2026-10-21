#ifndef POKEBW2_FIELD_GYM_VIRBANK_H
#define POKEBW2_FIELD_GYM_VIRBANK_H

#include "types.h"
#include "struct_decls.h"

// The Virbank City gym (zones 455 and 502), overlay 100: its three animated models and the music, whose tracks the
// event variables 0x4000 to 0x4003 unmute and whose volume follows where the player stands. The name is a guess: the
// overlay has no string

// Overlay 36's gimmick table calls these
void GymVirbank_Init(Field *field);
void GymVirbank_End(Field *field);
void GymVirbank_Update(Field *field);

// Overlay 36's gym script commands. Stops the animations of the model, paused
void GymVirbank_StopModelAnimations(GameSystem *gsys, int model);
// Sets a flag of the actor
void GymVirbank_SetActorFlag(GameSystem *gsys, int actorId);

#endif // POKEBW2_FIELD_GYM_VIRBANK_H
