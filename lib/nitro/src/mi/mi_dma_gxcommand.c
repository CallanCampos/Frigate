// NitroSDK's mi_dma_gxcommand.c: sending a display list to the geometry engine's command FIFO by DMA, either a
// FIFO-full of it at a time or all at once.
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The most that the first of them sends at a time: 118 words, which fit in the FIFO (256 entries) when it is less than
// half full
#define MIi_GX_LENGTH_ONCE 0x1d8

typedef struct MIiGXDmaParams {
    // Set while a transfer runs, and cleared by an interrupt handler
    vu32 isBusy;
    u32 dmaNo;
    // What remains to be sent
    u32 src;
    u32 length;
    MIDmaCallback callback;
    void *arg;
    // What the FIFO interrupted for, and the function of its interrupt, until the transfer is over
    GXFifoIntrCond fifoCond;
    OSIrqFunction fifoFunc;
} MIiGXDmaParams;

static MIiGXDmaParams MIi_GXDmaParams;

static void MIi_FIFOCallback(void);
static void MIi_DMACallback(void *arg);
static void MIi_DMAFastCallback(void *arg);

void MI_SendGXCommandAsync(u32 dmaNo, const void *src, u32 length, MIDmaCallback callback, void *arg) {
    OSIntrMode enabled;

    if (length == 0) {
        if (callback != NULL) {
            callback(arg);
        }
        return;
    }
    while (MIi_GXDmaParams.isBusy) {
    }
    while (!(((reg_G3X_GXSTAT & REG_G3X_GXSTAT_FIFO_STAT_MASK) >> REG_G3X_GXSTAT_FIFO_STAT_SHIFT) &
             REG_G3X_GXSTAT_FIFO_LESS_HALF)) {
    }
    MIi_GXDmaParams.isBusy = TRUE;
    MIi_GXDmaParams.dmaNo = dmaNo;
    MIi_GXDmaParams.src = (u32)src;
    MIi_GXDmaParams.length = length;
    MIi_GXDmaParams.callback = callback;
    MIi_GXDmaParams.arg = arg;
    MIi_CheckDma0SourceAddress(dmaNo, (u32)src, length, MI_DMA_SRC_INC);
    MI_WaitDma(dmaNo);
    enabled = OS_DisableInterrupts();
    MIi_GXDmaParams.fifoCond = (GXFifoIntrCond)((reg_G3X_GXSTAT & REG_G3X_GXSTAT_FI_MASK) >> REG_G3X_GXSTAT_FI_SHIFT);
    MIi_GXDmaParams.fifoFunc = OS_GetIrqFunction(OS_IE_GXFIFO);
    G3X_SetFifoIntrCond(GX_FIFOINTR_COND_UNDERHALF);
    OS_SetIrqFunction(OS_IE_GXFIFO, MIi_FIFOCallback);
    OS_EnableIrqMask(OS_IE_GXFIFO);
    MIi_FIFOCallback();
    OS_RestoreInterrupts(enabled);
}

// The FIFO's interrupt: sends the next part, and the last with a DMA that ends in MIi_DMACallback
static void MIi_FIFOCallback(void) {
    u32 src;
    u32 part = MIi_GXDmaParams.length;

    if (part == 0) {
        return;
    }
    if (part >= MIi_GX_LENGTH_ONCE) {
        part = MIi_GX_LENGTH_ONCE;
    }
    src = MIi_GXDmaParams.src;
    MIi_GXDmaParams.length -= part;
    MIi_GXDmaParams.src += part;
    if (MIi_GXDmaParams.length == 0) {
        OSi_EnterDmaCallback(MIi_GXDmaParams.dmaNo, MIi_DMACallback, NULL);
        MIi_DmaSetParameters(MIi_GXDmaParams.dmaNo, src, REG_GXFIFO_ADDR,
                             MI_DMA_ENABLE | MI_DMA_IF_ENABLE | MI_DMA_32BIT_BUS | MI_DMA_DEST_FIX | (part >> 2), 0);
        OS_ResetRequestIrqMask(OS_IE_GXFIFO);
    } else {
        MIi_DmaSetParameters(MIi_GXDmaParams.dmaNo, src, REG_GXFIFO_ADDR,
                             MI_DMA_ENABLE | MI_DMA_32BIT_BUS | MI_DMA_DEST_FIX | (part >> 2), 0);
        OS_ResetRequestIrqMask(OS_IE_GXFIFO);
    }
}

// The end of the last part: restores what the FIFO interrupted for, and calls back
static void MIi_DMACallback(void *arg) {
    MIDmaCallback callback;
    void *callbackArg;

    OS_DisableIrqMask(OS_IE_GXFIFO);
    G3X_SetFifoIntrCond(MIi_GXDmaParams.fifoCond);
    OS_SetIrqFunction(OS_IE_GXFIFO, MIi_GXDmaParams.fifoFunc);
    MIi_GXDmaParams.isBusy = FALSE;
    callback = MIi_GXDmaParams.callback;
    callbackArg = MIi_GXDmaParams.arg;
    if (callback != NULL) {
        callback(callbackArg);
    }
}

void MI_SendGXCommandAsyncFast(u32 dmaNo, const void *src, u32 length, MIDmaCallback callback, void *arg) {
    if (length == 0) {
        if (callback != NULL) {
            callback(arg);
        }
        return;
    }
    while (MIi_GXDmaParams.isBusy) {
    }
    MIi_GXDmaParams.isBusy = TRUE;
    MIi_GXDmaParams.dmaNo = dmaNo;
    MIi_GXDmaParams.callback = callback;
    MIi_GXDmaParams.arg = arg;
    MIi_CheckAnotherAutoDMA(dmaNo, MI_DMA_TIMING_GXFIFO);
    MIi_CheckDma0SourceAddress(dmaNo, (u32)src, length, MI_DMA_SRC_INC);
    MI_WaitDma(dmaNo);
    OSi_EnterDmaCallback(dmaNo, MIi_DMAFastCallback, NULL);
    MIi_DmaSetParameters(dmaNo, (u32)src, REG_GXFIFO_ADDR,
                         MI_DMA_ENABLE | MI_DMA_IF_ENABLE | MI_DMA_TIMING_GXFIFO | MI_DMA_32BIT_BUS | MI_DMA_DEST_FIX |
                             (length >> 2),
                         0);
}

// The end of the DMA: calls back
static void MIi_DMAFastCallback(void *arg) {
    void *callbackArg;
    MIDmaCallback callback;

    MIi_GXDmaParams.isBusy = FALSE;
    callbackArg = MIi_GXDmaParams.arg;
    callback = MIi_GXDmaParams.callback;
    if (callback != NULL) {
        callback(callbackArg);
    }
}
