#include "dwc_util_address_settings.h"
#include "dwc/bm.h"
#include "dwc/ext.h"
#include "dwc_util_address_edit.h"
#include "dwc_util_bg_overlay.h"
#include "dwc_util_button_sprites.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_fade.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_heap.h"
#include "dwc_util_main.h"
#include "dwc_util_oam_attr.h"
#include "dwc_util_oam_buf.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_save_settings.h"
#include "dwc_util_screen.h"
#include "dwc_util_screen_ap.h"
#include "dwc_util_shared.h"
#include "dwc_util_string.h"
#include "dwc_util_task.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_wep_keyboard.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The rows of the list, which the scroll shows four of, and the height of a row in pixels
enum {
    ADDRESS_ROW_SSID,
    ADDRESS_ROW_WEP_KEY,
    ADDRESS_ROW_AUTO_ADDRESS,
    ADDRESS_ROW_IP,
    ADDRESS_ROW_NETMASK,
    ADDRESS_ROW_GATEWAY,
    ADDRESS_ROW_AUTO_DNS,
    ADDRESS_ROW_DNS1,
    ADDRESS_ROW_DNS2,
};
#define ADDRESS_ROW_HEIGHT 29
// The scroll of the list at its end, in pixels, and the travel of the scroll bar's handle
#define ADDRESS_SCROLL_MAX 0x91
#define ADDRESS_BAR_LENGTH 0x37

// What the cursor or a touch is on: a row, one of the two answers of a row of DHCP, or a button
enum {
    ADDRESS_ITEM_SSID,
    ADDRESS_ITEM_WEP_KEY,
    ADDRESS_ITEM_AUTO_ADDRESS_YES,
    ADDRESS_ITEM_AUTO_ADDRESS_NO,
    ADDRESS_ITEM_IP,
    ADDRESS_ITEM_NETMASK,
    ADDRESS_ITEM_GATEWAY,
    ADDRESS_ITEM_AUTO_DNS_YES,
    ADDRESS_ITEM_AUTO_DNS_NO,
    ADDRESS_ITEM_DNS1,
    ADDRESS_ITEM_DNS2,
    ADDRESS_ITEM_TEST,
    ADDRESS_ITEM_SAVE,
    ADDRESS_ITEM_BACK,
    ADDRESS_ITEM_NONE,
};

// A rectangle of the touch screen, as dwc_util_touch.c takes it: its left, top, width and height, or its left, top,
// right and bottom, in a struct so that it can be copied and moved down by a row
typedef struct DWCUtilAddressRect {
    u16 coords[4];
} DWCUtilAddressRect;

typedef struct DWCUtilAddressSettings {
    DWCUtilTask *task;
    // The frames that each answer of the two rows of DHCP stays lit after a touch
    u8 lit[4];
    // The screen behind the rows, the palette, and the palette whose rows light up the buttons
    u16 *backScreen;
    void *palettes[2];
    DWCUtilText *text;
    // The marks of the rows' answers and arrows, and the icon of a connection that a setup made
    GXOamAttr *marks[7];
    GXOamAttr *setupIcon;
    // The task that scrolls the list by a row, and the VBlank task that scrolls the BGs
    DWCUtilTask *scrollTask;
    DWCUtilTask *vblankTask;
    // What was chosen, and the button the cursor was on before it went up to the list
    u8 chosen;
    u8 dragDelay;
    u8 lastButton;
    // The choice of DHCP for the DNS servers, which comes back when DHCP gives the address again
    u8 savedAutoDns;
    u8 moved;
    u8 dragging;
    // Whether the end of the list has played its sound, for the scroll bar's arrows and for the buttons
    u8 barAtEnd;
    u8 atEnd;
} DWCUtilAddressSettings;

// The cursor, where the settings were opened from, the cursor's row on the screen and the scroll of the list in
// pixels, which stay for the next visit, and the screen's work
typedef struct DWCUtilAddressSettingsState {
    u8 cursor;
    u8 from;
    u8 row;
    u16 scroll;
    DWCUtilAddressSettings *work;
} DWCUtilAddressSettingsState;

static void func_ov182_021b2e6c(void);
static void func_ov182_021b2fac(void);
static void func_ov182_021b2fd4(void);
static void func_ov182_021b300c(void);
static void func_ov182_021b3030(void);
static void func_ov182_021b303c(void);
static BOOL func_ov182_021b31c4(void);
static void func_ov182_021b3220(int item);
static void func_ov182_021b331c(BOOL autoAddress);
static void func_ov182_021b3368(BOOL autoDns);
static void func_ov182_021b3390(void);
static void func_ov182_021b34f8(void);
static void func_ov182_021b3510(void);
static void func_ov182_021b3544(void);
static void func_ov182_021b3740(void);
static void func_ov182_021b37b4(int index, int row);
static void func_ov182_021b38c0(int index, int row);
static BOOL func_ov182_021b3998(int item);
static void func_ov182_021b39fc(int mark, int frame, int row);
static void func_ov182_021b3a9c(const u8 *str, int row);
static void func_ov182_021b3b6c(const u8 *address, int row);
static void func_ov182_021b3bc4(void);
static int func_ov182_021b3ce0(void);
static int func_ov182_021b3e68(int index);
static int func_ov182_021b3e94(int row);
static void func_ov182_021b3eac(DWCUtilTask *task, void *arg);
static void func_ov182_021b3f24(DWCUtilTask *task, void *arg);
static void func_ov182_021b3f84(DWCUtilTask *task, void *arg);
static void func_ov182_021b3fd8(void);
static void func_ov182_021b4060(void);
static void func_ov182_021b40c8(int item);
static void func_ov182_021b40e8(int direction);
static void func_ov182_021b4278(void);
static void func_ov182_021b4298(DWCUtilTask *task, void *arg);
static void func_ov182_021b42c8(void);
static BOOL func_ov182_021b4334(void);
static void func_ov182_021b43b0(void);
static void func_ov182_021b43e0(void);

static DWCUtilAddressSettingsState sAddressSettings;

