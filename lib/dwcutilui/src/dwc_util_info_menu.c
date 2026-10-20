#include "dwc_util_info_menu.h"
#include "dwc/bm.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_dialog.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_option_menu.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_string.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/gx.h"
#include "nitro/os.h"
#include "nitro/pm.h"

static void DWCUtilInfoMenu_LoadEraseGraphics(void);
static void DWCUtilInfoMenu_EraseFadeIn(void);
static void DWCUtilInfoMenu_EraseWaitFadeIn(void);
static void DWCUtilInfoMenu_EraseWaitButtons(void);
static void DWCUtilInfoMenu_EraseMain(void);
static void DWCUtilInfoMenu_EraseHandleInput(void);
static void func_ov182_021aec38(void);
static void DWCUtilInfoMenu_EraseHandleChoice(void);
static void DWCUtilInfoMenu_EraseDisableButtons(void);
static void DWCUtilInfoMenu_EraseFadeOut(void);
static void DWCUtilInfoMenu_EraseWaitFadeOut(void);
static void DWCUtilInfoMenu_EraseWaitDialog(void);
static void DWCUtilInfoMenu_EraseWaitDialogClosed(void);
static void DWCUtilInfoMenu_EraseSettings(void);
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
static void DWCUtilInfoMenu_LoadInfoGraphics(void);
static void DWCUtilInfoMenu_DrawConsoleInfo(void);
static void DWCUtilInfoMenu_InfoFadeIn(void);
static void DWCUtilInfoMenu_InfoWaitFadeIn(void);
static void DWCUtilInfoMenu_InfoWaitButtons(void);
static void DWCUtilInfoMenu_InfoMain(void);
static void DWCUtilInfoMenu_InfoHandleInput(void);
static void func_ov182_021af19c(void);
static void DWCUtilInfoMenu_InfoHandleChoice(void);
static void DWCUtilInfoMenu_InfoDisableButtons(void);
static void DWCUtilInfoMenu_InfoFadeOut(void);
static void DWCUtilInfoMenu_InfoWaitFadeOut(void);

// Whether the player confirmed the erasing
static u8 sErase;

void DWCUtilInfoMenu_InitErase(void) {
    sErase = FALSE;
    DWCUtilInfoMenu_LoadEraseGraphics();
    DWCUtilBgOverlay_Show(0x12);
    DWCUtilScreen_Start(0x3b, -1, 0);
    DWCUtilGfxResource_ShowMessage(0x17);
    DWCUtilMain_SetUpdate(DWCUtilInfoMenu_EraseFadeIn);
}

static void DWCUtilInfoMenu_LoadEraseGraphics(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilInfoMenu_EraseFadeIn(void) {
    DWCUtilFade_Start(2, 1, 1, 8);
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(1, 1);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilInfoMenu_EraseWaitFadeIn);
}

static void DWCUtilInfoMenu_EraseWaitFadeIn(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(3);
        DWCUtilMain_SetUpdate(DWCUtilInfoMenu_EraseWaitButtons);
    }
}

static void DWCUtilInfoMenu_EraseWaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilInfoMenu_EraseMain);
    }
}

static void DWCUtilInfoMenu_EraseMain(void) {
    DWCUtilInfoMenu_EraseHandleInput();
    func_ov182_021aec38();
    DWCUtilInfoMenu_EraseHandleChoice();
}

static void DWCUtilInfoMenu_EraseHandleInput(void) {
    DWCUtilInfoMenu_EraseHandleInputInline();
}

static void func_ov182_021aec38(void) {
}

static void DWCUtilInfoMenu_EraseHandleChoice(void) {
    switch (DWCUtilUiObject_GetState()) {
    case 0:
        DWCUtilSound_PlaySe(7);
        DWCUtilMain_SetUpdate(DWCUtilInfoMenu_EraseDisableButtons);
        break;
    case 1:
        DWCUtilSound_PlaySe(6);
        DWCUtilDialog_Open(0x18, 0, 1, -1, 0);
        DWCUtilUiObject_Disable();
        DWCUtilMain_SetUpdate(DWCUtilInfoMenu_EraseWaitDialog);
        break;
    }
}

