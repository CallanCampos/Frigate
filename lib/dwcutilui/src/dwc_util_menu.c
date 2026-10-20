#include "dwc_util_menu.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_heap.h"
#include "dwc_util_oam.h"
#include "dwc_util_oam_attr.h"
#include "dwc_util_oam_buf.h"
#include "dwc_util_shared.h"
#include "dwc_util_sound.h"
#include "dwc_util_task.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "nitro/gx.h"

typedef struct DWCUtilMenuPosition {
    u16 x;
    u16 y;
} DWCUtilMenuPosition;

typedef struct DWCUtilMenuRect {
    u16 x;
    u16 y;
    u16 width;
    u16 height;
} DWCUtilMenuRect;

typedef struct DWCUtilMenu {
    // The digits' labels, three to a row of keys, and the OAM entries that show them
    DWCUtilText *rows[4];
    GXOamAttr *keyOams[10];
    GXOamAttr *arrowOams[2];
    DWCUtilOamNode *buttons[2];
    DWCUtilOamNode *rowNodes[4];
    DWCUtilOamNode *cursor;
    DWCUtilTask *task;
    // The code of the key chosen this frame
    u8 code;
    // The key the touch started on, and the one shown pressed
    s8 touched;
    s8 pressed;
    // The key the cursor is on, and the one it came from to the buttons' row
    s8 cursorPos;
    s8 lastPos;
    // The frames the left arrow has been held
    u8 repeat;
    u8 leftEnabled;
    u8 digitsEnabled;
    u8 rightEnabled;
    // Whether B has buzzed since it was pressed
    u8 buzzed;
} DWCUtilMenu;

static const u8 sButtonEffects[2] = { 4, 5 };
static const u8 sArrowCells[2] = { 0x37, 0x38 };
static const u8 sButtonCells[2] = { 0x3c, 0x3d };
static const u8 sKeyPalettes[2] = { 2, 3 };
static const u8 sButtonCodes[3] = { 0x12, 0x13, 0 };
static const u8 sArrowCodes[3] = { 0x10, 0x11, 0 };
static const u16 sKeySize[2] = { 0x1c, 0x14 };
// The ROM keeps this size among the file's tables, but none of the file's code reads it
const u16 data_ov182_021bfe8e[2] = { 0x0c, 0x04 };
static const u16 sArrowSize[2] = { 0x1c, 0x14 };
static const u16 sButtonSize[2] = { 0x78, 0x12 };
static const DWCUtilMenuPosition sArrowPositions[2] = { { 0x72, 0x91 }, { 0x92, 0x91 } };
static const DWCUtilMenuPosition sButtonPositions[2] = { { 0x04, 0xaa }, { 0x84, 0xaa } };
static const u8 sDigitCodes[12] = { '7', '8', '9', '4', '5', '6', '1', '2', '3', '0' };
static const u16 sLabels[13] = { '7', '8', '9', '4', '5', '6', '1', '2', '3', '0', ' ', ' ' };
static const DWCUtilMenuPosition sKeyPositions[10] = {
    { 0x52, 0x4c }, { 0x72, 0x4c }, { 0x92, 0x4c }, { 0x52, 0x63 }, { 0x72, 0x63 },
    { 0x92, 0x63 }, { 0x52, 0x7a }, { 0x72, 0x7a }, { 0x92, 0x7a }, { 0x52, 0x91 },
};
static const DWCUtilMenuPosition sCursorPositions[14] = {
    { 0x50, 0x4a }, { 0x70, 0x4a }, { 0x90, 0x4a }, { 0x50, 0x61 }, { 0x70, 0x61 }, { 0x90, 0x61 }, { 0x50, 0x78 },
    { 0x70, 0x78 }, { 0x90, 0x78 }, { 0x50, 0x8f }, { 0x70, 0x8f }, { 0x90, 0x8f }, { 0x02, 0xa8 }, { 0x82, 0xa8 },
};
// The key the cursor moves to from each key, left, up, right and down; -1 and -2 pick an arrow or a key of the top
// row from where the cursor came to the buttons from
static const s8 sNavigation[14][4] = {
    { 2, 12, 1, 3 },  { 0, 13, 2, 4 },  { 1, 13, 0, 5 },  { 5, 0, 4, 6 },     { 3, 1, 5, 7 },
    { 4, 2, 3, 8 },   { 8, 3, 7, 9 },   { 6, 4, 8, 10 },  { 7, 5, 6, 11 },    { 11, 6, 10, 12 },
    { 9, 7, 11, 13 }, { 10, 8, 9, 13 }, { 13, 9, 13, 0 }, { 12, -1, 12, -2 },
};

