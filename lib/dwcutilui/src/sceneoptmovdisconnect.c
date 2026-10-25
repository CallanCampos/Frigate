#include "sceneoptmovdisconnect.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_main.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "move.h"
#include "nitro/gx.h"
#include "sceneoptmenu.h"

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

void DWCi_SceneOptMovDisconnect(void) {
    sFrames = 0;
    initGraphics();
    DWCi_CMNlMsgDraw(0x21);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCi_CMNlLoadResource("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlVisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
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
        DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
        DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCi_SetParam_(0, 1);
        DWCi_ChangeScene(DWCi_SceneOptMenu);
    }
}

static void wait(void) {
    sFrames++;
    if (sFrames >= 120) {
        DWCi_ChangeScene(DWCUtilTimedNotice_FadeOut);
    }
}
