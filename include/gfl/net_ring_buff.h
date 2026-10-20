#ifndef POKEBW2_GFL_NET_RING_BUFF_H
#define POKEBW2_GFL_NET_RING_BUFF_H

#include "types.h"

// A ring buffer of bytes for the network library (net_ring_buff.c, a name the ROM does not embed; pokeplatinum's
// comm_ring_buffer.c is the same code)

typedef struct {
    u8 *pDataArea;
    int size;
    // Set by net_system.c when data from a machine arrives
    BOOL received;
    // Where reading and writing continue
    s16 startPos;
    // volatile: the original reloads it at each use; the receive path advances it while the main loop reads the buffer
    volatile s16 endPos;
} NetRingBuff;

void NetRingBuff_Init(NetRingBuff *ring, u8 *pDataArea, int size);
// Writes size bytes, or sets the network error if they do not fit
void NetRingBuff_Write(NetRingBuff *ring, const u8 *pDataArea, int size);
// Reads up to size bytes, and at most max, returning how many it read
int NetRingBuff_Read(NetRingBuff *ring, u8 *dest, int size, int max);
u8 NetRingBuff_ReadByte(NetRingBuff *ring);
// A big-endian u16
int NetRingBuff_ReadU16(NetRingBuff *ring);
int NetRingBuff_GetUsedSize(NetRingBuff *ring);
int NetRingBuff_GetFreeSize(NetRingBuff *ring);
// Makes what has been written readable, which needs nothing here
void NetRingBuff_Commit(NetRingBuff *ring);

#endif // POKEBW2_GFL_NET_RING_BUFF_H
