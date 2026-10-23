#include "dwc_util_dialog.h"
#include "dwc_util_cell.h"
#include "dwc_util_connection.h"
#include "dwc_util_display.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_heap.h"
#include "dwc_util_msg_resource.h"
#include "dwc_util_oam.h"
#include "dwc_util_task.h"
#include "dwc_util_text.h"
#include "dwc_util_touch.h"
#include "nitro/gx.h"
#include "nitro/hw.h"

static void DWCUtilDialog_DarkenTask(DWCUtilTask *task, void *arg);
static void DWCUtilDialog_SlideUpTask(DWCUtilTask *task, void *arg);
static void DWCUtilDialog_ChooseTask(DWCUtilTask *task, void *arg);
static void DWCUtilDialog_TimerTask(DWCUtilTask *task, void *arg);
static void DWCUtilDialog_MoveTo(int y);
static void dispCell(DWCUtilOamNode *node, int top);
static void pushButton(int button);
static void DWCUtilDialog_CloseDelayTask(DWCUtilTask *task, void *arg);
static void DWCUtilDialog_SlideDownTask(DWCUtilTask *task, void *arg);
static void DWCUtilDialog_BrightenTask(DWCUtilTask *task, void *arg);
static void DWCUtilDialog_DestroyTask(DWCUtilTask *task, void *arg);

#define DIALOG_LAYOUTS 6
// The layout without buttons, which closes by itself
#define DIALOG_LAYOUT_TIMED 5

typedef struct DWCUtilDialog {
    DWCUtilOamNode *frame;
    // The objects that show the text
    DWCUtilOamNode *textObjects;
    DWCUtilOamNode *buttons[2];
    DWCUtilText *text;
    DWCUtilTask *task;
    // The frames that the timed layout has been up
    u16 frames;
    s8 brightness;
    s8 choice;
    u8 layout;
    // The frames since the dialog was closed, before it slides down
    u8 delay;
    u8 fade;
} DWCUtilDialog;

// The offset of the text in the frame, which nothing reads (DWCUtilDialog_MoveTo adds 8 to each coordinate). For each
// layout: the frame's cell and position, how many buttons it has, which of the two places each button takes,
// the button that A and B press (-1 for none), each button's cell and size, the size of the text and the size of the
// window that hides what is below the frame. With MWCC's sort of the file's data by size, this order gives the ROM's
// .rodata
const u16 gDialogTextOffset[2] = { 8, 8 };
static const u8 sDialogButtonCounts[DIALOG_LAYOUTS] = { 2, 1, 2, 1, 2, 0 };
static const u8 sDialogFrameCells[DIALOG_LAYOUTS] = { 0, 0, 0x46, 0x46, 0x4f, 0x2f };
static const u16 sDialogWindowRect[4] = { 4, 0x1d, 0xfc, 0x44 };
static const u8 sDialogButtonSlots[DIALOG_LAYOUTS - 1][2] = { { 0, 1 }, { 1, 0 }, { 0, 1 }, { 1, 0 }, { 0, 1 } };
static const s8 sDialogKeyButtons[DIALOG_LAYOUTS - 1][2] = { { 1, 0 }, { 0, -1 }, { 1, 0 }, { 0, -1 }, { 1, 0 } };
static const u8 sDialogButtonCells[DIALOG_LAYOUTS - 1][2] = {
    { 0x1b, 0x19 }, { 0x57, 0 }, { 0x23, 0x1d }, { 0x59, 0 }, { 0x23, 0x1d },
};
static const u16 sDialogButtonSizes[DIALOG_LAYOUTS - 1][2] = {
    { 0x6c, 0x10 }, { 0x6c, 0x10 }, { 0x78, 0x10 }, { 0x78, 0x10 }, { 0x78, 0x10 },
};
static const u16 sDialogTextSizes[DIALOG_LAYOUTS][2] = {
    { 0xd8, 0x50 }, { 0xd8, 0x50 }, { 0xe6, 0x4f }, { 0xe6, 0x4f }, { 0xe6, 0x48 }, { 0xda, 0x5c },
};
static const u16 sDialogFramePositions[DIALOG_LAYOUTS][2] = {
    { 0x0b, 0x27 }, { 0x0b, 0x27 }, { 0x04, 0x4c }, { 0x04, 0x4c }, { 0x04, 0x54 }, { 0x0b, 0x27 },
};
static const u16 sDialogWindowSizes[DIALOG_LAYOUTS][2] = {
    { 0xea, 0x72 }, { 0xea, 0x72 }, { 0xf8, 0x70 }, { 0xf8, 0x70 }, { 0x64, 0x70 }, { 0xea, 0x72 },
};
static const u16 sDialogButtonPositions[DIALOG_LAYOUTS - 1][2][2] = {
    { { 0x10, 0x84 }, { 0x84, 0x84 } }, { { 0x10, 0x84 }, { 0x84, 0x84 } }, { { 0x09, 0xa7 }, { 0x83, 0xa7 } },
    { { 0x09, 0xa7 }, { 0x83, 0xa7 } }, { { 0x09, 0xa7 }, { 0x83, 0xa7 } },
};

