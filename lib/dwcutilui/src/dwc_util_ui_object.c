#include "dwc_util_ui_object.h"
#include "dwc_util_shared.h"

// The name is descriptive: overlay 182 has no embedded filename for this file.
typedef struct DWCUtilUiObject {
    void *objects[3];
    DWCUtilTask *task;
    DWCUtilTask *selectTask;
    u16 counter;
    s8 state;
    u8 variant;
    u8 locked;
    u8 exiting;
} DWCUtilUiObject;

typedef struct DWCUtilUiObjectPosition {
    u16 x;
    u16 y;
} DWCUtilUiObjectPosition;

// Per variant: how many objects it shows, where each of them sits, and which
// resource each of them uses. The hit rectangle the selection tests is 120x16.
// Declared in the order that gives the ROM's layout: MWCC sorts the tables by
// size, and the two pairs of equal size come out in an order that depends on it.
static const u16 sHitSize[2] = { 120, 16 };
static const DWCUtilUiObjectPosition sPositions[2] = { { 8, 172 }, { 132, 172 } };
static const u8 sCounts[8] = { 2, 1, 1, 2, 1, 1, 2, 0 };
static const u8 sUiResources[7][2] = {
    { 0x27, 0x1f }, { 0x25, 0x00 }, { 0x27, 0x00 }, { 0x23, 0x1d }, { 0x21, 0x00 }, { 0x59, 0x00 }, { 0x27, 0x21 },
};
static const u8 sPosIndex[7][2] = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 }, { 1, 0 }, { 1, 0 }, { 0, 1 } };

static DWCUtilUiObject *sUiObject;

static void disp(int y);
static void DWCUtilUiObject_Raise(DWCUtilTask *task, void *arg);
static void DWCUtilUiObject_RaiseDone(DWCUtilTask *task, void *arg);
static void DWCUtilUiObject_Select(DWCUtilTask *task, void *arg);
static void DWCUtilUiObject_Lower(DWCUtilTask *task, void *arg);
static void DWCUtilUiObject_Destroy(DWCUtilTask *task, void *arg);
static void DWCUtilUiObject_Revert(DWCUtilTask *task, void *arg);

void DWCi_BTNlInit(int variant) {
    int count = sCounts[variant];
    int i;

    sUiObject = DWCi_HEAPlAllocEx(sizeof(DWCUtilUiObject), 4);
    sUiObject->state = -2;
    sUiObject->variant = variant;
    for (i = 0; i < count; i++) {
        sUiObject->objects[i] = DWCUtilCell_AllocOamRun(0, sUiResources[variant][i], 1);
        DWCi_CELLlSetPriority(sUiObject->objects[i], -1, 1);
    }
    sUiObject->objects[2] = DWCUtilCell_AllocOamRun(0, 1, 1);
    DWCi_CELLlSetPriority(sUiObject->objects[2], -1, 1);
    disp(0xc0);
    sUiObject->task = DWCi_TSKlForm(0, DWCUtilUiObject_Raise, NULL, 0x78);
}

void DWCi_BTNlEnd(void) {
    sUiObject->exiting = TRUE;
    DWCi_TSKlChangeFunc(sUiObject->task, DWCUtilUiObject_Lower);
}

int DWCi_BTNlGet(void) {
    return sUiObject->state;
}

void DWCi_BTNlSet(int state) {
    if (sUiObject->state == -1) {
        sUiObject->state = state;
    }
}

void DWCi_BTNlSetForce(int state) {
    sUiObject->state = state;
}

BOOL DWCi_BTNlIsEnd(void) {
    if (sUiObject == NULL) {
        return TRUE;
    }
    if (sUiObject->exiting == FALSE) {
        return TRUE;
    }
    return FALSE;
}

void DWCi_BTNlEnable(void) {
    sUiObject->locked = FALSE;
}

void DWCi_BTNlDisable(void) {
    sUiObject->locked = TRUE;
}

