#include "dwc_util_ui_object.h"
#include "dwc_util_shared.h"

// The name is descriptive: overlay 182 has no embedded filename for this file.
typedef struct DWCUtilUiObject {
    void *objects[3];
    void *task;
    void *selectTask;
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

static void DWCUtilUiObject_SetPosition(int y);
static void DWCUtilUiObject_Raise(void *task);
static void DWCUtilUiObject_RaiseDone(void *task);
static void DWCUtilUiObject_Select(void *task);
static void DWCUtilUiObject_Lower(void *task);
static void DWCUtilUiObject_Destroy(void *task);
static void DWCUtilUiObject_Revert(void *task);

void DWCUtilUiObject_Init(int variant) {
    int count = sCounts[variant];
    int i;

    sUiObject = func_ov182_021bc4d8(sizeof(DWCUtilUiObject), 4);
    sUiObject->state = -2;
    sUiObject->variant = variant;
    for (i = 0; i < count; i++) {
        sUiObject->objects[i] = func_ov182_021bb97c(0, sUiResources[variant][i], 1);
        func_ov182_021bb878(sUiObject->objects[i], -1, 1);
    }
    sUiObject->objects[2] = func_ov182_021bb97c(0, 1, 1);
    func_ov182_021bb878(sUiObject->objects[2], -1, 1);
    DWCUtilUiObject_SetPosition(0xc0);
    sUiObject->task = func_ov182_021bd200(0, DWCUtilUiObject_Raise, 0, 0x78);
}

void DWCUtilUiObject_Exit(void) {
    sUiObject->exiting = TRUE;
    func_ov182_021bd264(sUiObject->task, DWCUtilUiObject_Lower);
}

int DWCUtilUiObject_GetState(void) {
    return sUiObject->state;
}

void DWCUtilUiObject_SetStateIfIdle(int state) {
    if (sUiObject->state == -1) {
        sUiObject->state = state;
    }
}

void DWCUtilUiObject_SetState(int state) {
    sUiObject->state = state;
}

BOOL DWCUtilUiObject_IsIdle(void) {
    if (sUiObject == NULL) {
        return TRUE;
    }
    if (sUiObject->exiting == FALSE) {
        return TRUE;
    }
    return FALSE;
}

void DWCUtilUiObject_Enable(void) {
    sUiObject->locked = FALSE;
}

void DWCUtilUiObject_Disable(void) {
    sUiObject->locked = TRUE;
}

static void DWCUtilUiObject_SetPosition(int y) {
    int count = sCounts[sUiObject->variant];
    int i;

    func_ov182_021bb7e0(sUiObject->objects[2], -1, 0, y);
    for (i = 0; i < count; i++) {
        func_ov182_021bb7e0(sUiObject->objects[i], -1, sPositions[sPosIndex[sUiObject->variant][i]].x, y + 4);
    }
}

static void DWCUtilUiObject_Raise(void *task) {
    int x;
    int y;

    func_ov182_021bb8bc(sUiObject->objects[2], 0, &x, &y);
    y -= 4;
    DWCUtilUiObject_SetPosition(y);
    if (y <= 0xa8) {
        DWCUtilUiObject_SetPosition(0xa8);
        func_ov182_021bd264(task, DWCUtilUiObject_RaiseDone);
    }
}

static void DWCUtilUiObject_RaiseDone(void *task) {
    sUiObject->state = -1;
    sUiObject->counter++;
    if (sUiObject->counter >= 4) {
        sUiObject->counter = 0;
        func_ov182_021bd264(task, DWCUtilUiObject_Select);
    }
}

static void DWCUtilUiObject_Select(void *task) {
    int count = sCounts[sUiObject->variant];
    int i;
    u16 rect[4];
    int resource;

    if (!sUiObject->locked) {
        if (sUiObject->state != -1) {
            return;
        }
        for (i = 0; i < count; i++) {
            func_ov182_021bc1c0(&sPositions[sPosIndex[sUiObject->variant][i]], sHitSize, rect);
            if (!func_ov182_021bc888(rect)) {
                continue;
            }
            if (sUiObject->selectTask != NULL) {
                break;
            }
            resource = sUiResources[sUiObject->variant][i];
            func_ov182_021bb914(0, resource + 1, func_ov182_021bb6ec(sUiObject->objects[i], 0));
            func_ov182_021bb7e0(sUiObject->objects[i], -1, sPositions[sPosIndex[sUiObject->variant][i]].x,
                                sPositions[sPosIndex[sUiObject->variant][i]].y);
            func_ov182_021bb878(sUiObject->objects[i], -1, 1);
            sUiObject->selectTask = func_ov182_021bd200(0, DWCUtilUiObject_Revert, 0, 0x6e);
            sUiObject->state = i;
            return;
        }
    }
    sUiObject->state = -1;
}

static void DWCUtilUiObject_Lower(void *task) {
    int x;
    int y;

    func_ov182_021bb8bc(sUiObject->objects[2], 0, &x, &y);
    y += 4;
    DWCUtilUiObject_SetPosition(y);
    if (y >= 0xc0) {
        func_ov182_021bd264(task, DWCUtilUiObject_Destroy);
    }
}

static void DWCUtilUiObject_Destroy(void *task) {
    int i;

    func_ov182_021bd268(0, task);
    if (sUiObject->selectTask != NULL) {
        func_ov182_021bd268(0, sUiObject->selectTask);
    }
    for (i = 0; i < 2; i++) {
        if (sUiObject->objects[i] != NULL) {
            func_ov182_021bb690(sUiObject->objects[i]);
        }
    }
    func_ov182_021bb690(sUiObject->objects[2]);
    func_ov182_021bc4f0(&sUiObject);
}

static void DWCUtilUiObject_Revert(void *task) {
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
        func_ov182_021bb914(0, resource, func_ov182_021bb6ec(sUiObject->objects[i], 0));
        func_ov182_021bb878(sUiObject->objects[i], -1, 1);
    }
    DWCUtilUiObject_SetPosition(0xa8);
    sUiObject->counter = 0;
    sUiObject->state = -1;
    if (sUiObject->selectTask != NULL) {
        sUiObject->selectTask = NULL;
        func_ov182_021bd268(0, task);
    }
}
