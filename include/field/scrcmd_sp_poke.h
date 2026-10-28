#ifndef POKEBW2_FIELD_SCRCMD_SP_POKE_H
#define POKEBW2_FIELD_SCRCMD_SP_POKE_H

// Overlay 12's scrcmd_sp_poke.c (a guess): script commands 0x23B to 0x23D, which drive overlay 133's gimmick

#include "types.h"
#include "struct_decls.h"
#include "system/vm.h"

BOOL func_ov012_02169c1c(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_SpPoke_NuvemaGimmickPlay(VM *vm, FieldScriptEnv *env);
BOOL Scrcmd_SpPoke_NuvemaGimmickWaitAnim(VM *vm, FieldScriptEnv *env);

#endif // POKEBW2_FIELD_SCRCMD_SP_POKE_H
