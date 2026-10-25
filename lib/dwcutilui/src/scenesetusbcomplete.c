#include "scenesetusbcomplete.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "graphics.h"
#include "headline.h"
#include "nitro/gx.h"
#include "scenesettestconfirm.h"
#include "sound.h"

static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procScene(void);
static void draw(void);
static void DWCUtilUsbDone_FadeOut(void);
static void endScene1(void);
static void wait(void);

// The frames since the message appeared
static u8 sUsbDoneFrames;

void DWCi_SceneSetUsbComplete(void) {
    sUsbDoneFrames = 0;
    initGraphics();
    DWCi_HLlLoad(8);
    DWCi_CMNlMsgDraw(0x29);
    DWCi_SNDlPlay(0x10);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCi_CMNlLoadResource("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
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

static void DWCUtilUsbDone_FadeOut(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_ChangeScene(endScene1);
}

// Once both screens have faded out, goes on to the message before the connection test with auto-connect
static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_FNTlDeleteBgLcd(0);
        DWCi_COMMENTlDelete();
        DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
        DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCi_SetParam_(0, 0);
        DWCi_SetParam2_(0, 2);
        DWCi_ChangeScene(DWCi_SceneSetTestConfirm);
    }
}

static void wait(void) {
    sUsbDoneFrames++;
    if (sUsbDoneFrames >= 120) {
        DWCi_ChangeScene(DWCUtilUsbDone_FadeOut);
    }
}
