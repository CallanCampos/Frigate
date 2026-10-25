#include "sceneoptmovcomplete.h"
#include "button.h"
#include "common.h"
#include "effect.h"
#include "graphics.h"
#include "input.h"
#include "main.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "sound.h"

static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static void func_ov182_021af998(void);
static void button(void);
static void endScene(void);
static void endScene1(void);
static void endScene2(void);

// The settings were sent: the player is told to turn the console off
void DWCi_SceneOptMovComplete(void) {
    initGraphics();
    DWCi_CMNlMsgDraw(0x1f);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
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
        DWCi_SNDlPlay(6);
        DWCi_ChangeScene(endScene);
    }
}

static void endScene(void) {
    DWCi_BTNlDisable();
    DWCi_EFFlWait(8);
    DWCi_ChangeScene(endScene1);
}

static void endScene1(void) {
    if (DWCi_EFFlCheck(1) == 0) {
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, DWC_UTIL_FADE_PLANES, 0x40);
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0, DWC_UTIL_FADE_PLANES, 0x40);
        DWCi_ChangeScene(endScene2);
    }
}

// Turns the console off once both screens have faded out
static void endScene2(void) {
    if (DWCi_EFFlCheck(1) == 0 && DWCi_EFFlCheck(0) == 0) {
        OS_SpinWaitSysCycles(0x800000);
        PM_ForceToPowerOff();
    }
}
