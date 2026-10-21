// The thread that reads the multiboot parent's program into the cache, and the jobs it runs (libmb). The names are
// descriptive
#include "mb_private.h"

// The jobs that wait for the thread to be ready, as a queue ordered by priority
#define MB_TASK_PRIORITY_MAX 0x1f
#define MB_TASK_PRIORITY_HIGHER 0x20
#define MB_TASK_PRIORITY_LOWER 0x21
#define MB_TASK_PRIORITY_KEEP 0x22

// The thread that runs the jobs, or NULL
static MBTaskThread *sTaskThread;

static void MBTask_ThreadMain(void *arg);

static void MBTask_ThreadMain(void *arg) {
    MBTaskThread *thread = arg;

    while (TRUE) {
        OSIntrMode irq = OS_DisableInterrupts();
        MBTask *task;
        void (*done)(MBTask *task);
        u32 priority;
        u32 next;

        while (thread->head == NULL) {
            OS_SetThreadPriority(&thread->thread, 0);
            OS_SleepThread(NULL);
        }
        task = thread->head;
        thread->head = thread->head->next;
        OS_SetThreadPriority(&thread->thread, task->priority);
        OS_RestoreInterrupts(irq);
        if (task->func != NULL) {
            task->func(task);
        }
        irq = OS_DisableInterrupts();
        done = task->done;
        priority = OS_GetThreadPriority(&thread->thread);
        if (thread->head == NULL) {
            next = 0;
        } else if (priority < thread->head->priority) {
            next = thread->head->priority;
        } else {
            next = priority;
        }
        if (next != priority) {
            OS_SetThreadPriority(&thread->thread, next);
        }
        task->next = NULL;
        task->busy = 0;
        if (done != NULL) {
            done(task);
        }
        if (task == &thread->exitTask) {
            break;
        }
        OS_RestoreInterrupts(irq);
    }
    OS_ExitThread();
}

// Starts the thread, with a block of memory for it and its stack
void MBTask_StartThread(MBTaskThread *thread, u32 size) {
    OSIntrMode irq = OS_DisableInterrupts();

    if (sTaskThread == NULL) {
        u32 stackSize;

        sTaskThread = thread;
        MBTask_Clear(&thread->exitTask);
        thread->head = NULL;
        stackSize = (size - sizeof(MBTaskThread) + sizeof(thread->stack)) & ~3;
        OS_CreateThread(&thread->thread, MBTask_ThreadMain, thread, thread->stack + stackSize, stackSize, 0);
        OS_WakeupThreadDirect(&thread->thread);
    }
    OS_RestoreInterrupts(irq);
}

BOOL MBTask_IsThreadStarted(void) {
    if (sTaskThread != NULL) {
        return TRUE;
    }
    return FALSE;
}

void MBTask_Clear(MBTask *task) {
    MI_CpuFill8(task, 0, sizeof(MBTask));
}

BOOL MBTask_IsBusy(const MBTask *task) {
    if (task->busy) {
        return TRUE;
    }
    return FALSE;
}

// Puts a job in the queue, after the jobs of a priority up to its own
void MBTask_Add(MBTask *task, void (*func)(MBTask *task), void (*done)(MBTask *task), u32 priority) {
    MBTaskThread *thread = sTaskThread;

    if (!MBTask_IsThreadStarted() || task->busy) {
        return;
    }
    if (priority > MB_TASK_PRIORITY_MAX) {
        u32 current = OS_GetThreadPriority(&thread->thread);

        if (priority == MB_TASK_PRIORITY_HIGHER) {
            priority = current - 1;
            if (current == 0) {
                priority = 0;
            }
        } else if (priority == MB_TASK_PRIORITY_LOWER) {
            priority = current + 1;
            if (current >= MB_TASK_PRIORITY_MAX) {
                priority = MB_TASK_PRIORITY_MAX;
            }
        } else if (priority == MB_TASK_PRIORITY_KEEP) {
            priority = current;
        } else {
            priority = MB_TASK_PRIORITY_MAX;
        }
    }
    {
        OSIntrMode irq = OS_DisableInterrupts();

        task->busy = 1;
        task->priority = priority;
        task->func = func;
        task->done = done;
        if (thread->head == NULL) {
            if (task == &thread->exitTask) {
                sTaskThread = NULL;
            }
            thread->head = task;
            OS_WakeupThreadDirect(&thread->thread);
        } else {
            MBTask *queued = thread->head;

            if (task == &thread->exitTask) {
                while (queued->next != NULL) {
                    queued = queued->next;
                }
                queued->next = task;
                sTaskThread = NULL;
            } else if (queued->priority > priority) {
                thread->head = task;
                task->next = queued;
            } else {
                MBTask *next;

                while ((next = queued->next) != NULL && next->priority <= priority) {
                    queued = next;
                }
                task->next = next;
                queued->next = task;
            }
        }
        OS_RestoreInterrupts(irq);
    }
}

// Ends the thread once its jobs are done, and then runs a function
void MBTask_EndThread(void (*done)(MBTask *task)) {
    OSIntrMode irq = OS_DisableInterrupts();

    if (MBTask_IsThreadStarted()) {
        MBTask_Add(&sTaskThread->exitTask, NULL, done, 0);
    }
    OS_RestoreInterrupts(irq);
}
