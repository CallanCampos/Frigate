#include "scrollbar.h"
#include "cell.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_heap.h"
#include "dwc_util_shared.h"
#include "input.h"
#include "nitro/fx.h"
#include "sound.h"
#include "task.h"

static void DWCUtilScrollbar_UpdateTask(DWCUtilTask *task, void *arg);
static void DWCUtilScrollbar_Drag(void);
static void playSound(int pos);
static void stepBar(int part);
static void DWCUtilScrollbar_JumpToTouch(void);
static int isTrigger(void);
static int DWCUtilScrollbar_GetHeldArrow(int part);
static void calcRect(int part, u16 *rect);
static void DWCUtilScrollbar_MoveHandle(int pos);

// The parts of the scroll bar that a touch can be on
enum {
    SCROLLBAR_PART_NONE,
    SCROLLBAR_PART_HANDLE,
    SCROLLBAR_PART_UP,
    SCROLLBAR_PART_DOWN,
    SCROLLBAR_PART_BAR,
};

// What the scroll bar is doing: nothing, the handle is dragged, or an arrow is held (SCROLLBAR_PART_UP and DOWN)
enum {
    SCROLLBAR_IDLE,
    SCROLLBAR_DRAGGING,
};

typedef struct DWCUtilScrollbar {
    DWCUtilOamNode *handle;
    u8 unk_04[8];
    DWCUtilTask *task;
    // The top of the bar, and where the touch grabbed the handle
    u16 origin[2];
    u16 grabPoint[2];
    // The handle's position when it was grabbed
    u8 grabPos;
    u8 range;
    u8 pos;
    u8 kind;
    u8 state;
    u8 event;
    u8 locked;
} DWCUtilScrollbar;

// The length and the cell of the handle of each kind. With MWCC's sort of the file's data by size, this order gives
// the ROM's .rodata
static const u8 sScrollbarHandleLengths[3] = { 0x55, 0x36, 0x1e };
static const u8 sScrollbarCells[3] = { 0x10, 0x0f, 0x0e };

static DWCUtilScrollbar *sScrollbar;

void DWCUtilScrollbar_Create(int kind, int range, u16 x, u16 y, int pos) {
    sScrollbar = DWCi_HEAPlAllocEx(sizeof(DWCUtilScrollbar), 4);
    sScrollbar->kind = kind;
    sScrollbar->range = range;
    sScrollbar->pos = pos;
    DWCUtilDisplay_SetPoint(x, y, sScrollbar->origin);
    sScrollbar->handle = DWCUtilCell_AllocOamRun(0, sScrollbarCells[kind], TRUE);
    DWCi_CELLlSetPosition(sScrollbar->handle, -1, x, y + pos);
    DWCi_CELLlSetPriority(sScrollbar->handle, -1, 1);
    sScrollbar->task = DWCi_TSKlForm(0, DWCUtilScrollbar_UpdateTask, NULL, 0x80);
}

void DWCi_SBlEnd(void) {
    DWCi_TSKlDeleteEx(0, sScrollbar->task);
    DWCi_CELLlDelete(sScrollbar->handle);
    DWCi_HEAPlFree_(&sScrollbar);
}

u8 DWCi_SBlGet(void) {
    return sScrollbar->pos;
}

u8 DWCi_SBlGetState(void) {
    return sScrollbar->event;
}

void DWCi_SBlSet(int pos) {
    DWCUtilScrollbar_MoveHandle(pos);
}

void DWCi_SBlEnable(void) {
    sScrollbar->locked = FALSE;
}

void DWCi_SBlDisable(void) {
    sScrollbar->locked = TRUE;
}

static void DWCUtilScrollbar_UpdateTask(DWCUtilTask *task, void *arg) {
    sScrollbar->event = DWC_UTIL_SCROLLBAR_NONE;
    switch (sScrollbar->state) {
    case SCROLLBAR_IDLE:
        if (sScrollbar->locked) {
            break;
        }
        switch (isTrigger()) {
        case SCROLLBAR_PART_HANDLE:
            if (sScrollbar->kind != 0) {
                DWCUtilSound_PlaySe(0x16);
                DWCUtilSound_SetVolume(0);
                sScrollbar->event = DWC_UTIL_SCROLLBAR_GRAB;
                DWCi_TPlGet(sScrollbar->grabPoint);
                sScrollbar->grabPos = sScrollbar->pos;
                sScrollbar->state = SCROLLBAR_DRAGGING;
            }
            break;
        case SCROLLBAR_PART_UP:
            stepBar(SCROLLBAR_PART_UP);
            break;
        case SCROLLBAR_PART_DOWN:
            stepBar(SCROLLBAR_PART_DOWN);
            break;
        case SCROLLBAR_PART_BAR:
            DWCUtilScrollbar_JumpToTouch();
            break;
        }
        break;
    case SCROLLBAR_DRAGGING:
        DWCUtilScrollbar_Drag();
        break;
    case SCROLLBAR_PART_UP:
        if (DWCUtilScrollbar_GetHeldArrow(SCROLLBAR_PART_UP) != SCROLLBAR_PART_UP) {
            sScrollbar->event = DWC_UTIL_SCROLLBAR_UP_END;
            sScrollbar->state = SCROLLBAR_IDLE;
        } else if (isTrigger() == SCROLLBAR_PART_UP) {
            stepBar(SCROLLBAR_PART_UP);
        }
        break;
    case SCROLLBAR_PART_DOWN:
        if (DWCUtilScrollbar_GetHeldArrow(SCROLLBAR_PART_DOWN) != SCROLLBAR_PART_DOWN) {
            sScrollbar->event = DWC_UTIL_SCROLLBAR_DOWN_END;
            sScrollbar->state = SCROLLBAR_IDLE;
        } else if (isTrigger() == SCROLLBAR_PART_DOWN) {
            stepBar(SCROLLBAR_PART_DOWN);
        }
        break;
    }
}

