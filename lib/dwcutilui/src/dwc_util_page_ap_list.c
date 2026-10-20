#include "dwc_util_page_ap_list.h"
#include "dwc_util_ap_search.h"
#include "dwc_util_auto_connect_intro.h"
#include "dwc_util_bg_buffer.h"
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
#include "dwc_util_screen.h"
#include "dwc_util_screen_way.h"
#include "dwc_util_shared.h"
#include "dwc_util_string.h"
#include "dwc_util_task.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "dwc_util_ui_object.h"
#include "dwc_util_wep_keyboard.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/mi.h"

// The rows of the list on the screen, the height of a row, and the cursor's place on the button above the list
#define AP_LIST_ROWS 4
#define AP_LIST_ROW_HEIGHT 28
#define AP_LIST_BUTTON 4

// The list of the access points that the scan found
typedef struct DWCUtilApList {
    DWCUtilApEntry *entries;
    // The screen behind the rows, and the palette whose row lights up the button
    void *backScreen;
    void *palette;
    DWCUtilText *text;
    // The icons of each row on the screen: the access point's security and its signal
    GXOamAttr *securityOams[AP_LIST_ROWS + 1];
    GXOamAttr *signalOams[AP_LIST_ROWS + 1];
    // The task that scrolls the list by a row, and the VBlank task that scrolls the BGs
    DWCUtilTask *scrollTask;
    DWCUtilTask *vblankTask;
    u16 scrollMax;
    u16 securityChars[3];
    u16 signalChars[4];
    s8 touched;
    u8 count;
    u8 chosen;
    // The scroll bar's travel
    u8 barLength;
    u8 confirmed;
    u8 dragDelay;
    u8 moved;
    u8 dragging;
    // Whether the end of the list has played its sound, for the scroll bar's arrows and for the buttons
    u8 barAtEnd;
    u8 atEnd;
} DWCUtilApList;

// The cursor and the scroll of the list, in pixels, which stay for the next visit
typedef struct DWCUtilApListPos {
    u8 cursor;
    u16 scroll;
} DWCUtilApListPos;

static void func_ov182_021b0cb4(void);
static void func_ov182_021b0d74(void);
static void func_ov182_021b0de4(void);
static void func_ov182_021b0ebc(void);
static void func_ov182_021b0ef4(void);
static void func_ov182_021b0f20(void);
static void func_ov182_021b0f40(void);
static void func_ov182_021b0f50(void);
static void func_ov182_021b10c8(void);
static void func_ov182_021b1254(void);
static void func_ov182_021b1308(void);
static void func_ov182_021b1320(void);
static void func_ov182_021b136c(void);
static void func_ov182_021b14c0(void);
static void func_ov182_021b1528(int index, int row);
static void func_ov182_021b15fc(int index, int row);
static void func_ov182_021b1658(void);
static void func_ov182_021b16d4(DWCUtilTask *task, void *arg);
static void func_ov182_021b1754(DWCUtilTask *task, void *arg);
static void func_ov182_021b17bc(DWCUtilTask *task, void *arg);
static void func_ov182_021b17f8(void);
static void func_ov182_021b182c(int direction);
static void func_ov182_021b18f8(void);
static void func_ov182_021b193c(void);
static void func_ov182_021b1994(void);
static void func_ov182_021b19b4(void);
static void func_ov182_021b19d4(void);

// The cells of the security icons, by the access point's security, and of the signal icons, by its link level
static const u8 sApListSecurityCells[3] = { 0x2e, 0x2d, 0x33 };
static const u8 sApListSignalCells[4] = { 0x18, 0x17, 0x16, 0x15 };
// Where the rows and the button are touched, and where the cursor frames them: left, top, right and bottom
static const u16 sApListRowRects[AP_LIST_ROWS + 1][4] = {
    { 7, 0x32, 0xd0, 0x4c }, { 7, 0x4e, 0xd0, 0x68 },    { 7, 0x6a, 0xd0, 0x84 },
    { 7, 0x86, 0xd0, 0xa0 }, { 0x85, 0x1b, 0xfd, 0x2c },
};
static const u16 sApListCursorRects[AP_LIST_ROWS + 1][4] = {
    { 4, 0x2e, 0xdb, 0x3f }, { 4, 0x4a, 0xdb, 0x5b },    { 4, 0x66, 0xdb, 0x77 },
    { 4, 0x82, 0xdb, 0x93 }, { 0x82, 0x18, 0xf0, 0x2c },
};