// The palette rows of the three buttons
static const u8 sAddressButtonPalettes[3] = { 6, 8, 7 };
// The title's label by language
static const s8 sAddressTitleLabels[7] = { -1, 0x23, 0x27, -1, 0x23, 0x2f, -1 };
// Where the rows are touched: the row's field, and the two answers of a row of DHCP
static const DWCUtilAddressRect sAddressRowRects[3] = {
    { { 0xcc, 0x34, 0x1c, 0x18 } },
    { { 0x8f, 0x34, 0x2c, 0x18 } },
    { { 0xc0, 0x34, 0x2c, 0x18 } },
};
// The item of each row, ADDRESS_ITEM_NONE for a row of DHCP, whose answers are the items
static const u8 sAddressRowItems[9] = {
    ADDRESS_ITEM_SSID,    ADDRESS_ITEM_WEP_KEY, ADDRESS_ITEM_NONE, ADDRESS_ITEM_IP,   ADDRESS_ITEM_NETMASK,
    ADDRESS_ITEM_GATEWAY, ADDRESS_ITEM_NONE,    ADDRESS_ITEM_DNS1, ADDRESS_ITEM_DNS2,
};
// How far down the BG of the rows' labels is, by the top row
static const u8 sAddressLabelOffsets[9] = { 0, 5, 2, 7, 4, 1, 6, 3, 0 };
// The row of each item
static const u8 sAddressItemRows[11] = { 0, 1, 2, 2, 3, 4, 5, 6, 6, 7, 8 };
// The cells of the marks: none, an arrow, the lit, the unlit and the greyed answers
static const u8 sAddressMarkCells[5][3] = {
    { 0, 0x29, 0x2c }, { 0x52, 0x53, 0x30 }, { 0, 0x2a, 0x30 }, { 0x54, 0x55, 0 }, { 0, 0x2b, 0 },
};
// Where the screen behind the rows starts, in its entries, by the top row
static const u16 sAddressScreenOffsets[9] = { 0, 0x60, 0xe0, 0x140, 0x1c0, 0x240, 0x2a0, 0x320, 0x3a0 };
// Where the buttons are touched: left, top, right and bottom
static const DWCUtilAddressRect sAddressButtonRects[3] = {
    { { 0x84, 0x1b, 0xfc, 0x2c } },
    { { 0x84, 0xac, 0xfc, 0xbd } },
    { { 0x04, 0xac, 0x7c, 0xbd } },
};
// Where the cursor frames the row's field, the two answers and the three buttons: left, top, right and bottom
static const DWCUtilAddressRect sAddressCursorRects[6] = {
    { { 0xc8, 0x31, 0xe0, 0x4d } }, { { 0xbc, 0x31, 0xe0, 0x4d } }, { { 0x8b, 0x31, 0xaf, 0x4d } },
    { { 0x82, 0x18, 0xee, 0x2c } }, { { 0x82, 0xa9, 0xee, 0xbd } }, { { 0x02, 0xa9, 0x6e, 0xbd } },
};

void func_ov182_021b2d48(void) {
    DWCUtilSettingsEdit *edit = func_ov182_021b781c();
    int i;

    sAddressSettings.work = func_ov182_021bc4d8(sizeof(DWCUtilAddressSettings), 4);
    sAddressSettings.work->savedAutoDns = edit->autoDns;
    sAddressSettings.work->lastButton = ADDRESS_ITEM_SAVE;
    func_ov182_021b2fac();
    func_ov182_021b2e6c();
    if (sAddressSettings.from == 0) {
        DWCUtilScreen_Start(0x30, sAddressTitleLabels[func_ov182_021ae018()], edit->index + 1);
    } else {
        DWCUtilScreen_Start(0x45, -1, 0);
    }
    func_ov182_021ab6a4(1);
    func_ov182_021ab9c0();
    func_ov182_021b70a0(2, 0x55, 0xf1, 0x41, sAddressSettings.scroll * ADDRESS_BAR_LENGTH / ADDRESS_SCROLL_MAX);
    sAddressSettings.work->text = func_ov182_021bbdec(0, 1);
    for (i = 0; i < 7; i++) {
        sAddressSettings.work->marks[i] = func_ov182_021bb960(0, 0x29);
    }
    switch (edit->connection.status) {
    case 1:
        sAddressSettings.work->setupIcon = func_ov182_021bb960(0, 0x50);
        G2_SetOBJPriority(sAddressSettings.work->setupIcon, 3);
        break;
    case 2:
        sAddressSettings.work->setupIcon = func_ov182_021bb960(0, 0x51);
        G2_SetOBJPriority(sAddressSettings.work->setupIcon, 3);
        break;
    }
    sAddressSettings.work->vblankTask = func_ov182_021bd200(1, func_ov182_021b3f84, NULL, 0x6e);
    sAddressSettings.work->task = func_ov182_021bd200(0, func_ov182_021b4298, NULL, 0x78);
    func_ov182_021b3740();
    func_ov182_021b3fd8();
    func_ov182_021adfbc(func_ov182_021b2fd4);
}

static void func_ov182_021b2e6c(void) {
    char backPath[] = "char/jb3ListBack.nsc.l";
    char palettePath[] = "char/ybBgStep2.ncl.l";
    char litPalettePath[] = "char/ybBgStep21.ncl.l";

    G2_SetBG2ControlText(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x7000, GX_BG_CHARBASE(0x18000));
    func_ov182_021ab67c("char/ybObjKb.ncl.l", GX_LoadOBJPltt);
    func_ov182_021ab67c("char/jbBgStep2.ncg.l", GX_LoadBG3Char);
    func_ov182_021ab67c("char/jbBgStep21.ncg.l", GX_LoadBG2Char);
    func_ov182_021ab67c(palettePath, GX_LoadBGPltt);
    func_ov182_021ab67c("char/jb3List.nsc.l", GX_LoadBG3Scr);
    sAddressSettings.work->backScreen = func_ov182_021bb37c(func_ov182_021ab63c(backPath), NULL, 4);
    func_ov182_021b6f38(sAddressSettings.work->backScreen);
    func_ov182_021b6f98();
    sAddressSettings.work->palettes[0] = func_ov182_021bb37c(func_ov182_021ab63c(palettePath), NULL, 4);
    sAddressSettings.work->palettes[1] = func_ov182_021bb37c(func_ov182_021ab63c(litPalettePath), NULL, 4);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(2);
    G2_SetBG2Priority(3);
    G2_SetBG3Priority(2);
}

// The cursor and the scroll start at the top unless the screen comes back from a row's keyboard or keypad
static void func_ov182_021b2fac(void) {
    int again;

    func_ov182_021b781c();
    func_ov182_021adffc(NULL, &again);
    if (again == 0) {
        sAddressSettings.scroll = 0;
        sAddressSettings.cursor = 0;
        sAddressSettings.row = 0;
    }
}

static void func_ov182_021b2fd4(void) {
    func_ov182_021bbac4(2, 1, 1, 8);
    func_ov182_021bbac4(2, 0, 0x1d, 8);
    func_ov182_021bc1dc(1, 1);
    func_ov182_021bc1dc(0, 0x1d);
    func_ov182_021adfbc(func_ov182_021b300c);
}

