#ifndef POKEBW2_SAVE_RESH_ZEK_H
#define POKEBW2_SAVE_RESH_ZEK_H

#include "types.h"
#include "pml/poke_party.h"
#include "struct_decls.h"
#include "system/game_data.h"

// Save block 0x2b, which swan calls ReshZek: the Reshiram or Zekrom fused with Kyurem, kept while they are one
struct ReshZekSave {
    // A copy of the game data's own CityState, Black City's or White Forest's
    CityState cityState;
    PartyPkm pkm;
}; // size: 0xe0

// The size of the block, and its init, which clears the city state (overlay 12's CityState_Init)
u32 ReshZekSave_GetSize(void);
void ReshZekSave_Init(ReshZekSave *reshZek);
ReshZekSave *getReshZekBlkAddress(SaveControl *save);
// Copies the city state in and out
void ReshZekSave_SetCityState(ReshZekSave *reshZek, const CityState *src);
void ReshZekSave_GetCityState(ReshZekSave *reshZek, CityState *dest);
// The fused Pokémon, and storing one
PartyPkm *ReshZekSave_GetPkm(ReshZekSave *reshZek);
void ReshZekSave_SetPkm(ReshZekSave *reshZek, const PartyPkm *pkm);

#endif // POKEBW2_SAVE_RESH_ZEK_H
