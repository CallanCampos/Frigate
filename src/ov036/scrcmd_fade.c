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

// FieldFadeEvent_Callback's sequence: start the fade of its mode, then wait for it
enum {
    FIELD_FADE_SEQ_INIT,
    FIELD_FADE_SEQ_OUT_BLACK,
    FIELD_FADE_SEQ_IN_BLACK,
    FIELD_FADE_SEQ_OUT_WHITE,
    FIELD_FADE_SEQ_IN_WHITE,
    FIELD_FADE_SEQ_WAIT,
};

// The planes that the fades brighten and darken, every one but BG1, and all of them, which a white fade takes on the
// sub screen
#define FIELD_FADE_PLANES                                                                                              \
    (GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 | GX_BLEND_PLANEMASK_OBJ |               \
     GX_BLEND_PLANEMASK_BD)
#define FIELD_FADE_PLANES_ALL                                                                                          \
    (GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_BG3 |               \
     GX_BLEND_PLANEMASK_OBJ | GX_BLEND_PLANEMASK_BD)

typedef struct {
    Field *field;
    u32 mode;
    BOOL useSubscreenMasterBrightness;
} FieldFadeEvent;

static BOOL letBrightnessFinishAdjusting(VM *vm, void *env);
static BOOL ScriptNative_WaitFieldFade(VM *vm, void *env);
static GameEventReturnCode FieldFadeEvent_Callback(GameEvent *event, u32 *state, void *data);

BOOL Scrcmd_Fade_FadeEx(VM *vm, FieldScriptEnv *env) {
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

BOOL Scrcmd_Fade_CallSeasonBanner(VM *vm, FieldScriptEnv *env) {
    ScriptWork *work = FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);
    GameData *gameData = GSYS_GetGameData(gsys);
    Field *field = GSYS_GetField(gsys);
    u32 season = GameData_GetSeason(gameData);

    ScriptWork_CallEvent(work,
                         CallFieldMapEntranceInTransition(gsys, field, FIELD_TRANSITION_SEASON, 0, 0, season, season));
    return FALSE;
}

BOOL Scrcmd_Fade_FadeInBlackQ_01A8(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), FIELD_FADE_TYPE_IN_QUICK, 0, 0);
    return FALSE;
}

BOOL Scrcmd_Fade_FadeInBlackQ(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), FIELD_FADE_TYPE_IN_QUICK, 0, 0);
    return FALSE;
}

BOOL Scrcmd_Fade_FadeOutBlackQ(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), FIELD_FADE_TYPE_OUT_QUICK, 0, 0);
    return FALSE;
}

BOOL Scrcmd_Fade_FadeInWhiteQ(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), FIELD_FADE_TYPE_IN_QUICK, 1, 0);
    return FALSE;
}

BOOL Scrcmd_Fade_FadeOutWhiteQ(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), FIELD_FADE_TYPE_OUT_QUICK, 1, 0);
    return FALSE;
}

BOOL Scrcmd_Fade_FadeInBlack(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), FIELD_FADE_TYPE_IN, 0, 0);
    return FALSE;
}

BOOL Scrcmd_Fade_FadeOutBlack(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), FIELD_FADE_TYPE_OUT, 0, 0);
    return FALSE;
}

BOOL Scrcmd_Fade_FadeInWhite(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), FIELD_FADE_TYPE_IN, 1, 0);
    return FALSE;
}

BOOL Scrcmd_Fade_FadeOutWhite(VM *vm, FieldScriptEnv *env) {
    FieldScriptEnv_GetScriptWork(env);
    GameSystem *gsys = FieldScriptEnv_GetGameSystem(env);

    FieldFadeTCB_Start(gsys, GSYS_GetField(gsys), FIELD_FADE_TYPE_OUT, 1, 0);
    return FALSE;
}

BOOL Scrcmd_Fade_FadeExWait(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, letBrightnessFinishAdjusting);
    return TRUE;
}

BOOL Scrcmd_Fade_FadeWait(VM *vm, FieldScriptEnv *env) {
    VM_SetNativeCallback(vm, ScriptNative_WaitFieldFade);
    return TRUE;
}

