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

static void func_ov182_021b65c8(void);
static void func_ov182_021b6610(void);
static void func_ov182_021b6648(void);
static void func_ov182_021b66a0(void);
static void func_ov182_021b66d0(void);
static void func_ov182_021b66d4(void);
static void func_ov182_021b66d8(void);
static void func_ov182_021b66fc(void);

void func_ov182_021b6588(void) {
    int from;

    func_ov182_021b74f4();
    DWCUtilMain_GetScreenArgs(NULL, &from);
    func_ov182_021b65c8();
    DWCUtilScreen_Start(0x31, -1, 0);
    if (from != 2) {
        DWCUtilBgOverlay_ShowConnectionSet();
    }
    if (from == 1) {
        func_ov182_021b7bb0();
    }
    DWCUtilMain_SetUpdate(func_ov182_021b6610);
}

static void func_ov182_021b65c8(void) {
    DWCUtilGfxResource_Load("char/xb4None.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b6610(void) {
    DWCUtilFade_Start(2, 1, 1, 8);
    DWCUtilFade_Start(2, 0, 0x14, 8);
    DWCUtilDisplay_ShowPlanes(1, 1);
    DWCUtilDisplay_ShowPlanes(0, 0x14);
    DWCUtilMain_SetUpdate(func_ov182_021b6648);
}

// Shows the message of where the utility was started from in a dialog with one button
static void func_ov182_021b6648(void) {
    int msgIndices[3] = { 0x2b, 0x2b, 0x49 };
    int from;

    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilMain_GetScreenArgs(NULL, &from);
        func_ov182_021b8cf0(msgIndices[from], 1, 1, -1, 0);
        DWCUtilMain_SetUpdate(func_ov182_021b66a0);
    }
}

static void func_ov182_021b66a0(void) {
    func_ov182_021b66d0();
    func_ov182_021b66d4();
    if (func_ov182_021b8f14() == 0) {
        DWCUtilSound_PlaySe(6);
        func_ov182_021b8eec();
        DWCUtilSound_PlaySe(6);
        DWCUtilMain_SetUpdate(func_ov182_021b66d8);
    }
}

static void func_ov182_021b66d0(void) {
}

static void func_ov182_021b66d4(void) {
}

static void func_ov182_021b66d8(void) {
    if (!func_ov182_021b8f24()) {
        DWCUtilFade_Start(3, 0, 0x14, 8);
        DWCUtilMain_SetUpdate(func_ov182_021b66fc);
    }
}

static void func_ov182_021b66fc(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilDisplay_HidePlanes(0, 0x14);
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(func_ov182_021b6104);
    }
}
