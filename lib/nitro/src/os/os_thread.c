// NitroSDK OS: threads (os_thread.c, the SDK's name for the module). The scheduler keeps a list of all threads sorted
// by priority and runs the first one that is ready, switching by saving and loading contexts. The thread that is
// running when the system starts is the launcher thread, and the idle thread, the least urgent, halts the CPU until an
// interrupt. Built with -ipa file: the file's statics are reached from one base, and the data that code the link
// removed used is kept as words and fields without a name.
#include "nitro/mi.h"
#include "nitro/os.h"

static u32 OSi_GetUnusedThreadId(void);
static void OSi_InsertLinkToQueue(OSThreadQueue *queue, OSThread *thread); // swan's scheduler_thread_list_append
static OSThread *OSi_RemoveLinkFromQueue(OSThreadQueue *queue);
static OSThread *OSi_RemoveSpecifiedLinkFromQueue(OSThreadQueue *queue, OSThread *thread);
static void OSi_InsertThreadToList(OSThread *thread);
static void OSi_RemoveThreadFromList(OSThread *thread);
static void OSi_RescheduleThread(void);                               // swan's scheduler_switch_thread
static void OSi_ExitThread_ArgSpecified(OSThread *thread, void *arg); // swan's scheduler_exit_thread
static void OSi_ExitThread(void *arg);                                // swan's scheduler_release_thread
static void OSi_ExitThread_Destroy(void);                             // swan's scheduler_release_thread_core
static void OSi_KillThreadWithPriority(OSThread *thread, void *arg, u32 priority);
static void OSi_CancelThreadAlarmForSleep(OSThread *thread);
static OSThread *OS_SelectThread(void);
static void OSi_SleepAlarmCallback(void *arg);
static void OSi_IdleThreadProc(void *arg);

// Where the running thread is: the thread info's current. Some of the code reads it through here
static OSThread **OSi_CurrentThreadPtr;
// The function that is called with the old and the new thread at each switch, before the one in the thread info
static OSSwitchThreadCallback OSi_SystemCallbackInSwitchThread;
// How many times the scheduler has been locked (NitroSDK's OS_DisableScheduler), which stops it from switching
static u32 OSi_RescheduleCount;
// Whether OS_InitThread has run (NitroSDK's OS_IsThreadAvailable), which other code reads
BOOL OSi_ThreadInitialized;
// A word that code the link removed used, the stack that a thread that ends runs the end on when it is not zero, and
// the last ID that was given to a thread
static struct {
    u32 unk_00;
    u32 exitStack;
    u32 idCount;
} sThreadState;
OSThreadInfo OSi_ThreadInfo;
static OSThread OSi_IdleThread;     // swan's g_IdleThread
static OSThread OSi_LauncherThread; // swan's g_MainThread
static u8 OSi_IdleThreadStack[0xc8];

static u32 OSi_GetUnusedThreadId(void) {
    return ++sThreadState.idCount;
}

