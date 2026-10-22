// The memory the voice packets come in: a pool of packets that are free. This is the first of four files that the ROM's
// vct_dsp.c range is (the split is the guess of the `.bss` runs, each sorted by size, and of `-ipa file` for each).
// libVCT's own split, in ntrtwl's reconstruction, calls it udp_buffer.c.

#include "nitro/mi.h"
#include "nitro/os.h"
#include "vct_dsp_internal.h"
#include "vct_internal.h"

// The packets that are free, and the memory they are made of
struct VCTPacketPool {
    VCTPacket *freeList;
    VCTPacket *base;
};

static VCTPacketPool sPacketPool;

BOOL VCT_InitPacketPool(void *buffer, u32 size) {
    unsigned int i;
    u32 count = size / sizeof(VCTPacket);

    sPacketPool.base = buffer;
    if (count < 4 || count > 0x48) {
        return FALSE;
    }
    if (buffer == NULL) {
        return FALSE;
    }
    if ((u32)buffer & 0x1f) {
        sPacketPool.base = NULL;
        return FALSE;
    }
    MI_CpuClearFast(buffer, size);
    for (i = 0; i < count - 1; i++) {
        sPacketPool.base[i].nextFree = &sPacketPool.base[i + 1];
    }
    (sPacketPool.base + count - 1)->nextFree = NULL;
    sPacketPool.freeList = sPacketPool.base;
    return TRUE;
}

void VCT_ClearPacketPool(void) {
    sPacketPool.freeList = NULL;
    sPacketPool.base = NULL;
}

VCTPacket *VCT_AllocPacket(void) {
    VCTPacket *packet = NULL;
    OSIntrMode irq = OS_DisableInterrupts();
    VCTPacket *head = sPacketPool.freeList;

    if (head != NULL) {
        packet = head;
        sPacketPool.freeList = head->nextFree;
    }
    OS_RestoreInterrupts(irq);
    return packet;
}

void VCT_FreePacket(VCTPacket *packet) {
    OSIntrMode irq = OS_DisableInterrupts();

    packet->nextFree = sPacketPool.freeList;
    sPacketPool.freeList = packet;
    packet->prev = NULL;
    packet->next = NULL;
    OS_RestoreInterrupts(irq);
}
