#include "scenesetusbready1.h"
#include "anime.h"
#include "button.h"
#include "comment.h"
#include "common.h"
#include "dwc_util_main.h"
#include "dwc_util_shared.h"
#include "effect.h"
#include "font.h"
#include "graphics.h"
#include "headline.h"
#include "input.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "scenesetselectap.h"
#include "scenesetusbcomplete.h"
#include "scenesetusbready.h"
#include "sound.h"
#include "usbap.h"
#include "window.h"

static BOOL DWCi_IsFold(void);
static void initGraphics(void);
static void procInit(void);
static void procInit1(void);
static void procInit2(void);
static void procScene(void);
static void input(void);
static void draw(void);
static void button(void);
static void endScene(void);
static void endScene1(void);
static void endScene2(void);
static void cbUsb(int result);
static void procNotFound(void);
static void procNotFound1(void);
static void procBoth(void);
static void procBoth1(void);

// How the setup ends
enum {
    USB_SETUP_RUNNING,
    USB_SETUP_DONE,   // the connector took the console
    USB_SETUP_CANCEL, // back to the first screen of the connection settings
    USB_SETUP_RETRY,  // back to the screen before the setup
};

static u8 sUsbSetupState;

// A copy of gfl/ui.c's GCTX_HIDIsLidClosed
static BOOL DWCi_IsFold(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void DWCi_SceneSetUsbReady1(void) {
    u16 nickName[OS_OWNERINFO_NICKNAME_MAX + 1];
    OSOwnerInfo info;

    sUsbSetupState = USB_SETUP_RUNNING;
    initGraphics();
    DWCi_HLlLoad(8);
    DWCi_CMNlStep(2);
    OS_GetOwnerInfo(&info);
    MI_CpuClear16(nickName, sizeof(nickName));
    MI_CpuCopy16(info.nickName, nickName, info.nickNameLength * 2);
    DWCi_CMNlMsgNameDraw(nickName, 0x28);
    DWCi_ANIMElInitEx(0);
    DWCi_USBAPlBegin(cbUsb);
    DWCi_SNDlPlay(0xb);
    DWCi_ChangeScene(procInit);
}

static void initGraphics(void) {
    DWCi_CMNlLoadResource("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    DWCi_CMNlLoadResource("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    DWCi_CMNlLoadResource("char/jb4Usb.nsc.l", GX_LoadBG2Scr);
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
        DWCi_CMNlButton(2);
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

// B, or closing the lid, presses the button that cancels
static void input(void) {
    if (DWCi_IPTlCheckTrigger(2)) {
        DWCi_BTNlSet(0);
    } else if (DWCi_IsFold()) {
        DWCi_BTNlSet(0);
    }
}

static void draw(void) {
}

static void button(void) {
    if (sUsbSetupState == USB_SETUP_RUNNING && DWCi_BTNlGet() == 0) {
        sUsbSetupState = USB_SETUP_CANCEL;
        DWCi_SNDlStop();
        DWCi_SNDlPlay(7);
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
        DWCi_BTNlEnd();
        if (sUsbSetupState == USB_SETUP_CANCEL || sUsbSetupState == USB_SETUP_RETRY) {
            DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 1, GX_BLEND_PLANEMASK_BG0, 8);
        }
        DWCi_EFFlFade(DWC_UTIL_FADE_TO_WHITE, 0,
                      GX_BLEND_PLANEMASK_BG0 | GX_BLEND_PLANEMASK_BG2 | GX_BLEND_PLANEMASK_OBJ, 8);
        DWCi_ChangeScene(endScene2);
    }
}

static void endScene2(void) {
    if (DWCi_EFFlCheck(0) == 0 && (sUsbSetupState != USB_SETUP_CANCEL || DWCi_EFFlCheck(1) == 0) && DWCi_BTNlIsEnd()) {
        DWCi_USBAPlEnd();
        DWCi_ANIMElEnd();
        DWCi_FNTlDeleteBgLcd(0);
        if (sUsbSetupState == USB_SETUP_CANCEL || sUsbSetupState == USB_SETUP_RETRY) {
            DWCi_COMMENTlDelete();
            DWCi_GXlInvisible(1, GX_PLANEMASK_BG0);
        }
        DWCi_GXlInvisible(0, GX_PLANEMASK_BG0 | GX_PLANEMASK_BG2 | GX_PLANEMASK_OBJ);
        if (sUsbSetupState == USB_SETUP_CANCEL) {
            DWCi_SetParam_(2, 1);
            DWCi_ChangeScene(DWCi_SceneSetSelectAp);
        } else if (sUsbSetupState == USB_SETUP_RETRY) {
            DWCi_SetParam_(2, 1);
            DWCi_ChangeScene(DWCi_SceneSetUsbReady);
        } else {
            DWCi_SetParam_(0, 0);
            DWCi_ChangeScene(DWCi_SceneSetUsbComplete);
        }
    }
}

// The search's callback
static void cbUsb(int result) {
    if (sUsbSetupState != USB_SETUP_RUNNING) {
        return;
    }
    switch (result) {
    case DWC_UTIL_USB_SCAN_NONE:
        sUsbSetupState = USB_SETUP_RETRY;
        DWCi_SNDlStop();
        DWCi_SNDlPlay(0x12);
        DWCi_WINlInitEx(0x10, 1, 1, -1, 0);
        DWCi_BTNlDisable();
        DWCi_ChangeScene(procNotFound);
        break;
    case DWC_UTIL_USB_SCAN_FOUND:
        sUsbSetupState = USB_SETUP_DONE;
        DWCi_SNDlStop();
        DWCi_ChangeScene(endScene);
        break;
    case DWC_UTIL_USB_SCAN_SEVERAL:
        DWCi_SNDlStop();
        DWCi_USBAPlSetCallback(NULL);
        DWCi_WINlInitEx(0x11, 0, 1, -1, 0);
        DWCi_BTNlDisable();
        DWCi_ChangeScene(procBoth);
        break;
    case 3:
        sUsbSetupState = USB_SETUP_CANCEL;
        DWCi_SNDlStop();
        DWCi_SNDlPlay(9);
        DWCi_ChangeScene(endScene);
        break;
    }
}

static void procNotFound(void) {
    if (DWCi_WINlGet() == 0) {
        DWCi_SNDlPlay(6);
        DWCi_WINlEnd();
        DWCi_ChangeScene(procNotFound1);
    }
}

static void procNotFound1(void) {
    if (!DWCi_WINlIsInit()) {
        DWCi_ChangeScene(endScene);
    }
}

// The dialog of several connectors: the first button ends the setup, the second searches again
static void procBoth(void) {
    switch (DWCi_WINlGet()) {
    case 1:
        sUsbSetupState = USB_SETUP_RETRY;
        DWCi_SNDlPlay(6);
        break;
    case 0:
        sUsbSetupState = USB_SETUP_DONE;
        DWCi_SNDlPlay(7);
        break;
    default:
        return;
    }
    DWCi_WINlEnd();
    DWCi_ChangeScene(procBoth1);
}

static void procBoth1(void) {
    if (!DWCi_WINlIsInit()) {
        if (sUsbSetupState == USB_SETUP_DONE) {
            DWCi_ChangeScene(endScene);
        } else {
            DWCi_SNDlPlay(0xb);
            DWCi_BTNlEnable();
            sUsbSetupState = USB_SETUP_RUNNING;
            DWCi_USBAPlSetCallback(cbUsb);
            DWCi_ChangeScene(procScene);
        }
    }
}
