#include "dwc_util_usb_setup.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_dialog.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_ap.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_effect.h"
#include "dwc_util_ui_object.h"
#include "dwc_util_usb_done.h"
#include "dwc_util_usb_intro.h"
#include "dwc_util_usb_scan.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"

static BOOL DWCUtilUsbSetup_IsLidClosed(void);
static void DWCUtilUsbSetup_LoadGraphics(void);
static void DWCUtilUsbSetup_FadeIn(void);
static void DWCUtilUsbSetup_WaitFadeIn(void);
static void DWCUtilUsbSetup_WaitButtons(void);
static void DWCUtilUsbSetup_Main(void);
static void DWCUtilUsbSetup_HandleInput(void);
static void func_ov182_021b6c94(void);
static void DWCUtilUsbSetup_HandleCancel(void);
static void DWCUtilUsbSetup_DisableButtons(void);
static void DWCUtilUsbSetup_FadeOut(void);
static void DWCUtilUsbSetup_WaitFadeOut(void);
static void DWCUtilUsbSetup_OnScanResult(int result);
static void DWCUtilUsbSetup_WaitNoneDialog(void);
static void DWCUtilUsbSetup_WaitNoneDialogClosed(void);
static void DWCUtilUsbSetup_WaitSeveralDialog(void);
static void DWCUtilUsbSetup_WaitSeveralDialogClosed(void);

// How the setup ends
enum {
    USB_SETUP_RUNNING,
    USB_SETUP_DONE,   // the connector took the console
    USB_SETUP_CANCEL, // back to the first screen of the connection settings
    USB_SETUP_RETRY,  // back to the screen before the setup
};

static u8 sUsbSetupState;

// A copy of gfl/ui.c's GCTX_HIDIsLidClosed
static BOOL DWCUtilUsbSetup_IsLidClosed(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void DWCUtilUsbSetup_Init(void) {
    u16 nickName[OS_OWNERINFO_NICKNAME_MAX + 1];
    OSOwnerInfo info;

    sUsbSetupState = USB_SETUP_RUNNING;
    DWCUtilUsbSetup_LoadGraphics();
    DWCUtilBgOverlay_Show(8);
    DWCUtilGfxResource_ShowStep(2);
    OS_GetOwnerInfo(&info);
    MI_CpuClear16(nickName, sizeof(nickName));
    MI_CpuCopy16(info.nickName, nickName, info.nickNameLength * 2);
    DWCUtilGfxResource_ShowMessageWithString(nickName, 0x28);
    DWCUtilUiEffect_Init(0);
    DWCUtilUsbScan_Start(DWCUtilUsbSetup_OnScanResult);
    DWCUtilSound_PlaySe(0xb);
    DWCUtilMain_SetUpdate(DWCUtilUsbSetup_FadeIn);
}

static void DWCUtilUsbSetup_LoadGraphics(void) {
    DWCUtilGfxResource_Load("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    DWCUtilGfxResource_Load("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    DWCUtilGfxResource_Load("char/jb4Usb.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilUsbSetup_FadeIn(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilUsbSetup_WaitFadeIn);
}

static void DWCUtilUsbSetup_WaitFadeIn(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(2);
        DWCUtilMain_SetUpdate(DWCUtilUsbSetup_WaitButtons);
    }
}

static void DWCUtilUsbSetup_WaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilUsbSetup_Main);
    }
}

static void DWCUtilUsbSetup_Main(void) {
    DWCUtilUsbSetup_HandleInput();
    func_ov182_021b6c94();
    DWCUtilUsbSetup_HandleCancel();
}

// B, or closing the lid, presses the button that cancels
static void DWCUtilUsbSetup_HandleInput(void) {
    if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (DWCUtilUsbSetup_IsLidClosed()) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b6c94(void) {
}

static void DWCUtilUsbSetup_HandleCancel(void) {
    if (sUsbSetupState == USB_SETUP_RUNNING && DWCUtilUiObject_GetState() == 0) {
        sUsbSetupState = USB_SETUP_CANCEL;
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(7);
        DWCUtilMain_SetUpdate(DWCUtilUsbSetup_DisableButtons);
    }
}

static void DWCUtilUsbSetup_DisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilUsbSetup_FadeOut);
}

static void DWCUtilUsbSetup_FadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        if (sUsbSetupState == USB_SETUP_CANCEL || sUsbSetupState == USB_SETUP_RETRY) {
            DWCUtilFade_Start(3, 1, 1, 8);
        }
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(DWCUtilUsbSetup_WaitFadeOut);
    }
}

