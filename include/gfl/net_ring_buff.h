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

void func_0203dfc0(NetRingBuff *ring, u8 *pDataArea, int size);
// Writes size bytes, or sets the network error if they do not fit
void func_0203dfd0(NetRingBuff *ring, const u8 *pDataArea, int size);
// Reads up to size bytes, and at most max, returning how many it read
int func_0203e038(NetRingBuff *ring, u8 *dest, int size, int max);
u8 func_0203e054(NetRingBuff *ring);
// A big-endian u16
int func_0203e080(NetRingBuff *ring);
int func_0203e0f4(NetRingBuff *ring);
int func_0203e110(NetRingBuff *ring);
// Makes what has been written readable, which needs nothing here
void func_0203e150(NetRingBuff *ring);

#endif // POKEBW2_GFL_NET_RING_BUFF_H