static void func_ov182_021b300c(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021adfbc(func_ov182_021b3030);
    }
}

static void func_ov182_021b3030(void) {
    func_ov182_021b303c();
    func_ov182_021b3390();
}

static void func_ov182_021b303c(void) {
    if (sAddressSettings.work->scrollTask != NULL || sAddressSettings.work->dragging || func_ov182_021b31c4()) {
        return;
    }
    if (func_ov182_021bc798(1)) {
        func_ov182_021b3220(sAddressSettings.cursor);
        return;
    }
    if (func_ov182_021bc798(2)) {
        func_ov182_021b80f4(7);
        sAddressSettings.work->chosen = ADDRESS_ITEM_BACK;
        func_ov182_021adfbc(func_ov182_021b34f8);
        return;
    }
    if (func_ov182_021bc7c8(0x200)) {
        if (sAddressSettings.scroll == ADDRESS_SCROLL_MAX) {
            if (!sAddressSettings.work->atEnd) {
                func_ov182_021b80f4(9);
                sAddressSettings.work->atEnd = TRUE;
            }
            return;
        }
        func_ov182_021b80f4(0x13);
        sAddressSettings.work->scrollTask = func_ov182_021bd200(0, func_ov182_021b3f24, NULL, 0x78);
        return;
    }
    if (func_ov182_021bc7f8(0x200)) {
        sAddressSettings.work->atEnd = FALSE;
        return;
    }
    if (func_ov182_021bc7c8(0x100)) {
        if (sAddressSettings.scroll == 0) {
            if (!sAddressSettings.work->atEnd) {
                func_ov182_021b80f4(9);
                sAddressSettings.work->atEnd = TRUE;
            }
            return;
        }
        func_ov182_021b80f4(0x13);
        sAddressSettings.work->scrollTask = func_ov182_021bd200(0, func_ov182_021b3eac, NULL, 0x78);
        return;
    }
    if (func_ov182_021bc7f8(0x100)) {
        sAddressSettings.work->atEnd = FALSE;
        return;
    }
    if (func_ov182_021bc7c8(0x40)) {
        func_ov182_021b40e8(1);
        return;
    }
    if (func_ov182_021bc7f8(0x40)) {
        sAddressSettings.work->atEnd = FALSE;
        return;
    }
    if (func_ov182_021bc7c8(0x80)) {
        func_ov182_021b40e8(3);
        return;
    }
    if (func_ov182_021bc7f8(0x80)) {
        sAddressSettings.work->atEnd = FALSE;
        return;
    }
    if (func_ov182_021bc7c8(0x20)) {
        func_ov182_021b40e8(0);
        return;
    }
    if (func_ov182_021bc7c8(0x10)) {
        func_ov182_021b40e8(2);
    }
}

// A touch of a row or a button: TRUE when the touch is on one
static BOOL func_ov182_021b31c4(void) {
    int item;

    func_ov182_021b781c();
    item = func_ov182_021b3ce0();
    if (item == ADDRESS_ITEM_NONE) {
        return FALSE;
    }
    if (!func_ov182_021b3998(item)) {
        func_ov182_021b80f4(9);
        return TRUE;
    }
    func_ov182_021b40c8(item);
    switch (item) {
    case ADDRESS_ITEM_AUTO_ADDRESS_YES:
    case ADDRESS_ITEM_AUTO_ADDRESS_NO:
    case ADDRESS_ITEM_AUTO_DNS_YES:
    case ADDRESS_ITEM_AUTO_DNS_NO:
        func_ov182_021b42c8();
    }
    func_ov182_021b3220(item);
    return TRUE;
}

// Chooses an item: an answer of DHCP takes effect, a row opens its keyboard or keypad, a button tests, saves or goes
// back
static void func_ov182_021b3220(int item) {
    if (!func_ov182_021b3998(item)) {
        func_ov182_021b80f4(9);
        return;
    }
    // The answers of a row of DHCP are yes (0) and no (1), the one chosen turns DHCP on when it is not no
    switch (item) {
    case ADDRESS_ITEM_AUTO_ADDRESS_YES:
    case ADDRESS_ITEM_AUTO_ADDRESS_NO:
        func_ov182_021b80f4(6);
        func_ov182_021b331c(((item - ADDRESS_ITEM_AUTO_ADDRESS_YES) ^ 1) != 0);
        return;
    case ADDRESS_ITEM_AUTO_DNS_YES:
    case ADDRESS_ITEM_AUTO_DNS_NO:
        func_ov182_021b80f4(6);
        func_ov182_021b3368(((item - ADDRESS_ITEM_AUTO_DNS_YES) ^ 1) != 0);
        return;
    }
    sAddressSettings.work->chosen = item;
    if (item == ADDRESS_ITEM_TEST || item == ADDRESS_ITEM_SAVE) {
        func_ov182_021b4278();
        if (!func_ov182_021b4334()) {
            func_ov182_021b80f4(9);
            func_ov182_021b8cf0(6, 1, 1, -1, 0);
            func_ov182_021adfbc(func_ov182_021b43b0);
            return;
        }
        if (item == ADDRESS_ITEM_TEST) {
            func_ov182_021b80f4(6);
        } else {
            func_ov182_021b80f4(0xe);
        }
    } else if (item == ADDRESS_ITEM_BACK) {
        func_ov182_021b4278();
        func_ov182_021b80f4(7);
    } else {
        func_ov182_021b80f4(6);
        func_ov182_021b39fc(0, 1, func_ov182_021b3e68(sAddressItemRows[item]));
        func_ov182_021b3bc4();
    }
    func_ov182_021b7174();
    func_ov182_021adfbc(func_ov182_021b34f8);
}

// DHCP for the address: without it, DHCP can't give the DNS servers either; with it, they come from DHCP again if
// they did
static void func_ov182_021b331c(BOOL autoAddress) {
    DWCUtilSettingsEdit *edit = func_ov182_021b781c();
    BOOL autoDns;

    if (edit->autoAddress == autoAddress) {
        return;
    }
    if (autoAddress) {
        autoDns = sAddressSettings.work->savedAutoDns != 0;
    } else {
        sAddressSettings.work->savedAutoDns = edit->autoDns;
        autoDns = FALSE;
    }
    func_ov182_021b782c(autoAddress);
    func_ov182_021b7840(autoDns);
    func_ov182_021b3740();
}

