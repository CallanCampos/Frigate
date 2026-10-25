#ifndef POKEBW2_DWCUTIL_TASK_H
#define POKEBW2_DWCUTIL_TASK_H

#include "types.h"
#include "struct_decls.h"

// The connection utility's tasks: two lists of callbacks in order of priority, list 0 run by the utility's main loop
// and list 1 by its VBlank handler. A task's callback is handed the task and the argument it was created with. A task
// that removes itself from its callback queues its removal, which takes place after the list has run.
// The file is the DWC utility library's task.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to that
// library's objects (layout/symbols_sdk.txt) shows it.

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

void DWCi_TSKlInit(void);
void DWCi_TSKlEnd(void);
// Runs the list's tasks, then removes the ones queued for removal
void DWCUtilTask_Run(int list);
// Adds a task, after the list's tasks of the same or a lower priority value
DWCUtilTask *DWCi_TSKlForm(int list, DWCUtilTaskCallback callback, void *arg, u8 priority);
DWCUtilTask *formTask(int list, DWCUtilTaskCallback callback, void *arg, u8 priority, u8 freeArg);
void DWCi_TSKlChangeFunc(DWCUtilTask *task, DWCUtilTaskCallback callback);
// Queues the task's removal
void DWCi_TSKlDelete(int list, DWCUtilTask *task);
void DWCi_TSKlDeleteEx(int list, DWCUtilTask *task);
void deleteTcb(int list, DWCUtilTask *task);
// Turns the running of the list's tasks on or off
void DWCi_TSKlOperate(int list, u8 active);

#endif // POKEBW2_DWCUTIL_TASK_H
