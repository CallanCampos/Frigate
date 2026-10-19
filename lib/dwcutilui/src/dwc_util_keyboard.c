#include "dwc_util_keyboard.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_heap.h"
#include "dwc_util_oam.h"
#include "dwc_util_oam_attr.h"
#include "dwc_util_oam_buf.h"
#include "dwc_util_shared.h"
#include "dwc_util_task.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "nitro/fx.h"
#include "nitro/gx.h"

// The keys are numbered in one sequence: the 47 character keys, row by row, then the four special keys (the
// capitals and shift keys, the space and the backspace), then the two buttons
#define KEYBOARD_CHAR_KEYS 47
#define KEYBOARD_SPECIAL_KEYS 4
#define KEYBOARD_FIRST_SPECIAL KEYBOARD_CHAR_KEYS
#define KEYBOARD_FIRST_BUTTON (KEYBOARD_FIRST_SPECIAL + KEYBOARD_SPECIAL_KEYS)

// The key code of the backspace
#define KEYBOARD_BACKSPACE 0x80

typedef struct DWCUtilKeyboardPoint {
    u16 x;
    u16 y;
} DWCUtilKeyboardPoint;

typedef struct DWCUtilKeyboard {
    // The labels of each mode's rows
    DWCUtilText *labels[3][4];
    GXOamAttr *charOams[KEYBOARD_CHAR_KEYS];
    GXOamAttr *specialOams[KEYBOARD_SPECIAL_KEYS];
    DWCUtilOamNode *buttons[2];
    // The OAM entries that show each row's labels
    DWCUtilOamNode *labelNodes[4];
    DWCUtilOamNode *cursor;
    DWCUtilTask *task;
    // The code of the key entered this frame
    u8 key;
    u8 mode;
    // The key the touch started on, the key drawn pressed, the key the cursor came from and the cursor's key
    s8 touched;
    s8 pressed;
    s8 lastPos;
    s8 cursorPos;
    // How long the backspace has been held
    u8 repeat;
    u8 backspaceEnabled;
    u8 charsEnabled;
    // Whether the error sound of a B press with the backspace off has played
    u8 errorPlayed;
} DWCUtilKeyboard;

