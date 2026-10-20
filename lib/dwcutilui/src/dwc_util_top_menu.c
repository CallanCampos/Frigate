#include "dwc_util_top_menu.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_link_icon.h"
#include "dwc_util_main.h"
#include "dwc_util_move.h"
#include "dwc_util_notice_screen.h"
#include "dwc_util_page_multi5.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_text.h"
#include "dwc_util_timed_notice.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_effect.h"
#include "dwc_util_ui_object.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nitro/pm.h"

static void DWCUtilTopMenu_LoadMoveGraphics(void);
static void DWCUtilTopMenu_MoveFadeIn(void);
static void DWCUtilTopMenu_MoveWaitFadeIn(void);
static void DWCUtilTopMenu_MoveMain(void);
static void func_ov182_021af7e4(void);
static void func_ov182_021af7e8(void);
static void DWCUtilTopMenu_MoveFadeOut(void);
static void DWCUtilTopMenu_MoveWaitFadeOut(void);
static void DWCUtilTopMenu_OnMoveEvent(int event);
static void DWCUtilTopMenu_InitSent(void);
static void DWCUtilTopMenu_SetSentPriorities(void);
static void DWCUtilTopMenu_SentFadeIn(void);
static void DWCUtilTopMenu_SentWaitFadeIn(void);
static void DWCUtilTopMenu_SentWaitButtons(void);
static void DWCUtilTopMenu_SentMain(void);
static void DWCUtilTopMenu_SentHandleInput(void);
static void func_ov182_021af998(void);
static void DWCUtilTopMenu_SentHandleChoice(void);
static void DWCUtilTopMenu_SentDisableButtons(void);
static void DWCUtilTopMenu_SentFadeOut(void);
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
    DWCUtilGfxResource_ShowMessage(0x1e);
    DWCUtilUiEffect_Init(0);
    DWCUtilLinkIcon_Init(1);
    DWCUtilSound_PlaySe(0xb);
    DWCUtilMain_SetUpdate(DWCUtilTopMenu_MoveFadeIn);
}

static void DWCUtilTopMenu_LoadMoveGraphics(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilTopMenu_MoveFadeIn(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilTopMenu_MoveWaitFadeIn);
}

static void DWCUtilTopMenu_MoveWaitFadeIn(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilMove_SetCallback(DWCUtilTopMenu_OnMoveEvent);
        DWCUtilMain_SetUpdate(DWCUtilTopMenu_MoveMain);
    }
}

static void DWCUtilTopMenu_MoveMain(void) {
    func_ov182_021af7e4();
    func_ov182_021af7e8();
}

static void func_ov182_021af7e4(void) {
}

static void func_ov182_021af7e8(void) {
}

static void DWCUtilTopMenu_MoveFadeOut(void) {
    DWCUtilFade_Start(3, 0, 0x15, 8);
    DWCUtilMain_SetUpdate(DWCUtilTopMenu_MoveWaitFadeOut);
}

static void DWCUtilTopMenu_MoveWaitFadeOut(void) {
    if (DWCUtilFade_IsActive(0) == 0 && DWCUtilMove_HasEnded()) {
        DWCUtilLinkIcon_Exit();
        DWCUtilUiEffect_Destroy();
        DWCUtilText_DestroyScreenBg(0);
        DWCUtilDisplay_HidePlanes(0, 0x15);
        DWCUtilMain_SetLayout(0, 1);
        if (sMoveResult == 0) {
            DWCUtilMain_SetUpdate(DWCUtilNoticeScreen_Init);
        } else if (sMoveResult == 2) {
            DWCUtilMain_SetUpdate(DWCUtilTimedNotice_Init);
        } else {
            DWCUtilMain_SetUpdate(DWCUtilTopMenu_InitSent);
        }
    }
}

