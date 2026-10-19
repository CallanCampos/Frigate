#include "types.h"
#include "field/field.h"
#include "field/field_script.h"
#include "field/script_network.h"
#include "gfl/heap.h"
#include "system/dsi.h"
#include "system/game_comm.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

BOOL ScriptNative_WaitGameCommDisconnect(VM *vm, void *arg) {
    FieldScriptEnv *env = arg;
    GameSystem *gsys;
    ScriptWork *work;
    GameCommSys *commSys;
    void *data;

    gsys = FieldScriptEnv_GetGameSystem(env);
    work = FieldScriptEnv_GetScriptWork(env);
    commSys = GSYS_GetGameCommSystem(gsys);
    GSYS_GetField(gsys);
    data = *ScriptWork_GetUserHeapPtr(work);
    switch (func_ov036_02180fc0(commSys)) {
    case 0:
        return FALSE;
    case 1:
        GFL_HeapFree(data);
        return TRUE;
    case 2:
        **(u16 **)data = 2;
        GFL_HeapFree(data);
        return TRUE;
    }
    return FALSE;
}

BOOL s0139_GameCommDisconnect(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work;
    void **slot;
    u16 *result;
    void *memory;

    work = FieldScriptEnv_GetScriptWork(env);
    slot = ScriptWork_GetUserHeapPtr(work);
    result = ScriptReadVar(vm, env);
    memory = GFL_HeapAllocate(4, 4, TRUE, "scrcmd_network.c", 0x63);
    *slot = memory;
    *(u16 **)memory = result;
    *result = 0;
    VM_SetNativeCallback(vm, ScriptNative_WaitGameCommDisconnect);
    return TRUE;
}

BOOL s013B_GameCommCheckDSiWiFi(VM *vm, FieldScriptEnv *env) {
    u16 *result = ScriptReadVar(vm, env);
    *result = isWirelessEnabled();
    return FALSE;
}

BOOL s013C_GameCommTryBoot(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;

    gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_TryBootGameComm(gsys);
    return TRUE;
}

BOOL s013D_GameCommPause(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    GameCommSys *commSys;

    gsys = FieldScriptEnv_GetGameSystem(env);
    commSys = GSYS_GetGameCommSystem(gsys);
    switch (GameCommSys_BootCheck(commSys)) {
    case 0:
    case 3:
    case 4:
        break;
    case 1:
    case 2:
    case 5:
        GameCommSys_ExitReq(commSys);
        break;
    }
    func_02016b24(gsys, 1);
    FieldScriptSubEvent_Register(11);
    return TRUE;
}

BOOL s013E_GameCommResume(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;

    gsys = FieldScriptEnv_GetGameSystem(env);
    func_02016b24(gsys, 0);
    GSYS_TryBootGameComm(gsys);
    FieldScriptSubEvent_Unregister(11);
    return TRUE;
}

BOOL GameCommPause_Finish(FinishScriptSubEventsWork *work, u32 *state) {
    GameSystem *gsys;

    gsys = work->gsys;
    func_02016b24(gsys, 0);
    GSYS_TryBootGameComm(gsys);
    return TRUE;
}

BOOL s024A_FieldSubscreenIsEnabled(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys;
    u16 *value;

    gsys = FieldScriptEnv_GetGameSystem(env);
    value = ScriptReadVar(vm, env);
    *value = GSYS_IsCGearOn(gsys);
    return FALSE;
}

BOOL s024B_FieldSubscreenDisable(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_SetCGearOn(gsys, 0);
    GameData_SetCGearPowerRequest(GSYS_GetGameData(gsys), 0);
    return FALSE;
}
