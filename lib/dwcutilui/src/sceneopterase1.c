#include "sceneopterase1.h"
#include "button.h"
#include "common.h"
#include "dwc_util_main.h"
#include "effect.h"
#include "graphics.h"
#include "input.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "setting.h"
#include "sound.h"

static void DWCUtilInfoMenu_LoadErasedGraphics(void);
static void DWCUtilInfoMenu_ErasedFadeIn(void);
static void DWCUtilInfoMenu_ErasedWaitFadeIn(void);
static void DWCUtilInfoMenu_ErasedWaitButtons(void);
static void DWCUtilInfoMenu_ErasedMain(void);
static void DWCUtilInfoMenu_ErasedHandleInput(void);
static void func_ov182_021aeea4(void);
static void DWCUtilInfoMenu_ErasedHandleChoice(void);
static void DWCUtilInfoMenu_ErasedDisableButtons(void);
static void DWCUtilInfoMenu_ErasedFadeOut(void);
static void DWCUtilInfoMenu_PowerOff(void);

// Erases the settings, and tells the player to turn the console off
void DWCUtilInfoMenu_EraseSettings(void) {
    DWCUtilInfoMenu_LoadErasedGraphics();
    DWCi_CMNlMsgDraw(0x19);
    DWCi_SETTINGlEraseAll();
    DWCi_ChangeScene(DWCUtilInfoMenu_ErasedFadeIn);
}

static void DWCUtilInfoMenu_LoadErasedGraphics(void) {
    DWCi_CMNlLoadResource("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilInfoMenu_ErasedFadeIn(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlVisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(DWCUtilInfoMenu_ErasedWaitFadeIn);
}

static void DWCUtilInfoMenu_ErasedWaitFadeIn(void) {
    if (DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(5);
        DWCi_ChangeScene(DWCUtilInfoMenu_ErasedWaitButtons);
    }
}

static void DWCUtilInfoMenu_ErasedWaitButtons(void) {
    if (DWCi_BTNlGet() != -2) {
        DWCi_BTNlEnable();
        DWCi_ChangeScene(DWCUtilInfoMenu_ErasedMain);
    }
}

static void DWCUtilInfoMenu_ErasedMain(void) {
    DWCUtilInfoMenu_ErasedHandleInput();
    func_ov182_021aeea4();
    DWCUtilInfoMenu_ErasedHandleChoice();
}

static void DWCUtilInfoMenu_ErasedHandleInput(void) {
    if (DWCi_IPTlCheckTrigger(1)) {
        DWCi_BTNlSet(0);
    }
}

static void func_ov182_021aeea4(void) {
}

static void DWCUtilInfoMenu_ErasedHandleChoice(void) {
    if (DWCi_BTNlGet() == 0) {
        DWCi_SNDlPlay(6);
        DWCi_ChangeScene(DWCUtilInfoMenu_ErasedDisableButtons);
    }
}

static void DWCUtilInfoMenu_ErasedDisableButtons(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(DWCUtilInfoMenu_ErasedFadeOut);
}

static void DWCUtilInfoMenu_ErasedFadeOut(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, DWC_UTIL_FADE_PLANES, 0x40);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, DWC_UTIL_FADE_PLANES, 0x40);
        DWCi_ChangeScene(DWCUtilInfoMenu_PowerOff);
    }
}

// Turns the console off once both screens have faded out
static void DWCUtilInfoMenu_PowerOff(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        OS_SpinWaitSysCycles(0x800000);
        PM_ForceToPowerOff();
    }
}
