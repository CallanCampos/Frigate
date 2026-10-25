#include "sceneoptmovfailure.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "input.h"
#include "main.h"
#include "move.h"
#include "nitro/gx.h"
#include "sceneopterase.h"
#include "sceneoptmenu.h"
#include "sceneoptmovwait.h"
#include "sound.h"

// Whether the sub screen shows a message, and the player's answer
typedef struct DWCUtilNoticeScreen {
    u8 subScreen;
    u8 yes;
} DWCUtilNoticeScreen;

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

static DWCUtilNoticeScreen sNoticeScreen;

void DWCi_SceneOptMovFailure(void) {
    initGraphics();
    DWCi_CMNlMsgDraw(0x20);
    sNoticeScreen.subScreen = DWCi_COMMENTlDispEx(0x3c, -1, 0);
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
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlVisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    if (sNoticeScreen.subScreen) {
        DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        DWCi_GXlVisible(1, GX_PLANEMASK_BG0);
    }
    DWCi_ChangeScene(procInit1);
}

static void procInit1(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(3);
        DWCi_ChangeScene(procInit2);
    }
}

static void procInit2(void) {
    if (DWCi_BTNlGet() != -2 && DWCi_MOVlIsEnd()) {
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
    case 1:
        sNoticeScreen.yes = TRUE;
        DWCi_SNDlPlay(6);
        break;
    case 0:
        sNoticeScreen.yes = FALSE;
        DWCi_SNDlPlay(7);
        break;
    default:
        return;
    }
    DWCi_ChangeScene(endScene);
}

static void endScene(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_BTNlEnd();
        if (!sNoticeScreen.yes) {
            DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        }
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

static void endScene2(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0 && DWCi_BTNlIsEnd()) {
        DWCi_FNTlDeleteBgLcd(0);
        if (!sNoticeScreen.yes) {
            DWCi_COMMENTlDelete();
            DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
        }
        DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        if (!sNoticeScreen.yes) {
            DWCi_SetParam_(0, 1);
            DWCi_ChangeScene(DWCi_SceneOptMenu);
        } else {
            DWCi_SetParam_(0, 1);
            DWCi_ChangeScene(DWCi_SceneOptMovWait);
        }
    }
}
