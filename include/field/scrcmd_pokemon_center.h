#ifndef POKEBW2_FIELD_SCRCMD_POKEMON_CENTER_H
#define POKEBW2_FIELD_SCRCMD_POKEMON_CENTER_H

#include "types.h"
#include "struct_decls.h"

// The commands of the Pokémon Centers' script plugin (plugin 13), which POKEMON_CENTER_SCRIPT_COMMANDS lists

BOOL Scrcmd_PokemonCenter_Medal(VM *vm, FieldScriptEnv *env);
// Sets a variable to whether a medal has been earned but not handed over, and another to the medal
BOOL Scrcmd_PokemonCenter_FindEarnedMedal(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonCenter_GetMatchState(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonCenter_SetMatchInProgress(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonCenter_ClearMatchInProgress(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonCenter_SetMatchState(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonCenter_AddBattlePoints(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_POKEMON_CENTER_H
