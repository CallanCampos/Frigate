#include "nhttp_internal.h"

// NHTTP_os: the mutex and the thread that runs the requests (see nhttp_internal.h for the names)

static void NHTTPi_CommThreadProc(void *arg);

void NHTTPi_InitMutex(OSMutex *mutex) {
    OS_InitMutex(mutex);
}

void NHTTPi_LockMutex(OSMutex *mutex) {
    OS_LockMutex(mutex);
}

// Starts the thread, making its message queue the first time
BOOL NHTTPi_createCommThread(NHttpThreadInfo *info, int priority, void *stack) {
    if (!NHTTPi_IsCreateCommThreadMessageQueue(info)) {
        func_0207acc4(&info->queue, info->messages, 3);
        NHTTPi_IsCreateCommThreadMessageQueueOn(info);
    }
    scheduler_init_thread(&info->thread, NHTTPi_CommThreadProc, NULL, (u8 *)stack + 0x2000, 0x2000, priority);
    scheduler_start_thread(&info->thread);
    return TRUE;
}

// Asks the thread to end, wakes it and waits for it
void NHTTPi_destroyCommThread(NHttpThreadInfo *info, NHttpState *state) {
    state->quit = TRUE;
    NHTTPi_kickCommThread(&info->queue);
    func_0207a800(&info->thread);
}

// Waits for a message, which is the thread's way to sleep until there is a request
BOOL NHTTPi_idleCommThread(OSMessageQueue *queue) {
    OSMessage message;

    return OS_ReceiveMessage(queue, &message, OS_MESSAGE_BLOCK);
}

void NHTTPi_kickCommThread(OSMessageQueue *queue) {
    OS_SendMessage(queue, NULL, OS_MESSAGE_NOBLOCK);
}

// Stops the program if it is not running where it has to: on the thread when outside is FALSE, anywhere but there when
// it is TRUE
void NHTTPi_CheckCurrentThread(NHttpThreadInfo *info, BOOL outside) {
    OSThread *thread = &info->thread;
    OSThread *current = g_IsThreadSwitchRequested.current;

    if (current != NULL) {
        if ((!outside && current != thread) || (outside && current == thread)) {
            sys_exit();
        }
    }
}

static void NHTTPi_CommThreadProc(void *arg) {
    NHTTPi_CommThreadProcMain();
}