static DWCUtilDialog *sDialog;

void DWCUtilDialog_Open(int msgIndex, int layout, BOOL fade, int slot, int value) {
    // Whether each layout's text has the effect of window 1
    u8 textEffects[DIALOG_LAYOUTS] = { 1, 1, 0, 0, 1, 1 };
    int textObjCount;
    u16 *str;
    int i;

    str = DWCi_MSGlGetEx(gDWCUtilMsg, msgIndex, slot, value);
    sDialog = DWCi_HEAPlAllocEx(sizeof(DWCUtilDialog), 4);
    sDialog->layout = layout;
    sDialog->choice = -2;
    sDialog->fade = fade;
    G2x_SetBlendBrightness_(REG_BLDCNT_ADDR, 0x1f, 0);
    sDialog->frame = DWCUtilCell_AllocOamRun(0, sDialogFrameCells[layout], FALSE);
    DWCi_CELLlSetPosition(sDialog->frame, -1, 0x100, 0);
    DWCi_CELLlSetPriority(sDialog->frame, -1, 0);
    for (i = 0; i < sDialogButtonCounts[layout]; i++) {
        sDialog->buttons[i] = DWCUtilCell_AllocOamRun(0, sDialogButtonCells[layout][i], FALSE);
        DWCi_CELLlSetPosition(sDialog->buttons[i], -1, 0x100, 0);
        DWCi_CELLlSetPriority(sDialog->buttons[i], -1, 0);
    }
    sDialog->text = DWCi_FNTlFormEx(0, 0x20, 0xc, 1, &textObjCount, 0);
    sDialog->textObjects = DWCi_CELLlForm(0, textObjCount, FALSE);
    DWCi_FNTlDrawRect(sDialog->text, 0, 0, sDialogTextSizes[layout][0], sDialogTextSizes[layout][1], 2,
                      DWCi_CMNlFontFlag(), str);
    DWCi_FNTlDisp(sDialog->text, 0x100, 0, sDialog->textObjects, 0);
    DWCi_GXlSetWndPlane(0, 0, 0x1f, FALSE);
    DWCi_GXlSetWndPlane(0, 1, 0x1f, textEffects[sDialog->layout]);
    DWCi_GXlSetWndPlane(0, 3, 0x1f, TRUE);
    DWCi_GXlSetWndPosition(0, 1, sDialogWindowRect);
    DWCUtilDialog_MoveTo(0xc0);
    GX_SetVisibleWnd(GX_WNDMASK_W0 | GX_WNDMASK_W1);
    if (fade) {
        sDialog->task = DWCi_TSKlForm(1, DWCUtilDialog_DarkenTask, NULL, 120);
    } else {
        sDialog->task = DWCi_TSKlForm(1, DWCUtilDialog_SlideUpTask, NULL, 120);
    }
}