// NitroSDK's OSi_InsertLinkToQueue: puts a thread in the queue after the ones of the same or a more urgent priority
static void OSi_InsertLinkToQueue(OSThreadQueue *queue, OSThread *thread) {
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
static void OSi_InsertThreadToList(OSThread *thread) {
    OSThread *next;
    OSThread *prev = NULL;
    OSThread *head = OSi_ThreadInfo.list;

    next = head;
    while (next != NULL && next->priority < thread->priority) {
        prev = next;
        next = next->next;
    }

    if (prev == NULL) {
        thread->next = head;
        OSi_ThreadInfo.list = thread;
    } else {
        thread->next = prev->next;
        prev->next = thread;
    }
}

static void OSi_RemoveThreadFromList(OSThread *thread) {
    OSThread *prev = NULL;
    OSThread *next = OSi_ThreadInfo.list;

    while (next != NULL && next != thread) {
        prev = next;
        next = next->next;
    }

    if (prev == NULL) {
        OSi_ThreadInfo.list = thread->next;
    } else {
        prev->next = thread->next;
    }
}

// NitroSDK's OSi_RescheduleThread: switches to the first thread that is ready, unless the scheduler is locked or an
// interrupt is being handled, when it only notes that a switch is wanted
static void OSi_RescheduleThread(void) {
    OSThread *current;
    OSThread *next;

    if (OSi_RescheduleCount != 0) {
        return;
    }

    // The flag and the callback are reached through a pointer to the thread info and the rest of it by name, which is
    // how the original has two literals for it
    OSThreadInfo *info = &OSi_ThreadInfo;

    if (OSi_ThreadInfo.irqDepth != 0 || OS_GetProcMode() == OS_PROCMODE_IRQ) {
        info->isNeedRescheduling = TRUE;
        return;
    }

    current = *OSi_CurrentThreadPtr;
    next = OS_SelectThread();
    if (current == next || next == NULL) {
        return;
    }

    if (current->state != OS_THREAD_STATE_TERMINATED) {
        if (OS_SaveContext(&current->context)) {
            return;
        }
    }
    if (OSi_SystemCallbackInSwitchThread != NULL) {
        OSi_SystemCallbackInSwitchThread(current, next);
    }
    if (info->switchCallback != NULL) {
        info->switchCallback(current, next);
    }
    OSi_ThreadInfo.current = next;
    OS_LoadContext(&next->context);
}

// NitroSDK's OS_InitThread: makes the running thread the launcher thread, with the system stack, and creates the idle
// thread
void OS_InitThread(void) {
    u32 stackTop;

    if (OSi_ThreadInitialized) {
        return;
    }
    OSi_ThreadInitialized = TRUE;
    OSi_CurrentThreadPtr = &OSi_ThreadInfo.current;

    OSi_LauncherThread.priority = OS_THREAD_LAUNCHER_PRIORITY;
    OSi_LauncherThread.id = 0;
    OSi_LauncherThread.state = OS_THREAD_STATE_READY;
    OSi_LauncherThread.next = NULL;
    OSi_LauncherThread.profiler = NULL;
    OSi_ThreadInfo.current = OSi_ThreadInfo.list = &OSi_LauncherThread;

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

    OSi_ThreadInfo.isNeedRescheduling = FALSE;
    OSi_ThreadInfo.irqDepth = 0;
    *(OSThreadInfo **)HW_THREADINFO_MAIN = &OSi_ThreadInfo;
    OS_SetSwitchThreadCallback(NULL);

    OS_CreateThread(&OSi_IdleThread, OSi_IdleThreadProc, NULL, OSi_IdleThreadStack + sizeof(OSi_IdleThreadStack),
                    sizeof(OSi_IdleThreadStack), OS_THREAD_PRIORITY_MAX);
    OSi_IdleThread.priority = OS_THREAD_PRIORITY_MAX + 1;
    OSi_IdleThread.state = OS_THREAD_STATE_READY;
}

BOOL OS_IsThreadAvailable(void) {
    return OSi_ThreadInitialized;
}

// NitroSDK's OS_CreateThread: sets up a thread, which stays asleep until it is started. The stack is where it begins,
// and the end of the function makes the thread exit
void OS_CreateThread(OSThread *thread, void (*func)(void *), void *arg, void *stack, u32 stackSize, u32 prio) {
    u32 enabled = OS_DisableInterrupts();
    u32 id = OSi_GetUnusedThreadId();
    u32 stackTop;

    thread->priority = prio;
    thread->state = OS_THREAD_STATE_WAITING;
    thread->id = id;
    thread->profiler = NULL;
    OSi_InsertThreadToList(thread);

    thread->stackBottom = (u32)stack;
    stackTop = (u32)stack - stackSize;
    thread->stackTop = stackTop;
    thread->stackWarningOffset = 0;
    *(u32 *)(thread->stackBottom - 8) = OS_STACK_BEGIN_MAGIC;
    *(u32 *)thread->stackTop = OS_STACK_END_MAGIC;
    OS_InitThreadQueue(&thread->joinQueue);

    OS_InitContext(&thread->context, (u32)func, (u32)stack - 8);
    thread->context.r[0] = (u32)arg;
    thread->context.lr = (u32)OS_ExitThread;
    MI_CpuClear32((void *)(stackTop + 4), stackSize - 12);

    thread->mutex = NULL;
    thread->mutexQueue.head = NULL;
    thread->mutexQueue.tail = NULL;
    OS_SetThreadDestructor(thread, NULL);
    thread->queue = NULL;
    thread->link.prev = NULL;
    thread->link.next = NULL;
    MI_CpuClear32(thread->specific, sizeof(thread->specific));
    thread->alarmForSleep = NULL;

    OS_RestoreInterrupts(enabled);
}

// NitroSDK's OS_ExitThread: what a thread returns to when its function ends
void OS_ExitThread(void) {
    OS_DisableInterrupts();
    OSi_ExitThread_ArgSpecified(OSi_ThreadInfo.current, NULL);
}

// NitroSDK's OSi_ExitThread_ArgSpecified: makes a thread continue with the end of it, on the exit stack if there is one
static void OSi_ExitThread_ArgSpecified(OSThread *thread, void *arg) {
    if (sThreadState.exitStack != 0) {
        OS_InitContext(&thread->context, (u32)OSi_ExitThread, sThreadState.exitStack);
        thread->context.cpsr |= 0x80;
        thread->state = OS_THREAD_STATE_READY;
        thread->context.r[0] = (u32)arg;
        OS_LoadContext(&thread->context);
    } else {
        OSi_ExitThread(arg);
    }
}

// NitroSDK's OSi_ExitThread: the end of a thread, which calls its destructor first
static void OSi_ExitThread(void *arg) {
    OSThread *thread = *OSi_CurrentThreadPtr;
    OSThreadDestructor destructor = thread->destructor;

    if (destructor != NULL) {
        thread->destructor = NULL;
        destructor(arg);
        OS_DisableInterrupts();
    }
    OSi_ExitThread_Destroy();
}

// NitroSDK's OSi_ExitThread_Destroy: takes the thread out of the lists, wakes the threads that wait for it and switches
// away for good
static void OSi_ExitThread_Destroy(void) {
    OSThread *thread = *OSi_CurrentThreadPtr;

    OS_DisableScheduler();
    OSi_UnlockAllMutex(thread);
    if (thread->queue != NULL) {
        OSi_RemoveSpecifiedLinkFromQueue(thread->queue, thread);
    }
    OSi_RemoveThreadFromList(thread);
    thread->state = OS_THREAD_STATE_TERMINATED;
    OS_WakeupThread(&thread->joinQueue);
    OS_EnableScheduler();
    OS_RescheduleThread();
    OS_Terminate();
}

// NitroSDK's OS_DestroyThread: ends a thread, waking the ones that wait for it
void OS_DestroyThread(OSThread *thread) {
    u32 enabled = OS_DisableInterrupts();

    if (OSi_ThreadInfo.current == thread) {
        OSi_ExitThread_Destroy();
    }
    OS_DisableScheduler();
    OSi_UnlockAllMutex(thread);
    OSi_CancelThreadAlarmForSleep(thread);
    if (thread->queue != NULL) {
        OSi_RemoveSpecifiedLinkFromQueue(thread->queue, thread);
    }
    OSi_RemoveThreadFromList(thread);
    thread->state = OS_THREAD_STATE_TERMINATED;
    OS_WakeupThread(&thread->joinQueue);
    OS_EnableScheduler();
    OS_RestoreInterrupts(enabled);
    OS_RescheduleThread();
}

// NitroSDK's OS_KillThread: makes a thread end, with a value for its destructor
void OS_KillThread(OSThread *thread, void *arg) {
    OSi_KillThreadWithPriority(thread, arg, OS_GetThreadPriority(thread));
}

// NitroSDK's OSi_KillThreadWithPriority: makes the end of a thread run next, at the priority
static void OSi_KillThreadWithPriority(OSThread *thread, void *arg, u32 priority) {
    u32 enabled = OS_DisableInterrupts();
    u32 stack;

    if (thread == OSi_ThreadInfo.current) {
        OSi_ExitThread_ArgSpecified(thread, arg);
    }
    OSi_CancelThreadAlarmForSleep(thread);

    stack = sThreadState.exitStack;
    if (stack == 0) {
        stack = thread->stackBottom - 8;
    }
    OS_InitContext(&thread->context, (u32)OSi_ExitThread, stack);
    thread->context.cpsr |= 0x80;
    thread->context.r[0] = (u32)arg;
    thread->state = OS_THREAD_STATE_READY;

    OS_DisableScheduler();
    OS_SetThreadPriority(thread, priority);
    OS_EnableScheduler();
    OSi_RescheduleThread();
    OS_RestoreInterrupts(enabled);
}

// Cancels the alarm that a thread sleeps with, if it has one
static void OSi_CancelThreadAlarmForSleep(OSThread *thread) {
    if (thread->alarmForSleep != NULL) {
        OS_CancelAlarm(thread->alarmForSleep);
    }
}

// Waits for a thread to end
void OS_JoinThread(OSThread *thread) {
    u32 enabled = OS_DisableInterrupts();

    while (thread->state != OS_THREAD_STATE_TERMINATED) {
        OS_SleepThread(&thread->joinQueue);
    }
    OS_RestoreInterrupts(enabled);
}

BOOL OS_IsThreadTerminated(const OSThread *thread) {
    if (thread->state == OS_THREAD_STATE_TERMINATED) {
        return TRUE;
    }
    return FALSE;
}

// TwlSDK's OS_SleepThreadDirect: puts a thread to sleep in a queue
void OS_SleepThreadDirect(OSThread *thread, OSThreadQueue *queue) {
    u32 enabled = OS_DisableInterrupts();

    if (thread->state == OS_THREAD_STATE_READY) {
        if (queue != NULL) {
            thread->queue = queue;
            OSi_InsertLinkToQueue(queue, thread);
        }
        thread->state = OS_THREAD_STATE_WAITING;
        OSi_RescheduleThread();
    }
    OS_RestoreInterrupts(enabled);
}

// NitroSDK's OS_SleepThread: puts the running thread to sleep in a queue
void OS_SleepThread(OSThreadQueue *queue) {
    u32 enabled = OS_DisableInterrupts();
    OSThread *current = *OSi_CurrentThreadPtr;

    if (queue != NULL) {
        current->queue = queue;
        OSi_InsertLinkToQueue(queue, current);
    }
    current->state = OS_THREAD_STATE_WAITING;
    OSi_RescheduleThread();
    OS_RestoreInterrupts(enabled);
}

// NitroSDK's OS_WakeupThread: makes all the threads in a queue ready
void OS_WakeupThread(OSThreadQueue *queue) {
    u32 enabled = OS_DisableInterrupts();

    if (queue->head != NULL) {
        while (queue->head != NULL) {
            OSThread *thread = OSi_RemoveLinkFromQueue(queue);

            thread->state = OS_THREAD_STATE_READY;
            thread->queue = NULL;
            thread->link.prev = thread->link.next = NULL;
        }
        OS_InitThreadQueue(queue);
        OSi_RescheduleThread();
    }
    OS_RestoreInterrupts(enabled);
}

// NitroSDK's OS_WakeupThreadDirect: makes a thread ready
void OS_WakeupThreadDirect(OSThread *thread) {
    u32 enabled = OS_DisableInterrupts();

    thread->state = OS_THREAD_STATE_READY;
    OSi_RescheduleThread();
    OS_RestoreInterrupts(enabled);
}

// NitroSDK's OS_SelectThread: the first thread of the list that is ready
static OSThread *OS_SelectThread(void) {
    OSThread *thread = OSi_ThreadInfo.list;

    while (thread != NULL && thread->state != OS_THREAD_STATE_READY) {
        thread = thread->next;
    }
    return thread;
}

void OS_RescheduleThread(void) {
    u32 enabled = OS_DisableInterrupts();

    OSi_RescheduleThread();
    OS_RestoreInterrupts(enabled);
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
    current = OSi_ThreadInfo.current;
    lastSame = NULL;
    sameCount = 0;
    enabled = OS_DisableInterrupts();

    thread = OSi_ThreadInfo.list;
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
        OS_RestoreInterrupts(enabled);
        return;
    }

    if (beforeCurrent == NULL) {
        OSi_ThreadInfo.list = current->next;
    } else {
        beforeCurrent->next = current->next;
    }
    current->next = lastSame->next;
    lastSame->next = current;
    OSi_RescheduleThread();
    OS_RestoreInterrupts(enabled);
}

