// The SDK's thread code on a host (spike): the decomp's os_thread.c, compiled as it is, read from its own path. Four
// of its functions can't run on a 64-bit host as written, so they are renamed out of the way before the include and
// written again below, in this file so that they can reach os_thread.c's statics:
//   - OS_InitThread puts the launcher's stack bounds and magic words in DTCM and stores a pointer at the 32-bit word
//     HW_THREADINFO_MAIN;
//   - OS_CreateThread and OS_KillThread pass the thread's function, its argument and its stack through the context's
//     32-bit words ((u32)func, (u32)arg, stackBottom), which truncates 64-bit pointers;
//   - OS_DestroyThread is kept, but wrapped, so that the host learns that the thread's fiber will never run again.
// Everything else (the queues, the sorted list, OSi_RescheduleThread, sleep, wakeup, yield, join, priorities, the
// scheduler lock, OS_Sleep, the exit path) is the decomp's own code.
#define OS_InitThread OSi_DsInitThread
#define OS_CreateThread OSi_DsCreateThread
#define OS_KillThread OSi_DsKillThread
#define OS_DestroyThread OSi_DsDestroyThread
#include "lib/nitro/src/os/os_thread.c"
#undef OS_InitThread
#undef OS_CreateThread
#undef OS_KillThread
#undef OS_DestroyThread

#include "os_host.h"

// os.h's prototypes were renamed with the functions
void OS_InitThread(void);
void OS_CreateThread(OSThread *thread, void (*func)(void *), void *arg, void *stack, u32 stackSize, u32 prio);
void OS_KillThread(OSThread *thread, void *arg);
void OS_DestroyThread(OSThread *thread);

// NitroSDK's OS_InitThread without the DS memory: the launcher runs on the fiber that is running, whose stack the SDK
// never sees, and nothing on a host reads HW_THREADINFO_MAIN
void OS_InitThread(void) {
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
    OSi_LauncherThread.stackBottom = 0;
    OSi_LauncherThread.stackTop = 0;
    OSi_LauncherThread.stackWarningOffset = 0;
    OS_InitThreadQueue(&OSi_LauncherThread.joinQueue);
    hal_os_bind_running_context(&OSi_LauncherThread.context);

    OSi_ThreadInfo.isNeedRescheduling = FALSE;
    OSi_ThreadInfo.irqDepth = 0;
    OS_SetSwitchThreadCallback(NULL);

    OS_CreateThread(&OSi_IdleThread, OSi_IdleThreadProc, NULL, OSi_IdleThreadStack + sizeof(OSi_IdleThreadStack),
                    sizeof(OSi_IdleThreadStack), OS_THREAD_PRIORITY_MAX);
    OSi_IdleThread.priority = OS_THREAD_PRIORITY_MAX + 1;
    OSi_IdleThread.state = OS_THREAD_STATE_READY;
}

// NitroSDK's OS_CreateThread with the pointers kept whole: the host context gets the function and its argument, and
// the magic words and the clearing go to the DS stack buffer through its real address. The buffer itself is unused
// on a host, since the thread runs on its fiber's stack; stackTop and stackBottom, which nothing outside os_thread.c
// reads, stay 0
void OS_CreateThread(OSThread *thread, void (*func)(void *), void *arg, void *stack, u32 stackSize, u32 prio) {
    u32 enabled = OS_DisableInterrupts();
    u32 id = OSi_GetUnusedThreadId();
    u8 *stackEnd = (u8 *)stack - stackSize;

    thread->priority = prio;
    thread->state = OS_THREAD_STATE_WAITING;
    thread->id = id;
    thread->profiler = NULL;
    OSi_InsertThreadToList(thread);

    thread->stackBottom = 0;
    thread->stackTop = 0;
    thread->stackWarningOffset = 0;
    *(u32 *)((u8 *)stack - 8) = OS_STACK_BEGIN_MAGIC;
    *(u32 *)stackEnd = OS_STACK_END_MAGIC;
    OS_InitThreadQueue(&thread->joinQueue);

    OS_InitContext(&thread->context, 0, 0);
    hal_os_set_context_entry(&thread->context, func, arg, TRUE);
    MI_CpuClear32(stackEnd + 4, stackSize - 12);

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

// NitroSDK's OS_KillThread (with OSi_KillThreadWithPriority): the thread's context restarts at OSi_ExitThread with the
// value for its destructor, with interrupts disabled as the DS sets the CPSR's I bit, and runs at its priority. The
// thread is not taken out of a queue it sleeps in; OSi_ExitThread_Destroy does that, as on the DS
void OS_KillThread(OSThread *thread, void *arg) {
    u32 priority = OS_GetThreadPriority(thread);
    u32 enabled = OS_DisableInterrupts();

    if (thread == OSi_ThreadInfo.current) {
        OSi_ExitThread_ArgSpecified(thread, arg);
    }
    OSi_CancelThreadAlarmForSleep(thread);

    OS_InitContext(&thread->context, 0, 0);
    hal_os_set_context_entry(&thread->context, OSi_ExitThread, arg, FALSE);
    thread->state = OS_THREAD_STATE_READY;

    OS_DisableScheduler();
    OS_SetThreadPriority(thread, priority);
    OS_EnableScheduler();
    OSi_RescheduleThread();
    OS_RestoreInterrupts(enabled);
}

// NitroSDK's OS_DestroyThread. Another thread's fiber, suspended wherever it was, is disposed of first, before the
// wakeups in the SDK's code can run a thread that creates it again; a thread that destroys itself never returns from
// the SDK's code, and OS_LoadContext disposes of it
void OS_DestroyThread(OSThread *thread) {
    if (thread != OSi_ThreadInfo.current) {
        hal_os_release_context(&thread->context);
    }
    OSi_DsDestroyThread(thread);
}

void hal_os_reset_threads(void) {
    OSi_CurrentThreadPtr = NULL;
    OSi_SystemCallbackInSwitchThread = NULL;
    OSi_RescheduleCount = 0;
    OSi_ThreadInitialized = FALSE;
    sThreadState.unk_00 = 0;
    sThreadState.exitStack = 0;
    sThreadState.idCount = 0;
    MI_CpuClear32(&OSi_ThreadInfo, sizeof(OSi_ThreadInfo));
    MI_CpuClear32(&OSi_IdleThread, sizeof(OSi_IdleThread));
    MI_CpuClear32(&OSi_LauncherThread, sizeof(OSi_LauncherThread));
}
