#include "keyboard.h"
#include "cell.h"
#include "cellinfo.h"
#include "dwc_util_shared.h"
#include "font.h"
#include "foundation.h"
#include "heap.h"
#include "input.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "obj.h"
#include "sound.h"
#include "task.h"

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

// The tables are defined in the order that gives the ROM's layout of MWCC's size sort, the special keys' positions and
// sizes just above the touch's start and the navigation above the cursor's move
static const u8 sButtonPalettes[2] = { 4, 5 };
static const u8 sButtonCells[2] = { 0x3c, 0x3d };
static const u8 sKeyPalettes[2] = { 2, 3 };
static const u8 sButtonKeys[3] = { 0x82, 0x83, 0 };
static const u16 sCharKeySize[2] = { 0x11, 0x12 };
static const u16 sLabelSize[2] = { 0x1c, 2 };
static const u16 sButtonSize[2] = { 0x78, 0x12 };
static const u8 sSpecialCells[KEYBOARD_SPECIAL_KEYS] = { 0x3a, 0x3b, 0x35, 0x39 };
// The codes of the special keys (0xff for the capitals and shift keys), and a 0 after them
static const u8 sSpecialKeys[KEYBOARD_SPECIAL_KEYS + 1] = { 0xff, 0xff, ' ', KEYBOARD_BACKSPACE, 0 };
static const DWCUtilKeyboardPoint sButtonPositions[2] = { { 0x04, 0xaa }, { 0x84, 0xaa } };
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

static void taskStart0(DWCUtilTask *task, void *arg);
static void taskStart1(DWCUtilTask *task, void *arg);
static void taskStart2(DWCUtilTask *task, void *arg);
static void taskStart3(DWCUtilTask *task, void *arg);
static void taskStart4(DWCUtilTask *task, void *arg);
static void taskMain(DWCUtilTask *task, void *arg);
static void touchTriger(void);
static void touchRelease(void);
static void pushCaps(void);
static void pushShift(void);
static void touch(void);
static void exchangeBack(int key);
static void key(void);
static void disp(int mode, int row, int y);
static void changePage(int mode);
static void changeColor(int key, int pressed);
static void dispFocus(void);
static void moveFocus(int direction);
static void taskEnd0(DWCUtilTask *task, void *arg);
static void taskEnd1(DWCUtilTask *task, void *arg);
static void taskEnd2(DWCUtilTask *task, void *arg);
static void taskEnd3(DWCUtilTask *task, void *arg);
static void taskEnd4(DWCUtilTask *task, void *arg);
static void taskEnd5(DWCUtilTask *task, void *arg);

