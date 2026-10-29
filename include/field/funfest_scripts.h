#ifndef POKEBW2_FIELD_FUNFEST_SCRIPTS_H
#define POKEBW2_FIELD_FUNFEST_SCRIPTS_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"

BOOL Scrcmd_Funfest_FunfestBGMReturn(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Funfest_FunfestMissionStart(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Funfest_FunfestMissionBroadcast(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Funfest_FunfestActorDelete(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Funfest_FunfestGetGenericInfo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Funfest_FunfestGetItemExchangeInfo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Funfest_FunfestGetItemSaleInfo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Funfest_FunfestGetPokemonQuizInfo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Funfest_FunfestGetPokemonQuizSpecies(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Funfest_FunfestGetPokemonQuizBogusSpecies(VM *vm, FieldScriptEnv *env);
BOOL func_ov033_021772c8(VM *vm, FieldScriptEnv *env);
BOOL func_ov033_021772f4(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_FUNFEST_SCRIPTS_H