static DWCUtilMenu *sMenu;

static void func_ov182_021acff0(DWCUtilTask *task, void *arg);
static void func_ov182_021ad038(DWCUtilTask *task, void *arg);
static void func_ov182_021ad080(DWCUtilTask *task, void *arg);
static void func_ov182_021ad0c8(DWCUtilTask *task, void *arg);
static void func_ov182_021ad110(DWCUtilTask *task, void *arg);
static void func_ov182_021ad15c(DWCUtilTask *task, void *arg);
static void func_ov182_021ad170(void);
static void func_ov182_021ad294(void);
static void func_ov182_021ad3b4(void);
static void func_ov182_021ad4ec(int key);
static void func_ov182_021ad518(void);
static void func_ov182_021ad634(int row, u32 y);
static void func_ov182_021ad7ac(int key, int pressed);
static void func_ov182_021ad838(void);
static void func_ov182_021ad894(int direction);
static void func_ov182_021ad90c(DWCUtilTask *task, void *arg);
static void func_ov182_021ad948(DWCUtilTask *task, void *arg);
static void func_ov182_021ad97c(DWCUtilTask *task, void *arg);
static void func_ov182_021ad9b0(DWCUtilTask *task, void *arg);
static void func_ov182_021ad9e4(DWCUtilTask *task, void *arg);
static void func_ov182_021ada18(DWCUtilTask *task, void *arg);

void func_ov182_021acd80(void) {
    DWCUtilMenuRect rect = { 0, 1, 0, 0 };
    int objCount;
    u16 label[2];
    int i;
    int k;
    int j;

    rect.width = 0x1c;
    rect.height = 0x14;
    sMenu = func_ov182_021bc4d8(sizeof(DWCUtilMenu), 4);
    sMenu->code = 0x1f;
    sMenu->cursorPos = 0;
    sMenu->leftEnabled = TRUE;
    sMenu->digitsEnabled = TRUE;
    sMenu->rightEnabled = TRUE;
    for (i = 0; i < 10; i++) {
        sMenu->keyOams[i] = func_ov182_021bb960(0, 0x36);
        G2_SetOBJEffect(sMenu->keyOams[i], GX_OAM_EFFECT_NODISPLAY, 0);
        G2_SetOBJPriority(sMenu->keyOams[i], 3);
    }
    for (i = 0; i < 2; i++) {
        sMenu->arrowOams[i] = func_ov182_021bb960(0, sArrowCells[i]);
        G2_SetOBJEffect(sMenu->arrowOams[i], GX_OAM_EFFECT_NODISPLAY, 0);
        G2_SetOBJPriority(sMenu->arrowOams[i], 3);
    }
    for (i = 0; i < 2; i++) {
        sMenu->buttons[i] = func_ov182_021bb97c(0, sButtonCells[i], TRUE);
        func_ov182_021bb6f8(sMenu->buttons[i], -1, GX_OAM_EFFECT_NODISPLAY, 0);
        func_ov182_021bb878(sMenu->buttons[i], -1, 3);
    }
    label[1] = 0;
    for (i = 0; i < 4; i++) {
        sMenu->rows[i] = func_ov182_021bbd3c(0, 0xc, 4, 0, &objCount, 0);
        rect.x = 0;
        for (j = 0, k = i * 3; j < 3; j++, k++) {
            label[0] = sLabels[k];
            func_ov182_021bc0a4(sMenu->rows[i], rect.x, rect.y, rect.width, rect.height, 2, 0x480, label);
            rect.x += 0x20;
        }
        sMenu->rowNodes[i] = func_ov182_021bb594(0, objCount, FALSE);
    }
    sMenu->cursor = func_ov182_021bb97c(0, 0x44, TRUE);
    func_ov182_021bb6f8(sMenu->cursor, -1, GX_OAM_EFFECT_NODISPLAY, 0);
    func_ov182_021bb878(sMenu->cursor, -1, 2);
    sMenu->task = func_ov182_021bd200(0, func_ov182_021acff0, NULL, 0x78);
    func_ov182_021ad634(0, 0xc0);
}

