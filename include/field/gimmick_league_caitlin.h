#ifndef POKEBW2_FIELD_GIMMICK_LEAGUE_CAITLIN_H
#define POKEBW2_FIELD_GIMMICK_LEAGUE_CAITLIN_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of Caitlin's room in the Pokémon League, zone 143 (overlay 125): six models with animations, and an
// actor that drops into the room. The name is a guess: the ROM has no string for it. Its sounds use Caitlin's Japanese
// name, SEQ_SE_SW_CATTLEYA_*

// Overlay 36's gimmick table calls these
void GimmickCaitlin_Setup(Field *field);
void GimmickCaitlin_End(Field *field);
void GimmickCaitlin_Update(Field *field);

// The League's script plugin (overlay 52) calls these
// Plays the animations of models 3 and 4 and saves it, when which is 0, or those of model 5 otherwise
void GimmickCaitlin_PlayModels34Or5(GameSystem *gsys, u16 which);
// The event that waits for the animation of model 1 to finish, then plays its others
GameEvent *GimmickCaitlin_CreateModel1Event(GameSystem *gsys);
// Plays the animations of model 0
void GimmickCaitlin_CallPlayModel0(GameSystem *gsys);
// Shows the actor and drops it into the room by speed sixteenths of a unit a frame, until it reaches the player's
// height
void GimmickCaitlin_DropActor(GameSystem *gsys, u32 actorId, u16 speed);

#endif // POKEBW2_FIELD_GIMMICK_LEAGUE_CAITLIN_H
