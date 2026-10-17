#include "dwc_util_main_menu.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_button_sprites.h"
#include "dwc_util_cell.h"
#include "dwc_util_connection.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_main.h"
#include "dwc_util_msg_resource.h"
#include "dwc_util_option_menu.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_screen.h"
#include "dwc_util_shared.h"
#include "dwc_util_task.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "nitro/gx.h"

// The choice on the first screen, and the palette whose rows light up the chosen button
typedef struct DWCUtilMainMenu {
    u8 selection;
    void *palette;
} DWCUtilMainMenu;

static void func_ov182_021ae51c(void);
static void func_ov182_021ae6d0(void);
static void func_ov182_021ae6f8(void);
static void func_ov182_021ae7a0(void);
static void func_ov182_021ae838(void);
static void func_ov182_021ae870(void);
static void func_ov182_021ae89c(void);
static void func_ov182_021ae8bc(void);
static void func_ov182_021ae8cc(void);
static void func_ov182_021ae990(void);
static void func_ov182_021ae994(void);
static void func_ov182_021ae9cc(void);
static void func_ov182_021ae9e4(void);
static void func_ov182_021aea18(void);
static void func_ov182_021aea8c(int direction);
static void func_ov182_021aead4(void);

// Where the cursor frames the two buttons, and where they are touched: left, top, right and bottom
static const u16 sCursorRects[2][4] = { { 6, 0x1e, 0x9e, 0x92 }, { 0xb2, 0x1e, 0xea, 0x92 } };
static const u16 sButtonRects[2][4] = { { 8, 0x20, 0xac, 0xa0 }, { 0xb4, 0x20, 0xf8, 0xa0 } };

// The message files by language
static char *sMsgPaths[7] = {
    "msg/jap.bmg.l", "msg/eng.bmg.l", "msg/fre.bmg.l", "msg/ger.bmg.l",
    "msg/ita.bmg.l", "msg/spa.bmg.l", "msg/kor.bmg.l",
};

static DWCUtilMainMenu sMainMenu;

void func_ov182_021ae4f4(void) {
    func_ov182_021bbac4(3, 1, 0x3f, 0x14);
    func_ov182_021bbac4(3, 0, 0x3f, 0x14);
    func_ov182_021adfbc(func_ov182_021ae51c);
}

static void func_ov182_021ae51c(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021bd2b8(0, 0);
        func_ov182_021bd2b8(1, 0);
        func_ov182_021bb8fc(1);
        func_ov182_021bb8fc(0);
        func_ov182_021ab95c();
        func_ov182_021ab7dc();
        func_ov182_021ae0ec(data_ov182_021c40b8);
        func_ov182_021ae0a8();
        func_ov182_021b780c();
        func_ov182_021ae04c();
    }
}

void func_ov182_021ae574(void) {
    func_ov182_021b77dc();
    func_ov182_021ae084();
    func_ov182_021ab7c8();
    func_ov182_021ab90c();
    DWCUtilScreen_Reset();
    if (func_ov182_021ae018() == 1 && func_ov182_021ae034(2)) {
        data_ov182_021c40b8 = func_ov182_021ae0b8("msg/usa.bmg.l");
    } else {
        data_ov182_021c40b8 = func_ov182_021ae0b8(sMsgPaths[func_ov182_021ae018()]);
    }
    func_ov182_021bb8dc(1, func_ov182_021ab63c("char/jtMain.nce.l"));
    func_ov182_021bb8dc(0, func_ov182_021ab63c("char/jbMain.nce.l"));
    func_ov182_021ab67c("char/jtBgMain.ncg.l", GXS_LoadBG1Char);
    func_ov182_021ab67c("char/jtBgMain.ncl.l", GXS_LoadBGPltt);
    func_ov182_021ab67c("char/jtObjMain.ncg.l", GXS_LoadOBJ);
    func_ov182_021ab67c("char/xtObjMain.ncl.l", GXS_LoadOBJPltt);
    func_ov182_021ab67c("char/jbBgStep1.ncg.l", GX_LoadBG2Char);
    func_ov182_021ab67c("char/jbBgStep1.ncl.l", GX_LoadBGPltt);
    func_ov182_021ab67c("char/jbObjMain.ncg.l", GX_LoadOBJ);
    func_ov182_021ab67c("char/ybObjMain.ncl.l", GX_LoadOBJPltt);
    switch (func_ov182_021ae024()) {
    case 0:
        func_ov182_021ab67c("char/jtTop.nsc.l", GXS_LoadBG1Scr);
        break;
    case 1:
        func_ov182_021ab67c("char/jtStep1.nsc.l", GXS_LoadBG1Scr);
        break;
    }
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    func_ov182_021bc1dc(1, 2);
    func_ov182_021bc1dc(0, 2);
    func_ov182_021adfbc(func_ov182_021ae6d0);
}

static void func_ov182_021ae6d0(void) {
    func_ov182_021bbac4(2, 1, 2, 0x14);
    func_ov182_021bbac4(2, 0, 2, 0x14);
    func_ov182_021adfbc(func_ov182_021ae6f8);
}

static void func_ov182_021ae6f8(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        switch (func_ov182_021ae024()) {
        case 0:
            func_ov182_021adfc8(0, 1);
            func_ov182_021adfbc(func_ov182_021ae748);
            break;
        case 1:
            func_ov182_021adfc8(1, 1);
            func_ov182_021adfbc(func_ov182_021b536c);
            break;
        }
    }
}

