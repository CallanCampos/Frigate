// NitroSDK OS: threads (os_thread.c, the SDK's name for the module). The scheduler keeps a list of all threads sorted
// by priority and runs the first one that is ready, switching by saving and loading contexts. The thread that is
// running when the system starts is the launcher thread, and the idle thread, the least urgent, halts the CPU until an
// interrupt. Built with -ipa file: the file's statics are reached from one base, and the data that code the link
// removed used is kept as words and fields without a name.
#include "nitro/mi.h"
#include "nitro/os.h"

static u32 func_0207a2e4(void);
static void scheduler_thread_list_append(OSThreadQueue *queue, OSThread *thread);
static OSThread *OSi_RemoveLinkFromQueue(OSThreadQueue *queue);
static OSThread *OSi_RemoveSpecifiedLinkFromQueue(OSThreadQueue *queue, OSThread *thread);
static void func_0207a3d0(OSThread *thread);
static void func_0207a408(OSThread *thread);
static void scheduler_switch_thread(void);
static void scheduler_exit_thread(OSThread *thread, void *arg);
static void scheduler_release_thread(void *arg);
static void scheduler_release_thread_core(void);
static void func_0207a780(OSThread *thread, void *arg, u32 priority);
static void func_0207a7f0(OSThread *thread);
static OSThread *func_0207a900(void);
static void func_0207aa74(void *arg);
static void func_0207aaa8(void *arg);

// Where the running thread is: the thread info's current. Some of the code reads it through here
static OSThread **sCurrentThreadPtr;
// The function that is called with the old and the new thread at each switch, before the one in the thread info
static OSSwitchThreadCallback sSwitchHook;
// How many times the scheduler has been locked (NitroSDK's OS_DisableScheduler), which stops it from switching
static u32 sSchedulerLockCount;
// Whether OS_InitThread has run (NitroSDK's OS_IsThreadAvailable), which other code reads
BOOL data_0214c21c;
// A word that code the link removed used, the stack that a thread that ends runs the end on when it is not zero, and
// the last ID that was given to a thread
static struct {
    u32 unk_00;
    u32 exitStack;
    u32 idCount;
} sThreadState;
// NitroSDK's OSi_ThreadInfo
OSThreadInfo g_IsThreadSwitchRequested;
static OSThread OSi_IdleThread;
static OSThread OSi_LauncherThread;
static u8 OSi_IdleThreadStack[0xc8];

// NitroSDK's OSi_GetUnusedThreadId
static u32 func_0207a2e4(void) {
    return ++sThreadState.idCount;
}

// NitroSDK's OSi_InsertLinkToQueue: puts a thread in the queue after the ones of the same or a more urgent priority
static void scheduler_thread_list_append(OSThreadQueue *queue, OSThread *thread) {
    OSThread *next = queue->head;
    OSThread *prev;

    while (next != NULL && next->priority <= thread->priority) {
        if (next == thread) {
            return;
        }
        next = next->link.next;
    }

    if (next == NULL) {
        prev = queue->tail;
        if (prev == NULL) {
            queue->head = thread;
        } else {
            prev->link.next = thread;
        }
        thread->link.prev = prev;
        thread->link.next = NULL;
        queue->tail = thread;
    } else {
        prev = next->link.prev;
        if (prev == NULL) {
            queue->head = thread;
        } else {
            prev->link.next = thread;
        }
        thread->link.prev = prev;
        thread->link.next = next;
        next->link.prev = thread;
    }
}

// NitroSDK's OSi_RemoveLinkFromQueue: takes the first thread out of the queue
static OSThread *OSi_RemoveLinkFromQueue(OSThreadQueue *queue) {
    OSThread *thread = queue->head;

    if (thread != NULL) {
        OSThread *next = thread->link.next;

        queue->head = next;
        if (next != NULL) {
            next->link.prev = NULL;
        } else {
            queue->tail = NULL;
            thread->queue = NULL;
        }
    }
    return thread;
}

