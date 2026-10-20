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

static void func_ov182_021aeb30(void);
static void func_ov182_021aeb80(void);
static void func_ov182_021aebb8(void);
static void func_ov182_021aebe0(void);
static void func_ov182_021aec00(void);
static void func_ov182_021aec10(void);
static void func_ov182_021aec38(void);
static void func_ov182_021aec3c(void);
static void func_ov182_021aec84(void);
static void func_ov182_021aec9c(void);
static void func_ov182_021aece0(void);
static void func_ov182_021aed60(void);
static void func_ov182_021aed98(void);
static void func_ov182_021aedb0(void);
static void func_ov182_021aedcc(void);
static void func_ov182_021aee1c(void);
static void func_ov182_021aee40(void);
static void func_ov182_021aee60(void);
static void func_ov182_021aee80(void);
static void func_ov182_021aee90(void);
static void func_ov182_021aeea4(void);
static void func_ov182_021aeea8(void);
static void func_ov182_021aeec4(void);
static void func_ov182_021aeedc(void);
static void func_ov182_021aef10(void);
static void func_ov182_021aef5c(void);
static void func_ov182_021aefac(void);
static void func_ov182_021af0f8(void);
static void func_ov182_021af130(void);
static void func_ov182_021af158(void);
static void func_ov182_021af178(void);
static void func_ov182_021af188(void);
static void func_ov182_021af19c(void);
static void func_ov182_021af1a0(void);
static void func_ov182_021af1bc(void);
static void func_ov182_021af1d4(void);
static void func_ov182_021af20c(void);

// Whether the player confirmed the erasing
static u8 sErase;

void func_ov182_021aeafc(void) {
    sErase = FALSE;
    func_ov182_021aeb30();
    DWCUtilBgOverlay_Show(0x12);
    DWCUtilScreen_Start(0x3b, -1, 0);
    DWCUtilGfxResource_ShowMessage(0x17);
    DWCUtilMain_SetUpdate(func_ov182_021aeb80);
}

static void func_ov182_021aeb30(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021aeb80(void) {
    DWCUtilFade_Start(2, 1, 1, 8);
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(1, 1);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(func_ov182_021aebb8);
}

static void func_ov182_021aebb8(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(3);
        DWCUtilMain_SetUpdate(func_ov182_021aebe0);
    }
}

static void func_ov182_021aebe0(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(func_ov182_021aec00);
    }
}

static void func_ov182_021aec00(void) {
    func_ov182_021aec10();
    func_ov182_021aec38();
    func_ov182_021aec3c();
}

static void func_ov182_021aec10(void) {
    func_ov182_021aec10Inline();
}

static void func_ov182_021aec38(void) {
}

static void func_ov182_021aec3c(void) {
    switch (DWCUtilUiObject_GetState()) {
    case 0:
        DWCUtilSound_PlaySe(7);
        DWCUtilMain_SetUpdate(func_ov182_021aec84);
        break;
    case 1:
        DWCUtilSound_PlaySe(6);
        DWCUtilDialog_Open(0x18, 0, 1, -1, 0);
        DWCUtilUiObject_Disable();
        DWCUtilMain_SetUpdate(func_ov182_021aed60);
        break;
    }
}

static void func_ov182_021aec84(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(func_ov182_021aec9c);
}

static void func_ov182_021aec9c(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        if (!sErase) {
            DWCUtilFade_Start(3, 1, 1, 8);
        }
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(func_ov182_021aece0);
    }
}

static void func_ov182_021aece0(void) {
    if (DWCUtilFade_IsActive(0) == 0 && (sErase || DWCUtilFade_IsActive(1) == 0) && DWCUtilUiObject_IsIdle()) {
        DWCUtilText_DestroyScreenBg(0);
        if (!sErase) {
            DWCUtilScreen_Stop();
            DWCUtilDisplay_HidePlanes(1, 1);
        }
        DWCUtilDisplay_HidePlanes(0, 0x15);
        if (!sErase) {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetUpdate(func_ov182_021af25c);
        } else {
            DWCUtilMain_SetLayout(0, 1);
            DWCUtilMain_SetUpdate(func_ov182_021aedb0);
        }
    }
}

// The confirmation: 1 erases the settings
static void func_ov182_021aed60(void) {
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
    DWCUtilMain_SetUpdate(func_ov182_021aed98);
}

static void func_ov182_021aed98(void) {
    if (!DWCUtilDialog_IsOpen()) {
        DWCUtilMain_SetUpdate(func_ov182_021aec84);
    }
}

