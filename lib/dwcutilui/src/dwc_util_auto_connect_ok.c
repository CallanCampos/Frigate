#include "dwc_util_auto_connect_ok.h"
#include "dwc_util_address_settings.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_main_menu.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_sub_screen.h"
#include "dwc_util_text.h"
#include "nitro/gx.h"

static void func_ov182_021b6400(void);
static void func_ov182_021b6448(void);
static void func_ov182_021b646c(void);
static void func_ov182_021b6484(void);
static void func_ov182_021b6490(void);
static void func_ov182_021b6494(void);
static void func_ov182_021b64cc(void);
static void func_ov182_021b6540(void);

// The frames since the message appeared
static u8 sAutoConnectOkFrames;

void func_ov182_021b63dc(void) {
    sAutoConnectOkFrames = 0;
    func_ov182_021b6400();
    func_ov182_021ab6f0(0x2d);
    func_ov182_021adfbc(func_ov182_021b6448);
}

static void func_ov182_021b6400(void) {
    func_ov182_021ab67c("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b6448(void) {
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(0, 0x15);
    func_ov182_021adfbc(func_ov182_021b646c);
}

static void func_ov182_021b646c(void) {
    if (func_ov182_021bbaac(0) == 0) {
        func_ov182_021adfbc(func_ov182_021b6484);
    }
}

static void func_ov182_021b6484(void) {
    func_ov182_021b6540();
    func_ov182_021b6490();
}

static void func_ov182_021b6490(void) {
}

static void func_ov182_021b6494(void) {
    int from;

    func_ov182_021adffc(NULL, &from);
    if (from != 0) {
        func_ov182_021b849c();
    }
    func_ov182_021bbac4(3, 1, 1, 8);
    func_ov182_021bbac4(3, 0, 0x15, 8);
    func_ov182_021adfbc(func_ov182_021b64cc);
}

// Once the fades and the buttons are done, back to the settings of the connection, or to the menu that the utility
// was started from
static void func_ov182_021b64cc(void) {
    int from;

    if (func_ov182_021bbaac(1) != 0 || func_ov182_021bbaac(0) != 0) {
        return;
    }
    if (!func_ov182_021b87b0()) {
        return;
    }
    func_ov182_021bbf7c(0);
    DWCUtilScreen_Stop();
    func_ov182_021bc228(1, 1);
    func_ov182_021bc228(0, 0x15);
    func_ov182_021adfc8(2, 1);
    func_ov182_021adffc(NULL, &from);
    if (from == 0) {
        func_ov182_021adff0(0, 0);
        func_ov182_021adfbc(func_ov182_021b2d48);
    } else {
        func_ov182_021adfbc(func_ov182_021ae748);
    }
}

// After three seconds, ends the utility's run when it was started for the connection test alone
static void func_ov182_021b6540(void) {
    int from;

    sAutoConnectOkFrames++;
    if (sAutoConnectOkFrames >= 180) {
        func_ov182_021adffc(NULL, &from);
        switch (func_ov182_021ae024()) {
        case 0:
            break;
        case 1:
            if (from != 0) {
                func_ov182_021adfbc(func_ov182_021ae4f4);
                return;
            }
            break;
        }
        func_ov182_021adfbc(func_ov182_021b6494);
    }
}
