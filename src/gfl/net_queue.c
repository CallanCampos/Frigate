#include "types.h"
#include "gfl/heap.h"
#include "gfl/net_queue.h"
#include "gfl/net_ring_buff.h"
#include "gfl/std.h"

static NetQueueEntry *NetQueue_FindFreeEntry(NetQueue *queue);
static int NetQueue_CountEntries(NetQueue *queue);
static BOOL NetQueueList_PopHead(NetQueueList *list);
static void NetSendBuffer_PutByte(NetSendBuffer *buffer, u8 byte);
static BOOL NetQueueEntry_WriteHeader(NetQueueEntry *entry, NetSendBuffer *buffer);
static BOOL NetQueueEntry_Write(NetQueueEntry *entry, NetSendBuffer *buffer, NetRingBuff *ring, BOOL checkSpace);
static NetQueueEntry *NetQueue_GetNext(NetQueue *queue);
static void NetQueue_RemoveNext(NetQueue *queue);

static NetQueueEntry *NetQueue_FindFreeEntry(NetQueue *queue) {
    NetQueueEntry *entry = queue->entries;
    int i;

    for (i = 0; i < queue->count; i++, entry++) {
        if (entry->command == 0) {
            return entry;
        }
    }
    return NULL;
}

BOOL NetQueue_IsEmpty(NetQueue *queue) {
    if (NetQueue_CountEntries(queue) == 0) {
        return TRUE;
    }
    return FALSE;
}

static int NetQueue_CountEntries(NetQueue *queue) {
    NetQueueEntry *entry = queue->entries;
    int i;
    int count = 0;

    for (i = 0; i < queue->count; i++) {
        if (entry->command != 0) {
            count++;
        }
        entry++;
    }
    return count;
}

static BOOL NetQueueList_PopHead(NetQueueList *list) {
    if (list->head != NULL) {
        NetQueueEntry *next = list->head->next;

        if (next != NULL) {
            list->head = next;
            next->prev = NULL;
        } else {
            list->head = NULL;
            list->tail = NULL;
        }
        return TRUE;
    }
    return FALSE;
}

static void NetSendBuffer_PutByte(NetSendBuffer *buffer, u8 byte) {
    *buffer->ptr = byte;
    buffer->ptr++;
    buffer->size--;
}

static BOOL NetQueueEntry_WriteHeader(NetQueueEntry *entry, NetSendBuffer *buffer) {
    NetSendBuffer_PutByte(buffer, entry->command >> 8);
    NetSendBuffer_PutByte(buffer, entry->command);
    NetSendBuffer_PutByte(buffer, entry->size >> 8);
    NetSendBuffer_PutByte(buffer, entry->size);
    NetSendBuffer_PutByte(buffer, entry->netId);
    return FALSE;
}

static BOOL NetQueueEntry_Write(NetQueueEntry *entry, NetSendBuffer *buffer, NetRingBuff *ring, BOOL checkSpace) {
    if (!entry->headerSent) {
        if (checkSpace && buffer->size < entry->size + NET_QUEUE_HEADER_SIZE) {
            return FALSE;
        }
        NetQueueEntry_WriteHeader(entry, buffer);
    }
    if (buffer->size < entry->size) {
        if (entry->data == NULL) {
            NetRingBuff_Read(ring, buffer->ptr, buffer->size, buffer->size);
        } else {
            MI_CpuCopy8(entry->data, buffer->ptr, buffer->size);
            entry->data += buffer->size;
        }
        entry->size -= buffer->size;
        buffer->size = 0xffff;
        entry->headerSent = TRUE;
        return TRUE;
    }
    if (entry->data == NULL) {
        NetRingBuff_Read(ring, buffer->ptr, entry->size, entry->size);
    } else {
        MI_CpuCopy8(entry->data, buffer->ptr, entry->size);
    }
    buffer->ptr += entry->size;
    buffer->size -= entry->size;
    return TRUE;
}

BOOL NetQueue_Push(NetQueue *queue, int command, u8 *data, int size, u32 unused, BOOL copy, int netId) {
    NetQueueEntry *entry = NetQueue_FindFreeEntry(queue);

    if (entry == NULL) {
        GFL_ASSERT(0);
        return FALSE;
    }
    // clang-format off
    GFL_ASSERT((u16)size < (0xffff/2));
    // clang-format on
    if (copy) {
        if (size + NET_QUEUE_HEADER_SIZE >= NetRingBuff_GetFreeSize(queue->ring)) {
            GFL_ASSERT(0);
            return FALSE;
        }
        NetRingBuff_Write(queue->ring, data, size);
        NetRingBuff_Commit(queue->ring);
        entry->data = NULL;
    } else {
        entry->data = data;
    }
    entry->size = size;
    entry->command = command;
    entry->netId = netId;
    entry->headerSent = FALSE;
    if (queue->list.tail == NULL) {
        queue->list.tail = entry;
        queue->list.head = entry;
    } else {
        queue->list.tail->next = entry;
        entry->prev = queue->list.tail;
        queue->list.tail = entry;
    }
    return TRUE;
}

static NetQueueEntry *NetQueue_GetNext(NetQueue *queue) {
    if (queue->current != NULL) {
        return queue->current;
    }
    if (queue->list.head != NULL) {
        return queue->list.head;
    }
    if (queue->list2.head != NULL) {
        return queue->list2.head;
    }
    return NULL;
}

static void NetQueue_RemoveNext(NetQueue *queue) {
    BOOL ret;

    if (queue->current != NULL) {
        queue->current = NULL;
        return;
    }
    ret = NetQueueList_PopHead(&queue->list);
    if (!ret) {
        ret = NetQueueList_PopHead(&queue->list2);
        GFL_ASSERT(ret);
    }
}

BOOL NetQueue_FillBuffer(NetQueue *queue, NetSendBuffer *buffer) {
    BOOL checkSpace = FALSE;

    while (buffer->size != 0) {
        NetQueueEntry *entry = NetQueue_GetNext(queue);

        if (entry == NULL) {
            break;
        }
        NetQueue_RemoveNext(queue);
        if (!NetQueueEntry_Write(entry, buffer, queue->ring, checkSpace)) {
            queue->current = entry;
            break;
        }
        checkSpace = TRUE;
        if (buffer->size == 0xffff) {
            queue->current = entry;
            return FALSE;
        }
        MI_CpuFill8(entry, 0, sizeof(NetQueueEntry));
    }
    return TRUE;
}

void NetQueue_Init(NetQueue *queue, int count, NetRingBuff *ring, HeapID heapId) {
    MI_CpuFill8(queue, 0, sizeof(NetQueue));
    queue->entries = GFL_HeapAllocate(heapId, count * sizeof(NetQueueEntry), TRUE, "net_queue.c", 371);
    queue->count = count;
    queue->ring = ring;
}

void NetQueue_Clear(NetQueue *queue) {
    MI_CpuFill8(queue->entries, 0, sizeof(NetQueueEntry) * queue->count);
    queue->list.head = NULL;
    queue->list.tail = NULL;
    queue->list2.head = NULL;
    queue->list2.tail = NULL;
    queue->current = NULL;
}

void NetQueue_Free(NetQueue *queue) {
    GFL_HeapFree(queue->entries);
}

BOOL NetQueue_HasCommand(NetQueue *queue, u16 command) {
    int i;
    NetQueueEntry *entry = queue->entries;

    for (i = 0; i < queue->count; i++, entry++) {
        if (entry->command == command) {
            return TRUE;
        }
    }
    return FALSE;
}
