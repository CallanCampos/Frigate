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

static void DWCUtilMenu_SlideInRow0Task(DWCUtilTask *task, void *arg);
static void DWCUtilMenu_SlideInRow1Task(DWCUtilTask *task, void *arg);
static void DWCUtilMenu_SlideInRow2Task(DWCUtilTask *task, void *arg);
static void DWCUtilMenu_SlideInRow3Task(DWCUtilTask *task, void *arg);
static void DWCUtilMenu_SlideInButtonsTask(DWCUtilTask *task, void *arg);
static void DWCUtilMenu_UpdateTask(DWCUtilTask *task, void *arg);
static void DWCUtilMenu_HandleTouchStart(void);
static void DWCUtilMenu_HandleTouchEnd(void);
static void DWCUtilMenu_HandleTouchHold(void);
static void DWCUtilMenu_SetPressedKey(int key);
static void DWCUtilMenu_HandlePad(void);
static void DWCUtilMenu_MoveRow(int row, u32 y);
static void DWCUtilMenu_DrawKey(int key, int pressed);
static void DWCUtilMenu_DrawCursor(void);
static void DWCUtilMenu_MoveCursor(int direction);
static void DWCUtilMenu_SlideOutButtonsTask(DWCUtilTask *task, void *arg);
static void DWCUtilMenu_SlideOutRow3Task(DWCUtilTask *task, void *arg);
static void DWCUtilMenu_SlideOutRow2Task(DWCUtilTask *task, void *arg);
static void DWCUtilMenu_SlideOutRow1Task(DWCUtilTask *task, void *arg);
static void DWCUtilMenu_SlideOutRow0Task(DWCUtilTask *task, void *arg);
static void DWCUtilMenu_DestroyTask(DWCUtilTask *task, void *arg);

void DWCUtilMenu_Open(void) {
    DWCUtilMenuRect rect = { 0, 1, 0, 0 };
    int objCount;
    u16 label[2];
    int i;
    int k;
    int j;

    rect.width = 0x1c;
    rect.height = 0x14;
    sMenu = DWCUtilHeap_AllocClear(sizeof(DWCUtilMenu), 4);
    sMenu->code = 0x1f;
    sMenu->cursorPos = 0;
    sMenu->leftEnabled = TRUE;
    sMenu->digitsEnabled = TRUE;
    sMenu->rightEnabled = TRUE;
    for (i = 0; i < 10; i++) {
        sMenu->keyOams[i] = DWCUtilCell_AllocOam(0, 0x36);
        G2_SetOBJEffect(sMenu->keyOams[i], GX_OAM_EFFECT_NODISPLAY, 0);
        G2_SetOBJPriority(sMenu->keyOams[i], 3);
    }
    for (i = 0; i < 2; i++) {
        sMenu->arrowOams[i] = DWCUtilCell_AllocOam(0, sArrowCells[i]);
        G2_SetOBJEffect(sMenu->arrowOams[i], GX_OAM_EFFECT_NODISPLAY, 0);
        G2_SetOBJPriority(sMenu->arrowOams[i], 3);
    }
    for (i = 0; i < 2; i++) {
        sMenu->buttons[i] = DWCUtilCell_AllocOamRun(0, sButtonCells[i], TRUE);
        DWCUtilOam_SetEffect(sMenu->buttons[i], -1, GX_OAM_EFFECT_NODISPLAY, 0);
        DWCUtilOam_SetPriority(sMenu->buttons[i], -1, 3);
    }
    label[1] = 0;
    for (i = 0; i < 4; i++) {
        DWCUtilText *text = DWCUtilText_CreateObj(0, 0xc, 4, 0, &objCount, 0);

        sMenu->rows[i] = text;
        rect.x = 0;
        for (j = 0, k = i * 3; j < 3; j++, k++) {
            label[0] = sLabels[k];
            DWCUtilText_DrawRect(sMenu->rows[i], rect.x, rect.y, rect.width, rect.height, 2, 0x480, label);
            rect.x += 0x20;
        }
        sMenu->rowNodes[i] = DWCUtilOam_Alloc(0, objCount, FALSE);
    }
    sMenu->cursor = DWCUtilCell_AllocOamRun(0, 0x44, TRUE);
    DWCUtilOam_SetEffect(sMenu->cursor, -1, GX_OAM_EFFECT_NODISPLAY, 0);
    DWCUtilOam_SetPriority(sMenu->cursor, -1, 2);
    sMenu->task = DWCUtilTask_Create(0, DWCUtilMenu_SlideInRow0Task, NULL, 0x78);
    DWCUtilMenu_MoveRow(0, 0xc0);
}