// Moves the handle with the touch while it is held near the bar, and lets it go when it isn't
static void DWCUtilScrollbar_Drag(void) {
    u16 point[2];
    int pos;
    int max;

    if (DWCi_TPlCheck(gDWCUtilTouchScreenRect)) {
        DWCi_TPlGet(point);
        if (point[0] >= sScrollbar->origin[0] - 30) {
            pos = sScrollbar->grabPos + (point[1] - sScrollbar->grabPoint[1]);
            if (pos < 0) {
                pos = 0;
            } else {
                max = sScrollbar->range - sScrollbarHandleLengths[sScrollbar->kind];
                if (pos >= max) {
                    pos = max;
                }
            }
            playSound(pos);
            DWCUtilScrollbar_MoveHandle(pos);
            sScrollbar->event = DWC_UTIL_SCROLLBAR_DRAG;
            return;
        }
    }
    DWCUtilSound_StopSe();
    sScrollbar->state = SCROLLBAR_IDLE;
    sScrollbar->event = DWC_UTIL_SCROLLBAR_RELEASE;
}

// The sound of the drag: louder and higher the farther the handle moves in a frame
static void playSound(int pos) {
    int distance = sScrollbar->pos - pos;
    int volume;
    int pitch;

    if (distance < 0) {
        distance = -distance;
    }
    if (distance < 2) {
        volume = 0;
    } else if (distance < 6) {
        volume = FX_DivS32(0x7f, 6 - distance);
    } else {
        volume = 0x7f;
    }
    DWCUtilSound_SetVolume(volume);
    if (distance < 2) {
        pitch = -0x100;
    } else if (distance < 6) {
        pitch = FX_DivS32(0x200, 6 - distance) - 0x100;
    } else {
        pitch = 0x100;
    }
    DWCUtilSound_SetTrackPitch(0xffff, pitch);
}

// An arrow is held
static void stepBar(int part) {
    sScrollbar->state = part;
    sScrollbar->event = part == SCROLLBAR_PART_UP ? DWC_UTIL_SCROLLBAR_UP : DWC_UTIL_SCROLLBAR_DOWN;
}

// The bar was touched: the handle's middle jumps to the touch
static void DWCUtilScrollbar_JumpToTouch(void) {
    u16 point[2];
    u8 length;
    int pos;
    int max;

    DWCi_TPlGet(point);
    length = sScrollbarHandleLengths[sScrollbar->kind];
    pos = point[1] - sScrollbar->origin[1] - length / 2;
    if (pos < 0) {
        pos = 0;
    } else {
        max = sScrollbar->range - length;
        if (pos >= max) {
            pos = max;
        }
    }
    DWCUtilScrollbar_MoveHandle(pos);
    sScrollbar->event = DWC_UTIL_SCROLLBAR_RELEASE;
}

// The part of the scroll bar that a new touch is on
static int isTrigger(void) {
    u16 rect[4];
    int part;

    calcRect(SCROLLBAR_PART_HANDLE, rect);
    if (DWCi_TPlCheckRepeat(rect)) {
        return SCROLLBAR_PART_HANDLE;
    }
    for (part = SCROLLBAR_PART_UP; part <= SCROLLBAR_PART_DOWN; part++) {
        calcRect(part, rect);
        if (DWCi_TPlCheckRepeat(rect)) {
            return part;
        }
    }
    part = SCROLLBAR_PART_BAR;
    calcRect(SCROLLBAR_PART_BAR, rect);
    if (!DWCi_TPlCheckTrigger(rect)) {
        part = SCROLLBAR_PART_NONE;
    }
    return part;
}

// The arrow that the touch is held on, of either arrow whichever part is given
static int DWCUtilScrollbar_GetHeldArrow(int part) {
    u16 rect[4];
    int i;

    for (i = SCROLLBAR_PART_UP; i <= SCROLLBAR_PART_DOWN; i++) {
        calcRect(i, rect);
        if (DWCi_TPlCheck(rect)) {
            return i;
        }
    }
    return SCROLLBAR_PART_NONE;
}

// The rectangle of a part, as in dwc_util_display.c
static void calcRect(int part, u16 *rect) {
    u16 top;

    rect[0] = sScrollbar->origin[0];
    rect[2] = rect[0] + 12;
    switch (part) {
    case SCROLLBAR_PART_HANDLE:
        rect[1] = sScrollbar->origin[1] + sScrollbar->pos;
        rect[3] = rect[1] + sScrollbarHandleLengths[sScrollbar->kind];
        break;
    case SCROLLBAR_PART_UP:
        rect[1] = sScrollbar->origin[1] - 13;
        rect[3] = sScrollbar->origin[1];
        break;
    case SCROLLBAR_PART_DOWN:
        rect[1] = sScrollbar->origin[1] + sScrollbar->range;
        rect[3] = rect[1] + 13;
        break;
    case SCROLLBAR_PART_BAR:
        top = sScrollbar->origin[1];
        rect[1] = top;
        rect[3] = top + sScrollbar->range;
        break;
    }
}

static void DWCUtilScrollbar_MoveHandle(int pos) {
    DWCi_CELLlSetPosition(sScrollbar->handle, -1, sScrollbar->origin[0], pos + sScrollbar->origin[1]);
    sScrollbar->pos = pos;
}