static void disp(int y) {
    int count = sCounts[sUiObject->variant];
    int i;

    DWCi_CELLlSetPosition(sUiObject->objects[2], -1, 0, y);
    for (i = 0; i < count; i++) {
        DWCi_CELLlSetPosition(sUiObject->objects[i], -1, sPositions[sPosIndex[sUiObject->variant][i]].x, y + 4);
    }
}

static void DWCUtilUiObject_Raise(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    DWCi_CELLlGetPosition(sUiObject->objects[2], 0, &x, &y);
    y -= 4;
    disp(y);
    if (y <= 0xa8) {
        disp(0xa8);
        DWCi_TSKlChangeFunc(task, DWCUtilUiObject_RaiseDone);
    }
}

static void DWCUtilUiObject_RaiseDone(DWCUtilTask *task, void *arg) {
    sUiObject->state = -1;
    sUiObject->counter++;
    if (sUiObject->counter >= 4) {
        sUiObject->counter = 0;
        DWCi_TSKlChangeFunc(task, DWCUtilUiObject_Select);
    }
}

static void DWCUtilUiObject_Select(DWCUtilTask *task, void *arg) {
    int count = sCounts[sUiObject->variant];
    int i;
    u16 rect[4];
    int resource;

    if (!sUiObject->locked) {
        if (sUiObject->state != -1) {
            return;
        }
        for (i = 0; i < count; i++) {
            DWCUtilDisplay_SetRectFromSize(&sPositions[sPosIndex[sUiObject->variant][i]], sHitSize, rect);
            if (!DWCi_TPlCheckTrigger(rect)) {
                continue;
            }
            if (sUiObject->selectTask != NULL) {
                break;
            }
            resource = sUiResources[sUiObject->variant][i];
            DWCUtilCell_Copy(0, resource + 1, DWCi_CELLlGetObj(sUiObject->objects[i], 0));
            DWCi_CELLlSetPosition(sUiObject->objects[i], -1, sPositions[sPosIndex[sUiObject->variant][i]].x,
                                  sPositions[sPosIndex[sUiObject->variant][i]].y);
            DWCi_CELLlSetPriority(sUiObject->objects[i], -1, 1);
            sUiObject->selectTask = DWCi_TSKlForm(0, DWCUtilUiObject_Revert, NULL, 0x6e);
            sUiObject->state = i;
            return;
        }
    }
    sUiObject->state = -1;
}

static void DWCUtilUiObject_Lower(DWCUtilTask *task, void *arg) {
    int x;
    int y;

    DWCi_CELLlGetPosition(sUiObject->objects[2], 0, &x, &y);
    y += 4;
    disp(y);
    if (y >= 0xc0) {
        DWCi_TSKlChangeFunc(task, DWCUtilUiObject_Destroy);
    }
}

static void DWCUtilUiObject_Destroy(DWCUtilTask *task, void *arg) {
    int i;

    DWCi_TSKlDelete(0, task);
    if (sUiObject->selectTask != NULL) {
        DWCi_TSKlDelete(0, sUiObject->selectTask);
    }
    for (i = 0; i < 2; i++) {
        if (sUiObject->objects[i] != NULL) {
            DWCi_CELLlDelete(sUiObject->objects[i]);
        }
    }
    DWCi_CELLlDelete(sUiObject->objects[2]);
    DWCi_HEAPlFree_(&sUiObject);
}

static void DWCUtilUiObject_Revert(DWCUtilTask *task, void *arg) {
    int i;
    int count;
    int resource;

    sUiObject->counter++;
    if (sUiObject->counter < 0x10) {
        return;
    }
    count = sCounts[sUiObject->variant];
    for (i = 0; i < count; i++) {
        resource = sUiResources[sUiObject->variant][i];
        DWCUtilCell_Copy(0, resource, DWCi_CELLlGetObj(sUiObject->objects[i], 0));
        DWCi_CELLlSetPriority(sUiObject->objects[i], -1, 1);
    }
    disp(0xa8);
    sUiObject->counter = 0;
    sUiObject->state = -1;
    if (sUiObject->selectTask != NULL) {
        sUiObject->selectTask = NULL;
        DWCi_TSKlDelete(0, task);
    }
}