// NitroSDK's OSi_RemoveSpecifiedLinkFromQueue: takes a thread out of the queue, and gives it back if it was there
static OSThread *OSi_RemoveSpecifiedLinkFromQueue(OSThreadQueue *queue, OSThread *thread) {
    OSThread *found;
    OSThread *next;
    OSThread *prev;

    for (found = queue->head; found != NULL; found = next) {
        next = found->link.next;

        if (found == thread) {
            prev = found->link.prev;

            if (queue->head == found) {
                queue->head = next;
            } else {
                prev->link.next = next;
            }
            if (queue->tail == found) {
                queue->tail = prev;
            } else {
                next->link.prev = prev;
            }
            break;
        }
    }
    return found;
}

// NitroSDK's OSi_RemoveMutexLinkFromQueue: takes the first mutex out of a thread's list of them
OSMutex *OSi_RemoveMutexLinkFromQueue(OSMutexQueue *queue) {
    OSMutex *mutex = queue->head;

    if (mutex != NULL) {
        OSMutex *next = mutex->link.next;

        queue->head = next;
        if (next != NULL) {
            next->link.prev = NULL;
        } else {
            queue->tail = NULL;
        }
    }
    return mutex;
}

// NitroSDK's OSi_InsertThreadToList: puts a thread in the list of all of them, before the ones that are less urgent
static void func_0207a3d0(OSThread *thread) {
    OSThread *next;
    OSThread *prev = NULL;
    OSThread *head = g_IsThreadSwitchRequested.list;

    next = head;
    while (next != NULL && next->priority < thread->priority) {
        prev = next;
        next = next->next;
    }

    if (prev == NULL) {
        thread->next = head;
        g_IsThreadSwitchRequested.list = thread;
    } else {
        thread->next = prev->next;
        prev->next = thread;
    }
}

// NitroSDK's OSi_RemoveThreadFromList
static void func_0207a408(OSThread *thread) {
    OSThread *prev = NULL;
    OSThread *next = g_IsThreadSwitchRequested.list;

    while (next != NULL && next != thread) {
        prev = next;
        next = next->next;
    }

    if (prev == NULL) {
        g_IsThreadSwitchRequested.list = thread->next;
    } else {
        prev->next = thread->next;
    }
}

// NitroSDK's OSi_RescheduleThread: switches to the first thread that is ready, unless the scheduler is locked or an
// interrupt is being handled, when it only notes that a switch is wanted
static void scheduler_switch_thread(void) {
    OSThread *current;
    OSThread *next;

    if (sSchedulerLockCount != 0) {
        return;
    }

    // The flag and the callback are reached through a pointer to the thread info and the rest of it by name, which is
    // how the original has two literals for it
    OSThreadInfo *info = &g_IsThreadSwitchRequested;

    if (g_IsThreadSwitchRequested.irqDepth != 0 || CPU_GetMode() == 0x12) {
        info->isNeedRescheduling = TRUE;
        return;
    }

    current = *sCurrentThreadPtr;
    next = func_0207a900();
    if (current == next || next == NULL) {
        return;
    }

    if (current->state != OS_THREAD_STATE_TERMINATED) {
        if (OS_SaveContext(&current->context)) {
            return;
        }
    }
    if (sSwitchHook != NULL) {
        sSwitchHook(current, next);
    }
    if (info->switchCallback != NULL) {
        info->switchCallback(current, next);
    }
    g_IsThreadSwitchRequested.current = next;
    func_0207abbc(&next->context);
}

