#include "dwc_util_prompt_yes_no.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_info_menu.h"
#include "dwc_util_main.h"
#include "dwc_util_option_menu.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_text.h"
#include "dwc_util_top_menu.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "nitro/gx.h"

static void DWCUtilPromptYesNo_LoadGraphics(void);
static void DWCUtilPromptYesNo_FadeIn(void);
static void DWCUtilPromptYesNo_WaitFadeIn(void);
static void DWCUtilPromptYesNo_WaitButtons(void);
static void DWCUtilPromptYesNo_Main(void);
static void DWCUtilPromptYesNo_HandleInput(void);
static void func_ov182_021afcd8(void);
static void DWCUtilPromptYesNo_HandleChoice(void);
static void DWCUtilPromptYesNo_DisableButtons(void);
static void DWCUtilPromptYesNo_FadeOut(void);
static void DWCUtilPromptYesNo_WaitFadeOut(void);

// Whether the player answered yes
static u8 sYes;

void DWCUtilPromptYesNo_Init(void) {
    sYes = FALSE;
    DWCUtilPromptYesNo_LoadGraphics();
    DWCUtilBgOverlay_Show(0x13);
    DWCUtilScreen_Start(0x3c, -1, 0);
    DWCUtilGfxResource_ShowMessage(0x1b);
    DWCUtilMain_SetUpdate(DWCUtilPromptYesNo_FadeIn);
}

static void DWCUtilPromptYesNo_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilPromptYesNo_FadeIn(void) {
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCUtilFade_Start(DWC_UTIL_FADE_FROM_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCUtilDisplay_ShowPlanes(1, GX_PLANEMASK_BG0);
    DWCUtilDisplay_ShowPlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCUtilMain_SetUpdate(DWCUtilPromptYesNo_WaitFadeIn);
}

static void DWCUtilPromptYesNo_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(3);
        DWCUtilMain_SetUpdate(DWCUtilPromptYesNo_WaitButtons);
    }
}

static void DWCUtilPromptYesNo_WaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilPromptYesNo_Main);
    }
}

static void DWCUtilPromptYesNo_Main(void) {
    DWCUtilPromptYesNo_HandleInput();
    func_ov182_021afcd8();
    DWCUtilPromptYesNo_HandleChoice();
}

static void DWCUtilPromptYesNo_HandleInput(void) {
    DWCUtilInfoMenu_EraseHandleInputInline();
}

static void func_ov182_021afcd8(void) {
}

static void DWCUtilPromptYesNo_HandleChoice(void) {
    switch (DWCUtilUiObject_GetState()) {
    case 0:
        DWCUtilSound_PlaySe(7);
        break;
    case 1:
        DWCUtilSound_PlaySe(6);
        sYes = TRUE;
        break;
    default:
        return;
    }
    DWCUtilMain_SetUpdate(DWCUtilPromptYesNo_DisableButtons);
}

static void DWCUtilPromptYesNo_DisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilPromptYesNo_FadeOut);
}

static void DWCUtilPromptYesNo_FadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        if (!sYes) {
            DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        }
        DWCUtilFade_Start(DWC_UTIL_FADE_TO_WHITE, 0,
                          GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCUtilMain_SetUpdate(DWCUtilPromptYesNo_WaitFadeOut);
    }
}

static void DWCUtilPromptYesNo_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(0) == 0 && (sYes || DWCUtilFade_IsActive(1) == 0) && DWCUtilUiObject_IsIdle()) {
        DWCUtilText_DestroyScreenBg(0);
        if (!sYes) {
            DWCUtilScreen_Stop();
            DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        }
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        if (!sYes) {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetUpdate(DWCUtilOptionMenu_Init);
        } else {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetUpdate(DWCUtilTopMenu_InitInstructions);
        }
    }
}
