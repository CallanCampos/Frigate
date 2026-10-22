#include "dwc_util_timed_notice.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_move.h"
#include "dwc_util_option_menu.h"
#include "dwc_util_screen.h"
#include "dwc_util_text.h"
#include "nitro/gx.h"

static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procScene(void);
static void draw(void);
static void DWCUtilTimedNotice_FadeOut(void);
static void endScene1(void);
static void wait(void);

// The frames the notice has been shown
static u8 sFrames;

void DWCUtilTimedNotice_Init(void) {
    sFrames = 0;
    initGraphics();
    DWCi_CMNlMsgDraw(0x21);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(procInit1);
}

static void procInit1(void) {
    if (DWCi_EFFlCheck(0) == 0 && DWCi_MOVlIsEnd()) {
        DWCi_ChangeScene(procScene);
    }
}

static void procScene(void) {
    wait();
    draw();
}

static void draw(void) {
}

static void DWCUtilTimedNotice_FadeOut(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_FNTlDeleteBgLcd(0);
        DWCi_COMMENTlDelete();
        DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCi_SetParam_(0, 1);
        DWCi_ChangeScene(DWCUtilOptionMenu_Init);
    }
}

static void wait(void) {
    sFrames++;
    if (sFrames >= 120) {
        DWCi_ChangeScene(DWCUtilTimedNotice_FadeOut);
    }
}
