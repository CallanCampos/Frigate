// The memory the voice packets come in: a pool of packets that are free. This is the first of four files that the ROM's
// vct_dsp.c range is (the split is the guess of the `.bss` runs, each sorted by size, and of `-ipa file` for each).
// libVCT's own split, in ntrtwl's reconstruction, calls it udp_buffer.c.

#include "nitro/mi.h"
#include "nitro/os.h"
#include "vct_dsp_internal.h"
#include "vct_internal.h"

// The packets that are free, and the memory they are made of
struct VCTPacketPool {
    UDPPacket *freeList;
    UDPPacket *base;
};

static VCTPacketPool sPacketPool;

BOOL vct_init_packet_buffer(void *buffer, u32 size) {
    unsigned int i;
    u32 count = size / sizeof(UDPPacket);

    sPacketPool.base = buffer;
    if (count < 4 || count > 0x48) {
        return FALSE;
    }
    if (buffer == NULL) {
        return FALSE;
    }
    if ((uptr)buffer & 0x1f) {
        sPacketPool.base = NULL;
        return FALSE;
    }
    MI_CpuClearFast(buffer, size);
    for (i = 0; i < count - 1; i++) {
        sPacketPool.base[i].free = &sPacketPool.base[i + 1];
    }
    (sPacketPool.base + count - 1)->free = NULL;
    sPacketPool.freeList = sPacketPool.base;
    return TRUE;
}

void vct_cleanup_packet_buffer(void) {
    sPacketPool.freeList = NULL;
    sPacketPool.base = NULL;
}

UDPPacket *vct_alloc_packet_buffer(void) {
    UDPPacket *packet = NULL;
    OSIntrMode irq = OS_DisableInterrupts();
    UDPPacket *head = sPacketPool.freeList;

    if (head != NULL) {
        packet = head;
        sPacketPool.freeList = head->free;
    }
    OS_RestoreInterrupts(irq);
    return packet;
}

void vct_free_packet_buffer(UDPPacket *packet) {
    OSIntrMode irq = OS_DisableInterrupts();

    packet->free = sPacketPool.freeList;
    sPacketPool.freeList = packet;
    packet->prev = NULL;
    packet->next = NULL;
    OS_RestoreInterrupts(irq);
}