static DWCUtilApListPos sApListPos;
static DWCUtilApList *sApList;

void func_ov182_021b0c28(void) {
    int again;

    sApList = func_ov182_021bc4d8(sizeof(DWCUtilApList), 4);
    func_ov182_021adffc(&again, NULL);
    if (again == 0) {
        sApListPos.cursor = 0;
        sApListPos.scroll = 0;
    }
    sApList->count = func_ov182_021b7630(&sApList->entries);
    func_ov182_021b0cb4();
    func_ov182_021ab9ac();
    DWCUtilScreen_Start(0x34, -1, 0);
    func_ov182_021ab6a4(2);
    func_ov182_021b0d74();
    func_ov182_021b0de4();
    sApList->text = func_ov182_021bbdec(0, 0);
    sApList->vblankTask = func_ov182_021bd200(1, func_ov182_021b17bc, NULL, 0x6e);
    func_ov182_021b14c0();
    func_ov182_021b17f8();
    func_ov182_021adfbc(func_ov182_021b0ebc);
}

static void func_ov182_021b0cb4(void) {
    char backPath[] = "char/xb4ApListBack.nsc.l";
    char palettePath[] = "char/ybBgStep31.ncl.l";

    func_ov182_021ab67c("char/jb4ApList.nsc.l", GX_LoadBG3Scr);
    sApList->backScreen = func_ov182_021bb37c(func_ov182_021ab63c(backPath), NULL, 4);
    func_ov182_021b6f38(sApList->backScreen);
    func_ov182_021b6f98();
    sApList->palette = func_ov182_021bb37c(func_ov182_021ab63c(palettePath), NULL, 4);
    G2S_SetBG0Priority(3);
    G2S_SetBG1Priority(3);
    G2_SetBG0Priority(3);
    G2_SetBG1Priority(2);
    G2_SetBG2Priority(3);
    G2_SetBG3Priority(2);
}

// The scroll bar, with a handle for the lists of up to 8 and of more access points
static void func_ov182_021b0d74(void) {
    int pos = 0;
    int kind;

    sApList->scrollMax = (sApList->count - AP_LIST_ROWS) * AP_LIST_ROW_HEIGHT;
    if (sApList->count <= AP_LIST_ROWS) {
        kind = 0;
        sApList->barLength = 0;
    } else if (sApList->count <= AP_LIST_ROWS * 2) {
        kind = 1;
        sApList->barLength = 0x1f;
    } else {
        kind = 2;
        sApList->barLength = 0x37;
    }
    if (kind != 0) {
        pos = FX_DivS32(sApListPos.scroll * sApList->barLength, sApList->scrollMax);
    }
    func_ov182_021b70a0(kind, 0x55, 0xec, 0x3f, pos);
}

static void func_ov182_021b0de4(void) {
    int rows = sApList->count;
    int i;
    u32 kind;
    u32 level;

    if (rows > AP_LIST_ROWS + 1) {
        rows = AP_LIST_ROWS + 1;
    }
    for (i = 0; i < rows; i++) {
        sApList->securityOams[i] = func_ov182_021bb960(0, 0x2e);
        sApList->signalOams[i] = func_ov182_021bb960(0, 0x18);
    }
    for (kind = 0; kind < NELEMS(sApListSecurityCells); kind++) {
        func_ov182_021bb914(0, sApListSecurityCells[kind], sApList->securityOams[0]);
        sApList->securityChars[kind] = sApList->securityOams[0]->attr2 & GX_OAM_ATTR2_NAME_MASK;
    }
    for (level = 0; level < NELEMS(sApListSignalCells); level++) {
        func_ov182_021bb914(0, sApListSignalCells[level], sApList->signalOams[0]);
        sApList->signalChars[level] = sApList->signalOams[0]->attr2 & GX_OAM_ATTR2_NAME_MASK;
    }
    for (i = 0; i < rows; i++) {
        G2_SetOBJPriority(sApList->securityOams[i], 3);
        G2_SetOBJPriority(sApList->signalOams[i], 3);
    }
}

static void func_ov182_021b0ebc(void) {
    func_ov182_021bbac4(2, 1, 1, 8);
    func_ov182_021bbac4(2, 0, 0x1d, 8);
    func_ov182_021bc1dc(1, 1);
    func_ov182_021bc1dc(0, 0x1d);
    func_ov182_021adfbc(func_ov182_021b0ef4);
}

