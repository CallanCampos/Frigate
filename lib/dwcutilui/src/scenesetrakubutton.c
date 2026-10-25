#include "scenesetrakubutton.h"
#include "anime.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "headline.h"
#include "input.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "rakuraku.h"
#include "scenesetrakubutton1.h"
#include "scenesetselectway.h"
#include "sound.h"
#include "window.h"

static BOOL DWCUtilTestProgress_IsLidClosed(void);
static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void DWCUtilTestProgress_HandleInput(void);
static void draw(void);
static void button(void);
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
    initGraphics();
    DWCi_HLlLoadSet();
    DWCi_CMNlMsgDraw(0x25);
    DWCi_ANIMElInitEx(1);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCUtilGfxResource_Load("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
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
        DWCi_CMNlButton(1);
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
    OS_Sleep(10);
    DWCUtilTestProgress_PollTest();
    DWCUtilTestProgress_HandleInput();
    draw();
    button();
}

static void DWCUtilTestProgress_HandleInput(void) {
    if (DWCi_IPTlCheckTrigger(2)) {
        DWCi_BTNlSet(0);
    } else if (DWCUtilTestProgress_IsLidClosed()) {
        DWCi_BTNlSet(0);
    }
}

static void draw(void) {
}

// The button ends the test
static void button(void) {
    if (DWCi_BTNlGet() == 0) {
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(7);
        DWCi_ChangeScene(DWCUtilTestProgress_DisableButtons);
    }
}

static void DWCUtilTestProgress_DisableButtons(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(DWCUtilTestProgress_FadeOut);
}

static void DWCUtilTestProgress_FadeOut(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        if (!sTestProgressPassed) {
            DWCi_BTNlEnd();
        }
        if (!sTestProgressPassed) {
            DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        }
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(DWCUtilTestProgress_WaitFadeOut);
    }
}

static void DWCUtilTestProgress_WaitFadeOut(void) {
    if (DWCi_EFFlCheck(0) == 0 && (sTestProgressPassed || DWCi_EFFlCheck(1) == 0) && DWCi_BTNlIsEnd()) {
        DWCi_ANIMElEnd();
        DWCi_FNTlDeleteBgLcd(0);
        if (!sTestProgressPassed) {
            DWCi_COMMENTlDelete();
            DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        }
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        if (!sTestProgressPassed) {
            DWCi_RAKUlEnd();
            DWCi_SetParam_(2, 1);
            DWCi_ChangeScene(DWCUtilScreenWay_Init);
        } else {
            DWCi_SetParam_(0, 0);
            DWCi_ChangeScene(DWCUtilTestConnect_Init);
        }
    }
}

// What the test reports
static void DWCUtilTestProgress_PollTest(void) {
    switch (DWCi_RAKUlGetState()) {
    case 2:
        sTestProgressPassed = TRUE;
        DWCi_ChangeScene(DWCUtilTestProgress_DisableButtons);
        break;
    case 4:
        sTestProgressPassed = FALSE;
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(9);
        DWCUtilDialog_Open(0xd, 1, 1, -1, 0);
        DWCi_BTNlDisable();
        DWCi_ChangeScene(DWCUtilTestProgress_WaitDialog);
        break;
    }
}

static void DWCUtilTestProgress_WaitDialog(void) {
    if (DWCi_WINlGet() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCi_WINlEnd();
        DWCi_ChangeScene(DWCUtilTestProgress_WaitDialogClosed);
    }
}

static void DWCUtilTestProgress_WaitDialogClosed(void) {
    if (!DWCi_WINlIsInit()) {
        DWCi_ChangeScene(DWCUtilTestProgress_DisableButtons);
    }
}