static const u8 sKeyPalettes[2] = { 2, 3 };
static const u8 sButtonPalettes[2] = { 4, 5 };
static const u8 sButtonCells[2] = { 0x3c, 0x3d };
static const u8 sButtonKeys[3] = { 0x82, 0x83, 0 };
static const u8 sSpecialCells[KEYBOARD_SPECIAL_KEYS] = { 0x3a, 0x3b, 0x35, 0x39 };
static const u16 sLabelSize[2] = { 0x1c, 2 };
static const u16 sButtonSize[2] = { 0x78, 0x12 };
static const u16 sCharKeySize[2] = { 0x11, 0x12 };
static const u8 sSpecialKeys[KEYBOARD_SPECIAL_KEYS] = { 0xff, 0xff, ' ', KEYBOARD_BACKSPACE };
static const DWCUtilKeyboardPoint sButtonPositions[2] = { { 0x04, 0xaa }, { 0x84, 0xaa } };
static const DWCUtilKeyboardPoint sSpecialPositions[KEYBOARD_SPECIAL_KEYS] = {
    { 0x04, 0x67 },
    { 0x04, 0x7d },
    { 0x04, 0x93 },
    { 0xdc, 0x51 },
};
static const DWCUtilKeyboardPoint sSpecialSizes[KEYBOARD_SPECIAL_KEYS] = {
    { 0x1b, 0x12 },
    { 0x20, 0x12 },
    { 0x28, 0x12 },
    { 0x20, 0x12 },
};
static const DWCUtilKeyboardPoint sCharKeyPositions[KEYBOARD_CHAR_KEYS] = {
    { 0x04, 0x51 }, { 0x16, 0x51 }, { 0x28, 0x51 }, { 0x3a, 0x51 }, { 0x4c, 0x51 }, { 0x5e, 0x51 }, { 0x70, 0x51 },
    { 0x82, 0x51 }, { 0x94, 0x51 }, { 0xa6, 0x51 }, { 0xb8, 0x51 }, { 0xca, 0x51 }, { 0x20, 0x67 }, { 0x32, 0x67 },
    { 0x44, 0x67 }, { 0x56, 0x67 }, { 0x68, 0x67 }, { 0x7a, 0x67 }, { 0x8c, 0x67 }, { 0x9e, 0x67 }, { 0xb0, 0x67 },
    { 0xc2, 0x67 }, { 0xd4, 0x67 }, { 0xe6, 0x67 }, { 0x25, 0x7d }, { 0x37, 0x7d }, { 0x49, 0x7d }, { 0x5b, 0x7d },
    { 0x6d, 0x7d }, { 0x7f, 0x7d }, { 0x91, 0x7d }, { 0xa3, 0x7d }, { 0xb5, 0x7d }, { 0xc7, 0x7d }, { 0xd9, 0x7d },
    { 0xeb, 0x7d }, { 0x2d, 0x93 }, { 0x3f, 0x93 }, { 0x51, 0x93 }, { 0x63, 0x93 }, { 0x75, 0x93 }, { 0x87, 0x93 },
    { 0x99, 0x93 }, { 0xab, 0x93 }, { 0xbd, 0x93 }, { 0xcf, 0x93 }, { 0xe1, 0x93 },
};
// Where the cursor sits on each key
static const DWCUtilKeyboardPoint sCursorPositions[KEYBOARD_FIRST_BUTTON + 2] = {
    { 0x02, 0x4f }, { 0x14, 0x4f }, { 0x26, 0x4f }, { 0x38, 0x4f }, { 0x4a, 0x4f }, { 0x5c, 0x4f }, { 0x6e, 0x4f },
    { 0x80, 0x4f }, { 0x92, 0x4f }, { 0xa4, 0x4f }, { 0xb6, 0x4f }, { 0xc8, 0x4f }, { 0x1e, 0x65 }, { 0x30, 0x65 },
    { 0x42, 0x65 }, { 0x54, 0x65 }, { 0x66, 0x65 }, { 0x78, 0x65 }, { 0x8a, 0x65 }, { 0x9c, 0x65 }, { 0xae, 0x65 },
    { 0xc0, 0x65 }, { 0xd2, 0x65 }, { 0xe4, 0x65 }, { 0x23, 0x7b }, { 0x35, 0x7b }, { 0x47, 0x7b }, { 0x59, 0x7b },
    { 0x6b, 0x7b }, { 0x7d, 0x7b }, { 0x8f, 0x7b }, { 0xa1, 0x7b }, { 0xb3, 0x7b }, { 0xc5, 0x7b }, { 0xd7, 0x7b },
    { 0xe9, 0x7b }, { 0x2b, 0x91 }, { 0x3d, 0x91 }, { 0x4f, 0x91 }, { 0x61, 0x91 }, { 0x73, 0x91 }, { 0x85, 0x91 },
    { 0x97, 0x91 }, { 0xa9, 0x91 }, { 0xbb, 0x91 }, { 0xcd, 0x91 }, { 0xdf, 0x91 }, { 0x02, 0x65 }, { 0x02, 0x7b },
    { 0x02, 0x91 }, { 0xda, 0x4f }, { 0x02, 0xa8 }, { 0x82, 0xa8 },
};
// The key the cursor moves to from each key, to the left, up, right and down. The negative values pick the key from
// the one the cursor came from
static const s8 sNavigation[KEYBOARD_FIRST_BUTTON + 2][4] = {
    { 0x32, 0x33, 0x01, 0x2f }, { 0x00, 0x33, 0x02, 0x0c }, { 0x01, 0x33, 0x03, 0x0d }, { 0x02, 0x33, 0x04, 0x0e },
    { 0x03, 0x33, 0x05, 0x0f }, { 0x04, 0x33, 0x06, 0x10 }, { 0x05, 0x34, 0x07, 0x11 }, { 0x06, 0x34, 0x08, 0x12 },
    { 0x07, 0x34, 0x09, 0x13 }, { 0x08, 0x34, 0x0a, 0x14 }, { 0x09, 0x34, 0x0b, 0x15 }, { 0x0a, 0x34, 0x32, 0x16 },
    { 0x2f, 0x01, 0x0d, 0x18 }, { 0x0c, 0x02, 0x0e, 0x19 }, { 0x0d, 0x03, 0x0f, 0x1a }, { 0x0e, 0x04, 0x10, 0x1b },
    { 0x0f, 0x05, 0x11, 0x1c }, { 0x10, 0x06, 0x12, 0x1d }, { 0x11, 0x07, 0x13, 0x1e }, { 0x12, 0x08, 0x14, 0x1f },
    { 0x13, 0x09, 0x15, 0x20 }, { 0x14, 0x0a, 0x16, 0x21 }, { 0x15, 0x0b, 0x17, 0x22 }, { 0x16, 0x32, 0x2f, 0x23 },
    { 0x30, 0x0c, 0x19, 0x24 }, { 0x18, 0x0d, 0x1a, 0x25 }, { 0x19, 0x0e, 0x1b, 0x26 }, { 0x1a, 0x0f, 0x1c, 0x27 },
    { 0x1b, 0x10, 0x1d, 0x28 }, { 0x1c, 0x11, 0x1e, 0x29 }, { 0x1d, 0x12, 0x1f, 0x2a }, { 0x1e, 0x13, 0x20, 0x2b },
    { 0x1f, 0x14, 0x21, 0x2c }, { 0x20, 0x15, 0x22, 0x2d }, { 0x21, 0x16, 0x23, 0x2e }, { 0x22, 0x17, 0x30, 0x2e },
    { 0x31, 0x18, 0x25, 0x33 }, { 0x24, 0x19, 0x26, 0x33 }, { 0x25, 0x1a, 0x27, 0x33 }, { 0x26, 0x1b, 0x28, 0x33 },
    { 0x27, 0x1c, 0x29, 0x33 }, { 0x28, 0x1d, 0x2a, 0x34 }, { 0x29, 0x1e, 0x2b, 0x34 }, { 0x2a, 0x1f, 0x2c, 0x34 },
    { 0x2b, 0x20, 0x2d, 0x34 }, { 0x2c, 0x21, 0x2e, 0x34 }, { 0x2d, -1, 0x31, 0x34 },   { 0x17, 0x00, 0x0c, 0x30 },
    { 0x23, 0x2f, 0x18, 0x31 }, { 0x2e, 0x30, 0x24, 0x33 }, { 0x0b, 0x34, 0x00, 0x17 }, { 0x34, -2, 0x34, -4 },
    { 0x33, -3, 0x33, -5 },
};

