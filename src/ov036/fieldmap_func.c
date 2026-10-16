#include "types.h"
#include "field/field_async_proc.h"
#include "gfl/heap.h"
#include "gfl/overlay.h"
#include "gfl/std.h"
#include "gfl/tcb.h"

// The field's asynchronous processes: processes that run beside the field, each with a task in the field's update
// and draw task managers, and with an overlay that is loaded while it runs, if it has one. The name is the ROM's own,
// from GFL_HeapAllocate's file argument. Function names from swan (https://github.com/ds-pokemon-hacking/swan, GPL-3.0)

struct FieldAsyncProc {
    FieldAsyncProcManager *manager;
    TCB *updateTask;
    TCB *drawTask;
    const FieldAsyncProcDef *def;
    void *work;
    u32 overlayId;
};

struct FieldAsyncProcManager {
    Field *field;
    u32 heapId;
    int count;
    FieldAsyncProc *procs;
    TCBManager *updateTasks;
    TCBManager *drawTasks;
};

static void FieldAsyncProc_TCBUpdate(TCB *tcb, void *data);
static void FieldAsyncProc_TCBDraw(TCB *tcb, void *data);

FieldAsyncProcManager *FieldAsyncProcManager_Create(Field *field, HeapID heapId, u32 count) {
    FieldAsyncProcManager *manager;
    u32 size;

    manager = GFL_HeapAllocate(heapId, sizeof(FieldAsyncProcManager), TRUE, "fieldmap_func.c", 70);
    manager->field = field;
    manager->heapId = heapId;
    manager->count = count;
    manager->procs = GFL_HeapAllocate(heapId, sizeof(FieldAsyncProc) * count, TRUE, "fieldmap_func.c", 74);
    size = GFL_TCBMgrCalcAllocSize(count);
    manager->drawTasks = GFL_HeapAllocate(heapId, size, TRUE, "fieldmap_func.c", 86);
    manager->drawTasks = GFL_TCBMgrCreate(count, manager->drawTasks);
    manager->updateTasks = GFL_HeapAllocate(heapId, size, TRUE, "fieldmap_func.c", 89);
    manager->updateTasks = GFL_TCBMgrCreate(count, manager->updateTasks);
    return manager;
}

void FieldAsyncProcManager_Free(FieldAsyncProcManager *manager) {
    int i;

    for (i = 0; i < manager->count; i++) {
        FieldAsyncProc_End(&manager->procs[i]);
    }
    GFL_HeapFree(manager->procs);
    GFL_HeapFree(manager->updateTasks);
    GFL_HeapFree(manager->drawTasks);
    GFL_HeapFree(manager);
}

void FieldAsyncProcManager_Update(FieldAsyncProcManager *manager) {
    GFL_TCBMgrUpdate(manager->updateTasks);
}

void FieldAsyncProcManager_Draw(FieldAsyncProcManager *manager) {
    GFL_TCBMgrUpdate(manager->drawTasks);
}

static void FieldAsyncProc_TCBUpdate(TCB *tcb, void *data) {
    FieldAsyncProc *proc = data;

    if (proc->def->update != NULL) {
        proc->def->update(proc, proc->manager->field, proc->work);
    }
}

static void FieldAsyncProc_TCBDraw(TCB *tcb, void *data) {
    FieldAsyncProc *proc = data;

    if (proc->def->draw != NULL) {
        proc->def->draw(proc, proc->manager->field, proc->work);
    }
}

FieldAsyncProc *FieldAsyncProcManager_AddProc(u32 overlayId, FieldAsyncProcManager *manager,
                                              const FieldAsyncProcDef *def) {
    int i;
    FieldAsyncProc *proc = manager->procs;

    for (i = 0; i < manager->count; i++, proc++) {
        if (proc->updateTask == NULL) {
            if (overlayId != OVERLAY_NONE) {
                GFL_OvlLoad(overlayId);
            }
            proc->overlayId = overlayId;
            proc->updateTask = GFL_TCBMgrAddTask(manager->updateTasks, FieldAsyncProc_TCBUpdate, proc, def->priority);
            proc->drawTask = GFL_TCBMgrAddTask(manager->drawTasks, FieldAsyncProc_TCBDraw, proc, def->priority);
            proc->manager = manager;
            proc->def = def;
            if (def->workSize != 0) {
                proc->work = GFL_HeapAllocate(manager->heapId, def->workSize, TRUE, "fieldmap_func.c", 195);
            }
            if (def->init != NULL) {
                def->init(proc, manager->field, proc->work);
            }
            return proc;
        }
    }
    return NULL;
}

void FieldAsyncProc_End(FieldAsyncProc *proc) {
    if (proc->updateTask != NULL) {
        if (proc->def->free != NULL) {
            proc->def->free(proc, proc->manager->field, proc->work);
        }
        if (proc->def->workSize != 0) {
            GFL_HeapFree(proc->work);
        }
        GFL_TCBRemove(proc->updateTask);
        GFL_TCBRemove(proc->drawTask);
        if (proc->overlayId != OVERLAY_NONE) {
            GFL_OvlUnload(proc->overlayId);
        }
        MI_CpuFill8(proc, 0, sizeof(FieldAsyncProc));
    }
}

void *FieldAsyncProc_GetData(FieldAsyncProc *proc) {
    return proc->work;
}
