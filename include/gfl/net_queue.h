#ifndef POKEBW2_GFL_NET_QUEUE_H
#define POKEBW2_GFL_NET_QUEUE_H

#include "types.h"
#include "gfl/heap.h"
#include "gfl/net_ring_buff.h"

// Commands waiting to be sent, from the sender's memory or copied to a ring buffer. Sending writes each command's
// header and data into the packet, splitting a command that does not fit

typedef struct NetQueueEntry NetQueueEntry;

struct NetQueueEntry {
    // NULL for data in the ring buffer
    u8 *data;
    NetQueueEntry *prev;
    NetQueueEntry *next;
    u16 size;
    // 0 for a free entry
    u16 command;
    u8 netId;
    // Whether the header has been sent, and the rest of the data goes next
    u8 headerSent;
};

typedef struct {
    NetQueueEntry *head;
    NetQueueEntry *tail;
} NetQueueList;

typedef struct {
    NetQueueList list;
    NetQueueList list2;
    // A command partly sent
    NetQueueEntry *current;
    NetRingBuff *ring;
    NetQueueEntry *entries;
    int count;
    u32 unk20;
    // Whether the last packet left commands to send, which net_system.c marks in the next packet
    BOOL pending;
} NetQueue;

// Where sending writes, and how much room is left
typedef struct {
    u8 *ptr;
    u16 size;
} NetSendBuffer;

// The header before each command's data: the command, the size and the sender
#define NET_QUEUE_HEADER_SIZE 5

BOOL NetQueue_IsEmpty(NetQueue *queue);
BOOL NetQueue_Push(NetQueue *queue, int command, u8 *data, int size, u32 unused, BOOL copy, int netId);
// Fills the buffer, returning FALSE if a command did not fit and continues next time
BOOL NetQueue_FillBuffer(NetQueue *queue, NetSendBuffer *buffer);
void NetQueue_Init(NetQueue *queue, int count, NetRingBuff *ring, HeapID heapId);
void NetQueue_Clear(NetQueue *queue);
void NetQueue_Free(NetQueue *queue);
BOOL NetQueue_HasCommand(NetQueue *queue, u16 command);

#endif // POKEBW2_GFL_NET_QUEUE_H
