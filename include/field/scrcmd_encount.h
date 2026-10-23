#ifndef POKEBW2_FIELD_SCRCMD_ENCOUNT_H
#define POKEBW2_FIELD_SCRCMD_ENCOUNT_H

// Overlay 36's scrcmd_encount.c: the script commands of wild encounters. Names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s0174_CallWildBattle(VM *vm, FieldScriptEnv *env);
BOOL s0297_CallWildBattleEx(VM *vm, FieldScriptEnv *env);
BOOL s0175_CallWildBattleEnd(VM *vm, FieldScriptEnv *env);
BOOL s0179_CallCaptureDemo(VM *vm, FieldScriptEnv *env);
BOOL s00BC_PhenomenonGetItemID(VM *vm, FieldScriptEnv *env);
BOOL ScrcmdProcFld_SpawnMovePokemonIfInactive(VM *vm, FieldScriptEnv *env);
BOOL s021E_FishingChallengeGetRandomPkm(VM *vm, FieldScriptEnv *env);
BOOL s02C2_RepelRearm(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_ENCOUNT_H
