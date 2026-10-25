#include "scenesetrakucomplete.h"
#include "common.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "graphics.h"
#include "headline.h"
#include "main.h"
#include "nitro/gx.h"
#include "scenesetaosscomplete.h"
#include "sound.h"

static void initGraphics(void);
static void procInit(void);
static void DWCUtilTestDone_WaitFadeIn(void);
static void DWCUtilTestDone_Main(void);
static void draw(void);
static void endScene(void);
static void endScene1(void);
static void wait(void);

// The frames since the message appeared
static u8 sTestDoneFrames;

void DWCi_SceneSetRakuComplete(void) {
    sTestDoneFrames = 0;
    initGraphics();
    DWCi_HLlLoadSet();
    DWCi_CMNlMsgDraw(0x26);
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
    DWCi_ChangeScene(DWCUtilTestDone_WaitFadeIn);
}

static void DWCUtilTestDone_WaitFadeIn(void) {
    if (DWCi_EFFlCheck(0) == 0) {
        DWCi_ChangeScene(DWCUtilTestDone_Main);
    }
}

static void DWCUtilTestDone_Main(void) {
    wait();
    draw();
}

static void draw(void) {
}

static void endScene(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    DWCUtilPageMulti4_WaitFadeOutInline();
}

static void wait(void) {
    sTestDoneFrames++;
    if (sTestDoneFrames >= 120) {
        DWCi_ChangeScene(endScene);
    }
}
