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

static void DWCUtilSubScreen_SlideInObject0Task(DWCUtilTask *task, void *arg);
static void DWCUtilSubScreen_SlideInObject1Task(DWCUtilTask *task, void *arg);
static void DWCUtilSubScreen_SlideInObject2Task(DWCUtilTask *task, void *arg);
static void DWCUtilSubScreen_SlideInObject3Task(DWCUtilTask *task, void *arg);
static void DWCUtilSubScreen_SlideInObject4Task(DWCUtilTask *task, void *arg);
static void DWCUtilSubScreen_SlideInDoneTask(DWCUtilTask *task, void *arg);
static void DWCUtilSubScreen_SlideOutObject4Task(DWCUtilTask *task, void *arg);
static void DWCUtilSubScreen_SlideOutObject3Task(DWCUtilTask *task, void *arg);
static void DWCUtilSubScreen_SlideOutObject2Task(DWCUtilTask *task, void *arg);
static void DWCUtilSubScreen_SlideOutObject1Task(DWCUtilTask *task, void *arg);
static void DWCUtilSubScreen_SlideOutObject0Task(DWCUtilTask *task, void *arg);
static void DWCUtilSubScreen_LoadScreenTask(DWCUtilTask *task, void *arg);

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

void DWCUtilSubScreen_SlideInRow(int step) {
    int i;

    sSubScreenRow = DWCi_HEAPlAllocEx(sizeof(DWCUtilSubScreenRow), 4);
    for (i = 0; i < SUB_SCREEN_OBJECTS; i++) {
        sSubScreenRow->objects[i] = DWCUtilCell_AllocOamRun(1, sSubScreenCells[step][i], TRUE);
        DWCi_CELLlSetPriority(sSubScreenRow->objects[i], -1, 0);
        DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, -42, sSubScreenPositions[i][1]);
        DWCi_CELLlSetPalette(sSubScreenRow->objects[i], -1, 0, sSubScreenPalettes[step][i]);
    }
    sSubScreenRow->task = DWCi_TSKlForm(0, DWCUtilSubScreen_SlideInObject0Task, NULL, 120);
    DWCi_STEPlChangeScr(step);
    DWCUtilSound_PlaySe(0xd);
}

