#include "dwc_util_usb_done.h"
#include "dwc_util_auto_connect_intro.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "nitro/gx.h"

static void func_ov182_021b6758(void);
static void func_ov182_021b67a8(void);
static void func_ov182_021b67cc(void);
static void func_ov182_021b67e4(void);
static void func_ov182_021b67f0(void);
static void func_ov182_021b67f4(void);
static void func_ov182_021b681c(void);
static void func_ov182_021b686c(void);

// The frames since the message appeared
static u8 sUsbDoneFrames;

void func_ov182_021b6728(void) {
    sUsbDoneFrames = 0;
    func_ov182_021b6758();
    func_ov182_021ab96c(8);
    func_ov182_021ab6f0(0x29);
    DWCUtilSound_PlaySe(0x10);
    DWCUtilMain_SetUpdate(func_ov182_021b67a8);
}

static void func_ov182_021b6758(void) {
    func_ov182_021ab67c("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b67a8(void) {
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(0, 0x15);
    DWCUtilMain_SetUpdate(func_ov182_021b67cc);
}

static void func_ov182_021b67cc(void) {
    if (func_ov182_021bbaac(0) == 0) {
        DWCUtilMain_SetUpdate(func_ov182_021b67e4);
    }
}

static void func_ov182_021b67e4(void) {
    func_ov182_021b686c();
    func_ov182_021b67f0();
}

static void func_ov182_021b67f0(void) {
}

static void func_ov182_021b67f4(void) {
    func_ov182_021bbac4(3, 1, 1, 8);
    func_ov182_021bbac4(3, 0, 0x15, 8);
    DWCUtilMain_SetUpdate(func_ov182_021b681c);
}

// Once both screens have faded out, goes on to the message before the connection test with auto-connect
static void func_ov182_021b681c(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021bbf7c(0);
        DWCUtilScreen_Stop();
        func_ov182_021bc228(1, 1);
        func_ov182_021bc228(0, 0x15);
        DWCUtilMain_SetLayout(0, 0);
        DWCUtilMain_SetScreenArgs(0, 2);
        DWCUtilMain_SetUpdate(func_ov182_021b6588);
    }
}

static void func_ov182_021b686c(void) {
    sUsbDoneFrames++;
    if (sUsbDoneFrames >= 120) {
        DWCUtilMain_SetUpdate(func_ov182_021b67f4);
    }
}