// The move's callback: once the settings are sent they are erased here
static void DWCUtilTopMenu_OnMoveEvent(int event) {
    if (event == DWC_UTIL_MOVE_EVENT_DONE) {
        DWCUtilWifiSettings_EraseAll();
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
    DWCUtilMove_SetCallback(NULL);
    DWCUtilMove_End();
    DWCUtilMain_SetUpdate(DWCUtilTopMenu_MoveFadeOut);
}

// The settings were sent: the player is told to turn the console off
static void DWCUtilTopMenu_InitSent(void) {
    DWCUtilTopMenu_SetSentPriorities();
    DWCUtilGfxResource_ShowMessage(0x1f);
    DWCUtilMain_SetUpdate(DWCUtilTopMenu_SentFadeIn);
}

static void DWCUtilTopMenu_SetSentPriorities(void) {
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilTopMenu_SentFadeIn(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilTopMenu_SentWaitFadeIn);
}

static void DWCUtilTopMenu_SentWaitFadeIn(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(5);
        DWCUtilMain_SetUpdate(DWCUtilTopMenu_SentWaitButtons);
    }
}

static void DWCUtilTopMenu_SentWaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilTopMenu_SentMain);
    }
}

static void DWCUtilTopMenu_SentMain(void) {
    DWCUtilTopMenu_SentHandleInput();
    func_ov182_021af998();
    DWCUtilTopMenu_SentHandleChoice();
}

static void DWCUtilTopMenu_SentHandleInput(void) {
    if (DWCUtilTouch_IsKeyPressed(1)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021af998(void) {
}

static void DWCUtilTopMenu_SentHandleChoice(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCUtilMain_SetUpdate(DWCUtilTopMenu_SentDisableButtons);
    }
}

static void DWCUtilTopMenu_SentDisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilTopMenu_SentFadeOut);
}

static void DWCUtilTopMenu_SentFadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilFade_Start(3, 1, 0x3f, 0x40);
        DWCUtilFade_Start(3, 0, 0x3f, 0x40);
        DWCUtilMain_SetUpdate(DWCUtilTopMenu_PowerOff);
    }
}

// Turns the console off once both screens have faded out
static void DWCUtilTopMenu_PowerOff(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        OS_SpinWaitSysCycles(0x800000);
        PM_ForceToPowerOff();
    }
}

void DWCUtilTopMenu_InitInstructions(void) {
    DWCUtilTopMenu_SetInstructionsPriorities();
    DWCUtilGfxResource_ShowMessage(0x1a);
    DWCUtilMain_SetUpdate(DWCUtilTopMenu_InstructionsFadeIn);
}

static void DWCUtilTopMenu_SetInstructionsPriorities(void) {
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilTopMenu_InstructionsFadeIn(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilTopMenu_InstructionsWaitFadeIn);
}

static void DWCUtilTopMenu_InstructionsWaitFadeIn(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(4);
        DWCUtilMain_SetUpdate(DWCUtilTopMenu_InstructionsWaitButtons);
    }
}

static void DWCUtilTopMenu_InstructionsWaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilTopMenu_InstructionsMain);
    }
}

static void DWCUtilTopMenu_InstructionsMain(void) {
    DWCUtilTopMenu_InstructionsHandleInput();
    func_ov182_021afb0c();
    DWCUtilTopMenu_InstructionsHandleChoice();
}

static void DWCUtilTopMenu_InstructionsHandleInput(void) {
    if (DWCUtilTouch_IsKeyPressed(1)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021afb0c(void) {
}

static void DWCUtilTopMenu_InstructionsHandleChoice(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCUtilMain_SetUpdate(DWCUtilTopMenu_InstructionsDisableButtons);
    }
}

static void DWCUtilTopMenu_InstructionsDisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilTopMenu_InstructionsFadeOut);
}

static void DWCUtilTopMenu_InstructionsFadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(DWCUtilTopMenu_InstructionsWaitFadeOut);
    }
}

static void DWCUtilTopMenu_InstructionsWaitFadeOut(void) {
    if (DWCUtilFade_IsActive(0) == 0 && DWCUtilUiObject_IsIdle()) {
        DWCUtilText_DestroyScreenBg(0);
        DWCUtilDisplay_HidePlanes(0, 0x15);
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(DWCUtilPageMulti5_Init);
    }
}
