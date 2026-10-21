#include "dwc/gsCore.h"
#include "../ghttp/ghttp_internal.h"
#include "dwc/gsMemory.h"
#include "dwc/gsPlatformUtil.h"
#include <stdlib.h>

// GameSpy's core (gsCore.c). The ROM names neither the file nor its functions; both are the later SDK's, but for
// gsiCoreAtExit, a name of ours

// The core: how many users started it, whether its lock was made, whether it started and whether it shuts down, the
// lock of its list and the list of its tasks
typedef struct GSCore {
    gsi_i32 refCount;
    // The flags are volatile because another thread sets them while gsCoreInitialize waits for them
    volatile gsi_bool critSecCreated;
    volatile gsi_bool isStaticInitComplete;
    volatile gsi_bool isShuttingDown;
    GSICriticalSection queueCrit;
    DArray *taskArray;
} GSCore;

static GSCore *gsiGetStaticCore(void);
static void gsiCoreAtExit(void);
static void gsiCoreTaskDispatchCallback(GSTask *task, int result);
static int gsiCoreRunTask(GSTask *task);

static GSCore sCore;

static GSCore *gsiGetStaticCore(void) {
    return &sCore;
}

static void gsiCoreAtExit(void) {
    gsiDeleteCriticalSection(&gsiGetStaticCore()->queueCrit);
}

void gsCoreInitialize(void) {
    GSCore *core = gsiGetStaticCore();

    while (core->isShuttingDown) {
        msleep(100);
    }
    if (gsiInterlockedIncrement(&core->refCount) == 1) {
        if (!core->critSecCreated) {
            gsiInitializeCriticalSection(&core->queueCrit);
            atexit(gsiCoreAtExit);
            core->critSecCreated = gsi_true;
        }
        gsiEnterCriticalSection(&core->queueCrit);
        gsiLeaveCriticalSection(&core->queueCrit);
        while (core->isShuttingDown) {
            msleep(100);
        }
        core->taskArray = func_ov011_02171514(sizeof(GSTask *), 10, NULL);
        ghttpStartup();
        core->isStaticInitComplete = gsi_true;
    } else {
        while (!core->critSecCreated) {
            msleep(100);
        }
        gsiEnterCriticalSection(&core->queueCrit);
        gsiLeaveCriticalSection(&core->queueCrit);
        while (!core->isStaticInitComplete) {
            msleep(100);
        }
    }
}

// Calls the task's callback once, with its result
static void gsiCoreTaskDispatchCallback(GSTask *task, int result) {
    if (task->isCallbackPending) {
        task->isCallbackPending = gsi_false;
        if (task->callbackFunc != NULL) {
            task->callbackFunc(task->taskData, result);
        }
    }
}

// Runs a task once: cancels it when it timed out, and calls its callback and frees it once it is done
static int gsiCoreRunTask(GSTask *task) {
    GSCore *core = gsiGetStaticCore();
    int result = 0;

    if (task == NULL) {
        return 4;
    }
    if (task->isRunning && task->thinkFunc != NULL) {
        result = task->thinkFunc(task->taskData);
    }
    if (!task->isCanceled && result == 1) {
        if (task->timeout != 0) {
            if (current_time() - task->startTime > task->timeout) {
                gsiCoreCancelTask(task);
                gsiCoreTaskDispatchCallback(task, 3);
            }
        }
    } else if (result != 1) {
        gsi_bool cleanup = gsi_true;
        int count;
        int i;

        if (task->isRunning) {
            gsiCoreTaskDispatchCallback(task, result);
            task->isRunning = gsi_false;
        }
        if (task->cleanupFunc != NULL) {
            cleanup = task->cleanupFunc(task->taskData);
        }
        if (cleanup) {
            gsiEnterCriticalSection(&core->queueCrit);
            count = func_ov011_02171588(core->taskArray);
            for (i = 0; i < count; i++) {
                if (*(GSTask **)func_ov011_0217158c(core->taskArray, i) == task) {
                    func_ov011_02171634(core->taskArray, i);
                    gsifree(task);
                    break;
                }
            }
            gsiLeaveCriticalSection(&core->queueCrit);
            return 4;
        }
    }
    return 1;
}

void gsCoreThink(gsi_u32 timeLimit) {
    GSCore *core = gsiGetStaticCore();
    gsi_bool allDone = gsi_true;

    if (core->isStaticInitComplete) {
        gsi_u32 startTime;
        int count;
        int i;

        gsiEnterCriticalSection(&core->queueCrit);
        startTime = current_time();
        count = func_ov011_02171588(core->taskArray);
        if (count > 0) {
            allDone = gsi_false;
        }
        for (i = count - 1; i >= 0; i--) {
            GSTask *task = *(GSTask **)func_ov011_0217158c(core->taskArray, i);

            if (task->autoThink) {
                gsiCoreRunTask(task);
            }
            if (timeLimit != 0 && current_time() - startTime > timeLimit) {
                break;
            }
        }
        if (core->isShuttingDown && allDone) {
            ghttpCleanup();
            if (core->taskArray != NULL) {
                func_ov011_02171558(core->taskArray);
                core->taskArray = NULL;
            }
            core->isShuttingDown = gsi_false;
        }
        gsiLeaveCriticalSection(&core->queueCrit);
    }
}

void gsCoreShutdown(void) {
    GSCore *core = gsiGetStaticCore();

    if (core->isStaticInitComplete) {
        gsiEnterCriticalSection(&core->queueCrit);
        if (gsiInterlockedDecrement(&core->refCount) != 0) {
            gsiLeaveCriticalSection(&core->queueCrit);
            return;
        }
        core->isShuttingDown = gsi_true;
        {
            int count = func_ov011_02171588(core->taskArray);
            int i;

            for (i = 0; i < count; i++) {
                gsiCoreCancelTask(*(GSTask **)func_ov011_0217158c(core->taskArray, i));
            }
        }
        gsiLeaveCriticalSection(&core->queueCrit);
    }
}

int gsCoreIsShutdown(void) {
    GSCore *core = gsiGetStaticCore();

    if (core->isShuttingDown) {
        return 1;
    }
    if (core->refCount == 0) {
        return 2;
    }
    return 0;
}

void gsiCoreExecuteTask(GSTask *task, gsi_u32 timeout) {
    GSCore *core = gsiGetStaticCore();

    task->isCallbackPending = gsi_true;
    task->isStarted = gsi_true;
    task->isRunning = gsi_true;
    task->timeout = timeout;
    task->startTime = current_time();
    if (task->executeFunc != NULL) {
        task->executeFunc(task->taskData);
    }
    gsiEnterCriticalSection(&core->queueCrit);
    func_ov011_021715a4(core->taskArray, &task);
    gsiLeaveCriticalSection(&core->queueCrit);
}

void gsiCoreCancelTask(GSTask *task) {
    GSCore *core = gsiGetStaticCore();

    gsiEnterCriticalSection(&core->queueCrit);
    if (task->isRunning && !task->isCanceled) {
        task->isCanceled = gsi_true;
        if (task->cancelFunc != NULL) {
            task->cancelFunc(task->taskData);
        }
    }
    gsiLeaveCriticalSection(&core->queueCrit);
}

GSTask *gsiCoreCreateTask(void) {
    GSTask *task = gsimalloc(sizeof(GSTask));

    if (task == NULL) {
        return NULL;
    }
    memset(task, 0, sizeof(GSTask));
    task->autoThink = gsi_true;
    return task;
}