static void func_ov182_021b0ef4(void) {
    if (func_ov182_021bbaac(1) == 0 && func_ov182_021bbaac(0) == 0) {
        func_ov182_021ab6d0(0);
        func_ov182_021adfbc(func_ov182_021b0f20);
    }
}

static void func_ov182_021b0f20(void) {
    if (DWCUtilUiObject_GetState() != -2) {
        DWCUtilUiObject_Enable();
        func_ov182_021adfbc(func_ov182_021b0f40);
    }
}

static void func_ov182_021b0f40(void) {
    func_ov182_021b0f50();
    func_ov182_021b10c8();
    func_ov182_021b1254();
}

static void func_ov182_021b0f50(void) {
    int i;

    if (sApList->scrollTask != NULL || sApList->dragging) {
        return;
    }
    if (func_ov182_021bc888(data_ov182_021c059c)) {
        sApList->touched = -1;
        for (i = 0; i < NELEMS(sApListRowRects); i++) {
            if (func_ov182_021bc888(sApListRowRects[i])) {
                if (i < AP_LIST_ROWS) {
                    sApList->touched = i;
                    break;
                }
                DWCUtilUiObject_SetStateIfIdle(1);
                sApListPos.cursor = i;
                func_ov182_021b17f8();
                return;
            }
        }
    }
    if (func_ov182_021bc948(data_ov182_021c059c)) {
        for (i = 0; i < AP_LIST_ROWS; i++) {
            if (func_ov182_021bc948(sApListRowRects[i])) {
                if (sApList->touched != i) {
                    break;
                }
                if (i >= sApList->count) {
                    func_ov182_021b80f4(9);
                    break;
                }
                DWCUtilUiObject_SetStateIfIdle(1);
                sApListPos.cursor = i;
                func_ov182_021b17f8();
                return;
            }
        }
    }
    if (func_ov182_021bc798(1)) {
        DWCUtilUiObject_SetStateIfIdle(1);
        func_ov182_021b7174();
    } else if (func_ov182_021bc798(2)) {
        DWCUtilUiObject_SetStateIfIdle(0);
    } else if (func_ov182_021bc7c8(0x200)) {
        func_ov182_021b193c();
    } else if (func_ov182_021bc7f8(0x200)) {
        sApList->atEnd = FALSE;
    } else if (func_ov182_021bc7c8(0x100)) {
        func_ov182_021b18f8();
    } else if (func_ov182_021bc7f8(0x100)) {
        sApList->atEnd = FALSE;
    } else if (func_ov182_021bc7c8(0x40)) {
        func_ov182_021b182c(1);
    } else if (func_ov182_021bc7f8(0x40)) {
        sApList->atEnd = FALSE;
    } else if (func_ov182_021bc7c8(0x80)) {
        func_ov182_021b182c(3);
    } else if (func_ov182_021bc7f8(0x80)) {
        sApList->atEnd = FALSE;
    }
}

// The scroll bar: its handle dragged, or its arrows
static void func_ov182_021b10c8(void) {
    int offset;

    if (sApList->scrollTask != NULL) {
        return;
    }
    if (sApList->dragDelay != 0) {
        sApList->dragDelay--;
    }
    switch (func_ov182_021b7150()) {
    case 1:
        sApList->dragging = TRUE;
        DWCUtilUiObject_Disable();
        break;
    case 2:
        if (sApList->dragDelay == 0) {
            func_ov182_021ab8e4();
            sApListPos.scroll = FX_DivS32(func_ov182_021b7144() * sApList->scrollMax, sApList->barLength);
            func_ov182_021b14c0();
            sApList->dragDelay = 4;
        }
        break;
    case 3:
        sApList->dragging = FALSE;
        DWCUtilUiObject_Enable();
        sApListPos.scroll = FX_DivS32(func_ov182_021b7144() * sApList->scrollMax, sApList->barLength);
        func_ov182_021b80f4(0x13);
        func_ov182_021b14c0();
        offset = FX_ModS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
        if (offset == 0) {
            func_ov182_021b17f8();
        } else if (offset < AP_LIST_ROW_HEIGHT / 2) {
            sApList->scrollTask = func_ov182_021bd200(0, func_ov182_021b16d4, NULL, 0x78);
        } else {
            sApList->scrollTask = func_ov182_021bd200(0, func_ov182_021b1754, NULL, 0x78);
        }
        break;
    case 4:
        if (sApListPos.scroll == 0) {
            if (!sApList->barAtEnd) {
                func_ov182_021b80f4(9);
                sApList->barAtEnd = TRUE;
            }
        } else {
            func_ov182_021b80f4(0x13);
            sApList->scrollTask = func_ov182_021bd200(0, func_ov182_021b16d4, NULL, 0x78);
        }
        break;
    case 6:
        if (sApList->count <= AP_LIST_ROWS || sApListPos.scroll == sApList->scrollMax) {
            if (!sApList->barAtEnd) {
                func_ov182_021b80f4(9);
                sApList->barAtEnd = TRUE;
            }
        } else {
            func_ov182_021b80f4(0x13);
            sApList->scrollTask = func_ov182_021bd200(0, func_ov182_021b1754, NULL, 0x78);
        }
        break;
    case 5:
    case 7:
        sApList->barAtEnd = FALSE;
        break;
    }
}

