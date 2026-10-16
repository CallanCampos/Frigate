// NitroSDK's mi_dma_card.c (the name is a guess): the DMA setup that CARD uses to read the cartridge's data port. The
// card code's table of DMA functions points to it, so it is global.
#include "nitro/mi.h"

// One word that repeats whenever the card has one ready (continuous mode, started by the card's own timing): from a
// fixed source address, the data port, to a destination that advances
#define MI_DMA_CARD_CNT                                                                                                \
    (MI_DMA_ENABLE | MI_DMA_TIMING_CARD | MI_DMA_32BIT_BUF | MI_DMA_CONTINUOUS_ON | MI_DMA_SRC_FIX | 1)

// NitroSDK's MIi_CardDmaCopy32. The size is not the transfer's count, which is 1; it only goes to the check of the
// source
void MIi_CardDmaCopy32(u32 dmaNo, const void *src, void *dest, u32 size) {
    vu32 *dmaCntp;

    MIi_CheckAnotherAutoDMA(dmaNo, (u32)-1);
    MIi_CheckDma0SourceAddress(dmaNo, (u32)src, size, MI_DMA_SRC_FIX);
    if (size != 0) {
        dmaCntp = &((vu32 *)REG_DMA0SAD_ADDR)[dmaNo * 3 + 2];
        while (*dmaCntp & MI_DMA_ENABLE) {
        }
        MIi_DmaSetParameters(dmaNo, (u32)src, (u32)dest, MI_DMA_CARD_CNT, 0);
    }
}
