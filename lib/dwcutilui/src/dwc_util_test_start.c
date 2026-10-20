#include "dwc_util_test_start.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_connection.h"
#include "dwc_util_dialog.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_way.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_test_progress.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_effect.h"
#include "dwc_util_ui_object.h"
#include "nitro/gx.h"
#include "nitro/os.h"

static BOOL func_ov182_021b4cc8(void);
static void func_ov182_021b4d28(void);
static void func_ov182_021b4d98(void);
static void func_ov182_021b4dd0(void);
static void func_ov182_021b4e00(void);
static void func_ov182_021b4e20(void);
static void func_ov182_021b4e3c(void);
static void func_ov182_021b4e60(void);
static void func_ov182_021b4e64(void);
static void func_ov182_021b4e84(void);
static void func_ov182_021b4e9c(void);
static void func_ov182_021b4ee4(void);
static void func_ov182_021b4f6c(void);
static void func_ov182_021b4fc8(void);
static void func_ov182_021b4fe8(void);

// Whether the test reached its first step; FALSE when it was ended or failed
static u8 sTestStartPassed;

// A copy of gfl/ui.c's GCTX_HIDIsLidClosed
static BOOL func_ov182_021b4cc8(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov182_021b4ce4(void) {
    sTestStartPassed = FALSE;
    func_ov182_021b4d28();
    func_ov182_021ab9ac();
    DWCUtilScreen_Start(0x37, -1, 0);
    func_ov182_021ab6a4(2);
    func_ov182_021ab6f0(0x24);
    DWCUtilUiEffect_Init(0);
    func_ov182_021b80f4(0xb);
    func_ov182_021adfbc(func_ov182_021b4d98);
}

static void func_ov182_021b4d28(void) {
    func_ov182_021ab67c("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    func_ov182_021ab67c("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    func_ov182_021ab67c("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b4d98(void) {
    func_ov182_021bbac4(2, 1, 1, 8);
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(1, 1);
    func_ov182_021bc1dc(0, 0x15);
    func_ov182_021adfbc(func_ov182_021b4dd0);
}

// Starts the test once the screens have faded in
static void func_ov182_021b4dd0(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021ae3d8();
        func_ov182_021ab6d0(1);
        func_ov182_021adfbc(func_ov182_021b4e00);
    }
}

static void func_ov182_021b4e00(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021b4e20);
    }
}

static void func_ov182_021b4e20(void) {
    OS_Sleep(10);
    func_ov182_021b4f6c();
    func_ov182_021b4e3c();
    func_ov182_021b4e60();
    func_ov182_021b4e64();
}

static void func_ov182_021b4e3c(void) {
    if (func_ov182_021bc798(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (func_ov182_021b4cc8()) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b4e60(void) {
}

// The button ends the test
static void func_ov182_021b4e64(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        func_ov182_021b8138();
        func_ov182_021b80f4(7);
        func_ov182_021adfbc(func_ov182_021b4e84);
    }
}

static void func_ov182_021b4e84(void) {
    DWCUtilUiObject_Disable();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021b4e9c);
}

static void func_ov182_021b4e9c(void) {
    if (func_ov182_021bbaac(1) == 0) {
        if (!sTestStartPassed) {
            DWCUtilUiObject_Exit();
        }
        if (!sTestStartPassed) {
            func_ov182_021bbac4(3, 1, 1, 8);
        }
        func_ov182_021bbac4(3, 0, 0x14, 8);
        func_ov182_021adfbc(func_ov182_021b4ee4);
    }
}

static void func_ov182_021b4ee4(void) {
    if (func_ov182_021bbaac(0) == 0 && (sTestStartPassed || func_ov182_021bbaac(1) == 0) && DWCUtilUiObject_IsIdle()) {
        DWCUtilUiEffect_Destroy();
        func_ov182_021bbf7c(0);
        if (!sTestStartPassed) {
            DWCUtilScreen_Stop();
            func_ov182_021bc228(1, 1);
        }
        func_ov182_021bc228(0, 0x15);
        if (!sTestStartPassed) {
            func_ov182_021ae424();
            func_ov182_021adfc8(2, 1);
            func_ov182_021adfbc(func_ov182_021b5bb8);
        } else {
            func_ov182_021adfc8(0, 0);
            func_ov182_021adfbc(func_ov182_021b45a4);
        }
    }
}

// What the test reports
static void func_ov182_021b4f6c(void) {
    switch (func_ov182_021ae440()) {
    case 1:
        sTestStartPassed = TRUE;
        func_ov182_021adfbc(func_ov182_021b4e84);
        break;
    case 4:
        sTestStartPassed = FALSE;
        func_ov182_021b8138();
        func_ov182_021b80f4(9);
        func_ov182_021b8cf0(0xd, 1, 1, -1, 0);
        DWCUtilUiObject_Disable();
        func_ov182_021adfbc(func_ov182_021b4fc8);
        break;
    }
}

static void func_ov182_021b4fc8(void) {
    if (func_ov182_021b8f14() == 0) {
        func_ov182_021b80f4(6);
        func_ov182_021b8eec();
        func_ov182_021adfbc(func_ov182_021b4fe8);
    }
}

static void func_ov182_021b4fe8(void) {
    if (!func_ov182_021b8f24()) {
        func_ov182_021adfbc(func_ov182_021b4e84);
    }
}
