#ifndef POKEBW2_FIELD_SCRCMD_JOIN_AVENUE_STORE_H
#define POKEBW2_FIELD_SCRCMD_JOIN_AVENUE_STORE_H

// Overlay 12's scrcmd_join_avenue_store.c (a guess): script commands from the end of overlay 12.
// s02C6_JoinAvenueStoreStart, s02C6_JoinAvenueStoreEnd and s02E3_LensFlareRequest are swan's names
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s02C6_JoinAvenueStoreStart(VM *vm, FieldScriptEnv *env);
BOOL s02C6_JoinAvenueStoreEnd(VM *vm, FieldScriptEnv *env);
BOOL s02CB_PokewoodGetLevel(VM *vm, FieldScriptEnv *env);
BOOL s02E3_LensFlareRequest(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_JOIN_AVENUE_STORE_H
