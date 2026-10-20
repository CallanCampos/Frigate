#include "dwc_util_screen_ap.h"
#include "dwc/bm.h"
#include "dwc_util_address_settings.h"
#include "dwc_util_bg_buffer.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_button_sprites.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_heap.h"
#include "dwc_util_main.h"
#include "dwc_util_main_menu.h"
#include "dwc_util_oam.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_way.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "dwc_util_usb_intro.h"
#include "dwc_util_wifi_settings.h"
#include "nitro/gx.h"

// The buttons of the screen, as the cursor goes over them: the three connections, the wide button, and the three
// buttons that erase a connection
#define SCREEN_AP_CONNECTIONS 3
#define SCREEN_AP_WIDE_BUTTON 3
#define SCREEN_AP_FIRST_ERASE 4
#define SCREEN_AP_BUTTONS 7

// How the screen ends: not yet, with a button, or with B
enum {
    SCREEN_AP_RUNNING,
    SCREEN_AP_CHOSEN,
    SCREEN_AP_CANCELLED,
};

typedef struct DWCUtilScreenAp {
    // The screen whose palette rows light up the chosen button
    void *palette;
    // The buttons of the connections, and the buttons that erase them, none for a connection not in use
    DWCUtilOamNode *buttons[SCREEN_AP_CONNECTIONS];
    DWCUtilOamNode *eraseButtons[SCREEN_AP_CONNECTIONS];
    // The column that the cursor left the top or the bottom row from, for the wide button to go back to
    u8 column;
    u8 end;
} DWCUtilScreenAp;

typedef struct DWCUtilScreenApState {
    u8 cursor;
    DWCUtilScreenAp *work;
} DWCUtilScreenApState;

static void func_ov182_021b53b4(void);
static void func_ov182_021b5464(void);
static void func_ov182_021b550c(void);
static void func_ov182_021b5544(void);
static void func_ov182_021b5570(void);
static void func_ov182_021b5598(void);
static void func_ov182_021b55a8(void);
static void func_ov182_021b5648(void);
static void func_ov182_021b564c(void);
static void func_ov182_021b5710(void);
static void func_ov182_021b573c(void);
static void func_ov182_021b5790(void);
static void func_ov182_021b58a8(void);
static void func_ov182_021b58f4(int direction);
static void func_ov182_021b59f0(void);
static void func_ov182_021b5a48(void);
static void func_ov182_021b5b20(void);
static void func_ov182_021b5b3c(void);

// The cell of a connection's button by its status (DWCBmConnection.status, 3 for one not in use), where the cursor
// frames each button and where it is touched (left, top, right and bottom), the slot of the erase prompt's number by
// language, and where the buttons of the connections and then their erase buttons are. Declared in the order that,
// with MWCC's sort of the file's data by size, gives the ROM's .rodata
static const u8 sScreenApCells[4] = { 0x13, 0x14, 0x12, 0x56 };
static const u16 sScreenApCursorRects[SCREEN_AP_BUTTONS][4] = {
    { 0x06, 0x1e, 0x46, 0x48 }, { 0x58, 0x1e, 0x98, 0x48 }, { 0xaa, 0x1e, 0xea, 0x48 }, { 0x06, 0x76, 0xea, 0x92 },
    { 0x09, 0x54, 0x43, 0x70 }, { 0x5b, 0x54, 0x95, 0x70 }, { 0xad, 0x54, 0xe7, 0x70 },
};
static const u16 sScreenApTouchRects[SCREEN_AP_BUTTONS][4] = {
    { 0x08, 0x20, 0x54, 0x56 }, { 0x5a, 0x20, 0xa6, 0x56 }, { 0xac, 0x20, 0xf8, 0x56 }, { 0x08, 0x78, 0xf8, 0xa0 },
    { 0x08, 0x54, 0x54, 0x70 }, { 0x5a, 0x54, 0xa6, 0x70 }, { 0xac, 0x54, 0xf8, 0x70 },
};
static const s8 sScreenApMsgSlots[8] = { 3, 0x2d, 0x27, 0x1d, 0x32, 0x30, 6, 0 };
static const u16 sScreenApButtonPos[SCREEN_AP_CONNECTIONS * 2][2] = {
    { 8, 0x30 }, { 0x5a, 0x30 }, { 0xac, 0x30 }, { 0x0c, 0x58 }, { 0x5e, 0x58 }, { 0xb0, 0x58 },
};

