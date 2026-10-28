#ifndef POKEBW2_FIELD_SCRCMD_MUSICAL_H
#define POKEBW2_FIELD_SCRCMD_MUSICAL_H

// Overlay 12's scrcmd_musical.c: the script commands of the musical. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Musical_MusicalCall(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalCallDressUp(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalGetInfo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalGetAudienceGift(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalAddProp(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_WordSetMusicalInfo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalClearAudienceGift(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalSelectPoke(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalComm(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalIsPropOwned(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_MusicalGetOwnedPropCount(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Musical_DreamWorldItems(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_MUSICAL_H
