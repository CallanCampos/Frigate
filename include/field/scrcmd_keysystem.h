#ifndef POKEBW2_FIELD_SCRCMD_KEYSYSTEM_H
#define POKEBW2_FIELD_SCRCMD_KEYSYSTEM_H

// Overlay 12's scrcmd_keysystem.c: the script commands of the Key System. Scrcmd_Keysystem_GameGetDifficulty and
// Scrcmd_Keysystem_CallUnovaLinkKeyUnlock are swan's names (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Keysystem_GameGetMyCity(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Keysystem_GameGetDifficulty(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Keysystem_KeyInfoGetChamberKey(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Keysystem_CallUnovaLinkKeyUnlock(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_KEYSYSTEM_H
