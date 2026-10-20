#include "dwc_util_test_connect.h"
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

static BOOL func_ov182_021b4864(void);
static void func_ov182_021b48ac(void);
static void func_ov182_021b48fc(void);
static void func_ov182_021b4920(void);
static void func_ov182_021b4940(void);
static void func_ov182_021b4960(void);
static void func_ov182_021b497c(void);
static void func_ov182_021b49a0(void);
static void func_ov182_021b49a4(void);
static void func_ov182_021b49c4(void);
static void func_ov182_021b49dc(void);
static void func_ov182_021b4a20(void);
static void func_ov182_021b4a9c(void);
static void func_ov182_021b4b30(void);
static void func_ov182_021b4b50(void);

// Whether the test connected; FALSE when it was ended or failed
static u8 sTestConnectPassed;

// A copy of gfl/ui.c's GCTX_HIDIsLidClosed
static BOOL func_ov182_021b4864(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov182_021b4880(void) {
    sTestConnectPassed = FALSE;
    func_ov182_021b48ac();
    func_ov182_021ab9ac();
    func_ov182_021ab6f0(0x2a);
    DWCUtilUiEffect_Init(2);
    func_ov182_021adfbc(func_ov182_021b48fc);
}

static void func_ov182_021b48ac(void) {
    func_ov182_021ab67c("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b48fc(void) {
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(0, 0x15);
    func_ov182_021adfbc(func_ov182_021b4920);
}

static void func_ov182_021b4920(void) {
    if (func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab6d0(1);
        func_ov182_021adfbc(func_ov182_021b4940);
    }
}

static void func_ov182_021b4940(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021b4960);
    }
}

static void func_ov182_021b4960(void) {
    OS_Sleep(10);
    func_ov182_021b4a9c();
    func_ov182_021b497c();
    func_ov182_021b49a0();
    func_ov182_021b49a4();
}

static void func_ov182_021b497c(void) {
    if (func_ov182_021bc798(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (func_ov182_021b4864()) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b49a0(void) {
}

// The button ends the test
static void func_ov182_021b49a4(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        func_ov182_021b8138();
        func_ov182_021b80f4(7);
        func_ov182_021adfbc(func_ov182_021b49c4);
    }
}

static void func_ov182_021b49c4(void) {
    DWCUtilUiObject_Disable();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021b49dc);
}

static void func_ov182_021b49dc(void) {
    if (func_ov182_021bbaac(1) == 0) {
        DWCUtilUiObject_Exit();
        if (!sTestConnectPassed) {
            func_ov182_021bbac4(3, 1, 1, 8);
        }
        func_ov182_021bbac4(3, 0, 0x15, 8);
        func_ov182_021adfbc(func_ov182_021b4a20);
    }
}

static void func_ov182_021b4a20(void) {
    if (func_ov182_021bbaac(0) == 0 && (sTestConnectPassed || func_ov182_021bbaac(1) == 0) &&
        DWCUtilUiObject_IsIdle()) {
        DWCUtilUiEffect_Destroy();
        func_ov182_021bbf7c(0);
        if (!sTestConnectPassed) {
            DWCUtilScreen_Stop();
            func_ov182_021bc228(1, 1);
        }
        func_ov182_021ae424();
        if (!sTestConnectPassed) {
            func_ov182_021adfc8(2, 1);
            func_ov182_021adfbc(func_ov182_021b5bb8);
        } else {
            func_ov182_021adfc8(0, 0);
            func_ov182_021adfbc(func_ov182_021b4b68);
        }
    }
}

// What the test reports: a connection with a WEP key or none is taken into the connection being edited
static void func_ov182_021b4a9c(void) {
    switch (func_ov182_021ae440()) {
    case 3:
        sTestConnectPassed = TRUE;
        func_ov182_021b8138();
        func_ov182_021ae4b0();
        func_ov182_021adfbc(func_ov182_021b49c4);
        break;
    case 4:
        sTestConnectPassed = FALSE;
        func_ov182_021b8138();
        func_ov182_021b80f4(9);
        func_ov182_021b8cf0(0xd, 1, 1, -1, 0);
        DWCUtilUiObject_Disable();
        func_ov182_021adfbc(func_ov182_021b4b30);
        break;
    case 5:
        sTestConnectPassed = FALSE;
        func_ov182_021b8138();
        func_ov182_021b80f4(0x12);
        func_ov182_021b8cf0(0xe, 1, 1, -1, 0);
        DWCUtilUiObject_Disable();
        func_ov182_021adfbc(func_ov182_021b4b30);
        break;
    }
}

static void func_ov182_021b4b30(void) {
    if (func_ov182_021b8f14() == 0) {
        func_ov182_021b80f4(6);
        func_ov182_021b8eec();
        func_ov182_021adfbc(func_ov182_021b4b50);
    }
}

static void func_ov182_021b4b50(void) {
    if (!func_ov182_021b8f24()) {
        func_ov182_021adfbc(func_ov182_021b49c4);
    }
}
