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

static void DWCUtilTimedNotice_LoadGraphics(void);
static void DWCUtilTimedNotice_FadeIn(void);
static void DWCUtilTimedNotice_WaitFadeIn(void);
static void DWCUtilTimedNotice_Main(void);
static void func_ov182_021afea8(void);
static void DWCUtilTimedNotice_FadeOut(void);
static void DWCUtilTimedNotice_WaitFadeOut(void);
static void DWCUtilTimedNotice_CountFrames(void);

// The frames the notice has been shown
static u8 sFrames;

void DWCUtilTimedNotice_Init(void) {
    sFrames = 0;
    DWCUtilTimedNotice_LoadGraphics();
    DWCUtilGfxResource_ShowMessage(0x21);
    DWCUtilMain_SetUpdate(DWCUtilTimedNotice_FadeIn);
}

static void DWCUtilTimedNotice_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilTimedNotice_FadeIn(void) {
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCUtilDisplay_ShowPlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCUtilMain_SetUpdate(DWCUtilTimedNotice_WaitFadeIn);
}

static void DWCUtilTimedNotice_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(0) == 0 && DWCUtilMove_HasEnded()) {
        DWCUtilMain_SetUpdate(DWCUtilTimedNotice_Main);
    }
}

static void DWCUtilTimedNotice_Main(void) {
    DWCUtilTimedNotice_CountFrames();
    func_ov182_021afea8();
}

static void func_ov182_021afea8(void) {
}

static void DWCUtilTimedNotice_FadeOut(void) {
    DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCUtilMain_SetUpdate(DWCUtilTimedNotice_WaitFadeOut);
}

static void DWCUtilTimedNotice_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilText_DestroyScreenBg(0);
        DWCUtilScreen_Stop();
        DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(DWCUtilOptionMenu_Init);
    }
}

static void DWCUtilTimedNotice_CountFrames(void) {
    sFrames++;
    if (sFrames >= 120) {
        DWCUtilMain_SetUpdate(DWCUtilTimedNotice_FadeOut);
    }
}
