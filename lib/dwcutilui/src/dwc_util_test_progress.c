#include "dwc_util_test_progress.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_connection.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_effect.h"
#include "dwc_util_ui_object.h"
#include "nitro/gx.h"
#include "nitro/os.h"

static BOOL func_ov182_021b4588(void);
static void func_ov182_021b45d0(void);
static void func_ov182_021b4620(void);
static void func_ov182_021b4644(void);
static void func_ov182_021b4664(void);
static void func_ov182_021b4684(void);
static void func_ov182_021b46a0(void);
static void func_ov182_021b46c4(void);
static void func_ov182_021b46c8(void);
static void func_ov182_021b46e8(void);
static void func_ov182_021b4700(void);
static void func_ov182_021b4748(void);
static void func_ov182_021b47d0(void);
static void func_ov182_021b482c(void);
static void func_ov182_021b484c(void);

// Whether the test reached its second step; FALSE when it was ended or failed
static u8 sTestProgressPassed;

// A copy of gfl/ui.c's GCTX_HIDIsLidClosed
static BOOL func_ov182_021b4588(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov182_021b45a4(void) {
    sTestProgressPassed = FALSE;
    func_ov182_021b45d0();
    func_ov182_021ab9ac();
    func_ov182_021ab6f0(0x25);
    DWCUtilUiEffect_Init(1);
    func_ov182_021adfbc(func_ov182_021b4620);
}

static void func_ov182_021b45d0(void) {
    func_ov182_021ab67c("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b4620(void) {
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(0, 0x15);
    func_ov182_021adfbc(func_ov182_021b4644);
}

static void func_ov182_021b4644(void) {
    if (func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab6d0(1);
        func_ov182_021adfbc(func_ov182_021b4664);
    }
}

static void func_ov182_021b4664(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021b4684);
    }
}

static void func_ov182_021b4684(void) {
    OS_Sleep(10);
    func_ov182_021b47d0();
    func_ov182_021b46a0();
    func_ov182_021b46c4();
    func_ov182_021b46c8();
}

static void func_ov182_021b46a0(void) {
    if (func_ov182_021bc798(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (func_ov182_021b4588()) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b46c4(void) {
}

// The button ends the test
static void func_ov182_021b46c8(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        func_ov182_021b8138();
        func_ov182_021b80f4(7);
        func_ov182_021adfbc(func_ov182_021b46e8);
    }
}

static void func_ov182_021b46e8(void) {
    DWCUtilUiObject_Disable();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021b4700);
}

static void func_ov182_021b4700(void) {
    if (func_ov182_021bbaac(1) == 0) {
        if (!sTestProgressPassed) {
            DWCUtilUiObject_Exit();
        }
        if (!sTestProgressPassed) {
            func_ov182_021bbac4(3, 1, 1, 8);
        }
        func_ov182_021bbac4(3, 0, 0x15, 8);
        func_ov182_021adfbc(func_ov182_021b4748);
    }
}

static void func_ov182_021b4748(void) {
    if (func_ov182_021bbaac(0) == 0 && (sTestProgressPassed || func_ov182_021bbaac(1) == 0) &&
        DWCUtilUiObject_IsIdle()) {
        DWCUtilUiEffect_Destroy();
        func_ov182_021bbf7c(0);
        if (!sTestProgressPassed) {
            DWCUtilScreen_Stop();
            func_ov182_021bc228(1, 1);
        }
        func_ov182_021bc228(0, 0x15);
        if (!sTestProgressPassed) {
            func_ov182_021ae424();
            func_ov182_021adfc8(2, 1);
            func_ov182_021adfbc(func_ov182_021b5bb8);
        } else {
            func_ov182_021adfc8(0, 0);
            func_ov182_021adfbc(func_ov182_021b4880);
        }
    }
}

// What the test reports
static void func_ov182_021b47d0(void) {
    switch (func_ov182_021ae440()) {
    case 2:
        sTestProgressPassed = TRUE;
        func_ov182_021adfbc(func_ov182_021b46e8);
        break;
    case 4:
        sTestProgressPassed = FALSE;
        func_ov182_021b8138();
        func_ov182_021b80f4(9);
        func_ov182_021b8cf0(0xd, 1, 1, -1, 0);
        DWCUtilUiObject_Disable();
        func_ov182_021adfbc(func_ov182_021b482c);
        break;
    }
}

static void func_ov182_021b482c(void) {
    if (func_ov182_021b8f14() == 0) {
        func_ov182_021b80f4(6);
        func_ov182_021b8eec();
        func_ov182_021adfbc(func_ov182_021b484c);
    }
}

static void func_ov182_021b484c(void) {
    if (!func_ov182_021b8f24()) {
        func_ov182_021adfbc(func_ov182_021b46e8);
    }
}
