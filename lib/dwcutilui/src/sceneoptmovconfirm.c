#include "sceneoptmovconfirm.h"
#include "button.h"
#include "common.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "input.h"
#include "main.h"
#include "nitro/gx.h"
#include "sceneoptmovwait.h"
#include "sound.h"

static void DWCUtilTopMenu_SetInstructionsPriorities(void);
static void DWCUtilTopMenu_InstructionsFadeIn(void);
static void DWCUtilTopMenu_InstructionsWaitFadeIn(void);
static void DWCUtilTopMenu_InstructionsWaitButtons(void);
static void DWCUtilTopMenu_InstructionsMain(void);
static void DWCUtilTopMenu_InstructionsHandleInput(void);
static void func_ov182_021afb0c(void);
static void DWCUtilTopMenu_InstructionsHandleChoice(void);
static void DWCUtilTopMenu_InstructionsDisableButtons(void);
static void DWCUtilTopMenu_InstructionsFadeOut(void);
static void DWCUtilTopMenu_InstructionsWaitFadeOut(void);

void DWCi_SceneOptMovConfirm(void) {
    DWCUtilTopMenu_SetInstructionsPriorities();
    DWCi_CMNlMsgDraw(0x1a);
    DWCi_ChangeScene(DWCUtilTopMenu_InstructionsFadeIn);
}

static void DWCUtilTopMenu_SetInstructionsPriorities(void) {
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilTopMenu_InstructionsFadeIn(void) {
    DWCi_EFFlFade(DWC_UTIL_FADE_FROM_WHITE, 0, GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ,
                  8);
    DWCi_GXlVisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
    DWCi_ChangeScene(DWCUtilTopMenu_InstructionsWaitFadeIn);
}

static void DWCUtilTopMenu_InstructionsWaitFadeIn(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        DWCi_CMNlButton(4);
        DWCi_ChangeScene(DWCUtilTopMenu_InstructionsWaitButtons);
    }
}

static void DWCUtilTopMenu_InstructionsWaitButtons(void) {
    if (DWCi_BTNlGet() != -2) {
        DWCi_BTNlEnable();
        DWCi_ChangeScene(DWCUtilTopMenu_InstructionsMain);
    }
}

static void DWCUtilTopMenu_InstructionsMain(void) {
    DWCUtilTopMenu_InstructionsHandleInput();
    func_ov182_021afb0c();
    DWCUtilTopMenu_InstructionsHandleChoice();
}

static void DWCUtilTopMenu_InstructionsHandleInput(void) {
    if (DWCi_IPTlCheckTrigger(1)) {
        DWCi_BTNlSet(0);
    }
}

static void func_ov182_021afb0c(void) {
}

static void DWCUtilTopMenu_InstructionsHandleChoice(void) {
    if (DWCi_BTNlGet() == 0) {
        DWCi_SNDlPlay(6);
        DWCi_ChangeScene(DWCUtilTopMenu_InstructionsDisableButtons);
    }
}

static void DWCUtilTopMenu_InstructionsDisableButtons(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(DWCUtilTopMenu_InstructionsFadeOut);
}

static void DWCUtilTopMenu_InstructionsFadeOut(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_BTNlEnd();
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(DWCUtilTopMenu_InstructionsWaitFadeOut);
    }
}

static void DWCUtilTopMenu_InstructionsWaitFadeOut(void) {
    if (DWCi_EFFlCheck(0) == 0 && DWCi_BTNlIsEnd()) {
        DWCi_FNTlDeleteBgLcd(0);
        DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        DWCi_SetParam_(0, 1);
        DWCi_ChangeScene(DWCi_SceneOptMovWait);
    }
}