static void DWCUtilInfoMenu_EraseDisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilInfoMenu_EraseFadeOut);
}

static void DWCUtilInfoMenu_EraseFadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        if (!sErase) {
            DWCUtilFade_Start(3, 1, 1, 8);
        }
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(DWCUtilInfoMenu_EraseWaitFadeOut);
    }
}

static void DWCUtilInfoMenu_EraseWaitFadeOut(void) {
    if (DWCUtilFade_IsActive(0) == 0 && (sErase || DWCUtilFade_IsActive(1) == 0) && DWCUtilUiObject_IsIdle()) {
        DWCUtilText_DestroyScreenBg(0);
        if (!sErase) {
            DWCUtilScreen_Stop();
            DWCUtilDisplay_HidePlanes(1, 1);
        }
        DWCUtilDisplay_HidePlanes(0, 0x15);
        if (!sErase) {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetUpdate(DWCUtilOptionMenu_Init);
        } else {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetUpdate(DWCUtilInfoMenu_EraseSettings);
        }
    }
}

// The confirmation: 1 erases the settings
static void DWCUtilInfoMenu_EraseWaitDialog(void) {
    switch (DWCUtilDialog_GetChoice()) {
    case 1:
        DWCUtilSound_PlaySe(0xe);
        sErase = TRUE;
        break;
    case 0:
        DWCUtilSound_PlaySe(7);
        break;
    default:
        return;
    }
    DWCUtilDialog_Close();
    DWCUtilMain_SetUpdate(DWCUtilInfoMenu_EraseWaitDialogClosed);
}

static void DWCUtilInfoMenu_EraseWaitDialogClosed(void) {
    if (!DWCUtilDialog_IsOpen()) {
        DWCUtilMain_SetUpdate(DWCUtilInfoMenu_EraseDisableButtons);
    }
}

// Erases the settings, and tells the player to turn the console off
static void DWCUtilInfoMenu_EraseSettings(void) {
    DWCUtilInfoMenu_LoadErasedGraphics();
    DWCUtilGfxResource_ShowMessage(0x19);
    DWCUtilWifiSettings_EraseAll();
    DWCUtilMain_SetUpdate(DWCUtilInfoMenu_ErasedFadeIn);
}

static void DWCUtilInfoMenu_LoadErasedGraphics(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void DWCUtilInfoMenu_ErasedFadeIn(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilInfoMenu_ErasedWaitFadeIn);
}

static void DWCUtilInfoMenu_ErasedWaitFadeIn(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(5);
        DWCUtilMain_SetUpdate(DWCUtilInfoMenu_ErasedWaitButtons);
    }
}

static void DWCUtilInfoMenu_ErasedWaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilInfoMenu_ErasedMain);
    }
}

static void DWCUtilInfoMenu_ErasedMain(void) {
    DWCUtilInfoMenu_ErasedHandleInput();
    func_ov182_021aeea4();
    DWCUtilInfoMenu_ErasedHandleChoice();
}

static void DWCUtilInfoMenu_ErasedHandleInput(void) {
    if (DWCUtilTouch_IsKeyPressed(1)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021aeea4(void) {
}

static void DWCUtilInfoMenu_ErasedHandleChoice(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCUtilMain_SetUpdate(DWCUtilInfoMenu_ErasedDisableButtons);
    }
}

static void DWCUtilInfoMenu_ErasedDisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilInfoMenu_ErasedFadeOut);
}

static void DWCUtilInfoMenu_ErasedFadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilFade_Start(3, 1, 0x3f, 0x40);
        DWCUtilFade_Start(3, 0, 0x3f, 0x40);
        DWCUtilMain_SetUpdate(DWCUtilInfoMenu_PowerOff);
    }
}

// Turns the console off once both screens have faded out
static void DWCUtilInfoMenu_PowerOff(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        OS_SpinWaitSysCycles(0x800000);
        PM_ForceToPowerOff();
    }
}

