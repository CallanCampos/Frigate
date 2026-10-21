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
static void DWCUtilPageMulti5_FadeIn(void);
static void DWCUtilPageMulti5_WaitFadeIn(void);
static void DWCUtilPageMulti5_WaitButtons(void);
static void DWCUtilPageMulti5_Main(void);
static void DWCUtilPageMulti5_HandleInput(void);
static void func_ov182_021b0558(void);
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
    DWCUtilMove_Start(DWCUtilPageMulti5_OnMoveEvent);
    sMoveWaitEnd = MOVE_WAIT_CANCELLED;
    DWCUtilPageMulti5_LoadGraphics();
    DWCUtilGfxResource_ShowMessage(0x1c);
    DWCUtilUiEffect_Init(0);
    DWCUtilSound_PlaySe(0xb);
    DWCUtilMain_SetUpdate(DWCUtilPageMulti5_FadeIn);
}

static void DWCUtilPageMulti5_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilPageMulti5_FadeIn(void) {
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCUtilDisplay_ShowPlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCUtilMain_SetUpdate(DWCUtilPageMulti5_WaitFadeIn);
}

static void DWCUtilPageMulti5_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(1);
        DWCUtilMain_SetUpdate(DWCUtilPageMulti5_WaitButtons);
    }
}

static void DWCUtilPageMulti5_WaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilMain_SetUpdate(DWCUtilPageMulti5_Main);
    }
}

static void DWCUtilPageMulti5_Main(void) {
    DWCUtilPageMulti5_HandleInput();
    func_ov182_021b0558();
    DWCUtilPageMulti5_HandleChoice();
}

static void DWCUtilPageMulti5_HandleInput(void) {
    if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (DWCUtilPageMulti5_IsLidClosed()) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b0558(void) {
}

static void DWCUtilPageMulti5_HandleChoice(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(7);
        DWCUtilMain_SetUpdate(DWCUtilPageMulti5_DisableButtons);
    }
}

static void DWCUtilPageMulti5_DisableButtons(void) {
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED) {
        DWCUtilMove_End();
    }
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilPageMulti5_FadeOut);
}

static void DWCUtilPageMulti5_FadeOut(void) {
    if (DWCUtilFade_IsActive(1) != 0) {
        return;
    }
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED && !DWCUtilMove_HasEnded()) {
        return;
    }
    DWCUtilUiObject_Exit();
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED) {
        DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    }
    DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCUtilMain_SetUpdate(DWCUtilPageMulti5_WaitFadeOut);
}

static void DWCUtilPageMulti5_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(0) != 0) {
        return;
    }
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED && DWCUtilFade_IsActive(1) != 0) {
        return;
    }
    if (!DWCUtilUiObject_IsIdle()) {
        return;
    }
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED && !DWCUtilMove_HasEnded()) {
        return;
    }
    DWCUtilUiEffect_Destroy();
    DWCUtilText_DestroyScreenBg(0);
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED) {
        DWCUtilScreen_Stop();
        DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
    }
    DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED) {
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(DWCUtilOptionMenu_Init);
    } else if (sMoveWaitEnd == MOVE_WAIT_EVENT) {
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(DWCUtilNoticeScreen_Init);
    } else {
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(DWCUtilPageMove_Init);
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
    DWCUtilMove_SetCallback(NULL);
    DWCUtilMain_SetUpdate(DWCUtilPageMulti5_DisableButtons);
}