// The key codes and labels of the character keys in each mode: lower case, shifted and capitals
static const u8 *sKeyCodes[3] = {
    "1234567890-=qwertyuiop[]asdfghjkl;'`zxcvbnm,./\\",
    "!@#$%^&*()_+QWERTYUIOP{}ASDFGHJKL:\"~ZXCVBNM<>?|",
    "1234567890-=QWERTYUIOP[]ASDFGHJKL;'`ZXCVBNM,./\\",
};
static const u16 *sKeyLabels[3] = {
    L"1234567890-=qwertyuiop[]asdfghjkl;'`zxcvbnm,./\\ ",
    L"!@#$%^&*()_+QWERTYUIOP{}ASDFGHJKL:\"~ZXCVBNM<>?| ",
    L"1234567890-=QWERTYUIOP[]ASDFGHJKL;'`ZXCVBNM,./\\ ",
};
// The sound of each special key
static int sSpecialSounds[KEYBOARD_SPECIAL_KEYS] = { 0, 0, 0, 2 };

static DWCUtilKeyboard *sKeyboard;

static void func_ov182_021abddc(DWCUtilTask *task, void *arg);
static void func_ov182_021abe34(DWCUtilTask *task, void *arg);
static void func_ov182_021abe8c(DWCUtilTask *task, void *arg);
static void func_ov182_021abee8(DWCUtilTask *task, void *arg);
static void func_ov182_021abf44(DWCUtilTask *task, void *arg);
static void func_ov182_021abf9c(DWCUtilTask *task, void *arg);
static void func_ov182_021abfb0(void);
static void func_ov182_021ac0e8(void);
static void func_ov182_021ac2bc(void);
static void func_ov182_021ac2e4(void);
static void func_ov182_021ac30c(void);
static void func_ov182_021ac448(int key);
static void func_ov182_021ac478(void);
static void func_ov182_021ac608(int mode, int row, int y);
static void func_ov182_021ac7a0(int mode);
static void func_ov182_021ac808(int key, int pressed);
static void func_ov182_021ac898(void);
static void func_ov182_021ac928(int direction);
static void func_ov182_021acbac(DWCUtilTask *task, void *arg);
static void func_ov182_021acbf4(DWCUtilTask *task, void *arg);
static void func_ov182_021acc34(DWCUtilTask *task, void *arg);
static void func_ov182_021acc74(DWCUtilTask *task, void *arg);
static void func_ov182_021accb0(DWCUtilTask *task, void *arg);
static void func_ov182_021accec(DWCUtilTask *task, void *arg);

