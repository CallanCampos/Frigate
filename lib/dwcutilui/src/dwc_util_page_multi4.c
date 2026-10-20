#include "dwc_util_page_multi4.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_text.h"
#include "nitro/gx.h"

static void func_ov182_021b0afc(void);
static void func_ov182_021b0b44(void);
static void func_ov182_021b0b68(void);
static void func_ov182_021b0b80(void);
static void func_ov182_021b0b8c(void);
static void func_ov182_021b0b90(void);
static void func_ov182_021b0bb8(void);
static void func_ov182_021b0c08(void);

// The frames since the message appeared
static u8 sPageMulti4Frames;

void func_ov182_021b0ad0(void) {
    sPageMulti4Frames = 0;
    func_ov182_021b0afc();
    func_ov182_021ab9ac();
    func_ov182_021ab6f0(0x23);
    func_ov182_021b80f4(0x10);
    func_ov182_021adfbc(func_ov182_021b0b44);
}

static void func_ov182_021b0afc(void) {
    func_ov182_021ab67c("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b0b44(void) {
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(0, 0x15);
    func_ov182_021adfbc(func_ov182_021b0b68);
}

static void func_ov182_021b0b68(void) {
    if (func_ov182_021bbaac(0) == 0) {
        func_ov182_021adfbc(func_ov182_021b0b80);
    }
}

static void func_ov182_021b0b80(void) {
    func_ov182_021b0c08();
    func_ov182_021b0b8c();
}

static void func_ov182_021b0b8c(void) {
}

static void func_ov182_021b0b90(void) {
    func_ov182_021bbac4(3, 1, 1, 8);
    func_ov182_021bbac4(3, 0, 0x15, 8);
    func_ov182_021adfbc(func_ov182_021b0bb8);
}

static void func_ov182_021b0bb8(void) {
    func_ov182_021b0bb8Inline();
}

static void func_ov182_021b0c08(void) {
    sPageMulti4Frames++;
    if (sPageMulti4Frames >= 120) {
        func_ov182_021adfbc(func_ov182_021b0b90);
    }
}
