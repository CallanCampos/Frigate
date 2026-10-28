// Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

#include "field/field_task.h"
#include "gfl/heap.h"

struct FieldTask {
    u32 state;
    void *data;
    FieldTaskCallback callback;
};

static void FieldTask_SetStartedCore(FieldTask *task);
static void FieldTask_ReqEnd(FieldTask *task);
static void FieldTask_ExecCallback(FieldTask *task);

FieldTask *FieldTask_Create(HeapID heapId, u32 dataSize, FieldTaskCallback callback) {
    FieldTask *task = GFL_HeapAllocate(heapId, sizeof(FieldTask), FALSE, "field_task.c", 59);

    task->state = FIELD_TASK_STATE_WAIT;
    task->data = GFL_HeapAllocate(heapId, dataSize, TRUE, "field_task.c", 61);
    task->callback = callback;
    return task;
}

void FieldTask_Free(FieldTask *task) {
    GFL_HeapFree(task->data);
    GFL_HeapFree(task);
}

void FieldTask_Run(FieldTask *task) {
    FieldTask_ExecCallback(task);
}

void FieldTask_SetStarted(FieldTask *task) {
    FieldTask_SetStartedCore(task);
}

void *FieldTask_GetData(FieldTask *task) {
    return task->data;
}

BOOL FieldTask_CheckStateReqEnd(FieldTask *task) {
    if (task->state == FIELD_TASK_STATE_END) {
        return TRUE;
    }
    return FALSE;
}

u32 FieldTask_GetState(FieldTask *task) {
    return task->state;
}

FieldTaskCallback FieldTask_GetCallback(FieldTask *task) {
    return task->callback;
}

static void FieldTask_SetStartedCore(FieldTask *task) {
    task->state = FIELD_TASK_STATE_RUNNING;
}

static void FieldTask_ReqEnd(FieldTask *task) {
    task->state = FIELD_TASK_STATE_END;
}

static void FieldTask_ExecCallback(FieldTask *task) {
    if (task->state == FIELD_TASK_STATE_RUNNING && task->callback(task->data) == TRUE) {
        FieldTask_ReqEnd(task);
    }
}