void func_ov182_021abb10(void) {
    u16 rect[4] = { 0, 0, 0, 0 };
    int objCount;
    u16 label[2];
    int i;
    int j;
    int n;
    int k;

    rect[2] = 0x11;
    rect[3] = 0x12;
    sKeyboard = func_ov182_021bc4d8(sizeof(DWCUtilKeyboard), 4);
    sKeyboard->key = 0xff;
    sKeyboard->cursorPos = 0;
    sKeyboard->backspaceEnabled = TRUE;
    sKeyboard->charsEnabled = TRUE;
    for (i = 0; i < KEYBOARD_CHAR_KEYS; i++) {
        sKeyboard->charOams[i] = func_ov182_021bb960(0, 0x34);
        G2_SetOBJEffect(sKeyboard->charOams[i], GX_OAM_EFFECT_NODISPLAY, 0);
        G2_SetOBJPriority(sKeyboard->charOams[i], 3);
    }
    for (i = 0; i < KEYBOARD_SPECIAL_KEYS; i++) {
        sKeyboard->specialOams[i] = func_ov182_021bb960(0, sSpecialCells[i]);
        G2_SetOBJEffect(sKeyboard->specialOams[i], GX_OAM_EFFECT_NODISPLAY, 0);
        G2_SetOBJPriority(sKeyboard->specialOams[i], 3);
    }
    for (i = 0; i < 2; i++) {
        sKeyboard->buttons[i] = func_ov182_021bb97c(0, sButtonCells[i], TRUE);
        func_ov182_021bb6f8(sKeyboard->buttons[i], -1, 0x200, 0);
        func_ov182_021bb878(sKeyboard->buttons[i], -1, 3);
    }
    label[1] = 0;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            DWCUtilText *text = func_ov182_021bbd3c(0, sLabelSize[0], sLabelSize[1], 0, &objCount, 0);

            sKeyboard->labels[i][j] = text;
            n = j * 12;
            rect[0] = 0;
            for (k = 0; k < 12; k++, n++) {
                label[0] = sKeyLabels[i][n];
                func_ov182_021bc0a4(sKeyboard->labels[i][j], rect[0], rect[1], rect[2], rect[3], 2, 0x480, label);
                rect[0] += 0x12;
            }
            if (i == 0) {
                sKeyboard->labelNodes[j] = func_ov182_021bb594(0, objCount, TRUE);
            }
        }
    }
    sKeyboard->cursor = func_ov182_021bb97c(0, 0x40, TRUE);
    func_ov182_021bb6f8(sKeyboard->cursor, -1, 0x200, 0);
    func_ov182_021bb878(sKeyboard->cursor, -1, 2);
    sKeyboard->task = func_ov182_021bd200(0, func_ov182_021abddc, NULL, 0x78);
    func_ov182_021ac608(sKeyboard->mode, 0, 0xc0);
}

void func_ov182_021abd6c(void) {
    func_ov182_021bb690(sKeyboard->cursor);
    func_ov182_021bd264(sKeyboard->task, func_ov182_021acbac);
}

u8 func_ov182_021abd94(void) {
    return sKeyboard->key;
}

void func_ov182_021abda4(u8 enabled) {
    sKeyboard->backspaceEnabled = enabled;
}

void func_ov182_021abdb8(u8 enabled) {
    sKeyboard->charsEnabled = enabled;
}

BOOL func_ov182_021abdc8(void) {
    if (sKeyboard != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void func_ov182_021abddc(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sKeyboard->charOams[0]).attr01 - 12;

    if (y > 0x51) {
        func_ov182_021ac608(sKeyboard->mode, 0, y);
        return;
    }
    func_ov182_021ac608(sKeyboard->mode, 0, 0x51);
    func_ov182_021ac608(sKeyboard->mode, 1, 0xc0);
    func_ov182_021bd264(task, func_ov182_021abe34);
}

static void func_ov182_021abe34(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sKeyboard->charOams[12]).attr01 - 12;

    if (y > 0x67) {
        func_ov182_021ac608(sKeyboard->mode, 1, y);
        return;
    }
    func_ov182_021ac608(sKeyboard->mode, 1, 0x67);
    func_ov182_021ac608(sKeyboard->mode, 2, 0xc0);
    func_ov182_021bd264(task, func_ov182_021abe8c);
}

static void func_ov182_021abe8c(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sKeyboard->charOams[24]).attr01 - 12;

    if (y > 0x7d) {
        func_ov182_021ac608(sKeyboard->mode, 2, y);
        return;
    }
    func_ov182_021ac608(sKeyboard->mode, 2, 0x7d);
    func_ov182_021ac608(sKeyboard->mode, 3, 0xc0);
    func_ov182_021bd264(task, func_ov182_021abee8);
}

static void func_ov182_021abee8(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sKeyboard->charOams[36]).attr01 - 12;

    if (y > 0x93) {
        func_ov182_021ac608(sKeyboard->mode, 3, y);
        return;
    }
    func_ov182_021ac608(sKeyboard->mode, 3, 0x93);
    func_ov182_021ac608(sKeyboard->mode, 4, 0xc0);
    func_ov182_021bd264(task, func_ov182_021abf44);
}

static void func_ov182_021abf44(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    func_ov182_021bb8bc(sKeyboard->buttons[0], 0, &x, &y);
    y -= 12;
    if (y > 0xaa) {
        func_ov182_021ac608(sKeyboard->mode, 4, y);
        return;
    }
    func_ov182_021ac608(sKeyboard->mode, 4, 0xaa);
    func_ov182_021ac898();
    func_ov182_021bd264(task, func_ov182_021abf9c);
}

static void func_ov182_021abf9c(DWCUtilTask *task, void *arg) {
    func_ov182_021abfb0();
    func_ov182_021ac0e8();
    func_ov182_021ac30c();
    func_ov182_021ac478();
}