void func_ov182_021acf7c(void) {
    func_ov182_021bb690(sMenu->cursor);
    func_ov182_021bd264(sMenu->task, func_ov182_021ad90c);
}

u8 func_ov182_021acf9c(void) {
    return sMenu->code;
}

void func_ov182_021acfac(u8 enabled) {
    sMenu->leftEnabled = enabled;
}

void func_ov182_021acfbc(u8 enabled) {
    sMenu->digitsEnabled = enabled;
}

void func_ov182_021acfcc(u8 enabled) {
    sMenu->rightEnabled = enabled;
}

BOOL func_ov182_021acfdc(void) {
    if (sMenu != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov182_021acff0(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[0]).attr01 - 12;

    if (y > 0x4c) {
        func_ov182_021ad634(0, y);
        return;
    }
    func_ov182_021ad634(0, 0x4c);
    func_ov182_021ad634(1, 0xc0);
    func_ov182_021bd264(task, func_ov182_021ad038);
}

static void func_ov182_021ad038(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[3]).attr01 - 12;

    if (y > 0x63) {
        func_ov182_021ad634(1, y);
        return;
    }
    func_ov182_021ad634(1, 0x63);
    func_ov182_021ad634(2, 0xc0);
    func_ov182_021bd264(task, func_ov182_021ad080);
}

static void func_ov182_021ad080(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[6]).attr01 - 12;

    if (y > 0x7a) {
        func_ov182_021ad634(2, y);
        return;
    }
    func_ov182_021ad634(2, 0x7a);
    func_ov182_021ad634(3, 0xc0);
    func_ov182_021bd264(task, func_ov182_021ad0c8);
}

static void func_ov182_021ad0c8(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[9]).attr01 - 12;

    if (y > 0x91) {
        func_ov182_021ad634(3, y);
        return;
    }
    func_ov182_021ad634(3, 0x91);
    func_ov182_021ad634(4, 0xc0);
    func_ov182_021bd264(task, func_ov182_021ad110);
}

static void func_ov182_021ad110(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    func_ov182_021bb8bc(sMenu->buttons[0], 0, &x, &y);
    y -= 12;
    if (y > 0xaa) {
        func_ov182_021ad634(4, y);
        return;
    }
    func_ov182_021ad634(4, 0xaa);
    func_ov182_021ad838();
    func_ov182_021bd264(task, func_ov182_021ad15c);
}

static void func_ov182_021ad15c(DWCUtilTask *task, void *arg) {
    func_ov182_021ad170();
    func_ov182_021ad294();
    func_ov182_021ad3b4();
    func_ov182_021ad518();
}

