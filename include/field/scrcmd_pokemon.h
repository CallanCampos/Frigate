#ifndef POKEBW2_FIELD_SCRCMD_POKEMON_H
#define POKEBW2_FIELD_SCRCMD_POKEMON_H

// Overlay 12's scrcmd_pokemon.c: the script commands of the party's and the boxes' Pokémon, and of the money. Names
// from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL CheckGetPartyPokemon(FieldScriptEnv *env, u32 index, PartyPkm **pkm);
u32 GetScrPokeStat(FieldScriptEnv *env, u32 index, u32 param);
BOOL CheckPokeMoveLearned_NonEgg(PartyPkm *pkm, u16 move);
BOOL Scrcmd_Pokemon_PokePartyGetTypes(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyRecoverAll(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetEVTotal(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyIsFullHP(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyIsFullPP(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyIsEgg(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetHappiness(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyAdjustHappiness(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetSpecies(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetForme(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetMemberByType(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetCountBySpecies(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_BoxGetCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyFindBySpecies(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyHasMove(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyHasMoveAny(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyAdd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyAddEx(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyAddNPoke(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetMoveCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetMove(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyLearnMove(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyIsOriginGame(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartySetForme(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyChangeRotomForme(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_MoveReminderCheckPkm(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyIsFromWhiteForest(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetMetDate(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartySetIV(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyGetParam(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_BoxAdd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_BoxAddEx(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_PokePartyAddEgg(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_MoneyAdd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_MoneySub(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Pokemon_MoneyCheck(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_POKEMON_H