static void func_ov182_021abfb0(void) {
    u16 rect[4];
    int i;

    if (func_ov182_021bc888(data_ov182_021c059c)) {
        sKeyboard->touched = -1;
        for (i = 0; i < KEYBOARD_CHAR_KEYS; i++) {
            func_ov182_021bc1c0(&sCharKeyPositions[i], sCharKeySize, rect);
            if (func_ov182_021bc888(rect)) {
                if (!sKeyboard->charsEnabled) {
                    func_ov182_021b80f4(9);
                    return;
                }
                func_ov182_021b80f4(0);
                sKeyboard->touched = i;
                return;
            }
        }
        for (i = 0; i < KEYBOARD_SPECIAL_KEYS; i++) {
            func_ov182_021bc1c0(&sSpecialPositions[i], &sSpecialSizes[i], rect);
            if (func_ov182_021bc888(rect)) {
                if ((i == 3 && !sKeyboard->backspaceEnabled) || (i == 2 && !sKeyboard->charsEnabled)) {
                    func_ov182_021b80f4(9);
                    return;
                }
                func_ov182_021b80f4(sSpecialSounds[i]);
                sKeyboard->touched = i + KEYBOARD_FIRST_SPECIAL;
                return;
            }
        }
        for (i = 0; i < 2; i++) {
            func_ov182_021bc1c0(&sButtonPositions[i], sButtonSize, rect);
            if (func_ov182_021bc888(rect)) {
                func_ov182_021b80f4(0);
                sKeyboard->touched = i + KEYBOARD_FIRST_BUTTON;
                return;
            }
        }
    }
}

static void func_ov182_021ac0e8(void) {
    u16 rect[4];
    int i;
    int j;

    i = 0;
    sKeyboard->key = 0;
    if (!func_ov182_021bc948(data_ov182_021c059c)) {
        return;
    }
    do {
        func_ov182_021bc1c0(&sCharKeyPositions[i], sCharKeySize, rect);
        if (func_ov182_021bc948(rect)) {
            if (sKeyboard->touched != i) {
                return;
            }
            sKeyboard->key = sKeyCodes[sKeyboard->mode][i];
            if (sKeyboard->mode == 1) {
                func_ov182_021ac7a0(0);
            }
            sKeyboard->cursorPos = i;
            func_ov182_021ac898();
            return;
        }
        i++;
    } while (i < KEYBOARD_CHAR_KEYS);
    j = 2;
    do {
        func_ov182_021bc1c0(&sSpecialPositions[j], &sSpecialSizes[j], rect);
        if (func_ov182_021bc948(rect)) {
            if (sKeyboard->touched != j + KEYBOARD_FIRST_SPECIAL) {
                return;
            }
            sKeyboard->key = sSpecialKeys[j];
            if (sKeyboard->mode == 1) {
                func_ov182_021ac7a0(0);
            }
            sKeyboard->cursorPos = j + KEYBOARD_FIRST_SPECIAL;
            func_ov182_021ac898();
            return;
        }
        j++;
    } while (j < KEYBOARD_SPECIAL_KEYS);
    j = 0;
    do {
        func_ov182_021bc1c0(&sButtonPositions[j], sButtonSize, rect);
        if (func_ov182_021bc948(rect)) {
            if (sKeyboard->touched != j + KEYBOARD_FIRST_BUTTON) {
                return;
            }
            sKeyboard->key = sButtonKeys[j];
            sKeyboard->cursorPos = j + KEYBOARD_FIRST_BUTTON;
            func_ov182_021ac898();
            return;
        }
        j++;
    } while (j < 2);
    func_ov182_021bc1c0(&sSpecialPositions[0], &sSpecialSizes[0], rect);
    if (func_ov182_021bc948(rect)) {
        if (sKeyboard->touched == KEYBOARD_FIRST_SPECIAL) {
            func_ov182_021ac2bc();
            sKeyboard->cursorPos = KEYBOARD_FIRST_SPECIAL;
            func_ov182_021ac898();
            return;
        }
    } else {
        func_ov182_021bc1c0(&sSpecialPositions[1], &sSpecialSizes[1], rect);
        if (func_ov182_021bc948(rect) && sKeyboard->touched == KEYBOARD_FIRST_SPECIAL + 1) {
            func_ov182_021ac2e4();
            sKeyboard->cursorPos = KEYBOARD_FIRST_SPECIAL + 1;
            func_ov182_021ac898();
        }
    }
}

static void func_ov182_021ac2bc(void) {
    func_ov182_021ac7a0(sKeyboard->mode == 2 ? 0 : 2);
    func_ov182_021b80f4(1);
}

static void func_ov182_021ac2e4(void) {
    func_ov182_021ac7a0(sKeyboard->mode == 1 ? 0 : 1);
    func_ov182_021b80f4(1);
}

