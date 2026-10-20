#include "dwc_util_ap_search.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_page_ap_list.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_effect.h"
#include "dwc_util_ui_object.h"
#include "nitro/gx.h"

// How the search ended, and the frames it has scanned for
typedef struct DWCUtilApSearch {
    u8 result; // 1 when it found an access point that the utility can set up, 2 when none, 3 when none of those
    u16 frames;
} DWCUtilApSearch;

static void func_ov182_021b5050(void);
static void func_ov182_021b50c0(void);
static void func_ov182_021b50f8(void);
static void func_ov182_021b5124(void);
static void func_ov182_021b5144(void);
static void func_ov182_021b5158(void);
static void func_ov182_021b5214(void);
static void func_ov182_021b522c(void);
static void func_ov182_021b5230(void);
static void func_ov182_021b5250(void);
static void func_ov182_021b5268(void);
static void func_ov182_021b52a0(void);
static void func_ov182_021b5334(void);
static void func_ov182_021b5354(void);

static DWCUtilApSearch sApSearch;

void func_ov182_021b5000(void) {
    sApSearch.frames = 0;
    sApSearch.result = 0;
    func_ov182_021b5050();
    DWCUtilScreen_Start(0x33, -1, 0);
    func_ov182_021ab9ac();
    func_ov182_021ab6a4(2);
    func_ov182_021ab6f0(0x33);
    DWCUtilUiEffect_Init(0);
    func_ov182_021b74d4();
    func_ov182_021b751c();
    func_ov182_021b80f4(0xa);
    func_ov182_021adfbc(func_ov182_021b50c0);
}

static void func_ov182_021b5050(void) {
    func_ov182_021ab67c("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    func_ov182_021ab67c("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    func_ov182_021ab67c("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b50c0(void) {
    func_ov182_021bbac4(2, 1, 1, 8);
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(1, 1);
    func_ov182_021bc1dc(0, 0x15);
    func_ov182_021adfbc(func_ov182_021b50f8);
}

static void func_ov182_021b50f8(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab6d0(1);
        func_ov182_021adfbc(func_ov182_021b5124);
    }
}

static void func_ov182_021b5124(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021b5144);
    }
}

static void func_ov182_021b5144(void) {
    func_ov182_021b5214();
    func_ov182_021b522c();
    func_ov182_021b5230();
    func_ov182_021b5158();
}

// After 300 frames, looks at what the scan found
static void func_ov182_021b5158(void) {
    DWCUtilApEntry *entries;
    int count;
    int i;

    sApSearch.frames++;
    if (sApSearch.frames < 300) {
        return;
    }
    func_ov182_021b8138();
    count = func_ov182_021b7630(&entries);
    if (count == 0) {
        sApSearch.result = 2;
        func_ov182_021b8cf0(0xf, 1, 1, -1, 0);
        func_ov182_021b80f4(0x12);
        DWCUtilUiObject_Disable();
        func_ov182_021adfbc(func_ov182_021b5334);
        return;
    }
    for (i = 0; i < count; i++) {
        if (entries[i].security != 2) {
            break;
        }
    }
    if (i == count) {
        sApSearch.result = 3;
        func_ov182_021b8cf0(0xe, 1, 1, -1, 0);
        func_ov182_021b80f4(0x12);
        DWCUtilUiObject_Disable();
        func_ov182_021adfbc(func_ov182_021b5334);
        return;
    }
    sApSearch.result = 1;
    func_ov182_021b80f4(0xf);
    func_ov182_021adfbc(func_ov182_021b5250);
}

static void func_ov182_021b5214(void) {
    if (func_ov182_021bc798(2)) {
        func_ov182_021b8138();
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b522c(void) {
}

// The button ends the search
static void func_ov182_021b5230(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        func_ov182_021b8138();
        func_ov182_021b80f4(7);
        func_ov182_021adfbc(func_ov182_021b5250);
    }
}

static void func_ov182_021b5250(void) {
    DWCUtilUiObject_Disable();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021b5268);
}

static void func_ov182_021b5268(void) {
    if (func_ov182_021bbaac(1) == 0) {
        DWCUtilUiObject_Exit();
        func_ov182_021bbac4(3, 1, 1, 8);
        func_ov182_021bbac4(3, 0, 0x15, 8);
        func_ov182_021adfbc(func_ov182_021b52a0);
    }
}

static void func_ov182_021b52a0(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0 && DWCUtilUiObject_IsIdle()) {
        while (!func_ov182_021b75d0()) {
        }
        DWCUtilUiEffect_Destroy();
        func_ov182_021bbf7c(0);
        DWCUtilScreen_Stop();
        func_ov182_021bc228(1, 1);
        func_ov182_021bc228(0, 0x15);
        if (sApSearch.result != 1) {
            func_ov182_021b74f4();
            func_ov182_021b7b20(func_ov182_021b781c()->index);
            func_ov182_021adfc8(2, 1);
            func_ov182_021adfbc(func_ov182_021b5bb8);
        } else {
            func_ov182_021adfc8(0, 1);
            func_ov182_021adff0(0, 0);
            func_ov182_021adfbc(func_ov182_021b0c28);
        }
    }
}

static void func_ov182_021b5334(void) {
    if (func_ov182_021b8f14() == 0) {
        func_ov182_021b80f4(6);
        func_ov182_021b8eec();
        func_ov182_021adfbc(func_ov182_021b5354);
    }
}

static void func_ov182_021b5354(void) {
    if (!func_ov182_021b8f24()) {
        func_ov182_021adfbc(func_ov182_021b5250);
    }
}
