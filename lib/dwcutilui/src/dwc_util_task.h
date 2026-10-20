#ifndef POKEBW2_DWCUTIL_DWC_UTIL_TASK_H
#define POKEBW2_DWCUTIL_DWC_UTIL_TASK_H

#include "types.h"
#include "struct_decls.h"

// The connection utility's tasks: two lists of callbacks in order of priority, list 0 run by the utility's main loop
// and list 1 by its VBlank handler. A task's callback is handed the task and the argument it was created with. A task
// that removes itself from its callback queues its removal, which takes place after the list has run.
// The ROM does not embed this source file's name; dwc_util_task.c is descriptive.

typedef void (*DWCUtilTaskCallback)(DWCUtilTask *task, void *arg);

struct DWCUtilTask {
    DWCUtilTask *prev;
    DWCUtilTask *next;
    DWCUtilTaskCallback callback;
    void *arg;
    u8 priority;
    // Whether removing the task frees its argument
    u8 freeArg;
};

void DWCUtilTask_Init(void);
void DWCUtilTask_Exit(void);
// Runs the list's tasks, then removes the ones queued for removal
void DWCUtilTask_Run(int list);
// Adds a task, after the list's tasks of the same or a lower priority value
DWCUtilTask *DWCUtilTask_Create(int list, DWCUtilTaskCallback callback, void *arg, u8 priority);
DWCUtilTask *DWCUtilTask_CreateEx(int list, DWCUtilTaskCallback callback, void *arg, u8 priority, u8 freeArg);
void DWCUtilTask_SetCallback(DWCUtilTask *task, DWCUtilTaskCallback callback);
// Queues the task's removal
void DWCUtilTask_QueueRemoval(int list, DWCUtilTask *task);
void DWCUtilTask_Remove(int list, DWCUtilTask *task);
void DWCUtilTask_Destroy(int list, DWCUtilTask *task);
// Turns the running of the list's tasks on or off
void DWCUtilTask_SetActive(int list, u8 active);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_TASK_H
