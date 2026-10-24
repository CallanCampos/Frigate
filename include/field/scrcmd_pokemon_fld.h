#ifndef POKEBW2_FIELD_SCRCMD_POKEMON_FLD_H
#define POKEBW2_FIELD_SCRCMD_POKEMON_FLD_H

// Overlay 36's scrcmd_pokemon_fld.c: the party's Reshiram (Black 2) or Zekrom (White 2), for the script command
// of scrcmd_pokemon_misc.c that swaps it in. GetOTZekromPartyIdx is swan's name
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"

// Swaps the party Pokémon at partyIdx with the player's own dragon of the version from a box
void ScrcmdPokemonFld_SwapPartyMonWithBoxedLegend(FieldScriptEnv *env, u32 partyIdx);
// Moves the player's own dragon of the version to the front of the party
void ScrcmdPokemonFld_MoveLegendToFront(FieldScriptEnv *env);
// The party index of the player's own dragon of the version, or -1
int GetOTZekromPartyIdx(GameData *gameData);

#endif // POKEBW2_FIELD_SCRCMD_POKEMON_FLD_H