static void func_ov182_021b3368(BOOL autoDns) {
    DWCUtilSettingsEdit *edit = func_ov182_021b781c();

    if (edit->autoDns != autoDns) {
        sAddressSettings.work->savedAutoDns = autoDns;
        func_ov182_021b7840(autoDns);
        func_ov182_021b3740();
    }
}

// The scroll bar: its handle dragged, or its arrows
static void func_ov182_021b3390(void) {
    int offset;

    if (sAddressSettings.work->scrollTask != NULL) {
        return;
    }
    if (sAddressSettings.work->dragDelay != 0) {
        sAddressSettings.work->dragDelay--;
    }
    switch (func_ov182_021b7150()) {
    case 1:
        sAddressSettings.work->dragging = TRUE;
        break;
    case 2:
        if (sAddressSettings.work->dragDelay == 0) {
            func_ov182_021ab8e4();
            sAddressSettings.scroll = func_ov182_021b7144() * ADDRESS_SCROLL_MAX / ADDRESS_BAR_LENGTH;
            func_ov182_021b3740();
            sAddressSettings.work->dragDelay = 4;
        }
        break;
    case 3:
        sAddressSettings.work->dragging = FALSE;
        sAddressSettings.scroll = func_ov182_021b7144() * ADDRESS_SCROLL_MAX / ADDRESS_BAR_LENGTH;
        func_ov182_021b80f4(0x13);
        func_ov182_021b3740();
        offset = FX_ModS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
        if (offset == 0) {
            func_ov182_021b4060();
        } else if (offset < 16) {
            sAddressSettings.work->scrollTask = func_ov182_021bd200(0, func_ov182_021b3eac, NULL, 0x78);
        } else {
            sAddressSettings.work->scrollTask = func_ov182_021bd200(0, func_ov182_021b3f24, NULL, 0x78);
        }
        break;
    case 4:
        if (sAddressSettings.scroll == 0) {
            if (!sAddressSettings.work->barAtEnd) {
                func_ov182_021b80f4(9);
                sAddressSettings.work->barAtEnd = TRUE;
            }
        } else {
            func_ov182_021b80f4(0x13);
            sAddressSettings.work->scrollTask = func_ov182_021bd200(0, func_ov182_021b3eac, NULL, 0x78);
        }
        break;
    case 6:
        if (sAddressSettings.scroll == ADDRESS_SCROLL_MAX) {
            if (!sAddressSettings.work->barAtEnd) {
                func_ov182_021b80f4(9);
                sAddressSettings.work->barAtEnd = TRUE;
            }
        } else {
            func_ov182_021b80f4(0x13);
            sAddressSettings.work->scrollTask = func_ov182_021bd200(0, func_ov182_021b3f24, NULL, 0x78);
        }
        break;
    case 5:
    case 7:
        sAddressSettings.work->barAtEnd = FALSE;
        break;
    }
}

static void func_ov182_021b34f8(void) {
    func_ov182_021b7174();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021b3510);
}

static void func_ov182_021b3510(void) {
    if (func_ov182_021bbaac(1) == 0) {
        func_ov182_021bbac4(3, 1, 1, 8);
        func_ov182_021bbac4(3, 0, 0x1d, 8);
        func_ov182_021adfbc(func_ov182_021b3544);
    }
}

// Frees the screen and goes on to what was chosen
static void func_ov182_021b3544(void) {
    int i;
    DWCUtilSettingsEdit *edit;
    void (*next)(void);

    if (func_ov182_021bbaac(1) != 0 || func_ov182_021bbaac(0) != 0) {
        return;
    }
    func_ov182_021bd280(0, sAddressSettings.work->task);
    func_ov182_021bd280(1, sAddressSettings.work->vblankTask);
    for (i = 0; i < 7; i++) {
        func_ov182_021bcc9c(sAddressSettings.work->marks[i]);
    }
    if (sAddressSettings.work->setupIcon != NULL) {
        func_ov182_021bcc9c(sAddressSettings.work->setupIcon);
    }
    func_ov182_021b711c();
    func_ov182_021bbf7c(0);
    func_ov182_021ab8e4();
    if (sAddressSettings.work->chosen != ADDRESS_ITEM_SAVE) {
        DWCUtilScreen_Stop();
    }
    func_ov182_021b6f78();
    func_ov182_021bb420(sAddressSettings.work->backScreen);
    for (i = 0; i < 2; i++) {
        func_ov182_021bb420(sAddressSettings.work->palettes[i]);
    }
    func_ov182_021ab67c("char/ybObjMain.ncl.l", GX_LoadOBJPltt);
    func_ov182_021bc228(1, 1);
    func_ov182_021bc228(0, 0x1d);
    G2_SetBG0Offset(0, 0);
    G2_SetBG2Offset(0, 0);
    G2_SetBG2ControlText(GX_BG_SCRSIZE_TEXT_256x256, GX_BG_COLORMODE_16, GX_BG_SCRBASE_0x7000, GX_BG_CHARBASE_0x10000);
    switch (sAddressSettings.work->chosen) {
    case ADDRESS_ITEM_SSID:
    case ADDRESS_ITEM_WEP_KEY:
        func_ov182_021adff0(sAddressSettings.work->chosen, 0);
        func_ov182_021adfc8(2, 0);
        next = func_ov182_021b19f4;
        break;
    case ADDRESS_ITEM_IP:
    case ADDRESS_ITEM_NETMASK:
    case ADDRESS_ITEM_GATEWAY:
    case ADDRESS_ITEM_DNS1:
    case ADDRESS_ITEM_DNS2:
        i = sAddressSettings.work->chosen - ADDRESS_ITEM_IP;
        if (sAddressSettings.work->chosen >= ADDRESS_ITEM_DNS1) {
            i -= 2;
        }
        func_ov182_021adfc8(2, 0);
        func_ov182_021adff0(i, 0);
        next = func_ov182_021b2120;
        break;
    case ADDRESS_ITEM_TEST:
        edit = func_ov182_021b781c();
        edit->connection.netmaskBits = DWCBm_CountMaskBits(edit->netmask);
        if (edit->autoAddress) {
            MI_CpuFill8(edit->connection.ip, 0, sizeof(edit->connection.ip));
            MI_CpuFill8(edit->connection.gateway, 0, sizeof(edit->connection.gateway));
            MI_CpuFill8(edit->netmask, 0, sizeof(edit->netmask));
            edit->connection.netmaskBits = 0;
        }
        if (edit->autoDns) {
            MI_CpuFill8(edit->connection.dns, 0, sizeof(edit->connection.dns));
        }
        func_ov182_021adfc8(2, 0);
        func_ov182_021adff0(0, 0);
        next = func_ov182_021b6104;
        break;
    case ADDRESS_ITEM_SAVE:
        func_ov182_021adfc8(0, 0);
        next = func_ov182_021b4404;
        break;
    case ADDRESS_ITEM_BACK:
        if (sAddressSettings.from == 0) {
            func_ov182_021adfc8(2, 1);
            next = func_ov182_021b536c;
        } else {
            edit = func_ov182_021b781c();
            func_ov182_021b7b20(edit->index);
            func_ov182_021adfc8(0, 1);
            next = func_ov182_021b5bb8;
        }
        break;
    default:
        goto end;
    }
    func_ov182_021adfbc(next);
end:
    func_ov182_021bc4f0(&sAddressSettings.work);
}

