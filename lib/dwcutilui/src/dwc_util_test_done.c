#include "dwc_util_test_done.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_page_multi4.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "nitro/gx.h"

static void func_ov182_021b4b94(void);
static void func_ov182_021b4be4(void);
static void func_ov182_021b4c08(void);
static void func_ov182_021b4c20(void);
static void func_ov182_021b4c2c(void);
static void func_ov182_021b4c30(void);
static void func_ov182_021b4c58(void);
static void func_ov182_021b4ca8(void);

// The frames since the message appeared
static u8 sTestDoneFrames;

void func_ov182_021b4b68(void) {
    sTestDoneFrames = 0;
    func_ov182_021b4b94();
    DWCUtilBgOverlay_ShowConnectionSet();
    DWCUtilGfxResource_ShowMessage(0x26);
    DWCUtilSound_PlaySe(0x10);
    DWCUtilMain_SetUpdate(func_ov182_021b4be4);
}

static void func_ov182_021b4b94(void) {
    DWCUtilGfxResource_Load("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b4be4(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(func_ov182_021b4c08);
}

static void func_ov182_021b4c08(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilMain_SetUpdate(func_ov182_021b4c20);
    }
}

static void func_ov182_021b4c20(void) {
    func_ov182_021b4ca8();
    func_ov182_021b4c2c();
}

static void func_ov182_021b4c2c(void) {
}

static void func_ov182_021b4c30(void) {
    DWCUtilFade_Start(3, 1, 1, 8);
    DWCUtilFade_Start(3, 0, 0x15, 8);
    DWCUtilMain_SetUpdate(func_ov182_021b4c58);
}

static void func_ov182_021b4c58(void) {
    func_ov182_021b0bb8Inline();
}

static void func_ov182_021b4ca8(void) {
    sTestDoneFrames++;
    if (sTestDoneFrames >= 120) {
        DWCUtilMain_SetUpdate(func_ov182_021b4c30);
    }
}
