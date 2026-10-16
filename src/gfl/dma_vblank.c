#include "types.h"
#include "gfl/dma_vblank.h"
#include "nitro/os.h"

// Settings of the four DMA channels, written to their control registers at the vertical blank when requested. The
// game never sets up the table, so this does nothing. The file's name is a guess: there is no name for it in the ROM

#define DMA_CHANNEL_COUNT 4

// The control registers of the DMA channels, 12 bytes apart
#define reg_DMA_CNT(ch) (*(vu32 *)(0x040000b8 + (ch) * 12))

typedef struct {
    u32 src;
    u32 dst;
    u32 : 1;
    u32 irq : 1;
    u32 : 3;
    u32 is32bit : 1;
    u32 : 1;
    u32 srcCtrl : 2;
    u32 dstCtrl : 2;
    u32 count : 21;
    u32 update : 1;
    u32 active : 1;
    u32 stop : 1;
} DMAVBlankChannel;

static DMAVBlankChannel *sDMAChannels;

static void func_0204f8c8(int ch);

void func_0204f864(void) {
    int ch;

    if (sDMAChannels == NULL) {
        return;
    }
    for (ch = 0; ch < DMA_CHANNEL_COUNT; ch++) {
        if (sDMAChannels[ch].update) {
            sDMAChannels[ch].update = FALSE;
            if (sDMAChannels[ch].count != 0) {
                func_0204f8c8(ch);
                sDMAChannels[ch].active = TRUE;
            }
        }
        if (sDMAChannels[ch].stop) {
            sDMAChannels[ch].stop = FALSE;
            func_0204f8c8(ch);
        }
    }
}

static void func_0204f8c8(int ch) {
    u32 mode = OS_DisableInterrupts();
    DMAVBlankChannel *channel = &sDMAChannels[ch];

    reg_DMA_CNT(ch) = (channel->irq << 30) | (channel->is32bit << 26) | (channel->srcCtrl << 23) |
                      (channel->dstCtrl << 21) | channel->count;
    OS_RestoreInterrupts(mode);
}
