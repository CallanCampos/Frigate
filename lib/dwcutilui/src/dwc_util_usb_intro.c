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
    func_ov182_021ab96c(8);
    DWCUtilScreen_Start(0x38, -1, 0);
    func_ov182_021ab6a4(1);
    func_ov182_021ab6f0(0x27);
    DWCUtilMain_SetUpdate(func_ov182_021b6938);
}

static void func_ov182_021b68c8(void) {
    func_ov182_021ab67c("char/jbBgStep2.ncg.l", GX_LoadBG2Char);
    func_ov182_021ab67c("char/ybBgStep2.ncl.l", GX_LoadBGPltt);
    func_ov182_021ab67c("char/xb3Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b6938(void) {
    func_ov182_021bbac4(2, 1, 1, 8);
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(1, 1);
    func_ov182_021bc1dc(0, 0x15);
    DWCUtilMain_SetUpdate(func_ov182_021b6970);
}

static void func_ov182_021b6970(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab6d0(6);
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
    func_ov182_021aec10Inline();
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
    func_ov182_021bbbf4(8);
    DWCUtilMain_SetUpdate(func_ov182_021b6a40);
}

static void func_ov182_021b6a40(void) {
    if (func_ov182_021bbaac(1) == 0) {
        DWCUtilUiObject_Exit();
        if (!sUsbIntroYes) {
            func_ov182_021bbac4(3, 1, 1, 8);
        }
        func_ov182_021bbac4(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(func_ov182_021b6a84);
    }
}

static void func_ov182_021b6a84(void) {
    if (func_ov182_021bbaac(0) == 0 && (sUsbIntroYes || func_ov182_021bbaac(1) == 0) && DWCUtilUiObject_IsIdle()) {
        func_ov182_021bbf7c(0);
        if (!sUsbIntroYes) {
            DWCUtilScreen_Stop();
            func_ov182_021bc228(1, 1);
        }
        func_ov182_021bc228(0, 0x15);
        if (!sUsbIntroYes) {
            DWCUtilMain_SetLayout(2, 1);
            DWCUtilMain_SetUpdate(func_ov182_021b536c);
        } else {
            DWCUtilMain_SetLayout(2, 1);
            DWCUtilMain_SetUpdate(func_ov182_021b6b1c);
        }
    }
}
