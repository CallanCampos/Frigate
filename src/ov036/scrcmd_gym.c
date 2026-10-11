// The script commands of the gyms' puzzles, which call into each gym's gimmick overlay. The ROM has no name for the
// file; scrcmd_gym.c is descriptive. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field_script.h"
#include "field/field_sound.h"
#include "field/gym_driftveil.h"
#include "field/gym_driftveil_lift.h"
#include "field/gym_gimmick.h"
#include "field/gym_icirrus.h"
#include "field/gym_mistralton.h"
#include "field/gym_opelucid.h"
#include "field/gym_virbank.h"
#include "field/scrcmd_gym.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

BOOL func_ov036_021aad44(VM *vm, FieldScriptEnv *env) {
    GymElec_ReapplyProgress(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    return FALSE;
}

BOOL func_ov036_021aad58(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    u16 a1 = VM_Read16(vm);
    u16 a2 = VM_Read16(vm);
    u16 a3 = VM_Read16(vm);

    GymElec_SetFollower(field, a1, a2, a3);
    return FALSE;
}

BOOL func_ov036_021aad90(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u8 progress = VM_Read16(vm);
    GameEvent *event = GymElec_SetProgress(gsys, progress);

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(FieldScriptEnv_GetScriptWork(env), event);
    return TRUE;
}

BOOL func_ov036_021aadc8(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));

    switch (VM_Read16(vm)) {
    case 0:
        GymElec_SetEffectsMode(field, 0);
        break;
    case 1:
        GymElec_SetEffectsMode(field, 1);
        break;
    case 2:
        GymElec_SetEffectsMode(field, 2);
        break;
    case 3:
        GymElec_ShowModel(field, FALSE);
        break;
    case 4:
        GymElec_ShowModel(field, TRUE);
        break;
    case 5:
        GymElec_ShowStageObject5(field, FALSE);
        break;
    case 6:
        GymElec_ShowStageObject5(field, TRUE);
        break;
    case 7:
        GymElec_SetBrightness(field, 8);
        break;
    case 8:
        GymElec_SetBrightness(field, 0);
        break;
    }
    return FALSE;
}

BOOL func_ov036_021aae48(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));

    GymInsect_PlayObject(field, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL func_ov036_021aae74(VM *vm, FieldScriptEnv *env) {
    GymInsect_ShowEffect(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    return FALSE;
}

BOOL func_ov036_021aae88(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 a1 = VM_Read16(vm);
    u16 a2 = VM_Read16(vm);
    GameEvent *event = DriftveilGym_CreateElevatorEvent(gsys, a1, a2);

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL func_ov036_021aaec8(VM *vm, FieldScriptEnv *env) {
    GameEvent *event;
    BOOL a1 = FALSE;
    BOOL a2 = FALSE;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    switch (VM_Read16(vm)) {
    case 0:
        break;
    case 1:
        a1 = TRUE;
        break;
    case 2:
        a1 = TRUE;
        a2 = TRUE;
        break;
    }
    event = DriftveilGym_CreateFreightEvent(gsys, a1, a2);
    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL func_ov036_021aaf1c(VM *vm, FieldScriptEnv *env) {
    GameEvent *event;
    BOOL a1;
    BOOL a2 = FALSE;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 mode = VM_Read16(vm);

    a1 = mode != 0;
    switch (mode) {
    case 0:
        a1 = FALSE;
        break;
    case 1:
        a1 = TRUE;
        break;
    case 2:
        a1 = FALSE;
        a2 = TRUE;
        break;
    }
    event = func_ov097_021eede4(gsys, a1, a2);
    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL func_ov036_021aaf78(VM *vm, FieldScriptEnv *env) {
    DriftveilGym_PrepareFreight(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

BOOL func_ov036_021aaf88(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    func_ov097_021eefe4(gsys, VM_Read16(vm));
    return FALSE;
}

BOOL func_ov036_021aafa8(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);
    GameEvent *event = GymMistralton_StartCannon(gsys, VM_Read16(vm));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL s02BB_Gym0601FanAmbienceStart(VM *vm, FieldScriptEnv *env) {
    FieldSnd_PlayAmbience(GameData_GetFieldSoundSystem(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))), 0x819);
    return FALSE;
}

BOOL func_ov036_021ab004(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = func_ov098_021eee0c(gsys, VM_Read16(vm));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL func_ov036_021ab03c(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = func_ov098_021eee48(gsys, VM_Read16(vm));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL func_ov036_021ab074(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    func_ov098_021eee84(gsys, VM_Read16(vm));
    return FALSE;
}

BOOL func_ov036_021ab094(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = GymOpelucid_CreateMoveEvent(gsys, VM_Read16(vm));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL func_ov036_021ab0cc(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = GymOpelucid_CreateButtonEvent(gsys, VM_Read16(vm));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL func_ov036_021ab104(VM *vm, FieldScriptEnv *env) {
    GymOpelucid_RefreshVars(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

BOOL func_ov036_021ab114(VM *vm, FieldScriptEnv *env) {
    GymOpelucid_FreeHeap(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

BOOL func_ov036_021ab124(VM *vm, FieldScriptEnv *env) {
    GymOpelucid_CreateHeap(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

BOOL func_ov036_021ab134(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);

    func_ov100_021eed10(gsys, VM_Read16(vm));
    return TRUE;
}

BOOL func_ov036_021ab158(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    func_ov100_021eed64(gsys, VM_Read16(vm));
    return TRUE;
}

BOOL func_ov036_021ab178(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);

    func_ov101_021eee48(gsys);
    return TRUE;
}

BOOL func_ov036_021ab190(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    Field *field = GSYS_GetField(gsys);

    func_ov101_021eeee0(gsys);
    return TRUE;
}

BOOL func_ov036_021ab1a8(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u8 a1 = VM_Read16(vm);
    GameEvent *event = func_ov102_021ef3e8(gsys, a1);

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL func_ov036_021ab1e0(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    u8 a1 = VM_Read16(vm);

    func_ov102_021ef6f8(gsys, a1);
    return FALSE;
}

BOOL func_ov036_021ab208(VM *vm, FieldScriptEnv *env) {
    func_ov102_021ef338(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}
