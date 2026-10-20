#include "dwc_util_test_progress.h"
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
#include "dwc_util_test_connect.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_effect.h"
#include "dwc_util_ui_object.h"
#include "nitro/gx.h"
#include "nitro/os.h"

static BOOL DWCUtilTestProgress_IsLidClosed(void);
static void DWCUtilTestProgress_LoadGraphics(void);
static void DWCUtilTestProgress_FadeIn(void);
static void DWCUtilTestProgress_WaitFadeIn(void);
static void DWCUtilTestProgress_WaitButtons(void);
static void DWCUtilTestProgress_Main(void);
static void DWCUtilTestProgress_HandleInput(void);
static void func_ov182_021b46c4(void);
static void DWCUtilTestProgress_HandleChoice(void);
static void DWCUtilTestProgress_DisableButtons(void);
static void DWCUtilTestProgress_FadeOut(void);
static void DWCUtilTestProgress_WaitFadeOut(void);
static void DWCUtilTestProgress_PollTest(void);
static void DWCUtilTestProgress_WaitDialog(void);
static void DWCUtilTestProgress_WaitDialogClosed(void);

// Whether the test reached its second step; FALSE when it was ended or failed
static u8 sTestProgressPassed;

// A copy of gfl/ui.c's GCTX_HIDIsLidClosed
static BOOL DWCUtilTestProgress_IsLidClosed(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void DWCUtilTestProgress_Init(void) {
    sTestProgressPassed = FALSE;
    DWCUtilTestProgress_LoadGraphics();
    DWCUtilBgOverlay_ShowConnectionSet();
    DWCUtilGfxResource_ShowMessage(0x25);
    DWCUtilUiEffect_Init(1);
    DWCUtilMain_SetUpdate(DWCUtilTestProgress_FadeIn);
}

static void DWCUtilTestProgress_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilTestProgress_FadeIn(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilTestProgress_WaitFadeIn);
}

static void DWCUtilTestProgress_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(1);
        DWCUtilMain_SetUpdate(DWCUtilTestProgress_WaitButtons);
    }
}

static void DWCUtilTestProgress_WaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilTestProgress_Main);
    }
}

static void DWCUtilTestProgress_Main(void) {
    OS_Sleep(10);
    DWCUtilTestProgress_PollTest();
    DWCUtilTestProgress_HandleInput();
    func_ov182_021b46c4();
    DWCUtilTestProgress_HandleChoice();
}

static void DWCUtilTestProgress_HandleInput(void) {
    if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (DWCUtilTestProgress_IsLidClosed()) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b46c4(void) {
}

// The button ends the test
static void DWCUtilTestProgress_HandleChoice(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(7);
        DWCUtilMain_SetUpdate(DWCUtilTestProgress_DisableButtons);
    }
}

static void DWCUtilTestProgress_DisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilTestProgress_FadeOut);
}

static void DWCUtilTestProgress_FadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        if (!sTestProgressPassed) {
            DWCUtilUiObject_Exit();
        }
        if (!sTestProgressPassed) {
            DWCUtilFade_Start(3, 1, 1, 8);
        }
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(DWCUtilTestProgress_WaitFadeOut);
    }
}

static void DWCUtilTestProgress_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(0) == 0 && (sTestProgressPassed || DWCUtilFade_IsActive(1) == 0) &&
        DWCUtilUiObject_IsIdle()) {
        DWCUtilUiEffect_Destroy();
        DWCUtilText_DestroyScreenBg(0);
        if (!sTestProgressPassed) {
            DWCUtilScreen_Stop();
            DWCUtilDisplay_HidePlanes(1, 1);
        }
        DWCUtilDisplay_HidePlanes(0, 0x15);
        if (!sTestProgressPassed) {
            DWCUtilConnection_Stop();
            DWCUtilMain_SetLayout(2, 1);
            DWCUtilMain_SetUpdate(DWCUtilScreenWay_Init);
        } else {
            DWCUtilMain_SetLayout(0, 0);
            DWCUtilMain_SetUpdate(DWCUtilTestConnect_Init);
        }
    }
}

// What the test reports
static void DWCUtilTestProgress_PollTest(void) {
    switch (DWCUtilConnection_GetState()) {
    case 2:
        sTestProgressPassed = TRUE;
        DWCUtilMain_SetUpdate(DWCUtilTestProgress_DisableButtons);
        break;
    case 4:
        sTestProgressPassed = FALSE;
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(9);
        DWCUtilDialog_Open(0xd, 1, 1, -1, 0);
        DWCUtilUiObject_Disable();
        DWCUtilMain_SetUpdate(DWCUtilTestProgress_WaitDialog);
        break;
    }
}

static void DWCUtilTestProgress_WaitDialog(void) {
    if (DWCUtilDialog_GetChoice() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCUtilDialog_Close();
        DWCUtilMain_SetUpdate(DWCUtilTestProgress_WaitDialogClosed);
    }
}

static void DWCUtilTestProgress_WaitDialogClosed(void) {
    if (!DWCUtilDialog_IsOpen()) {
        DWCUtilMain_SetUpdate(DWCUtilTestProgress_DisableButtons);
    }
}
