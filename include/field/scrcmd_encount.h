#ifndef POKEBW2_FIELD_SCRCMD_ENCOUNT_H
#define POKEBW2_FIELD_SCRCMD_ENCOUNT_H

// Overlay 36's scrcmd_encount.c: the script commands of wild encounters. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_Encount_CallWildBattle(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Encount_CallWildBattleEx(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Encount_CallWildBattleEnd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Encount_CallCaptureDemo(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Encount_PhenomenonGetItemID(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Encount_SpawnMovePokemonIfInactive(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Encount_FishingChallengeGetRandomPkm(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_Encount_RepelRearm(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_ENCOUNT_H