static void func_ov182_021ac30c(void) {
    u16 rect[4];
    int i;
    int key;

    if (func_ov182_021bc828(data_ov182_021c059c)) {
        for (i = 0; i < KEYBOARD_CHAR_KEYS; i++) {
            func_ov182_021bc1c0(&sCharKeyPositions[i], sCharKeySize, rect);
            if (func_ov182_021bc828(rect)) {
                if (sKeyboard->touched != i) {
                    goto release;
                }
                key = i;
                goto press;
            }
        }
        for (i = 2; i < KEYBOARD_SPECIAL_KEYS; i++) {
            func_ov182_021bc1c0(&sSpecialPositions[i], &sSpecialSizes[i], rect);
            if (func_ov182_021bc828(rect)) {
                if (sKeyboard->touched == i + KEYBOARD_FIRST_SPECIAL) {
                    func_ov182_021ac448(i + KEYBOARD_FIRST_SPECIAL);
                    if (i != 3) {
                        return;
                    }
                    sKeyboard->repeat++;
                    if (sKeyboard->repeat < 0x28) {
                        return;
                    }
                    if (!sKeyboard->backspaceEnabled) {
                        func_ov182_021b80f4(9);
                        sKeyboard->touched = -1;
                        return;
                    }
                    sKeyboard->key = KEYBOARD_BACKSPACE;
                    sKeyboard->repeat -= 7;
                    return;
                }
                goto release;
            }
        }
        for (i = 0; i < 2; i++) {
            func_ov182_021bc1c0(&sButtonPositions[i], sButtonSize, rect);
            if (func_ov182_021bc828(rect)) {
                i += KEYBOARD_FIRST_BUTTON;
                if (sKeyboard->touched != i) {
                    break;
                }
                key = i;
                goto press;
            }
        }
    }
release:
    key = -1;
press:
    func_ov182_021ac448(key);
    sKeyboard->repeat = 0;
}

static void func_ov182_021ac448(int key) {
    if (key != sKeyboard->pressed) {
        func_ov182_021ac808(key, 1);
        func_ov182_021ac808(sKeyboard->pressed, 0);
        sKeyboard->pressed = key;
    }
}

// The pad: the cross keys move the cursor (left 0x20, up 0x40, right 0x10, down 0x80), A (1) presses its key, B (2)
// is backspace, and X (0x400) and Y (0x800) are the capitals and shift keys
static void func_ov182_021ac478(void) {
    int key;

    if (func_ov182_021bc7c8(0x20)) {
        func_ov182_021ac928(0);
    }
    if (func_ov182_021bc7c8(0x40)) {
        func_ov182_021ac928(1);
    }
    if (func_ov182_021bc7c8(0x10)) {
        func_ov182_021ac928(2);
    }
    if (func_ov182_021bc7c8(0x80)) {
        func_ov182_021ac928(3);
    }
    if (func_ov182_021bc798(1)) {
        key = sKeyboard->cursorPos;
        if (key < KEYBOARD_CHAR_KEYS) {
            if (!sKeyboard->charsEnabled) {
                func_ov182_021b80f4(9);
                return;
            }
            sKeyboard->key = sKeyCodes[sKeyboard->mode][key];
            if (sKeyboard->mode != 1) {
                return;
            }
            func_ov182_021ac7a0(0);
            return;
        }
        if (key - KEYBOARD_FIRST_SPECIAL < KEYBOARD_SPECIAL_KEYS) {
            switch (key - KEYBOARD_FIRST_SPECIAL) {
            case 0:
                func_ov182_021ac2bc();
                return;
            case 1:
                func_ov182_021ac2e4();
                return;
            case 2:
                if (!sKeyboard->charsEnabled) {
                    func_ov182_021b80f4(9);
                    return;
                }
                break;
            case 3:
                if (!sKeyboard->backspaceEnabled) {
                    func_ov182_021b80f4(9);
                    return;
                }
                break;
            }
            if (sKeyboard->mode == 1) {
                func_ov182_021ac7a0(0);
            }
            sKeyboard->key = sSpecialKeys[key - KEYBOARD_FIRST_SPECIAL];
            return;
        }
        sKeyboard->key = sButtonKeys[key - KEYBOARD_FIRST_BUTTON];
    }
    if (func_ov182_021bc7c8(2)) {
        if (!sKeyboard->backspaceEnabled) {
            if (sKeyboard->errorPlayed) {
                return;
            }
            func_ov182_021b80f4(9);
            sKeyboard->errorPlayed = TRUE;
            return;
        }
        sKeyboard->key = KEYBOARD_BACKSPACE;
    } else if (func_ov182_021bc7f8(2)) {
        sKeyboard->errorPlayed = FALSE;
    }
    if (func_ov182_021bc798(0x400)) {
        func_ov182_021ac2bc();
    }
    if (func_ov182_021bc798(0x800)) {
        func_ov182_021ac2e4();
    }
}

