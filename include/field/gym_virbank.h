#ifndef POKEBW2_FIELD_GYM_VIRBANK_H
#define POKEBW2_FIELD_GYM_VIRBANK_H

#include "types.h"
#include "struct_decls.h"

// The Virbank City gym (zones 455 and 502), overlay 100: its three animated models and the music, whose tracks the
// event variables 0x4000 to 0x4003 unmute and whose volume follows where the player stands. The name is a guess: the
// overlay has no string

// Overlay 36's gimmick table calls these
void func_ov100_021eec80(Field *field);
void func_ov100_021eecd0(Field *field);
void func_ov100_021eecf8(Field *field);

// Overlay 36's gym script commands. Stops the animations of the model, paused
void func_ov100_021eed10(GameSystem *gsys, int model);
// Sets a flag of the actor
void func_ov100_021eed64(GameSystem *gsys, int actorId);

#endif // POKEBW2_FIELD_GYM_VIRBANK_H
