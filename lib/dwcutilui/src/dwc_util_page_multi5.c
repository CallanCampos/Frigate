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

static BOOL func_ov182_021b0420(void);
static void func_ov182_021b0474(void);
static void func_ov182_021b04c4(void);
static void func_ov182_021b04e8(void);
static void func_ov182_021b0508(void);
static void func_ov182_021b0524(void);
static void func_ov182_021b0534(void);
static void func_ov182_021b0558(void);
static void func_ov182_021b055c(void);
static void func_ov182_021b057c(void);
static void func_ov182_021b05a4(void);
static void func_ov182_021b05f4(void);
static void func_ov182_021b06a0(int event);

static u8 sMoveWaitEnd;

// Whether the lid is closed
static BOOL func_ov182_021b0420(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov182_021b043c(void) {
    func_ov182_021ae134(func_ov182_021b06a0);
    sMoveWaitEnd = MOVE_WAIT_CANCELLED;
    func_ov182_021b0474();
    DWCUtilGfxResource_ShowMessage(0x1c);
    DWCUtilUiEffect_Init(0);
    DWCUtilSound_PlaySe(0xb);
    DWCUtilMain_SetUpdate(func_ov182_021b04c4);
}

static void func_ov182_021b0474(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b04c4(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(func_ov182_021b04e8);
}

static void func_ov182_021b04e8(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(1);
        DWCUtilMain_SetUpdate(func_ov182_021b0508);
    }
}

static void func_ov182_021b0508(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilMain_SetUpdate(func_ov182_021b0524);
    }
}

static void func_ov182_021b0524(void) {
    func_ov182_021b0534();
    func_ov182_021b0558();
    func_ov182_021b055c();
}

static void func_ov182_021b0534(void) {
    if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (func_ov182_021b0420()) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b0558(void) {
}

static void func_ov182_021b055c(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(7);
        DWCUtilMain_SetUpdate(func_ov182_021b057c);
    }
}

static void func_ov182_021b057c(void) {
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED) {
        func_ov182_021ae1d4();
    }
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(func_ov182_021b05a4);
}

static void func_ov182_021b05a4(void) {
    if (DWCUtilFade_IsActive(1) != 0) {
        return;
    }
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED && !func_ov182_021ae210()) {
        return;
    }
    DWCUtilUiObject_Exit();
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED) {
        DWCUtilFade_Start(3, 1, 1, 8);
    }
    DWCUtilFade_Start(3, 0, 0x15, 8);
    DWCUtilMain_SetUpdate(func_ov182_021b05f4);
}

static void func_ov182_021b05f4(void) {
    if (DWCUtilFade_IsActive(0) != 0) {
        return;
    }
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED && DWCUtilFade_IsActive(1) != 0) {
        return;
    }
    if (!DWCUtilUiObject_IsIdle()) {
        return;
    }
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED && !func_ov182_021ae210()) {
        return;
    }
    DWCUtilUiEffect_Destroy();
    DWCUtilText_DestroyScreenBg(0);
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED) {
        DWCUtilScreen_Stop();
        DWCUtilDisplay_HidePlanes(1, 1);
    }
    DWCUtilDisplay_HidePlanes(0, 0x15);
    if (sMoveWaitEnd == MOVE_WAIT_CANCELLED) {
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(func_ov182_021af25c);
    } else if (sMoveWaitEnd == MOVE_WAIT_EVENT) {
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(func_ov182_021aff3c);
    } else {
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(func_ov182_021b0198);
    }
}

static void func_ov182_021b06a0(int event) {
    DWCUtilSound_StopSe();
    if (event == 0) {
        sMoveWaitEnd = MOVE_WAIT_CONNECTED;
        DWCUtilSound_PlaySe(0x10);
    } else {
        sMoveWaitEnd = MOVE_WAIT_EVENT;
        DWCUtilSound_PlaySe(0x12);
    }
    func_ov182_021ae224(NULL);
    DWCUtilMain_SetUpdate(func_ov182_021b057c);
}