static DWCUtilScreenApState sScreenAp;

void func_ov182_021b536c(void) {
    sScreenAp.work = func_ov182_021bc4d8(sizeof(DWCUtilScreenAp), 4);
    sScreenAp.work->end = SCREEN_AP_RUNNING;
    func_ov182_021b53b4();
    func_ov182_021ab96c(1);
    DWCUtilScreen_Start(0x2f, -1, 0);
    func_ov182_021ab6a4(0);
    func_ov182_021b5464();
    func_ov182_021b58a8();
    func_ov182_021adfbc(func_ov182_021b550c);
}

static void func_ov182_021b53b4(void) {
    char palettePath[] = "char/ybBgStep11.ncl.l";

    func_ov182_021ab67c("char/ybObjWay.ncl.l", GX_LoadOBJPltt);
    func_ov182_021ab67c("char/jbBgStep1.ncg.l", GX_LoadBG2Char);
    func_ov182_021ab67c("char/jbBgStep1.ncl.l", GX_LoadBGPltt);
    func_ov182_021ab67c("char/jb2Ap.nsc.l", GX_LoadBG2Scr);
    sScreenAp.work->palette = func_ov182_021bb37c(func_ov182_021ab63c(palettePath), NULL, 4);
    func_ov182_021bc1dc(1, 0x10);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG1Priority(3);
    G2_SetBG2Priority(3);
}

// The buttons of the connections, and the erase buttons of those in use
static void func_ov182_021b5464(void) {
    int i;
    int status;

    for (i = 0; i < SCREEN_AP_CONNECTIONS; i++) {
        status = func_ov182_021b7b0c(i);
        if (status == DWC_BM_CONNECTION_UNUSED) {
            status = 3;
        } else {
            int pos = i + SCREEN_AP_CONNECTIONS;

            sScreenAp.work->eraseButtons[i] = func_ov182_021bb97c(0, 0x11, TRUE);
            func_ov182_021bb7e0(sScreenAp.work->eraseButtons[i], -1, sScreenApButtonPos[pos][0],
                                sScreenApButtonPos[pos][1]);
            func_ov182_021bb878(sScreenAp.work->eraseButtons[i], -1, 3);
        }
        sScreenAp.work->buttons[i] = func_ov182_021bb97c(0, sScreenApCells[status], TRUE);
        func_ov182_021bb7e0(sScreenAp.work->buttons[i], -1, sScreenApButtonPos[i][0], sScreenApButtonPos[i][1]);
        func_ov182_021bb878(sScreenAp.work->buttons[i], -1, 3);
    }
}

static void func_ov182_021b550c(void) {
    func_ov182_021bbac4(2, 1, 1, 8);
    func_ov182_021bbac4(2, 0, 0x14, 8);
    func_ov182_021bc1dc(1, 1);
    func_ov182_021bc1dc(0, 0x14);
    func_ov182_021adfbc(func_ov182_021b5544);
}

static void func_ov182_021b5544(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab6d0(0);
        func_ov182_021adfbc(func_ov182_021b5570);
    }
}

static void func_ov182_021b5570(void) {
    if (DWCUtilUiObject_GetState() != -2 && func_ov182_021b854c() != TRUE) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021b5598);
    }
}

static void func_ov182_021b5598(void) {
    func_ov182_021b55a8();
    func_ov182_021b5648();
    func_ov182_021b564c();
}