void func_ov182_021ae748(void) {
    func_ov182_021ae7a0();
    func_ov182_021ab96c(0);
    DWCUtilScreen_Start(0x2e, -1, 0);
    func_ov182_021b87d0(4);
    func_ov182_021ab85c(sCursorRects[sMainMenu.selection][0], sCursorRects[sMainMenu.selection][2],
                        sCursorRects[sMainMenu.selection][1], sCursorRects[sMainMenu.selection][3]);
    func_ov182_021adfbc(func_ov182_021ae838);
}

static void func_ov182_021ae7a0(void) {
    char palettePath[] = "char/ybBgStep11.ncl.l";

    func_ov182_021ab67c("char/jbBgStep1.ncg.l", GX_LoadBG2Char);
    func_ov182_021ab67c("char/jbBgStep1.ncl.l", GX_LoadBGPltt);
    func_ov182_021ab67c("char/jb2Menu.nsc.l", GX_LoadBG2Scr);
    sMainMenu.palette = func_ov182_021bb37c(func_ov182_021ab63c(palettePath), NULL, 4);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

static void func_ov182_021ae838(void) {
    func_ov182_021bbac4(2, 1, 1, 8);
    func_ov182_021bbac4(2, 0, 0x14, 8);
    func_ov182_021bc1dc(1, 1);
    func_ov182_021bc1dc(0, 0x14);
    func_ov182_021adfbc(func_ov182_021ae870);
}

static void func_ov182_021ae870(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab6d0(0);
        func_ov182_021adfbc(func_ov182_021ae89c);
    }
}

static void func_ov182_021ae89c(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021ae8bc);
    }
}

static void func_ov182_021ae8bc(void) {
    func_ov182_021ae8cc();
    func_ov182_021ae990();
    func_ov182_021ae994();
}

static void func_ov182_021ae8cc(void) {
    u32 i;

    for (i = 0; i < 2; i++) {
        if (func_ov182_021bc888(sButtonRects[i])) {
            DWCUtilUiObject_SetStateIfIdle(1);
            sMainMenu.selection = i;
            func_ov182_021ab85c(sCursorRects[sMainMenu.selection][0], sCursorRects[sMainMenu.selection][2],
                                sCursorRects[sMainMenu.selection][1], sCursorRects[sMainMenu.selection][3]);
            return;
        }
    }
    if (func_ov182_021bc798(1)) {
        DWCUtilUiObject_SetStateIfIdle(1);
    } else if (func_ov182_021bc798(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (func_ov182_021bc7c8(0x40)) {
        func_ov182_021aea8c(1);
    } else if (func_ov182_021bc7c8(0x80)) {
        func_ov182_021aea8c(3);
    } else if (func_ov182_021bc7c8(0x20)) {
        func_ov182_021aea8c(0);
    } else if (func_ov182_021bc7c8(0x10)) {
        func_ov182_021aea8c(2);
    }
}

static void func_ov182_021ae990(void) {
}

static void func_ov182_021ae994(void) {
    switch (DWCUtilUiObject_GetState()) {
    case 0:
        func_ov182_021b80f4(7);
        func_ov182_021adfbc(func_ov182_021ae4f4);
        break;
    case 1:
        func_ov182_021b80f4(6);
        func_ov182_021aead4();
        func_ov182_021adfbc(func_ov182_021ae9cc);
        break;
    }
}

static void func_ov182_021ae9cc(void) {
    DWCUtilUiObject_Disable();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021ae9e4);
}

static void func_ov182_021ae9e4(void) {
    if (func_ov182_021bbaac(1) == 0) {
        func_ov182_021bbac4(3, 1, 1, 8);
        func_ov182_021bbac4(3, 0, 0x16, 8);
        func_ov182_021adfbc(func_ov182_021aea18);
    }
}

static void func_ov182_021aea18(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab8e4();
        DWCUtilScreen_Stop();
        func_ov182_021bb420(sMainMenu.palette);
        func_ov182_021bc228(1, 1);
        func_ov182_021bc228(0, 0x14);
        switch (sMainMenu.selection) {
        case 0:
            func_ov182_021adfc8(1, 0);
            func_ov182_021adfbc(func_ov182_021b536c);
            break;
        case 1:
            func_ov182_021adfc8(0, 0);
            func_ov182_021adfbc(func_ov182_021af25c);
            break;
        }
    }
}

// Left and right move the cursor to the other button; up and down don't move it
static void func_ov182_021aea8c(int direction) {
    if (direction != 1 && direction != 3) {
        sMainMenu.selection ^= 1;
        func_ov182_021b80f4(8);
        func_ov182_021ab85c(sCursorRects[sMainMenu.selection][0], sCursorRects[sMainMenu.selection][2],
                            sCursorRects[sMainMenu.selection][1], sCursorRects[sMainMenu.selection][3]);
    }
}

// Lights up the chosen button with its row of the palette
static void func_ov182_021aead4(void) {
    u8 rows[2] = { 1, 2 };

    func_ov182_021b7014(sMainMenu.palette, rows[sMainMenu.selection], rows[sMainMenu.selection]);
}