// Draws the rows on the screen from the scroll
static void func_ov182_021b3740(void) {
    int i;
    int top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    int index;

    func_ov182_021bc118(sAddressSettings.work->text, 0);
    for (i = 0, index = top; i < 5; i++, index++) {
        func_ov182_021b37b4(index, i);
    }
    func_ov182_021b6fac(sAddressSettings.work->backScreen + sAddressScreenOffsets[top], 0, 0x1e, 0x13);
    for (i = 0; i < 5; i++, top++) {
        func_ov182_021b38c0(top, i);
    }
    func_ov182_021b6f98();
    func_ov182_021bc184(sAddressSettings.work->text);
    func_ov182_021b3bc4();
}

// The text of a row: the SSID, the WEP key as stars, or an address of the connection's own
static void func_ov182_021b37b4(int index, int row) {
    DWCUtilSettingsEdit *edit = func_ov182_021b781c();
    u8 stars[33];
    int length;

    switch (index) {
    case ADDRESS_ROW_SSID:
        func_ov182_021b3a9c(edit->connection.ssid, row);
        return;
    case ADDRESS_ROW_WEP_KEY:
        switch (edit->connection.wepMode) {
        case 0:
            return;
        case 1:
            length = 10;
            break;
        case 2:
            length = 26;
            break;
        case 3:
            length = 32;
        }
        if (edit->connection.wepKeyText == 1) {
            length /= 2;
        }
        MI_CpuFill8(stars, 0, sizeof(stars));
        memset(stars, '*', length);
        func_ov182_021b3a9c(stars, row);
        return;
    case ADDRESS_ROW_IP:
        if (!edit->autoAddress) {
            func_ov182_021b3b6c(edit->connection.ip, row);
        }
        break;
    case ADDRESS_ROW_NETMASK:
        if (!edit->autoAddress) {
            func_ov182_021b3b6c(edit->netmask, row);
        }
        break;
    case ADDRESS_ROW_GATEWAY:
        if (!edit->autoAddress) {
            func_ov182_021b3b6c(edit->connection.gateway, row);
        }
        break;
    case ADDRESS_ROW_DNS1:
        if (!edit->autoDns) {
            func_ov182_021b3b6c(edit->connection.dns[0], row);
        }
        break;
    case ADDRESS_ROW_DNS2:
        if (!edit->autoDns) {
            func_ov182_021b3b6c(edit->connection.dns[1], row);
        }
    }
}

// The marks of a row: its arrow, or its two answers lit as they were touched and greyed when they can't be chosen
static void func_ov182_021b38c0(int index, int row) {
    DWCUtilSettingsEdit *edit = func_ov182_021b781c();
    int mark;
    int frame;
    int other;
    BOOL lit;

    switch (index) {
    case ADDRESS_ROW_SSID:
    case ADDRESS_ROW_WEP_KEY:
        mark = 0;
        frame = 0;
        if (func_ov182_021b3998(ADDRESS_ITEM_SSID)) {
            break;
        }
        frame = 2;
        break;
    case ADDRESS_ROW_AUTO_ADDRESS:
        lit = FALSE;
        frame = 0;
        if (edit->autoAddress) {
            mark = 1;
            other = 4;
        } else {
            mark = 2;
            other = 3;
        }
        if (sAddressSettings.work->lit[0] != 0) {
            frame = 1;
        }
        if (sAddressSettings.work->lit[1] != 0) {
            lit = TRUE;
        }
        func_ov182_021b39fc(other, lit, row);
        break;
    case ADDRESS_ROW_IP:
    case ADDRESS_ROW_NETMASK:
    case ADDRESS_ROW_GATEWAY:
        mark = 0;
        if (edit->autoAddress) {
            frame = 2;
        } else {
            frame = mark;
        }
        break;
    case ADDRESS_ROW_AUTO_DNS:
        lit = FALSE;
        frame = 0;
        if (edit->autoDns) {
            mark = 1;
            other = 4;
        } else {
            if (!edit->autoAddress) {
                frame = 2;
            }
            mark = 2;
            other = 3;
        }
        if (sAddressSettings.work->lit[2] != 0) {
            frame = 1;
        }
        if (sAddressSettings.work->lit[3] != 0) {
            lit = TRUE;
        }
        func_ov182_021b39fc(other, lit, row);
        break;
    case ADDRESS_ROW_DNS1:
    case ADDRESS_ROW_DNS2:
        mark = 0;
        if (edit->autoDns) {
            frame = 2;
        } else {
            frame = mark;
        }
        break;
    default:
        mark = 0;
        frame = 2;
        break;
    }
    func_ov182_021b39fc(mark, frame, row);
}

// Whether an item can be chosen: not the SSID or the key of a connection that a setup made, nor what DHCP gives
static BOOL func_ov182_021b3998(int item) {
    DWCUtilSettingsEdit *edit = func_ov182_021b781c();
    BOOL result = TRUE;

    switch (item) {
    case ADDRESS_ITEM_AUTO_DNS_YES:
        if (!edit->autoAddress) {
            result = FALSE;
        }
        break;
    case ADDRESS_ITEM_SSID:
    case ADDRESS_ITEM_WEP_KEY:
        if (edit->connection.status == 1 || edit->connection.status == 2) {
            result = FALSE;
        }
        break;
    case ADDRESS_ITEM_IP:
    case ADDRESS_ITEM_NETMASK:
    case ADDRESS_ITEM_GATEWAY:
        if (edit->autoAddress) {
            result = FALSE;
        }
        break;
    case ADDRESS_ITEM_DNS1:
    case ADDRESS_ITEM_DNS2:
        if (edit->autoDns) {
            result = FALSE;
        }
        break;
    }
    return result;
}

