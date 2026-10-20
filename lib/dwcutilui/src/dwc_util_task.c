#include "dwc_util_task.h"
#include "dwc_util_cell.h"
#include "dwc_util_heap.h"
#include "dwc_util_list.h"
#include "nitro/os.h"

typedef struct DWCUtilTaskList {
    DWCUtilStack *freeTasks;
    // The tasks queued for removal
    DWCUtilStack *removals;
    DWCUtilList *list;
    // Priority 0 and 0xff, which bracket the list's tasks
    DWCUtilTask head;
    DWCUtilTask tail;
    int count;
    u8 active;
    DWCUtilTask *tasks;
} DWCUtilTaskList;

static const u8 sTaskCounts[2] = { 0x80, 0x20 };

static DWCUtilTaskList *sTaskLists;

void func_ov182_021bd0d4(void) {
    int i;

    sTaskLists = func_ov182_021bc4d8(sizeof(DWCUtilTaskList) * 2, 4);
    for (i = 0; i < 2; i++) {
        int count = sTaskCounts[i];

        sTaskLists[i].count = count;
        sTaskLists[i].tasks = func_ov182_021bc4d8(count * sizeof(DWCUtilTask), 4);
        sTaskLists[i].freeTasks = func_ov182_021bb9c8(count, (u8 *)sTaskLists[i].tasks, sizeof(DWCUtilTask));
        sTaskLists[i].removals = func_ov182_021bb9ac(count);
        sTaskLists[i].list = func_ov182_021bcb3c();
        sTaskLists[i].head.priority = 0;
        sTaskLists[i].tail.priority = 0xff;
        func_ov182_021bcbbc(sTaskLists[i].list, &sTaskLists[i].head);
        func_ov182_021bcbb0(sTaskLists[i].list, &sTaskLists[i].tail);
        sTaskLists[i].active = TRUE;
    }
}

void func_ov182_021bd16c(void) {
    int i;

    for (i = 0; i < 2; i++) {
        func_ov182_021bcb58(sTaskLists[i].list);
        func_ov182_021bb9ec(sTaskLists[i].freeTasks);
    }
    func_ov182_021bc4f0(&sTaskLists);
}

void func_ov182_021bd19c(int list) {
    DWCUtilTask *task;
    int i;

    if (sTaskLists[list].active) {
        for (task = sTaskLists[list].head.next; task != &sTaskLists[list].tail; task = task->next) {
            task->callback(task, task->arg);
        }
        for (i = 0; i < sTaskLists[list].count; i++) {
            task = func_ov182_021bba38(sTaskLists[list].removals);
            if (task == NULL) {
                break;
            }
            func_ov182_021bd288(list, task);
        }
    }
}

DWCUtilTask *func_ov182_021bd200(int list, DWCUtilTaskCallback callback, void *arg, u8 priority) {
    return func_ov182_021bd210(list, callback, arg, priority, FALSE);
}

DWCUtilTask *func_ov182_021bd210(int list, DWCUtilTaskCallback callback, void *arg, u8 priority, u8 freeArg) {
    DWCUtilTask *task = func_ov182_021bba38(sTaskLists[list].freeTasks);
    DWCUtilTask *position;
    u32 irq;

    task->callback = callback;
    task->arg = arg;
    task->priority = priority;
    task->freeArg = freeArg;
    irq = OS_DisableIrqMask(OS_IE_V_BLANK);
    position = sTaskLists[list].head.next;
    for (;;) {
        if (priority < position->priority) {
            func_ov182_021bcb90(position, task);
            break;
        }
        position = position->next;
    }
    OS_EnableIrqMask(irq);
    return task;
}

void func_ov182_021bd264(DWCUtilTask *task, DWCUtilTaskCallback callback) {
    task->callback = callback;
}

void func_ov182_021bd268(int list, DWCUtilTask *task) {
    func_ov182_021bba00(sTaskLists[list].removals, task);
}

void func_ov182_021bd280(int list, DWCUtilTask *task) {
    func_ov182_021bd288(list, task);
}

void func_ov182_021bd288(int list, DWCUtilTask *task) {
    if (task->freeArg) {
        func_ov182_021bc4f0(&task->arg);
    }
    func_ov182_021bcb6c(task);
    func_ov182_021bba00(sTaskLists[list].freeTasks, task);
}

void func_ov182_021bd2b8(int list, u8 active) {
    sTaskLists[list].active = active;
}
