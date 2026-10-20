#include "dwc_util_option_menu.h"
#include "dwc/bm.h"
#include "dwc_util_bg_buffer.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_button_sprites.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_info_menu.h"
#include "dwc_util_main.h"
#include "dwc_util_main_menu.h"
#include "dwc_util_oam_buf.h"
#include "dwc_util_prompt_yes_no.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The chosen button, whether the player started it, the palette whose rows light up the buttons, and the cursor's
// sprite
typedef struct DWCUtilOptionMenu {
    u8 selection;
    u8 started;
    void *palette;
    GXOamAttr *cursor;
} DWCUtilOptionMenu;

static void func_ov182_021af2ec(void);
static void func_ov182_021af3f0(void);
static void func_ov182_021af428(void);
static void func_ov182_021af454(void);
static void func_ov182_021af474(void);
static void func_ov182_021af484(void);
static void func_ov182_021af524(void);
static void func_ov182_021af528(void);
static void func_ov182_021af598(void);
static void func_ov182_021af5b0(void);
static void func_ov182_021af5f0(void);
static void func_ov182_021af6a4(int direction);
static void func_ov182_021af6f8(void);

// The palette rows that light up each button
static const u8 sButtonRows[3] = { 1, 2, 3 };
// Where the three buttons are touched, and where the cursor frames them: left, top, right and bottom
static const u16 sButtonRects[3][4] = {
    { 0x08, 0x24, 0xf8, 0x44 },
    { 0x08, 0x50, 0xf8, 0x70 },
    { 0x08, 0x7c, 0xf8, 0x9c },
};
static const u16 sCursorRects[3][4] = {
    { 0x06, 0x22, 0xea, 0x36 },
    { 0x06, 0x4e, 0xea, 0x62 },
    { 0x06, 0x7a, 0xea, 0x8e },
};

static DWCUtilOptionMenu sOptionMenu;

void func_ov182_021af25c(void) {
    // The cursor sprite's position
    static const u16 cursorPos[2] = { 0xe0, 0x84 };

    sOptionMenu.started = FALSE;
    func_ov182_021af2ec();
    func_ov182_021ab96c(0x10);
    func_ov182_021b87d0(3);
    DWCUtilScreen_Start(0x39, -1, 0);
    sOptionMenu.cursor = func_ov182_021bb960(0, 0x5b);
    G2_SetOBJPosition(sOptionMenu.cursor, cursorPos[0], cursorPos[1]);
    G2_SetOBJPriority(sOptionMenu.cursor, 3);
    func_ov182_021ab85c(sCursorRects[sOptionMenu.selection][0], sCursorRects[sOptionMenu.selection][2],
                        sCursorRects[sOptionMenu.selection][1], sCursorRects[sOptionMenu.selection][3]);
    func_ov182_021adfbc(func_ov182_021af3f0);
}

// Loads the menu's graphics; without a Wi-Fi ID, the rows of the erasing's and the move's buttons are greyed out
static void func_ov182_021af2ec(void) {
    char plttPath[] = "char/ybBgOption.ncl.l";
    char rowsPlttPath[] = "char/ybBgOption1.ncl.l";
    u8 *pltt;
    DWCBmWifiIdInfo info;
    int i;

    func_ov182_021ab67c("char/jbBgOption.ncg.l", GX_LoadBG2Char);
    func_ov182_021ab67c("char/jb5OptMenu.nsc.l", GX_LoadBG2Scr);
    sOptionMenu.palette = func_ov182_021bb37c(func_ov182_021ab63c(rowsPlttPath), NULL, 4);
    pltt = func_ov182_021bb37c(func_ov182_021ab63c(plttPath), NULL, 4);
    DWCBm_GetWifiIdInfo(&info);
    if (info.id == 0) {
        u8 *src = pltt + 0xc0;
        u8 *dest = pltt + 0x40;

        for (i = 0; i < 2; i++) {
            MI_CpuCopy8(src, dest, 0x20);
            src += 0x20;
            dest += 0x20;
        }
    }
    DC_FlushRange(pltt, 0x200);
    GX_LoadBGPltt(pltt, 0, 0x200);
    func_ov182_021bb420(pltt);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021af3f0(void) {
    func_ov182_021bbac4(2, 1, 1, 8);
    func_ov182_021bbac4(2, 0, 0x14, 8);
    func_ov182_021bc1dc(1, 1);
    func_ov182_021bc1dc(0, 0x14);
    func_ov182_021adfbc(func_ov182_021af428);
}

static void func_ov182_021af428(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab6d0(0);
        func_ov182_021adfbc(func_ov182_021af454);
    }
}

