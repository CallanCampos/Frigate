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

static void func_ov182_021afbd8(void);
static void func_ov182_021afc28(void);
static void func_ov182_021afc60(void);
static void func_ov182_021afc80(void);
static void func_ov182_021afca0(void);
static void func_ov182_021afcb0(void);
static void func_ov182_021afcd8(void);
static void func_ov182_021afcdc(void);
static void func_ov182_021afd10(void);
static void func_ov182_021afd28(void);
static void func_ov182_021afd6c(void);

// Whether the player answered yes
static u8 sYes;

void func_ov182_021afba4(void) {
    sYes = FALSE;
    func_ov182_021afbd8();
    DWCUtilBgOverlay_Show(0x13);
    DWCUtilScreen_Start(0x3c, -1, 0);
    DWCUtilGfxResource_ShowMessage(0x1b);
    DWCUtilMain_SetUpdate(func_ov182_021afc28);
}

static void func_ov182_021afbd8(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021afc28(void) {
    DWCUtilFade_Start(2, 1, 1, 8);
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(1, 1);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(func_ov182_021afc60);
}

static void func_ov182_021afc60(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(3);
        DWCUtilMain_SetUpdate(func_ov182_021afc80);
    }
}

static void func_ov182_021afc80(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(func_ov182_021afca0);
    }
}

static void func_ov182_021afca0(void) {
    func_ov182_021afcb0();
    func_ov182_021afcd8();
    func_ov182_021afcdc();
}

static void func_ov182_021afcb0(void) {
    func_ov182_021aec10Inline();
}

static void func_ov182_021afcd8(void) {
}

static void func_ov182_021afcdc(void) {
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
    DWCUtilMain_SetUpdate(func_ov182_021afd10);
}

static void func_ov182_021afd10(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(func_ov182_021afd28);
}

static void func_ov182_021afd28(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        if (!sYes) {
            DWCUtilFade_Start(3, 1, 1, 8);
        }
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(func_ov182_021afd6c);
    }
}

static void func_ov182_021afd6c(void) {
    if (DWCUtilFade_IsActive(0) == 0 && (sYes || DWCUtilFade_IsActive(1) == 0) && DWCUtilUiObject_IsIdle()) {
        DWCUtilText_DestroyScreenBg(0);
        if (!sYes) {
            DWCUtilScreen_Stop();
            DWCUtilDisplay_HidePlanes(1, 1);
        }
        DWCUtilDisplay_HidePlanes(0, 0x15);
        if (!sYes) {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetUpdate(func_ov182_021af25c);
        } else {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetUpdate(func_ov182_021afa28);
        }
    }
}