// NitroSDK's OS_InitThread: makes the running thread the launcher thread, with the system stack, and creates the idle
// thread
void scheduler_init(void) {
    u32 stackTop;

    if (data_0214c21c) {
        return;
    }
    data_0214c21c = TRUE;
    sCurrentThreadPtr = &g_IsThreadSwitchRequested.current;

    OSi_LauncherThread.priority = OS_THREAD_LAUNCHER_PRIORITY;
    OSi_LauncherThread.id = 0;
    OSi_LauncherThread.state = OS_THREAD_STATE_READY;
    OSi_LauncherThread.next = NULL;
    OSi_LauncherThread.profiler = NULL;
    g_IsThreadSwitchRequested.current = g_IsThreadSwitchRequested.list = &OSi_LauncherThread;

    if (HW_SYS_STACK_SIZE <= 0) {
        stackTop = HW_DTCM_SYSRV_END - HW_SYS_STACK_SIZE;
    } else {
        stackTop = HW_IRQ_STACK_END - HW_SYS_STACK_SIZE;
    }
    OSi_LauncherThread.stackBottom = HW_IRQ_STACK_END;
    OSi_LauncherThread.stackTop = stackTop;
    OSi_LauncherThread.stackWarningOffset = 0;
    *(u32 *)(OSi_LauncherThread.stackBottom - 8) = OS_STACK_BEGIN_MAGIC;
    *(u32 *)OSi_LauncherThread.stackTop = OS_STACK_END_MAGIC;
    OS_InitThreadQueue(&OSi_LauncherThread.joinQueue);

    g_IsThreadSwitchRequested.isNeedRescheduling = FALSE;
    g_IsThreadSwitchRequested.irqDepth = 0;
    *(OSThreadInfo **)HW_THREADINFO_MAIN = &g_IsThreadSwitchRequested;
    func_0207aa8c(NULL);

    scheduler_init_thread(&OSi_IdleThread, func_0207aaa8, NULL, OSi_IdleThreadStack + sizeof(OSi_IdleThreadStack),
                          sizeof(OSi_IdleThreadStack), OS_THREAD_PRIORITY_MAX);
    OSi_IdleThread.priority = OS_THREAD_PRIORITY_MAX + 1;
    OSi_IdleThread.state = OS_THREAD_STATE_READY;
}

// NitroSDK's OS_IsThreadAvailable
BOOL func_0207a57c(void) {
    return data_0214c21c;
}

// NitroSDK's OS_CreateThread: sets up a thread, which stays asleep until it is started. The stack is where it begins,
// and the end of the function makes the thread exit
void scheduler_init_thread(OSThread *thread, void (*func)(void *), void *arg, void *stack, u32 stackSize, u32 prio) {
    u32 enabled = CPU_IRQDisable();
    u32 stackTop;

    thread->id = func_0207a2e4();
    thread->state = OS_THREAD_STATE_WAITING;
    thread->priority = prio;
    thread->profiler = NULL;
    func_0207a3d0(thread);

    thread->stackBottom = (u32)stack;
    stackTop = (u32)stack - stackSize;
    thread->stackTop = stackTop;
    thread->stackWarningOffset = 0;
    *(u32 *)(thread->stackBottom - 8) = OS_STACK_BEGIN_MAGIC;
    *(u32 *)thread->stackTop = OS_STACK_END_MAGIC;
    OS_InitThreadQueue(&thread->joinQueue);

    OS_InitContext(&thread->context, (u32)func, (u32)stack - 8);
    thread->context.r[0] = (u32)arg;
    thread->context.lr = (u32)scheduler_thread_exit_proc;
    MI_CpuClear32((void *)(stackTop + 4), stackSize - 12);

    thread->mutex = NULL;
    thread->mutexQueue.head = NULL;
    thread->mutexQueue.tail = NULL;
    func_0207aafc(thread, NULL);
    thread->queue = NULL;
    thread->link.prev = NULL;
    thread->link.next = NULL;
    MI_CpuClear32(thread->specific, sizeof(thread->specific));
    thread->alarmForSleep = NULL;

    CPU_SetIRQMask(enabled);
}

// NitroSDK's OSi_ExitThread: what a thread returns to when its function ends
void scheduler_thread_exit_proc(void) {
    CPU_IRQDisable();
    scheduler_exit_thread(g_IsThreadSwitchRequested.current, NULL);
}

