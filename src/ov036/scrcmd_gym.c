// The script commands of the gyms' puzzles, which call into each gym's gimmick overlay. The ROM has no name for the
// file; scrcmd_gym.c is descriptive. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0),
// except the commands swan has no name for, which are ours, after the gimmick functions they call.
#include "types.h"
#include "constants/sound.h"
#include "field/field_script.h"
#include "field/field_sound.h"
#include "field/gimmick_roller_coaster.h"
#include "field/gym_driftveil.h"
#include "field/gym_driftveil_lift.h"
#include "field/gym_gimmick.h"
#include "field/gym_humilau.h"
#include "field/gym_icirrus.h"
#include "field/gym_mistralton.h"
#include "field/gym_opelucid.h"
#include "field/gym_virbank.h"
#include "field/scrcmd_gym.h"
#include "system/game_data.h"
#include "system/game_system.h"
#include "system/vm.h"

BOOL s025B_GymElecReapplyProgress(VM *vm, FieldScriptEnv *env) {
    GymElec_ReapplyProgress(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    return FALSE;
}

BOOL s025C_GymElecSetFollower(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    u16 a1 = VM_Read16(vm);
    u16 a2 = VM_Read16(vm);
    u16 a3 = VM_Read16(vm);

    GymElec_SetFollower(field, a1, a2, a3);
    return FALSE;
}

BOOL s025D_GymElecSetProgress(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u8 progress = VM_Read16(vm);
    GameEvent *event = GymElec_SetProgress(gsys, progress);

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(FieldScriptEnv_GetScriptWork(env), event);
    return TRUE;
}

BOOL s025F_GymElecSetStage(VM *vm, FieldScriptEnv *env) {
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

BOOL s0268_GymInsectPlayObject(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));

    GymInsect_PlayObject(field, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL s0269_GymInsectShowEffect(VM *vm, FieldScriptEnv *env) {
    GymInsect_ShowEffect(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    return FALSE;
}

BOOL ScrcmdFieldGimmickPlateDoors_StartDoorEvent(VM *vm, FieldScriptEnv *env) {
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

BOOL s018D_DriftveilGymCallFreight(VM *vm, FieldScriptEnv *env) {
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

BOOL s018E_DriftveilLiftCallMove(VM *vm, FieldScriptEnv *env) {
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
    event = DriftveilLift_CreateMoveEvent(gsys, a1, a2);
    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL s0186_DriftveilGymPrepareFreight(VM *vm, FieldScriptEnv *env) {
    DriftveilGym_PrepareFreight(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

BOOL s0187_DriftveilLiftPlaceAtBottom(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    DriftveilLift_PlaceAtBottom(gsys, VM_Read16(vm));
    return FALSE;
}

BOOL s028E_MistraltonGymFireCannon(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);
    GameEvent *event = GymMistralton_StartCannon(gsys, VM_Read16(vm));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL s02BB_Gym0601FanAmbienceStart(VM *vm, FieldScriptEnv *env) {
    FieldSnd_PlayAmbience(GameData_GetFieldSoundSystem(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))),
                          SEQ_SE_GYM_H02);
    return FALSE;
}

BOOL s018F_IcirrusGymCallSwitchAnim(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = GymIcirrus_CreateSwitchAnimEvent(gsys, VM_Read16(vm));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL s0190_IcirrusGymCallFlipSwitch(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = GymIcirrus_CreateFlipSwitchEvent(gsys, VM_Read16(vm));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL s0191_IcirrusGymToggleMovementAtPlace(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GymIcirrus_ToggleMovementAtPlace(gsys, VM_Read16(vm));
    return FALSE;
}

BOOL s02A5_OpelucidGymCallMove(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = GymOpelucid_CreateMoveEvent(gsys, VM_Read16(vm));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL s02A7_OpelucidGymCallButton(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = GymOpelucid_CreateButtonEvent(gsys, VM_Read16(vm));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL s02A6_OpelucidGymRefreshVars(VM *vm, FieldScriptEnv *env) {
    GymOpelucid_RefreshVars(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

BOOL s02A8_OpelucidGymFreeHeap(VM *vm, FieldScriptEnv *env) {
    GymOpelucid_FreeHeap(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

BOOL s02A9_OpelucidGymCreateHeap(VM *vm, FieldScriptEnv *env) {
    GymOpelucid_CreateHeap(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

BOOL s0291_VirbankGymStopModelAnimations(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);

    GymVirbank_StopModelAnimations(gsys, VM_Read16(vm));
    return TRUE;
}

BOOL s0292_VirbankGymSetActorFlag(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GymVirbank_SetActorFlag(gsys, VM_Read16(vm));
    return TRUE;
}

BOOL s02A2_HumilauGymPlayLeaderSplash(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);

    GymHumilau_PlayLeaderSplash(gsys);
    return TRUE;
}

BOOL s02A3_HumilauGymPlayLeaderWaves(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);

    GymHumilau_PlayLeaderWaves(gsys);
    return TRUE;
}

BOOL s017C_RollerCoasterCallSwitch(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u8 a1 = VM_Read16(vm);
    GameEvent *event = RollerCoaster_CreateSwitchEvent(gsys, a1);

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL ScrcmdFieldGimmickCartLanes_UnlockLaneFromScript(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    u8 a1 = VM_Read16(vm);

    RollerCoaster_SetCarIntroSeen(gsys, a1);
    return FALSE;
}

BOOL s024D_RollerCoasterAmbienceStart(VM *vm, FieldScriptEnv *env) {
    RollerCoaster_StartAmbience(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}
