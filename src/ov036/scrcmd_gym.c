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

// What Scrcmd_Gym_GymElecSetStage does: set the effects to one of three modes, show the first model or the player's,
// fade in or play stage object 5, or brighten the screen and reset it
enum {
    GYM_ELEC_STAGE_EFFECTS_MODE_0,
    GYM_ELEC_STAGE_EFFECTS_MODE_1,
    GYM_ELEC_STAGE_EFFECTS_MODE_2,
    GYM_ELEC_STAGE_SHOW_FIRST_MODEL,
    GYM_ELEC_STAGE_SHOW_PLAYER_MODEL,
    GYM_ELEC_STAGE_FADE_IN_OBJECT_5,
    GYM_ELEC_STAGE_PLAY_OBJECT_5,
    GYM_ELEC_STAGE_BRIGHTEN,
    GYM_ELEC_STAGE_RESET_BRIGHTNESS,
};

// How Scrcmd_Gym_DriftveilGymCallFreight rides the freight elevator
enum {
    FREIGHT_RIDE_DOWN,
    FREIGHT_RIDE_UP,
    FREIGHT_RIDE_UP_WITH_RIDER,
};

// How Scrcmd_Gym_DriftveilLiftCallMove rides the lift: the first lift up or down, or the second up with its rider
enum {
    LIFT_RIDE_UP,
    LIFT_RIDE_DOWN,
    LIFT_RIDE_UP_WITH_RIDER,
};

BOOL Scrcmd_Gym_GymElecReapplyProgress(VM *vm, FieldScriptEnv *env) {
    GymElec_ReapplyProgress(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    return FALSE;
}

BOOL Scrcmd_Gym_GymElecSetFollower(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));
    u16 attach = VM_Read16(vm);
    u16 actorId = VM_Read16(vm);
    u16 index = VM_Read16(vm);

    GymElec_SetFollower(field, attach, actorId, index);
    return FALSE;
}

BOOL Scrcmd_Gym_GymElecSetProgress(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u8 progress = VM_Read16(vm);
    GameEvent *event = GymElec_SetProgress(gsys, progress);

    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(FieldScriptEnv_GetScriptWork(env), event);
    return TRUE;
}

BOOL Scrcmd_Gym_GymElecSetStage(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));

    switch (VM_Read16(vm)) {
    case GYM_ELEC_STAGE_EFFECTS_MODE_0:
        GymElec_SetEffectsMode(field, 0);
        break;
    case GYM_ELEC_STAGE_EFFECTS_MODE_1:
        GymElec_SetEffectsMode(field, 1);
        break;
    case GYM_ELEC_STAGE_EFFECTS_MODE_2:
        GymElec_SetEffectsMode(field, 2);
        break;
    case GYM_ELEC_STAGE_SHOW_FIRST_MODEL:
        GymElec_ShowModel(field, FALSE);
        break;
    case GYM_ELEC_STAGE_SHOW_PLAYER_MODEL:
        GymElec_ShowModel(field, TRUE);
        break;
    case GYM_ELEC_STAGE_FADE_IN_OBJECT_5:
        GymElec_ShowStageObject5(field, FALSE);
        break;
    case GYM_ELEC_STAGE_PLAY_OBJECT_5:
        GymElec_ShowStageObject5(field, TRUE);
        break;
    case GYM_ELEC_STAGE_BRIGHTEN:
        GymElec_SetBrightness(field, 8);
        break;
    case GYM_ELEC_STAGE_RESET_BRIGHTNESS:
        GymElec_SetBrightness(field, 0);
        break;
    }
    return FALSE;
}

BOOL Scrcmd_Gym_GymInsectPlayObject(VM *vm, FieldScriptEnv *env) {
    Field *field = GSYS_GetField(FieldScriptEnv_GetGameSystem(env));

    GymInsect_PlayObject(field, ScriptReadAny(vm, env));
    return FALSE;
}

BOOL Scrcmd_Gym_GymInsectShowEffect(VM *vm, FieldScriptEnv *env) {
    GymInsect_ShowEffect(GSYS_GetField(FieldScriptEnv_GetGameSystem(env)));
    return FALSE;
}

