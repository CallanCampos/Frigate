#include "dwc_util_info_menu.h"
#include "dwc/bm.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_string.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
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
    func_ov182_021ab96c(0x12);
    DWCUtilScreen_Start(0x3b, -1, 0);
    func_ov182_021ab6f0(0x17);
    func_ov182_021adfbc(func_ov182_021aeb80);
}

static void func_ov182_021aeb30(void) {
    func_ov182_021ab67c("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021aeb80(void) {
    func_ov182_021bbac4(2, 1, 1, 8);
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(1, 1);
    func_ov182_021bc1dc(0, 0x15);
    func_ov182_021adfbc(func_ov182_021aebb8);
}

static void func_ov182_021aebb8(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab6d0(3);
        func_ov182_021adfbc(func_ov182_021aebe0);
    }
}

static void func_ov182_021aebe0(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021aec00);
    }
}

static void func_ov182_021aec00(void) {
    func_ov182_021aec10();
    func_ov182_021aec38();
    func_ov182_021aec3c();
}

static void func_ov182_021aec10(void) {
    if (func_ov182_021bc798(1)) {
        DWCUtilUiObject_SetStateIfIdle(1);
    }
    if (func_ov182_021bc798(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021aec38(void) {
}

static void func_ov182_021aec3c(void) {
    switch (DWCUtilUiObject_GetState()) {
    case 0:
        func_ov182_021b80f4(7);
        func_ov182_021adfbc(func_ov182_021aec84);
        break;
    case 1:
        func_ov182_021b80f4(6);
        func_ov182_021b8cf0(0x18, 0, 1, -1, 0);
        DWCUtilUiObject_Disable();
        func_ov182_021adfbc(func_ov182_021aed60);
        break;
    }
}

static void func_ov182_021aec84(void) {
    DWCUtilUiObject_Disable();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021aec9c);
}

static void func_ov182_021aec9c(void) {
    if (func_ov182_021bbaac(1) == 0) {
        DWCUtilUiObject_Exit();
        if (!sErase) {
            func_ov182_021bbac4(3, 1, 1, 8);
        }
        func_ov182_021bbac4(3, 0, 0x15, 8);
        func_ov182_021adfbc(func_ov182_021aece0);
    }
}

static void func_ov182_021aece0(void) {
    if (func_ov182_021bbaac(0) == 0 && (sErase || func_ov182_021bbaac(1) == 0) && DWCUtilUiObject_IsIdle()) {
        func_ov182_021bbf7c(0);
        if (!sErase) {
            DWCUtilScreen_Stop();
            func_ov182_021bc228(1, 1);
        }
        func_ov182_021bc228(0, 0x15);
        if (!sErase) {
            func_ov182_021adfc8(0, 1);
            func_ov182_021adfbc(func_ov182_021af25c);
        } else {
            func_ov182_021adfc8(0, 1);
            func_ov182_021adfbc(func_ov182_021aedb0);
        }
    }
}

// The confirmation: 1 erases the settings
static void func_ov182_021aed60(void) {
    switch (func_ov182_021b8f14()) {
    case 1:
        func_ov182_021b80f4(0xe);
        sErase = TRUE;
        break;
    case 0:
        func_ov182_021b80f4(7);
        break;
    default:
        return;
    }
    func_ov182_021b8eec();
    func_ov182_021adfbc(func_ov182_021aed98);
}

static void func_ov182_021aed98(void) {
    if (!func_ov182_021b8f24()) {
        func_ov182_021adfbc(func_ov182_021aec84);
    }
}

// Erases the settings, and tells the player to turn the console off
static void func_ov182_021aedb0(void) {
    func_ov182_021aedcc();
    func_ov182_021ab6f0(0x19);
    func_ov182_021b7ea0();
    func_ov182_021adfbc(func_ov182_021aee1c);
}

static void func_ov182_021aedcc(void) {
    func_ov182_021ab67c("char/yb5Multi.nsc.l", GX_LoadBG2Scr);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021aee1c(void) {
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(0, 0x15);
    func_ov182_021adfbc(func_ov182_021aee40);
}

static void func_ov182_021aee40(void) {
    if (func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab6d0(5);
        func_ov182_021adfbc(func_ov182_021aee60);
    }
}

static void func_ov182_021aee60(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021aee80);
    }
}

static void func_ov182_021aee80(void) {
    func_ov182_021aee90();
    func_ov182_021aeea4();
    func_ov182_021aeea8();
}

static void func_ov182_021aee90(void) {
    if (func_ov182_021bc798(1)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021aeea4(void) {
}

static void func_ov182_021aeea8(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        func_ov182_021b80f4(6);
        func_ov182_021adfbc(func_ov182_021aeec4);
    }
}

static void func_ov182_021aeec4(void) {
    DWCUtilUiObject_Disable();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021aeedc);
}

static void func_ov182_021aeedc(void) {
    if (func_ov182_021bbaac(1) == 0) {
        func_ov182_021bbac4(3, 1, 0x3f, 0x40);
        func_ov182_021bbac4(3, 0, 0x3f, 0x40);
        func_ov182_021adfbc(func_ov182_021aef10);
    }
}

// Turns the console off once both screens have faded out
static void func_ov182_021aef10(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        OS_SpinWaitSysCycles(0x800000);
        PM_ForceToPowerOff();
    }
}