void DWCUtilMenu_Close(void) {
    DWCUtilOam_Free(sMenu->cursor);
    DWCUtilTask_SetCallback(sMenu->task, DWCUtilMenu_SlideOutButtonsTask);
}

u8 DWCUtilMenu_GetKey(void) {
    return sMenu->code;
}

void DWCUtilMenu_SetLeftEnabled(u8 enabled) {
    sMenu->leftEnabled = enabled;
}

void DWCUtilMenu_SetDigitsEnabled(u8 enabled) {
    sMenu->digitsEnabled = enabled;
}

void DWCUtilMenu_SetRightEnabled(u8 enabled) {
    sMenu->rightEnabled = enabled;
}

BOOL DWCUtilMenu_IsOpen(void) {
    if (sMenu != NULL) {
        return TRUE;
    }
    return FALSE;
}

static void DWCUtilMenu_SlideInRow0Task(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[0]).attr01 - 12;

    if (y > 0x4c) {
        DWCUtilMenu_MoveRow(0, y);
        return;
    }
    DWCUtilMenu_MoveRow(0, 0x4c);
    DWCUtilMenu_MoveRow(1, 0xc0);
    DWCUtilTask_SetCallback(task, DWCUtilMenu_SlideInRow1Task);
}

static void DWCUtilMenu_SlideInRow1Task(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[3]).attr01 - 12;

    if (y > 0x63) {
        DWCUtilMenu_MoveRow(1, y);
        return;
    }
    DWCUtilMenu_MoveRow(1, 0x63);
    DWCUtilMenu_MoveRow(2, 0xc0);
    DWCUtilTask_SetCallback(task, DWCUtilMenu_SlideInRow2Task);
}

static void DWCUtilMenu_SlideInRow2Task(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[6]).attr01 - 12;

    if (y > 0x7a) {
        DWCUtilMenu_MoveRow(2, y);
        return;
    }
    DWCUtilMenu_MoveRow(2, 0x7a);
    DWCUtilMenu_MoveRow(3, 0xc0);
    DWCUtilTask_SetCallback(task, DWCUtilMenu_SlideInRow3Task);
}

static void DWCUtilMenu_SlideInRow3Task(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[9]).attr01 - 12;

    if (y > 0x91) {
        DWCUtilMenu_MoveRow(3, y);
        return;
    }
    DWCUtilMenu_MoveRow(3, 0x91);
    DWCUtilMenu_MoveRow(4, 0xc0);
    DWCUtilTask_SetCallback(task, DWCUtilMenu_SlideInButtonsTask);
}

static void DWCUtilMenu_SlideInButtonsTask(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    DWCUtilOam_GetNodePosition(sMenu->buttons[0], 0, &x, &y);
    y -= 12;
    if (y > 0xaa) {
        DWCUtilMenu_MoveRow(4, y);
        return;
    }
    DWCUtilMenu_MoveRow(4, 0xaa);
    DWCUtilMenu_DrawCursor();
    DWCUtilTask_SetCallback(task, DWCUtilMenu_UpdateTask);
}

static void DWCUtilMenu_UpdateTask(DWCUtilTask *task, void *arg) {
    DWCUtilMenu_HandleTouchStart();
    DWCUtilMenu_HandleTouchEnd();
    DWCUtilMenu_HandleTouchHold();
    DWCUtilMenu_HandlePad();
}