static void func_ov182_021b55a8(void) {
    u32 i;

    for (i = 0; i < SCREEN_AP_BUTTONS; i++) {
        if (func_ov182_021bc888(sScreenApTouchRects[i])) {
            DWCUtilUiObject_SetStateIfIdle(1);
            sScreenAp.cursor = i;
            func_ov182_021b58a8();
            return;
        }
    }
    if (func_ov182_021bc798(1)) {
        DWCUtilUiObject_SetStateIfIdle(1);
    } else if (func_ov182_021bc798(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (func_ov182_021bc7c8(0x40)) {
        func_ov182_021b58f4(1);
    } else if (func_ov182_021bc7c8(0x80)) {
        func_ov182_021b58f4(3);
    } else if (func_ov182_021bc7c8(0x20)) {
        func_ov182_021b58f4(0);
    } else if (func_ov182_021bc7c8(0x10)) {
        func_ov182_021b58f4(2);
    }
}

static void func_ov182_021b5648(void) {
}

static void func_ov182_021b564c(void) {
    int index;

    switch (DWCUtilUiObject_GetState()) {
    case 0:
        switch (func_ov182_021ae024()) {
        case 0:
            func_ov182_021b80f4(7);
            sScreenAp.work->end = SCREEN_AP_CANCELLED;
            break;
        case 1:
            DWCUtilUiObject_Disable();
            func_ov182_021adfbc(func_ov182_021ae4f4);
            return;
        }
        break;
    case 1:
        sScreenAp.work->end = SCREEN_AP_CHOSEN;
        if (sScreenAp.cursor >= SCREEN_AP_FIRST_ERASE) {
            index = sScreenAp.cursor - SCREEN_AP_FIRST_ERASE;
            if (func_ov182_021b7b0c(index) == DWC_BM_CONNECTION_UNUSED) {
                func_ov182_021b80f4(9);
                DWCUtilUiObject_SetState(-1);
                return;
            }
            func_ov182_021b80f4(6);
            func_ov182_021b8cf0(0x46, 0, 1, sScreenApMsgSlots[func_ov182_021ae018()], index + 1);
            func_ov182_021b59f0();
            DWCUtilUiObject_Disable();
            func_ov182_021adfbc(func_ov182_021b5a48);
            return;
        }
        if (sScreenAp.cursor <= 2) {
            func_ov182_021b7b20(sScreenAp.cursor);
        }
        func_ov182_021b80f4(6);
        func_ov182_021b5b3c();
        break;
    default:
        return;
    }
    func_ov182_021adfbc(func_ov182_021b5710);
}

static void func_ov182_021b5710(void) {
    if (sScreenAp.work->end == SCREEN_AP_CANCELLED) {
        func_ov182_021b849c();
    }
    DWCUtilUiObject_Disable();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021b573c);
}

static void func_ov182_021b573c(void) {
    if (func_ov182_021bbaac(1) == 0) {
        if (sScreenAp.work->end == SCREEN_AP_CHOSEN &&
            (sScreenAp.cursor == SCREEN_AP_WIDE_BUTTON ||
             func_ov182_021b781c()->connection.status != DWC_BM_CONNECTION_UNUSED)) {
            DWCUtilUiObject_Exit();
        }
        func_ov182_021bbac4(3, 1, 1, 8);
        func_ov182_021bbac4(3, 0, 0x14, 8);
        func_ov182_021adfbc(func_ov182_021b5790);
    }
}

static void func_ov182_021b5790(void) {
    u32 i;

    if (func_ov182_021bbaac(1) != 0 || func_ov182_021bbaac(0) != 0) {
        return;
    }
    if (!func_ov182_021b87b0() || !DWCUtilUiObject_IsIdle()) {
        return;
    }
    for (i = 0; i < SCREEN_AP_CONNECTIONS; i++) {
        if (sScreenAp.work->buttons[i] != NULL) {
            func_ov182_021bb690(sScreenAp.work->buttons[i]);
        }
    }
    for (i = 0; i < SCREEN_AP_CONNECTIONS; i++) {
        if (sScreenAp.work->eraseButtons[i] != NULL) {
            func_ov182_021bb690(sScreenAp.work->eraseButtons[i]);
        }
    }
    func_ov182_021ab8e4();
    DWCUtilScreen_Stop();
    func_ov182_021bb420(sScreenAp.work->palette);
    func_ov182_021ab67c("char/ybObjMain.ncl.l", GX_LoadOBJPltt);
    func_ov182_021bc228(1, 1);
    func_ov182_021bc228(0, 0x14);
    if (sScreenAp.work->end == SCREEN_AP_CANCELLED) {
        func_ov182_021adfc8(0, 0);
        func_ov182_021adfbc(func_ov182_021ae748);
    } else {
        switch (sScreenAp.cursor) {
        case 0:
        case 1:
        case 2:
            func_ov182_021adfc8(2, 0);
            if (func_ov182_021b781c()->connection.status == DWC_BM_CONNECTION_UNUSED) {
                func_ov182_021adfbc(func_ov182_021b5bb8);
            } else {
                func_ov182_021adff0(0, 0);
                func_ov182_021b43f8(0);
                func_ov182_021adfbc(func_ov182_021b2d48);
            }
            break;
        case SCREEN_AP_WIDE_BUTTON:
            func_ov182_021adfc8(2, 1);
            func_ov182_021adfbc(func_ov182_021b688c);
            break;
        }
    }
    func_ov182_021bc4f0(&sScreenAp.work);
}

// Frames the button under the cursor: the buttons of the top row and the wide button with the four corners, an erase
// button with the pair of sprites
static void func_ov182_021b58a8(void) {
    if (sScreenAp.cursor < SCREEN_AP_FIRST_ERASE) {
        func_ov182_021ab85c(sScreenApCursorRects[sScreenAp.cursor][0], sScreenApCursorRects[sScreenAp.cursor][2],
                            sScreenApCursorRects[sScreenAp.cursor][1], sScreenApCursorRects[sScreenAp.cursor][3]);
        return;
    }
    func_ov182_021ab7f0(0, sScreenApCursorRects[sScreenAp.cursor][0], sScreenApCursorRects[sScreenAp.cursor][2],
                        sScreenApCursorRects[sScreenAp.cursor][1]);
}

// Moves the cursor left, up, right or down (0 to 3)
static void func_ov182_021b58f4(int direction) {
    BOOL moved = TRUE;

    switch (sScreenAp.cursor) {
    case 0:
        sScreenAp.work->column = 0;
        if (direction == 0) {
            sScreenAp.cursor = 2;
        } else if (direction == 2) {
            sScreenAp.cursor = 1;
        } else if (direction == 1) {
            sScreenAp.cursor = 3;
        } else {
            sScreenAp.cursor = 4;
        }
        break;
    case 1:
        sScreenAp.work->column = 1;
        if (direction == 0) {
            sScreenAp.cursor = 0;
        } else if (direction == 2) {
            sScreenAp.cursor = 2;
        } else if (direction == 1) {
            sScreenAp.cursor = 3;
        } else {
            sScreenAp.cursor = 5;
        }
        break;
    case 2:
        sScreenAp.work->column = 2;
        if (direction == 0) {
            sScreenAp.cursor = 1;
        } else if (direction == 2) {
            sScreenAp.cursor = 0;
        } else if (direction == 1) {
            sScreenAp.cursor = 3;
        } else {
            sScreenAp.cursor = 6;
        }
        break;
    case 3: {
        u8 column = sScreenAp.work->column;
        int erase = column + SCREEN_AP_FIRST_ERASE;

        if (direction == 1) {
            sScreenAp.cursor = erase;
        } else if (direction == 3) {
            sScreenAp.cursor = column;
        } else {
            moved = FALSE;
        }
        break;
    }
    case 4:
        sScreenAp.work->column = 0;
        if (direction == 0) {
            sScreenAp.cursor = 6;
        } else if (direction == 2) {
            sScreenAp.cursor = 5;
        } else if (direction == 1) {
            sScreenAp.cursor = 0;
        } else {
            sScreenAp.cursor = 3;
        }
        break;
    case 5:
        sScreenAp.work->column = 1;
        if (direction == 0) {
            sScreenAp.cursor = 4;
        } else if (direction == 2) {
            sScreenAp.cursor = 6;
        } else if (direction == 1) {
            sScreenAp.cursor = 1;
        } else {
            sScreenAp.cursor = 3;
        }
        break;
    case 6:
        sScreenAp.work->column = 2;
        if (direction == 0) {
            sScreenAp.cursor = 5;
        } else if (direction == 2) {
            sScreenAp.cursor = 4;
        } else if (direction == 1) {
            sScreenAp.cursor = 2;
        } else {
            sScreenAp.cursor = 3;
        }
        break;
    }
    if (moved) {
        func_ov182_021b80f4(8);
        func_ov182_021b58a8();
    }
}

// Shows the chosen erase button pressed
static void func_ov182_021b59f0(void) {
    int index = sScreenAp.cursor - SCREEN_AP_FIRST_ERASE;

    func_ov182_021bb914(0, 0x32, func_ov182_021bb6ec(sScreenAp.work->eraseButtons[index], 0));
    func_ov182_021bb7e0(sScreenAp.work->eraseButtons[index], -1, sScreenApButtonPos[index + 3][0],
                        sScreenApButtonPos[index + 3][1]);
    func_ov182_021bb878(sScreenAp.work->eraseButtons[index], -1, 3);
}

// The answer to whether to erase the connection: yes erases it and shows its button as not in use, no shows the erase
// button again
static void func_ov182_021b5a48(void) {
    int index = sScreenAp.cursor - SCREEN_AP_FIRST_ERASE;

    switch (func_ov182_021b8f14()) {
    case 1:
        func_ov182_021b80f4(0xe);
        func_ov182_021b7e78(index);
        func_ov182_021bb914(0, 0x56, func_ov182_021bb6ec(sScreenAp.work->buttons[index], 0));
        func_ov182_021bb7e0(sScreenAp.work->buttons[index], -1, sScreenApButtonPos[index][0],
                            sScreenApButtonPos[index][1]);
        func_ov182_021bb878(sScreenAp.work->buttons[index], -1, 3);
        func_ov182_021bb690(sScreenAp.work->eraseButtons[index]);
        sScreenAp.work->eraseButtons[index] = NULL;
        break;
    case 0:
        func_ov182_021b80f4(7);
        func_ov182_021bb914(0, 0x11, func_ov182_021bb6ec(sScreenAp.work->eraseButtons[index], 0));
        func_ov182_021bb7e0(sScreenAp.work->eraseButtons[index], -1, sScreenApButtonPos[index + 3][0],
                            sScreenApButtonPos[index + 3][1]);
        func_ov182_021bb878(sScreenAp.work->eraseButtons[index], -1, 3);
        break;
    default:
        return;
    }
    func_ov182_021b8eec();
    func_ov182_021adfbc(func_ov182_021b5b20);
}

static void func_ov182_021b5b20(void) {
    if (!func_ov182_021b8f24()) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021b5598);
    }
}

// Lights up the chosen button: the palette row of its place, and the button's own palette by the connection's status
static void func_ov182_021b5b3c(void) {
    u8 rows[4] = { 3, 4, 5, 7 };
    u8 palettes[4] = { 10, 9, 11, 6 };
    int status;

    if (sScreenAp.cursor <= SCREEN_AP_WIDE_BUTTON) {
        func_ov182_021b7014(sScreenAp.work->palette, rows[sScreenAp.cursor], rows[sScreenAp.cursor]);
        if (sScreenAp.cursor != SCREEN_AP_WIDE_BUTTON) {
            status = func_ov182_021b7b0c(sScreenAp.cursor);
            if (status > 2) {
                status = 3;
            }
            func_ov182_021bb778(sScreenAp.work->buttons[sScreenAp.cursor], -1, 0, palettes[status]);
        }
    }
}