void DWCi_WINlEnd(void) {
    DWCi_TSKlDelete(0, sDialog->task);
    sDialog->task = DWCi_TSKlForm(1, DWCUtilDialog_CloseDelayTask, NULL, 120);
}

int DWCi_WINlGet(void) {
    return sDialog->choice;
}

BOOL DWCi_WINlIsInit(void) {
    if (sDialog != NULL) {
        return TRUE;
    }
    return FALSE;
}

// Darkens the screen behind the dialog
static void DWCUtilDialog_DarkenTask(DWCUtilTask *task, void *arg) {
    sDialog->brightness--;
    G2x_ChangeBlendBrightness_(REG_BLDCNT_ADDR, sDialog->brightness);
    if (sDialog->brightness <= -12) {
        DWCi_TSKlChangeFunc(task, DWCUtilDialog_SlideUpTask);
    }
}

// Slides the dialog up 12 pixels a frame to its place
static void DWCUtilDialog_SlideUpTask(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    DWCi_CELLlGetPosition(sDialog->frame, 0, &x, &y);
    y -= 12;
    if (y > sDialogFramePositions[sDialog->layout][1]) {
        DWCUtilDialog_MoveTo(y);
        return;
    }
    DWCUtilDialog_MoveTo(sDialogFramePositions[sDialog->layout][1]);
    if (sDialog->layout == DIALOG_LAYOUT_TIMED) {
        sDialog->task = DWCi_TSKlForm(0, DWCUtilDialog_TimerTask, NULL, 120);
    } else {
        sDialog->task = DWCi_TSKlForm(0, DWCUtilDialog_ChooseTask, NULL, 120);
    }
    DWCi_TSKlDelete(1, task);
}

// The buttons: a touch on one, or A or B, chooses it
static void DWCUtilDialog_ChooseTask(DWCUtilTask *task, void *arg) {
    u16 rect[4];
    int i;

    for (i = 0; i < sDialogButtonCounts[sDialog->layout]; i++) {
        DWCUtilDisplay_SetRectFromSize(sDialogButtonPositions[sDialog->layout][sDialogButtonSlots[sDialog->layout][i]],
                                       sDialogButtonSizes[sDialog->layout], rect);
        if (DWCi_TPlCheckTrigger(rect)) {
            sDialog->choice = i;
            break;
        }
    }
    if (DWCi_IPTlCheckTrigger(1)) {
        sDialog->choice = sDialogKeyButtons[sDialog->layout][0];
    }
    if (DWCi_IPTlCheckTrigger(2)) {
        sDialog->choice = sDialogKeyButtons[sDialog->layout][1];
    }
    for (i = 0; i < sDialogButtonCounts[sDialog->layout]; i++) {
        if (i == sDialog->choice) {
            pushButton(i);
            return;
        }
    }
    sDialog->choice = -1;
}

static void DWCUtilDialog_TimerTask(DWCUtilTask *task, void *arg) {
    sDialog->choice = -1;
    sDialog->frames++;
    if (sDialog->frames >= 120) {
        DWCi_TSKlDelete(0, task);
        sDialog->task = DWCi_TSKlForm(1, DWCUtilDialog_CloseDelayTask, NULL, 120);
    }
}

