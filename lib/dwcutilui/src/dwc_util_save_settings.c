#include "dwc_util_save_settings.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_dialog.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_ap.h"
#include "dwc_util_shared.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/gx.h"

static void DWCUtilSaveSettings_LoadGraphics(void);
static void DWCUtilSaveSettings_FadeIn(void);
static void DWCUtilSaveSettings_WaitFadeIn(void);
static void DWCUtilSaveSettings_Main(void);
static void func_ov182_021b4514(void);
static void func_ov182_021b4518(void);
static void DWCUtilSaveSettings_FadeOut(void);
static void DWCUtilSaveSettings_WaitFadeOut(void);

void DWCUtilSaveSettings_Init(void) {
    DWCUtilSaveSettings_LoadGraphics();
    DWCUtilBgOverlay_ShowConnectionSet();
    DWCUtilWifiSettings_Save();
    DWCUtilMain_SetUpdate(DWCUtilSaveSettings_FadeIn);
}

static void DWCUtilSaveSettings_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/xb4None.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilSaveSettings_FadeIn(void) {
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCUtilDisplay_ShowPlanes(1, GX_PLANEMASK_BG0);
    DWCUtilDisplay_ShowPlanes(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCUtilMain_SetUpdate(DWCUtilSaveSettings_WaitFadeIn);
}

// Shows the message of the save once the screens have faded in
static void DWCUtilSaveSettings_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilDialog_Open(0x44, 5, 1, -1, 0);
        DWCUtilMain_SetUpdate(DWCUtilSaveSettings_Main);
    }
}

static void DWCUtilSaveSettings_Main(void) {
    func_ov182_021b4514();
    func_ov182_021b4518();
    if (!DWCUtilDialog_IsOpen()) {
        DWCUtilMain_SetUpdate(DWCUtilSaveSettings_FadeOut);
    }
}

static void func_ov182_021b4514(void) {
}

static void func_ov182_021b4518(void) {
}

static void DWCUtilSaveSettings_FadeOut(void) {
    DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCUtilMain_SetUpdate(DWCUtilSaveSettings_WaitFadeOut);
}

static void DWCUtilSaveSettings_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilScreen_Stop();
        DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCUtilMain_SetLayout(2, 1);
        DWCUtilMain_SetUpdate(DWCUtilScreenAp_Init);
    }
}