// Moves the row's keys, or with row 4 the two buttons, to y
static void func_ov182_021ac608(int mode, int row, int y) {
    const u8 charCounts[5] = { 12, 12, 12, 11, 0 };
    const u8 specialCounts[5] = { 1, 1, 1, 1, 0 };
    const u8 buttonCounts[5] = { 0, 0, 0, 0, 2 };
    int index;
    int i;
    int special;

    index = row * 12;
    for (i = 0; i < charCounts[row]; i++, index++) {
        G2_SetOBJEffect(sKeyboard->charOams[index], GX_OAM_EFFECT_NONE, 0);
        G2_SetOBJPosition(sKeyboard->charOams[index], sCharKeyPositions[index].x, y);
    }
    if (row < 4) {
        func_ov182_021bc124(sKeyboard->labels[mode][row], sCharKeyPositions[row * 12].x, y, sKeyboard->labelNodes[row],
                            2);
    }
    special = FX_ModS32(row + 3, 4);
    for (i = 0; i < specialCounts[row]; i++) {
        G2_SetOBJEffect(sKeyboard->specialOams[special], GX_OAM_EFFECT_NONE, 0);
        G2_SetOBJPosition(sKeyboard->specialOams[special], sSpecialPositions[special].x, y);
    }
    for (i = 0; i < buttonCounts[row]; i++) {
        func_ov182_021bb6f8(sKeyboard->buttons[i], -1, 0, 0);
        func_ov182_021bb7e0(sKeyboard->buttons[i], -1, sButtonPositions[i].x, y);
    }
}

static void func_ov182_021ac7a0(int mode) {
    int i;
    int capitals = FALSE;
    int shift = FALSE;

    sKeyboard->mode = mode;
    for (i = 0; i < 4; i++) {
        func_ov182_021ac608(mode, i, sCharKeyPositions[i * 12].y);
    }
    if (mode == 2) {
        capitals = TRUE;
    } else if (mode == 1) {
        shift = TRUE;
    }
    func_ov182_021ac808(KEYBOARD_FIRST_SPECIAL, capitals);
    func_ov182_021ac808(KEYBOARD_FIRST_SPECIAL + 1, shift);
}

static void func_ov182_021ac808(int key, int pressed) {
    if (key < 0) {
        return;
    }
    if (key < KEYBOARD_CHAR_KEYS) {
        G2_SetOBJMode(sKeyboard->charOams[key], GX_OAM_MODE_NORMAL, sKeyPalettes[pressed]);
    } else if (key - KEYBOARD_FIRST_SPECIAL < KEYBOARD_SPECIAL_KEYS) {
        G2_SetOBJMode(sKeyboard->specialOams[key - KEYBOARD_FIRST_SPECIAL], GX_OAM_MODE_NORMAL, sKeyPalettes[pressed]);
    } else {
        func_ov182_021bb778(sKeyboard->buttons[key - KEYBOARD_FIRST_BUTTON], -1, 0, sButtonPalettes[pressed]);
    }
}

static void func_ov182_021ac898(void) {
    int cell;

    switch (sKeyboard->cursorPos) {
    case KEYBOARD_FIRST_SPECIAL:
        cell = 0x42;
        break;
    case KEYBOARD_FIRST_SPECIAL + 1:
        cell = 0x41;
        break;
    case KEYBOARD_FIRST_SPECIAL + 2:
        cell = 0x43;
        break;
    case KEYBOARD_FIRST_SPECIAL + 3:
        cell = 0x41;
        break;
    case KEYBOARD_FIRST_BUTTON:
    case KEYBOARD_FIRST_BUTTON + 1:
        cell = 0x45;
        break;
    default:
        cell = 0x40;
        break;
    }
    func_ov182_021bb914(0, cell, func_ov182_021bb6ec(sKeyboard->cursor, 0));
    func_ov182_021bb878(sKeyboard->cursor, -1, 2);
    func_ov182_021bb7e0(sKeyboard->cursor, -1, sCursorPositions[sKeyboard->cursorPos].x,
                        sCursorPositions[sKeyboard->cursorPos].y);
}