// The start of a touch: which key it is on
static void func_ov182_021ad170(void) {
    u16 rect[4];
    int i;

    if (func_ov182_021bc888(data_ov182_021c059c)) {
        sMenu->touched = -1;
        for (i = 0; i < 10; i++) {
            func_ov182_021bc1c0(&sKeyPositions[i], sKeySize, rect);
            if (func_ov182_021bc888(rect)) {
                if (!sMenu->digitsEnabled) {
                    func_ov182_021b80f4(9);
                    return;
                }
                func_ov182_021b80f4(0);
                sMenu->touched = i;
                return;
            }
        }
        for (i = 0; i < 2; i++) {
            func_ov182_021bc1c0(&sArrowPositions[i], sArrowSize, rect);
            if (func_ov182_021bc888(rect)) {
                if ((i == 0 && !sMenu->leftEnabled) || (i == 1 && !sMenu->rightEnabled)) {
                    func_ov182_021b80f4(9);
                    return;
                }
                func_ov182_021b80f4(0);
                sMenu->touched = i + 10;
                return;
            }
        }
        for (i = 0; i < 2; i++) {
            func_ov182_021bc1c0(&sButtonPositions[i], sButtonSize, rect);
            if (func_ov182_021bc888(rect)) {
                func_ov182_021b80f4(0);
                sMenu->touched = i + 12;
                return;
            }
        }
    }
}

// The end of a touch: the key it chooses, if it ends on the key it started on
static void func_ov182_021ad294(void) {
    u16 rect[4];
    int i;

    sMenu->code = 0;
    if (func_ov182_021bc948(data_ov182_021c059c)) {
        for (i = 0; i < 10; i++) {
            func_ov182_021bc1c0(&sKeyPositions[i], sKeySize, rect);
            if (func_ov182_021bc948(rect)) {
                if (sMenu->touched != i) {
                    return;
                }
                sMenu->code = sDigitCodes[i];
                sMenu->cursorPos = i;
                func_ov182_021ad838();
                return;
            }
        }
        for (i = 0; i < 2; i++) {
            func_ov182_021bc1c0(&sArrowPositions[i], sArrowSize, rect);
            if (func_ov182_021bc948(rect)) {
                if (sMenu->touched != i + 10) {
                    return;
                }
                sMenu->code = sArrowCodes[i];
                sMenu->cursorPos = i + 10;
                func_ov182_021ad838();
                return;
            }
        }
        for (i = 0; i < 2; i++) {
            func_ov182_021bc1c0(&sButtonPositions[i], sButtonSize, rect);
            if (func_ov182_021bc948(rect)) {
                if (sMenu->touched != i + 12) {
                    return;
                }
                sMenu->code = sButtonCodes[i];
                sMenu->cursorPos = i + 12;
                func_ov182_021ad838();
                return;
            }
        }
    }
}

// A held touch: the key it holds is shown pressed, and the left arrow repeats
static void func_ov182_021ad3b4(void) {
    u16 rect[4];
    int i;

    if (func_ov182_021bc828(data_ov182_021c059c)) {
        for (i = 0; i < 10; i++) {
            func_ov182_021bc1c0(&sKeyPositions[i], sKeySize, rect);
            if (func_ov182_021bc828(rect)) {
                if (sMenu->touched != i) {
                    goto released;
                }
                goto held;
            }
        }
        for (i = 0; i < 2; i++) {
            func_ov182_021bc1c0(&sArrowPositions[i], sArrowSize, rect);
            if (func_ov182_021bc828(rect)) {
                if (sMenu->touched != i + 10) {
                    goto released;
                }
                func_ov182_021ad4ec(i + 10);
                if (i != 0) {
                    goto clear;
                }
                sMenu->repeat++;
                if (sMenu->repeat < 0x28) {
                    return;
                }
                if (!sMenu->leftEnabled) {
                    func_ov182_021b80f4(9);
                    sMenu->touched = -1;
                    return;
                }
                sMenu->code = 0x10;
                sMenu->repeat -= 7;
                return;
            }
        }
        for (i = 0; i < 2; i++) {
            func_ov182_021bc1c0(&sButtonPositions[i], sButtonSize, rect);
            if (func_ov182_021bc828(rect)) {
                if (sMenu->touched == i + 12) {
                    i += 12;
                    goto held;
                }
                break;
            }
        }
    }
released:
    i = -1;
held:
    func_ov182_021ad4ec(i);
clear:
    sMenu->repeat = 0;
}