void DWCi_KBlInit(void) {
    u16 rect[4] = { 0, 0, 0, 0 };
    int objCount;
    u16 label[2];
    int i;
    int j;
    int n;
    int k;

    rect[2] = 0x11;
    rect[3] = 0x12;
    sKeyboard = DWCi_HEAPlAllocEx(sizeof(DWCUtilKeyboard), 4);
    sKeyboard->key = 0xff;
    sKeyboard->cursorPos = 0;
    sKeyboard->backspaceEnabled = TRUE;
    sKeyboard->charsEnabled = TRUE;
    for (i = 0; i < KEYBOARD_CHAR_KEYS; i++) {
        sKeyboard->charOams[i] = DWCi_CEINlSetExObj(0, 0x34);
        G2_SetOBJEffect(sKeyboard->charOams[i], GX_OAM_EFFECT_NODISPLAY, 0);
        G2_SetOBJPriority(sKeyboard->charOams[i], 3);
    }
    for (i = 0; i < KEYBOARD_SPECIAL_KEYS; i++) {
        sKeyboard->specialOams[i] = DWCi_CEINlSetExObj(0, sSpecialCells[i]);
        G2_SetOBJEffect(sKeyboard->specialOams[i], GX_OAM_EFFECT_NODISPLAY, 0);
        G2_SetOBJPriority(sKeyboard->specialOams[i], 3);
    }
    for (i = 0; i < 2; i++) {
        sKeyboard->buttons[i] = DWCi_CEINlSetExCell(0, sButtonCells[i], TRUE);
        DWCi_CELLlSetEffect(sKeyboard->buttons[i], -1, 0x200, 0);
        DWCi_CELLlSetPriority(sKeyboard->buttons[i], -1, 3);
    }
    label[1] = 0;
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            DWCUtilText *text = DWCi_FNTlFormEx(0, sLabelSize[0], sLabelSize[1], 0, &objCount, 0);

            sKeyboard->labels[i][j] = text;
            n = j * 12;
            rect[0] = 0;
            for (k = 0; k < 12; k++, n++) {
                label[0] = sKeyLabels[i][n];
                DWCi_FNTlDrawRect(sKeyboard->labels[i][j], rect[0], rect[1], rect[2], rect[3], 2, 0x480, label);
                rect[0] += 0x12;
            }
            if (i == 0) {
                sKeyboard->labelNodes[j] = DWCi_CELLlForm(0, objCount, TRUE);
            }
        }
    }
    sKeyboard->cursor = DWCi_CEINlSetExCell(0, 0x40, TRUE);
    DWCi_CELLlSetEffect(sKeyboard->cursor, -1, 0x200, 0);
    DWCi_CELLlSetPriority(sKeyboard->cursor, -1, 2);
    sKeyboard->task = DWCi_TSKlForm(0, taskStart0, NULL, 0x78);
    disp(sKeyboard->mode, 0, 0xc0);
}

void DWCi_KBlEnd(void) {
    DWCi_CELLlDelete(sKeyboard->cursor);
    DWCi_TSKlChangeFunc(sKeyboard->task, taskEnd0);
}

u8 DWCi_KBlGet(void) {
    return sKeyboard->key;
}

void DWCi_KBlSetErase(u8 enabled) {
    sKeyboard->backspaceEnabled = enabled;
}

void DWCi_KBlSetInput(u8 enabled) {
    sKeyboard->charsEnabled = enabled;
}

