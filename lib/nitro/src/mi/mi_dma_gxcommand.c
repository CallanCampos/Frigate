// NitroSDK's mi_dma_gxcommand.c: sending a display list to the geometry engine's command FIFO by DMA, either a
// FIFO-full of it at a time or all at once.
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// The most that the first of them sends at a time: 118 words, which fit in the FIFO (256 entries) when it is less than
// half full
#define MI_GX_DMA_MAX_PART 0x1d8

typedef struct MIiGXDmaState {
    // Set while a transfer runs, and cleared by an interrupt handler
    vu32 isBusy;
    u32 dmaNo;
    // What remains to be sent
    u32 src;
    u32 length;
    MIDmaCallback callback;
    void *arg;
    // What the FIFO interrupted for, and the function of its interrupt, until the transfer is over
    u32 fifoIrqCondition;
    OSIrqFunction irqFunc;
} MIiGXDmaState;

static MIiGXDmaState sGXDmaState;

static void func_020784d0(void);
static void func_02078550(void *arg);
static void func_0207860c(void *arg);

void func_02078428(u32 dmaNo, const void *src, u32 length, MIDmaCallback callback, void *arg) {
    u32 enabled;

    if (length == 0) {
        if (callback != NULL) {
            callback(arg);
        }
        return;
    }
    while (sGXDmaState.isBusy) {
    }
    while (!(((reg_G3X_GXSTAT & REG_G3X_GXSTAT_FIFO_STAT_MASK) >> REG_G3X_GXSTAT_FIFO_STAT_SHIFT) &
             REG_G3X_GXSTAT_FIFO_LESS_HALF)) {
    }
    sGXDmaState.isBusy = TRUE;
    sGXDmaState.dmaNo = dmaNo;
    sGXDmaState.src = (u32)src;
    sGXDmaState.length = length;
    sGXDmaState.callback = callback;
    sGXDmaState.arg = arg;
    dma_check_source_valid(dmaNo, (u32)src, length, MI_DMA_SRC_INC);
    func_0207829c(dmaNo);
    enabled = CPU_IRQDisable();
    sGXDmaState.fifoIrqCondition = (reg_G3X_GXSTAT & REG_G3X_GXSTAT_FIFO_IRQ_MASK) >> REG_G3X_GXSTAT_FIFO_IRQ_SHIFT;
    sGXDmaState.irqFunc = func_02079d88(OS_IE_GXFIFO);
    reg_G3X_GXSTAT = (reg_G3X_GXSTAT & ~REG_G3X_GXSTAT_FIFO_IRQ_MASK) | (1 << REG_G3X_GXSTAT_FIFO_IRQ_SHIFT);
    CPU_SetIRQFunc(OS_IE_GXFIFO, func_020784d0);
    CPU_EnableInterrupts(OS_IE_GXFIFO);
    func_020784d0();
    CPU_SetIRQMask(enabled);
}

// The FIFO's interrupt: sends the next part, and the last with a DMA that ends in func_02078550
static void func_020784d0(void) {
    u32 src;
    u32 part = sGXDmaState.length;

    if (part == 0) {
        return;
    }
    if (part >= MI_GX_DMA_MAX_PART) {
        part = MI_GX_DMA_MAX_PART;
    }
    src = sGXDmaState.src;
    sGXDmaState.length -= part;
    sGXDmaState.src += part;
    if (sGXDmaState.length == 0) {
        irq_setDMACallback(sGXDmaState.dmaNo, func_02078550, NULL);
        dma_beginSyncEx(sGXDmaState.dmaNo, src, REG_GXFIFO_ADDR,
                        MI_DMA_ENABLE | MI_DMA_IF_ENABLE | MI_DMA_32BIT_BUF | MI_DMA_DEST_FIX | (part >> 2), 0);
        CPU_AcknowledgeInterrupts(OS_IE_GXFIFO);
    } else {
        dma_beginSyncEx(sGXDmaState.dmaNo, src, REG_GXFIFO_ADDR,
                        MI_DMA_ENABLE | MI_DMA_32BIT_BUF | MI_DMA_DEST_FIX | (part >> 2), 0);
        CPU_AcknowledgeInterrupts(OS_IE_GXFIFO);
    }
}

// The end of the last part: restores what the FIFO interrupted for, and calls back
static void func_02078550(void *arg) {
    MIDmaCallback callback;
    void *callbackArg;

    CPU_DisableInterrupts(OS_IE_GXFIFO);
    reg_G3X_GXSTAT = (sGXDmaState.fifoIrqCondition << REG_G3X_GXSTAT_FIFO_IRQ_SHIFT) |
                     (reg_G3X_GXSTAT & ~REG_G3X_GXSTAT_FIFO_IRQ_MASK);
    CPU_SetIRQFunc(OS_IE_GXFIFO, sGXDmaState.irqFunc);
    sGXDmaState.isBusy = FALSE;
    callback = sGXDmaState.callback;
    callbackArg = sGXDmaState.arg;
    if (callback != NULL) {
        callback(callbackArg);
    }
}

void func_02078594(u32 dmaNo, const void *src, u32 length, MIDmaCallback callback, void *arg) {
    if (length == 0) {
        if (callback != NULL) {
            callback(arg);
        }
        return;
    }
    while (sGXDmaState.isBusy) {
    }
    sGXDmaState.isBusy = TRUE;
    sGXDmaState.dmaNo = dmaNo;
    sGXDmaState.callback = callback;
    sGXDmaState.arg = arg;
    func_02078344(dmaNo, MI_DMA_TIMING_GXFIFO);
    dma_check_source_valid(dmaNo, (u32)src, length, MI_DMA_SRC_INC);
    func_0207829c(dmaNo);
    irq_setDMACallback(dmaNo, func_0207860c, NULL);
    dma_beginSyncEx(dmaNo, (u32)src, REG_GXFIFO_ADDR,
                    MI_DMA_ENABLE | MI_DMA_IF_ENABLE | MI_DMA_TIMING_GXFIFO | MI_DMA_32BIT_BUF | MI_DMA_DEST_FIX |
                        (length >> 2),
                    0);
}

// The end of the DMA: calls back
static void func_0207860c(void *arg) {
    void *callbackArg;
    MIDmaCallback callback;

    sGXDmaState.isBusy = FALSE;
    callbackArg = sGXDmaState.arg;
    callback = sGXDmaState.callback;
    if (callback != NULL) {
        callback(callbackArg);
    }
}