static void func_ov182_021b1254(void) {
    int index;

    if (sApList->scrollTask != NULL || sApList->dragging) {
        return;
    }
    switch (DWCUtilUiObject_GetState()) {
    case 0:
        func_ov182_021b80f4(7);
        break;
    case 1:
        if (sApListPos.cursor == AP_LIST_BUTTON) {
            sApList->confirmed = TRUE;
            func_ov182_021b80f4(6);
            func_ov182_021b1994();
        } else {
            index = sApListPos.cursor + FX_DivS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
            if (sApList->entries[index].security == 2) {
                func_ov182_021b80f4(9);
                func_ov182_021b7174();
                DWCUtilUiObject_Disable();
                func_ov182_021b8cf0(0xe, 1, 1, -1, 0);
                func_ov182_021adfbc(func_ov182_021b19b4);
                return;
            }
            sApList->confirmed = TRUE;
            sApList->chosen = index;
            func_ov182_021b80f4(6);
        }
        break;
    default:
        return;
    }
    func_ov182_021adfbc(func_ov182_021b1308);
}

static void func_ov182_021b1308(void) {
    DWCUtilUiObject_Disable();
    func_ov182_021bbbf4(8);
    func_ov182_021adfbc(func_ov182_021b1320);
}

static void func_ov182_021b1320(void) {
    if (func_ov182_021bbaac(1) == 0) {
        if (sApList->confirmed) {
            DWCUtilUiObject_Exit();
        } else {
            DWCUtilUiObject_Disable();
        }
        func_ov182_021bbac4(3, 1, 1, 8);
        func_ov182_021bbac4(3, 0, 0x1d, 8);
        func_ov182_021adfbc(func_ov182_021b136c);
    }
}

// Frees the list and goes on: back, to the button's screen, or to the chosen access point's security key
static void func_ov182_021b136c(void) {
    int i;

    if (func_ov182_021bbaac(1) != 0 || func_ov182_021bbaac(0) != 0 || !DWCUtilUiObject_IsIdle()) {
        return;
    }
    func_ov182_021bd280(1, sApList->vblankTask);
    for (i = 0; i < AP_LIST_ROWS + 1; i++) {
        if (sApList->securityOams[i] != NULL) {
            func_ov182_021bcc9c(sApList->securityOams[i]);
        }
        if (sApList->signalOams[i] != NULL) {
            func_ov182_021bcc9c(sApList->signalOams[i]);
        }
    }
    func_ov182_021bbf38(sApList->text);
    func_ov182_021b711c();
    func_ov182_021ab8e4();
    DWCUtilScreen_Stop();
    func_ov182_021b6f78();
    func_ov182_021bb420(sApList->backScreen);
    func_ov182_021bb420(sApList->palette);
    func_ov182_021bc228(1, 1);
    func_ov182_021bc228(0, 0x1d);
    G2_SetBG0Offset(0, 0);
    G2_SetBG2Offset(0, 0);
    if (!sApList->confirmed) {
        func_ov182_021b74f4();
        func_ov182_021b7b20(func_ov182_021b781c()->index);
        func_ov182_021adfc8(2, 0);
        func_ov182_021adfbc(func_ov182_021b5bb8);
    } else if (sApListPos.cursor == AP_LIST_BUTTON) {
        func_ov182_021b74f4();
        func_ov182_021adfc8(0, 1);
        func_ov182_021adfbc(func_ov182_021b5000);
    } else {
        func_ov182_021b7854(sApList->entries[sApList->chosen].ssid);
        func_ov182_021adfc8(0, 0);
        if (sApList->entries[sApList->chosen].security != 0) {
            func_ov182_021adfc8(0, 1);
            func_ov182_021adff0(1, 1);
            func_ov182_021adfbc(func_ov182_021b19f4);
        } else {
            func_ov182_021adfc8(0, 1);
            func_ov182_021adff0(0, 1);
            func_ov182_021adfbc(func_ov182_021b6588);
        }
    }
    func_ov182_021bc4f0(&sApList);
}

