#ifndef POKEBW2_FIELD_SCRCMD_POKEMON_MISC_H
#define POKEBW2_FIELD_SCRCMD_POKEMON_MISC_H

// Overlay 36's scrcmd_pokemon_misc.c (a descriptive name): script commands of the party's Pokémon, the Pokédex,
// in-game trades and the move tutors. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"

BOOL Scrcmd_PokemonMisc_CallPokeSelect(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_TrainerCardSetFavePokemon(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_TrainerCardSaveGymVictoryParty(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_WordSetGymVictoryParty(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_PokePartyGetIV(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_PokeDexGetEvaluationParams(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_PokeDexIsComplete(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_MoveTutorCheckParty(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_MoveTutorGetMoveID(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_MoveTutorCheckPkm(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_MoveTutorCallPokeSelect(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_PokePartyCheckPokerus(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_PokePartyGetHiddenPowerType(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_CallPokeNameInput(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_CallPokeMoveReplace(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_CallEggHatch(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_FieldTradeCheck(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_FieldTradeStart(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_FieldTradeGetSpecies(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_ItemGetRandomOwnedTMMove(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_FieldTradeSavePokemon(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_WordSetLoadPastTradePkmName(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_PokemonMisc_ControlLegendMon(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_POKEMON_MISC_H
