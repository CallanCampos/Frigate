#include "types.h"
#include "gfl/heap.h"
#include "gfl/net_queue.h"
#include "gfl/net_ring_buff.h"
#include "gfl/std.h"

static NetQueueEntry *func_0203e154(NetQueue *queue);
static int func_0203e188(NetQueue *queue);
static BOOL func_0203e1ac(NetQueueList *list);
static void func_0203e1d0(NetSendBuffer *buffer, u8 byte);
static BOOL func_0203e1e4(NetQueueEntry *entry, NetSendBuffer *buffer);
static BOOL func_0203e22c(NetQueueEntry *entry, NetSendBuffer *buffer, NetRingBuff *ring, BOOL checkSpace);
static NetQueueEntry *func_0203e364(NetQueue *queue);
static void func_0203e384(NetQueue *queue);

static NetQueueEntry *func_0203e154(NetQueue *queue) {
    NetQueueEntry *entry = queue->entries;
    int i;

    for (i = 0; i < queue->count; i++, entry++) {
        if (entry->command == 0) {
            return entry;
        }
    }
    return NULL;
}

BOOL func_0203e174(NetQueue *queue) {
    if (func_0203e188(queue) == 0) {
        return TRUE;
    }
    return FALSE;
}

static int func_0203e188(NetQueue *queue) {
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

static BOOL func_0203e1ac(NetQueueList *list) {
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

static void func_0203e1d0(NetSendBuffer *buffer, u8 byte) {
    *buffer->ptr = byte;
    buffer->ptr++;
    buffer->size--;
}

static BOOL func_0203e1e4(NetQueueEntry *entry, NetSendBuffer *buffer) {
    func_0203e1d0(buffer, entry->command >> 8);
    func_0203e1d0(buffer, entry->command);
    func_0203e1d0(buffer, entry->size >> 8);
    func_0203e1d0(buffer, entry->size);
    func_0203e1d0(buffer, entry->netId);
    return FALSE;
}

static BOOL func_0203e22c(NetQueueEntry *entry, NetSendBuffer *buffer, NetRingBuff *ring, BOOL checkSpace) {
    if (!entry->headerSent) {
        if (checkSpace && buffer->size < entry->size + NET_QUEUE_HEADER_SIZE) {
            return FALSE;
        }
        func_0203e1e4(entry, buffer);
    }
    if (buffer->size < entry->size) {
        if (entry->data == NULL) {
            func_0203e038(ring, buffer->ptr, buffer->size, buffer->size);
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
        func_0203e038(ring, buffer->ptr, entry->size, entry->size);
    } else {
        MI_CpuCopy8(entry->data, buffer->ptr, entry->size);
    }
    buffer->ptr += entry->size;
    buffer->size -= entry->size;
    return TRUE;
}

BOOL func_0203e2c0(NetQueue *queue, int command, u8 *data, int size, u32 unused, BOOL copy, int netId) {
    NetQueueEntry *entry = func_0203e154(queue);

    if (entry == NULL) {
        GFL_ASSERT(0);
        return FALSE;
    }
    // clang-format off
    GFL_ASSERT((u16)size < (0xffff/2));
    // clang-format on
    if (copy) {
        if (size + NET_QUEUE_HEADER_SIZE >= func_0203e110(queue->ring)) {
            GFL_ASSERT(0);
            return FALSE;
        }
        func_0203dfd0(queue->ring, data, size);
        func_0203e150(queue->ring);
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

static NetQueueEntry *func_0203e364(NetQueue *queue) {
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

static void func_0203e384(NetQueue *queue) {
    BOOL ret;

    if (queue->current != NULL) {
        queue->current = NULL;
        return;
    }
    ret = func_0203e1ac(&queue->list);
    if (!ret) {
        ret = func_0203e1ac(&queue->list2);
        GFL_ASSERT(ret);
    }
}

BOOL func_0203e3bc(NetQueue *queue, NetSendBuffer *buffer) {
    BOOL checkSpace = FALSE;

    while (buffer->size != 0) {
        NetQueueEntry *entry = func_0203e364(queue);

        if (entry == NULL) {
            break;
        }
        func_0203e384(queue);
        if (!func_0203e22c(entry, buffer, queue->ring, checkSpace)) {
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

void func_0203e418(NetQueue *queue, int count, NetRingBuff *ring, HeapID heapId) {
    MI_CpuFill8(queue, 0, sizeof(NetQueue));
    queue->entries = GFL_HeapAllocate(heapId, count * sizeof(NetQueueEntry), TRUE, "net_queue.c", 371);
    queue->count = count;
    queue->ring = ring;
}

void func_0203e44c(NetQueue *queue) {
    MI_CpuFill8(queue->entries, 0, sizeof(NetQueueEntry) * queue->count);
    queue->list.head = NULL;
    queue->list.tail = NULL;
    queue->list2.head = NULL;
    queue->list2.tail = NULL;
    queue->current = NULL;
}

void func_0203e46c(NetQueue *queue) {
    GFL_HeapFree(queue->entries);
}

BOOL func_0203e478(NetQueue *queue, u16 command) {
    int i;
    NetQueueEntry *entry = queue->entries;

    for (i = 0; i < queue->count; i++, entry++) {
        if (entry->command == command) {
            return TRUE;
        }
    }
    return FALSE;
}
