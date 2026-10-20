#include "dwc_util_scrollbar.h"
#include "dwc_util_cell.h"
#include "dwc_util_display.h"
#include "dwc_util_heap.h"
#include "dwc_util_oam.h"
#include "dwc_util_shared.h"
#include "dwc_util_task.h"
#include "dwc_util_touch.h"
#include "nitro/fx.h"

static void func_ov182_021b7184(DWCUtilTask *task, void *arg);
static void func_ov182_021b726c(void);
static void func_ov182_021b72ec(int pos);
static void func_ov182_021b734c(int part);
static void func_ov182_021b7368(void);
static int func_ov182_021b73ac(void);
static int func_ov182_021b7408(int part);
static void func_ov182_021b7434(int part, u16 *rect);
static void func_ov182_021b74b0(int pos);

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

void func_ov182_021b70a0(int kind, int range, u16 x, u16 y, int pos) {
    sScrollbar = func_ov182_021bc4d8(sizeof(DWCUtilScrollbar), 4);
    sScrollbar->kind = kind;
    sScrollbar->range = range;
    sScrollbar->pos = pos;
    func_ov182_021bc1a8(x, y, sScrollbar->origin);
    sScrollbar->handle = func_ov182_021bb97c(0, sScrollbarCells[kind], TRUE);
    func_ov182_021bb7e0(sScrollbar->handle, -1, x, y + pos);
    func_ov182_021bb878(sScrollbar->handle, -1, 1);
    sScrollbar->task = func_ov182_021bd200(0, func_ov182_021b7184, NULL, 0x80);
}

void func_ov182_021b711c(void) {
    func_ov182_021bd280(0, sScrollbar->task);
    func_ov182_021bb690(sScrollbar->handle);
    func_ov182_021bc4f0(&sScrollbar);
}

u8 func_ov182_021b7144(void) {
    return sScrollbar->pos;
}

u8 func_ov182_021b7150(void) {
    return sScrollbar->event;
}

void func_ov182_021b715c(int pos) {
    func_ov182_021b74b0(pos);
}

void func_ov182_021b7164(void) {
    sScrollbar->locked = FALSE;
}

void func_ov182_021b7174(void) {
    sScrollbar->locked = TRUE;
}

static void func_ov182_021b7184(DWCUtilTask *task, void *arg) {
    sScrollbar->event = DWC_UTIL_SCROLLBAR_NONE;
    switch (sScrollbar->state) {
    case SCROLLBAR_IDLE:
        if (sScrollbar->locked) {
            break;
        }
        switch (func_ov182_021b73ac()) {
        case SCROLLBAR_PART_HANDLE:
            if (sScrollbar->kind != 0) {
                func_ov182_021b80f4(0x16);
                func_ov182_021b810c(0);
                sScrollbar->event = DWC_UTIL_SCROLLBAR_GRAB;
                func_ov182_021bc9e4(sScrollbar->grabPoint);
                sScrollbar->grabPos = sScrollbar->pos;
                sScrollbar->state = SCROLLBAR_DRAGGING;
            }
            break;
        case SCROLLBAR_PART_UP:
            func_ov182_021b734c(SCROLLBAR_PART_UP);
            break;
        case SCROLLBAR_PART_DOWN:
            func_ov182_021b734c(SCROLLBAR_PART_DOWN);
            break;
        case SCROLLBAR_PART_BAR:
            func_ov182_021b7368();
            break;
        }
        break;
    case SCROLLBAR_DRAGGING:
        func_ov182_021b726c();
        break;
    case SCROLLBAR_PART_UP:
        if (func_ov182_021b7408(SCROLLBAR_PART_UP) != SCROLLBAR_PART_UP) {
            sScrollbar->event = DWC_UTIL_SCROLLBAR_UP_END;
            sScrollbar->state = SCROLLBAR_IDLE;
        } else if (func_ov182_021b73ac() == SCROLLBAR_PART_UP) {
            func_ov182_021b734c(SCROLLBAR_PART_UP);
        }
        break;
    case SCROLLBAR_PART_DOWN:
        if (func_ov182_021b7408(SCROLLBAR_PART_DOWN) != SCROLLBAR_PART_DOWN) {
            sScrollbar->event = DWC_UTIL_SCROLLBAR_DOWN_END;
            sScrollbar->state = SCROLLBAR_IDLE;
        } else if (func_ov182_021b73ac() == SCROLLBAR_PART_DOWN) {
            func_ov182_021b734c(SCROLLBAR_PART_DOWN);
        }
        break;
    }
}

