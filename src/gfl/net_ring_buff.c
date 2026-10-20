#include "types.h"
#include "gfl/net_ring_buff.h"
#include "gfl/net_system.h"
#include "gfl/std.h"

static int NetRingBuff_Peek(NetRingBuff *ring, u8 *dest, int size, int max);
static int NetRingBuff_WrapPos(NetRingBuff *ring, int pos);
static int NetRingBuff_CalcFreeSize(NetRingBuff *ring);

void NetRingBuff_Init(NetRingBuff *ring, u8 *pDataArea, int size) {
    ring->pDataArea = pDataArea;
    ring->size = (s16)size;
    ring->startPos = 0;
    ring->endPos = 0;
}

void NetRingBuff_Write(NetRingBuff *ring, const u8 *pDataArea, int size) {
    int i;
    int j;

    if (NetRingBuff_CalcFreeSize(ring) <= size) {
        NetSystem_SetError();
        return;
    }
    j = 0;
    for (i = ring->endPos; i < ring->endPos + size; i++, j++) {
        GFL_ASSERT(pDataArea);
        ring->pDataArea[NetRingBuff_WrapPos(ring, i)] = pDataArea[j];
    }
    ring->endPos = NetRingBuff_WrapPos(ring, i);
}

int NetRingBuff_Read(NetRingBuff *ring, u8 *dest, int size, int max) {
    int read = NetRingBuff_Peek(ring, dest, size, max);

    ring->startPos = NetRingBuff_WrapPos(ring, ring->startPos + read);
    return read;
}

u8 NetRingBuff_ReadByte(NetRingBuff *ring) {
    u8 byte;

    if (NetRingBuff_Read(ring, &byte, 1, 1) == 1) {
        return byte;
    }
    GFL_ASSERT(0);
    return 0;
}

int NetRingBuff_ReadU16(NetRingBuff *ring) {
    u16 high = NetRingBuff_ReadByte(ring) << 8;

    return (u16)(high | NetRingBuff_ReadByte(ring));
}

static int NetRingBuff_Peek(NetRingBuff *ring, u8 *dest, int size, int max) {
    int i;
    int j = 0;

    for (i = ring->startPos; i < ring->startPos + size; i++) {
        if (ring->endPos == NetRingBuff_WrapPos(ring, i) || j == max) {
            return j;
        }
        dest[j] = ring->pDataArea[NetRingBuff_WrapPos(ring, i)];
        j++;
    }
    return j;
}

// The bytes written but not read
int NetRingBuff_GetUsedSize(NetRingBuff *ring) {
    if (ring->startPos > ring->endPos) {
        return ring->size + ring->endPos - ring->startPos;
    }
    return ring->endPos - ring->startPos;
}

// The bytes that can be written
int NetRingBuff_GetFreeSize(NetRingBuff *ring) {
    return ring->size - NetRingBuff_GetUsedSize(ring);
}

static int NetRingBuff_WrapPos(NetRingBuff *ring, int pos) {
    return pos % ring->size;
}

static int NetRingBuff_CalcFreeSize(NetRingBuff *ring) {
    int end;

    if (ring->startPos > ring->endPos) {
        end = ring->size + ring->endPos;
    } else {
        end = ring->endPos;
    }
    end -= ring->startPos;
    return ring->size - end;
}

void NetRingBuff_Commit(NetRingBuff *ring) {
}
