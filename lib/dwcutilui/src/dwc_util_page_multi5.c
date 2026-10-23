#include "dwc_util_page_multi5.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_move.h"
#include "dwc_util_notice_screen.h"
#include "dwc_util_option_menu.h"
#include "dwc_util_page_move.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_effect.h"
#include "dwc_util_ui_object.h"
#include "nitro/gx.h"
#include "nitro/os.h"

// How the wait ends: the player ends the move, a console has downloaded the move's program, or the move reported
// another event
enum {
    MOVE_WAIT_CANCELLED,
    MOVE_WAIT_CONNECTED,
    MOVE_WAIT_EVENT,
};

static BOOL DWCUtilPageMulti5_IsLidClosed(void);
static void DWCUtilPageMulti5_LoadGraphics(void);
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
static BOOL DWCUtilPageMulti5_IsLidClosed(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void DWCUtilPageMulti5_Init(void) {
    DWCi_MOVlInit(DWCUtilPageMulti5_OnMoveEvent);
    sMoveWaitEnd = MOVE_WAIT_CANCELLED;
    DWCUtilPageMulti5_LoadGraphics();
    DWCi_CMNlMsgDraw(0x1c);
    DWCi_ANIMElInitEx(0);
    DWCUtilSound_PlaySe(0xb);
    DWCi_ChangeScene(procInit);
}

static void DWCUtilPageMulti5_LoadGraphics(void) {
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
    } else if (DWCUtilPageMulti5_IsLidClosed()) {
        DWCi_BTNlSet(0);
    }
}

static void draw(void) {
}

static void DWCUtilPageMulti5_HandleChoice(void) {
    if (DWCi_BTNlGet() == 0) {
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(7);
        DWCi_ChangeScene(DWCUtilPageMulti5_DisableButtons);
    }
}

static void DWCUtilPageMulti5_DisableButtons(void) {
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED) {
        DWCUtilMove_End();
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
        DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
    }
    DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED) {
        DWCi_SetParam_(0, 1);
        DWCi_ChangeScene(DWCUtilOptionMenu_Init);
    } else if (sMoveWaitEnd == MOVE_WAIT_EVENT) {
        DWCi_SetParam_(0, 1);
        DWCi_ChangeScene(DWCUtilNoticeScreen_Init);
    } else {
        DWCi_SetParam_(0, 1);
        DWCi_ChangeScene(DWCUtilPageMove_Init);
    }
}

static void DWCUtilPageMulti5_OnMoveEvent(int event) {
    DWCUtilSound_StopSe();
    if (event == 0) {
        sMoveWaitEnd = MOVE_WAIT_CONNECTED;
        DWCUtilSound_PlaySe(0x10);
    } else {
        sMoveWaitEnd = MOVE_WAIT_EVENT;
        DWCUtilSound_PlaySe(0x12);
    }
    DWCi_MOVlChangeCallback(NULL);
    DWCi_ChangeScene(DWCUtilPageMulti5_DisableButtons);
}
