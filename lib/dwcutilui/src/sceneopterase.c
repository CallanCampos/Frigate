#include "sceneopterase.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "headline.h"
#include "input.h"
#include "main.h"
#include "nitro/gx.h"
#include "sceneopterase1.h"
#include "sceneoptmenu.h"
#include "sound.h"
#include "window.h"

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
static void procConfirm(void);
static void procConfirm1(void);

// Whether the player confirmed the erasing
static u8 sErase;

void DWCi_SceneOptErase(void) {
    sErase = FALSE;
    initGraphics();
    DWCi_HLlLoad(0x12);
    DWCi_COMMENTlDispEx(0x3b, -1, 0);
    DWCi_CMNlMsgDraw(0x17);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCi_CMNlLoadResource("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
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

static void procInit1(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(3);
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
    draw();
    button();
}

static void input(void) {
    DWCUtilInfoMenu_EraseHandleInputInline();
}

static void draw(void) {
}

static void button(void) {
    switch (DWCi_BTNlGet()) {
    case 0:
        DWCi_SNDlPlay(7);
        DWCi_ChangeScene(endScene);
        break;
    case 1:
        DWCi_SNDlPlay(6);
        DWCi_WINlInitEx(0x18, 0, 1, -1, 0);
        DWCi_BTNlDisable();
        DWCi_ChangeScene(procConfirm);
        break;
    }
}

static void endScene(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_BTNlEnd();
        if (!sErase) {
            DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        }
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

static void endScene2(void) {
    if (DWCi_EFFlCheck(0) == 0 && (sErase || DWCi_EFFlCheck(1) == 0) && DWCi_BTNlIsEnd()) {
        DWCi_FNTlDeleteBgLcd(0);
        if (!sErase) {
            DWCi_COMMENTlDelete();
            DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
        }
        DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        if (!sErase) {
            DWCi_SetParam_(0, 1);
            DWCi_ChangeScene(DWCi_SceneOptMenu);
        } else {
            DWCi_SetParam_(0, 1);
            DWCi_ChangeScene(DWCUtilInfoMenu_EraseSettings);
        }
    }
}

// The confirmation: 1 erases the settings
static void procConfirm(void) {
    switch (DWCi_WINlGet()) {
    case 1:
        DWCi_SNDlPlay(0xe);
        sErase = TRUE;
        break;
    case 0:
        DWCi_SNDlPlay(7);
        break;
    default:
        return;
    }
    DWCi_WINlEnd();
    DWCi_ChangeScene(procConfirm1);
}

static void procConfirm1(void) {
    if (!DWCi_WINlIsInit()) {
        DWCi_ChangeScene(endScene);
    }
}
