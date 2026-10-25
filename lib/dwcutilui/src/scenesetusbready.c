#include "scenesetusbready.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_display.h"
#include "dwc_util_info_menu.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "headline.h"
#include "nitro/gx.h"
#include "scenesetselectap.h"
#include "scenesetusbready1.h"
#include "sound.h"

static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static void draw(void);
static void button(void);
static void DWCUtilUsbIntro_DisableButtons(void);
static void endScene1(void);
static void endScene2(void);

// Whether the player chose to set up the connector
static u8 sUsbIntroYes;

void DWCUtilUsbIntro_Init(void) {
    sUsbIntroYes = FALSE;
    initGraphics();
    DWCi_HLlLoad(8);
    DWCi_COMMENTlDispEx(0x38, -1, 0);
    DWCi_CMNlStep(1);
    DWCi_CMNlMsgDraw(0x27);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCUtilGfxResource_Load("char/jbBgStep2.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/ybBgStep2.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/xb3Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void procInit(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(procInit1);
}

static void procInit1(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(6);
        DWCi_ChangeScene(procInit2);
    }
}

static void procInit2(void) {
    if (DWCi_BTNlGet() != -2) {
        DWCi_BTNlEnable();
        DWCi_ChangeScene(procScene);
    }
}

static void procScene(void) {
    input();
    draw();
    button();
}

static void input(void) {
    DWCUtilInfoMenu_EraseHandleInputInline();
}

static void draw(void) {
}

static void button(void) {
    switch (DWCi_BTNlGet()) {
    case 0:
        DWCUtilSound_PlaySe(7);
        break;
    case 1:
        DWCUtilSound_PlaySe(6);
        sUsbIntroYes = TRUE;
        break;
    default:
        return;
    }
    DWCi_ChangeScene(DWCUtilUsbIntro_DisableButtons);
}

static void DWCUtilUsbIntro_DisableButtons(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_BTNlEnd();
        if (!sUsbIntroYes) {
            DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        }
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

static void endScene2(void) {
    if (DWCi_EFFlCheck(0) == 0 && (sUsbIntroYes || DWCi_EFFlCheck(1) == 0) && DWCi_BTNlIsEnd()) {
        DWCi_FNTlDeleteBgLcd(0);
        if (!sUsbIntroYes) {
            DWCi_COMMENTlDelete();
            DWCUtilDisplay_HidePlanes(1, GX_PLANEMASK_BG0);
        }
        DWCUtilDisplay_HidePlanes(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        if (!sUsbIntroYes) {
            DWCi_SetParam_(2, 1);
            DWCi_ChangeScene(DWCUtilScreenAp_Init);
        } else {
            DWCi_SetParam_(2, 1);
            DWCi_ChangeScene(DWCUtilUsbSetup_Init);
        }
    }
}