// The start of a touch: which key it is on
static void DWCUtilMenu_HandleTouchStart(void) {
    u16 rect[4];
    int i;

    if (DWCUtilTouch_IsTriggeredIn(gDWCUtilTouchScreenRect)) {
        sMenu->touched = -1;
        for (i = 0; i < 10; i++) {
            DWCUtilDisplay_SetRectFromSize(&sKeyPositions[i], sKeySize, rect);
            if (DWCUtilTouch_IsTriggeredIn(rect)) {
                if (!sMenu->digitsEnabled) {
                    DWCUtilSound_PlaySe(9);
                    return;
                }
                DWCUtilSound_PlaySe(0);
                sMenu->touched = i;
                return;
            }
        }
        for (i = 0; i < 2; i++) {
            DWCUtilDisplay_SetRectFromSize(&sArrowPositions[i], sArrowSize, rect);
            if (DWCUtilTouch_IsTriggeredIn(rect)) {
                if ((i == 0 && !sMenu->leftEnabled) || (i == 1 && !sMenu->rightEnabled)) {
                    DWCUtilSound_PlaySe(9);
                    return;
                }
                DWCUtilSound_PlaySe(0);
                sMenu->touched = i + 10;
                return;
            }
        }
        for (i = 0; i < 2; i++) {
            DWCUtilDisplay_SetRectFromSize(&sButtonPositions[i], sButtonSize, rect);
            if (DWCUtilTouch_IsTriggeredIn(rect)) {
                DWCUtilSound_PlaySe(0);
                sMenu->touched = i + 12;
                return;
            }
        }
    }
}

// The end of a touch: the key it chooses, if it ends on the key it started on
static void DWCUtilMenu_HandleTouchEnd(void) {
    u16 rect[4];
    int i;

    sMenu->code = 0;
    if (DWCUtilTouch_IsReleasedIn(gDWCUtilTouchScreenRect)) {
        for (i = 0; i < 10; i++) {
            DWCUtilDisplay_SetRectFromSize(&sKeyPositions[i], sKeySize, rect);
            if (DWCUtilTouch_IsReleasedIn(rect)) {
                if (sMenu->touched != i) {
                    return;
                }
                sMenu->code = sDigitCodes[i];
                sMenu->cursorPos = i;
                DWCUtilMenu_DrawCursor();
                return;
            }
        }
        for (i = 0; i < 2; i++) {
            DWCUtilDisplay_SetRectFromSize(&sArrowPositions[i], sArrowSize, rect);
            if (DWCUtilTouch_IsReleasedIn(rect)) {
                if (sMenu->touched != i + 10) {
                    return;
                }
                sMenu->code = sArrowCodes[i];
                sMenu->cursorPos = i + 10;
                DWCUtilMenu_DrawCursor();
                return;
            }
        }
        for (i = 0; i < 2; i++) {
            DWCUtilDisplay_SetRectFromSize(&sButtonPositions[i], sButtonSize, rect);
            if (DWCUtilTouch_IsReleasedIn(rect)) {
                if (sMenu->touched != i + 12) {
                    return;
                }
                sMenu->code = sButtonCodes[i];
                sMenu->cursorPos = i + 12;
                DWCUtilMenu_DrawCursor();
                return;
            }
        }
    }
}