BOOL DWCi_KBlIsInit(void) {
    if (sKeyboard != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void taskStart0(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    G2_GetOBJPosition(sKeyboard->charOams[0], (u32 *)&x, (u32 *)&y);
    y -= 12;
    if (y > 0x51) {
        disp(sKeyboard->mode, 0, y);
        return;
    }
    disp(sKeyboard->mode, 0, 0x51);
    disp(sKeyboard->mode, 1, 0xc0);
    DWCi_TSKlChangeFunc(task, taskStart1);
}

static void taskStart1(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    G2_GetOBJPosition(sKeyboard->charOams[12], (u32 *)&x, (u32 *)&y);
    y -= 12;
    if (y > 0x67) {
        disp(sKeyboard->mode, 1, y);
        return;
    }
    disp(sKeyboard->mode, 1, 0x67);
    disp(sKeyboard->mode, 2, 0xc0);
    DWCi_TSKlChangeFunc(task, taskStart2);
}

static void taskStart2(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    G2_GetOBJPosition(sKeyboard->charOams[24], (u32 *)&x, (u32 *)&y);
    y -= 12;
    if (y > 0x7d) {
        disp(sKeyboard->mode, 2, y);
        return;
    }
    disp(sKeyboard->mode, 2, 0x7d);
    disp(sKeyboard->mode, 3, 0xc0);
    DWCi_TSKlChangeFunc(task, taskStart3);
}

static void taskStart3(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    G2_GetOBJPosition(sKeyboard->charOams[36], (u32 *)&x, (u32 *)&y);
    y -= 12;
    if (y > 0x93) {
        disp(sKeyboard->mode, 3, y);
        return;
    }
    disp(sKeyboard->mode, 3, 0x93);
    disp(sKeyboard->mode, 4, 0xc0);
    DWCi_TSKlChangeFunc(task, taskStart4);
}

static void taskStart4(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    DWCi_CELLlGetPosition(sKeyboard->buttons[0], 0, &x, &y);
    y -= 12;
    if (y > 0xaa) {
        disp(sKeyboard->mode, 4, y);
        return;
    }
    disp(sKeyboard->mode, 4, 0xaa);
    dispFocus();
    DWCi_TSKlChangeFunc(task, taskMain);
}

static void taskMain(DWCUtilTask *task, void *arg) {
    touchTriger();
    touchRelease();
    touch();
    key();
}

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

static void touchTriger(void) {
    u16 rect[4];
    int i;

    if (DWCi_TPlCheckTrigger(gDWCUtilTouchScreenRect)) {
        sKeyboard->touched = -1;
        for (i = 0; i < KEYBOARD_CHAR_KEYS; i++) {
            DWCi_Coord2Rect(&sCharKeyPositions[i], sCharKeySize, rect);
            if (DWCi_TPlCheckTrigger(rect)) {
                if (!sKeyboard->charsEnabled) {
                    DWCi_SNDlPlay(9);
                    return;
                }
                DWCi_SNDlPlay(0);
                sKeyboard->touched = i;
                return;
            }
        }
        for (i = 0; i < KEYBOARD_SPECIAL_KEYS; i++) {
            DWCi_Coord2Rect(&sSpecialPositions[i], &sSpecialSizes[i], rect);
            if (DWCi_TPlCheckTrigger(rect)) {
                if ((i == 3 && !sKeyboard->backspaceEnabled) || (i == 2 && !sKeyboard->charsEnabled)) {
                    DWCi_SNDlPlay(9);
                    return;
                }
                DWCi_SNDlPlay(sSpecialSounds[i]);
                sKeyboard->touched = i + KEYBOARD_FIRST_SPECIAL;
                return;
            }
        }
        for (i = 0; i < 2; i++) {
            DWCi_Coord2Rect(&sButtonPositions[i], sButtonSize, rect);
            if (DWCi_TPlCheckTrigger(rect)) {
                DWCi_SNDlPlay(0);
                sKeyboard->touched = i + KEYBOARD_FIRST_BUTTON;
                return;
            }
        }
    }
}

static void touchRelease(void) {
    u16 rect[4];
    int i;
    int j;

    i = 0;
    sKeyboard->key = 0;
    if (!DWCi_TPlCheckRelease(gDWCUtilTouchScreenRect)) {
        return;
    }
    do {
        DWCi_Coord2Rect(&sCharKeyPositions[i], sCharKeySize, rect);
        if (DWCi_TPlCheckRelease(rect)) {
            if (sKeyboard->touched != i) {
                return;
            }
            sKeyboard->key = sKeyCodes[sKeyboard->mode][i];
            if (sKeyboard->mode == 1) {
                changePage(0);
            }
            sKeyboard->cursorPos = i;
            dispFocus();
            return;
        }
        i++;
    } while (i < KEYBOARD_CHAR_KEYS);
    j = 2;
    do {
        DWCi_Coord2Rect(&sSpecialPositions[j], &sSpecialSizes[j], rect);
        if (DWCi_TPlCheckRelease(rect)) {
            if (sKeyboard->touched != j + KEYBOARD_FIRST_SPECIAL) {
                return;
            }
            sKeyboard->key = sSpecialKeys[j];
            if (sKeyboard->mode == 1) {
                changePage(0);
            }
            sKeyboard->cursorPos = j + KEYBOARD_FIRST_SPECIAL;
            dispFocus();
            return;
        }
        j++;
    } while (j < KEYBOARD_SPECIAL_KEYS);
    j = 0;
    do {
        DWCi_Coord2Rect(&sButtonPositions[j], sButtonSize, rect);
        if (DWCi_TPlCheckRelease(rect)) {
            if (sKeyboard->touched != j + KEYBOARD_FIRST_BUTTON) {
                return;
            }
            sKeyboard->key = sButtonKeys[j];
            sKeyboard->cursorPos = j + KEYBOARD_FIRST_BUTTON;
            dispFocus();
            return;
        }
        j++;
    } while (j < 2);
    DWCi_Coord2Rect(&sSpecialPositions[0], &sSpecialSizes[0], rect);
    if (DWCi_TPlCheckRelease(rect)) {
        if (sKeyboard->touched == KEYBOARD_FIRST_SPECIAL) {
            pushCaps();
            sKeyboard->cursorPos = KEYBOARD_FIRST_SPECIAL;
            dispFocus();
            return;
        }
    } else {
        DWCi_Coord2Rect(&sSpecialPositions[1], &sSpecialSizes[1], rect);
        if (DWCi_TPlCheckRelease(rect) && sKeyboard->touched == KEYBOARD_FIRST_SPECIAL + 1) {
            pushShift();
            sKeyboard->cursorPos = KEYBOARD_FIRST_SPECIAL + 1;
            dispFocus();
        }
    }
}

static void pushCaps(void) {
    changePage(sKeyboard->mode == 2 ? 0 : 2);
    DWCi_SNDlPlay(1);
}

static void pushShift(void) {
    changePage(sKeyboard->mode == 1 ? 0 : 1);
    DWCi_SNDlPlay(1);
}

static void touch(void) {
    u16 rect[4];
    int i;

    if (DWCi_TPlCheck(gDWCUtilTouchScreenRect)) {
        for (i = 0; i < KEYBOARD_CHAR_KEYS; i++) {
            DWCi_Coord2Rect(&sCharKeyPositions[i], sCharKeySize, rect);
            if (DWCi_TPlCheck(rect)) {
                if (sKeyboard->touched != i) {
                    goto release;
                }
                goto held;
            }
        }
        for (i = 2; i < KEYBOARD_SPECIAL_KEYS; i++) {
            DWCi_Coord2Rect(&sSpecialPositions[i], &sSpecialSizes[i], rect);
            if (DWCi_TPlCheck(rect)) {
                if (sKeyboard->touched == i + KEYBOARD_FIRST_SPECIAL) {
                    exchangeBack(i + KEYBOARD_FIRST_SPECIAL);
                    if (i != 3) {
                        return;
                    }
                    sKeyboard->repeat++;
                    if (sKeyboard->repeat < 0x28) {
                        return;
                    }
                    if (!sKeyboard->backspaceEnabled) {
                        DWCi_SNDlPlay(9);
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
            DWCi_Coord2Rect(&sButtonPositions[i], sButtonSize, rect);
            if (DWCi_TPlCheck(rect)) {
                i += KEYBOARD_FIRST_BUTTON;
                if (sKeyboard->touched == i) {
                held:
                    exchangeBack(i);
                    goto clear;
                }
                break;
            }
        }
    }
release:
    exchangeBack(-1);
clear:
    sKeyboard->repeat = 0;
}

static void exchangeBack(int key) {
    if (key != sKeyboard->pressed) {
        changeColor(key, 1);
        changeColor(sKeyboard->pressed, 0);
        sKeyboard->pressed = key;
    }
}

// The pad: the cross keys move the cursor (left 0x20, up 0x40, right 0x10, down 0x80), A (1) presses its key, B (2)
// is backspace, and X (0x400) and Y (0x800) are the capitals and shift keys
static void key(void) {
    int key;

    if (DWCi_IPTlCheckRepeat(0x20)) {
        moveFocus(0);
    }
    if (DWCi_IPTlCheckRepeat(0x40)) {
        moveFocus(1);
    }
    if (DWCi_IPTlCheckRepeat(0x10)) {
        moveFocus(2);
    }
    if (DWCi_IPTlCheckRepeat(0x80)) {
        moveFocus(3);
    }
    if (DWCi_IPTlCheckTrigger(1)) {
        key = sKeyboard->cursorPos;
        if (key < KEYBOARD_CHAR_KEYS) {
            if (!sKeyboard->charsEnabled) {
                DWCi_SNDlPlay(9);
                return;
            }
            sKeyboard->key = sKeyCodes[sKeyboard->mode][key];
            if (sKeyboard->mode != 1) {
                return;
            }
            changePage(0);
            return;
        }
        if (key - KEYBOARD_FIRST_SPECIAL < KEYBOARD_SPECIAL_KEYS) {
            switch (key - KEYBOARD_FIRST_SPECIAL) {
            case 0:
                pushCaps();
                return;
            case 1:
                pushShift();
                return;
            case 2:
                if (!sKeyboard->charsEnabled) {
                    DWCi_SNDlPlay(9);
                    return;
                }
                break;
            case 3:
                if (!sKeyboard->backspaceEnabled) {
                    DWCi_SNDlPlay(9);
                    return;
                }
                break;
            }
            if (sKeyboard->mode == 1) {
                changePage(0);
            }
            sKeyboard->key = sSpecialKeys[key - KEYBOARD_FIRST_SPECIAL];
            return;
        }
        sKeyboard->key = sButtonKeys[key - KEYBOARD_FIRST_BUTTON];
    }
    if (DWCi_IPTlCheckRepeat(2)) {
        if (!sKeyboard->backspaceEnabled) {
            if (sKeyboard->errorPlayed) {
                return;
            }
            DWCi_SNDlPlay(9);
            sKeyboard->errorPlayed = TRUE;
            return;
        }
        sKeyboard->key = KEYBOARD_BACKSPACE;
    } else if (DWCi_IPTlCheckRelease(2)) {
        sKeyboard->errorPlayed = FALSE;
    }
    if (DWCi_IPTlCheckTrigger(0x400)) {
        pushCaps();
    }
    if (DWCi_IPTlCheckTrigger(0x800)) {
        pushShift();
    }
}

// Moves the row's keys, or with row 4 the two buttons, to y
static void disp(int mode, int row, int y) {
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
        DWCi_FNTlDisp(sKeyboard->labels[mode][row], sCharKeyPositions[row * 12].x, y, sKeyboard->labelNodes[row], 2);
    }
    special = FX_ModS32(row + 3, 4);
    for (i = 0; i < specialCounts[row]; i++) {
        G2_SetOBJEffect(sKeyboard->specialOams[special], GX_OAM_EFFECT_NONE, 0);
        G2_SetOBJPosition(sKeyboard->specialOams[special], sSpecialPositions[special].x, y);
    }
    for (i = 0; i < buttonCounts[row]; i++) {
        DWCi_CELLlSetEffect(sKeyboard->buttons[i], -1, 0, 0);
        DWCi_CELLlSetPosition(sKeyboard->buttons[i], -1, sButtonPositions[i].x, y);
    }
}

static void changePage(int mode) {
    int i;
    int capitals = FALSE;
    int shift = FALSE;

    sKeyboard->mode = mode;
    for (i = 0; i < 4; i++) {
        disp(mode, i, sCharKeyPositions[i * 12].y);
    }
    if (mode == 2) {
        capitals = TRUE;
    } else if (mode == 1) {
        shift = TRUE;
    }
    changeColor(KEYBOARD_FIRST_SPECIAL, capitals);
    changeColor(KEYBOARD_FIRST_SPECIAL + 1, shift);
}

static void changeColor(int key, int pressed) {
    if (key < 0) {
        return;
    }
    if (key < KEYBOARD_CHAR_KEYS) {
        G2_SetOBJMode(sKeyboard->charOams[key], GX_OAM_MODE_NORMAL, sKeyPalettes[pressed]);
    } else if (key - KEYBOARD_FIRST_SPECIAL < KEYBOARD_SPECIAL_KEYS) {
        G2_SetOBJMode(sKeyboard->specialOams[key - KEYBOARD_FIRST_SPECIAL], GX_OAM_MODE_NORMAL, sKeyPalettes[pressed]);
    } else {
        DWCi_CELLlSetPalette(sKeyboard->buttons[key - KEYBOARD_FIRST_BUTTON], -1, 0, sButtonPalettes[pressed]);
    }
}

static void dispFocus(void) {
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
    DWCi_CEINlSet(0, cell, DWCi_CELLlGetObj(sKeyboard->cursor, 0));
    DWCi_CELLlSetPriority(sKeyboard->cursor, -1, 2);
    DWCi_CELLlSetPosition(sKeyboard->cursor, -1, sCursorPositions[sKeyboard->cursorPos].x,
                          sCursorPositions[sKeyboard->cursorPos].y);
}

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

static void moveFocus(int direction) {
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
    dispFocus();
    DWCi_SNDlPlay(8);
}

static void taskEnd0(DWCUtilTask *task, void *arg) {
    GXOamAttr *oam;
    int x;
    int y;

    oam = DWCi_CELLlGetObj(sKeyboard->buttons[0], 0);
    G2_GetOBJPosition(oam, (u32 *)&x, (u32 *)&y);
    y += 12;
    disp(sKeyboard->mode, 4, y);
    if (y >= 0xc0) {
        DWCi_TSKlChangeFunc(task, taskEnd1);
    }
}

static void taskEnd1(DWCUtilTask *task, void *arg) {
    GXOamAttr *oam;
    int x;
    int y;

    oam = sKeyboard->charOams[36];
    G2_GetOBJPosition(oam, (u32 *)&x, (u32 *)&y);
    y += 12;
    disp(sKeyboard->mode, 3, y);
    if (y >= 0xc0) {
        DWCi_TSKlChangeFunc(task, taskEnd2);
    }
}

static void taskEnd2(DWCUtilTask *task, void *arg) {
    GXOamAttr *oam;
    int x;
    int y;

    oam = sKeyboard->charOams[24];
    G2_GetOBJPosition(oam, (u32 *)&x, (u32 *)&y);
    y += 12;
    disp(sKeyboard->mode, 2, y);
    if (y >= 0xc0) {
        DWCi_TSKlChangeFunc(task, taskEnd3);
    }
}

static void taskEnd3(DWCUtilTask *task, void *arg) {
    GXOamAttr *oam;
    int x;
    int y;

    oam = sKeyboard->charOams[12];
    G2_GetOBJPosition(oam, (u32 *)&x, (u32 *)&y);
    y += 12;
    disp(sKeyboard->mode, 1, y);
    if (y >= 0xc0) {
        DWCi_TSKlChangeFunc(task, taskEnd4);
    }
}

static void taskEnd4(DWCUtilTask *task, void *arg) {
    GXOamAttr *oam;
    int x;
    int y;

    oam = sKeyboard->charOams[0];
    G2_GetOBJPosition(oam, (u32 *)&x, (u32 *)&y);
    y += 12;
    disp(sKeyboard->mode, 0, y);
    if (y >= 0xc0) {
        DWCi_TSKlChangeFunc(task, taskEnd5);
    }
}

static void taskEnd5(DWCUtilTask *task, void *arg) {
    int i;
    int j;

    DWCi_TSKlDelete(0, task);
    for (i = 0; i < 3; i++) {
        for (j = 0; j < 4; j++) {
            DWCi_FNTlDelete(sKeyboard->labels[i][j]);
            if (i == 0) {
                DWCi_CELLlDelete(sKeyboard->labelNodes[j]);
            }
        }
    }
    for (i = 0; i < 2; i++) {
        DWCi_CELLlDelete(sKeyboard->buttons[i]);
    }
    for (i = 0; i < KEYBOARD_SPECIAL_KEYS; i++) {
        DWCi_OBJlDelete(sKeyboard->specialOams[i]);
    }
    for (i = 0; i < KEYBOARD_CHAR_KEYS; i++) {
        DWCi_OBJlDelete(sKeyboard->charOams[i]);
    }
    DWCi_HEAPlFree_(&sKeyboard);
}
