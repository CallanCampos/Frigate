#include "sceneoptmovwait.h"
#include "anime.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "input.h"
#include "move.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "sceneoptmenu.h"
#include "sceneoptmovfailure.h"
#include "sceneoptmovuser.h"
#include "sound.h"

// How the wait ends: the player ends the move, a console has downloaded the move's program, or the move reported
// another event
enum {
    MOVE_WAIT_CANCELLED,
    MOVE_WAIT_CONNECTED,
    MOVE_WAIT_EVENT,
};

static BOOL DWCi_IsFold(void);
static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void DWCUtilPageMulti5_WaitButtons(void);
static void procScene(void);
static void DWCUtilPageMulti5_HandleInput(void);
static void draw(void);
static void DWCUtilPageMulti5_HandleChoice(void);
static void DWCUtilPageMulti5_DisableButtons(void);
static void DWCUtilPageMulti5_FadeOut(void);
static void DWCUtilPageMulti5_WaitFadeOut(void);
static void DWCUtilPageMulti5_OnMoveEvent(int event);

static u8 sMoveWaitEnd;

// Whether the lid is closed
static BOOL DWCi_IsFold(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void DWCi_SceneOptMovWait(void) {
    DWCi_MOVlInit(DWCUtilPageMulti5_OnMoveEvent);
    sMoveWaitEnd = MOVE_WAIT_CANCELLED;
    initGraphics();
    DWCi_CMNlMsgDraw(0x1c);
    DWCi_ANIMElInitEx(0);
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
        DWCi_CMNlButton(1);
        DWCi_ChangeScene(DWCUtilPageMulti5_WaitButtons);
    }
}

static void DWCUtilPageMulti5_WaitButtons(void) {
    if (DWCi_BTNlGet() != -2) {
        DWCi_ChangeScene(procScene);
    }
}

static void procScene(void) {
    DWCUtilPageMulti5_HandleInput();
    draw();
    DWCUtilPageMulti5_HandleChoice();
}

static void DWCUtilPageMulti5_HandleInput(void) {
    if (DWCi_IPTlCheckTrigger(2)) {
        DWCi_BTNlSet(0);
    } else if (DWCi_IsFold()) {
        DWCi_BTNlSet(0);
    }
}

static void draw(void) {
}

static void DWCUtilPageMulti5_HandleChoice(void) {
    if (DWCi_BTNlGet() == 0) {
        DWCi_SNDlStop();
        DWCi_SNDlPlay(7);
        DWCi_ChangeScene(DWCUtilPageMulti5_DisableButtons);
    }
}

static void DWCUtilPageMulti5_DisableButtons(void) {
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED) {
        DWCi_MOVlEnd();
    }
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(DWCUtilPageMulti5_FadeOut);
}

static void DWCUtilPageMulti5_FadeOut(void) {
    if (DWCi_EFFlCheck(1) != 0) {
        return;
    }
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED && !DWCi_MOVlIsEnd()) {
        return;
    }
    DWCi_BTNlEnd();
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED) {
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    }
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_ChangeScene(DWCUtilPageMulti5_WaitFadeOut);
}

static void DWCUtilPageMulti5_WaitFadeOut(void) {
    if (DWCi_EFFlCheck(0) != 0) {
        return;
    }
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED && DWCi_EFFlCheck(1) != 0) {
        return;
    }
    if (!DWCi_BTNlIsEnd()) {
        return;
    }
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED && !DWCi_MOVlIsEnd()) {
        return;
    }
    DWCi_ANIMElEnd();
    DWCi_FNTlDeleteBgLcd(0);
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED) {
        DWCi_COMMENTlDelete();
        DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    }
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED) {
        DWCi_SetParam_(0, 1);
        DWCi_ChangeScene(DWCi_SceneOptMenu);
    } else if (sMoveWaitEnd == MOVE_WAIT_EVENT) {
        DWCi_SetParam_(0, 1);
        DWCi_ChangeScene(DWCi_SceneOptMovFailure);
    } else {
        DWCi_SetParam_(0, 1);
        DWCi_ChangeScene(DWCi_SceneOptMovUser);
    }
}

static void DWCUtilPageMulti5_OnMoveEvent(int event) {
    DWCi_SNDlStop();
    if (event == 0) {
        sMoveWaitEnd = MOVE_WAIT_CONNECTED;
        DWCi_SNDlPlay(0x10);
    } else {
        sMoveWaitEnd = MOVE_WAIT_EVENT;
        DWCi_SNDlPlay(0x12);
    }
    DWCi_MOVlChangeCallback(NULL);
    DWCi_ChangeScene(DWCUtilPageMulti5_DisableButtons);
}
