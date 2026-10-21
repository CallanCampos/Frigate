#include "dwc_util_auto_connect_intro.h"
#include "dwc_util_ap_scan.h"
#include "dwc_util_auto_connect.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_dialog.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/gx.h"

static void DWCUtilAutoConnectIntro_LoadGraphics(void);
static void DWCUtilAutoConnectIntro_FadeIn(void);
static void DWCUtilAutoConnectIntro_WaitFadeIn(void);
static void DWCUtilAutoConnectIntro_WaitDialog(void);
static void func_ov182_021b66d0(void);
static void func_ov182_021b66d4(void);
static void DWCUtilAutoConnectIntro_WaitDialogClosed(void);
static void DWCUtilAutoConnectIntro_WaitFadeOut(void);

void DWCUtilAutoConnectIntro_Init(void) {
    int from;

    DWCUtilApScan_Exit();
    DWCUtilMain_GetScreenArgs(NULL, &from);
    DWCUtilAutoConnectIntro_LoadGraphics();
    DWCUtilScreen_Start(0x31, -1, 0);
    if (from != 2) {
        DWCUtilBgOverlay_ShowConnectionSet();
    }
    if (from == 1) {
        DWCUtilWifiSettings_Save();
    }
    DWCUtilMain_SetUpdate(DWCUtilAutoConnectIntro_FadeIn);
}

static void DWCUtilAutoConnectIntro_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/xb4None.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilAutoConnectIntro_FadeIn(void) {
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCUtilDisplay_ShowPlanes(1, GX_PLANEMASK_BG0);
    DWCUtilDisplay_ShowPlanes(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCUtilMain_SetUpdate(DWCUtilAutoConnectIntro_WaitFadeIn);
}

// Shows the message of where the utility was started from in a dialog with one button
static void DWCUtilAutoConnectIntro_WaitFadeIn(void) {
    int msgIndices[3] = { 0x2b, 0x2b, 0x49 };
    int from;

    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilMain_GetScreenArgs(NULL, &from);
        DWCUtilDialog_Open(msgIndices[from], 1, 1, -1, 0);
        DWCUtilMain_SetUpdate(DWCUtilAutoConnectIntro_WaitDialog);
    }
}

static void DWCUtilAutoConnectIntro_WaitDialog(void) {
    func_ov182_021b66d0();
    func_ov182_021b66d4();
    if (DWCUtilDialog_GetChoice() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCUtilDialog_Close();
        DWCUtilSound_PlaySe(6);
        DWCUtilMain_SetUpdate(DWCUtilAutoConnectIntro_WaitDialogClosed);
    }
}

static void func_ov182_021b66d0(void) {
}

static void func_ov182_021b66d4(void) {
}

static void DWCUtilAutoConnectIntro_WaitDialogClosed(void) {
    if (!DWCUtilDialog_IsOpen()) {
        DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCUtilMain_SetUpdate(DWCUtilAutoConnectIntro_WaitFadeOut);
    }
}

static void DWCUtilAutoConnectIntro_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(DWCUtilAutoConnect_Init);
    }
}
