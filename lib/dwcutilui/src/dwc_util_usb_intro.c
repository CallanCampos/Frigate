#include "dwc_util_usb_intro.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_info_menu.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_ap.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_text.h"
#include "dwc_util_ui_object.h"
#include "dwc_util_usb_setup.h"
#include "nitro/gx.h"

static void func_ov182_021b68c8(void);
static void func_ov182_021b6938(void);
static void func_ov182_021b6970(void);
static void func_ov182_021b6998(void);
static void func_ov182_021b69b8(void);
static void func_ov182_021b69c8(void);
static void func_ov182_021b69f0(void);
static void func_ov182_021b69f4(void);
static void func_ov182_021b6a28(void);
static void func_ov182_021b6a40(void);
static void func_ov182_021b6a84(void);

// Whether the player chose to set up the connector
static u8 sUsbIntroYes;

void func_ov182_021b688c(void) {
    sUsbIntroYes = FALSE;
    func_ov182_021b68c8();
    DWCUtilBgOverlay_Show(8);
    DWCUtilScreen_Start(0x38, -1, 0);
    DWCUtilGfxResource_ShowStep(1);
    DWCUtilGfxResource_ShowMessage(0x27);
    DWCUtilMain_SetUpdate(func_ov182_021b6938);
}

static void func_ov182_021b68c8(void) {
    DWCUtilGfxResource_Load("char/jbBgStep2.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/ybBgStep2.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/xb3Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b6938(void) {
    DWCUtilFade_Start(2, 1, 1, 8);
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(1, 1);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(func_ov182_021b6970);
}

static void func_ov182_021b6970(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(6);
        DWCUtilMain_SetUpdate(func_ov182_021b6998);
    }
}

static void func_ov182_021b6998(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(func_ov182_021b69b8);
    }
}

static void func_ov182_021b69b8(void) {
    func_ov182_021b69c8();
    func_ov182_021b69f0();
    func_ov182_021b69f4();
}

static void func_ov182_021b69c8(void) {
    DWCUtilInfoMenu_EraseHandleInputInline();
}

static void func_ov182_021b69f0(void) {
}

static void func_ov182_021b69f4(void) {
    switch (DWCUtilUiObject_GetState()) {
    case 0:
        DWCUtilSound_PlaySe(7);
        break;
    case 1:
        DWCUtilSound_PlaySe(6);
        sUsbIntroYes = TRUE;
        break;
    default:
        return;
    }
    DWCUtilMain_SetUpdate(func_ov182_021b6a28);
}

static void func_ov182_021b6a28(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(func_ov182_021b6a40);
}

static void func_ov182_021b6a40(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        if (!sUsbIntroYes) {
            DWCUtilFade_Start(3, 1, 1, 8);
        }
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(func_ov182_021b6a84);
    }
}

static void func_ov182_021b6a84(void) {
    if (DWCUtilFade_IsActive(0) == 0 && (sUsbIntroYes || DWCUtilFade_IsActive(1) == 0) && DWCUtilUiObject_IsIdle()) {
        DWCUtilText_DestroyScreenBg(0);
        if (!sUsbIntroYes) {
            DWCUtilScreen_Stop();
            DWCUtilDisplay_HidePlanes(1, 1);
        }
        DWCUtilDisplay_HidePlanes(0, 0x15);
        if (!sUsbIntroYes) {
            DWCUtilMain_SetLayout(2, 1);
            DWCUtilMain_SetUpdate(DWCUtilScreenAp_Init);
        } else {
            DWCUtilMain_SetLayout(2, 1);
            DWCUtilMain_SetUpdate(func_ov182_021b6b1c);
        }
    }
}
