#ifndef POKEBW2_FIELD_SCRCMD_SP_POKE_GIMMICK_H
#define POKEBW2_FIELD_SCRCMD_SP_POKE_GIMMICK_H

// Overlay 12's scrcmd_sp_poke_gimmick.c (a guess after overlay 129's file): the script commands of the special Pokémon.
// s020D_PokePartyFindEx is swan's name (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL s020E_SpPokeJump(VM *vm, FieldScriptEnv *env);
BOOL s020F_SpPokeAppear(VM *vm, FieldScriptEnv *env);
BOOL s0210_SpPokeWaitAnimEnd(VM *vm, FieldScriptEnv *env);
BOOL s0211_SpPokeWaitFrame(VM *vm, FieldScriptEnv *env);
BOOL s020D_PokePartyFindEx(VM *vm, FieldScriptEnv *env);
BOOL s0215_SetSpecialTransferUsed(VM *vm, FieldScriptEnv *env);
BOOL func_ov012_02165950(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_SP_POKE_GIMMICK_H
