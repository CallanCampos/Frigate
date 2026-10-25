#include "sceneoptmovcomm.h"
#include "anime.h"
#include "common.h"
#include "dwc_util_main.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "icon.h"
#include "move.h"
#include "nitro/gx.h"
#include "sceneoptmovcomplete.h"
#include "sceneoptmovdisconnect.h"
#include "sceneoptmovfailure.h"
#include "setting.h"
#include "sound.h"
#include "task.h"

static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void DWCUtilTopMenu_MoveMain(void);
static void func_ov182_021af7e4(void);
static void draw(void);
static void DWCUtilTopMenu_MoveFadeOut(void);
static void DWCUtilTopMenu_MoveWaitFadeOut(void);
static void DWCUtilTopMenu_OnMoveEvent(int event);

// How the move went: 0 when it failed, 1 once it sent the settings, 2 when it ended without sending them
static u8 sMoveResult;

void DWCi_SceneOptMovComm(void) {
    initGraphics();
    DWCi_CMNlMsgDraw(0x1e);
    DWCi_ANIMElInitEx(0);
    DWCi_ICONlInit(1);
    DWCi_SNDlPlay(0xb);
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
        DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCi_SetParam_(0, 1);
        if (sMoveResult == 0) {
            DWCi_ChangeScene(DWCi_SceneOptMovFailure);
        } else if (sMoveResult == 2) {
            DWCi_ChangeScene(DWCi_SceneOptMovDisconnect);
        } else {
            DWCi_ChangeScene(DWCi_SceneOptMovComplete);
        }
    }
}

// The move's callback: once the settings are sent they are erased here
static void DWCUtilTopMenu_OnMoveEvent(int event) {
    if (event == DWC_UTIL_MOVE_EVENT_DONE) {
        DWCi_SETTINGlEraseAll();
        sMoveResult = 1;
        DWCi_SNDlStop();
        DWCi_SNDlPlay(0x10);
    } else {
        if (event == DWC_UTIL_MOVE_EVENT_ENDED) {
            sMoveResult = 2;
        } else {
            sMoveResult = 0;
        }
        DWCi_SNDlStop();
        DWCi_SNDlPlay(0x12);
    }
    DWCi_MOVlChangeCallback(NULL);
    DWCi_MOVlEnd();
    DWCi_ChangeScene(DWCUtilTopMenu_MoveFadeOut);
}