// Draws the rows on the screen from the scroll
static void func_ov182_021b14c0(void) {
    int top = FX_DivS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
    int rows = sApList->count;
    int row;
    int index;

    func_ov182_021bc118(sApList->text, 0);
    if (rows > AP_LIST_ROWS + 1) {
        rows = AP_LIST_ROWS + 1;
    }
    for (row = 0, index = top; row < rows; row++) {
        func_ov182_021b1528(index++, row);
    }
    for (row = 0; row < rows; row++) {
        func_ov182_021b15fc(top++, row);
    }
    func_ov182_021bc184(sApList->text);
    func_ov182_021b1658();
}

// The access point's SSID, on two lines when it is longer than 16 characters
static void func_ov182_021b1528(int index, int row) {
    int length = func_ov182_021bcef8(sApList->entries[index].ssid, sizeof(sApList->entries[index].ssid));
    int y = row * AP_LIST_ROW_HEIGHT;
    u16 line[17];
    int count;
    int i;

    if (index >= sApList->count) {
        return;
    }
    if (length <= 16) {
        y += 6;
    }
    MI_CpuFill8(line, 0, sizeof(line));
    count = length <= 16 ? length : 16;
    for (i = 0; i < count; i++) {
        line[i] = sApList->entries[index].ssid[i];
    }
    func_ov182_021bc030(sApList->text, 10, y, 2, 10, line, 1);
    if (length > 16) {
        MI_CpuFill8(line, 0, sizeof(line));
        for (i = 0; i < length - 16; i++) {
            line[i] = sApList->entries[index].ssid[16 + i];
        }
        y += 12;
        func_ov182_021bc030(sApList->text, 10, y, 2, 10, line, 1);
    }
}

static void func_ov182_021b15fc(int index, int row) {
    if (index >= sApList->count) {
        return;
    }
    G2_SetOBJCharName(sApList->securityOams[row], sApList->securityChars[sApList->entries[index].security]);
    G2_SetOBJCharName(sApList->signalOams[row], sApList->signalChars[sApList->entries[index].linkLevel]);
}

// Moves the icons to the rows' places from the scroll, and has the BGs follow at the next VBlank
static void func_ov182_021b1658(void) {
    int y = 0x36 - FX_ModS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
    int rows = sApList->count;
    int row;

    if (rows > AP_LIST_ROWS + 1) {
        rows = AP_LIST_ROWS + 1;
    }
    for (row = 0; row < rows; row++) {
        DWCUtilOam_SetPosition(sApList->securityOams[row], 0xb3, y - 2);
        DWCUtilOam_SetPosition(sApList->signalOams[row], 0xd2, y + 1);
        y += AP_LIST_ROW_HEIGHT;
    }
    sApList->moved = TRUE;
}

// Scrolls the list up by 4 pixels a frame, until it rests on a row
static void func_ov182_021b16d4(DWCUtilTask *task, void *arg) {
    int offset;

    func_ov182_021b7174();
    func_ov182_021ab8e4();
    if (sApListPos.scroll > 4) {
        sApListPos.scroll -= 4;
    } else {
        sApListPos.scroll = 0;
    }
    offset = FX_ModS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
    if (offset == AP_LIST_ROW_HEIGHT - 4) {
        func_ov182_021b14c0();
        return;
    }
    if (offset > AP_LIST_ROW_HEIGHT - 4) {
        sApListPos.scroll += AP_LIST_ROW_HEIGHT - offset;
        offset = 0;
    }
    func_ov182_021b1658();
    if (offset == 0) {
        func_ov182_021b715c(FX_DivS32(sApListPos.scroll * sApList->barLength, sApList->scrollMax));
        func_ov182_021b7164();
        func_ov182_021b17f8();
        sApList->scrollTask = NULL;
        func_ov182_021bd268(0, task);
    }
}

