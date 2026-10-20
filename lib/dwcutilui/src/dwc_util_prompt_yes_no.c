#include "dwc_util_prompt_yes_no.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_info_menu.h"
#include "dwc_util_main.h"
#include "dwc_util_option_menu.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_text.h"
#include "dwc_util_top_menu.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "nitro/gx.h"

static void func_ov182_021afbd8(void);
static void func_ov182_021afc28(void);
static void func_ov182_021afc60(void);
static void func_ov182_021afc80(void);
static void func_ov182_021afca0(void);
static void func_ov182_021afcb0(void);
static void func_ov182_021afcd8(void);
static void func_ov182_021afcdc(void);
static void func_ov182_021afd10(void);
static void func_ov182_021afd28(void);
static void func_ov182_021afd6c(void);

// Whether the player answered yes
static u8 sYes;

void func_ov182_021afba4(void) {
    sYes = FALSE;
    func_ov182_021afbd8();
    func_ov182_021ab96c(0x13);
    DWCUtilScreen_Start(0x3c, -1, 0);
    func_ov182_021ab6f0(0x1b);
    func_ov182_021adfbc(func_ov182_021afc28);
}

static void func_ov182_021afbd8(void) {
    func_ov182_021ab67c("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021afc28(void) {
    func_ov182_021bbac4(2, 1, 1, 8);
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(1, 1);
    func_ov182_021bc1dc(0, 0x15);
    func_ov182_021adfbc(func_ov182_021afc60);
}

static void func_ov182_021afc60(void) {
    if (func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab6d0(3);
        func_ov182_021adfbc(func_ov182_021afc80);
    }
}

static void func_ov182_021afc80(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021afca0);
    }
}

static void func_ov182_021afca0(void) {
    func_ov182_021afcb0();
    func_ov182_021afcd8();
    func_ov182_021afcdc();
}

static void func_ov182_021afcb0(void) {
    func_ov182_021aec10Inline();
}

static void func_ov182_021afcd8(void) {
}

static void func_ov182_021afcdc(void) {
    switch (DWCUtilUiObject_GetState()) {
    case 0:
        func_ov182_021b80f4(7);
        break;
    case 1:
        func_ov182_021b80f4(6);
        sYes = TRUE;
        break;
    default:
        return;
    }
    func_ov182_021adfbc(func_ov182_021afd10);
}

static void func_ov182_021afd10(void) {
    DWCUtilUiObject_Disable();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021afd28);
}

static void func_ov182_021afd28(void) {
    if (func_ov182_021bbaac(1) == 0) {
        DWCUtilUiObject_Exit();
        if (!sYes) {
            func_ov182_021bbac4(3, 1, 1, 8);
        }
        func_ov182_021bbac4(3, 0, 0x15, 8);
        func_ov182_021adfbc(func_ov182_021afd6c);
    }
}

static void func_ov182_021afd6c(void) {
    if (func_ov182_021bbaac(0) == 0 && (sYes || func_ov182_021bbaac(1) == 0) && DWCUtilUiObject_IsIdle()) {
        func_ov182_021bbf7c(0);
        if (!sYes) {
            DWCUtilScreen_Stop();
            func_ov182_021bc228(1, 1);
        }
        func_ov182_021bc228(0, 0x15);
        if (!sYes) {
            func_ov182_021adfc8(0, 1);
            func_ov182_021adfbc(func_ov182_021af25c);
        } else {
            func_ov182_021adfc8(0, 1);
            func_ov182_021adfbc(func_ov182_021afa28);
        }
    }
}