static void DWCUtilUsbSetup_WaitFadeOut(void) {
    if (DWCUtilFade_IsActive(0) == 0 && (sUsbSetupState != USB_SETUP_CANCEL || DWCUtilFade_IsActive(1) == 0) &&
        DWCUtilUiObject_IsIdle()) {
        DWCUtilUsbScan_Stop();
        DWCUtilUiEffect_Destroy();
        DWCUtilText_DestroyScreenBg(0);
        if (sUsbSetupState == USB_SETUP_CANCEL || sUsbSetupState == USB_SETUP_RETRY) {
            DWCUtilScreen_Stop();
            DWCUtilDisplay_HidePlanes(1, 1);
        }
        DWCUtilDisplay_HidePlanes(0, 0x15);
        if (sUsbSetupState == USB_SETUP_CANCEL) {
            DWCUtilMain_SetLayout(2, 1);
            DWCUtilMain_SetUpdate(DWCUtilScreenAp_Init);
        } else if (sUsbSetupState == USB_SETUP_RETRY) {
            DWCUtilMain_SetLayout(2, 1);
            DWCUtilMain_SetUpdate(DWCUtilUsbIntro_Init);
        } else {
            DWCUtilMain_SetLayout(0, 0);
            DWCUtilMain_SetUpdate(DWCUtilUsbDone_Init);
        }
    }
}

// The search's callback
static void DWCUtilUsbSetup_OnScanResult(int result) {
    if (sUsbSetupState != USB_SETUP_RUNNING) {
        return;
    }
    switch (result) {
    case DWC_UTIL_USB_SCAN_NONE:
        sUsbSetupState = USB_SETUP_RETRY;
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(0x12);
        DWCUtilDialog_Open(0x10, 1, 1, -1, 0);
        DWCUtilUiObject_Disable();
        DWCUtilMain_SetUpdate(DWCUtilUsbSetup_WaitNoneDialog);
        break;
    case DWC_UTIL_USB_SCAN_FOUND:
        sUsbSetupState = USB_SETUP_DONE;
        DWCUtilSound_StopSe();
        DWCUtilMain_SetUpdate(DWCUtilUsbSetup_DisableButtons);
        break;
    case DWC_UTIL_USB_SCAN_SEVERAL:
        DWCUtilSound_StopSe();
        DWCUtilUsbScan_SetCallback(NULL);
        DWCUtilDialog_Open(0x11, 0, 1, -1, 0);
        DWCUtilUiObject_Disable();
        DWCUtilMain_SetUpdate(DWCUtilUsbSetup_WaitSeveralDialog);
        break;
    case 3:
        sUsbSetupState = USB_SETUP_CANCEL;
        DWCUtilSound_StopSe();
        DWCUtilSound_PlaySe(9);
        DWCUtilMain_SetUpdate(DWCUtilUsbSetup_DisableButtons);
        break;
    }
}

static void DWCUtilUsbSetup_WaitNoneDialog(void) {
    if (DWCUtilDialog_GetChoice() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCUtilDialog_Close();
        DWCUtilMain_SetUpdate(DWCUtilUsbSetup_WaitNoneDialogClosed);
    }
}

static void DWCUtilUsbSetup_WaitNoneDialogClosed(void) {
    if (!DWCUtilDialog_IsOpen()) {
        DWCUtilMain_SetUpdate(DWCUtilUsbSetup_DisableButtons);
    }
}

// The dialog of several connectors: the first button ends the setup, the second searches again
static void DWCUtilUsbSetup_WaitSeveralDialog(void) {
    switch (DWCUtilDialog_GetChoice()) {
    case 1:
        sUsbSetupState = USB_SETUP_RETRY;
        DWCUtilSound_PlaySe(6);
        break;
    case 0:
        sUsbSetupState = USB_SETUP_DONE;
        DWCUtilSound_PlaySe(7);
        break;
    default:
        return;
    }
    DWCUtilDialog_Close();
    DWCUtilMain_SetUpdate(DWCUtilUsbSetup_WaitSeveralDialogClosed);
}

static void DWCUtilUsbSetup_WaitSeveralDialogClosed(void) {
    if (!DWCUtilDialog_IsOpen()) {
        if (sUsbSetupState == USB_SETUP_DONE) {
            DWCUtilMain_SetUpdate(DWCUtilUsbSetup_DisableButtons);
        } else {
            DWCUtilSound_PlaySe(0xb);
            DWCUtilUiObject_Enable();
            sUsbSetupState = USB_SETUP_RUNNING;
            DWCUtilUsbScan_SetCallback(DWCUtilUsbSetup_OnScanResult);
            DWCUtilMain_SetUpdate(DWCUtilUsbSetup_Main);
        }
    }
}
