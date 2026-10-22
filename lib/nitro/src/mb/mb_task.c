// The thread that reads the multiboot parent's program into the cache, and the jobs it runs (libmb). The names are
// descriptive
#include "mb_private.h"

// The jobs that wait for the thread to be ready, as a queue ordered by priority
#define MB_TASK_PRIORITY_ABOVE 0x20
#define MB_TASK_PRIORITY_BELOW 0x21
#define MB_TASK_PRIORITY_NORMAL 0x22

// The thread that runs the jobs, or NULL
static MBiTaskWork *mbi_task_work;

static void MBi_TaskThread(void *arg);

static void MBi_TaskThread(void *arg) {
    MBiTaskWork *thread = arg;

    while (TRUE) {
        OSIntrMode irq = OS_DisableInterrupts();
        MBiTaskInfo *task;
        void (*done)(MBiTaskInfo *task);
        u32 priority;
        u32 next;

        while (thread->list == NULL) {
            OS_SetThreadPriority(&thread->th, 0);
            OS_SleepThread(NULL);
        }
        task = thread->list;
        thread->list = thread->list->next;
        OS_SetThreadPriority(&thread->th, task->priority);
        OS_RestoreInterrupts(irq);
        if (task->task != NULL) {
            task->task(task);
        }
        irq = OS_DisableInterrupts();
        done = task->callback;
        priority = OS_GetThreadPriority(&thread->th);
        if (thread->list == NULL) {
            next = 0;
        } else if (priority < thread->list->priority) {
            next = thread->list->priority;
        } else {
            next = priority;
        }
        if (next != priority) {
            OS_SetThreadPriority(&thread->th, next);
        }
        task->next = NULL;
        task->busy = 0;
        if (done != NULL) {
            done(task);
        }
        if (task == &thread->end_task) {
            break;
        }
        OS_RestoreInterrupts(irq);
    }
    OS_ExitThread();
}

// Starts the thread, with a block of memory for it and its stack
void MBi_InitTaskThread(MBiTaskWork *thread, u32 size) {
    OSIntrMode irq = OS_DisableInterrupts();

    if (mbi_task_work == NULL) {
        u32 stackSize;

        mbi_task_work = thread;
        MBi_InitTaskInfo(&thread->end_task);
        thread->list = NULL;
        stackSize = (size - sizeof(MBiTaskWork) + sizeof(thread->stack)) & ~3;
        OS_CreateThread(&thread->th, MBi_TaskThread, thread, thread->stack + stackSize, stackSize, 0);
        OS_WakeupThreadDirect(&thread->th);
    }
    OS_RestoreInterrupts(irq);
}

BOOL MBi_IsTaskAvailable(void) {
    if (mbi_task_work != NULL) {
        return TRUE;
    }
    return FALSE;
}

void MBi_InitTaskInfo(MBiTaskInfo *task) {
    MI_CpuFill8(task, 0, sizeof(MBiTaskInfo));
}

BOOL MBi_IsTaskBusy(const MBiTaskInfo *task) {
    if (task->busy) {
        return TRUE;
    }
    return FALSE;
}

// Puts a job in the queue, after the jobs of a priority up to its own
void MBi_SetTask(MBiTaskInfo *task, void (*func)(MBiTaskInfo *task), void (*done)(MBiTaskInfo *task), u32 priority) {
    MBiTaskWork *thread = mbi_task_work;

    if (!MBi_IsTaskAvailable() || task->busy) {
        return;
    }
    if (priority > OS_THREAD_PRIORITY_MAX) {
        u32 current = OS_GetThreadPriority(&thread->th);

        if (priority == MB_TASK_PRIORITY_ABOVE) {
            priority = current - 1;
            if (current == 0) {
                priority = 0;
            }
        } else if (priority == MB_TASK_PRIORITY_BELOW) {
            priority = current + 1;
            if (current >= OS_THREAD_PRIORITY_MAX) {
                priority = OS_THREAD_PRIORITY_MAX;
            }
        } else if (priority == MB_TASK_PRIORITY_NORMAL) {
            priority = current;
        } else {
            priority = OS_THREAD_PRIORITY_MAX;
        }
    }
    {
        OSIntrMode irq = OS_DisableInterrupts();

        task->busy = 1;
        task->priority = priority;
        task->task = func;
        task->callback = done;
        if (thread->list == NULL) {
            if (task == &thread->end_task) {
                mbi_task_work = NULL;
            }
            thread->list = task;
            OS_WakeupThreadDirect(&thread->th);
        } else {
            MBiTaskInfo *queued = thread->list;

            if (task == &thread->end_task) {
                while (queued->next != NULL) {
                    queued = queued->next;
                }
                queued->next = task;
                mbi_task_work = NULL;
            } else if (queued->priority > priority) {
                thread->list = task;
                task->next = queued;
            } else {
                MBiTaskInfo *next;

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
void MBi_EndTaskThread(void (*done)(MBiTaskInfo *task)) {
    OSIntrMode irq = OS_DisableInterrupts();

    if (MBi_IsTaskAvailable()) {
        MBi_SetTask(&mbi_task_work->end_task, NULL, done, 0);
    }
    OS_RestoreInterrupts(irq);
}