static void func_ov182_021ad4ec(int key) {
    if (key != sMenu->pressed) {
        func_ov182_021ad7ac(key, TRUE);
        func_ov182_021ad7ac(sMenu->pressed, FALSE);
        sMenu->pressed = key;
    }
}

// The keys: the D-pad moves the cursor, A chooses its key and B is the left arrow
static void func_ov182_021ad518(void) {
    int key;

    if (func_ov182_021bc7c8(0x20)) {
        func_ov182_021ad894(0);
    }
    if (func_ov182_021bc7c8(0x40)) {
        func_ov182_021ad894(1);
    }
    if (func_ov182_021bc7c8(0x10)) {
        func_ov182_021ad894(2);
    }
    if (func_ov182_021bc7c8(0x80)) {
        func_ov182_021ad894(3);
    }
    if (func_ov182_021bc798(1)) {
        key = sMenu->cursorPos;
        if (key < 10) {
            if (sMenu->digitsEnabled) {
                sMenu->code = sDigitCodes[key];
                return;
            }
            func_ov182_021b80f4(9);
            return;
        }
        if (key - 10 < 2) {
            if ((key - 10 == 0 && !sMenu->leftEnabled) || (key - 10 == 1 && !sMenu->rightEnabled)) {
                func_ov182_021b80f4(9);
                return;
            }
            sMenu->code = sArrowCodes[key - 10];
            return;
        }
        sMenu->code = sButtonCodes[key - 12];
    }
    if (func_ov182_021bc7c8(2)) {
        if (!sMenu->leftEnabled) {
            if (!sMenu->buzzed) {
                func_ov182_021b80f4(9);
                sMenu->buzzed = TRUE;
            }
        } else {
            sMenu->code = 0x10;
        }
    } else if (func_ov182_021bc7f8(2)) {
        sMenu->buzzed = FALSE;
    }
}

// Moves a row of keys to y: rows 0 to 3 of keys, with the arrows on row 3, and row 4 of the buttons
static void func_ov182_021ad634(int row, u32 y) {
    const u8 keyCounts[5] = { 3, 3, 3, 1, 0 };
    const u8 arrowCounts[5] = { 0, 0, 0, 2, 0 };
    const u8 buttonCounts[5] = { 0, 0, 0, 0, 2 };
    int key = row * 3;
    int i;

    for (i = 0; i < keyCounts[row]; i++) {
        G2_SetOBJEffect(sMenu->keyOams[key], GX_OAM_EFFECT_NONE, 0);
        sMenu->keyOams[key]->attr01 =
            ((sKeyPositions[key].x & 0x1ff) << GX_OAM_ATTR01_X_SHIFT) |
            ((sMenu->keyOams[key]->attr01 & ~(GX_OAM_ATTR01_X_MASK | GX_OAM_ATTR01_Y_MASK)) | (y & 0xff));
        key++;
    }
    if (row < 4) {
        func_ov182_021bc124(sMenu->rows[row], sKeyPositions[row * 3].x, y, sMenu->rowNodes[row], 2);
    }
    for (i = 0; i < arrowCounts[row]; i++) {
        G2_SetOBJEffect(sMenu->arrowOams[i], GX_OAM_EFFECT_NONE, 0);
        sMenu->arrowOams[i]->attr01 =
            ((sArrowPositions[i].x & 0x1ff) << GX_OAM_ATTR01_X_SHIFT) |
            ((sMenu->arrowOams[i]->attr01 & ~(GX_OAM_ATTR01_X_MASK | GX_OAM_ATTR01_Y_MASK)) | (y & 0xff));
    }
    for (i = 0; i < buttonCounts[row]; i++) {
        func_ov182_021bb6f8(sMenu->buttons[i], -1, GX_OAM_EFFECT_NONE, 0);
        func_ov182_021bb7e0(sMenu->buttons[i], -1, sButtonPositions[i].x, y);
    }
}

