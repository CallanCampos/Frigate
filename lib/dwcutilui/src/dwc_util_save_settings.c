#include "dwc_util_save_settings.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_ap.h"
#include "dwc_util_shared.h"
#include "nitro/gx.h"

static void func_ov182_021b4420(void);
static void func_ov182_021b4488(void);
static void func_ov182_021b44c0(void);
static void func_ov182_021b44f4(void);
static void func_ov182_021b4514(void);
static void func_ov182_021b4518(void);
static void func_ov182_021b451c(void);
static void func_ov182_021b4544(void);

void func_ov182_021b4404(void) {
    func_ov182_021b4420();
    func_ov182_021ab9ac();
    func_ov182_021b7bb0();
    func_ov182_021adfbc(func_ov182_021b4488);
}

static void func_ov182_021b4420(void) {
    func_ov182_021ab67c("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    func_ov182_021ab67c("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    func_ov182_021ab67c("char/xb4None.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b4488(void) {
    func_ov182_021bbac4(2, 1, 1, 8);
    func_ov182_021bbac4(2, 0, 0x14, 8);
    func_ov182_021bc1dc(1, 1);
    func_ov182_021bc1dc(0, 0x14);
    func_ov182_021adfbc(func_ov182_021b44c0);
}

// Shows the message of the save once the screens have faded in
static void func_ov182_021b44c0(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021b8cf0(0x44, 5, 1, -1, 0);
        func_ov182_021adfbc(func_ov182_021b44f4);
    }
}

static void func_ov182_021b44f4(void) {
    func_ov182_021b4514();
    func_ov182_021b4518();
    if (!func_ov182_021b8f24()) {
        func_ov182_021adfbc(func_ov182_021b451c);
    }
}

static void func_ov182_021b4514(void) {
}

static void func_ov182_021b4518(void) {
}

static void func_ov182_021b451c(void) {
    func_ov182_021bbac4(3, 1, 1, 8);
    func_ov182_021bbac4(3, 0, 0x14, 8);
    func_ov182_021adfbc(func_ov182_021b4544);
}

static void func_ov182_021b4544(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        DWCUtilScreen_Stop();
        func_ov182_021bc228(1, 1);
        func_ov182_021bc228(0, 0x14);
        func_ov182_021adfc8(2, 1);
        func_ov182_021adfbc(func_ov182_021b536c);
    }
}
