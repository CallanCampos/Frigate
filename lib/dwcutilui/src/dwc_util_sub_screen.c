#include "dwc_util_sub_screen.h"
#include "dwc_util_cell.h"
#include "dwc_util_gfx_resource.h"
#include "dwc_util_heap.h"
#include "dwc_util_oam.h"
#include "dwc_util_rom_resource.h"
#include "dwc_util_sound.h"
#include "dwc_util_task.h"
#include "nitro/gx.h"
#include "nitro/os.h"

static void func_ov182_021b8200(DWCUtilTask *task, void *arg);
static void func_ov182_021b8288(DWCUtilTask *task, void *arg);
static void func_ov182_021b8310(DWCUtilTask *task, void *arg);
static void func_ov182_021b8398(DWCUtilTask *task, void *arg);
static void func_ov182_021b8420(DWCUtilTask *task, void *arg);
static void func_ov182_021b8484(DWCUtilTask *task, void *arg);
static void func_ov182_021b8564(DWCUtilTask *task, void *arg);
static void func_ov182_021b85bc(DWCUtilTask *task, void *arg);
static void func_ov182_021b8638(DWCUtilTask *task, void *arg);
static void func_ov182_021b86b4(DWCUtilTask *task, void *arg);
static void func_ov182_021b8730(DWCUtilTask *task, void *arg);
static void func_ov182_021b8804(DWCUtilTask *task, void *arg);

#define SUB_SCREEN_OBJECTS 5

typedef struct DWCUtilSubScreenRow {
    DWCUtilOamNode *objects[SUB_SCREEN_OBJECTS];
    // The task of the slide, NULL once the row has slid in
    DWCUtilTask *task;
    u8 leaving;
} DWCUtilSubScreenRow;

// The cell of each object of the row of each step, the screen of each BG 1, where each object stops, and the palette
// of each object. With MWCC's sort of the file's data by size, this order gives the ROM's .rodata and .data
static const u8 sSubScreenCells[3][SUB_SCREEN_OBJECTS] = {
    { 1, 0, 5, 0, 6 },
    { 4, 0, 2, 0, 6 },
    { 4, 0, 5, 0, 3 },
};
static const char *sSubScreenNames[] = {
    "char/jtStep1.nsc.l", "char/jtStep2.nsc.l", "char/jtStep3.nsc.l", "char/jtOption.nsc.l", "char/jtTop.nsc.l",
};
static const u16 sSubScreenPositions[SUB_SCREEN_OBJECTS][2] = {
    { 0x20, 0x21 }, { 0x50, 0x30 }, { 0x68, 0x21 }, { 0x98, 0x30 }, { 0xb0, 0x21 },
};
static const u8 sSubScreenPalettes[3][SUB_SCREEN_OBJECTS] = {
    { 2, 1, 3, 1, 3 },
    { 5, 1, 4, 1, 5 },
    { 7, 1, 7, 1, 6 },
};

static DWCUtilSubScreenRow *sSubScreenRow;
// The screen that the VBlank task loads
static void *sSubScreenData;

void func_ov182_021b8154(int step) {
    int i;

    sSubScreenRow = DWCUtilHeap_AllocClear(sizeof(DWCUtilSubScreenRow), 4);
    for (i = 0; i < SUB_SCREEN_OBJECTS; i++) {
        sSubScreenRow->objects[i] = DWCUtilCell_AllocOamRun(1, sSubScreenCells[step][i], TRUE);
        DWCUtilOam_SetPriority(sSubScreenRow->objects[i], -1, 0);
        DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, -42, sSubScreenPositions[i][1]);
        DWCUtilOam_SetMode(sSubScreenRow->objects[i], -1, 0, sSubScreenPalettes[step][i]);
    }
    sSubScreenRow->task = DWCUtilTask_Create(0, func_ov182_021b8200, NULL, 120);
    func_ov182_021b87d0(step);
    DWCUtilSound_PlaySe(0xd);
}

