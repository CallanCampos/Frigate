// NitroSDK OS: message queues (os_message.c, the SDK's name for the module). A queue is a ring of messages, with the
// threads that wait to send into it and to receive from it.
#include "nitro/os.h"

void OS_InitMessageQueue(OSMessageQueue *mq, OSMessage *msgArray, s32 msgCount) {
    OS_InitThreadQueue(&mq->queueSend);
    OS_InitThreadQueue(&mq->queueReceive);
    mq->msgArray = msgArray;
    mq->msgCount = msgCount;
    mq->firstIndex = 0;
    mq->usedCount = 0;
}

// NitroSDK's OS_SendMessage: puts the message at the end of the queue, waiting for room if it is full and flags says
// OS_MESSAGE_BLOCK
BOOL OS_SendMessage(OSMessageQueue *mq, OSMessage msg, s32 flags) {
    OSIntrMode enabled = OS_DisableInterrupts();

    while (mq->msgCount <= mq->usedCount) {
        if (!(flags & OS_MESSAGE_BLOCK)) {
            OS_RestoreInterrupts(enabled);
            return FALSE;
        }
        OS_SleepThread(&mq->queueSend);
    }

    mq->msgArray[(mq->firstIndex + mq->usedCount) % mq->msgCount] = msg;
    mq->usedCount++;

    OS_WakeupThread(&mq->queueReceive);
    OS_RestoreInterrupts(enabled);
    return TRUE;
}

// NitroSDK's OS_ReceiveMessage: takes the first message out of the queue (msg may be NULL to drop it), waiting for one
// if it is empty and flags says OS_MESSAGE_BLOCK
BOOL OS_ReceiveMessage(OSMessageQueue *mq, OSMessage *msg, s32 flags) {
    OSIntrMode enabled = OS_DisableInterrupts();

    while (mq->usedCount == 0) {
        if (!(flags & OS_MESSAGE_BLOCK)) {
            OS_RestoreInterrupts(enabled);
            return FALSE;
        }
        OS_SleepThread(&mq->queueReceive);
    }

    if (msg != NULL) {
        *msg = mq->msgArray[mq->firstIndex];
    }
    mq->firstIndex = (mq->firstIndex + 1) % mq->msgCount;
    mq->usedCount--;

    OS_WakeupThread(&mq->queueSend);
    OS_RestoreInterrupts(enabled);
    return TRUE;
}

// NitroSDK's OS_JamMessage: puts the message at the front of the queue
BOOL OS_JamMessage(OSMessageQueue *mq, OSMessage msg, s32 flags) {
    OSIntrMode enabled = OS_DisableInterrupts();

    while (mq->msgCount <= mq->usedCount) {
        if (!(flags & OS_MESSAGE_BLOCK)) {
            OS_RestoreInterrupts(enabled);
            return FALSE;
        }
        OS_SleepThread(&mq->queueSend);
    }

    mq->firstIndex = (mq->firstIndex + mq->msgCount - 1) % mq->msgCount;
    mq->msgArray[mq->firstIndex] = msg;
    mq->usedCount++;

    OS_WakeupThread(&mq->queueReceive);
    OS_RestoreInterrupts(enabled);
    return TRUE;
}

// NitroSDK's OS_ReadMessage: the first message, which stays in the queue
BOOL OS_ReadMessage(OSMessageQueue *mq, OSMessage *msg, s32 flags) {
    OSIntrMode enabled = OS_DisableInterrupts();

    while (mq->usedCount == 0) {
        if (!(flags & OS_MESSAGE_BLOCK)) {
            OS_RestoreInterrupts(enabled);
            return FALSE;
        }
        OS_SleepThread(&mq->queueReceive);
    }

    if (msg != NULL) {
        *msg = mq->msgArray[mq->firstIndex];
    }

    OS_RestoreInterrupts(enabled);
    return TRUE;
}
