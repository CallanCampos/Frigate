// The ITCM part of NitroSDK's mi_dma.c: set up a DMA channel and wait for its register writes to settle.
#include "nitro/mi.h"
#include "nitro/os.h"

#define MI_DMA_FILL_DATA_ADDR 0x040000e0
#define MI_DMA0_DUMMY_CNT (MI_DMA_ENABLE | MI_DMA_SRC_FIX | MI_DMA_DEST_FIX | 1)

void MIi_DmaSetParameters(u32 dmaNo, u32 src, u32 dest, u32 ctrl, u32 flags) {
    u32 interrupts;
    MIDmaRegs *dma;

    if (!(flags & MI_DMA_SET_NO_INTR)) {
        interrupts = OS_DisableInterrupts();
    }

    dma = &MI_DMA_REGS[dmaNo];
    if (flags & MI_DMA_SET_FILL32) {
        ((vu32 *)MI_DMA_FILL_DATA_ADDR)[dmaNo] = src;
        src = MI_DMA_FILL_DATA_ADDR + dmaNo * sizeof(u32);
    } else if (flags & MI_DMA_SET_FILL16) {
        ((vu16 *)MI_DMA_FILL_DATA_ADDR)[dmaNo * 2] = src;
        src = MI_DMA_FILL_DATA_ADDR + dmaNo * sizeof(u32);
    }

    dma->src = src;
    dma->dest = dest;
    dma->cnt = ctrl;

    if (flags & MI_DMA_SET_WAIT) {
        (void)MI_DMA_REGS[0].src;
        (void)MI_DMA_REGS[0].src;
        if (!(flags & MI_DMA_SET_NO_DMA0_FIX) && dmaNo == 0) {
            dma->src = 0;
            dma->dest = 0;
            dma->cnt = MI_DMA0_DUMMY_CNT;
        }
    }

    if (!(flags & MI_DMA_SET_NO_INTR)) {
        OS_RestoreInterrupts(interrupts);
    }
    if (flags & MI_DMA_SET_WAIT) {
        (void)MI_DMA_REGS[0].src;
        (void)MI_DMA_REGS[0].src;
    }
}
