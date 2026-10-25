#include "task.h"
#include "deque.h"
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

void DWCi_TSKlInit(void) {
    int i;

    sTaskLists = DWCi_HEAPlAllocEx(sizeof(DWCUtilTaskList) * 2, 4);
    for (i = 0; i < 2; i++) {
        int count = sTaskCounts[i];

        sTaskLists[i].count = count;
        sTaskLists[i].tasks = DWCi_HEAPlAllocEx(count * sizeof(DWCUtilTask), 4);
        sTaskLists[i].freeTasks = DWCi_QUElFormEx(count, (u8 *)sTaskLists[i].tasks, sizeof(DWCUtilTask));
        sTaskLists[i].removals = DWCi_QUElForm(count);
        sTaskLists[i].list = DWCi_LSTlForm();
        sTaskLists[i].head.priority = 0;
        sTaskLists[i].tail.priority = 0xff;
        DWCi_LSTlPushFront(sTaskLists[i].list, &sTaskLists[i].head);
        DWCi_LSTlPushBack(sTaskLists[i].list, &sTaskLists[i].tail);
        sTaskLists[i].active = TRUE;
    }
}

void DWCi_TSKlEnd(void) {
    int i;

    for (i = 0; i < 2; i++) {
        DWCi_LSTlDelete(sTaskLists[i].list);
        DWCi_QUElDelete(sTaskLists[i].freeTasks);
    }
    DWCi_HEAPlFree_(&sTaskLists);
}

void DWCUtilTask_Run(int list) {
    DWCUtilTask *task;
    int i;

    if (sTaskLists[list].active) {
        for (task = sTaskLists[list].head.next; task != &sTaskLists[list].tail; task = task->next) {
            task->callback(task, task->arg);
        }
        for (i = 0; i < sTaskLists[list].count; i++) {
            task = DWCi_QUElPopFront(sTaskLists[list].removals);
            if (task == NULL) {
                break;
            }
            deleteTcb(list, task);
        }
    }
}

DWCUtilTask *DWCi_TSKlForm(int list, DWCUtilTaskCallback callback, void *arg, u8 priority) {
    return formTask(list, callback, arg, priority, FALSE);
}

DWCUtilTask *formTask(int list, DWCUtilTaskCallback callback, void *arg, u8 priority, u8 freeArg) {
    DWCUtilTask *task = DWCi_QUElPopFront(sTaskLists[list].freeTasks);
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
            DWCi_LSTlInsert(position, task);
            break;
        }
        position = position->next;
    }
    OS_EnableIrqMask(irq);
    return task;
}

void DWCi_TSKlChangeFunc(DWCUtilTask *task, DWCUtilTaskCallback callback) {
    task->callback = callback;
}

void DWCi_TSKlDelete(int list, DWCUtilTask *task) {
    DWCi_QUElPushBack(sTaskLists[list].removals, task);
}

void DWCi_TSKlDeleteEx(int list, DWCUtilTask *task) {
    deleteTcb(list, task);
}

void deleteTcb(int list, DWCUtilTask *task) {
    if (task->freeArg) {
        DWCi_HEAPlFree_(&task->arg);
    }
    DWCi_LSTlErase(task);
    DWCi_QUElPushBack(sTaskLists[list].freeTasks, task);
}

void DWCi_TSKlOperate(int list, u8 active) {
    sTaskLists[list].active = active;
}