void DWCUtilInfoMenu_InitInfo(void) {
    DWCUtilInfoMenu_LoadInfoGraphics();
    DWCUtilBgOverlay_Show(0x11);
    DWCUtilScreen_Start(0x3a, -1, 0);
    DWCUtilInfoMenu_DrawConsoleInfo();
    DWCUtilMain_SetUpdate(DWCUtilInfoMenu_InfoFadeIn);
}

static void DWCUtilInfoMenu_LoadInfoGraphics(void) {
    DWCUtilGfxResource_Load("char/jb5Info.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

// Writes the console's MAC address and its Wi-Fi ID, in four groups of four digits, the last of them the ID's last
// digit and three zeros
static void DWCUtilInfoMenu_DrawConsoleInfo(void) {
    u64 id;
    int i;
    DWCBmWifiIdInfo info;
    int groups[4];
    u16 wide[0x14];
    u8 mac[6];
    char text[0x14];
    DWCUtilText *textObj;

    textObj = DWCUtilText_CreateBg(0, 0);
    OS_GetMacAddress(mac);
    OS_SNPrintf(text, sizeof(text), "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    DWCUtilString_Widen(wide, (const u8 *)text, sizeof(text));
    DWCUtilText_DrawRect(textObj, 8, 0x40, 0xf0, 0x1c, 2, 0x480, wide);
    DWCBm_GetWifiIdInfo(&info);
    id = info.id;
    if (id != 0) {
        groups[3] = (id % 10) * 1000;
        id /= 10;
        for (i = 0; i < 3; i++) {
            groups[2 - i] = id % 10000;
            id /= 10000;
        }
        OS_SNPrintf(text, sizeof(text), "%04d-%04d-%04d-%04d", groups[0], groups[1], groups[2], groups[3]);
        DWCUtilString_Widen(wide, (const u8 *)text, sizeof(text));
    } else {
        OS_SNPrintf(text, sizeof(text), "-------------------");
        DWCUtilString_Widen(wide, (const u8 *)text, sizeof(text));
    }
    DWCUtilText_DrawRect(textObj, 8, 0x78, 0xf0, 0x1c, 2, 0x480, wide);
    DWCUtilText_MarkDirty(textObj);
}

static void DWCUtilInfoMenu_InfoFadeIn(void) {
    DWCUtilFade_Start(2, 1, 1, 8);
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(1, 1);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(DWCUtilInfoMenu_InfoWaitFadeIn);
}

static void DWCUtilInfoMenu_InfoWaitFadeIn(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(2);
        DWCUtilMain_SetUpdate(DWCUtilInfoMenu_InfoWaitButtons);
    }
}

static void DWCUtilInfoMenu_InfoWaitButtons(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(DWCUtilInfoMenu_InfoMain);
    }
}

static void DWCUtilInfoMenu_InfoMain(void) {
    DWCUtilInfoMenu_InfoHandleInput();
    func_ov182_021af19c();
    DWCUtilInfoMenu_InfoHandleChoice();
}

static void DWCUtilInfoMenu_InfoHandleInput(void) {
    if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021af19c(void) {
}

static void DWCUtilInfoMenu_InfoHandleChoice(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        DWCUtilSound_PlaySe(7);
        DWCUtilMain_SetUpdate(DWCUtilInfoMenu_InfoDisableButtons);
    }
}

static void DWCUtilInfoMenu_InfoDisableButtons(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(DWCUtilInfoMenu_InfoFadeOut);
}

static void DWCUtilInfoMenu_InfoFadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        DWCUtilFade_Start(3, 1, 1, 8);
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(DWCUtilInfoMenu_InfoWaitFadeOut);
    }
}

static void DWCUtilInfoMenu_InfoWaitFadeOut(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0 && DWCUtilUiObject_IsIdle()) {
        DWCUtilText_DestroyScreenBg(0);
        DWCUtilScreen_Stop();
        DWCUtilDisplay_HidePlanes(1, 1);
        DWCUtilDisplay_HidePlanes(0, 0x15);
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(DWCUtilOptionMenu_Init);
    }
}
