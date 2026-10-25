#include "sceneoptmovcomplete.h"
#include "button.h"
#include "common.h"
#include "dwc_util_main.h"
#include "effect.h"
#include "graphics.h"
#include "input.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "sound.h"

static void initGraphics(void);
static void DWCUtilTopMenu_SentFadeIn(void);
static void DWCUtilTopMenu_SentWaitFadeIn(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static void func_ov182_021af998(void);
static void button(void);
static void DWCUtilTopMenu_SentDisableButtons(void);
static void endScene1(void);
static void DWCUtilTopMenu_PowerOff(void);

// The settings were sent: the player is told to turn the console off
void DWCi_SceneOptMovConfirm(void) {
    initGraphics();
    DWCi_CMNlMsgDraw(0x1f);
    DWCi_ChangeScene(DWCUtilTopMenu_SentFadeIn);
}

static void initGraphics(void) {
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilTopMenu_SentFadeIn(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(DWCUtilTopMenu_SentWaitFadeIn);
}

static void DWCUtilTopMenu_SentWaitFadeIn(void) {
    if (DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(5);
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
    func_ov182_021af998();
    button();
}

static void input(void) {
    if (DWCi_IPTlCheckTrigger(1)) {
        DWCi_BTNlSet(0);
    }
}

static void func_ov182_021af998(void) {
}

static void button(void) {
    if (DWCi_BTNlGet() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCi_ChangeScene(DWCUtilTopMenu_SentDisableButtons);
    }
}

static void DWCUtilTopMenu_SentDisableButtons(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, DWC_UTIL_FADE_PLANES, 0x40);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, DWC_UTIL_FADE_PLANES, 0x40);
        DWCi_ChangeScene(DWCUtilTopMenu_PowerOff);
    }
}

// Turns the console off once both screens have faded out
static void DWCUtilTopMenu_PowerOff(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        OS_SpinWaitSysCycles(0x800000);
        PM_ForceToPowerOff();
    }
}
