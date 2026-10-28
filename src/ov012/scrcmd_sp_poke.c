// Script commands 0x23B to 0x23D, which drive overlay 133's gimmick (gimmick_nuvema.c). The name is a guess
#include "types.h"
#include "field/field.h"
#include "field/field_map.h"
#include "field/field_script.h"
#include "field/gimmick_nuvema.h"
#include "field/scrcmd_sp_poke.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

static void func_ov012_02169c64(GameSystem *gsys);

BOOL func_ov012_02169c1c(VM *vm, FieldScriptEnv *env) {
    func_ov012_02169c64(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

BOOL Scrcmd_SpPoke_NuvemaGimmickPlay(VM *vm, FieldScriptEnv *env) {
    GimmickNuvema_PlayModel(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    return FALSE;
}

BOOL Scrcmd_SpPoke_NuvemaGimmickWaitAnim(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);

    ScriptWork_CallEvent(work, GimmickNuvema_CreateWaitEvent(FieldScriptEnv_GetGameSystem(env)));
    return TRUE;
}

static void func_ov012_02169c64(GameSystem *gsys) {
    u32 *state = GimmickState_GetUserData(GameData_GetGimmickState(GSYS_GetGameData(gsys)), GIMMICK_NUVEMA);

    *state = TRUE;
}