// NitroSDK's OSi_ExitThread_ArgSpecified: makes a thread continue with the end of it, on the exit stack if there is one
static void scheduler_exit_thread(OSThread *thread, void *arg) {
    if (sThreadState.exitStack != 0) {
        OS_InitContext(&thread->context, (u32)scheduler_release_thread, sThreadState.exitStack);
        thread->context.cpsr |= 0x80;
        thread->state = OS_THREAD_STATE_READY;
        thread->context.r[0] = (u32)arg;
        func_0207abbc(&thread->context);
    } else {
        scheduler_release_thread(arg);
    }
}

// NitroSDK's OSi_ExitThread_Destroy: the end of a thread, which calls its destructor first
static void scheduler_release_thread(void *arg) {
    OSThread *thread = *sCurrentThreadPtr;
    OSThreadDestructor destructor = thread->destructor;

    if (destructor != NULL) {
        thread->destructor = NULL;
        destructor(arg);
        CPU_IRQDisable();
    }
    scheduler_release_thread_core();
}

static void scheduler_release_thread_core(void) {
    OSThread *thread = *sCurrentThreadPtr;

    OS_DisableScheduler();
    func_0207aeb0(thread);
    if (thread->queue != NULL) {
        OSi_RemoveSpecifiedLinkFromQueue(thread->queue, thread);
    }
    func_0207a408(thread);
    thread->state = OS_THREAD_STATE_TERMINATED;
    OS_WakeupThread(&thread->joinQueue);
    OS_EnableScheduler();
    OS_RescheduleThread();
    sys_exit();
}

// NitroSDK's OS_DestroyThread: ends a thread, waking the ones that wait for it
void func_0207a710(OSThread *thread) {
    u32 enabled = CPU_IRQDisable();

    if (g_IsThreadSwitchRequested.current == thread) {
        scheduler_release_thread_core();
    }
    OS_DisableScheduler();
    func_0207aeb0(thread);
    func_0207a7f0(thread);
    if (thread->queue != NULL) {
        OSi_RemoveSpecifiedLinkFromQueue(thread->queue, thread);
    }
    func_0207a408(thread);
    thread->state = OS_THREAD_STATE_TERMINATED;
    OS_WakeupThread(&thread->joinQueue);
    OS_EnableScheduler();
    CPU_SetIRQMask(enabled);
    OS_RescheduleThread();
}

// NitroSDK's OS_KillThread: makes a thread end, with a value for its destructor
void func_0207a768(OSThread *thread, void *arg) {
    func_0207a780(thread, arg, func_0207aa00(thread));
}

// NitroSDK's OSi_KillThreadWithPriority: makes the end of a thread run next, at the priority
static void func_0207a780(OSThread *thread, void *arg, u32 priority) {
    u32 enabled = CPU_IRQDisable();
    u32 stack;

    if (thread == g_IsThreadSwitchRequested.current) {
        scheduler_exit_thread(thread, arg);
    }
    func_0207a7f0(thread);

    stack = sThreadState.exitStack;
    if (stack == 0) {
        stack = thread->stackBottom - 8;
    }
    OS_InitContext(&thread->context, (u32)scheduler_release_thread, stack);
    thread->context.cpsr |= 0x80;
    thread->context.r[0] = (u32)arg;
    thread->state = OS_THREAD_STATE_READY;

    OS_DisableScheduler();
    func_0207a998(thread, priority);
    OS_EnableScheduler();
    scheduler_switch_thread();
    CPU_SetIRQMask(enabled);
}

// Cancels the alarm that a thread sleeps with, if it has one
static void func_0207a7f0(OSThread *thread) {
    if (thread->alarmForSleep != NULL) {
        OS_CancelAlarm(thread->alarmForSleep);
    }
}

