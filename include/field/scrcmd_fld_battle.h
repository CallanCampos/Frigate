#ifndef POKEBW2_FIELD_SCRCMD_FLD_BATTLE_H
#define POKEBW2_FIELD_SCRCMD_FLD_BATTLE_H

// Overlay 36's scrcmd_fld_battle.c: the script commands that check teams against a regulation

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_FldBattle_RegisterAllowedSpecies(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_FldBattle_CheckPartyRegulation(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_FLD_BATTLE_H