static void func_ov182_021ad7ac(int key, int pressed) {
    if (key < 0) {
        return;
    }
    if (key < 10) {
        G2_SetOBJMode(sMenu->keyOams[key], GX_OAM_MODE_NORMAL, sKeyPalettes[pressed]);
    } else if (key - 10 < 2) {
        G2_SetOBJMode(sMenu->arrowOams[key - 10], GX_OAM_MODE_NORMAL, sKeyPalettes[pressed]);
    } else {
        func_ov182_021bb778(sMenu->buttons[key - 12], -1, GX_OAM_MODE_NORMAL, sButtonEffects[pressed]);
    }
}

static void func_ov182_021ad838(void) {
    int cell = 0x44;

    if (sMenu->cursorPos > 11) {
        cell = 0x45;
    }
    func_ov182_021bb914(0, cell, func_ov182_021bb6ec(sMenu->cursor, 0));
    func_ov182_021bb878(sMenu->cursor, -1, 2);
    func_ov182_021bb7e0(sMenu->cursor, -1, sCursorPositions[sMenu->cursorPos].x, sCursorPositions[sMenu->cursorPos].y);
}

static void func_ov182_021ad894(int direction) {
    int previous = sMenu->cursorPos;

    sMenu->cursorPos = sNavigation[previous][direction];
    if (sMenu->cursorPos == 13 && (direction == 1 || direction == 3)) {
        sMenu->lastPos = previous;
    } else if (sMenu->cursorPos == -1) {
        if (sMenu->lastPos == 1 || sMenu->lastPos == 10) {
            sMenu->cursorPos = 10;
        } else {
            sMenu->cursorPos = 11;
        }
    } else if (sMenu->cursorPos == -2) {
        if (sMenu->lastPos == 1 || sMenu->lastPos == 10) {
            sMenu->cursorPos = 1;
        } else {
            sMenu->cursorPos = 2;
        }
    }
    func_ov182_021ad838();
    func_ov182_021b80f4(8);
}

static void func_ov182_021ad90c(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(func_ov182_021bb6ec(sMenu->buttons[0], 0)).attr01 + 12;

    func_ov182_021ad634(4, y);
    if (y >= 0xc0) {
        func_ov182_021bd264(task, func_ov182_021ad948);
    }
}

static void func_ov182_021ad948(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[9]).attr01 + 12;

    func_ov182_021ad634(3, y);
    if (y >= 0xc0) {
        func_ov182_021bd264(task, func_ov182_021ad97c);
    }
}

static void func_ov182_021ad97c(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[6]).attr01 + 12;

    func_ov182_021ad634(2, y);
    if (y >= 0xc0) {
        func_ov182_021bd264(task, func_ov182_021ad9b0);
    }
}

static void func_ov182_021ad9b0(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[3]).attr01 + 12;

    func_ov182_021ad634(1, y);
    if (y >= 0xc0) {
        func_ov182_021bd264(task, func_ov182_021ad9e4);
    }
}

static void func_ov182_021ad9e4(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[0]).attr01 + 12;

    func_ov182_021ad634(0, y);
    if (y >= 0xc0) {
        func_ov182_021bd264(task, func_ov182_021ada18);
    }
}

static void func_ov182_021ada18(DWCUtilTask *task, void *arg) {
    int i;

    func_ov182_021bd268(0, task);
    for (i = 0; i < 4; i++) {
        func_ov182_021bb690(sMenu->rowNodes[i]);
        func_ov182_021bbdcc(sMenu->rows[i]);
    }
    for (i = 0; i < 2; i++) {
        func_ov182_021bb690(sMenu->buttons[i]);
    }
    for (i = 0; i < 2; i++) {
        func_ov182_021bcc9c(sMenu->arrowOams[i]);
    }
    for (i = 0; i < 10; i++) {
        func_ov182_021bcc9c(sMenu->keyOams[i]);
    }
    func_ov182_021bc4f0(&sMenu);
}
