#include "dwc_util_auto_connect_ok.h"
#include "dwc_util_address_settings.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_main_menu.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_sub_screen.h"
#include "dwc_util_text.h"
#include "nitro/gx.h"

static void DWCUtilAutoConnectOk_LoadGraphics(void);
static void DWCUtilAutoConnectOk_FadeIn(void);
static void DWCUtilAutoConnectOk_WaitFadeIn(void);
static void DWCUtilAutoConnectOk_Main(void);
static void func_ov182_021b6490(void);
static void DWCUtilAutoConnectOk_FadeOut(void);
static void DWCUtilAutoConnectOk_WaitFadeOut(void);
static void DWCUtilAutoConnectOk_UpdateTimer(void);

// The frames since the message appeared
static u8 sAutoConnectOkFrames;

void DWCUtilAutoConnectOk_Init(void) {
    sAutoConnectOkFrames = 0;
    DWCUtilAutoConnectOk_LoadGraphics();
    DWCUtilGfxResource_ShowMessage(0x2d);
    DWCUtilMain_SetUpdate(DWCUtilAutoConnectOk_FadeIn);
}

static void DWCUtilAutoConnectOk_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilAutoConnectOk_FadeIn(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilAutoConnectOk_WaitFadeIn);
}

static void DWCUtilAutoConnectOk_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilMain_SetUpdate(DWCUtilAutoConnectOk_Main);
    }
}

static void DWCUtilAutoConnectOk_Main(void) {
    DWCUtilAutoConnectOk_UpdateTimer();
    func_ov182_021b6490();
}

static void func_ov182_021b6490(void) {
}

static void DWCUtilAutoConnectOk_FadeOut(void) {
    int from;

    DWCUtilMain_GetScreenArgs(NULL, &from);
    if (from != 0) {
        DWCUtilSubScreen_SlideOutRow();
    }
    DWCUtilFade_Start(3, 1, 1, 8);
    DWCUtilFade_Start(3, 0, 0x15, 8);
    DWCUtilMain_SetUpdate(DWCUtilAutoConnectOk_WaitFadeOut);
}

// Once the fades and the buttons are done, back to the settings of the connection, or to the menu that the utility
// was started from
static void DWCUtilAutoConnectOk_WaitFadeOut(void) {
    int from;

    if (DWCUtilFade_IsActive(1) != 0 || DWCUtilFade_IsActive(0) != 0) {
        return;
    }
    if (!DWCUtilSubScreen_IsNotSlidingOut()) {
        return;
    }
    DWCUtilText_DestroyScreenBg(0);
    DWCUtilScreen_Stop();
    DWCUtilDisplay_HidePlanes(1, 1);
    DWCUtilDisplay_HidePlanes(0, 0x15);
    DWCUtilMain_SetLayout(2, 1);
    DWCUtilMain_GetScreenArgs(NULL, &from);
    if (from == 0) {
        DWCUtilMain_SetScreenArgs(0, 0);
        DWCUtilMain_SetUpdate(DWCUtilAddressSettings_Init);
    } else {
        DWCUtilMain_SetUpdate(func_ov182_021ae748);
    }
}

// After three seconds, ends the utility's run when it was started for the connection test alone
static void DWCUtilAutoConnectOk_UpdateTimer(void) {
    int from;

    sAutoConnectOkFrames++;
    if (sAutoConnectOkFrames >= 180) {
        DWCUtilMain_GetScreenArgs(NULL, &from);
        switch (DWCUtilMain_GetStartScreen()) {
        case 0:
            break;
        case 1:
            if (from != 0) {
                DWCUtilMain_SetUpdate(func_ov182_021ae4f4);
                return;
            }
            break;
        }
        DWCUtilMain_SetUpdate(DWCUtilAutoConnectOk_FadeOut);
    }
}
