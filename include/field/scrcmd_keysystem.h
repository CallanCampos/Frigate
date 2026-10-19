#ifndef POKEBW2_FIELD_SCRCMD_KEYSYSTEM_H
#define POKEBW2_FIELD_SCRCMD_KEYSYSTEM_H

// Overlay 12's scrcmd_keysystem.c: the script commands of the Key System. s02AF_GameGetDifficulty and
// s02B0_CallUnovaLinkKeyUnlock are swan's names (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s02AE_GameGetMyCity(VM *vm, FieldScriptEnv *env);
BOOL s02AF_GameGetDifficulty(VM *vm, FieldScriptEnv *env);
BOOL s02B1_KeyInfoGetChamberKey(VM *vm, FieldScriptEnv *env);
BOOL s02B0_CallUnovaLinkKeyUnlock(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_KEYSYSTEM_H
