#include "dwc_util_top_menu.h"
#include "anime.h"
#include "button.h"
#include "common.h"
#include "dwc_util_display.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "icon.h"
#include "input.h"
#include "move.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "sceneoptmovdisconnect.h"
#include "sceneoptmovfailure.h"
#include "sceneoptmovwait.h"
#include "setting.h"
#include "sound.h"

static void DWCUtilTopMenu_LoadMoveGraphics(void);
static void procInit(void);
static void procInit1(void);
static void DWCUtilTopMenu_MoveMain(void);
static void func_ov182_021af7e4(void);
static void draw(void);
static void DWCUtilTopMenu_MoveFadeOut(void);
static void DWCUtilTopMenu_MoveWaitFadeOut(void);
static void DWCUtilTopMenu_OnMoveEvent(int event);
static void DWCi_SceneOptMovConfirm(void);
static void initGraphics(void);
static void DWCUtilTopMenu_SentFadeIn(void);
static void DWCUtilTopMenu_SentWaitFadeIn(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static void func_ov182_021af998(void);
static void button(void);
static void DWCUtilTopMenu_SentDisableButtons(void);
static void endScene1(void);
static void DWCUtilTopMenu_PowerOff(void);
static void DWCUtilTopMenu_SetInstructionsPriorities(void);
static void DWCUtilTopMenu_InstructionsFadeIn(void);
static void DWCUtilTopMenu_InstructionsWaitFadeIn(void);
static void DWCUtilTopMenu_InstructionsWaitButtons(void);
static void DWCUtilTopMenu_InstructionsMain(void);
static void DWCUtilTopMenu_InstructionsHandleInput(void);
static void func_ov182_021afb0c(void);
static void DWCUtilTopMenu_InstructionsHandleChoice(void);
static void DWCUtilTopMenu_InstructionsDisableButtons(void);
static void DWCUtilTopMenu_InstructionsFadeOut(void);
static void DWCUtilTopMenu_InstructionsWaitFadeOut(void);

// How the move went: 0 when it failed, 1 once it sent the settings, 2 when it ended without sending them
static u8 sMoveResult;

void DWCUtilTopMenu_InitMove(void) {
    DWCUtilTopMenu_LoadMoveGraphics();
    DWCi_CMNlMsgDraw(0x1e);
    DWCi_ANIMElInitEx(0);
    DWCUtilLinkIcon_Init(1);
    DWCUtilSound_PlaySe(0xb);
    DWCi_ChangeScene(procInit);
}

static void DWCUtilTopMenu_LoadMoveGraphics(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(procInit1);
}

static void procInit1(void) {
    if (DWCi_EFFlCheck(0) == 0) {
        DWCi_MOVlChangeCallback(DWCUtilTopMenu_OnMoveEvent);
        DWCi_ChangeScene(DWCUtilTopMenu_MoveMain);
    }
}

static void DWCUtilTopMenu_MoveMain(void) {
    func_ov182_021af7e4();
    draw();
}

static void func_ov182_021af7e4(void) {
}

static void draw(void) {
}

static void DWCUtilTopMenu_MoveFadeOut(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_ChangeScene(DWCUtilTopMenu_MoveWaitFadeOut);
}

static void DWCUtilTopMenu_MoveWaitFadeOut(void) {
    if (DWCi_EFFlCheck(0) == 0 && DWCi_MOVlIsEnd()) {
        DWCi_ICONlEnd();
        DWCi_ANIMElEnd();
        DWCi_FNTlDeleteBgLcd(0);
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCi_SetParam_(0, 1);
        if (sMoveResult == 0) {
            DWCi_ChangeScene(DWCUtilNoticeScreen_Init);
        } else if (sMoveResult == 2) {
            DWCi_ChangeScene(DWCUtilTimedNotice_Init);
        } else {
            DWCi_ChangeScene(DWCi_SceneOptMovConfirm);
        }
    }
}

// The move's callback: once the settings are sent they are erased here
static void DWCUtilTopMenu_OnMoveEvent(int event) {
    if (event == DWC_UTIL_MOVE_EVENT_DONE) {
        DWCi_SETTINGlEraseAll();
        sMoveResult = 1;
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(0x10);
    } else {
        if (event == DWC_UTIL_MOVE_EVENT_ENDED) {
            sMoveResult = 2;
        } else {
            sMoveResult = 0;
        }
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(0x12);
    }
    DWCi_MOVlChangeCallback(NULL);
    DWCUtilMove_End();
    DWCi_ChangeScene(DWCUtilTopMenu_MoveFadeOut);
}

// The settings were sent: the player is told to turn the console off
static void DWCi_SceneOptMovConfirm(void) {
    initGraphics();
    DWCi_CMNlMsgDraw(0x1f);
    DWCi_ChangeScene(DWCUtilTopMenu_SentFadeIn);
}

static void initGraphics(void) {
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilTopMenu_SentFadeIn(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(DWCUtilTopMenu_SentWaitFadeIn);
}

static void DWCUtilTopMenu_SentWaitFadeIn(void) {
    if (DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(5);
        DWCi_ChangeScene(procInit2);
    }
}

static void procInit2(void) {
    if (DWCi_BTNlGet() != -2) {
        DWCi_BTNlEnable();
        DWCi_ChangeScene(procScene);
    }
}

static void procScene(void) {
    input();
    func_ov182_021af998();
    button();
}

static void input(void) {
    if (DWCi_IPTlCheckTrigger(1)) {
        DWCi_BTNlSet(0);
    }
}

static void func_ov182_021af998(void) {
}

static void button(void) {
    if (DWCi_BTNlGet() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCi_ChangeScene(DWCUtilTopMenu_SentDisableButtons);
    }
}

static void DWCUtilTopMenu_SentDisableButtons(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, DWC_UTIL_FADE_PLANES, 0x40);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, DWC_UTIL_FADE_PLANES, 0x40);
        DWCi_ChangeScene(DWCUtilTopMenu_PowerOff);
    }
}

