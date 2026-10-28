#ifndef POKEBW2_FIELD_SCRCMD_SP_POKE_GIMMICK_H
#define POKEBW2_FIELD_SCRCMD_SP_POKE_GIMMICK_H

// Overlay 12's scrcmd_sp_poke_gimmick.c (a guess after overlay 129's file): the script commands of the special Pokémon.
// Scrcmd_SpPokeGimmick_PokePartyFindEx is swan's name (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL Scrcmd_SpPokeGimmick_SpPokeJump(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_SpPokeGimmick_SpPokeAppear(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_SpPokeGimmick_SpPokeWaitAnimEnd(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_SpPokeGimmick_SpPokeWaitFrame(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_SpPokeGimmick_PokePartyFindEx(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_SpPokeGimmick_SetSpecialTransferUsed(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_SpPokeGimmick_ReadSaveByte(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_SP_POKE_GIMMICK_H
