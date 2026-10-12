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
        u32 irq = CPU_IRQDisable();
        MBTask *task;
        void (*done)(MBTask *task);
        u32 priority;
        u32 next;

        while (thread->head == NULL) {
            func_0207a998(&thread->thread, 0);
            scheduler_yield(NULL);
        }
        task = thread->head;
        thread->head = thread->head->next;
        func_0207a998(&thread->thread, task->priority);
        CPU_SetIRQMask(irq);
        if (task->func != NULL) {
            task->func(task);
        }
        irq = CPU_IRQDisable();
        done = task->done;
        priority = func_0207aa00(&thread->thread);
        if (thread->head == NULL) {
            next = 0;
        } else if (priority < thread->head->priority) {
            next = thread->head->priority;
        } else {
            next = priority;
        }
        if (next != priority) {
            func_0207a998(&thread->thread, next);
        }
        task->next = NULL;
        task->busy = 0;
        if (done != NULL) {
            done(task);
        }
        if (task == &thread->exitTask) {
            break;
        }
        CPU_SetIRQMask(irq);
    }
    scheduler_thread_exit_proc();
}

// Starts the thread, with a block of memory for it and its stack
void MBTask_StartThread(MBTaskThread *thread, u32 size) {
    u32 irq = CPU_IRQDisable();

    if (sTaskThread == NULL) {
        u32 stackSize;

        sTaskThread = thread;
        MBTask_Clear(&thread->exitTask);
        thread->head = NULL;
        stackSize = (size - sizeof(MBTaskThread) + sizeof(thread->stack)) & ~3;
        scheduler_init_thread(&thread->thread, MBTask_ThreadMain, thread, thread->stack + stackSize, stackSize, 0);
        scheduler_start_thread(&thread->thread);
    }
    CPU_SetIRQMask(irq);
}

BOOL MBTask_IsThreadStarted(void) {
    if (sTaskThread != NULL) {
        return TRUE;
    }
    return FALSE;
}

void MBTask_Clear(MBTask *task) {
    sys_memset(task, 0, sizeof(MBTask));
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
        u32 current = func_0207aa00(&thread->thread);

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
        u32 irq = CPU_IRQDisable();

        task->busy = 1;
        task->priority = priority;
        task->func = func;
        task->done = done;
        if (thread->head == NULL) {
            if (task == &thread->exitTask) {
                sTaskThread = NULL;
            }
            thread->head = task;
            scheduler_start_thread(&thread->thread);
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
        CPU_SetIRQMask(irq);
    }
}

// Ends the thread once its jobs are done, and then runs a function
void MBTask_EndThread(void (*done)(MBTask *task)) {
    u32 irq = CPU_IRQDisable();

    if (MBTask_IsThreadStarted()) {
        MBTask_Add(&sTaskThread->exitTask, NULL, done, 0);
    }
    CPU_SetIRQMask(irq);
}
