#include "dwc_util_usb_setup.h"
#include "dwc_util_bg_overlay.h"
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

static BOOL func_ov182_021b6b00(void);
static void func_ov182_021b6b8c(void);
static void func_ov182_021b6bfc(void);
static void func_ov182_021b6c20(void);
static void func_ov182_021b6c40(void);
static void func_ov182_021b6c60(void);
static void func_ov182_021b6c70(void);
static void func_ov182_021b6c94(void);
static void func_ov182_021b6c98(void);
static void func_ov182_021b6cc8(void);
static void func_ov182_021b6ce0(void);
static void func_ov182_021b6d28(void);
static void func_ov182_021b6dcc(int result);
static void func_ov182_021b6e80(void);
static void func_ov182_021b6ea0(void);
static void func_ov182_021b6eb8(void);
static void func_ov182_021b6ef0(void);

// How the setup ends
enum {
    USB_SETUP_RUNNING,
    USB_SETUP_DONE,   // the connector took the console
    USB_SETUP_CANCEL, // back to the first screen of the connection settings
    USB_SETUP_RETRY,  // back to the screen before the setup
};

static u8 sUsbSetupState;

// A copy of gfl/ui.c's GCTX_HIDIsLidClosed
static BOOL func_ov182_021b6b00(void) {
    if (PAD_DetectFold()) {
        return TRUE;
    }
    return FALSE;
}

void func_ov182_021b6b1c(void) {
    u16 nickName[OS_OWNERINFO_NICKNAME_MAX + 1];
    OSOwnerInfo info;

    sUsbSetupState = USB_SETUP_RUNNING;
    func_ov182_021b6b8c();
    func_ov182_021ab96c(8);
    func_ov182_021ab6a4(2);
    OS_GetOwnerInfo(&info);
    MI_CpuClear16(nickName, sizeof(nickName));
    MI_CpuCopy16(info.nickName, nickName, info.nickNameLength * 2);
    func_ov182_021ab738(nickName, 0x28);
    DWCUtilUiEffect_Init(0);
    func_ov182_021b8834(func_ov182_021b6dcc);
    func_ov182_021b80f4(0xb);
    func_ov182_021adfbc(func_ov182_021b6bfc);
}

static void func_ov182_021b6b8c(void) {
    func_ov182_021ab67c("char/jbBgStep3.ncg.l", GX_LoadBG2Char);
    func_ov182_021ab67c("char/ybBgStep3.ncl.l", GX_LoadBGPltt);
    func_ov182_021ab67c("char/jb4Usb.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021b6bfc(void) {
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(0, 0x15);
    func_ov182_021adfbc(func_ov182_021b6c20);
}

static void func_ov182_021b6c20(void) {
    if (func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab6d0(2);
        func_ov182_021adfbc(func_ov182_021b6c40);
    }
}

static void func_ov182_021b6c40(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021b6c60);
    }
}

static void func_ov182_021b6c60(void) {
    func_ov182_021b6c70();
    func_ov182_021b6c94();
    func_ov182_021b6c98();
}

