#include "types.h"
#include "constants/arc.h"
#include "constants/narc_intro.h"
#include "constants/sound.h"
#include "demo/intro.h"
#include "demo/intro_script.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/clact.h"
#include "gfl/fade.h"
#include "gfl/graphics.h"
#include "gfl/key.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nnsys/g2d.h"
#include "save/player_info.h"
#include "system/brightness.h"
#include "system/mcss.h"
#include "system/printsys.h"
#include "system/wordset.h"

// Runs the intro's scripts (intro_script.c)

// Up to this many commands run at once
#define RUNNING_MAX 8

typedef struct {
    u32 state;
    s32 counter;
    u32 unk8;
} IntroCmdWork;

struct IntroCmd {
    HeapID heapId;
    IntroParam *param;
    IntroMcss *mcss;
    IntroG3d *g3d;
    IntroParticle *particle;
    IntroGraphic *graphic;
    IntroMsg *msg;
    u32 script;
    const IntroCmdEntry *running[RUNNING_MAX];
    IntroCmdWork work[RUNNING_MAX];
    // The next command of the script
    u32 pc;
    SaveControlIntr *saveTask;
    // The screen of the band, which INTRO_CMD_OPEN_BAND copies to BG 2 a few rows at a time
    u16 bandScreen[64 * 32];
    ClActor *obj;
    u32 objChars;
    u32 objPalette;
    u32 objCellAnims;
};