// Erases the settings, and tells the player to turn the console off
static void func_ov182_021aedb0(void) {
    func_ov182_021aedcc();
    DWCUtilGfxResource_ShowMessage(0x19);
    func_ov182_021b7ea0();
    DWCUtilMain_SetUpdate(func_ov182_021aee1c);
}

static void func_ov182_021aedcc(void) {
    DWCUtilGfxResource_Load("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021aee1c(void) {
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(func_ov182_021aee40);
}

static void func_ov182_021aee40(void) {
    if (DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(5);
        DWCUtilMain_SetUpdate(func_ov182_021aee60);
    }
}

static void func_ov182_021aee60(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(func_ov182_021aee80);
    }
}

static void func_ov182_021aee80(void) {
    func_ov182_021aee90();
    func_ov182_021aeea4();
    func_ov182_021aeea8();
}

static void func_ov182_021aee90(void) {
    if (DWCUtilTouch_IsKeyPressed(1)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021aeea4(void) {
}

static void func_ov182_021aeea8(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        DWCUtilSound_PlaySe(6);
        DWCUtilMain_SetUpdate(func_ov182_021aeec4);
    }
}

static void func_ov182_021aeec4(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(func_ov182_021aeedc);
}

static void func_ov182_021aeedc(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilFade_Start(3, 1, 0x3f, 0x40);
        DWCUtilFade_Start(3, 0, 0x3f, 0x40);
        DWCUtilMain_SetUpdate(func_ov182_021aef10);
    }
}

// Turns the console off once both screens have faded out
static void func_ov182_021aef10(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        OS_SpinWaitSysCycles(0x800000);
        PM_ForceToPowerOff();
    }
}

void func_ov182_021aef34(void) {
    func_ov182_021aef5c();
    DWCUtilBgOverlay_Show(0x11);
    DWCUtilScreen_Start(0x3a, -1, 0);
    func_ov182_021aefac();
    DWCUtilMain_SetUpdate(func_ov182_021af0f8);
}

static void func_ov182_021aef5c(void) {
    DWCUtilGfxResource_Load("char/jb5Info.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

// Writes the console's MAC address and its Wi-Fi ID, in four groups of four digits, the last of them the ID's last
// digit and three zeros
static void func_ov182_021aefac(void) {
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

static void func_ov182_021af0f8(void) {
    DWCUtilFade_Start(2, 1, 1, 8);
    DWCUtilFade_Start(2, 0, 0x15, 8);
    DWCUtilDisplay_ShowPlanes(1, 1);
    DWCUtilDisplay_ShowPlanes(0, 0x15);
    DWCUtilMain_SetUpdate(func_ov182_021af130);
}

static void func_ov182_021af130(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0) {
        DWCUtilGfxResource_ShowButtons(2);
        DWCUtilMain_SetUpdate(func_ov182_021af158);
    }
}

static void func_ov182_021af158(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        DWCUtilMain_SetUpdate(func_ov182_021af178);
    }
}

static void func_ov182_021af178(void) {
    func_ov182_021af188();
    func_ov182_021af19c();
    func_ov182_021af1a0();
}

static void func_ov182_021af188(void) {
    if (DWCUtilTouch_IsKeyPressed(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021af19c(void) {
}

static void func_ov182_021af1a0(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        DWCUtilSound_PlaySe(7);
        DWCUtilMain_SetUpdate(func_ov182_021af1bc);
    }
}

static void func_ov182_021af1bc(void) {
    DWCUtilUiObject_Disable();
    DWCUtilFade_StartWait(8);
    DWCUtilMain_SetUpdate(func_ov182_021af1d4);
}

static void func_ov182_021af1d4(void) {
    if (DWCUtilFade_IsActive(1) == 0) {
        DWCUtilUiObject_Exit();
        DWCUtilFade_Start(3, 1, 1, 8);
        DWCUtilFade_Start(3, 0, 0x15, 8);
        DWCUtilMain_SetUpdate(func_ov182_021af20c);
    }
}

static void func_ov182_021af20c(void) {
    if (DWCUtilFade_IsActive(1) == 0 && DWCUtilFade_IsActive(0) == 0 && DWCUtilUiObject_IsIdle()) {
        DWCUtilText_DestroyScreenBg(0);
        DWCUtilScreen_Stop();
        DWCUtilDisplay_HidePlanes(1, 1);
        DWCUtilDisplay_HidePlanes(0, 0x15);
        DWCUtilMain_SetLayout(0, 1);
        DWCUtilMain_SetUpdate(func_ov182_021af25c);
    }
}
