#ifndef POKEBW2_FIELD_SCRCMD_MUSICAL_H
#define POKEBW2_FIELD_SCRCMD_MUSICAL_H

// Overlay 12's scrcmd_musical.c: the script commands of the musical. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s0163_MusicalCall(VM *vm, FieldScriptEnv *env);
BOOL s0164_MusicalCallDressUp(VM *vm, FieldScriptEnv *env);
BOOL s0165_MusicalGetInfo(VM *vm, FieldScriptEnv *env);
BOOL s0166_MusicalGetAudienceGift(VM *vm, FieldScriptEnv *env);
BOOL s0168_MusicalAddProp(VM *vm, FieldScriptEnv *env);
BOOL s005D_WordSetMusicalInfo(VM *vm, FieldScriptEnv *env);
BOOL s0169_MusicalClearAudienceGift(VM *vm, FieldScriptEnv *env);
BOOL s016A_MusicalSelectPoke(VM *vm, FieldScriptEnv *env);
BOOL s0167_MusicalComm(VM *vm, FieldScriptEnv *env);
BOOL s02EE_MusicalIsPropOwned(VM *vm, FieldScriptEnv *env);
BOOL s02EF_MusicalGetOwnedPropCount(VM *vm, FieldScriptEnv *env);
BOOL s01DC_DreamWorldItems(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_MUSICAL_H
