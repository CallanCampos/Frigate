#ifndef POKEBW2_DWC_GSCORE_H
#define POKEBW2_DWC_GSCORE_H

#include "dwc/darray.h"
#include "dwc/gsPlatformThread.h"
#include "dwc/nonport.h"

// GameSpy's core (gsCore.c), in overlay 11: the list of tasks, such as the SOAP requests of dwc/gsSoap.h, that
// gsCoreThink runs, times out and frees. A task's think function returns what its state is: 1 while it runs, and 2
// (cancelled), 3 (timed out) or 4 (finished) once it is done, which the callback is given

typedef struct GSTask GSTask;
typedef void (*GSTaskExecuteFunc)(void *taskData);
typedef void (*GSTaskCallbackFunc)(void *taskData, int result);
typedef void (*GSTaskCancelFunc)(void *taskData);
typedef gsi_bool (*GSTaskCleanupFunc)(void *taskData);
typedef int (*GSTaskThinkFunc)(void *taskData);

// A task: its time limit and when it started, whether it started, runs and was cancelled, whether its callback is
// still to be called, and the data that its functions are given
struct GSTask {
    int id;
    gsi_u32 timeout;
    gsi_u32 startTime;
    gsi_bool autoThink;
    gsi_bool isStarted;
    gsi_bool isRunning;
    gsi_bool isCanceled;
    gsi_bool isCallbackPending;
    void *taskData;
    GSTaskExecuteFunc executeFunc;
    GSTaskCallbackFunc callbackFunc;
    GSTaskCancelFunc cancelFunc;
    GSTaskCleanupFunc cleanupFunc;
    GSTaskThinkFunc thinkFunc;
};

// Starts the core for one more user of it; runs the tasks, for at most timeLimit milliseconds unless it is 0; and ends
// it for a user, which cancels every task once the last one has ended it
void gsCoreInitialize(void);
void gsCoreThink(gsi_u32 timeLimit);
void gsCoreShutdown(void);
// 0 while the core is in use, 1 while it shuts down and 2 once it is shut down
int gsCoreIsShutdown(void);
// Makes a task (zeroed, so that the caller sets its functions), starts it with a time limit, and cancels it
GSTask *gsiCoreCreateTask(void);
void gsiCoreExecuteTask(GSTask *task, gsi_u32 timeout);
void gsiCoreCancelTask(GSTask *task);

#endif // POKEBW2_DWC_GSCORE_H
