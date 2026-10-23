#include "dwc/gsCore.h"
#include "../ghttp/ghttp_internal.h"
#include "dwc/gsMemory.h"
#include "dwc/gsPlatformUtil.h"
#include <stdlib.h>

// GameSpy's core (gsCore.c). The ROM names neither the file nor its functions; both are the later SDK's

// The core: how many users started it, whether its lock was made, whether it started and whether it shuts down, the
// lock of its list and the list of its tasks
typedef struct GSCoreMgr {
    gsi_u32 mRefCount;
    // The flags are volatile because another thread sets them while gsCoreInitialize waits for them
    volatile gsi_bool critSecCreated;
    volatile gsi_bool mIsStaticInitComplete;
    volatile gsi_bool mIsShuttingDown;
    GSICriticalSection mQueueCrit;
    DArray mTaskArray;
} GSCoreMgr;

static GSCoreMgr *gsiGetStaticCore(void);
static void gsiCoreAtExitShutdown(void);
static void gsiCoreTaskDispatchCallback(GSTask *task, GSTaskResult result);
static GSTaskResult gsCoreTaskThink(GSTask *task);

static GSCoreMgr gStaticCore;

static GSCoreMgr *gsiGetStaticCore(void) {
    return &gStaticCore;
}

static void gsiCoreAtExitShutdown(void) {
    gsiDeleteCriticalSection(&gsiGetStaticCore()->mQueueCrit);
}

void gsCoreInitialize(void) {
    GSCoreMgr *core = gsiGetStaticCore();

    while (core->mIsShuttingDown) {
        msleep(100);
    }
    if (gsiInterlockedIncrement(&core->mRefCount) == 1) {
        if (!core->critSecCreated) {
            gsiInitializeCriticalSection(&core->mQueueCrit);
            atexit(gsiCoreAtExitShutdown);
            core->critSecCreated = gsi_true;
        }
        gsiEnterCriticalSection(&core->mQueueCrit);
        gsiLeaveCriticalSection(&core->mQueueCrit);
        while (core->mIsShuttingDown) {
            msleep(100);
        }
        core->mTaskArray = ArrayNew(sizeof(GSTask *), 10, NULL);
        ghttpStartup();
        core->mIsStaticInitComplete = gsi_true;
    } else {
        while (!core->critSecCreated) {
            msleep(100);
        }
        gsiEnterCriticalSection(&core->mQueueCrit);
        gsiLeaveCriticalSection(&core->mQueueCrit);
        while (!core->mIsStaticInitComplete) {
            msleep(100);
        }
    }
}

// Calls the task's callback once, with its result
static void gsiCoreTaskDispatchCallback(GSTask *task, GSTaskResult result) {
    if (task->mIsCallbackPending) {
        task->mIsCallbackPending = gsi_false;
        if (task->mCallbackFunc != NULL) {
            task->mCallbackFunc(task->mTaskData, result);
        }
    }
}

// Runs a task once: cancels it when it timed out, and calls its callback and frees it once it is done
static GSTaskResult gsCoreTaskThink(GSTask *task) {
    GSCoreMgr *core = gsiGetStaticCore();
    GSTaskResult result = GSTaskResult_None;

    if (task == NULL) {
        return GSTaskResult_Finished;
    }
    if (task->mIsRunning && task->mThinkFunc != NULL) {
        result = task->mThinkFunc(task->mTaskData);
    }
    if (!task->mIsCanceled && result == GSTaskResult_InProgress) {
        if (task->mTimeout != 0) {
            if (current_time() - task->mStartTime > task->mTimeout) {
                gsiCoreCancelTask(task);
                gsiCoreTaskDispatchCallback(task, GSTaskResult_TimedOut);
            }
        }
    } else if (result != GSTaskResult_InProgress) {
        gsi_bool cleanup = gsi_true;
        int count;
        int i;

        if (task->mIsRunning) {
            gsiCoreTaskDispatchCallback(task, result);
            task->mIsRunning = gsi_false;
        }
        if (task->mCleanupFunc != NULL) {
            cleanup = task->mCleanupFunc(task->mTaskData);
        }
        if (cleanup) {
            gsiEnterCriticalSection(&core->mQueueCrit);
            count = ArrayLength(core->mTaskArray);
            for (i = 0; i < count; i++) {
                if (*(GSTask **)ArrayNth(core->mTaskArray, i) == task) {
                    ArrayRemoveAt(core->mTaskArray, i);
                    gsifree(task);
                    break;
                }
            }
            gsiLeaveCriticalSection(&core->mQueueCrit);
            return GSTaskResult_Finished;
        }
    }
    return GSTaskResult_InProgress;
}

