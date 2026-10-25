#include "scenesetlistsave.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "graphics.h"
#include "headline.h"
#include "nitro/gx.h"
#include "scenesetselectap.h"
#include "setting.h"
#include "window.h"

static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procScene(void);
static void input(void);
static void draw(void);
static void DWCUtilSaveSettings_FadeOut(void);
static void endScene1(void);

void DWCUtilSaveSettings_Init(void) {
    initGraphics();
    DWCi_HLlLoadSet();
    DWCi_SETTINGlSet();
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCUtilGfxResource_Load("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
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

// Shows the message of the save once the screens have faded in
static void procInit1(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCUtilDialog_Open(0x44, 5, 1, -1, 0);
        DWCi_ChangeScene(procScene);
    }
}

static void procScene(void) {
    input();
    draw();
    if (!DWCi_WINlIsInit()) {
        DWCi_ChangeScene(DWCUtilSaveSettings_FadeOut);
    }
}

static void input(void) {
}

static void draw(void) {
}

static void DWCUtilSaveSettings_FadeOut(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_COMMENTlDelete();
        DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCi_SetParam_(2, 1);
        DWCi_ChangeScene(DWCUtilScreenAp_Init);
    }
}
