#include "types.h"
#include "field/field_script.h"
#include "field/scrcmd_pokemon_center.h"

// The command table of the Pokémon Centers' script plugin. The overlay has it ahead of the rest of its rodata, which
// MWCC lays out by size within a file, so it is in a file of its own, linked before the commands.

const FieldScriptCommand POKEMON_CENTER_SCRIPT_COMMANDS[] = {
    PokemonCenterCmd_Medal,
    PokemonCenterCmd_FindEarnedMedal,
    NULL,
    NULL,
    NULL,
    PokemonCenterCmd_GetMatchState,
    PokemonCenterCmd_SetMatchInProgress,
    PokemonCenterCmd_ClearMatchInProgress,
    func_ov065_021e65dc,
    PokemonCenterCmd_AddBattlePoints,
    NULL,
    NULL,
    NULL,
    (FieldScriptCommand)0xFFFFFFFF,
};