// B, or closing the lid, presses the button that cancels
static void func_ov182_021b6c70(void) {
    if (func_ov182_021bc798(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (func_ov182_021b6b00()) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021b6c94(void) {
}

static void func_ov182_021b6c98(void) {
    if (sUsbSetupState == USB_SETUP_RUNNING && DWCUtilUiObject_GetState() == 0) {
        sUsbSetupState = USB_SETUP_CANCEL;
        func_ov182_021b8138();
        func_ov182_021b80f4(7);
        func_ov182_021adfbc(func_ov182_021b6cc8);
    }
}

static void func_ov182_021b6cc8(void) {
    DWCUtilUiObject_Disable();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021b6ce0);
}

static void func_ov182_021b6ce0(void) {
    if (func_ov182_021bbaac(1) == 0) {
        DWCUtilUiObject_Exit();
        if (sUsbSetupState == USB_SETUP_CANCEL || sUsbSetupState == USB_SETUP_RETRY) {
            func_ov182_021bbac4(3, 1, 1, 8);
        }
        func_ov182_021bbac4(3, 0, 0x15, 8);
        func_ov182_021adfbc(func_ov182_021b6d28);
    }
}

static void func_ov182_021b6d28(void) {
    if (func_ov182_021bbaac(0) == 0 && (sUsbSetupState != USB_SETUP_CANCEL || func_ov182_021bbaac(1) == 0) &&
        DWCUtilUiObject_IsIdle()) {
        func_ov182_021b895c();
        DWCUtilUiEffect_Destroy();
        func_ov182_021bbf7c(0);
        if (sUsbSetupState == USB_SETUP_CANCEL || sUsbSetupState == USB_SETUP_RETRY) {
            DWCUtilScreen_Stop();
            func_ov182_021bc228(1, 1);
        }
        func_ov182_021bc228(0, 0x15);
        if (sUsbSetupState == USB_SETUP_CANCEL) {
            func_ov182_021adfc8(2, 1);
            func_ov182_021adfbc(func_ov182_021b536c);
        } else if (sUsbSetupState == USB_SETUP_RETRY) {
            func_ov182_021adfc8(2, 1);
            func_ov182_021adfbc(func_ov182_021b688c);
        } else {
            func_ov182_021adfc8(0, 0);
            func_ov182_021adfbc(func_ov182_021b6728);
        }
    }
}

// The search's callback
static void func_ov182_021b6dcc(int result) {
    if (sUsbSetupState != USB_SETUP_RUNNING) {
        return;
    }
    switch (result) {
    case DWC_UTIL_USB_SCAN_NONE:
        sUsbSetupState = USB_SETUP_RETRY;
        func_ov182_021b8138();
        func_ov182_021b80f4(0x12);
        func_ov182_021b8cf0(0x10, 1, 1, -1, 0);
        DWCUtilUiObject_Disable();
        func_ov182_021adfbc(func_ov182_021b6e80);
        break;
    case DWC_UTIL_USB_SCAN_FOUND:
        sUsbSetupState = USB_SETUP_DONE;
        func_ov182_021b8138();
        func_ov182_021adfbc(func_ov182_021b6cc8);
        break;
    case DWC_UTIL_USB_SCAN_SEVERAL:
        func_ov182_021b8138();
        func_ov182_021b89e8(NULL);
        func_ov182_021b8cf0(0x11, 0, 1, -1, 0);
        DWCUtilUiObject_Disable();
        func_ov182_021adfbc(func_ov182_021b6eb8);
        break;
    case 3:
        sUsbSetupState = USB_SETUP_CANCEL;
        func_ov182_021b8138();
        func_ov182_021b80f4(9);
        func_ov182_021adfbc(func_ov182_021b6cc8);
        break;
    }
}

static void func_ov182_021b6e80(void) {
    if (func_ov182_021b8f14() == 0) {
        func_ov182_021b80f4(6);
        func_ov182_021b8eec();
        func_ov182_021adfbc(func_ov182_021b6ea0);
    }
}

static void func_ov182_021b6ea0(void) {
    if (!func_ov182_021b8f24()) {
        func_ov182_021adfbc(func_ov182_021b6cc8);
    }
}

// The dialog of several connectors: the first button ends the setup, the second searches again
static void func_ov182_021b6eb8(void) {
    switch (func_ov182_021b8f14()) {
    case 1:
        sUsbSetupState = USB_SETUP_RETRY;
        func_ov182_021b80f4(6);
        break;
    case 0:
        sUsbSetupState = USB_SETUP_DONE;
        func_ov182_021b80f4(7);
        break;
    default:
        return;
    }
    func_ov182_021b8eec();
    func_ov182_021adfbc(func_ov182_021b6ef0);
}

static void func_ov182_021b6ef0(void) {
    if (!func_ov182_021b8f24()) {
        if (sUsbSetupState == USB_SETUP_DONE) {
            func_ov182_021adfbc(func_ov182_021b6cc8);
        } else {
            func_ov182_021b80f4(0xb);
            DWCUtilUiObject_Enable();
            sUsbSetupState = USB_SETUP_RUNNING;
            func_ov182_021b89e8(func_ov182_021b6dcc);
            func_ov182_021adfbc(func_ov182_021b6c60);
        }
    }
}
