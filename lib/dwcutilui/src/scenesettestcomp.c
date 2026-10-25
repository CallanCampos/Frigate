#include "scenesettestcomp.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "nitro/gx.h"
#include "sceneend.h"
#include "scenemenu.h"
#include "scenesetlist.h"
#include "step.h"

static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procScene(void);
static void draw(void);
static void DWCUtilAutoConnectOk_FadeOut(void);
static void endScene1(void);
static void wait(void);

// The frames since the message appeared
static u8 sAutoConnectOkFrames;

void DWCi_SceneSetTestCompletet(void) {
    sAutoConnectOkFrames = 0;
    initGraphics();
    DWCi_CMNlMsgDraw(0x2d);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCi_CMNlLoadResource("char/xb4Multi.nsc.l", GX_LoadBG2Scr);
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

static void DWCUtilAutoConnectOk_FadeOut(void) {
    int from;

    DWCi_GetParam2_(NULL, &from);
    if (from != 0) {
        DWCi_STEPlEnd();
    }
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_ChangeScene(endScene1);
}

// Once the fades and the buttons are done, back to the settings of the connection, or to the menu that the utility
// was started from
static void endScene1(void) {
    int from;

    if (DWCi_EFFlCheck(1) != 0 || DWCi_EFFlCheck(0) != 0) {
        return;
    }
    if (!DWCi_STEPlIsEnd()) {
        return;
    }
    DWCi_FNTlDeleteBgLcd(0);
    DWCi_COMMENTlDelete();
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_SetParam_(2, 1);
    DWCi_GetParam2_(NULL, &from);
    if (from == 0) {
        DWCi_SetParam2_(0, 0);
        DWCi_ChangeScene(DWCUtilAddressSettings_Init);
    } else {
        DWCi_ChangeScene(DWCi_SceneMenu);
    }
}

// After three seconds, ends the utility's run when it was started for the connection test alone
static void wait(void) {
    int from;

    sAutoConnectOkFrames++;
    if (sAutoConnectOkFrames >= 180) {
        DWCi_GetParam2_(NULL, &from);
        switch (DWCi_Entry()) {
        case 0:
            break;
        case 1:
            if (from != 0) {
                DWCi_ChangeScene(DWCi_SceneEnd);
                return;
            }
            break;
        }
        DWCi_ChangeScene(DWCUtilAutoConnectOk_FadeOut);
    }
}
