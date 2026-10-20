#include "dwc_util_ap_search.h"
#include "dwc_util_ap_scan.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_dialog.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_page_ap_list.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_way.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_effect.h"
#include "dwc_util_ui_object.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/gx.h"

// How the search ended, and the frames it has scanned for
typedef struct DWCUtilApSearch {
    u8 result; // 1 when it found an access point that the utility can set up, 2 when none, 3 when none of those
    u16 frames;
} DWCUtilApSearch;

static void DWCUtilApSearch_LoadGraphics(void);
static void DWCUtilApSearch_FadeIn(void);
static void DWCUtilApSearch_WaitFadeIn(void);
static void DWCUtilApSearch_WaitButtons(void);
static void DWCUtilApSearch_Main(void);
static void DWCUtilApSearch_CheckScan(void);
static void DWCUtilApSearch_HandleInput(void);
static void func_ov182_021b522c(void);
static void DWCUtilApSearch_HandleChoice(void);
static void DWCUtilApSearch_DisableButtons(void);
static void DWCUtilApSearch_FadeOut(void);
static void DWCUtilApSearch_WaitFadeOut(void);
static void DWCUtilApSearch_WaitDialog(void);
static void DWCUtilApSearch_WaitDialogClosed(void);

static DWCUtilApSearch sApSearch;

void DWCUtilApSearch_Init(void) {
    sApSearch.frames = 0;
    sApSearch.result = 0;
    DWCUtilApSearch_LoadGraphics();
    DWCUtilScreen_Start(0x33, -1, 0);
    DWCUtilBgOverlay_ShowConnectionSet();
    DWCUtilGfxResource_ShowStep(2);
    DWCUtilGfxResource_ShowMessage(0x33);
    DWCUtilUiEffect_Init(0);
    DWCUtilApScan_Init();
    DWCUtilApScan_Start();
    DWCUtilSound_PlaySe(0xa);
    DWCUtilMain_SetUpdate(DWCUtilApSearch_FadeIn);
}

static void DWCUtilApSearch_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilApSearch_FadeIn(void) {
    DWCUtilFade_Start(2, 1, 1, 8);
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(1, 1);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilApSearch_WaitFadeIn);
}

static void DWCUtilApSearch_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(1);
        DWCUtilMain_SetUpdate(DWCUtilApSearch_WaitButtons);
    }
}

static void DWCUtilApSearch_WaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilApSearch_Main);
    }
}

static void DWCUtilApSearch_Main(void) {
    DWCUtilApSearch_HandleInput();
    func_ov182_021b522c();
    DWCUtilApSearch_HandleChoice();
    DWCUtilApSearch_CheckScan();
}

// After 300 frames, looks at what the scan found
static void DWCUtilApSearch_CheckScan(void) {
    DWCUtilApEntry *entries;
    int count;
    int i;

    sApSearch.frames++;
    if (sApSearch.frames < 300) {
        return;
    }
    DWCUtilSound_StopSe();
    count = DWCUtilApScan_GetEntries(&entries);
    if (count == 0) {
        sApSearch.result = 2;
        DWCUtilDialog_Open(0xf, 1, 1, -1, 0);
        DWCUtilSound_PlaySe(0x12);
        DWCUtilUiObject_Disable();
        DWCUtilMain_SetUpdate(DWCUtilApSearch_WaitDialog);
        return;
    }
    for (i = 0; i < count; i++) {
        if (entries[i].security != 2) {
            break;
        }
    }
    if (i == count) {
        sApSearch.result = 3;
        DWCUtilDialog_Open(0xe, 1, 1, -1, 0);
        DWCUtilSound_PlaySe(0x12);
        DWCUtilUiObject_Disable();
        DWCUtilMain_SetUpdate(DWCUtilApSearch_WaitDialog);
        return;
    }
    sApSearch.result = 1;
    DWCUtilSound_PlaySe(0xf);
    DWCUtilMain_SetUpdate(DWCUtilApSearch_DisableButtons);
}

static void DWCUtilApSearch_HandleInput(void) {
    if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilSound_StopSe();
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b522c(void) {
}

// The button ends the search
static void DWCUtilApSearch_HandleChoice(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(7);
        DWCUtilMain_SetUpdate(DWCUtilApSearch_DisableButtons);
    }
}

static void DWCUtilApSearch_DisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilApSearch_FadeOut);
}

static void DWCUtilApSearch_FadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        DWCUtilFade_Start(3, 1, 1, 8);
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(DWCUtilApSearch_WaitFadeOut);
    }
}

static void DWCUtilApSearch_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0 && DWCUtilUiObject_IsIdle()) {
        while (!DWCUtilApScan_Stop()) {
        }
        DWCUtilUiEffect_Destroy();
        DWCUtilText_DestroyScreenBg(0);
        DWCUtilScreen_Stop();
        DWCUtilDisplay_HidePlanes(1, 1);
        DWCUtilDisplay_HidePlanes(0, 0x15);
        if (sApSearch.result != 1) {
            DWCUtilApScan_Exit();
            DWCUtilWifiSettings_EditConnection(DWCUtilWifiSettings_GetEdit()->index);
            DWCUtilMain_SetLayout(2, 1);
            DWCUtilMain_SetUpdate(DWCUtilScreenWay_Init);
        } else {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetScreenArgs(0, 0);
            DWCUtilMain_SetUpdate(DWCUtilPageApList_Init);
        }
    }
}

static void DWCUtilApSearch_WaitDialog(void) {
    if (DWCUtilDialog_GetChoice() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCUtilDialog_Close();
        DWCUtilMain_SetUpdate(DWCUtilApSearch_WaitDialogClosed);
    }
}

static void DWCUtilApSearch_WaitDialogClosed(void) {
    if (!DWCUtilDialog_IsOpen()) {
        DWCUtilMain_SetUpdate(DWCUtilApSearch_DisableButtons);
    }
}