// Puts the frame's top at y, with the text and the buttons, and the window below it
static void DWCUtilDialog_MoveTo(int y) {
    u16 rect[4];
    const u16 *pos;
    int i;
    int bottom;
    int top;

    DWCi_CELLlSetPosition(sDialog->frame, -1, sDialogFramePositions[sDialog->layout][0], y);
    DWCi_CELLlSetPosition(sDialog->textObjects, -1, sDialogFramePositions[sDialog->layout][0] + 8, y + 8);
    dispCell(sDialog->frame, y);
    dispCell(sDialog->textObjects, y);
    for (i = 0; i < sDialogButtonCounts[sDialog->layout]; i++) {
        pos = sDialogButtonPositions[sDialog->layout][sDialogButtonSlots[sDialog->layout][i]];
        DWCi_CELLlSetPosition(sDialog->buttons[i], -1, pos[0], y + pos[1] - sDialogFramePositions[sDialog->layout][1]);
        dispCell(sDialog->buttons[i], y);
    }
    top = (u8)y;
    if (top >= 0xc0) {
        bottom = 0;
        top = 0;
    } else {
        bottom = top + sDialogWindowSizes[sDialog->layout][1];
    }
    if (bottom > 0xc0) {
        bottom = 0xc0;
    }
    DWCUtilDisplay_SetRect(sDialogFramePositions[sDialog->layout][0], top,
                           sDialogFramePositions[sDialog->layout][0] + sDialogWindowSizes[sDialog->layout][0], bottom,
                           rect);
    DWCi_GXlSetWndPosition(0, 0, rect);
}

// Hides the objects of a node that are above top or below the screen
static void dispCell(DWCUtilOamNode *node, int top) {
    int count = DWCi_CELLlGetObjNum(node);
    int x;
    int y;
    int effect;
    int i;

    for (i = 0; i < count; i++) {
        DWCi_CELLlGetPosition(node, i, &x, &y);
        if (y >= top && y < 0xc0) {
            effect = 0;
        } else {
            effect = 0x200;
        }
        DWCi_CELLlSetEffect(node, i, effect, 0);
    }
}

// Lights the button chosen
static void pushButton(int button) {
    DWCUtilCell_Copy(0, sDialogButtonCells[sDialog->layout][button] + 1, DWCi_CELLlGetObj(sDialog->buttons[button], 0));
    DWCi_CELLlSetPosition(sDialog->buttons[button], -1,
                          sDialogButtonPositions[sDialog->layout][sDialogButtonSlots[sDialog->layout][button]][0],
                          sDialogButtonPositions[sDialog->layout][sDialogButtonSlots[sDialog->layout][button]][1]);
    DWCi_CELLlSetPriority(sDialog->buttons[button], -1, 0);
}

static void DWCUtilDialog_CloseDelayTask(DWCUtilTask *task, void *arg) {
    sDialog->delay++;
    if (sDialog->delay >= 8) {
        DWCi_TSKlChangeFunc(task, DWCUtilDialog_SlideDownTask);
    }
}

// Slides the dialog down 12 pixels a frame off the screen
static void DWCUtilDialog_SlideDownTask(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    DWCi_CELLlGetPosition(sDialog->frame, 0, &x, &y);
    y += 12;
    DWCUtilDialog_MoveTo(y);
    if (y >= 0xc0) {
        if (sDialog->fade) {
            DWCi_TSKlChangeFunc(task, DWCUtilDialog_BrightenTask);
        } else {
            DWCi_TSKlChangeFunc(task, DWCUtilDialog_DestroyTask);
        }
    }
}

static void DWCUtilDialog_BrightenTask(DWCUtilTask *task, void *arg) {
    sDialog->brightness++;
    G2x_ChangeBlendBrightness_(REG_BLDCNT_ADDR, sDialog->brightness);
    if (sDialog->brightness >= 0) {
        DWCi_TSKlChangeFunc(task, DWCUtilDialog_DestroyTask);
    }
}

static void DWCUtilDialog_DestroyTask(DWCUtilTask *task, void *arg) {
    int i;

    GX_SetVisibleWnd(GX_WNDMASK_NONE);
    DWCi_CELLlDelete(sDialog->frame);
    DWCi_CELLlDelete(sDialog->textObjects);
    for (i = 0; i < sDialogButtonCounts[sDialog->layout]; i++) {
        if (sDialog->buttons[i] != NULL) {
            DWCi_CELLlDelete(sDialog->buttons[i]);
        }
    }
    DWCi_FNTlDelete(sDialog->text);
    DWCi_TSKlDelete(1, task);
    DWCi_HEAPlFree_(&sDialog);
}