// Waits for a thread to end
void OS_JoinThread(OSThread *thread) {
    u32 enabled = CPU_IRQDisable();

    while (thread->state != OS_THREAD_STATE_TERMINATED) {
        scheduler_yield(&thread->joinQueue);
    }
    CPU_SetIRQMask(enabled);
}

// NitroSDK's OS_IsThreadTerminated
BOOL scheduler_is_thread_ended(const OSThread *thread) {
    if (thread->state == OS_THREAD_STATE_TERMINATED) {
        return TRUE;
    }
    return FALSE;
}

// TwlSDK's OS_SleepThreadDirect: puts a thread to sleep in a queue
void scheduler_join_thread(OSThread *thread, OSThreadQueue *queue) {
    u32 enabled = CPU_IRQDisable();

    if (thread->state == OS_THREAD_STATE_READY) {
        if (queue != NULL) {
            thread->queue = queue;
            scheduler_thread_list_append(queue, thread);
        }
        thread->state = OS_THREAD_STATE_WAITING;
        scheduler_switch_thread();
    }
    CPU_SetIRQMask(enabled);
}

// NitroSDK's OS_SleepThread: puts the running thread to sleep in a queue
void scheduler_yield(OSThreadQueue *queue) {
    u32 enabled = CPU_IRQDisable();
    OSThread *current = *sCurrentThreadPtr;

    if (queue != NULL) {
        current->queue = queue;
        scheduler_thread_list_append(queue, current);
    }
    current->state = OS_THREAD_STATE_WAITING;
    scheduler_switch_thread();
    CPU_SetIRQMask(enabled);
}

// NitroSDK's OS_WakeupThread: makes all the threads in a queue ready
void OS_WakeupThread(OSThreadQueue *queue) {
    u32 enabled = CPU_IRQDisable();

    if (queue->head != NULL) {
        while (queue->head != NULL) {
            OSThread *thread = OSi_RemoveLinkFromQueue(queue);

            thread->state = OS_THREAD_STATE_READY;
            thread->queue = NULL;
            thread->link.next = NULL;
            thread->link.prev = thread->link.next;
        }
        OS_InitThreadQueue(queue);
        scheduler_switch_thread();
    }
    CPU_SetIRQMask(enabled);
}

// NitroSDK's OS_WakeupThreadDirect: makes a thread ready
void scheduler_start_thread(OSThread *thread) {
    u32 enabled = CPU_IRQDisable();

    thread->state = OS_THREAD_STATE_READY;
    scheduler_switch_thread();
    CPU_SetIRQMask(enabled);
}

// NitroSDK's OSi_SelectThread: the first thread of the list that is ready
static OSThread *func_0207a900(void) {
    OSThread *thread = g_IsThreadSwitchRequested.list;

    while (thread != NULL && thread->state != OS_THREAD_STATE_READY) {
        thread = thread->next;
    }
    return thread;
}

void OS_RescheduleThread(void) {
    u32 enabled = CPU_IRQDisable();

    scheduler_switch_thread();
    CPU_SetIRQMask(enabled);
}

// Moves the running thread behind the other threads of its priority
void OS_YieldThread(void) {
    int sameCount;
    OSThread *current;
    OSThread *lastSame;
    OSThread *beforeCurrent;
    OSThread *thread;
    OSThread *prev;
    u32 enabled;

    beforeCurrent = NULL;
    current = g_IsThreadSwitchRequested.current;
    lastSame = NULL;
    sameCount = 0;
    enabled = CPU_IRQDisable();

    thread = g_IsThreadSwitchRequested.list;
    prev = NULL;
    while (thread != NULL) {
        if (thread == current) {
            beforeCurrent = prev;
        }
        if (current->priority == thread->priority) {
            lastSame = thread;
            sameCount++;
        }
        prev = thread;
        thread = thread->next;
    }

    if (sameCount <= 1 || lastSame == current) {
        CPU_SetIRQMask(enabled);
        return;
    }

    if (beforeCurrent == NULL) {
        g_IsThreadSwitchRequested.list = current->next;
    } else {
        beforeCurrent->next = current->next;
    }
    current->next = lastSame->next;
    lastSame->next = current;
    scheduler_switch_thread();
    CPU_SetIRQMask(enabled);
}

