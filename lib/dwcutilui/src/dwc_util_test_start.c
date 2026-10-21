#include "dwc_util_test_start.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_connection.h"
#include "dwc_util_dialog.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_way.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_test_progress.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_effect.h"
#include "dwc_util_ui_object.h"
#include "nitro/gx.h"
#include "nitro/os.h"

static BOOL DWCUtilTestStart_IsLidClosed(void);
static void DWCUtilTestStart_LoadGraphics(void);
static void DWCUtilTestStart_FadeIn(void);
static void DWCUtilTestStart_WaitFadeIn(void);
static void DWCUtilTestStart_WaitButtons(void);
static void DWCUtilTestStart_Main(void);
static void DWCUtilTestStart_HandleInput(void);
static void func_ov182_021b4e60(void);
static void DWCUtilTestStart_HandleChoice(void);
static void DWCUtilTestStart_DisableButtons(void);
static void DWCUtilTestStart_FadeOut(void);
static void DWCUtilTestStart_WaitFadeOut(void);
static void DWCUtilTestStart_PollTest(void);
static void DWCUtilTestStart_WaitDialog(void);
static void DWCUtilTestStart_WaitDialogClosed(void);

// Whether the test reached its first step; FALSE when it was ended or failed
static u8 sTestStartPassed;

// A copy of gfl/ui.c's GCTX_HIDIsLidClosed
static BOOL DWCUtilTestStart_IsLidClosed(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void DWCUtilTestStart_Init(void) {
    sTestStartPassed = FALSE;
    DWCUtilTestStart_LoadGraphics();
    DWCUtilBgOverlay_ShowConnectionSet();
    DWCUtilScreen_Start(0x37, -1, 0);
    DWCUtilGfxResource_ShowStep(2);
    DWCUtilGfxResource_ShowMessage(0x24);
    DWCUtilUiEffect_Init(0);
    DWCUtilSound_PlaySe(0xb);
    DWCUtilMain_SetUpdate(DWCUtilTestStart_FadeIn);
}

static void DWCUtilTestStart_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilTestStart_FadeIn(void) {
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCUtilDisplay_ShowPlanes(1, GX_PLANEMASK_BG0);
    DWCUtilDisplay_ShowPlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCUtilMain_SetUpdate(DWCUtilTestStart_WaitFadeIn);
}

// Starts the test once the screens have faded in
static void DWCUtilTestStart_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilConnection_Start();
        DWCUtilGfxResource_ShowButtons(1);
        DWCUtilMain_SetUpdate(DWCUtilTestStart_WaitButtons);
    }
}

static void DWCUtilTestStart_WaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilTestStart_Main);
    }
}

static void DWCUtilTestStart_Main(void) {
    OS_Sleep(10);
    DWCUtilTestStart_PollTest();
    DWCUtilTestStart_HandleInput();
    func_ov182_021b4e60();
    DWCUtilTestStart_HandleChoice();
}

static void DWCUtilTestStart_HandleInput(void) {
    if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (DWCUtilTestStart_IsLidClosed()) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b4e60(void) {
}

// The button ends the test
static void DWCUtilTestStart_HandleChoice(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(7);
        DWCUtilMain_SetUpdate(DWCUtilTestStart_DisableButtons);
    }
}

static void DWCUtilTestStart_DisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilTestStart_FadeOut);
}

static void DWCUtilTestStart_FadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        if (!sTestStartPassed) {
            DWCUtilUiObject_Exit();
        }
        if (!sTestStartPassed) {
            DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        }
        DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCUtilMain_SetUpdate(DWCUtilTestStart_WaitFadeOut);
    }
}

static void DWCUtilTestStart_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(0) == 0 && (sTestStartPassed || DWCUtilFade_IsActive(1) == 0) &&
        DWCUtilUiObject_IsIdle()) {
        DWCUtilUiEffect_Destroy();
        DWCUtilText_DestroyScreenBg(0);
        if (!sTestStartPassed) {
            DWCUtilScreen_Stop();
            DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        }
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        if (!sTestStartPassed) {
            DWCUtilConnection_Stop();
            DWCUtilMain_SetLayout(2, 1);
            DWCUtilMain_SetUpdate(DWCUtilScreenWay_Init);
        } else {
            DWCUtilMain_SetLayout(0, 0);
            DWCUtilMain_SetUpdate(DWCUtilTestProgress_Init);
        }
    }
}

// What the test reports
static void DWCUtilTestStart_PollTest(void) {
    switch (DWCUtilConnection_GetState()) {
    case 1:
        sTestStartPassed = TRUE;
        DWCUtilMain_SetUpdate(DWCUtilTestStart_DisableButtons);
        break;
    case 4:
        sTestStartPassed = FALSE;
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(9);
        DWCUtilDialog_Open(0xd, 1, 1, -1, 0);
        DWCUtilUiObject_Disable();
        DWCUtilMain_SetUpdate(DWCUtilTestStart_WaitDialog);
        break;
    }
}

static void DWCUtilTestStart_WaitDialog(void) {
    if (DWCUtilDialog_GetChoice() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCUtilDialog_Close();
        DWCUtilMain_SetUpdate(DWCUtilTestStart_WaitDialogClosed);
    }
}

static void DWCUtilTestStart_WaitDialogClosed(void) {
    if (!DWCUtilDialog_IsOpen()) {
        DWCUtilMain_SetUpdate(DWCUtilTestStart_DisableButtons);
    }
}
