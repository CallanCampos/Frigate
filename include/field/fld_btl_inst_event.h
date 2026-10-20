#ifndef POKEBW2_FIELD_FLD_BTL_INST_EVENT_H
#define POKEBW2_FIELD_FLD_BTL_INST_EVENT_H

// Overlay 12's events of the battle facilities (fld_btl_inst_event.c, a guess after fld_btl_inst_tool.c beside it):
// picking the Pokémon to enter, and a Trainer's message in a balloon

#include "types.h"
#include "field/bsubway_scr.h"
#include "struct_decls.h"

// The party screen for picking the Pokémon to enter, under the regulation of the file. The picked slots go to
// picked, the screen's choice and result to choice and result, and the Pokémon picked are added to entered
GameEvent *EventBtlInstPokeSelect_Create(GameSystem *gsys, u32 a1, u32 mode, u32 regulationId, PokeParty *party,
                                         u8 *picked, u32 *choice, u32 *result, PokeParty *entered);
// The message of trainers[0], or of trainers[index] when it has a phrase, in a balloon over the actor
GameEvent *EventBtlInstTrainerMsg_Create(GameSystem *gsys, BSubwayTrainer *trainers, u32 index, u16 actorId);

#endif // POKEBW2_FIELD_FLD_BTL_INST_EVENT_H