// Turns the console off once both screens have faded out
static void DWCUtilTopMenu_PowerOff(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        OS_SpinWaitSysCycles(0x800000);
        PM_ForceToPowerOff();
    }
}

void DWCUtilTopMenu_InitInstructions(void) {
    DWCUtilTopMenu_SetInstructionsPriorities();
    DWCi_CMNlMsgDraw(0x1a);
    DWCi_ChangeScene(DWCUtilTopMenu_InstructionsFadeIn);
}

static void DWCUtilTopMenu_SetInstructionsPriorities(void) {
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilTopMenu_InstructionsFadeIn(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(DWCUtilTopMenu_InstructionsWaitFadeIn);
}

static void DWCUtilTopMenu_InstructionsWaitFadeIn(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(4);
        DWCi_ChangeScene(DWCUtilTopMenu_InstructionsWaitButtons);
    }
}

static void DWCUtilTopMenu_InstructionsWaitButtons(void) {
    if (DWCi_BTNlGet() != -2) {
        DWCi_BTNlEnable();
        DWCi_ChangeScene(DWCUtilTopMenu_InstructionsMain);
    }
}

static void DWCUtilTopMenu_InstructionsMain(void) {
    DWCUtilTopMenu_InstructionsHandleInput();
    func_ov182_021afb0c();
    DWCUtilTopMenu_InstructionsHandleChoice();
}

static void DWCUtilTopMenu_InstructionsHandleInput(void) {
    if (DWCi_IPTlCheckTrigger(1)) {
        DWCi_BTNlSet(0);
    }
}

static void func_ov182_021afb0c(void) {
}

static void DWCUtilTopMenu_InstructionsHandleChoice(void) {
    if (DWCi_BTNlGet() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCi_ChangeScene(DWCUtilTopMenu_InstructionsDisableButtons);
    }
}

static void DWCUtilTopMenu_InstructionsDisableButtons(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(DWCUtilTopMenu_InstructionsFadeOut);
}

static void DWCUtilTopMenu_InstructionsFadeOut(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_BTNlEnd();
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(DWCUtilTopMenu_InstructionsWaitFadeOut);
    }
}

static void DWCUtilTopMenu_InstructionsWaitFadeOut(void) {
    if (DWCi_EFFlCheck(0) == 0 && DWCi_BTNlIsEnd()) {
        DWCi_FNTlDeleteBgLcd(0);
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCi_SetParam_(0, 1);
        DWCi_ChangeScene(DWCUtilPageMulti5_Init);
    }
}
