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

static void DWCUtilUsbDone_LoadGraphics(void);
static void DWCUtilUsbDone_FadeIn(void);
static void DWCUtilUsbDone_WaitFadeIn(void);
static void DWCUtilUsbDone_Main(void);
static void func_ov182_021b67f0(void);
static void DWCUtilUsbDone_FadeOut(void);
static void DWCUtilUsbDone_WaitFadeOut(void);
static void DWCUtilUsbDone_CountFrames(void);

// The frames since the message appeared
static u8 sUsbDoneFrames;

void DWCUtilUsbDone_Init(void) {
    sUsbDoneFrames = 0;
    DWCUtilUsbDone_LoadGraphics();
    DWCUtilBgOverlay_Show(8);
    DWCUtilGfxResource_ShowMessage(0x29);
    DWCUtilSound_PlaySe(0x10);
    DWCUtilMain_SetUpdate(DWCUtilUsbDone_FadeIn);
}

static void DWCUtilUsbDone_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilUsbDone_FadeIn(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilUsbDone_WaitFadeIn);
}

static void DWCUtilUsbDone_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilMain_SetUpdate(DWCUtilUsbDone_Main);
    }
}

static void DWCUtilUsbDone_Main(void) {
    DWCUtilUsbDone_CountFrames();
    func_ov182_021b67f0();
}

static void func_ov182_021b67f0(void) {
}

static void DWCUtilUsbDone_FadeOut(void) {
    DWCUtilFade_Start(3, 1, 1, 8);
    DWCUtilFade_Start(3, 0, 0x15, 8);
    DWCUtilMain_SetUpdate(DWCUtilUsbDone_WaitFadeOut);
}

// Once both screens have faded out, goes on to the message before the connection test with auto-connect
static void DWCUtilUsbDone_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilText_DestroyScreenBg(0);
        DWCUtilScreen_Stop();
        DWCUtilDisplay_HidePlanes(1, 1);
        DWCUtilDisplay_HidePlanes(0, 0x15);
        DWCUtilMain_SetLayout(0, 0);
        DWCUtilMain_SetScreenArgs(0, 2);
        DWCUtilMain_SetUpdate(DWCUtilAutoConnectIntro_Init);
    }
}

static void DWCUtilUsbDone_CountFrames(void) {
    sUsbDoneFrames++;
    if (sUsbDoneFrames >= 120) {
        DWCUtilMain_SetUpdate(DWCUtilUsbDone_FadeOut);
    }
}
