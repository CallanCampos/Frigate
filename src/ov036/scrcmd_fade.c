// The script commands that fade the screens: to and from black or white, quickly or slowly, and their waits. The ROM
// has no name for the file; scrcmd_fade.c is descriptive. Function names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)
// Additional helper and table names, and those of commands 0x2E8 and 0x2E9, are our descriptive guesses.
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
    FIELD_FADE_OUT_BLACK,
    FIELD_FADE_IN_BLACK,
    FIELD_FADE_OUT_WHITE,
    FIELD_FADE_IN_WHITE,
};

typedef struct {
    Field *field;
    u32 mode;
    BOOL useSubscreenMasterBrightness;
} FieldFadeEvent;

static BOOL letBrightnessFinishAdjusting(VM *vm, void *env);
static BOOL ScriptNative_WaitFieldFade(VM *vm, void *env);
static GameEventReturnCode FieldFadeEvent_Callback(GameEvent *event, u32 *state, void *data);

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

static BOOL ScriptNative_WaitFieldFade(VM *vm, void *env) {
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
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 0, 0, 0);
    return FALSE;
}

BOOL s01A3_FadeInBlackQ(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 0, 0, 0);
    return FALSE;
}

BOOL s01A4_FadeOutBlackQ(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 1, 0, 0);
    return FALSE;
}

BOOL s01A5_FadeInWhiteQ(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 0, 1, 0);
    return FALSE;
}

BOOL s01A6_FadeOutWhiteQ(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 1, 1, 0);
    return FALSE;
}

BOOL s01AB_FadeInBlack(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 2, 0, 0);
    return FALSE;
}

BOOL s01AC_FadeOutBlack(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 3, 0, 0);
    return FALSE;
}

BOOL s01AD_FadeInWhite(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 2, 1, 0);
    return FALSE;
}

BOOL s01AE_FadeOutWhite(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), 3, 1, 0);
    return FALSE;
}

BOOL s00B4_FadeExWait(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, letBrightnessFinishAdjusting);
    return TRUE;
}

BOOL s01A7_FadeWait(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, ScriptNative_WaitFieldFade);
    return TRUE;
}

// Fades out to black when black is 1, otherwise white
BOOL s02E8_CallFieldFadeOut(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    Field *field = GSYS_GetField(gsys);
    u16 black = ScriptReadAny(vm, env);
    u16 useSubscreenMasterBrightness = ScriptReadAny(vm, env);
    GameEvent *event;
    u32 mode;
    FieldFadeEvent *fade;

    mode = FIELD_FADE_OUT_BLACK;
    event = GameEvent_Create(gsys, NULL, FieldFadeEvent_Callback, sizeof(FieldFadeEvent));
    fade = GameEvent_GetData(event);
    if (black != 1) {
        mode = FIELD_FADE_OUT_WHITE;
    }
    fade->mode = mode;
    fade->field = field;
    fade->useSubscreenMasterBrightness = useSubscreenMasterBrightness;
    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

BOOL s02E9_CallFieldFadeIn(VM *vm, FieldScriptEnv *env) {
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    Field *field = GSYS_GetField(gsys);
    u16 black = ScriptReadAny(vm, env);
    u16 useSubscreenMasterBrightness = ScriptReadAny(vm, env);
    GameEvent *event = GameEvent_Create(gsys, NULL, FieldFadeEvent_Callback, sizeof(FieldFadeEvent));
    FieldFadeEvent *fade = GameEvent_GetData(event);

    if (black == 1) {
        fade->mode = FIELD_FADE_IN_BLACK;
    } else {
        fade->mode = FIELD_FADE_IN_WHITE;
    }
    fade->field = field;
    fade->useSubscreenMasterBrightness = useSubscreenMasterBrightness;
    if (event == NULL) {
        return FALSE;
    }
    ScriptWork_CallEvent(work, event);
    return TRUE;
}

static GameEventReturnCode FieldFadeEvent_Callback(GameEvent *event, u32 *state, void *data) {
    FieldFadeEvent *fade = data;
    BOOL resetLCD;
    u32 enabledBGs;

    switch (*state) {
    case 0:
        resetLCD = FALSE;
        switch (fade->mode) {
        case FIELD_FADE_OUT_BLACK:
            *state = 1;
            break;
        case FIELD_FADE_IN_BLACK:
            if (fade->useSubscreenMasterBrightness == TRUE) {
                GX_SetMasterBrightness(-16);
                GXS_SetMasterBrightness(-16);
                resetLCD = TRUE;
            }
            *state = 2;
            break;
        case FIELD_FADE_OUT_WHITE:
            *state = 3;
            break;
        case FIELD_FADE_IN_WHITE:
            if (fade->useSubscreenMasterBrightness == TRUE) {
                GX_SetMasterBrightness(16);
                GXS_SetMasterBrightness(16);
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
        if (fade->useSubscreenMasterBrightness == TRUE) {
            BrightnessController_StartTransition(16, -16, 0, 0x3d, BRIGHTNESS_MAIN_SCREEN);
            GFL_FadeSet(FADE_ENGINE_B_BLACK, 0, 16, 1);
        } else {
            BrightnessController_StartTransition(4, -16, 0, 0x3d, BRIGHTNESS_BOTH_SCREENS);
        }
        *state = 5;
        break;
    case 2:
        if (fade->useSubscreenMasterBrightness == TRUE) {
            GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 1);
        } else {
            BrightnessController_StartTransition(6, 0, -16, 0x3d, BRIGHTNESS_BOTH_SCREENS);
        }
        *state = 5;
        break;
    case 3:
        if (fade->useSubscreenMasterBrightness == TRUE) {
            BrightnessController_StartTransition(16, 16, 0, 0x3d, BRIGHTNESS_MAIN_SCREEN);
            GFL_FadeSet(FADE_ENGINE_B_WHITE, 0, 16, 1);
        } else {
            BrightnessController_StartTransition(4, 16, 0, 0x3d, BRIGHTNESS_MAIN_SCREEN);
            BrightnessController_StartTransition(4, 16, 0, 0x3f, BRIGHTNESS_SUB_SCREEN);
        }
        *state = 5;
        break;
    case 4:
        if (fade->useSubscreenMasterBrightness == TRUE) {
            GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 16, 0, 1);
        } else {
            BrightnessController_StartTransition(6, 0, 16, 0x3d, BRIGHTNESS_MAIN_SCREEN);
            BrightnessController_StartTransition(6, 0, 16, 0x3f, BRIGHTNESS_SUB_SCREEN);
        }
        *state = 5;
        break;
    case 5:
        if (BrightnessController_IsTransitionComplete(BRIGHTNESS_BOTH_SCREENS) == TRUE &&
            GFL_FadeIsRunning() == FALSE) {
            if (fade->mode == FIELD_FADE_IN_BLACK || fade->mode == FIELD_FADE_IN_WHITE) {
                BrightnessController_Reset(BRIGHTNESS_BOTH_SCREENS);
            }
            return GAMEEVENT_DONE;
        }
        break;
    }
    return GAMEEVENT_CONTINUE;
}
