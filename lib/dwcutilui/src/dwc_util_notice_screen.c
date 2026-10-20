#include "dwc_util_notice_screen.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_info_menu.h"
#include "dwc_util_main.h"
#include "dwc_util_move.h"
#include "dwc_util_option_menu.h"
#include "dwc_util_page_multi5.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "nitro/gx.h"

// Whether the sub screen shows a message, and the player's answer
typedef struct DWCUtilNoticeScreen {
    u8 subScreen;
    u8 yes;
} DWCUtilNoticeScreen;

static void DWCUtilNoticeScreen_LoadGraphics(void);
static void DWCUtilNoticeScreen_FadeIn(void);
static void DWCUtilNoticeScreen_WaitFadeIn(void);
static void DWCUtilNoticeScreen_WaitButtons(void);
static void DWCUtilNoticeScreen_Main(void);
static void DWCUtilNoticeScreen_HandleInput(void);
static void func_ov182_021b0088(void);
static void DWCUtilNoticeScreen_HandleChoice(void);
static void DWCUtilNoticeScreen_DisableButtons(void);
static void DWCUtilNoticeScreen_FadeOut(void);
static void DWCUtilNoticeScreen_WaitFadeOut(void);

static DWCUtilNoticeScreen sNoticeScreen;

void DWCUtilNoticeScreen_Init(void) {
    DWCUtilNoticeScreen_LoadGraphics();
    DWCUtilGfxResource_ShowMessage(0x20);
    sNoticeScreen.subScreen = DWCUtilScreen_Start(0x3c, -1, 0);
    DWCUtilMain_SetUpdate(DWCUtilNoticeScreen_FadeIn);
}

static void DWCUtilNoticeScreen_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilNoticeScreen_FadeIn(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    if (sNoticeScreen.subScreen) {
        DWCUtilFade_Start(2, 1, 1, 8);
        DWCUtilDisplay_ShowPlanes(1, 1);
    }
    DWCUtilMain_SetUpdate(DWCUtilNoticeScreen_WaitFadeIn);
}

static void DWCUtilNoticeScreen_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(3);
        DWCUtilMain_SetUpdate(DWCUtilNoticeScreen_WaitButtons);
    }
}

static void DWCUtilNoticeScreen_WaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2 && DWCUtilMove_HasEnded()) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilNoticeScreen_Main);
    }
}

static void DWCUtilNoticeScreen_Main(void) {
    DWCUtilNoticeScreen_HandleInput();
    func_ov182_021b0088();
    DWCUtilNoticeScreen_HandleChoice();
}

static void DWCUtilNoticeScreen_HandleInput(void) {
    DWCUtilInfoMenu_EraseHandleInputInline();
}

static void func_ov182_021b0088(void) {
}

static void DWCUtilNoticeScreen_HandleChoice(void) {
    switch (DWCUtilUiObject_GetState()) {
    case 1:
        sNoticeScreen.yes = TRUE;
        DWCUtilSound_PlaySe(6);
        break;
    case 0:
        sNoticeScreen.yes = FALSE;
        DWCUtilSound_PlaySe(7);
        break;
    default:
        return;
    }
    DWCUtilMain_SetUpdate(DWCUtilNoticeScreen_DisableButtons);
}

static void DWCUtilNoticeScreen_DisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilNoticeScreen_FadeOut);
}

static void DWCUtilNoticeScreen_FadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        if (!sNoticeScreen.yes) {
            DWCUtilFade_Start(3, 1, 1, 8);
        }
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(DWCUtilNoticeScreen_WaitFadeOut);
    }
}

static void DWCUtilNoticeScreen_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0 && DWCUtilUiObject_IsIdle()) {
        DWCUtilText_DestroyScreenBg(0);
        if (!sNoticeScreen.yes) {
            DWCUtilScreen_Stop();
            DWCUtilDisplay_HidePlanes(1, 1);
        }
        DWCUtilDisplay_HidePlanes(0, 0x15);
        if (!sNoticeScreen.yes) {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetUpdate(DWCUtilOptionMenu_Init);
        } else {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetUpdate(DWCUtilPageMulti5_Init);
        }
    }
}