// The slide in: the objects from the first that hasn't stopped on move 8 pixels a frame until it stops
static void func_ov182_021b8200(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCUtilOam_GetNodePosition(sSubScreenRow->objects[0], 0, &x, &y);
    x += 8;
    if (x < 0x20 || x > 0x100) {
        for (i = 0; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
    } else {
        x = 0x20;
        for (i = 0; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
        DWCUtilTask_SetCallback(task, func_ov182_021b8288);
    }
}

static void func_ov182_021b8288(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCUtilOam_GetNodePosition(sSubScreenRow->objects[1], 0, &x, &y);
    x += 8;
    if (x < 0x50 || x > 0x100) {
        for (i = 1; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
    } else {
        x = 0x50;
        for (i = 1; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
        DWCUtilTask_SetCallback(task, func_ov182_021b8310);
    }
}

static void func_ov182_021b8310(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCUtilOam_GetNodePosition(sSubScreenRow->objects[2], 0, &x, &y);
    x += 8;
    if (x < 0x68 || x > 0x100) {
        for (i = 2; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
    } else {
        x = 0x68;
        for (i = 2; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
        DWCUtilTask_SetCallback(task, func_ov182_021b8398);
    }
}

static void func_ov182_021b8398(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCUtilOam_GetNodePosition(sSubScreenRow->objects[3], 0, &x, &y);
    x += 8;
    if (x < 0x98 || x > 0x100) {
        for (i = 3; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
    } else {
        x = 0x98;
        for (i = 3; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
        DWCUtilTask_SetCallback(task, func_ov182_021b8420);
    }
}

static void func_ov182_021b8420(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    DWCUtilOam_GetNodePosition(sSubScreenRow->objects[4], 0, &x, &y);
    x += 8;
    if (x < 0xb0 || x > 0x100) {
        DWCUtilOam_SetNodePosition(sSubScreenRow->objects[4], -1, x, 0x21);
    } else {
        x = 0xb0;
        DWCUtilOam_SetNodePosition(sSubScreenRow->objects[4], -1, x, 0x21);
        DWCUtilTask_SetCallback(task, func_ov182_021b8484);
    }
}

static void func_ov182_021b8484(DWCUtilTask *task, void *arg) {
    DWCUtilTask_QueueRemoval(0, task);
    sSubScreenRow->task = NULL;
}

void func_ov182_021b849c(void) {
    sSubScreenRow->leaving = TRUE;
    sSubScreenRow->task = DWCUtilTask_Create(0, func_ov182_021b8564, NULL, 120);
}

void func_ov182_021b84c0(int step) {
    int i;

    for (i = 0; i < SUB_SCREEN_OBJECTS; i += 2) {
        DWCUtilCell_Copy(1, sSubScreenCells[step][i], DWCUtilOam_GetEntry(sSubScreenRow->objects[i], 0));
        DWCUtilOam_SetPriority(sSubScreenRow->objects[i], -1, 0);
        DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, sSubScreenPositions[i][0], sSubScreenPositions[i][1]);
        DWCUtilOam_SetMode(sSubScreenRow->objects[i], -1, 0, sSubScreenPalettes[step][i]);
    }
    func_ov182_021b87d0(step);
}

BOOL func_ov182_021b854c(void) {
    if (sSubScreenRow->task != NULL) {
        return TRUE;
    }
    return FALSE;
}

// The slide out: the objects from the last that hasn't stopped on move 8 pixels a frame to the left until it reaches
// the stop of the one before it, and then all of them off the screen
static void func_ov182_021b8564(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    DWCUtilOam_GetNodePosition(sSubScreenRow->objects[4], 0, &x, &y);
    x -= 8;
    if (x > 0x98) {
        DWCUtilOam_SetNodePosition(sSubScreenRow->objects[4], -1, x, 0x21);
    } else {
        x = 0x98;
        DWCUtilOam_SetNodePosition(sSubScreenRow->objects[4], -1, x, 0x21);
        DWCUtilTask_SetCallback(task, func_ov182_021b85bc);
    }
}

static void func_ov182_021b85bc(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCUtilOam_GetNodePosition(sSubScreenRow->objects[3], 0, &x, &y);
    x -= 8;
    if (x > 0x68) {
        for (i = 3; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
    } else {
        x = 0x68;
        for (i = 3; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
        DWCUtilTask_SetCallback(task, func_ov182_021b8638);
    }
}

static void func_ov182_021b8638(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCUtilOam_GetNodePosition(sSubScreenRow->objects[2], 0, &x, &y);
    x -= 8;
    if (x > 0x50) {
        for (i = 2; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
    } else {
        x = 0x50;
        for (i = 2; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
        DWCUtilTask_SetCallback(task, func_ov182_021b86b4);
    }
}

static void func_ov182_021b86b4(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCUtilOam_GetNodePosition(sSubScreenRow->objects[1], 0, &x, &y);
    x -= 8;
    if (x > 0x20) {
        for (i = 1; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
    } else {
        x = 0x20;
        for (i = 1; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
        DWCUtilTask_SetCallback(task, func_ov182_021b8730);
    }
}

// Once the row is off the left of the screen (its x wraps to 0x100 and above), frees it
static void func_ov182_021b8730(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCUtilOam_GetNodePosition(sSubScreenRow->objects[0], 0, &x, &y);
    x -= 8;
    for (i = 0; i < SUB_SCREEN_OBJECTS; i++) {
        DWCUtilOam_SetNodePosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
    }
    if (x <= 0x1d6 && x >= 0x100) {
        DWCUtilTask_QueueRemoval(0, task);
        for (i = 0; i < SUB_SCREEN_OBJECTS; i++) {
            DWCUtilOam_Free(sSubScreenRow->objects[i]);
        }
        DWCUtilHeap_FreeAndNull(&sSubScreenRow);
    }
}

BOOL func_ov182_021b87b0(void) {
    DWCUtilSubScreenRow *row = sSubScreenRow;

    if (row == NULL) {
        return TRUE;
    }
    if (!row->leaving) {
        return TRUE;
    }
    return FALSE;
}

void func_ov182_021b87d0(int screen) {
    sSubScreenData = DWCUtilRomResource_LoadFile(DWCUtilGfxResource_LocalizePath(sSubScreenNames[screen]), NULL, 4);
    DWCUtilTask_Create(1, func_ov182_021b8804, NULL, 120);
}

static void func_ov182_021b8804(DWCUtilTask *task, void *arg) {
    DC_FlushRange(sSubScreenData, 0x600);
    GXS_LoadBG1Scr(sSubScreenData, 0, 0x600);
    DWCUtilRomResource_FreeFile(sSubScreenData);
    DWCUtilTask_QueueRemoval(1, task);
}