static void func_ov182_021ac928(int direction) {
    s8 previous = sKeyboard->cursorPos;
    s8 next;

    sKeyboard->cursorPos = sNavigation[previous][direction];
    next = sKeyboard->cursorPos;
    if (next == 0x2e && direction == 3) {
        sKeyboard->lastPos = previous;
    } else if (next == 0x33 && (direction == 1 || direction == 3)) {
        sKeyboard->lastPos = previous;
    } else if (next == 0x34 && (direction == 1 || direction == 3)) {
        if (previous != 0x2e) {
            sKeyboard->lastPos = previous;
        }
    } else if (next == -1) {
        if (sKeyboard->lastPos == 0x23 || sKeyboard->lastPos == 0x32) {
            sKeyboard->cursorPos = 0x23;
        } else {
            sKeyboard->cursorPos = 0x22;
        }
    } else if (next == -2) {
        switch (sKeyboard->lastPos) {
        case 0:
        case 0x31:
            sKeyboard->cursorPos = 0x31;
            break;
        case 1:
        case 0x24:
            sKeyboard->cursorPos = 0x24;
            break;
        case 3:
        case 0x26:
            sKeyboard->cursorPos = 0x26;
            break;
        case 4:
        case 0x27:
            sKeyboard->cursorPos = 0x27;
            break;
        case 5:
        case 0x28:
            sKeyboard->cursorPos = 0x28;
            break;
        default:
            sKeyboard->cursorPos = 0x25;
            break;
        }
    } else if (next == -3) {
        switch (sKeyboard->lastPos) {
        case 6:
        case 0x29:
            sKeyboard->cursorPos = 0x29;
            break;
        case 7:
        case 0x2a:
            sKeyboard->cursorPos = 0x2a;
            break;
        case 9:
        case 0x2c:
            sKeyboard->cursorPos = 0x2c;
            break;
        case 10:
        case 0x2d:
            sKeyboard->cursorPos = 0x2d;
            break;
        case 11:
        case 0x22:
        case 0x23:
        case 0x32:
            sKeyboard->cursorPos = 0x2e;
            break;
        default:
            sKeyboard->cursorPos = 0x2b;
            break;
        }
    } else if (next == -4) {
        switch (sKeyboard->lastPos) {
        case 0:
        case 0x31:
            sKeyboard->cursorPos = 0;
            break;
        case 1:
        case 0x24:
            sKeyboard->cursorPos = 1;
            break;
        case 3:
        case 0x26:
            sKeyboard->cursorPos = 3;
            break;
        case 4:
        case 0x27:
            sKeyboard->cursorPos = 4;
            break;
        case 5:
        case 0x28:
            sKeyboard->cursorPos = 5;
            break;
        default:
            sKeyboard->cursorPos = 2;
            break;
        }
    } else if (next == -5) {
        switch (sKeyboard->lastPos) {
        case 6:
        case 0x29:
            sKeyboard->cursorPos = 6;
            break;
        case 7:
        case 0x2a:
            sKeyboard->cursorPos = 7;
            break;
        case 9:
        case 0x2c:
            sKeyboard->cursorPos = 9;
            break;
        case 10:
        case 0x2d:
            sKeyboard->cursorPos = 10;
            break;
        case 11:
        case 0x22:
            sKeyboard->cursorPos = 11;
            break;
        case 0x23:
        case 0x32:
            sKeyboard->cursorPos = 0x32;
            break;
        default:
            sKeyboard->cursorPos = 8;
            break;
        }
    }
    func_ov182_021ac898();
    func_ov182_021b80f4(8);
}

static void func_ov182_021acbac(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(func_ov182_021bb6ec(sKeyboard->buttons[0], 0)).attr01 + 12;

    func_ov182_021ac608(sKeyboard->mode, 4, y);
    if (y >= 0xc0) {
        func_ov182_021bd264(task, func_ov182_021acbf4);
    }
}

static void func_ov182_021acbf4(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sKeyboard->charOams[36]).attr01 + 12;

    func_ov182_021ac608(sKeyboard->mode, 3, y);
    if (y >= 0xc0) {
        func_ov182_021bd264(task, func_ov182_021acc34);
    }
}

static void func_ov182_021acc34(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sKeyboard->charOams[24]).attr01 + 12;

    func_ov182_021ac608(sKeyboard->mode, 2, y);
    if (y >= 0xc0) {
        func_ov182_021bd264(task, func_ov182_021acc74);
    }
}

static void func_ov182_021acc74(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sKeyboard->charOams[12]).attr01 + 12;

    func_ov182_021ac608(sKeyboard->mode, 1, y);
    if (y >= 0xc0) {
        func_ov182_021bd264(task, func_ov182_021accb0);
    }
}

static void func_ov182_021accb0(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sKeyboard->charOams[0]).attr01 + 12;

    func_ov182_021ac608(sKeyboard->mode, 0, y);
    if (y >= 0xc0) {
        func_ov182_021bd264(task, func_ov182_021accec);
    }
}

static void func_ov182_021accec(DWCUtilTask *task, void *arg) {
    int i;
    int j;

    func_ov182_021bd268(0, task);
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            func_ov182_021bbdcc(sKeyboard->labels[i][j]);
            if (i == 0) {
                func_ov182_021bb690(sKeyboard->labelNodes[j]);
            }
        }
    }
    for (i = 0; i < 2; i++) {
        func_ov182_021bb690(sKeyboard->buttons[i]);
    }
    for (i = 0; i < KEYBOARD_SPECIAL_KEYS; i++) {
        func_ov182_021bcc9c(sKeyboard->specialOams[i]);
    }
    for (i = 0; i < KEYBOARD_CHAR_KEYS; i++) {
        func_ov182_021bcc9c(sKeyboard->charOams[i]);
    }
    func_ov182_021bc4f0(&sKeyboard);
}