void func_ov182_021aef34(void) {
    func_ov182_021aef5c();
    func_ov182_021ab96c(0x11);
    DWCUtilScreen_Start(0x3a, -1, 0);
    func_ov182_021aefac();
    func_ov182_021adfbc(func_ov182_021af0f8);
}

static void func_ov182_021aef5c(void) {
    func_ov182_021ab67c("char/jb5Info.nsc.l", GX_LoadBG2Scr);
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

    textObj = func_ov182_021bbdec(0, 0);
    OS_GetMacAddress(mac);
    OS_SNPrintf(text, sizeof(text), "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    func_ov182_021bcf10(wide, (const u8 *)text, sizeof(text));
    func_ov182_021bc0a4(textObj, 8, 0x40, 0xf0, 0x1c, 2, 0x480, wide);
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
        func_ov182_021bcf10(wide, (const u8 *)text, sizeof(text));
    } else {
        OS_SNPrintf(text, sizeof(text), "-------------------");
        func_ov182_021bcf10(wide, (const u8 *)text, sizeof(text));
    }
    func_ov182_021bc0a4(textObj, 8, 0x78, 0xf0, 0x1c, 2, 0x480, wide);
    func_ov182_021bc184(textObj);
}

static void func_ov182_021af0f8(void) {
    func_ov182_021bbac4(2, 1, 1, 8);
    func_ov182_021bbac4(2, 0, 0x15, 8);
    func_ov182_021bc1dc(1, 1);
    func_ov182_021bc1dc(0, 0x15);
    func_ov182_021adfbc(func_ov182_021af130);
}

static void func_ov182_021af130(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab6d0(2);
        func_ov182_021adfbc(func_ov182_021af158);
    }
}

static void func_ov182_021af158(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021af178);
    }
}

static void func_ov182_021af178(void) {
    func_ov182_021af188();
    func_ov182_021af19c();
    func_ov182_021af1a0();
}

static void func_ov182_021af188(void) {
    if (func_ov182_021bc798(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    }
}

static void func_ov182_021af19c(void) {
}

static void func_ov182_021af1a0(void) {
    if (DWCUtilUiObject_GetState() == 0) {
        func_ov182_021b80f4(7);
        func_ov182_021adfbc(func_ov182_021af1bc);
    }
}

static void func_ov182_021af1bc(void) {
    DWCUtilUiObject_Disable();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021af1d4);
}

static void func_ov182_021af1d4(void) {
    if (func_ov182_021bbaac(1) == 0) {
        DWCUtilUiObject_Exit();
        func_ov182_021bbac4(3, 1, 1, 8);
        func_ov182_021bbac4(3, 0, 0x15, 8);
        func_ov182_021adfbc(func_ov182_021af20c);
    }
}

static void func_ov182_021af20c(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0 && DWCUtilUiObject_IsIdle()) {
        func_ov182_021bbf7c(0);
        DWCUtilScreen_Stop();
        func_ov182_021bc228(1, 1);
        func_ov182_021bc228(0, 0x15);
        func_ov182_021adfc8(0, 1);
        func_ov182_021adfbc(func_ov182_021af25c);
    }
}