// A held touch: the key it holds is shown pressed, and the left arrow repeats
static void DWCUtilMenu_HandleTouchHold(void) {
    u16 rect[4];
    int i;

    if (DWCUtilTouch_IsTouchedIn(gDWCUtilTouchScreenRect)) {
        for (i = 0; i < 10; i++) {
            DWCUtilDisplay_SetRectFromSize(&sKeyPositions[i], sKeySize, rect);
            if (DWCUtilTouch_IsTouchedIn(rect)) {
                if (sMenu->touched != i) {
                    goto released;
                }
                goto held;
            }
        }
        for (i = 0; i < 2; i++) {
            DWCUtilDisplay_SetRectFromSize(&sArrowPositions[i], sArrowSize, rect);
            if (DWCUtilTouch_IsTouchedIn(rect)) {
                if (sMenu->touched != i + 10) {
                    goto released;
                }
                DWCUtilMenu_SetPressedKey(i + 10);
                if (i != 0) {
                    goto clear;
                }
                sMenu->repeat++;
                if (sMenu->repeat < 0x28) {
                    return;
                }
                if (!sMenu->leftEnabled) {
                    DWCUtilSound_PlaySe(9);
                    sMenu->touched = -1;
                    return;
                }
                sMenu->code = 0x10;
                sMenu->repeat -= 7;
                return;
            }
        }
        for (i = 0; i < 2; i++) {
            DWCUtilDisplay_SetRectFromSize(&sButtonPositions[i], sButtonSize, rect);
            if (DWCUtilTouch_IsTouchedIn(rect)) {
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
    DWCUtilMenu_SetPressedKey(i);
clear:
    sMenu->repeat = 0;
}

static void DWCUtilMenu_SetPressedKey(int key) {
    if (key != sMenu->pressed) {
        DWCUtilMenu_DrawKey(key, TRUE);
        DWCUtilMenu_DrawKey(sMenu->pressed, FALSE);
        sMenu->pressed = key;
    }
}

// The keys: the D-pad moves the cursor, A chooses its key and B is the left arrow
static void DWCUtilMenu_HandlePad(void) {
    int key;

    if (DWCUtilTouch_IsKeyRepeated(0x20)) {
        DWCUtilMenu_MoveCursor(0);
    }
    if (DWCUtilTouch_IsKeyRepeated(0x40)) {
        DWCUtilMenu_MoveCursor(1);
    }
    if (DWCUtilTouch_IsKeyRepeated(0x10)) {
        DWCUtilMenu_MoveCursor(2);
    }
    if (DWCUtilTouch_IsKeyRepeated(0x80)) {
        DWCUtilMenu_MoveCursor(3);
    }
    if (DWCUtilTouch_IsKeyPressed(1)) {
        key = sMenu->cursorPos;
        if (key < 10) {
            if (sMenu->digitsEnabled) {
                sMenu->code = sDigitCodes[key];
                return;
            }
            DWCUtilSound_PlaySe(9);
            return;
        }
        if (key - 10 < 2) {
            if ((key - 10 == 0 && !sMenu->leftEnabled) || (key - 10 == 1 && !sMenu->rightEnabled)) {
                DWCUtilSound_PlaySe(9);
                return;
            }
            sMenu->code = sArrowCodes[key - 10];
            return;
        }
        sMenu->code = sButtonCodes[key - 12];
    }
    if (DWCUtilTouch_IsKeyRepeated(2)) {
        if (!sMenu->leftEnabled) {
            if (!sMenu->buzzed) {
                DWCUtilSound_PlaySe(9);
                sMenu->buzzed = TRUE;
            }
        } else {
            sMenu->code = 0x10;
        }
    } else if (DWCUtilTouch_IsKeyReleased(2)) {
        sMenu->buzzed = FALSE;
    }
}

// Moves a row of keys to y: rows 0 to 3 of keys, with the arrows on row 3, and row 4 of the buttons
static void DWCUtilMenu_MoveRow(int row, u32 y) {
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
        DWCUtilText_ShowObj(sMenu->rows[row], sKeyPositions[row * 3].x, y, sMenu->rowNodes[row], 2);
    }
    for (i = 0; i < arrowCounts[row]; i++) {
        G2_SetOBJEffect(sMenu->arrowOams[i], GX_OAM_EFFECT_NONE, 0);
        sMenu->arrowOams[i]->attr01 =
            ((sArrowPositions[i].x & 0x1ff) << GX_OAM_ATTR01_X_SHIFT) |
            ((sMenu->arrowOams[i]->attr01 & ~(GX_OAM_ATTR01_X_MASK | GX_OAM_ATTR01_Y_MASK)) | (y & 0xff));
    }
    for (i = 0; i < buttonCounts[row]; i++) {
        DWCUtilOam_SetEffect(sMenu->buttons[i], -1, GX_OAM_EFFECT_NONE, 0);
        DWCUtilOam_SetNodePosition(sMenu->buttons[i], -1, sButtonPositions[i].x, y);
    }
}

static void DWCUtilMenu_DrawKey(int key, int pressed) {
    if (key < 0) {
        return;
    }
    if (key < 10) {
        G2_SetOBJMode(sMenu->keyOams[key], GX_OAM_MODE_NORMAL, sKeyPalettes[pressed]);
    } else if (key - 10 < 2) {
        G2_SetOBJMode(sMenu->arrowOams[key - 10], GX_OAM_MODE_NORMAL, sKeyPalettes[pressed]);
    } else {
        DWCUtilOam_SetMode(sMenu->buttons[key - 12], -1, GX_OAM_MODE_NORMAL, sButtonEffects[pressed]);
    }
}

static void DWCUtilMenu_DrawCursor(void) {
    int cell = 0x44;

    if (sMenu->cursorPos > 11) {
        cell = 0x45;
    }
    DWCUtilCell_Copy(0, cell, DWCUtilOam_GetEntry(sMenu->cursor, 0));
    DWCUtilOam_SetPriority(sMenu->cursor, -1, 2);
    DWCUtilOam_SetNodePosition(sMenu->cursor, -1, sCursorPositions[sMenu->cursorPos].x,
                               sCursorPositions[sMenu->cursorPos].y);
}

static void DWCUtilMenu_MoveCursor(int direction) {
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
    DWCUtilMenu_DrawCursor();
    DWCUtilSound_PlaySe(8);
}

static void DWCUtilMenu_SlideOutButtonsTask(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(DWCUtilOam_GetEntry(sMenu->buttons[0], 0)).attr01 + 12;

    DWCUtilMenu_MoveRow(4, y);
    if (y >= 0xc0) {
        DWCUtilTask_SetCallback(task, DWCUtilMenu_SlideOutRow3Task);
    }
}

static void DWCUtilMenu_SlideOutRow3Task(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[9]).attr01 + 12;

    DWCUtilMenu_MoveRow(3, y);
    if (y >= 0xc0) {
        DWCUtilTask_SetCallback(task, DWCUtilMenu_SlideOutRow2Task);
    }
}

static void DWCUtilMenu_SlideOutRow2Task(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[6]).attr01 + 12;

    DWCUtilMenu_MoveRow(2, y);
    if (y >= 0xc0) {
        DWCUtilTask_SetCallback(task, DWCUtilMenu_SlideOutRow1Task);
    }
}

