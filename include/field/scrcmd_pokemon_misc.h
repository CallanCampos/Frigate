#ifndef POKEBW2_FIELD_SCRCMD_POKEMON_MISC_H
#define POKEBW2_FIELD_SCRCMD_POKEMON_MISC_H

// Overlay 36's scrcmd_pokemon_misc.c (a descriptive name): script commands of the party's Pokémon, the Pokédex,
// in-game trades and the move tutors. Names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"

BOOL s0107_CallPokeSelect(VM *vm, FieldScriptEnv *env);
BOOL s011D_TrainerCardSetFavePokemon(VM *vm, FieldScriptEnv *env);
BOOL s011E_TrainerCardSaveGymVictoryParty(VM *vm, FieldScriptEnv *env);
BOOL s011F_WordSetGymVictoryParty(VM *vm, FieldScriptEnv *env);
BOOL s0224_PokePartyGetIV(VM *vm, FieldScriptEnv *env);
BOOL s01C5_PokeDexGetEvaluationParams(VM *vm, FieldScriptEnv *env);
BOOL s01C4_PokeDexIsComplete(VM *vm, FieldScriptEnv *env);
BOOL s01D1_MoveTutorCheckParty(VM *vm, FieldScriptEnv *env);
BOOL s01D3_MoveTutorGetMoveID(VM *vm, FieldScriptEnv *env);
BOOL s01D2_MoveTutorCheckPkm(VM *vm, FieldScriptEnv *env);
BOOL s01D4_MoveTutorCallPokeSelect(VM *vm, FieldScriptEnv *env);
BOOL s0100_PokePartyCheckPokerus(VM *vm, FieldScriptEnv *env);
BOOL s0223_PokePartyGetHiddenPowerType(VM *vm, FieldScriptEnv *env);
BOOL s0105_CallPokeNameInput(VM *vm, FieldScriptEnv *env);
BOOL s0109_CallPokeMoveReplace(VM *vm, FieldScriptEnv *env);
BOOL s0106_CallEggHatch(VM *vm, FieldScriptEnv *env);
BOOL s01B5_FieldTradeCheck(VM *vm, FieldScriptEnv *env);
BOOL s01B4_FieldTradeStart(VM *vm, FieldScriptEnv *env);
BOOL s02C0_FieldTradeGetSpecies(VM *vm, FieldScriptEnv *env);
BOOL s0225_ItemGetRandomOwnedTMMove(VM *vm, FieldScriptEnv *env);
BOOL s0120_FieldTradeSavePokemon(VM *vm, FieldScriptEnv *env);
BOOL s022F_WordSetLoadPastTradePkmName(VM *vm, FieldScriptEnv *env);
BOOL func_ov036_021af9dc(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_POKEMON_MISC_H