// NitroSDK's OS_SetThreadPriority: gives false for a thread that is not in the list and for the idle thread
BOOL OS_SetThreadPriority(OSThread *thread, u32 priority) {
    OSThread *found = OSi_ThreadInfo.list;
    OSThread *prev = NULL;
    u32 enabled = OS_DisableInterrupts();

    while (found != NULL && found != thread) {
        prev = found;
        found = found->next;
    }
    if (found == NULL || found == &OSi_IdleThread) {
        OS_RestoreInterrupts(enabled);
        return FALSE;
    }

    if (found->priority != priority) {
        if (prev == NULL) {
            OSi_ThreadInfo.list = thread->next;
        } else {
            prev->next = thread->next;
        }
        thread->priority = priority;
        OSi_InsertThreadToList(thread);
        OSi_RescheduleThread();
    }
    OS_RestoreInterrupts(enabled);
    return TRUE;
}

u32 OS_GetThreadPriority(const OSThread *thread) {
    return thread->priority;
}

// NitroSDK's OS_Sleep: sleeps for some milliseconds, with an alarm that wakes the thread and clears the pointer to it
// that it is given, which the loop looks at
void OS_Sleep(u32 msec) {
    OSAlarm alarm;
    OSThread *thread;
    u32 enabled;

    OS_CreateAlarm(&alarm);
    thread = *OSi_CurrentThreadPtr;
    enabled = OS_DisableInterrupts();
    thread->alarmForSleep = &alarm;
    OS_SetAlarm(&alarm, OS_MilliSecondsToTicks(msec), OSi_SleepAlarmCallback, &thread);
    while (thread != NULL) {
        OS_SleepThread(NULL);
    }
    OS_RestoreInterrupts(enabled);
}

