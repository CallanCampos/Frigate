#include "dwc_util_notice_screen.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_move.h"
#include "dwc_util_option_menu.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "nitro/gx.h"

// Whether the sub screen shows a message, and the player's answer
typedef struct DWCUtilNoticeScreen {
    u8 subScreen;
    u8 yes;
} DWCUtilNoticeScreen;

static void func_ov182_021aff68(void);
static void func_ov182_021affb8(void);
static void func_ov182_021b0000(void);
static void func_ov182_021b0028(void);
static void func_ov182_021b0050(void);
static void func_ov182_021b0060(void);
static void func_ov182_021b0088(void);
static void func_ov182_021b008c(void);
static void func_ov182_021b00c0(void);
static void func_ov182_021b00d8(void);
static void func_ov182_021b011c(void);

static DWCUtilNoticeScreen sNoticeScreen;

void func_ov182_021aff3c(void) {
    func_ov182_021aff68();
    func_ov182_021ab6f0(0x20);
    sNoticeScreen.subScreen = DWCUtilScreen_Start(0x3c, -1, 0);
    func_ov182_021adfbc(func_ov182_021affb8);
}

static void func_ov182_021aff68(void) {
    func_ov182_021ab67c("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021affb8(void) {
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(0, 0x15);
    if (sNoticeScreen.subScreen) {
        func_ov182_021bbac4(2, 1, 1, 8);
        func_ov182_021bc1dc(1, 1);
    }
    func_ov182_021adfbc(func_ov182_021b0000);
}

static void func_ov182_021b0000(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab6d0(3);
        func_ov182_021adfbc(func_ov182_021b0028);
    }
}

static void func_ov182_021b0028(void) {
    if (DWCUtilUiObject_GetState() != -2 && func_ov182_021ae210()) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021b0050);
    }
}

static void func_ov182_021b0050(void) {
    func_ov182_021b0060();
    func_ov182_021b0088();
    func_ov182_021b008c();
}

static void func_ov182_021b0060(void) {
    if (func_ov182_021bc798(1)) {
        DWCUtilUiObject_SetStateIfIdle(1);
    }
    if (func_ov182_021bc798(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b0088(void) {
}

static void func_ov182_021b008c(void) {
    switch (DWCUtilUiObject_GetState()) {
    case 1:
        sNoticeScreen.yes = TRUE;
        func_ov182_021b80f4(6);
        break;
    case 0:
        sNoticeScreen.yes = FALSE;
        func_ov182_021b80f4(7);
        break;
    default:
        return;
    }
    func_ov182_021adfbc(func_ov182_021b00c0);
}

static void func_ov182_021b00c0(void) {
    DWCUtilUiObject_Disable();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021b00d8);
}

static void func_ov182_021b00d8(void) {
    if (func_ov182_021bbaac(1) == 0) {
        DWCUtilUiObject_Exit();
        if (!sNoticeScreen.yes) {
            func_ov182_021bbac4(3, 1, 1, 8);
        }
        func_ov182_021bbac4(3, 0, 0x15, 8);
        func_ov182_021adfbc(func_ov182_021b011c);
    }
}

static void func_ov182_021b011c(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0 && DWCUtilUiObject_IsIdle()) {
        func_ov182_021bbf7c(0);
        if (!sNoticeScreen.yes) {
            DWCUtilScreen_Stop();
            func_ov182_021bc228(1, 1);
        }
        func_ov182_021bc228(0, 0x15);
        if (!sNoticeScreen.yes) {
            func_ov182_021adfc8(0, 1);
            func_ov182_021adfbc(func_ov182_021af25c);
        } else {
            func_ov182_021adfc8(0, 1);
            func_ov182_021adfbc(func_ov182_021b043c);
        }
    }
}