// Shows a mark of a row in a frame, the marks of the answers of the two rows of DHCP in their own entries
static void func_ov182_021b39fc(int mark, int frame, int row) {
    u16 x[5];
    GXOamAttr **oam;

    x[0] = 0xcc;
    x[1] = 0xc0;
    x[2] = 0xc0;
    x[3] = 0x8f;
    x[4] = 0x8f;
    oam = &sAddressSettings.work->marks[row];
    if (mark == 1 || mark == 2) {
        if (func_ov182_021b3e94(row) == ADDRESS_ROW_AUTO_ADDRESS) {
            oam = &sAddressSettings.work->marks[5];
        } else {
            oam = &sAddressSettings.work->marks[6];
        }
    }
    if (sAddressMarkCells[mark][frame] != 0) {
        func_ov182_021bb914(0, sAddressMarkCells[mark][frame], *oam);
        DWCUtilOam_SetPosition(*oam, x[mark], 0);
        G2_SetOBJPriority(*oam, 3);
        return;
    }
    DWCUtilOam_SetPosition(*oam, 0x100, 0);
}

// The SSID or the key of a row, on two lines when it is longer than 16 characters, a space shown as its own character
static void func_ov182_021b3a9c(const u8 *str, int row) {
    u16 line[17];
    int length;
    int i;
    int count;
    int y;

    MI_CpuFill8(line, 0, sizeof(line));
    length = func_ov182_021bcef8(str, 32);
    count = length;
    if (count > 16) {
        count = 16;
    }
    for (i = 0; i < count; i++) {
        if (str[i] == ' ') {
            line[i] = 0xe01d;
        } else {
            line[i] = str[i];
        }
    }
    y = row * ADDRESS_ROW_HEIGHT + 2;
    if (length <= 16) {
        y += 5;
    }
    func_ov182_021bc030(sAddressSettings.work->text, 0x48, y, 2, 8, line, 1);
    if (length > 16) {
        MI_CpuFill8(line, 0, sizeof(line));
        for (i = 0; i < length - 16; i++) {
            if (str[16 + i] == ' ') {
                line[i] = 0xe01d;
            } else {
                line[i] = str[16 + i];
            }
        }
        func_ov182_021bc030(sAddressSettings.work->text, 0x48, y + 12, 2, 8, line, 1);
    }
}

// An address of a row, as four numbers of three places
static void func_ov182_021b3b6c(const u8 *address, int row) {
    u16 str[16];
    char digits[16];

    OS_SNPrintf(digits, sizeof(digits), "%3d.%3d.%3d.%3d", address[0], address[1], address[2], address[3]);
    func_ov182_021bcf10(str, (u8 *)digits, 16);
    func_ov182_021bc030(sAddressSettings.work->text, 0x5f, row * ADDRESS_ROW_HEIGHT + 8, 2, 7, str, 1);
}

// Moves the marks and the setup's icon to the rows' places from the scroll, and has the BGs follow at the next VBlank
static void func_ov182_021b3bc4(void) {
    int top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    int y = 0x34 - FX_ModS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    int i;
    int x;
    int oamY;
    int markY;
    GXOamAttr *oam;

    if (sAddressSettings.work->setupIcon != NULL) {
        if (top == 0) {
            x = 0x26;
        } else {
            x = 0x100;
        }
        DWCUtilOam_SetPosition(sAddressSettings.work->setupIcon, x, y);
    }
    markY = y;
    for (i = 0; i < 5; i++) {
        oam = sAddressSettings.work->marks[i];
        DWCUtilOam_GetPosition(oam, &x, &oamY);
        DWCUtilOam_SetPosition(oam, x, markY);
        markY += ADDRESS_ROW_HEIGHT;
    }
    if (top <= ADDRESS_ROW_AUTO_ADDRESS) {
        oam = sAddressSettings.work->marks[5];
        DWCUtilOam_GetPosition(oam, &x, &oamY);
        DWCUtilOam_SetPosition(oam, x, y + (ADDRESS_ROW_AUTO_ADDRESS - top) * ADDRESS_ROW_HEIGHT);
    } else {
        DWCUtilOam_SetPosition(sAddressSettings.work->marks[5], 0x100, 0);
    }
    if (top >= ADDRESS_ROW_AUTO_ADDRESS && top <= ADDRESS_ROW_AUTO_DNS) {
        oam = sAddressSettings.work->marks[6];
        DWCUtilOam_GetPosition(oam, &x, &oamY);
        DWCUtilOam_SetPosition(oam, x, y + (ADDRESS_ROW_AUTO_DNS - top) * ADDRESS_ROW_HEIGHT);
    } else {
        DWCUtilOam_SetPosition(sAddressSettings.work->marks[6], 0x100, 0);
    }
    sAddressSettings.work->moved = TRUE;
}

// What a touch is on, ADDRESS_ITEM_NONE for nothing
static int func_ov182_021b3ce0(void) {
    DWCUtilAddressRect rect;
    int top;
    int i;

    if (!func_ov182_021bc888(data_ov182_021c059c)) {
        return ADDRESS_ITEM_NONE;
    }
    top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    rect = sAddressRowRects[0];
    for (i = 0; i < 4; i++) {
        if (top != ADDRESS_ROW_AUTO_ADDRESS && top != ADDRESS_ROW_AUTO_DNS && func_ov182_021bc9a8(rect.coords)) {
            return sAddressRowItems[top];
        }
        top++;
        rect.coords[1] += ADDRESS_ROW_HEIGHT;
    }
    top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    for (i = 0; i < 4; i++) {
        if (top == ADDRESS_ROW_AUTO_ADDRESS) {
            rect = sAddressRowRects[2];
            rect.coords[1] += i * ADDRESS_ROW_HEIGHT;
            if (func_ov182_021bc9a8(rect.coords)) {
                return ADDRESS_ITEM_AUTO_ADDRESS_YES;
            }
            rect = sAddressRowRects[1];
            rect.coords[1] += i * ADDRESS_ROW_HEIGHT;
            if (func_ov182_021bc9a8(rect.coords)) {
                return ADDRESS_ITEM_AUTO_ADDRESS_NO;
            }
            break;
        }
        top++;
    }
    top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    for (i = 0; i < 4; i++) {
        if (top == ADDRESS_ROW_AUTO_DNS) {
            rect = sAddressRowRects[2];
            rect.coords[1] += i * ADDRESS_ROW_HEIGHT;
            if (func_ov182_021bc9a8(rect.coords)) {
                return ADDRESS_ITEM_AUTO_DNS_YES;
            }
            rect = sAddressRowRects[1];
            rect.coords[1] += i * ADDRESS_ROW_HEIGHT;
            if (func_ov182_021bc9a8(rect.coords)) {
                return ADDRESS_ITEM_AUTO_DNS_NO;
            }
            break;
        }
        top++;
    }
    for (i = 0; i < 3; i++) {
        if (func_ov182_021bc888(sAddressButtonRects[i].coords)) {
            return i + ADDRESS_ITEM_TEST;
        }
    }
    return ADDRESS_ITEM_NONE;
}

