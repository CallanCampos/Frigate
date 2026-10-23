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

static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void DWCUtilAutoConnectIntro_WaitDialog(void);
static void draw(void);
static void func_ov182_021b66d4(void);
static void endScene1(void);
static void DWCUtilAutoConnectIntro_WaitFadeOut(void);

void DWCUtilAutoConnectIntro_Init(void) {
    int from;

    DWCi_SEARCHlEnd();
    DWCi_GetParam2_(NULL, &from);
    initGraphics();
    DWCi_COMMENTlDispEx(0x31, -1, 0);
    if (from != 2) {
        DWCi_HLlLoadSet();
    }
    if (from == 1) {
        DWCi_SETTINGlSet();
    }
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCUtilGfxResource_Load("char/xb4None.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(procInit1);
}

// Shows the message of where the utility was started from in a dialog with one button
static void procInit1(void) {
    int msgIndices[3] = { 0x2b, 0x2b, 0x49 };
    int from;

    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_GetParam2_(NULL, &from);
        DWCUtilDialog_Open(msgIndices[from], 1, 1, -1, 0);
        DWCi_ChangeScene(DWCUtilAutoConnectIntro_WaitDialog);
    }
}

static void DWCUtilAutoConnectIntro_WaitDialog(void) {
    draw();
    func_ov182_021b66d4();
    if (DWCi_WINlGet() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCi_WINlEnd();
        DWCUtilSound_PlaySe(6);
        DWCi_ChangeScene(endScene1);
    }
}

static void draw(void) {
}

static void func_ov182_021b66d4(void) {
}

static void endScene1(void) {
    if (!DWCi_WINlIsInit()) {
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(DWCUtilAutoConnectIntro_WaitFadeOut);
    }
}

static void DWCUtilAutoConnectIntro_WaitFadeOut(void) {
    if (DWCi_EFFlCheck(0) == 0) {
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCi_SetParam_(0, 1);
        DWCi_ChangeScene(DWCUtilAutoConnect_Init);
    }
}
