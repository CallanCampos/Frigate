#ifndef POKEBW2_FIELD_GIMMICK_LEAGUE_CHAMPION_H
#define POKEBW2_FIELD_GIMMICK_LEAGUE_CHAMPION_H

#include "types.h"
#include "struct_decls.h"

// The gimmick of the Champion's room in the Pokémon League, zone 144 (overlay 120): the models that change when the
// challenger is welcomed, and the shake of the camera. The name is a guess: the ROM has no string for it. Its sounds
// use the Champion's Japanese name, SEQ_SE_SW_CHAMPIRIS_*

// Overlay 36's gimmick table calls these
void GimmickChampion_Setup(Field *field);
void GimmickChampion_End(Field *field);
void GimmickChampion_Update(Field *field);

// The League's script plugin (overlay 52) calls these
// The event that changes the room: model 4 plays, then the models change
GameEvent *GimmickChampion_CallChangeEvent(GameSystem *gsys);
// The event that plays the animations of models 1 and 3, and shakes the camera
GameEvent *GimmickChampion_CallOpenEvent(GameSystem *gsys);
// Takes the shadow off the actor
void GimmickChampion_HideActorShadow(GameSystem *gsys, u32 actorId);
// Gives the actor its shadow back
void GimmickChampion_ShowActorShadow(GameSystem *gsys, u32 actorId);
// Saves that the room is open, so that it is shown that way after a reload
void GimmickChampion_CallSetOpen(GameSystem *gsys);

#endif // POKEBW2_FIELD_GIMMICK_LEAGUE_CHAMPION_H