// Moves the handle with the touch while it is held near the bar, and lets it go when it isn't
static void func_ov182_021b726c(void) {
    u16 point[2];
    int pos;
    int max;

    if (func_ov182_021bc828(data_ov182_021c059c)) {
        func_ov182_021bc9e4(point);
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
            func_ov182_021b72ec(pos);
            func_ov182_021b74b0(pos);
            sScrollbar->event = DWC_UTIL_SCROLLBAR_DRAG;
            return;
        }
    }
    func_ov182_021b8138();
    sScrollbar->state = SCROLLBAR_IDLE;
    sScrollbar->event = DWC_UTIL_SCROLLBAR_RELEASE;
}

// The sound of the drag: louder and higher the farther the handle moves in a frame
static void func_ov182_021b72ec(int pos) {
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
    func_ov182_021b810c(volume);
    if (distance < 2) {
        pitch = -0x100;
    } else if (distance < 6) {
        pitch = FX_DivS32(0x200, 6 - distance) - 0x100;
    } else {
        pitch = 0x100;
    }
    func_ov182_021b8120(0xffff, pitch);
}

// An arrow is held
static void func_ov182_021b734c(int part) {
    sScrollbar->state = part;
    sScrollbar->event = part == SCROLLBAR_PART_UP ? DWC_UTIL_SCROLLBAR_UP : DWC_UTIL_SCROLLBAR_DOWN;
}

// The bar was touched: the handle's middle jumps to the touch
static void func_ov182_021b7368(void) {
    u16 point[2];
    u8 length;
    int pos;
    int max;

    func_ov182_021bc9e4(point);
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
    func_ov182_021b74b0(pos);
    sScrollbar->event = DWC_UTIL_SCROLLBAR_RELEASE;
}

// The part of the scroll bar that a new touch is on
static int func_ov182_021b73ac(void) {
    u16 rect[4];
    int part;

    func_ov182_021b7434(SCROLLBAR_PART_HANDLE, rect);
    if (func_ov182_021bc8e8(rect)) {
        return SCROLLBAR_PART_HANDLE;
    }
    for (part = SCROLLBAR_PART_UP; part <= SCROLLBAR_PART_DOWN; part++) {
        func_ov182_021b7434(part, rect);
        if (func_ov182_021bc8e8(rect)) {
            return part;
        }
    }
    part = SCROLLBAR_PART_BAR;
    func_ov182_021b7434(SCROLLBAR_PART_BAR, rect);
    if (!func_ov182_021bc888(rect)) {
        part = SCROLLBAR_PART_NONE;
    }
    return part;
}

// The arrow that the touch is held on, of either arrow whichever part is given
static int func_ov182_021b7408(int part) {
    u16 rect[4];
    int i;

    for (i = SCROLLBAR_PART_UP; i <= SCROLLBAR_PART_DOWN; i++) {
        func_ov182_021b7434(i, rect);
        if (func_ov182_021bc828(rect)) {
            return i;
        }
    }
    return SCROLLBAR_PART_NONE;
}

// The rectangle of a part, as in dwc_util_display.c
static void func_ov182_021b7434(int part, u16 *rect) {
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

static void func_ov182_021b74b0(int pos) {
    func_ov182_021bb7e0(sScrollbar->handle, -1, sScrollbar->origin[0], pos + sScrollbar->origin[1]);
    sScrollbar->pos = pos;
}
