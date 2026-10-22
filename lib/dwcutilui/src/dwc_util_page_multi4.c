#include "dwc_util_page_multi4.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_text.h"
#include "nitro/gx.h"

static void initGraphics(void);
static void procInit(void);
static void DWCUtilPageMulti4_WaitFadeIn(void);
static void procScene(void);
static void draw(void);
static void DWCUtilPageMulti4_FadeOut(void);
static void DWCUtilPageMulti4_WaitFadeOut(void);
static void wait(void);

// The frames since the message appeared
static u8 sPageMulti4Frames;

void DWCUtilPageMulti4_Init(void) {
    sPageMulti4Frames = 0;
    initGraphics();
    DWCi_HLlLoadSet();
    DWCi_CMNlMsgDraw(0x23);
    DWCUtilSound_PlaySe(0x10);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCUtilGfxResource_Load("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(DWCUtilPageMulti4_WaitFadeIn);
}

static void DWCUtilPageMulti4_WaitFadeIn(void) {
    if (DWCi_EFFlCheck(0) == 0) {
        DWCi_ChangeScene(procScene);
    }
}

static void procScene(void) {
    wait();
    draw();
}

static void draw(void) {
}

static void DWCUtilPageMulti4_FadeOut(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_ChangeScene(DWCUtilPageMulti4_WaitFadeOut);
}

static void DWCUtilPageMulti4_WaitFadeOut(void) {
    DWCUtilPageMulti4_WaitFadeOutInline();
}

static void wait(void) {
    sPageMulti4Frames++;
    if (sPageMulti4Frames >= 120) {
        DWCi_ChangeScene(DWCUtilPageMulti4_FadeOut);
    }
}