// The alarm's handler: wakes the thread that sleeps, whose pointer it clears to say so
static void OSi_SleepAlarmCallback(void *arg) {
    OSThread *thread = *(OSThread **)arg;

    *(OSThread **)arg = NULL;
    thread->alarmForSleep = NULL;
    OS_WakeupThreadDirect(thread);
}

OSSwitchThreadCallback OS_SetSwitchThreadCallback(OSSwitchThreadCallback callback) {
    u32 enabled = OS_DisableInterrupts();
    OSSwitchThreadCallback previous = OSi_ThreadInfo.switchCallback;

    OSi_ThreadInfo.switchCallback = callback;
    OS_RestoreInterrupts(enabled);
    return previous;
}

static void OSi_IdleThreadProc(void *arg) {
    OS_EnableInterrupts();
    while (TRUE) {
        OS_Halt();
    }
}

// Gives the count it had
u32 OS_DisableScheduler(void) {
    u32 enabled = OS_DisableInterrupts();
    u32 old;

    // As in the SDK, old is left unset when the count is at its maximum, which 0xffffffff nested locks would take
    if (OSi_RescheduleCount < 0xffffffff) {
        old = OSi_RescheduleCount++;
    }
    OS_RestoreInterrupts(enabled);
    return old;
}

u32 OS_EnableScheduler(void) {
    u32 enabled = OS_DisableInterrupts();
    u32 old = 0;

    if (OSi_RescheduleCount != 0) {
        old = OSi_RescheduleCount--;
    }
    OS_RestoreInterrupts(enabled);
    return old;
}

void OS_SetThreadDestructor(OSThread *thread, OSThreadDestructor destructor) {
    thread->destructor = destructor;
}