// Returns TRUE once the command has ended. The arguments are not const: loads through a const pointer are scheduled
// differently, so the scripts' entries are cast when passed
typedef BOOL (*IntroCmdFunc)(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
typedef BOOL (*IntroCondFunc)(IntroCmd *cmd);
typedef void (*IntroWordFunc)(IntroCmd *cmd, u32 slot);

// Each version has its own palette for the BGs
#ifdef BLACK2
#define BG_PALETTE_FILE 2
#else
#define BG_PALETTE_FILE 1
#endif

// The planes that INTRO_CMD_SET_BRIGHTNESS and the transition darken: all but BG 1, which has the messages
#define BRIGHTNESS_PLANES (GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_BG3 | GX_PLANEMASK_OBJ)

static BOOL IntroCmd_Nop(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_GoTo(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_GoToModeScript(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_YesNo(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_If(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_LoadGraphics(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SetResult(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_Wait(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_Fade(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SetBrightness(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_StartBrightnessTransition(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_CheckBrightness(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_BGMPlay(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_WaitBGMPlaying(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_BGMFadeOut(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_BGMFadeIn(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SEPlay(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SEStop(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_WaitInput(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_LoadMessages(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SetWord(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_Message(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_ClearMessage(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_AddSprite(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SetSpriteVisible(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SetSpriteAnimation(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_WaitSpriteAnimation(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_ClearSpriteAnimationEnded(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_MoveSpriteX(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_FadeSprite(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_Talk(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SaveStart(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SavePause(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SaveResume(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_Save43(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SaveWait(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SetMsgLanguage(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SetGender(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_PokemonAppear(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_Particles(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SetModelVisible(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_SetModelFrame(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_OpenModel(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_ChooseGender(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_ModelBack(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_OpenBand(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_CloseBand(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_CreateObj(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_FreeObj(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_ObjFadeIn(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCmd_ObjFadeOut(IntroCmd *cmd, IntroCmdWork *work, s32 *args);
static BOOL IntroCond_PlayerIsMale(IntroCmd *cmd);
static void IntroWord_PlayerName(IntroCmd *cmd, u32 slot);
static void IntroWord_RivalName(IntroCmd *cmd, u32 slot);
static BOOL IntroCmd_Start(IntroCmd *cmd, const IntroCmdEntry *entry);
static BOOL IntroCmd_RunCommands(IntroCmd *cmd);
static void IntroCmd_End(IntroCmd *cmd, u8 slot);

static const ClActorSetup sObjSetup = { 128, 72, 0, 0, 1 };

// Professor Juniper's sprite
static const MCSSLoadInfo sProfessorLoadInfo = { ARCID_INTRO,
                                                 NARC_INTRO_PROFESSOR_NCGR,
                                                 NARC_INTRO_PROFESSOR_NCLR,
                                                 NARC_INTRO_PROFESSOR_NCER,
                                                 NARC_INTRO_PROFESSOR_NANR,
                                                 NARC_INTRO_PROFESSOR_NMCR,
                                                 NARC_INTRO_PROFESSOR_NMAR,
                                                 NARC_INTRO_PROFESSOR_BIN,
                                                 0 };

// INTRO_COND_*
static IntroCondFunc sConditions[] = {
    IntroCond_PlayerIsMale,
};

// INTRO_WORD_*
static IntroWordFunc sWordFuncs[] = {
    IntroWord_PlayerName,
    IntroWord_RivalName,
};

static IntroCmdFunc sCommands[] = {
    [INTRO_CMD_NONE] = NULL,
    [INTRO_CMD_GOTO] = IntroCmd_GoTo,
    [INTRO_CMD_GOTO_MODE_SCRIPT] = IntroCmd_GoToModeScript,
    [INTRO_CMD_YES_NO] = IntroCmd_YesNo,
    [INTRO_CMD_IF] = IntroCmd_If,
    [INTRO_CMD_LOAD_GRAPHICS] = IntroCmd_LoadGraphics,
    [INTRO_CMD_SET_RESULT] = IntroCmd_SetResult,
    [INTRO_CMD_WAIT] = IntroCmd_Wait,
    [INTRO_CMD_FADE] = IntroCmd_Fade,
    [INTRO_CMD_SET_BRIGHTNESS] = IntroCmd_SetBrightness,
    [INTRO_CMD_START_BRIGHTNESS_TRANSITION] = IntroCmd_StartBrightnessTransition,
    [INTRO_CMD_CHECK_BRIGHTNESS] = IntroCmd_CheckBrightness,
    [INTRO_CMD_BGM_PLAY] = IntroCmd_BGMPlay,
    [INTRO_CMD_BGM_FADE_OUT] = IntroCmd_BGMFadeOut,
    [INTRO_CMD_BGM_FADE_IN] = IntroCmd_BGMFadeIn,
    [INTRO_CMD_WAIT_BGM_PLAYING] = IntroCmd_WaitBGMPlaying,
    [INTRO_CMD_SE_PLAY] = IntroCmd_SEPlay,
    [INTRO_CMD_SE_STOP] = IntroCmd_SEStop,
    [INTRO_CMD_WAIT_INPUT] = IntroCmd_WaitInput,
    [INTRO_CMD_LOAD_MESSAGES] = IntroCmd_LoadMessages,
    [INTRO_CMD_SET_WORD] = IntroCmd_SetWord,
    [INTRO_CMD_MESSAGE] = IntroCmd_Message,
    [INTRO_CMD_CLEAR_MESSAGE] = IntroCmd_ClearMessage,
    [INTRO_CMD_ADD_SPRITE] = IntroCmd_AddSprite,
    [INTRO_CMD_SET_SPRITE_VISIBLE] = IntroCmd_SetSpriteVisible,
    [INTRO_CMD_SET_SPRITE_ANIMATION] = IntroCmd_SetSpriteAnimation,
    [INTRO_CMD_WAIT_SPRITE_ANIMATION] = IntroCmd_WaitSpriteAnimation,
    [INTRO_CMD_CLEAR_SPRITE_ANIMATION_ENDED] = IntroCmd_ClearSpriteAnimationEnded,
    [INTRO_CMD_MOVE_SPRITE_X] = IntroCmd_MoveSpriteX,
    [INTRO_CMD_FADE_SPRITE] = IntroCmd_FadeSprite,
    [INTRO_CMD_TALK] = IntroCmd_Talk,
    [INTRO_CMD_SET_MSG_LANGUAGE] = IntroCmd_SetMsgLanguage,
    [INTRO_CMD_SET_GENDER] = IntroCmd_SetGender,
    [INTRO_CMD_POKEMON_APPEAR] = IntroCmd_PokemonAppear,
    [INTRO_CMD_PARTICLES] = IntroCmd_Particles,
    [INTRO_CMD_SET_MODEL_VISIBLE] = IntroCmd_SetModelVisible,
    [INTRO_CMD_SET_MODEL_FRAME] = IntroCmd_SetModelFrame,
    [INTRO_CMD_OPEN_MODEL] = IntroCmd_OpenModel,
    [INTRO_CMD_CHOOSE_GENDER] = IntroCmd_ChooseGender,
    [INTRO_CMD_MODEL_BACK] = IntroCmd_ModelBack,
    [INTRO_CMD_SAVE_START] = IntroCmd_SaveStart,
    [INTRO_CMD_SAVE_PAUSE] = IntroCmd_SavePause,
    [INTRO_CMD_SAVE_RESUME] = IntroCmd_SaveResume,
    [INTRO_CMD_SAVE_43] = IntroCmd_Save43,
    [INTRO_CMD_SAVE_WAIT] = IntroCmd_SaveWait,
    [INTRO_CMD_OPEN_BAND] = IntroCmd_OpenBand,
    [INTRO_CMD_CLOSE_BAND] = IntroCmd_CloseBand,
    [INTRO_CMD_CREATE_OBJ] = IntroCmd_CreateObj,
    [INTRO_CMD_FREE_OBJ] = IntroCmd_FreeObj,
    [INTRO_CMD_OBJ_FADE_IN] = IntroCmd_ObjFadeIn,
    [INTRO_CMD_OBJ_FADE_OUT] = IntroCmd_ObjFadeOut,
    [INTRO_CMD_NOP] = IntroCmd_Nop,
    [INTRO_CMD_END] = NULL,
};

static BOOL IntroCmd_Nop(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    return TRUE;
}

static BOOL IntroCmd_GoTo(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    cmd->script = args[0];
    cmd->pc = 0;
    return TRUE;
}

// Mode 0 starts from the beginning too
static BOOL IntroCmd_GoToModeScript(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    s32 script = cmd->param->mode;

    if (script == 0) {
        script++;
    }
    IntroCmd_GoTo(cmd, NULL, &script);
    return TRUE;
}

// Starts the command after the question for yes, or the one after that for no, and goes on past both
static void IntroCmd_FinishYesNo(IntroCmd *cmd, s32 choice) {
    const IntroCmdEntry *entry = &IntroScript_Get(cmd->script)[cmd->pc];

    IntroMsg_CloseMenu(cmd->msg);
    if (choice == 0) {
        if (entry != NULL) {
            IntroCmd_Start(cmd, entry);
        }
    } else if (++entry != NULL) {
        IntroCmd_Start(cmd, entry);
    }
    cmd->pc += 2;
}

static BOOL IntroCmd_YesNo(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    IntroMenuItem items[2];
    s32 choice;
    u32 result;

    switch (work->state) {
    case 0:
        items[0].message = args[0];
        items[0].value = 0;
        items[1].message = args[1];
        items[1].value = 1;
        IntroMsg_OpenMenu(cmd->msg, items, 2, args[2]);
        work->state++;
        break;
    case 1:
        IntroMsg_UpdateMenu(cmd->msg);
        result = IntroMsg_GetMenuResult(cmd->msg, &choice);
        if (result == INTRO_MENU_CHOSEN) {
            IntroCmd_FinishYesNo(cmd, choice);
            return TRUE;
        } else if (result == INTRO_MENU_CANCELLED) {
            IntroCmd_FinishYesNo(cmd, 1);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// Starts the next command if the condition holds, or the one after it if not, and goes on past both
static BOOL IntroCmd_If(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    const IntroCmdEntry *entry = &IntroScript_Get(cmd->script)[cmd->pc];

    if (!sConditions[args[0]](cmd)) {
        entry++;
    }
    if (entry != NULL) {
        IntroCmd_Start(cmd, entry);
    }
    cmd->pc += 2;
    return TRUE;
}

static BOOL IntroCmd_LoadGraphics(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    HeapID heapId = cmd->heapId;
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_INTRO, heapId);
    NNSG2dScreenData *screen;
    void *file;

    GFL_G2DIOLoadArcNCLRDefault(arc, BG_PALETTE_FILE, 0, 0, 0x40, heapId);
    GFL_G2DIOLoadArcNCLRDefault(arc, BG_PALETTE_FILE, 4, 0, 0x20, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0, 6, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 3, 6, 0, 0, FALSE, heapId);
    GFL_BGSysLoadArcNCGRStatic(arc, 0, 3, 0, 0, FALSE, heapId);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 4, 3, 0, 0, FALSE, heapId);
    if (args[0] == TRUE) {
        GFL_BGSysLoadArcNCGRStatic(arc, 5, 2, 0, 0, FALSE, heapId);
        file = GFL_G2DIOReadNSCRArc(arc, 7, FALSE, &screen, heapId);
        sys_memcpy16(screen->rawData, cmd->bandScreen, sizeof(cmd->bandScreen));
        GFL_HeapFree(file);
    }
    GFL_ArcToolFree(arc);
    return TRUE;
}

static BOOL IntroCmd_SetResult(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    cmd->param->result = args[0];
    return TRUE;
}

static BOOL IntroCmd_Wait(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    if (work->counter > args[0]) {
        return TRUE;
    }
    work->counter++;
    return FALSE;
}

static BOOL IntroCmd_Fade(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    switch (work->state) {
    case 0:
        GFL_FadeSet(args[0], args[1], args[2], args[3]);
        work->state++;
        break;
    case 1:
        if (!GFL_FadeIsRunning()) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL IntroCmd_SetBrightness(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    BrightnessController_SetScreenBrightness(args[0], BRIGHTNESS_PLANES, BRIGHTNESS_MAIN_SCREEN);
    return TRUE;
}

static BOOL IntroCmd_StartBrightnessTransition(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    BrightnessController_StartTransition(args[0], args[1], args[2], BRIGHTNESS_PLANES, BRIGHTNESS_MAIN_SCREEN);
    return TRUE;
}

static BOOL IntroCmd_CheckBrightness(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    BrightnessController_IsTransitionComplete(BRIGHTNESS_MAIN_SCREEN);
    return TRUE;
}

static BOOL IntroCmd_BGMPlay(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    GFL_SndBGMPlay(args[0], SND_CHANNEL_MASK_ALL);
    return TRUE;
}

static BOOL IntroCmd_WaitBGMPlaying(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    if (GFL_SndBGMIsPlaying()) {
        return TRUE;
    }
    return FALSE;
}

static BOOL IntroCmd_BGMFadeOut(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    GFL_SndBGMFadeOut(args[0]);
    return TRUE;
}

static BOOL IntroCmd_BGMFadeIn(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    GFL_SndBGMFadeIn(args[0]);
    return TRUE;
}

static BOOL IntroCmd_SEPlay(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    s32 player;

    GFL_SndSEPlay(args[0]);
    player = GFL_SndSeqGetPlayerIndex(args[0]);
    if (args[1] != 0) {
        GFL_SndPlayerSetVolume(player, args[1]);
    }
    if (args[2] != 0) {
        GFL_SndPlayerSetParams(player, -1, -1, args[2]);
    }
    return TRUE;
}

static BOOL IntroCmd_SEStop(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    GFL_SndPlayerStop(GFL_SndSeqGetPlayerIndex(args[0]));
    return TRUE;
}

static BOOL IntroCmd_WaitInput(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    if (func_0203da48() || GCTX_HIDGetPressedKeys()) {
        return TRUE;
    }
    return FALSE;
}

static BOOL IntroCmd_LoadMessages(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    IntroMsg_LoadMessages(cmd->msg, args[0], args[1]);
    return TRUE;
}

static BOOL IntroCmd_SetWord(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    sWordFuncs[args[0]](cmd, args[1]);
    return TRUE;
}

static BOOL IntroCmd_Message(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    switch (work->state) {
    case 0:
        IntroMsg_Print(cmd->msg, args[0], args[1]);
        work->state++;
        break;
    case 1:
        if (IntroMsg_UpdatePrint(cmd->msg)) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL IntroCmd_ClearMessage(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    IntroMsg_Clear(cmd->msg);
    return TRUE;
}

static BOOL IntroCmd_AddSprite(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    if (args[1] == 0) {
        IntroMcss_Add(cmd->mcss, args[2], args[3], 0, &sProfessorLoadInfo, args[0]);
    } else {
        IntroMcss_AddPokemon(cmd->mcss, args[2], args[3], FX32_ONE, args[1], args[0]);
    }
    return TRUE;
}

static BOOL IntroCmd_SetSpriteVisible(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    IntroMcss_SetVisible(cmd->mcss, args[1], args[0]);
    return TRUE;
}

static BOOL IntroCmd_SetSpriteAnimation(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    IntroMcss_SetAnimation(cmd->mcss, args[0], args[1], args[2]);
    return TRUE;
}

static BOOL IntroCmd_WaitSpriteAnimation(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    return IntroMcss_IsAnimationEnded(cmd->mcss, args[0]);
}

static BOOL IntroCmd_ClearSpriteAnimationEnded(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    IntroMcss_ClearAnimationEnded(cmd->mcss, args[0]);
    return TRUE;
}

static BOOL IntroCmd_MoveSpriteX(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    return IntroMcss_MoveX(cmd->mcss, args[0], args[1], args[2]);
}

static BOOL IntroCmd_FadeSprite(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    switch (work->state) {
    case 0:
        if (args[1]) {
            IntroMcss_SetAlpha(cmd->mcss, args[0], 1);
        } else {
            IntroMcss_SetAlpha(cmd->mcss, args[0], 31);
        }
        IntroMcss_SetVisible(cmd->mcss, TRUE, args[0]);
        work->counter = 31;
        work->state++;
        break;
    case 1:
        work->counter -= 2;
        if (work->counter <= 0) {
            IntroMcss_SetAlpha(cmd->mcss, args[0], 31);
            IntroMcss_SetVisible(cmd->mcss, args[1], args[0]);
            return TRUE;
        }
        if (args[1] == TRUE) {
            IntroMcss_SetAlpha(cmd->mcss, args[0], (u8)(31 - work->counter));
        } else if (args[1] == FALSE) {
            IntroMcss_SetAlpha(cmd->mcss, args[0], (u8)work->counter);
        }
        break;
    }
    return FALSE;
}

static BOOL IntroCmd_Talk(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    s32 message = args[0];
    s32 sprite = args[1];

    switch (work->state) {
    case 0:
        IntroMsg_Print(cmd->msg, message, args[2]);
        work->state++;
        break;
    case 1:
        if (IntroMsg_UpdatePrint(cmd->msg)) {
            func_ov294_021a3798(cmd->mcss, sprite, TRUE);
            return TRUE;
        }
        switch (IntroMsg_GetPrintState(cmd->msg)) {
        case PRINT_STREAM_PAUSED:
            func_ov294_021a3798(cmd->mcss, sprite, TRUE);
            work->counter = 0;
            break;
        case PRINT_STREAM_RUNNING:
            func_ov294_021a3798(cmd->mcss, sprite, work->counter % 12 < 6 ? TRUE : FALSE);
            work->counter++;
            break;
        case PRINT_STREAM_DONE:
            break;
        }
        break;
    }
    return FALSE;
}

static BOOL IntroCmd_SaveStart(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    SaveControlIntr_Start(cmd->saveTask);
    return TRUE;
}

static BOOL IntroCmd_SavePause(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    switch (work->state) {
    case 0:
        SaveControlIntr_Pause(cmd->saveTask);
        work->state++;
        break;
    case 1:
        if (SaveControlIntr_IsPausedOrDone(cmd->saveTask) == TRUE) {
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL IntroCmd_SaveResume(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    SaveControlIntr_Resume(cmd->saveTask);
    return TRUE;
}

static BOOL IntroCmd_Save43(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    SaveControlIntr_Nop(cmd->saveTask);
    return TRUE;
}

static BOOL IntroCmd_SaveWait(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    switch (work->state) {
    case 0:
        if (SaveControlIntr_IsDone(cmd->saveTask) == TRUE) {
            return TRUE;
        }
        // Creating save data. Please wait...
        IntroMsg_Print(cmd->msg, 5, FALSE);
        work->state++;
        break;
    case 1:
        if (IntroMsg_UpdatePrint(cmd->msg) == TRUE) {
            IntroMsg_ShowWaitIcon(cmd->msg);
            work->state++;
        }
        break;
    case 2:
        if (SaveControlIntr_IsDone(cmd->saveTask) == TRUE) {
            IntroMsg_HideWaitIcon(cmd->msg);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL IntroCmd_SetMsgLanguage(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    Config *config = cmd->param->config;

    if (args[0] == FALSE) {
        func_02008a8c(config, FALSE);
    } else {
        func_02008a8c(config, TRUE);
    }
    return TRUE;
}

static BOOL IntroCmd_SetGender(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    PlayerInfo *info = cmd->param->playerInfo;

    if (args[0] == 0) {
        setTrainerGender(info, GENDER_MALE);
    } else {
        setTrainerGender(info, GENDER_FEMALE);
    }
    return TRUE;
}

// The Pokémon drops to its place, then plays its animation and cries
static BOOL IntroCmd_PokemonAppear(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    switch (work->state) {
    case 0:
        if (work->counter == 3) {
            work->counter = 0;
            work->state++;
        } else {
            work->counter++;
        }
        break;
    case 1:
        if (IntroMcss_DecreaseY(cmd->mcss, FX32_ONE, FX32_CONST(-11.5)) == TRUE) {
            GFL_SndSEPlay(SEQ_SE_TOUJOU_INTRO);
            work->state++;
        }
        break;
    case 2:
        if (!GFL_SndPlayerIsActiveAny()) {
            IntroMcss_SetAnimation(cmd->mcss, INTRO_SPRITE_POKEMON, 0, TRUE);
            PokeVoice_StartPlayback(cmd->param->pokeVoice);
            work->state++;
        }
        break;
    case 3:
        if (!PokeVoice_IsPlaying(cmd->param->pokeVoice)) {
            work->state = 0;
            return TRUE;
        }
        break;
    }
    return FALSE;
}

static BOOL IntroCmd_Particles(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 1, 9, 0, 0);
    IntroParticle_SetPos(cmd->particle, args[0], args[1], 0);
    return TRUE;
}

static BOOL IntroCmd_SetModelVisible(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    IntroG3d_SetVisible(cmd->g3d, args[0]);
    return TRUE;
}

static BOOL IntroCmd_SetModelFrame(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    IntroG3d_SetFrame(cmd->g3d, args[0]);
    return TRUE;
}

static BOOL IntroCmd_OpenModel(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    if (IntroG3d_Open(cmd->g3d)) {
        return TRUE;
    }
    return FALSE;
}

// The model turns left for the boy and right for the girl. work->counter is the choice, 0 for the boy
static BOOL IntroCmd_ChooseGender(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    switch (work->state) {
    case 0:
        IntroG3d_SetMode(cmd->g3d, 0);
        work->state = 4;
        break;
    case 1:
        if (GCTX_HIDGetPressedKeys() & PAD_KEY_RIGHT) {
            if (work->counter == 1) {
                break;
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            IntroG3d_SetMode(cmd->g3d, 2);
            work->counter = 1;
            work->state = 4;
        } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_LEFT) {
            if (work->counter == 0) {
                break;
            }
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            IntroG3d_SetMode(cmd->g3d, 1);
            work->counter = 0;
            work->state = 4;
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            GFL_SndSEPlay(SEQ_SE_DECIDE2);
            work->state++;
        }
        break;
    case 2: {
        PlayerInfo *info = cmd->param->playerInfo;

        if (work->counter == 0) {
            setTrainerGender(info, GENDER_MALE);
            IntroG3d_SetMode(cmd->g3d, 3);
        } else {
            setTrainerGender(info, GENDER_FEMALE);
            IntroG3d_SetMode(cmd->g3d, 4);
        }
        work->state++;
        break;
    }
    case 3:
        if (IntroG3d_Animate(cmd->g3d)) {
            return TRUE;
        }
        break;
    case 4:
        if (IntroG3d_Animate(cmd->g3d)) {
            work->state = 1;
        }
        break;
    }
    return FALSE;
}

static BOOL IntroCmd_ModelBack(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    if (IntroG3d_AnimateBack(cmd->g3d)) {
        return TRUE;
    }
    return FALSE;
}

// Copies the band to BG 2 from its middle rows outwards, two rows a frame
static BOOL IntroCmd_OpenBand(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    switch (work->state) {
    case 0:
        GFL_BGSysSetBGEnabled(2, TRUE);
        GFL_BGSysLoadScrAreaLarge(2, 0, 8, 64, 2, cmd->bandScreen, 0, 8, 64, 32);
        GFL_BGSysQueueScrLoad(2);
        work->state++;
        break;
    case 1:
        GFL_BGSysLoadScrAreaLarge(2, 0, 7, 64, 1, cmd->bandScreen, 0, 7, 64, 32);
        GFL_BGSysLoadScrAreaLarge(2, 0, 10, 64, 1, cmd->bandScreen, 0, 10, 64, 32);
        GFL_BGSysQueueScrLoad(2);
        work->state++;
        break;
    case 2:
        GFL_BGSysLoadScrAreaLarge(2, 0, 6, 64, 1, cmd->bandScreen, 0, 6, 64, 32);
        GFL_BGSysLoadScrAreaLarge(2, 0, 11, 64, 1, cmd->bandScreen, 0, 11, 64, 32);
        GFL_BGSysQueueScrLoad(2);
        work->state++;
        break;
    case 3:
        GFL_BGSysLoadScrAreaLarge(2, 0, 5, 64, 1, cmd->bandScreen, 0, 5, 64, 32);
        GFL_BGSysLoadScrAreaLarge(2, 0, 12, 64, 1, cmd->bandScreen, 0, 12, 64, 32);
        GFL_BGSysQueueScrLoad(2);
        work->state++;
        break;
    case 4:
        GFL_BGSysLoadScrAreaLarge(2, 0, 4, 64, 1, cmd->bandScreen, 0, 4, 64, 32);
        GFL_BGSysLoadScrAreaLarge(2, 0, 13, 64, 1, cmd->bandScreen, 0, 13, 64, 32);
        GFL_BGSysQueueScrLoad(2);
        work->state = 0;
        return TRUE;
    }
    return FALSE;
}

// Clears BG 2 two rows a frame, then hides it. The rows do not all match the ones that INTRO_CMD_OPEN_BAND fills
static BOOL IntroCmd_CloseBand(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    switch (work->state) {
    case 0:
        GFL_BGSysFillScrArea(2, 0, 0, 7, 64, 1, 0);
        GFL_BGSysFillScrArea(2, 0, 0, 16, 64, 1, 0);
        GFL_BGSysQueueScrLoad(2);
        work->state++;
        break;
    case 1:
        GFL_BGSysFillScrArea(2, 0, 0, 8, 64, 1, 0);
        GFL_BGSysFillScrArea(2, 0, 0, 15, 64, 1, 0);
        GFL_BGSysQueueScrLoad(2);
        work->state++;
        break;
    case 2:
        GFL_BGSysFillScrArea(2, 0, 0, 6, 64, 1, 0);
        GFL_BGSysFillScrArea(2, 0, 0, 14, 64, 1, 0);
        GFL_BGSysQueueScrLoad(2);
        work->state++;
        break;
    case 3:
        GFL_BGSysFillScrArea(2, 0, 0, 5, 64, 1, 0);
        GFL_BGSysFillScrArea(2, 0, 0, 13, 64, 1, 0);
        GFL_BGSysQueueScrLoad(2);
        work->state++;
        break;
    case 4:
        GFL_BGSysSetBGEnabled(2, FALSE);
        work->state = 0;
        return TRUE;
    }
    return FALSE;
}

static BOOL IntroCmd_CreateObj(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_INTRO, cmd->heapId);
    ClActorSetup setup;

    cmd->objChars = func_0204b81c(arc, 10, 0, 0, cmd->heapId);
    cmd->objPalette = func_0204bba0(arc, 11, 0, 0x20, cmd->heapId);
    cmd->objCellAnims = func_0204bde0(arc, 9, 8, cmd->heapId);
    GFL_ArcToolFree(arc);
    setup = sObjSetup;
    cmd->obj = func_0204c040(IntroGraphic_GetClActUnit(cmd->graphic), cmd->objChars, cmd->objPalette,
                             cmd->objCellAnims, &setup, 0, cmd->heapId);
    func_0204c124(cmd->obj, FALSE);
    return TRUE;
}

static BOOL IntroCmd_FreeObj(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    func_0204b98c(cmd->objChars);
    func_0204bcd0(cmd->objPalette);
    func_0204be64(cmd->objCellAnims);
    func_0204c108(cmd->obj);
    return TRUE;
}

// Fades the actor in over 16 frames with the blend registers
static BOOL IntroCmd_ObjFadeIn(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    u32 i;

    switch (work->state) {
    case 0:
        gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 0, 12, 0, 16);
        func_0204c318(cmd->obj, GX_OAM_MODE_XLU);
        func_0204c124(cmd->obj, TRUE);
        work->state++;
        break;
    case 16:
        for (i = 0; i < 3; i++) {
            if (func_0204c370(cmd->obj) == GX_OAM_MODE_XLU) {
                func_0204c318(cmd->obj, GX_OAM_MODE_NORMAL);
            }
        }
        work->state = 0;
        return TRUE;
    default:
        work->state++;
        reg_G2_BLDALPHA = work->state | ((16 - work->state) << 8);
        break;
    }
    return FALSE;
}

static BOOL IntroCmd_ObjFadeOut(IntroCmd *cmd, IntroCmdWork *work, s32 *args) {
    u32 i;

    switch (work->state) {
    case 0:
        gfxRegSetAlphaBlend(REG_BLDCNT_ADDR, 0, 12, 16, 0);
        for (i = 0; i < 3; i++) {
            func_0204c318(cmd->obj, GX_OAM_MODE_XLU);
        }
        work->state++;
        break;
    case 16:
        for (i = 0; i < 3; i++) {
            func_0204c318(cmd->obj, GX_OAM_MODE_NORMAL);
            func_0204c124(cmd->obj, FALSE);
        }
        work->state = 0;
        return TRUE;
    default:
        work->state++;
        reg_G2_BLDALPHA = (16 - work->state) | (work->state << 8);
        break;
    }
    return FALSE;
}

static BOOL IntroCond_PlayerIsMale(IntroCmd *cmd) {
    if (getTrainerGender(cmd->param->playerInfo) == GENDER_MALE) {
        return TRUE;
    }
    return FALSE;
}

static void IntroWord_PlayerName(IntroCmd *cmd, u32 slot) {
    copyVarForText(IntroMsg_GetWordSet(cmd->msg), slot, cmd->param->playerInfo);
}

static void IntroWord_RivalName(IntroCmd *cmd, u32 slot) {
    GFL_WordSetLoadStr(IntroMsg_GetWordSet(cmd->msg), slot, cmd->param->rivalName);
}

IntroCmd *IntroCmd_Create(IntroG3d *g3d, IntroParticle *particle, IntroMcss *mcss, IntroParam *param,
                          IntroGraphic *graphic, HeapID heapId) {
    IntroCmd *cmd = GFL_HeapAllocate(heapId, sizeof(IntroCmd), TRUE, "intro_cmd.c", 1804);

    cmd->heapId = heapId;
    cmd->param = param;
    cmd->mcss = mcss;
    cmd->g3d = g3d;
    cmd->particle = particle;
    cmd->graphic = graphic;
    cmd->saveTask = param->saveTask;
    cmd->msg = IntroMsg_Create(heapId);
    return cmd;
}

void IntroCmd_Free(IntroCmd *cmd) {
    IntroMsg_Free(cmd->msg);
    GFL_HeapFree(cmd);
}

// Runs the commands, and when they have all ended, starts the next one, and the ones that run with it. A jump to
// another script happens at once
BOOL IntroCmd_Update(IntroCmd *cmd) {
    int i;
    const IntroCmdEntry *entry;

    IntroMsg_Update(cmd->msg);
    if (IntroCmd_RunCommands(cmd) == FALSE) {
        for (i = 0; i < RUNNING_MAX + 1; i++) {
            entry = &IntroScript_Get(cmd->script)[cmd->pc];
            if (entry->cmd == INTRO_CMD_GOTO) {
                sCommands[entry->cmd](cmd, &cmd->work[i], (s32 *)entry->args);
                entry = IntroScript_Get(cmd->script);
            }
            if (entry->cmd == INTRO_CMD_END) {
                return FALSE;
            }
            cmd->pc++;
            if (!IntroCmd_Start(cmd, entry)) {
                break;
            }
        }
    }
    return TRUE;
}

// Puts a command in a free slot, and returns whether the next one starts with it
static BOOL IntroCmd_Start(IntroCmd *cmd, const IntroCmdEntry *entry) {
    int i;

    for (i = 0; i < RUNNING_MAX; i++) {
        if (cmd->running[i] == NULL) {
            cmd->running[i] = entry;
            break;
        }
    }
    return entry->runNext;
}

// Returns TRUE while a command is still running
static BOOL IntroCmd_RunCommands(IntroCmd *cmd) {
    BOOL running = FALSE;
    int i;
    const IntroCmdEntry *entry;

    for (i = 0; i < RUNNING_MAX; i++) {
        entry = cmd->running[i];
        if (entry != NULL) {
            if (sCommands[entry->cmd](cmd, &cmd->work[i], (s32 *)entry->args) == FALSE) {
                running = TRUE;
            } else {
                IntroCmd_End(cmd, i);
            }
        }
    }
    return running;
}

static void IntroCmd_End(IntroCmd *cmd, u8 slot) {
    cmd->running[slot] = NULL;
    cmd->work[slot].state = 0;
    cmd->work[slot].counter = 0;
}
