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

static void DWCUtilTestDone_LoadGraphics(void);
static void DWCUtilTestDone_FadeIn(void);
static void DWCUtilTestDone_WaitFadeIn(void);
static void DWCUtilTestDone_Main(void);
static void func_ov182_021b4c2c(void);
static void DWCUtilTestDone_FadeOut(void);
static void DWCUtilTestDone_WaitFadeOut(void);
static void DWCUtilTestDone_UpdateTimer(void);

// The frames since the message appeared
static u8 sTestDoneFrames;

void DWCUtilTestDone_Init(void) {
    sTestDoneFrames = 0;
    DWCUtilTestDone_LoadGraphics();
    DWCUtilBgOverlay_ShowConnectionSet();
    DWCUtilGfxResource_ShowMessage(0x26);
    DWCUtilSound_PlaySe(0x10);
    DWCUtilMain_SetUpdate(DWCUtilTestDone_FadeIn);
}

static void DWCUtilTestDone_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilTestDone_FadeIn(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilTestDone_WaitFadeIn);
}

static void DWCUtilTestDone_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilMain_SetUpdate(DWCUtilTestDone_Main);
    }
}

static void DWCUtilTestDone_Main(void) {
    DWCUtilTestDone_UpdateTimer();
    func_ov182_021b4c2c();
}

static void func_ov182_021b4c2c(void) {
}

static void DWCUtilTestDone_FadeOut(void) {
    DWCUtilFade_Start(3, 1, 1, 8);
    DWCUtilFade_Start(3, 0, 0x15, 8);
    DWCUtilMain_SetUpdate(DWCUtilTestDone_WaitFadeOut);
}

static void DWCUtilTestDone_WaitFadeOut(void) {
    func_ov182_021b0bb8Inline();
}

static void DWCUtilTestDone_UpdateTimer(void) {
    sTestDoneFrames++;
    if (sTestDoneFrames >= 120) {
        DWCUtilMain_SetUpdate(DWCUtilTestDone_FadeOut);
    }
}
