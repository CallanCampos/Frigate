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

static void DWCUtilPageMulti4_LoadGraphics(void);
static void DWCUtilPageMulti4_FadeIn(void);
static void DWCUtilPageMulti4_WaitFadeIn(void);
static void DWCUtilPageMulti4_Main(void);
static void func_ov182_021b0b8c(void);
static void DWCUtilPageMulti4_FadeOut(void);
static void DWCUtilPageMulti4_WaitFadeOut(void);
static void DWCUtilPageMulti4_UpdateTimer(void);

// The frames since the message appeared
static u8 sPageMulti4Frames;

void DWCUtilPageMulti4_Init(void) {
    sPageMulti4Frames = 0;
    DWCUtilPageMulti4_LoadGraphics();
    DWCUtilBgOverlay_ShowConnectionSet();
    DWCUtilGfxResource_ShowMessage(0x23);
    DWCUtilSound_PlaySe(0x10);
    DWCUtilMain_SetUpdate(DWCUtilPageMulti4_FadeIn);
}

static void DWCUtilPageMulti4_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilPageMulti4_FadeIn(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilPageMulti4_WaitFadeIn);
}

static void DWCUtilPageMulti4_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilMain_SetUpdate(DWCUtilPageMulti4_Main);
    }
}

static void DWCUtilPageMulti4_Main(void) {
    DWCUtilPageMulti4_UpdateTimer();
    func_ov182_021b0b8c();
}

static void func_ov182_021b0b8c(void) {
}

static void DWCUtilPageMulti4_FadeOut(void) {
    DWCUtilFade_Start(3, 1, 1, 8);
    DWCUtilFade_Start(3, 0, 0x15, 8);
    DWCUtilMain_SetUpdate(DWCUtilPageMulti4_WaitFadeOut);
}

static void DWCUtilPageMulti4_WaitFadeOut(void) {
    DWCUtilPageMulti4_WaitFadeOutInline();
}

static void DWCUtilPageMulti4_UpdateTimer(void) {
    sPageMulti4Frames++;
    if (sPageMulti4Frames >= 120) {
        DWCUtilMain_SetUpdate(DWCUtilPageMulti4_FadeOut);
    }
}
