#include "dwc_util_top_menu.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_link_icon.h"
#include "dwc_util_main.h"
#include "dwc_util_move.h"
#include "dwc_util_notice_screen.h"
#include "dwc_util_page_multi5.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_text.h"
#include "dwc_util_timed_notice.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_effect.h"
#include "dwc_util_ui_object.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nitro/pm.h"

static void func_ov182_021af740(void);
static void func_ov182_021af790(void);
static void func_ov182_021af7b4(void);
static void func_ov182_021af7d8(void);
static void func_ov182_021af7e4(void);
static void func_ov182_021af7e8(void);
static void func_ov182_021af7ec(void);
static void func_ov182_021af808(void);
static void func_ov182_021af870(int event);
static void func_ov182_021af8bc(void);
static void func_ov182_021af8d4(void);
static void func_ov182_021af910(void);
static void func_ov182_021af934(void);
static void func_ov182_021af954(void);
static void func_ov182_021af974(void);
static void func_ov182_021af984(void);
static void func_ov182_021af998(void);
static void func_ov182_021af99c(void);
static void func_ov182_021af9b8(void);
static void func_ov182_021af9d0(void);
static void func_ov182_021afa04(void);
static void func_ov182_021afa40(void);
static void func_ov182_021afa7c(void);
static void func_ov182_021afaa0(void);
static void func_ov182_021afac8(void);
static void func_ov182_021afae8(void);
static void func_ov182_021afaf8(void);
static void func_ov182_021afb0c(void);
static void func_ov182_021afb10(void);
static void func_ov182_021afb2c(void);
static void func_ov182_021afb44(void);
static void func_ov182_021afb6c(void);

// How the move went: 0 when it failed, 1 once it sent the settings, 2 when it ended without sending them
static u8 sMoveResult;

void func_ov182_021af714(void) {
    func_ov182_021af740();
    DWCUtilGfxResource_ShowMessage(0x1e);
    DWCUtilUiEffect_Init(0);
    DWCUtilLinkIcon_Init(1);
    DWCUtilSound_PlaySe(0xb);
    DWCUtilMain_SetUpdate(func_ov182_021af790);
}

static void func_ov182_021af740(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021af790(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(func_ov182_021af7b4);
}

static void func_ov182_021af7b4(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        func_ov182_021ae224(func_ov182_021af870);
        DWCUtilMain_SetUpdate(func_ov182_021af7d8);
    }
}

static void func_ov182_021af7d8(void) {
    func_ov182_021af7e4();
    func_ov182_021af7e8();
}

static void func_ov182_021af7e4(void) {
}

static void func_ov182_021af7e8(void) {
}

static void func_ov182_021af7ec(void) {
    DWCUtilFade_Start(3, 0, 0x15, 8);
    DWCUtilMain_SetUpdate(func_ov182_021af808);
}

static void func_ov182_021af808(void) {
    if (DWCUtilFade_IsActive(0) == 0 && func_ov182_021ae210()) {
        DWCUtilLinkIcon_Exit();
        DWCUtilUiEffect_Destroy();
        DWCUtilText_DestroyScreenBg(0);
        DWCUtilDisplay_HidePlanes(0, 0x15);
        DWCUtilMain_SetLayout(0, 1);
        if (sMoveResult == 0) {
            DWCUtilMain_SetUpdate(func_ov182_021aff3c);
        } else if (sMoveResult == 2) {
            DWCUtilMain_SetUpdate(func_ov182_021afdec);
        } else {
            DWCUtilMain_SetUpdate(func_ov182_021af8bc);
        }
    }
}

// The move's callback: once the settings are sent they are erased here
static void func_ov182_021af870(int event) {
    if (event == DWC_UTIL_MOVE_EVENT_DONE) {
        func_ov182_021b7ea0();
        sMoveResult = 1;
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(0x10);
    } else {
        if (event == DWC_UTIL_MOVE_EVENT_ENDED) {
            sMoveResult = 2;
        } else {
            sMoveResult = 0;
        }
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(0x12);
    }
    func_ov182_021ae224(NULL);
    func_ov182_021ae1d4();
    DWCUtilMain_SetUpdate(func_ov182_021af7ec);
}

// The settings were sent: the player is told to turn the console off
static void func_ov182_021af8bc(void) {
    func_ov182_021af8d4();
    DWCUtilGfxResource_ShowMessage(0x1f);
    DWCUtilMain_SetUpdate(func_ov182_021af910);
}

static void func_ov182_021af8d4(void) {
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021af910(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(func_ov182_021af934);
}

static void func_ov182_021af934(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(5);
        DWCUtilMain_SetUpdate(func_ov182_021af954);
    }
}

static void func_ov182_021af954(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(func_ov182_021af974);
    }
}

static void func_ov182_021af974(void) {
    func_ov182_021af984();
    func_ov182_021af998();
    func_ov182_021af99c();
}

static void func_ov182_021af984(void) {
    if (DWCUtilTouch_IsKeyPressed(1)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021af998(void) {
}

static void func_ov182_021af99c(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCUtilMain_SetUpdate(func_ov182_021af9b8);
    }
}

static void func_ov182_021af9b8(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(func_ov182_021af9d0);
}

static void func_ov182_021af9d0(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilFade_Start(3, 1, 0x3f, 0x40);
        DWCUtilFade_Start(3, 0, 0x3f, 0x40);
        DWCUtilMain_SetUpdate(func_ov182_021afa04);
    }
}

// Turns the console off once both screens have faded out
static void func_ov182_021afa04(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        OS_SpinWaitSysCycles(0x800000);
        PM_ForceToPowerOff();
    }
}

void func_ov182_021afa28(void) {
    func_ov182_021afa40();
    DWCUtilGfxResource_ShowMessage(0x1a);
    DWCUtilMain_SetUpdate(func_ov182_021afa7c);
}

static void func_ov182_021afa40(void) {
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021afa7c(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(func_ov182_021afaa0);
}

static void func_ov182_021afaa0(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(4);
        DWCUtilMain_SetUpdate(func_ov182_021afac8);
    }
}

static void func_ov182_021afac8(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(func_ov182_021afae8);
    }
}

static void func_ov182_021afae8(void) {
    func_ov182_021afaf8();
    func_ov182_021afb0c();
    func_ov182_021afb10();
}

static void func_ov182_021afaf8(void) {
    if (DWCUtilTouch_IsKeyPressed(1)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021afb0c(void) {
}

static void func_ov182_021afb10(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCUtilMain_SetUpdate(func_ov182_021afb2c);
    }
}

static void func_ov182_021afb2c(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(func_ov182_021afb44);
}

static void func_ov182_021afb44(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(func_ov182_021afb6c);
    }
}

static void func_ov182_021afb6c(void) {
    if (DWCUtilFade_IsActive(0) == 0 && DWCUtilUiObject_IsIdle()) {
        DWCUtilText_DestroyScreenBg(0);
        DWCUtilDisplay_HidePlanes(0, 0x15);
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(func_ov182_021b043c);
    }
}