static void func_ov182_021af454(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021af474);
    }
}

static void func_ov182_021af474(void) {
    func_ov182_021af484();
    func_ov182_021af524();
    func_ov182_021af528();
}

static void func_ov182_021af484(void) {
    u32 i;

    for (i = 0; i < 3; i++) {
        if (func_ov182_021bc888(sButtonRects[i])) {
            DWCUtilUiObject_SetStateIfIdle(1);
            sOptionMenu.selection = i;
            func_ov182_021ab85c(sCursorRects[sOptionMenu.selection][0], sCursorRects[sOptionMenu.selection][2],
                                sCursorRects[sOptionMenu.selection][1], sCursorRects[sOptionMenu.selection][3]);
            return;
        }
    }
    if (func_ov182_021bc798(1)) {
        DWCUtilUiObject_SetStateIfIdle(1);
    } else if (func_ov182_021bc798(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (func_ov182_021bc7c8(0x40)) {
        func_ov182_021af6a4(1);
    } else if (func_ov182_021bc7c8(0x80)) {
        func_ov182_021af6a4(3);
    }
}

static void func_ov182_021af524(void) {
}

// Without a Wi-Fi ID, only the console's information can be chosen
static void func_ov182_021af528(void) {
    DWCBmWifiIdInfo info;

    switch (DWCUtilUiObject_GetState()) {
    case 0:
        func_ov182_021b80f4(7);
        break;
    case 1:
        DWCBm_GetWifiIdInfo(&info);
        if (sOptionMenu.selection != 0 && info.id == 0) {
            func_ov182_021b80f4(9);
            DWCUtilUiObject_SetState(-1);
            return;
        }
        func_ov182_021b80f4(6);
        func_ov182_021af6f8();
        sOptionMenu.started = TRUE;
        break;
    default:
        return;
    }
    func_ov182_021adfbc(func_ov182_021af598);
}

static void func_ov182_021af598(void) {
    DWCUtilUiObject_Disable();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021af5b0);
}

static void func_ov182_021af5b0(void) {
    if (func_ov182_021bbaac(1) == 0) {
        if (sOptionMenu.started) {
            DWCUtilUiObject_Exit();
        }
        func_ov182_021bbac4(3, 1, 1, 8);
        func_ov182_021bbac4(3, 0, 0x14, 8);
        func_ov182_021adfbc(func_ov182_021af5f0);
    }
}

static void func_ov182_021af5f0(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0 && DWCUtilUiObject_IsIdle()) {
        func_ov182_021bcc9c(sOptionMenu.cursor);
        func_ov182_021ab8e4();
        DWCUtilScreen_Stop();
        func_ov182_021bb420(sOptionMenu.palette);
        func_ov182_021bc228(1, 1);
        func_ov182_021bc228(0, 0x14);
        if (!sOptionMenu.started) {
            func_ov182_021adfc8(0, 0);
            func_ov182_021adfbc(func_ov182_021ae748);
        } else {
            switch (sOptionMenu.selection) {
            case 0:
                func_ov182_021adfc8(0, 1);
                func_ov182_021adfbc(func_ov182_021aef34);
                break;
            case 1:
                func_ov182_021adfc8(0, 1);
                func_ov182_021adfbc(func_ov182_021aeafc);
                break;
            case 2:
                func_ov182_021adfc8(0, 1);
                func_ov182_021adfbc(func_ov182_021afba4);
                break;
            }
        }
    }
}

// Up and down move the cursor through the three buttons, wrapping around
static void func_ov182_021af6a4(int direction) {
    if (direction == 1) {
        sOptionMenu.selection = FX_ModS32(sOptionMenu.selection + 2, 3);
    } else {
        sOptionMenu.selection = FX_ModS32(sOptionMenu.selection + 1, 3);
    }
    func_ov182_021b80f4(8);
    func_ov182_021ab85c(sCursorRects[sOptionMenu.selection][0], sCursorRects[sOptionMenu.selection][2],
                        sCursorRects[sOptionMenu.selection][1], sCursorRects[sOptionMenu.selection][3]);
}

// Lights up the chosen button with its row of the palette
static void func_ov182_021af6f8(void) {
    func_ov182_021b7014(sOptionMenu.palette, sButtonRows[sOptionMenu.selection], sButtonRows[sOptionMenu.selection]);
}
