#ifndef POKEBW2_FIELD_ENCOUNTER_EFFECT_MODEL_H
#define POKEBW2_FIELD_ENCOUNTER_EFFECT_MODEL_H

#include "types.h"
#include "struct_decls.h"
#include "system/game_event.h"

// Overlay 149 runs the encounter effects that play an animated 3D model over a capture of the screen. The file name
// is descriptive, since the overlay embeds no name

// The create functions of the effects, by the model they play. white picks the fade to white that ends the effect
GameEvent *EncEffModel_CreateModel7(GameSystem *gsys, Field *field, BOOL white);
GameEvent *EncEffModel_CreateModel9(GameSystem *gsys, Field *field, BOOL white);
GameEvent *EncEffModel_CreateModel5(GameSystem *gsys, Field *field, BOOL white);
GameEvent *EncEffModel_CreateModel3(GameSystem *gsys, Field *field, BOOL white);
GameEvent *EncEffModel_CreateModel1(GameSystem *gsys, Field *field, BOOL white);
// Zoroark's, which also plays its sound effect
GameEvent *EncEffModel_CreateZoroark(GameSystem *gsys, Field *field, BOOL white);
GameEvent *EncEffModel_Create(GameSystem *gsys, Field *field, u32 modelId, u32 animId, BOOL white);
// The render function of the effects
void EncEffModel_Render(EncEff *effect);
GameEventReturnCode EncEffModel_EventCallback(GameEvent *event, u32 *state, void *data);

#endif // POKEBW2_FIELD_ENCOUNTER_EFFECT_MODEL_H
