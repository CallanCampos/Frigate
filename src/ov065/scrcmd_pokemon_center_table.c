#include "types.h"
#include "field/field_script.h"
#include "field/scrcmd_pokemon_center.h"

// The command table of the Pokémon Centers' script plugin. The overlay has it ahead of the rest of its rodata, which
// MWCC lays out by size within a file, so it is in a file of its own, linked before the commands.

const FieldScriptCommand POKEMON_CENTER_SCRIPT_COMMANDS[] = {
    Scrcmd_PokemonCenter_Medal,
    Scrcmd_PokemonCenter_FindEarnedMedal,
    NULL,
    NULL,
    NULL,
    Scrcmd_PokemonCenter_GetMatchState,
    Scrcmd_PokemonCenter_SetMatchInProgress,
    Scrcmd_PokemonCenter_ClearMatchInProgress,
    Scrcmd_PokemonCenter_SetMatchState,
    Scrcmd_PokemonCenter_AddBattlePoints,
    NULL,
    NULL,
    NULL,
    (FieldScriptCommand)0xFFFFFFFF,
};
