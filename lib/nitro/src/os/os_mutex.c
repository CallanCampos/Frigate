// NitroSDK OS: mutexes (os_mutex.c, the SDK's name for the module). Thumb code. A thread may lock a mutex again, and
// each mutex keeps the threads that wait for it in a queue; the mutexes that a thread has are in a list of its own, so
// that they can be unlocked when the thread ends. This is TwlSDK's version, which keeps a type in the top byte of the
// word that holds the lock count.
#include "nitro/os.h"

// The word at OSMutex.count: the lock count in the low 24 bits and the type in the top 8. The types are TwlSDK's
// standard, read and write mutexes: a standard one (the only one that is made here) and a write one are unlocked by the
// thread that has it, a read one by any thread
#define OS_MUTEX_TYPE_STD 0x10000000
#define OS_MUTEX_TYPE_R 0x20000000
#define OS_MUTEX_TYPE_W 0x30000000
#define OS_MUTEX_TYPE_MASK 0xff000000
#define OS_MUTEX_COUNT_MASK 0x00ffffff

#define OSi_GetMutexType(m) ((m)->count & OS_MUTEX_TYPE_MASK)
#define OSi_GetMutexCount(m) ((m)->count & OS_MUTEX_COUNT_MASK)
#define OSi_SetMutexType(m, t) ((m)->count = ((m)->count & OS_MUTEX_COUNT_MASK) | (t))
#define OSi_SetMutexCount(m, c) ((m)->count = ((m)->count & OS_MUTEX_TYPE_MASK) | ((c) & OS_MUTEX_COUNT_MASK))

static void OSi_UnlockMutexCore(OSMutex *mutex, u32 type);
static void OSi_EnqueueTail(OSThread *thread, OSMutex *mutex);
static void OSi_DequeueItem(OSThread *thread, OSMutex *mutex);

// NitroSDK's OS_InitMutex
void OS_InitMutex(OSMutex *mutex) {
    OS_InitThreadQueue(&mutex->queue);
    mutex->thread = NULL;
    OSi_SetMutexCount(mutex, 0);
    OSi_SetMutexType(mutex, 0);
}

// NitroSDK's OS_LockMutex: waits until the mutex is free (or already the thread's), then holds it
void OS_LockMutex(OSMutex *mutex) {
    OSIntrMode irq = OS_DisableInterrupts();
    OSThread *current = OS_GetCurrentThread();

    for (;;) {
        if (OS_TryLockMutex(mutex)) {
            break;
        }
        current->mutex = mutex;
        OS_SleepThread(&mutex->queue);
        current->mutex = NULL;
    }
    OS_RestoreInterrupts(irq);
}

// NitroSDK's OS_UnlockMutex, which unlocks a mutex of type 1
void OS_UnlockMutex(OSMutex *mutex) {
    OSi_UnlockMutexCore(mutex, OS_MUTEX_TYPE_STD);
}

// NitroSDK's OSi_UnlockAllMutex: unlocks every mutex that a thread has, whatever its count, and wakes the threads that
// wait for it
void OSi_UnlockAllMutex(OSThread *thread) {
    OSMutex *mutex;

    while (thread->mutexQueue.head != NULL) {
        mutex = OSi_RemoveMutexLinkFromQueue(&thread->mutexQueue);
        OSi_SetMutexCount(mutex, 0);
        mutex->thread = NULL;
        OSi_SetMutexType(mutex, 0);
        OS_WakeupThread(&mutex->queue);
    }
}

// TRUE when the thread has the mutex now, either by taking it when it is free or by locking
// it again
BOOL OS_TryLockMutex(OSMutex *mutex) {
    OSIntrMode irq = OS_DisableInterrupts();
    OSThread *current = OS_GetCurrentThread();
    BOOL locked;

    if (mutex->thread == NULL) {
        OSi_SetMutexType(mutex, OS_MUTEX_TYPE_STD);
        OSi_SetMutexCount(mutex, mutex->count + 1);
        mutex->thread = current;
        OSi_EnqueueTail(current, mutex);
        locked = TRUE;
    } else if (mutex->thread == current) {
        OSi_SetMutexCount(mutex, mutex->count + 1);
        locked = TRUE;
    } else {
        locked = FALSE;
    }
    OS_RestoreInterrupts(irq);
    return locked;
}

// TwlSDK's body of OS_UnlockMutex: lowers the count of the mutex, if it is of the type that is asked for (any type when
// that is 0) and the thread may unlock it, and once the count is 0 gives the mutex up and wakes the threads that wait
static void OSi_UnlockMutexCore(OSMutex *mutex, u32 type) {
    OSIntrMode irq = OS_DisableInterrupts();
    OSThread *current = OS_GetCurrentThread();
    BOOL free = FALSE;
    u32 mutexType;

    if (type != 0 && type != OSi_GetMutexType(mutex)) {
        OS_RestoreInterrupts(irq);
        return;
    }
    mutexType = OSi_GetMutexType(mutex);
    switch (mutexType) {
    case OS_MUTEX_TYPE_STD:
    case OS_MUTEX_TYPE_W:
        if (mutex->thread == current) {
            mutex->count--;
            mutex->count = mutexType | (mutex->count & OS_MUTEX_COUNT_MASK);
            if (OSi_GetMutexCount(mutex) == 0) {
                free = TRUE;
            }
        }
        break;
    case OS_MUTEX_TYPE_R:
        mutex->count--;
        mutex->count = mutexType | (mutex->count & OS_MUTEX_COUNT_MASK);
        if (OSi_GetMutexCount(mutex) == 0) {
            free = TRUE;
        }
        break;
    default:
        OS_RestoreInterrupts(irq);
        return;
    }
    if (free) {
        OSi_DequeueItem(current, mutex);
        mutex->thread = NULL;
        OSi_SetMutexType(mutex, 0);
        OS_WakeupThread(&mutex->queue);
    }
    OS_RestoreInterrupts(irq);
}

// NitroSDK's OSi_EnqueueTail, for mutexes: puts a mutex at the end of the list of the mutexes that a thread has
static void OSi_EnqueueTail(OSThread *thread, OSMutex *mutex) {
    OSMutex *prev = thread->mutexQueue.tail;

    if (prev == NULL) {
        thread->mutexQueue.head = mutex;
    } else {
        prev->link.next = mutex;
    }
    mutex->link.prev = prev;
    mutex->link.next = NULL;
    thread->mutexQueue.tail = mutex;
}

// NitroSDK's OSi_DequeueItem, for mutexes: takes a mutex out of that list
static void OSi_DequeueItem(OSThread *thread, OSMutex *mutex) {
    OSMutex *next = mutex->link.next;
    OSMutex *prev = mutex->link.prev;

    if (next == NULL) {
        thread->mutexQueue.tail = prev;
    } else {
        next->link.prev = prev;
    }
    if (prev == NULL) {
        thread->mutexQueue.head = next;
    } else {
        prev->link.next = next;
    }
}