static void DWCUtilMenu_SlideOutRow1Task(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[3]).attr01 + 12;

    DWCUtilMenu_MoveRow(1, y);
    if (y >= 0xc0) {
        DWCUtilTask_SetCallback(task, DWCUtilMenu_SlideOutRow0Task);
    }
}

static void DWCUtilMenu_SlideOutRow0Task(DWCUtilTask *task, void *arg) {
    int y = (u8)DWCUtilOam_ReadAttr01(sMenu->keyOams[0]).attr01 + 12;

    DWCUtilMenu_MoveRow(0, y);
    if (y >= 0xc0) {
        DWCUtilTask_SetCallback(task, DWCUtilMenu_DestroyTask);
    }
}

static void DWCUtilMenu_DestroyTask(DWCUtilTask *task, void *arg) {
    int i;

    DWCUtilTask_QueueRemoval(0, task);
    for (i = 0; i < 4; i++) {
        DWCUtilOam_Free(sMenu->rowNodes[i]);
        DWCUtilText_DestroyObj(sMenu->rows[i]);
    }
    for (i = 0; i < 2; i++) {
        DWCUtilOam_Free(sMenu->buttons[i]);
    }
    for (i = 0; i < 2; i++) {
        DWCUtilOamBuf_Free(sMenu->arrowOams[i]);
    }
    for (i = 0; i < 10; i++) {
        DWCUtilOamBuf_Free(sMenu->keyOams[i]);
    }
    DWCUtilHeap_FreeAndNull(&sMenu);
}
