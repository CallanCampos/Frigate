#ifndef POKEBW2_DWC_GSCORE_H
#define POKEBW2_DWC_GSCORE_H

#include "dwc/darray.h"
#include "dwc/gsPlatformThread.h"
#include "dwc/nonport.h"

// GameSpy's core (gsCore.c), in overlay 11: the list of tasks, such as the SOAP requests of dwc/gsSoap.h, that
// gsCoreThink runs, times out and frees. A task's think function returns what its state is, which the callback is
// given once it is done

typedef enum { GSCore_IN_USE, GSCore_SHUTDOWN_PENDING, GSCore_SHUTDOWN_COMPLETE } GSCoreValue;

typedef enum {
    GSTaskResult_None,
    GSTaskResult_InProgress,
    GSTaskResult_Canceled,
    GSTaskResult_TimedOut,
    GSTaskResult_Finished
} GSTaskResult;

typedef void (*GSTaskExecuteFunc)(void *taskData);
typedef void (*GSTaskCallbackFunc)(void *taskData, GSTaskResult result);
typedef void (*GSTaskCancelFunc)(void *taskData);
typedef gsi_bool (*GSTaskCleanupFunc)(void *taskData);
typedef GSTaskResult (*GSTaskThinkFunc)(void *taskData);

// A task: its time limit and when it started, whether it started, runs and was cancelled, whether its callback is
// still to be called, and the data that its functions are given
typedef struct GSTask {
    int id;
    gsi_time timeout;
    gsi_time startTime;
    gsi_bool autoThink;
    gsi_i32 isStarted;
    gsi_i32 isRunning;
    gsi_i32 isCanceled;
    gsi_i32 isCallbackPending;
    void *taskData;
    GSTaskExecuteFunc executeFunc;
    GSTaskCallbackFunc callbackFunc;
    GSTaskCancelFunc cancelFunc;
    GSTaskCleanupFunc cleanupFunc;
    GSTaskThinkFunc thinkFunc;
} GSTask;

// Starts the core for one more user of it; runs the tasks, for at most timeLimit milliseconds unless it is 0; and ends
// it for a user, which cancels every task once the last one has ended it
void gsCoreInitialize(void);
void gsCoreThink(gsi_time timeLimit);
void gsCoreShutdown(void);
GSCoreValue gsCoreIsShutdown(void);
// Makes a task (zeroed, so that the caller sets its functions), starts it with a time limit, and cancels it
GSTask *gsiCoreCreateTask(void);
void gsiCoreExecuteTask(GSTask *task, gsi_time timeout);
void gsiCoreCancelTask(GSTask *task);

#endif // POKEBW2_DWC_GSCORE_H
