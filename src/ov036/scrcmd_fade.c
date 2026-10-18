// The script commands that fade the screens: to and from black or white, quickly or slowly, and their waits. The ROM
// has no name for the file; scrcmd_fade.c is descriptive. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
#include "types.h"
#include "field/field.h"
#include "field/field_event.h"
#include "field/field_script.h"
#include "field/scrcmd_fade.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "nitro/hw.h"
#include "system/brightness.h"
#include "system/game_data.h"
#include "system/game_event.h"
#include "system/game_system.h"
#include "system/vm.h"

enum {
    FIELD_FADE_IN_SLOW,
    FIELD_FADE_IN,
    FIELD_FADE_OUT_SLOW,
    FIELD_FADE_OUT,
};

typedef struct {
    Field *field;
    u32 mode;
    BOOL white;
} FieldFadeEvent;

static BOOL letBrightnessFinishAdjusting(VM *vm, void *env);
static BOOL func_ov036_021ab25c(VM *vm, void *env);
static GameEventReturnCode func_ov036_021ab580(GameEvent *event, u32 *state, void *data);

BOOL s00B3_FadeEx(VM *vm, FieldScriptEnv *env) {
    u16 mode = VM_Read16(vm);
    u16 start = VM_Read16(vm);
    u16 end = VM_Read16(vm);
    u16 slowness = VM_Read16(vm);

    GFL_FadeSet(mode, start, end, slowness);
    return FALSE;
}

static BOOL letBrightnessFinishAdjusting(VM *vm, void *env) {
    if (GFL_FadeIsRunning() == TRUE) {
        return FALSE;
    }
    return TRUE;
}

static BOOL func_ov036_021ab25c(VM *vm, void *env) {
    if (Field_GetFadeFlag(GSYS_GetField(FieldScriptEnv_GetGameSystem(env))) == TRUE) {
        return FALSE;
    }
    return TRUE;
}

BOOL s01AA_CallSeasonBanner(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    u32 season = GameData_GetSeason(gameData);

    ScriptWork_CallEvent(work, CallFieldMapEntranceInTransition(gsys, field, 3, 0, 0, season, season));
    return FALSE;
}

BOOL s01A8_FadeInBlackQ_(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 0, 0, 0);
    return FALSE;
}

BOOL s01A3_FadeInBlackQ(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 0, 0, 0);
    return FALSE;
}

BOOL s01A4_FadeOutBlackQ(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 1, 0, 0);
    return FALSE;
}

BOOL s01A5_FadeInWhiteQ(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 0, 1, 0);
    return FALSE;
}

BOOL s01A6_FadeOutWhiteQ(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 1, 1, 0);
    return FALSE;
}

BOOL s01AB_FadeInBlack(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 2, 0, 0);
    return FALSE;
}

BOOL s01AC_FadeOutBlack(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 3, 0, 0);
    return FALSE;
}

BOOL s01AD_FadeInWhite(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 2, 1, 0);
    return FALSE;
}

BOOL s01AE_FadeOutWhite(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 3, 1, 0);
    return FALSE;
}

BOOL s00B4_FadeExWait(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, letBrightnessFinishAdjusting);
    return TRUE;
}

BOOL s01A7_FadeWait(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, func_ov036_021ab25c);
    return TRUE;
}

// Fade in, slowly unless fast is 1
BOOL func_ov036_021ab498(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    Field *field = GSYS_GetField(gsys);
    u16 fast = ScriptReadAny(vm, env);
    u16 white = ScriptReadAny(vm, env);
    GameEvent *event;
    u32 mode;
    FieldFadeEvent *fade;

    mode = FIELD_FADE_IN_SLOW;
    event = GameEvent_Create(gsys, NULL, func_ov036_021ab580, sizeof(FieldFadeEvent));
    fade = GameEvent_GetData(event);
    if (fast != 1) {
        mode = FIELD_FADE_OUT_SLOW;
    }
    fade->mode = mode;
    fade->field = field;
    fade->white = white;
    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL func_ov036_021ab50c(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    Field *field = GSYS_GetField(gsys);
    u16 fast = ScriptReadAny(vm, env);
    u16 white = ScriptReadAny(vm, env);
    GameEvent *event = GameEvent_Create(gsys, NULL, func_ov036_021ab580, sizeof(FieldFadeEvent));
    FieldFadeEvent *fade = GameEvent_GetData(event);

    if (fast == 1) {
        fade->mode = FIELD_FADE_IN;
    } else {
        fade->mode = FIELD_FADE_OUT;
    }
    fade->field = field;
    fade->white = white;
    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static GameEventReturnCode func_ov036_021ab580(GameEvent *event, u32 *state, void *data) {
    FieldFadeEvent *fade = data;
    BOOL resetLCD;
    u32 enabledBGs;

    switch (*state) {
    case 0:
        resetLCD = FALSE;
        switch (fade->mode) {
        case FIELD_FADE_IN_SLOW:
            *state = 1;
            break;
        case FIELD_FADE_IN:
            if (fade->white == TRUE) {
                GXx_SetMasterBrightness_(REG_MASTER_BRIGHT_ADDR, -16);
                GXx_SetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR, -16);
                resetLCD = TRUE;
            }
            *state = 2;
            break;
        case FIELD_FADE_OUT_SLOW:
            *state = 3;
            break;
        case FIELD_FADE_OUT:
            if (fade->white == TRUE) {
                GXx_SetMasterBrightness_(REG_MASTER_BRIGHT_ADDR, 16);
                GXx_SetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR, 16);
                resetLCD = TRUE;
            }
            *state = 4;
            break;
        }
        if (resetLCD == TRUE) {
            enabledBGs = GFL_BGSysGetEnabledBGsA();
            FieldG2D_SetLCDConfig();
            GFL_BGSysSetEnabledBGsA(enabledBGs);
            FieldG2D_Prepare3DSurface(fade->field);
        }
        break;
    case 1:
        if (fade->white == TRUE) {
            BrightnessController_StartTransition(16, -16, 0, 0x3d, 1);
            GFL_FadeSet(2, 0, 16, 1);
        } else {
            BrightnessController_StartTransition(4, -16, 0, 0x3d, 3);
        }
        *state = 5;
        break;
    case 2:
        if (fade->white == TRUE) {
            GFL_FadeSet(3, 16, 0, 1);
        } else {
            BrightnessController_StartTransition(6, 0, -16, 0x3d, 3);
        }
        *state = 5;
        break;
    case 3:
        if (fade->white == TRUE) {
            BrightnessController_StartTransition(16, 16, 0, 0x3d, 1);
            GFL_FadeSet(8, 0, 16, 1);
        } else {
            BrightnessController_StartTransition(4, 16, 0, 0x3d, 1);
            BrightnessController_StartTransition(4, 16, 0, 0x3f, 2);
        }
        *state = 5;
        break;
    case 4:
        if (fade->white == TRUE) {
            GFL_FadeSet(12, 16, 0, 1);
        } else {
            BrightnessController_StartTransition(6, 0, 16, 0x3d, 1);
            BrightnessController_StartTransition(6, 0, 16, 0x3f, 2);
        }
        *state = 5;
        break;
    case 5:
        if (BrightnessController_IsTransitionComplete(3) == TRUE && GFL_FadeIsRunning() == FALSE) {
            if (fade->mode == FIELD_FADE_IN || fade->mode == FIELD_FADE_OUT) {
                BrightnessController_Reset(3);
            }
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}