// The slide in: the objects from the first that hasn't stopped on move 8 pixels a frame until it stops
static void DWCUtilSubScreen_SlideInObject0Task(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCi_CELLlGetPosition(sSubScreenRow->objects[0], 0, &x, &y);
    x += 8;
    if (x < 0x20 || x > 0x100) {
        for (i = 0; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
    } else {
        x = 0x20;
        for (i = 0; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
        DWCi_TSKlChangeFunc(task, DWCUtilSubScreen_SlideInObject1Task);
    }
}

static void DWCUtilSubScreen_SlideInObject1Task(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCi_CELLlGetPosition(sSubScreenRow->objects[1], 0, &x, &y);
    x += 8;
    if (x < 0x50 || x > 0x100) {
        for (i = 1; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
    } else {
        x = 0x50;
        for (i = 1; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
        DWCi_TSKlChangeFunc(task, DWCUtilSubScreen_SlideInObject2Task);
    }
}

static void DWCUtilSubScreen_SlideInObject2Task(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCi_CELLlGetPosition(sSubScreenRow->objects[2], 0, &x, &y);
    x += 8;
    if (x < 0x68 || x > 0x100) {
        for (i = 2; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
    } else {
        x = 0x68;
        for (i = 2; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
        DWCi_TSKlChangeFunc(task, DWCUtilSubScreen_SlideInObject3Task);
    }
}

static void DWCUtilSubScreen_SlideInObject3Task(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCi_CELLlGetPosition(sSubScreenRow->objects[3], 0, &x, &y);
    x += 8;
    if (x < 0x98 || x > 0x100) {
        for (i = 3; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
    } else {
        x = 0x98;
        for (i = 3; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
        DWCi_TSKlChangeFunc(task, DWCUtilSubScreen_SlideInObject4Task);
    }
}

static void DWCUtilSubScreen_SlideInObject4Task(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    DWCi_CELLlGetPosition(sSubScreenRow->objects[4], 0, &x, &y);
    x += 8;
    if (x < 0xb0 || x > 0x100) {
        DWCi_CELLlSetPosition(sSubScreenRow->objects[4], -1, x, 0x21);
    } else {
        x = 0xb0;
        DWCi_CELLlSetPosition(sSubScreenRow->objects[4], -1, x, 0x21);
        DWCi_TSKlChangeFunc(task, DWCUtilSubScreen_SlideInDoneTask);
    }
}

static void DWCUtilSubScreen_SlideInDoneTask(DWCUtilTask *task, void *arg) {
    DWCi_TSKlDelete(0, task);
    sSubScreenRow->task = NULL;
}

void DWCi_STEPlEnd(void) {
    sSubScreenRow->leaving = TRUE;
    sSubScreenRow->task = DWCi_TSKlForm(0, DWCUtilSubScreen_SlideOutObject4Task, NULL, 120);
}

void DWCi_STEPlChange(int step) {
    int i;

    for (i = 0; i < SUB_SCREEN_OBJECTS; i += 2) {
        DWCUtilCell_Copy(1, sSubScreenCells[step][i], DWCi_CELLlGetObj(sSubScreenRow->objects[i], 0));
        DWCi_CELLlSetPriority(sSubScreenRow->objects[i], -1, 0);
        DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, sSubScreenPositions[i][0], sSubScreenPositions[i][1]);
        DWCi_CELLlSetPalette(sSubScreenRow->objects[i], -1, 0, sSubScreenPalettes[step][i]);
    }
    DWCi_STEPlChangeScr(step);
}

BOOL DWCi_STEPlGet(void) {
    if (sSubScreenRow->task != NULL) {
        return TRUE;
    }
    return FALSE;
}

// The slide out: the objects from the last that hasn't stopped on move 8 pixels a frame to the left until it reaches
// the stop of the one before it, and then all of them off the screen
static void DWCUtilSubScreen_SlideOutObject4Task(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    DWCi_CELLlGetPosition(sSubScreenRow->objects[4], 0, &x, &y);
    x -= 8;
    if (x > 0x98) {
        DWCi_CELLlSetPosition(sSubScreenRow->objects[4], -1, x, 0x21);
    } else {
        x = 0x98;
        DWCi_CELLlSetPosition(sSubScreenRow->objects[4], -1, x, 0x21);
        DWCi_TSKlChangeFunc(task, DWCUtilSubScreen_SlideOutObject3Task);
    }
}

static void DWCUtilSubScreen_SlideOutObject3Task(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCi_CELLlGetPosition(sSubScreenRow->objects[3], 0, &x, &y);
    x -= 8;
    if (x > 0x68) {
        for (i = 3; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
    } else {
        x = 0x68;
        for (i = 3; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
        DWCi_TSKlChangeFunc(task, DWCUtilSubScreen_SlideOutObject2Task);
    }
}

static void DWCUtilSubScreen_SlideOutObject2Task(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCi_CELLlGetPosition(sSubScreenRow->objects[2], 0, &x, &y);
    x -= 8;
    if (x > 0x50) {
        for (i = 2; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
    } else {
        x = 0x50;
        for (i = 2; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
        DWCi_TSKlChangeFunc(task, DWCUtilSubScreen_SlideOutObject1Task);
    }
}

static void DWCUtilSubScreen_SlideOutObject1Task(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCi_CELLlGetPosition(sSubScreenRow->objects[1], 0, &x, &y);
    x -= 8;
    if (x > 0x20) {
        for (i = 1; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
    } else {
        x = 0x20;
        for (i = 1; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
        }
        DWCi_TSKlChangeFunc(task, DWCUtilSubScreen_SlideOutObject0Task);
    }
}

// Once the row is off the left of the screen (its x wraps to 0x100 and above), frees it
static void DWCUtilSubScreen_SlideOutObject0Task(DWCUtilTask *task, void *arg) {
    int x;
    int y;
    int i;

    DWCi_CELLlGetPosition(sSubScreenRow->objects[0], 0, &x, &y);
    x -= 8;
    for (i = 0; i < SUB_SCREEN_OBJECTS; i++) {
        DWCi_CELLlSetPosition(sSubScreenRow->objects[i], -1, x, sSubScreenPositions[i][1]);
    }
    if (x <= 0x1d6 && x >= 0x100) {
        DWCi_TSKlDelete(0, task);
        for (i = 0; i < SUB_SCREEN_OBJECTS; i++) {
            DWCi_CELLlDelete(sSubScreenRow->objects[i]);
        }
        DWCi_HEAPlFree_(&sSubScreenRow);
    }
}

BOOL DWCi_STEPlIsEnd(void) {
    DWCUtilSubScreenRow *row = sSubScreenRow;

    if (row == NULL) {
        return TRUE;
    }
    if (!row->leaving) {
        return TRUE;
    }
    return FALSE;
}

void DWCi_STEPlChangeScr(int screen) {
    sSubScreenData = DWCUtilRomResource_LoadFile(DWCi_Char(sSubScreenNames[screen]), NULL, 4);
    DWCi_TSKlForm(1, DWCUtilSubScreen_LoadScreenTask, NULL, 120);
}

static void DWCUtilSubScreen_LoadScreenTask(DWCUtilTask *task, void *arg) {
    DC_FlushRange(sSubScreenData, 0x600);
    GXS_LoadBG1Scr(sSubScreenData, 0, 0x600);
    DWCi_ARClRelease(sSubScreenData);
    DWCi_TSKlDelete(1, task);
}
