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

void DWCUtilTask_Init(void) {
    int i;

    sTaskLists = DWCUtilHeap_AllocClear(sizeof(DWCUtilTaskList) * 2, 4);
    for (i = 0; i < 2; i++) {
        int count = sTaskCounts[i];

        sTaskLists[i].count = count;
        sTaskLists[i].tasks = DWCUtilHeap_AllocClear(count * sizeof(DWCUtilTask), 4);
        sTaskLists[i].freeTasks = DWCUtilStack_CreateFilled(count, (u8 *)sTaskLists[i].tasks, sizeof(DWCUtilTask));
        sTaskLists[i].removals = DWCUtilStack_Create(count);
        sTaskLists[i].list = DWCUtilList_Create();
        sTaskLists[i].head.priority = 0;
        sTaskLists[i].tail.priority = 0xff;
        DWCUtilList_Prepend(sTaskLists[i].list, &sTaskLists[i].head);
        DWCUtilList_Append(sTaskLists[i].list, &sTaskLists[i].tail);
        sTaskLists[i].active = TRUE;
    }
}

void DWCUtilTask_Exit(void) {
    int i;

    for (i = 0; i < 2; i++) {
        DWCUtilList_Destroy(sTaskLists[i].list);
        DWCUtilStack_Destroy(sTaskLists[i].freeTasks);
    }
    DWCUtilHeap_FreeAndNull(&sTaskLists);
}

void DWCUtilTask_Run(int list) {
    DWCUtilTask *task;
    int i;

    if (sTaskLists[list].active) {
        for (task = sTaskLists[list].head.next; task != &sTaskLists[list].tail; task = task->next) {
            task->callback(task, task->arg);
        }
        for (i = 0; i < sTaskLists[list].count; i++) {
            task = DWCUtilStack_Pop(sTaskLists[list].removals);
            if (task == NULL) {
                break;
            }
            DWCUtilTask_Destroy(list, task);
        }
    }
}

DWCUtilTask *DWCUtilTask_Create(int list, DWCUtilTaskCallback callback, void *arg, u8 priority) {
    return DWCUtilTask_CreateEx(list, callback, arg, priority, FALSE);
}

DWCUtilTask *DWCUtilTask_CreateEx(int list, DWCUtilTaskCallback callback, void *arg, u8 priority, u8 freeArg) {
    DWCUtilTask *task = DWCUtilStack_Pop(sTaskLists[list].freeTasks);
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
            DWCUtilList_InsertBefore(position, task);
            break;
        }
        position = position->next;
    }
    OS_EnableIrqMask(irq);
    return task;
}

void DWCUtilTask_SetCallback(DWCUtilTask *task, DWCUtilTaskCallback callback) {
    task->callback = callback;
}

void DWCUtilTask_QueueRemoval(int list, DWCUtilTask *task) {
    DWCUtilStack_Push(sTaskLists[list].removals, task);
}

void DWCUtilTask_Remove(int list, DWCUtilTask *task) {
    DWCUtilTask_Destroy(list, task);
}

void DWCUtilTask_Destroy(int list, DWCUtilTask *task) {
    if (task->freeArg) {
        DWCUtilHeap_FreeAndNull(&task->arg);
    }
    DWCUtilList_Remove(task);
    DWCUtilStack_Push(sTaskLists[list].freeTasks, task);
}

void DWCUtilTask_SetActive(int list, u8 active) {
    sTaskLists[list].active = active;
}
