#include "dwc_util_save_settings.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_dialog.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_ap.h"
#include "dwc_util_shared.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/gx.h"

static void func_ov182_021b4420(void);
static void func_ov182_021b4488(void);
static void func_ov182_021b44c0(void);
static void func_ov182_021b44f4(void);
static void func_ov182_021b4514(void);
static void func_ov182_021b4518(void);
static void func_ov182_021b451c(void);
static void func_ov182_021b4544(void);

void func_ov182_021b4404(void) {
    func_ov182_021b4420();
    DWCUtilBgOverlay_ShowConnectionSet();
    func_ov182_021b7bb0();
    DWCUtilMain_SetUpdate(func_ov182_021b4488);
}

static void func_ov182_021b4420(void) {
    DWCUtilGfxResource_Load("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/xb4None.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b4488(void) {
    DWCUtilFade_Start(2, 1, 1, 8);
    DWCUtilFade_Start(2, 0, 0x14, 8);
    DWCUtilDisplay_ShowPlanes(1, 1);
    DWCUtilDisplay_ShowPlanes(0, 0x14);
    DWCUtilMain_SetUpdate(func_ov182_021b44c0);
}

// Shows the message of the save once the screens have faded in
static void func_ov182_021b44c0(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilDialog_Open(0x44, 5, 1, -1, 0);
        DWCUtilMain_SetUpdate(func_ov182_021b44f4);
    }
}

static void func_ov182_021b44f4(void) {
    func_ov182_021b4514();
    func_ov182_021b4518();
    if (!DWCUtilDialog_IsOpen()) {
        DWCUtilMain_SetUpdate(func_ov182_021b451c);
    }
}

static void func_ov182_021b4514(void) {
}

static void func_ov182_021b4518(void) {
}

static void func_ov182_021b451c(void) {
    DWCUtilFade_Start(3, 1, 1, 8);
    DWCUtilFade_Start(3, 0, 0x14, 8);
    DWCUtilMain_SetUpdate(func_ov182_021b4544);
}

static void func_ov182_021b4544(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilScreen_Stop();
        DWCUtilDisplay_HidePlanes(1, 1);
        DWCUtilDisplay_HidePlanes(0, 0x14);
        DWCUtilMain_SetLayout(2, 1);
        DWCUtilMain_SetUpdate(func_ov182_021b536c);
    }
}
