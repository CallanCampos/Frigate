#include "dwc_util_usb_intro.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_info_menu.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_ap.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_text.h"
#include "dwc_util_ui_object.h"
#include "dwc_util_usb_setup.h"
#include "nitro/gx.h"

static void DWCUtilUsbIntro_LoadGraphics(void);
static void DWCUtilUsbIntro_FadeIn(void);
static void DWCUtilUsbIntro_WaitFadeIn(void);
static void DWCUtilUsbIntro_WaitButtons(void);
static void DWCUtilUsbIntro_Main(void);
static void DWCUtilUsbIntro_HandleInput(void);
static void func_ov182_021b69f0(void);
static void DWCUtilUsbIntro_HandleChoice(void);
static void DWCUtilUsbIntro_DisableButtons(void);
static void DWCUtilUsbIntro_FadeOut(void);
static void DWCUtilUsbIntro_WaitFadeOut(void);

// Whether the player chose to set up the connector
static u8 sUsbIntroYes;

void DWCUtilUsbIntro_Init(void) {
    sUsbIntroYes = FALSE;
    DWCUtilUsbIntro_LoadGraphics();
    DWCUtilBgOverlay_Show(8);
    DWCUtilScreen_Start(0x38, -1, 0);
    DWCUtilGfxResource_ShowStep(1);
    DWCUtilGfxResource_ShowMessage(0x27);
    DWCUtilMain_SetUpdate(DWCUtilUsbIntro_FadeIn);
}

static void DWCUtilUsbIntro_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/jbBgStep2.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/ybBgStep2.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/xb3Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilUsbIntro_FadeIn(void) {
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCUtilDisplay_ShowPlanes(1, GX_PLANEMASK_BG0);
    DWCUtilDisplay_ShowPlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCUtilMain_SetUpdate(DWCUtilUsbIntro_WaitFadeIn);
}

static void DWCUtilUsbIntro_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(6);
        DWCUtilMain_SetUpdate(DWCUtilUsbIntro_WaitButtons);
    }
}

static void DWCUtilUsbIntro_WaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilUsbIntro_Main);
    }
}

static void DWCUtilUsbIntro_Main(void) {
    DWCUtilUsbIntro_HandleInput();
    func_ov182_021b69f0();
    DWCUtilUsbIntro_HandleChoice();
}

static void DWCUtilUsbIntro_HandleInput(void) {
    DWCUtilInfoMenu_EraseHandleInputInline();
}

static void func_ov182_021b69f0(void) {
}

static void DWCUtilUsbIntro_HandleChoice(void) {
    switch (DWCUtilUiObject_GetState()) {
    case 0:
        DWCUtilSound_PlaySe(7);
        break;
    case 1:
        DWCUtilSound_PlaySe(6);
        sUsbIntroYes = TRUE;
        break;
    default:
        return;
    }
    DWCUtilMain_SetUpdate(DWCUtilUsbIntro_DisableButtons);
}

static void DWCUtilUsbIntro_DisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilUsbIntro_FadeOut);
}

static void DWCUtilUsbIntro_FadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        if (!sUsbIntroYes) {
            DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        }
        DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 0,
                          GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCUtilMain_SetUpdate(DWCUtilUsbIntro_WaitFadeOut);
    }
}

static void DWCUtilUsbIntro_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(0) == 0 && (sUsbIntroYes || DWCUtilFade_IsActive(1) == 0) && DWCUtilUiObject_IsIdle()) {
        DWCUtilText_DestroyScreenBg(0);
        if (!sUsbIntroYes) {
            DWCUtilScreen_Stop();
            DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        }
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        if (!sUsbIntroYes) {
            DWCUtilMain_SetLayout(2, 1);
            DWCUtilMain_SetUpdate(DWCUtilScreenAp_Init);
        } else {
            DWCUtilMain_SetLayout(2, 1);
            DWCUtilMain_SetUpdate(DWCUtilUsbSetup_Init);
        }
    }
}