// The row on the screen that a row of the list is on, -1 when it isn't shown
static int func_ov182_021b3e68(int index) {
    int top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    int i;

    for (i = 0; i < 4; i++, top++) {
        if (top == index) {
            return i;
        }
    }
    return -1;
}

// The row of the list that a row on the screen shows
static int func_ov182_021b3e94(int row) {
    int top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);

    return top + row;
}

// Scrolls the list up by 6 pixels a frame, until it rests on a row
static void func_ov182_021b3eac(DWCUtilTask *task, void *arg) {
    int offset;

    func_ov182_021b7174();
    func_ov182_021ab8e4();
    if (sAddressSettings.scroll > 6) {
        sAddressSettings.scroll -= 6;
    } else {
        sAddressSettings.scroll = 0;
    }
    offset = FX_ModS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    if (offset == ADDRESS_ROW_HEIGHT - 6) {
        func_ov182_021b3740();
        return;
    }
    if (offset > ADDRESS_ROW_HEIGHT - 6) {
        sAddressSettings.scroll += ADDRESS_ROW_HEIGHT - offset;
        offset = 0;
    }
    func_ov182_021b3bc4();
    if (offset == 0) {
        func_ov182_021b715c(sAddressSettings.scroll * ADDRESS_BAR_LENGTH / ADDRESS_SCROLL_MAX);
        func_ov182_021b7164();
        func_ov182_021b4060();
        sAddressSettings.work->scrollTask = NULL;
        func_ov182_021bd268(0, task);
    }
}

// Scrolls the list down by 6 pixels a frame, until it rests on a row
static void func_ov182_021b3f24(DWCUtilTask *task, void *arg) {
    int offset;

    func_ov182_021b7174();
    func_ov182_021ab8e4();
    sAddressSettings.scroll += 6;
    offset = FX_ModS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    if (offset >= 6) {
        func_ov182_021b3bc4();
        return;
    }
    sAddressSettings.scroll -= offset;
    func_ov182_021b3740();
    func_ov182_021b715c(sAddressSettings.scroll * ADDRESS_BAR_LENGTH / ADDRESS_SCROLL_MAX);
    func_ov182_021b7164();
    func_ov182_021b4060();
    sAddressSettings.work->scrollTask = NULL;
    func_ov182_021bd268(0, task);
}

static void func_ov182_021b3f84(DWCUtilTask *task, void *arg) {
    int top;
    int offset;

    if (sAddressSettings.work->moved) {
        top = FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
        offset = FX_ModS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
        G2_SetBG0Offset(0, offset - 0x33);
        G2_SetBG2Offset(0, offset - 0x33 + sAddressLabelOffsets[top]);
        sAddressSettings.work->moved = FALSE;
    }
}

// Frames the item the cursor is on
static void func_ov182_021b3fd8(void) {
    u8 rects[14] = { 0, 0, 1, 2, 0, 0, 0, 1, 2, 0, 0, 3, 4, 5 };
    DWCUtilAddressRect rect;
    int index = rects[sAddressSettings.cursor];

    if (index >= 3) {
        func_ov182_021ab7f0(3, sAddressCursorRects[index].coords[0], sAddressCursorRects[index].coords[2],
                            sAddressCursorRects[index].coords[1]);
        return;
    }
    rect = sAddressCursorRects[index];
    rect.coords[1] += sAddressSettings.row * ADDRESS_ROW_HEIGHT;
    func_ov182_021ab7f0(1, rect.coords[0], rect.coords[2], rect.coords[1]);
}

// Puts the cursor on the item of its row, the chosen answer of a row of DHCP
static void func_ov182_021b4060(void) {
    DWCUtilSettingsEdit *edit;
    int index;

    if (sAddressSettings.cursor == ADDRESS_ITEM_TEST || sAddressSettings.cursor == ADDRESS_ITEM_SAVE ||
        sAddressSettings.cursor == ADDRESS_ITEM_BACK) {
        func_ov182_021b3fd8();
        return;
    }
    edit = func_ov182_021b781c();
    index = sAddressSettings.row + FX_DivS32(sAddressSettings.scroll, ADDRESS_ROW_HEIGHT);
    switch (index) {
    case ADDRESS_ROW_AUTO_ADDRESS:
        if (edit->autoAddress) {
            sAddressSettings.cursor = ADDRESS_ITEM_AUTO_ADDRESS_YES;
        } else {
            sAddressSettings.cursor = ADDRESS_ITEM_AUTO_ADDRESS_NO;
        }
        break;
    case ADDRESS_ROW_AUTO_DNS:
        if (edit->autoDns) {
            sAddressSettings.cursor = ADDRESS_ITEM_AUTO_DNS_YES;
        } else {
            sAddressSettings.cursor = ADDRESS_ITEM_AUTO_DNS_NO;
        }
        break;
    default:
        sAddressSettings.cursor = sAddressRowItems[index];
        break;
    }
    func_ov182_021b3fd8();
}

static void func_ov182_021b40c8(int item) {
    sAddressSettings.cursor = item;
    sAddressSettings.row = func_ov182_021b3e68(sAddressItemRows[item]);
    func_ov182_021b3fd8();
}

