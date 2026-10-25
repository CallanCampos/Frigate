#include "scenesetrakuready.h"
#include "anime.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "headline.h"
#include "input.h"
#include "main.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "rakuraku.h"
#include "scenesetrakubutton.h"
#include "scenesetselectway.h"
#include "sound.h"
#include "window.h"

static BOOL DWCi_IsFold(void);
static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static void draw(void);
static void button(void);
static void endScene(void);
static void endScene1(void);
static void endScene2(void);
static void checkRaku(void);
static void procError(void);
static void procError1(void);

// Whether the test reached its first step; FALSE when it was ended or failed
static u8 sTestStartPassed;

// A copy of gfl/ui.c's GCTX_HIDIsLidClosed
static BOOL DWCi_IsFold(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void DWCUtilTestStart_Init(void) {
    sTestStartPassed = FALSE;
    initGraphics();
    DWCi_HLlLoadSet();
    DWCi_COMMENTlDispEx(0x37, -1, 0);
    DWCi_CMNlStep(2);
    DWCi_CMNlMsgDraw(0x24);
    DWCi_ANIMElInitEx(0);
    DWCi_SNDlPlay(0xb);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCi_CMNlLoadResource("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    DWCi_CMNlLoadResource("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    DWCi_CMNlLoadResource("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlVisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlVisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(procInit1);
}

// Starts the test once the screens have faded in
static void procInit1(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_RAKUlInit();
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
    checkRaku();
    input();
    draw();
    button();
}

static void input(void) {
    if (DWCi_IPTlCheckTrigger(2)) {
        DWCi_BTNlSet(0);
    } else if (DWCi_IsFold()) {
        DWCi_BTNlSet(0);
    }
}

static void draw(void) {
}

// The button ends the test
static void button(void) {
    if (DWCi_BTNlGet() == 0) {
        DWCi_SNDlStop();
        DWCi_SNDlPlay(7);
        DWCi_ChangeScene(endScene);
    }
}

static void endScene(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        if (!sTestStartPassed) {
            DWCi_BTNlEnd();
        }
        if (!sTestStartPassed) {
            DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        }
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

static void endScene2(void) {
    if (DWCi_EFFlCheck(0) == 0 && (sTestStartPassed || DWCi_EFFlCheck(1) == 0) && DWCi_BTNlIsEnd()) {
        DWCi_ANIMElEnd();
        DWCi_FNTlDeleteBgLcd(0);
        if (!sTestStartPassed) {
            DWCi_COMMENTlDelete();
            DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
        }
        DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        if (!sTestStartPassed) {
            DWCi_RAKUlEnd();
            DWCi_SetParam_(2, 1);
            DWCi_ChangeScene(DWCi_SceneSetSelectWay);
        } else {
            DWCi_SetParam_(0, 0);
            DWCi_ChangeScene(DWCi_SceneSetRakuButton);
        }
    }
}

// What the test reports
static void checkRaku(void) {
    switch (DWCi_RAKUlGetState()) {
    case 1:
        sTestStartPassed = TRUE;
        DWCi_ChangeScene(endScene);
        break;
    case 4:
        sTestStartPassed = FALSE;
        DWCi_SNDlStop();
        DWCi_SNDlPlay(9);
        DWCi_WINlInitEx(0xd, 1, 1, -1, 0);
        DWCi_BTNlDisable();
        DWCi_ChangeScene(procError);
        break;
    }
}

static void procError(void) {
    if (DWCi_WINlGet() == 0) {
        DWCi_SNDlPlay(6);
        DWCi_WINlEnd();
        DWCi_ChangeScene(procError1);
    }
}

static void procError1(void) {
    if (!DWCi_WINlIsInit()) {
        DWCi_ChangeScene(endScene);
    }
}