BOOL Scrcmd_Gym_DriftveilGymCallElevator(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 elevator = VM_Read16(vm);
    u16 down = VM_Read16(vm);
    GameEvent *event = DriftveilGym_CreateElevatorEvent(gsys, elevator, down);

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_Gym_DriftveilGymCallFreight(VM *vm, FieldScriptEnv *env) {
    GameEvent *event;
    BOOL up = FALSE;
    BOOL withRider = FALSE;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    switch (VM_Read16(vm)) {
    case FREIGHT_RIDE_DOWN:
        break;
    case FREIGHT_RIDE_UP:
        up = TRUE;
        break;
    case FREIGHT_RIDE_UP_WITH_RIDER:
        up = TRUE;
        withRider = TRUE;
        break;
    }
    event = DriftveilGym_CreateFreightEvent(gsys, up, withRider);
    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_Gym_DriftveilLiftCallMove(VM *vm, FieldScriptEnv *env) {
    GameEvent *event;
    BOOL down;
    BOOL withRider = FALSE;
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u16 mode = VM_Read16(vm);

    down = mode != LIFT_RIDE_UP;
    switch (mode) {
    case LIFT_RIDE_UP:
        down = FALSE;
        break;
    case LIFT_RIDE_DOWN:
        down = TRUE;
        break;
    case LIFT_RIDE_UP_WITH_RIDER:
        down = FALSE;
        withRider = TRUE;
        break;
    }
    event = DriftveilLift_CreateMoveEvent(gsys, down, withRider);
    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_Gym_DriftveilGymPrepareFreight(VM *vm, FieldScriptEnv *env) {
    DriftveilGym_PrepareFreight(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

BOOL Scrcmd_Gym_DriftveilLiftPlaceAtBottom(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    DriftveilLift_PlaceAtBottom(gsys, VM_Read16(vm));
    return FALSE;
}

BOOL Scrcmd_Gym_MistraltonGymFireCannon(VM *vm, FieldScriptEnv *env) {
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

BOOL Scrcmd_Gym_Gym0601FanAmbienceStart(VM *vm, FieldScriptEnv *env) {
    FieldSnd_PlayAmbience(GameData_GetFieldSoundSystem(GSYS_GetGameData(FieldScriptEnv_GetGameSystem(env))),
                          SEQ_SE_GYM_H02);
    return FALSE;
}

BOOL Scrcmd_Gym_IcirrusGymCallSwitchAnim(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = GymIcirrus_CreateSwitchAnimEvent(gsys, VM_Read16(vm));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_Gym_IcirrusGymCallFlipSwitch(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = GymIcirrus_CreateFlipSwitchEvent(gsys, VM_Read16(vm));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_Gym_IcirrusGymToggleMovementAtPlace(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GymIcirrus_ToggleMovementAtPlace(gsys, VM_Read16(vm));
    return FALSE;
}

BOOL Scrcmd_Gym_OpelucidGymCallMove(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = GymOpelucid_CreateMoveEvent(gsys, VM_Read16(vm));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_Gym_OpelucidGymCallButton(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameEvent *event = GymOpelucid_CreateButtonEvent(gsys, VM_Read16(vm));

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_Gym_OpelucidGymRefreshVars(VM *vm, FieldScriptEnv *env) {
    GymOpelucid_RefreshVars(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

BOOL Scrcmd_Gym_OpelucidGymFreeHeap(VM *vm, FieldScriptEnv *env) {
    GymOpelucid_FreeHeap(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

BOOL Scrcmd_Gym_OpelucidGymCreateHeap(VM *vm, FieldScriptEnv *env) {
    GymOpelucid_CreateHeap(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}

BOOL Scrcmd_Gym_VirbankGymStopModelAnimations(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);

    GymVirbank_StopModelAnimations(gsys, VM_Read16(vm));
    return TRUE;
}

BOOL Scrcmd_Gym_VirbankGymSetActorFlag(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    GymVirbank_SetActorFlag(gsys, VM_Read16(vm));
    return TRUE;
}

BOOL Scrcmd_Gym_HumilauGymPlayLeaderSplash(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);

    GymHumilau_PlayLeaderSplash(gsys);
    return TRUE;
}

BOOL Scrcmd_Gym_HumilauGymPlayLeaderWaves(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GSYS_GetField(gsys);

    GymHumilau_PlayLeaderWaves(gsys);
    return TRUE;
}

BOOL Scrcmd_Gym_RollerCoasterCallSwitch(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    u8 switchId = VM_Read16(vm);
    GameEvent *event = RollerCoaster_CreateSwitchEvent(gsys, switchId);

    if (event == NULL) {
        return TRUE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL Scrcmd_Gym_RollerCoasterSetCarIntroSeen(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    u8 car = VM_Read16(vm);

    RollerCoaster_SetCarIntroSeen(gsys, car);
    return FALSE;
}

BOOL Scrcmd_Gym_RollerCoasterAmbienceStart(VM *vm, FieldScriptEnv *env) {
    RollerCoaster_StartAmbience(FieldScriptEnv_GetGameSystem(env));
    return FALSE;
}