void gsCoreThink(gsi_time timeLimit) {
    GSCoreMgr *core = gsiGetStaticCore();
    gsi_bool allDone = gsi_true;

    if (core->mIsStaticInitComplete) {
        gsi_time startTime;
        int count;
        int i;

        gsiEnterCriticalSection(&core->mQueueCrit);
        startTime = current_time();
        count = ArrayLength(core->mTaskArray);
        if (count > 0) {
            allDone = gsi_false;
        }
        for (i = count - 1; i >= 0; i--) {
            GSTask *task = *(GSTask **)ArrayNth(core->mTaskArray, i);

            if (task->mAutoThink) {
                gsCoreTaskThink(task);
            }
            if (timeLimit != 0 && current_time() - startTime > timeLimit) {
                break;
            }
        }
        if (core->mIsShuttingDown && allDone) {
            ghttpCleanup();
            if (core->mTaskArray != NULL) {
                ArrayFree(core->mTaskArray);
                core->mTaskArray = NULL;
            }
            core->mIsShuttingDown = gsi_false;
        }
        gsiLeaveCriticalSection(&core->mQueueCrit);
    }
}

void gsCoreShutdown(void) {
    GSCoreMgr *core = gsiGetStaticCore();

    if (core->mIsStaticInitComplete) {
        gsiEnterCriticalSection(&core->mQueueCrit);
        if (gsiInterlockedDecrement(&core->mRefCount) != 0) {
            gsiLeaveCriticalSection(&core->mQueueCrit);
            return;
        }
        core->mIsShuttingDown = gsi_true;
        {
            int count = ArrayLength(core->mTaskArray);
            int i;

            for (i = 0; i < count; i++) {
                gsiCoreCancelTask(*(GSTask **)ArrayNth(core->mTaskArray, i));
            }
        }
        gsiLeaveCriticalSection(&core->mQueueCrit);
    }
}

GSCoreValue gsCoreIsShutdown(void) {
    GSCoreMgr *core = gsiGetStaticCore();

    if (core->mIsShuttingDown) {
        return GSCore_SHUTDOWN_PENDING;
    }
    if (core->mRefCount == 0) {
        return GSCore_SHUTDOWN_COMPLETE;
    }
    return GSCore_IN_USE;
}

void gsiCoreExecuteTask(GSTask *task, gsi_time timeout) {
    GSCoreMgr *core = gsiGetStaticCore();

    task->mIsCallbackPending = gsi_true;
    task->mIsStarted = gsi_true;
    task->mIsRunning = gsi_true;
    task->mTimeout = timeout;
    task->mStartTime = current_time();
    if (task->mExecuteFunc != NULL) {
        task->mExecuteFunc(task->mTaskData);
    }
    gsiEnterCriticalSection(&core->mQueueCrit);
    ArrayAppend(core->mTaskArray, &task);
    gsiLeaveCriticalSection(&core->mQueueCrit);
}

void gsiCoreCancelTask(GSTask *task) {
    GSCoreMgr *core = gsiGetStaticCore();

    gsiEnterCriticalSection(&core->mQueueCrit);
    if (task->mIsRunning && !task->mIsCanceled) {
        task->mIsCanceled = gsi_true;
        if (task->mCancelFunc != NULL) {
            task->mCancelFunc(task->mTaskData);
        }
    }
    gsiLeaveCriticalSection(&core->mQueueCrit);
}

GSTask *gsiCoreCreateTask(void) {
    GSTask *task = gsimalloc(sizeof(GSTask));

    if (task == NULL) {
        return NULL;
    }
    memset(task, 0, sizeof(GSTask));
    task->mAutoThink = gsi_true;
    return task;
}