// Fades out to black when black is 1, otherwise white
BOOL Scrcmd_Fade_CallFieldFadeOut(VM *vm, FieldScriptEnv *env) {
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

BOOL Scrcmd_Fade_CallFieldFadeIn(VM *vm, FieldScriptEnv *env) {
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
    case FIELD_FADE_SEQ_INIT:
        resetLCD = FALSE;
        switch (fade->mode) {
        case FIELD_FADE_OUT_BLACK:
            *state = FIELD_FADE_SEQ_OUT_BLACK;
            break;
        case FIELD_FADE_IN_BLACK:
            if (fade->useSubscreenMasterBrightness == TRUE) {
                GX_SetMasterBrightness(-16);
                GXS_SetMasterBrightness(-16);
                resetLCD = TRUE;
            }
            *state = FIELD_FADE_SEQ_IN_BLACK;
            break;
        case FIELD_FADE_OUT_WHITE:
            *state = FIELD_FADE_SEQ_OUT_WHITE;
            break;
        case FIELD_FADE_IN_WHITE:
            if (fade->useSubscreenMasterBrightness == TRUE) {
                GX_SetMasterBrightness(16);
                GXS_SetMasterBrightness(16);
                resetLCD = TRUE;
            }
            *state = FIELD_FADE_SEQ_IN_WHITE;
            break;
        }
        if (resetLCD == TRUE) {
            enabledBGs = GFL_BGSysGetEnabledBGsA();
            FieldG2D_SetLCDConfig();
            GFL_BGSysSetEnabledBGsA(enabledBGs);
            FieldG2D_Prepare3DSurface(fade->field);
        }
        break;
    case FIELD_FADE_SEQ_OUT_BLACK:
        if (fade->useSubscreenMasterBrightness == TRUE) {
            BrightnessController_StartTransition(16, -16, 0, FIELD_FADE_PLANES, BRIGHTNESS_MAIN_SCREEN);
            GFL_FadeSet(FADE_ENGINE_B_BLACK, 0, 16, 1);
        } else {
            BrightnessController_StartTransition(4, -16, 0, FIELD_FADE_PLANES, BRIGHTNESS_BOTH_SCREENS);
        }
        *state = FIELD_FADE_SEQ_WAIT;
        break;
    case FIELD_FADE_SEQ_IN_BLACK:
        if (fade->useSubscreenMasterBrightness == TRUE) {
            GFL_FadeSet(FADE_ENGINE_A_BLACK | FADE_ENGINE_B_BLACK, 16, 0, 1);
        } else {
            BrightnessController_StartTransition(6, 0, -16, FIELD_FADE_PLANES, BRIGHTNESS_BOTH_SCREENS);
        }
        *state = FIELD_FADE_SEQ_WAIT;
        break;
    case FIELD_FADE_SEQ_OUT_WHITE:
        if (fade->useSubscreenMasterBrightness == TRUE) {
            BrightnessController_StartTransition(16, 16, 0, FIELD_FADE_PLANES, BRIGHTNESS_MAIN_SCREEN);
            GFL_FadeSet(FADE_ENGINE_B_WHITE, 0, 16, 1);
        } else {
            BrightnessController_StartTransition(4, 16, 0, FIELD_FADE_PLANES, BRIGHTNESS_MAIN_SCREEN);
            BrightnessController_StartTransition(4, 16, 0, FIELD_FADE_PLANES_ALL, BRIGHTNESS_SUB_SCREEN);
        }
        *state = FIELD_FADE_SEQ_WAIT;
        break;
    case FIELD_FADE_SEQ_IN_WHITE:
        if (fade->useSubscreenMasterBrightness == TRUE) {
            GFL_FadeSet(FADE_ENGINE_A_WHITE | FADE_ENGINE_B_WHITE, 16, 0, 1);
        } else {
            BrightnessController_StartTransition(6, 0, 16, FIELD_FADE_PLANES, BRIGHTNESS_MAIN_SCREEN);
            BrightnessController_StartTransition(6, 0, 16, FIELD_FADE_PLANES_ALL, BRIGHTNESS_SUB_SCREEN);
        }
        *state = FIELD_FADE_SEQ_WAIT;
        break;
    case FIELD_FADE_SEQ_WAIT:
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