// Scrolls the list down by 4 pixels a frame, until it rests on a row
static void func_ov182_021b1754(DWCUtilTask *task, void *arg) {
    int offset;

    func_ov182_021b7174();
    func_ov182_021ab8e4();
    sApListPos.scroll += 4;
    offset = FX_ModS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT);
    if (offset >= 4) {
        func_ov182_021b1658();
        return;
    }
    sApListPos.scroll -= offset;
    func_ov182_021b14c0();
    func_ov182_021b715c(FX_DivS32(sApListPos.scroll * sApList->barLength, sApList->scrollMax));
    func_ov182_021b7164();
    func_ov182_021b17f8();
    sApList->scrollTask = NULL;
    func_ov182_021bd268(0, task);
}

static void func_ov182_021b17bc(DWCUtilTask *task, void *arg) {
    if (sApList->moved) {
        int y = FX_ModS32(sApListPos.scroll, AP_LIST_ROW_HEIGHT) - 0x32;

        G2_SetBG0Offset(0, y);
        G2_SetBG2Offset(0, y);
        sApList->moved = FALSE;
    }
}

static void func_ov182_021b17f8(void) {
    u8 cursor = sApListPos.cursor;
    int pair = 2;

    if (cursor >= AP_LIST_ROWS) {
        pair = 3;
    }
    func_ov182_021ab7f0(pair, sApListCursorRects[cursor][0], sApListCursorRects[cursor][2],
                        sApListCursorRects[cursor][1]);
}

// Moves the cursor up (1) or down (3), scrolling the list at its ends
static void func_ov182_021b182c(int direction) {
    BOOL moved = TRUE;

    switch (sApListPos.cursor) {
    case 0:
        if (direction == 1) {
            if (sApListPos.scroll == 0) {
                sApListPos.cursor = AP_LIST_BUTTON;
            } else {
                func_ov182_021b80f4(0x13);
                sApList->scrollTask = func_ov182_021bd200(0, func_ov182_021b16d4, NULL, 0x78);
                return;
            }
        } else if (sApList->count > 1) {
            sApListPos.cursor++;
        } else {
            moved = FALSE;
        }
        break;
    case 1:
    case 2:
        if (direction == 1) {
            sApListPos.cursor--;
        } else if (sApList->count > sApListPos.cursor + 1) {
            sApListPos.cursor++;
        } else {
            moved = FALSE;
        }
        break;
    case 3:
        if (direction == 1) {
            sApListPos.cursor--;
        } else {
            func_ov182_021b193c();
            return;
        }
        break;
    case AP_LIST_BUTTON:
        if (direction == 1) {
            moved = FALSE;
        } else {
            sApListPos.scroll = 0;
            sApListPos.cursor = 0;
            func_ov182_021b14c0();
            func_ov182_021b715c(0);
        }
        break;
    }
    if (!moved) {
        if (!sApList->atEnd) {
            func_ov182_021b80f4(9);
            sApList->atEnd = TRUE;
        }
    } else {
        func_ov182_021b80f4(8);
        func_ov182_021b17f8();
    }
}

// Scrolls the list up by a row
static void func_ov182_021b18f8(void) {
    if (sApListPos.scroll == 0) {
        if (!sApList->atEnd) {
            func_ov182_021b80f4(9);
            sApList->atEnd = TRUE;
        }
    } else {
        func_ov182_021b80f4(0x13);
        sApList->scrollTask = func_ov182_021bd200(0, func_ov182_021b16d4, NULL, 0x78);
    }
}

// Scrolls the list down by a row
static void func_ov182_021b193c(void) {
    if (sApListPos.scroll == sApList->scrollMax || sApList->count <= AP_LIST_ROWS) {
        if (!sApList->atEnd) {
            func_ov182_021b80f4(9);
            sApList->atEnd = TRUE;
        }
    } else {
        func_ov182_021b80f4(0x13);
        sApList->scrollTask = func_ov182_021bd200(0, func_ov182_021b1754, NULL, 0x78);
    }
}

// Lights up the button: its row of the palette
static void func_ov182_021b1994(void) {
    u8 rows[] = { 2 };

    func_ov182_021b7014(sApList->palette, rows[0], rows[0]);
}

static void func_ov182_021b19b4(void) {
    if (func_ov182_021b8f14() == 0) {
        func_ov182_021b80f4(6);
        func_ov182_021b8eec();
        func_ov182_021adfbc(func_ov182_021b19d4);
    }
}

static void func_ov182_021b19d4(void) {
    if (!func_ov182_021b8f24()) {
        DWCUtilUiObject_Enable();
        func_ov182_021b7164();
        func_ov182_021adfbc(func_ov182_021b0f40);
    }
}