// NitroSDK's OS_SetThreadPriority: gives false for a thread that is not in the list and for the idle thread
BOOL func_0207a998(OSThread *thread, u32 priority) {
    OSThread *found = g_IsThreadSwitchRequested.list;
    OSThread *prev = NULL;
    u32 enabled = CPU_IRQDisable();

    while (found != NULL && found != thread) {
        prev = found;
        found = found->next;
    }
    if (found == NULL || found == &OSi_IdleThread) {
        CPU_SetIRQMask(enabled);
        return FALSE;
    }

    if (found->priority != priority) {
        if (prev == NULL) {
            g_IsThreadSwitchRequested.list = thread->next;
        } else {
            prev->next = thread->next;
        }
        thread->priority = priority;
        func_0207a3d0(thread);
        scheduler_switch_thread();
    }
    CPU_SetIRQMask(enabled);
    return TRUE;
}

// NitroSDK's OS_GetThreadPriority
u32 func_0207aa00(const OSThread *thread) {
    return thread->priority;
}

// NitroSDK's OS_Sleep: sleeps for some milliseconds, with an alarm that wakes the thread and clears the pointer to it
// that it is given, which the loop looks at
void func_0207aa04(u32 msec) {
    OSAlarm alarm;
    OSThread *thread;
    u32 enabled;

    OS_CreateAlarm(&alarm);
    thread = *sCurrentThreadPtr;
    enabled = CPU_IRQDisable();
    thread->alarmForSleep = &alarm;
    OS_SetAlarm(&alarm, OS_MilliSecondsToTicks(msec), func_0207aa74, &thread);
    while (thread != NULL) {
        scheduler_yield(NULL);
    }
    CPU_SetIRQMask(enabled);
}

// The alarm's handler: wakes the thread that sleeps, whose pointer it clears to say so
static void func_0207aa74(void *arg) {
    OSThread *thread = *(OSThread **)arg;

    *(OSThread **)arg = NULL;
    thread->alarmForSleep = NULL;
    scheduler_start_thread(thread);
}

// NitroSDK's OS_SetSwitchThreadCallback
OSSwitchThreadCallback func_0207aa8c(OSSwitchThreadCallback callback) {
    u32 enabled = CPU_IRQDisable();
    OSSwitchThreadCallback previous = g_IsThreadSwitchRequested.switchCallback;

    g_IsThreadSwitchRequested.switchCallback = callback;
    CPU_SetIRQMask(enabled);
    return previous;
}

// NitroSDK's OSi_IdleThreadProc
static void func_0207aaa8(void *arg) {
    CPU_IRQEnable();
    while (TRUE) {
        cp15_halt();
    }
}

// Gives the count it had
u32 OS_DisableScheduler(void) {
    u32 enabled = CPU_IRQDisable();
    u32 old;
    u32 count = sSchedulerLockCount;

    // BUG: old is only set when the count can still grow, so it is garbage once the scheduler was locked 0xffffffff
    // times
#ifdef BUGFIX
    old = count;
#endif
    if (count < 0xffffffff) {
        sSchedulerLockCount = count + 1;
        old = count;
    }
    CPU_SetIRQMask(enabled);
    return old;
}

u32 OS_EnableScheduler(void) {
    u32 enabled = CPU_IRQDisable();
    u32 old = 0;
    u32 count = sSchedulerLockCount;

    if (count != 0) {
        sSchedulerLockCount = count - 1;
        old = count;
    }
    CPU_SetIRQMask(enabled);
    return old;
}

// NitroSDK's OS_SetThreadDestructor
void func_0207aafc(OSThread *thread, OSThreadDestructor destructor) {
    thread->destructor = destructor;
}
