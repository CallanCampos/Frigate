#include "dwc_util_timed_notice.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_move.h"
#include "dwc_util_option_menu.h"
#include "dwc_util_screen.h"
#include "dwc_util_text.h"
#include "nitro/gx.h"

static void func_ov182_021afe10(void);
static void func_ov182_021afe58(void);
static void func_ov182_021afe7c(void);
static void func_ov182_021afe9c(void);
static void func_ov182_021afea8(void);
static void func_ov182_021afeac(void);
static void func_ov182_021afed4(void);
static void func_ov182_021aff1c(void);

// The frames the notice has been shown
static u8 sFrames;

void func_ov182_021afdec(void) {
    sFrames = 0;
    func_ov182_021afe10();
    func_ov182_021ab6f0(0x21);
    DWCUtilMain_SetUpdate(func_ov182_021afe58);
}

static void func_ov182_021afe10(void) {
    func_ov182_021ab67c("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021afe58(void) {
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(0, 0x15);
    DWCUtilMain_SetUpdate(func_ov182_021afe7c);
}

static void func_ov182_021afe7c(void) {
    if (func_ov182_021bbaac(0) == 0 && func_ov182_021ae210()) {
        DWCUtilMain_SetUpdate(func_ov182_021afe9c);
    }
}

static void func_ov182_021afe9c(void) {
    func_ov182_021aff1c();
    func_ov182_021afea8();
}

static void func_ov182_021afea8(void) {
}

static void func_ov182_021afeac(void) {
    func_ov182_021bbac4(3, 1, 1, 8);
    func_ov182_021bbac4(3, 0, 0x15, 8);
    DWCUtilMain_SetUpdate(func_ov182_021afed4);
}

static void func_ov182_021afed4(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021bbf7c(0);
        DWCUtilScreen_Stop();
        func_ov182_021bc228(1, 1);
        func_ov182_021bc228(0, 0x15);
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(func_ov182_021af25c);
    }
}

static void func_ov182_021aff1c(void) {
    sFrames++;
    if (sFrames >= 120) {
        DWCUtilMain_SetUpdate(func_ov182_021afeac);
    }
}