// Moves the cursor: right (0), up (1), left (2) or down (3), scrolling the list at its ends
static void func_ov182_021b40e8(int direction) {
    DWCUtilSettingsEdit *edit = func_ov182_021b781c();
    u8 cursor = sAddressSettings.cursor;
    int moved = 0;

    if (sAddressSettings.cursor == ADDRESS_ITEM_AUTO_DNS_NO && !edit->autoAddress) {
        if (direction == 0) {
            return;
        }
        if (direction == 2) {
            return;
        }
    }
    switch (sAddressSettings.cursor) {
    case ADDRESS_ITEM_SSID:
        if (direction == 1) {
            sAddressSettings.cursor = ADDRESS_ITEM_TEST;
        } else if (direction == 3) {
            sAddressSettings.row++;
        } else {
            moved = 2;
        }
        break;
    case ADDRESS_ITEM_DNS2:
        if (direction == 1) {
            sAddressSettings.row--;
        } else if (direction == 3) {
            sAddressSettings.cursor = sAddressSettings.work->lastButton;
        } else {
            moved = 2;
        }
        break;
    case ADDRESS_ITEM_TEST:
        if (direction == 1) {
            if (sAddressSettings.work->atEnd) {
                return;
            }
            func_ov182_021b80f4(9);
            sAddressSettings.work->atEnd = TRUE;
            return;
        }
        if (direction == 3) {
            sAddressSettings.cursor = ADDRESS_ITEM_SSID;
            sAddressSettings.row = 0;
            sAddressSettings.scroll = 0;
            func_ov182_021b3740();
            func_ov182_021b715c(0);
        } else {
            moved = 2;
        }
        break;
    case ADDRESS_ITEM_SAVE:
    case ADDRESS_ITEM_BACK:
        sAddressSettings.work->lastButton = sAddressSettings.cursor;
        if (direction == 1) {
            sAddressSettings.cursor = ADDRESS_ITEM_DNS2;
            sAddressSettings.row = 3;
            sAddressSettings.scroll = ADDRESS_SCROLL_MAX;
            func_ov182_021b3740();
            func_ov182_021b715c(ADDRESS_BAR_LENGTH);
        } else if (direction == 3) {
            if (sAddressSettings.work->atEnd) {
                return;
            }
            func_ov182_021b80f4(9);
            sAddressSettings.work->atEnd = TRUE;
            return;
        } else if (cursor == ADDRESS_ITEM_SAVE) {
            sAddressSettings.cursor = ADDRESS_ITEM_BACK;
        } else {
            sAddressSettings.cursor = ADDRESS_ITEM_SAVE;
        }
        break;
    default:
        if (direction == 1) {
            if (sAddressSettings.row != 0) {
                sAddressSettings.row--;
            } else {
                func_ov182_021b80f4(0x13);
                sAddressSettings.work->scrollTask = func_ov182_021bd200(0, func_ov182_021b3eac, NULL, 0x78);
                return;
            }
        } else if (direction == 3) {
            if (sAddressSettings.row < 3) {
                sAddressSettings.row++;
            } else {
                func_ov182_021b80f4(0x13);
                sAddressSettings.work->scrollTask = func_ov182_021bd200(0, func_ov182_021b3f24, NULL, 0x78);
                return;
            }
        } else {
            moved = 2;
            if (sAddressSettings.cursor == ADDRESS_ITEM_AUTO_ADDRESS_YES) {
                sAddressSettings.cursor = ADDRESS_ITEM_AUTO_ADDRESS_NO;
            } else if (sAddressSettings.cursor == ADDRESS_ITEM_AUTO_ADDRESS_NO) {
                sAddressSettings.cursor = ADDRESS_ITEM_AUTO_ADDRESS_YES;
            } else if (sAddressSettings.cursor == ADDRESS_ITEM_AUTO_DNS_YES) {
                sAddressSettings.cursor = ADDRESS_ITEM_AUTO_DNS_NO;
            } else if (sAddressSettings.cursor == ADDRESS_ITEM_AUTO_DNS_NO) {
                sAddressSettings.cursor = ADDRESS_ITEM_AUTO_DNS_YES;
            } else {
                break;
            }
            func_ov182_021b80f4(8);
            func_ov182_021b3fd8();
        }
        break;
    }
    if (moved != 2) {
        func_ov182_021b80f4(8);
        if (moved == 0) {
            func_ov182_021b4060();
        }
    }
}

// Lights up the chosen button: its row of the palette
static void func_ov182_021b4278(void) {
    func_ov182_021b7014(sAddressSettings.work->palettes[1], sAddressButtonPalettes[sAddressSettings.cursor - 0xb],
                        sAddressButtonPalettes[sAddressSettings.cursor - 0xb]);
}

// Counts down the frames that the answers stay lit
static void func_ov182_021b4298(DWCUtilTask *task, void *arg) {
    int i;

    for (i = 0; i < 4; i++) {
        if (sAddressSettings.work->lit[i] != 0) {
            sAddressSettings.work->lit[i]--;
            if (sAddressSettings.work->lit[i] == 0) {
                func_ov182_021b3740();
            }
        }
    }
}

// Lights the touched answer for 20 frames, and the other answer of its row no longer
static void func_ov182_021b42c8(void) {
    u8 items[4] = { ADDRESS_ITEM_AUTO_ADDRESS_YES, ADDRESS_ITEM_AUTO_ADDRESS_NO, ADDRESS_ITEM_AUTO_DNS_YES,
                    ADDRESS_ITEM_AUTO_DNS_NO };
    int i;

    for (i = 0; i < 4; i++) {
        if (sAddressSettings.cursor == items[i]) {
            sAddressSettings.work->lit[i] = 20;
            if (i & 1) {
                sAddressSettings.work->lit[i - 1] = 0;
                return;
            }
            sAddressSettings.work->lit[i + 1] = 0;
            return;
        }
    }
}

// Whether the settings can be saved: an SSID, a usable DNS server unless DHCP gives them, and a usable address and
// gateway that suit the mask unless DHCP gives them
static BOOL func_ov182_021b4334(void) {
    DWCUtilSettingsEdit *edit = func_ov182_021b781c();

    if (edit->connection.ssid[0] == 0) {
        return FALSE;
    }
    if (!edit->autoDns && !DWCBm_IsUsableAddress(edit->connection.dns[0]) &&
        !DWCBm_IsUsableAddress(edit->connection.dns[1])) {
        return FALSE;
    }
    if (!edit->autoAddress) {
        if (!DWCBm_IsUsableAddress(edit->connection.ip)) {
            return FALSE;
        }
        if (!DWCBm_IsUsableAddress(edit->connection.gateway)) {
            return FALSE;
        }
        if (!DWCBm_HasHostBits(edit->connection.ip, edit->netmask)) {
            return FALSE;
        }
    }
    return TRUE;
}

static void func_ov182_021b43b0(void) {
    if (func_ov182_021b8f14() == 0) {
        func_ov182_021b80f4(6);
        func_ov182_021b8eec();
        func_ov182_021b7060(sAddressSettings.work->palettes[0]);
        func_ov182_021adfbc(func_ov182_021b43e0);
    }
}

static void func_ov182_021b43e0(void) {
    if (!func_ov182_021b8f24()) {
        func_ov182_021adfbc(func_ov182_021b3030);
    }
}

void func_ov182_021b43f8(int from) {
    sAddressSettings.from = from;
}
