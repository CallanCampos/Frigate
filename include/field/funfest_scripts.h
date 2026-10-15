#ifndef POKEBW2_FIELD_FUNFEST_SCRIPTS_H
#define POKEBW2_FIELD_FUNFEST_SCRIPTS_H

// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"

BOOL s0279_FunfestBGMReturn(VM *vm, FieldScriptEnv *env);
BOOL s0274_FunfestMissionStart(VM *vm, FieldScriptEnv *env);
BOOL s0276_FunfestMissionBroadcast(VM *vm, FieldScriptEnv *env);
BOOL s0277_FunfestActorDelete(VM *vm, FieldScriptEnv *env);
BOOL s027A_FunfestGetGenericInfo(VM *vm, FieldScriptEnv *env);
BOOL s027B_FunfestGetItemExchangeInfo(VM *vm, FieldScriptEnv *env);
BOOL s027C_FunfestGetItemSaleInfo(VM *vm, FieldScriptEnv *env);
BOOL s027D_FunfestGetPokemonQuizInfo(VM *vm, FieldScriptEnv *env);
BOOL s027E_FunfestGetPokemonQuizSpecies(VM *vm, FieldScriptEnv *env);
BOOL s027F_FunfestGetPokemonQuizBogusSpecies(VM *vm, FieldScriptEnv *env);
BOOL func_ov033_021772c8(VM *vm, FieldScriptEnv *env);
BOOL func_ov033_021772f4(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_FUNFEST_SCRIPTS_H
