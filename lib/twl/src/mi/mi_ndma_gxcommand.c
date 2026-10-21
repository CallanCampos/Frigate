// TwlSDK's display-list sends by the new DMA channels, in the LTD autoload: NitroSDK's mi_dma_gxcommand.c (ARM9 main)
// for the DSi's channels, either a FIFO-full of the list at a time or all at once. The file name is inferred from
// NitroSDK's, and the static functions are named after their NitroSDK counterparts
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "twl/mi.h"

// The most that the first of them sends at a time: 118 words, which fit in the FIFO (256 entries) when it is less than
// half full
#define MI_GX_DMA_MAX_PART 0x1d8

// The address modes of MIi_NDmaAsync's transfers to a fixed destination, without and with the interrupt at the end
#define MI_NDMA_MODE_SEND 8
#define MI_NDMA_MODE_SEND_IF 9

typedef struct MIiGXNDmaState {
    // Set while a transfer runs, and cleared by an interrupt handler
    vu32 isBusy;
    u32 ndmaNo;
    // What remains to be sent
    u32 src;
    u32 length;
    MINDmaCallback callback;
    void *arg;
    // What the FIFO interrupted for, and the function of its interrupt, until the transfer is over
    GXFifoIntrCond fifoIrqCondition;
    OSIrqFunction irqFunc;
} MIiGXNDmaState;

static void MIi_NDmaFIFOCallback(void);
static void MIi_NDmaDMACallback(void *arg);
static void MIi_NDmaDMAFastCallback(void *arg);

static MIiGXNDmaState sGXNDmaState;

#pragma define_section LTDMAIN ".ltdmain"                                                                              \
                               ".ltdmain.bss"                                                                          \
                               ".ltdmain" abs32 RWX
#pragma section LTDMAIN begin

void MI_SendNDmaGXCommandAsync(u32 ndmaNo, const void *src, u32 length, MINDmaCallback callback, void *arg) {
    u32 enabled;

    if (length == 0) {
        if (callback != NULL) {
            callback(arg);
        }
        return;
    }
    while (sGXNDmaState.isBusy) {
    }
    while (!(((reg_G3X_GXSTAT & REG_G3X_GXSTAT_FIFO_STAT_MASK) >> REG_G3X_GXSTAT_FIFO_STAT_SHIFT) &
             REG_G3X_GXSTAT_FIFO_LESS_HALF)) {
    }
    sGXNDmaState.isBusy = TRUE;
    sGXNDmaState.ndmaNo = ndmaNo;
    sGXNDmaState.src = (u32)src;
    sGXNDmaState.length = length;
    sGXNDmaState.callback = callback;
    sGXNDmaState.arg = arg;
    MI_WaitNDma(ndmaNo);
    enabled = OS_DisableInterrupts();
    sGXNDmaState.fifoIrqCondition =
        (GXFifoIntrCond)((reg_G3X_GXSTAT & REG_G3X_GXSTAT_FIFO_IRQ_MASK) >> REG_G3X_GXSTAT_FIFO_IRQ_SHIFT);
    sGXNDmaState.irqFunc = OS_GetIrqFunction(OS_IE_GXFIFO);
    G3X_SetFifoIntrCond(GX_FIFOINTR_COND_UNDERHALF);
    OS_SetIrqFunction(OS_IE_GXFIFO, MIi_NDmaFIFOCallback);
    OS_EnableIrqMask(OS_IE_GXFIFO);
    MIi_NDmaFIFOCallback();
    OS_RestoreInterrupts(enabled);
}

// The FIFO's interrupt: sends the next part, and the last with an interrupt that ends in MIi_NDmaDMACallback
static void MIi_NDmaFIFOCallback(void) {
    u32 src;
    u32 part = sGXNDmaState.length;

    if (part == 0) {
        return;
    }
    if (part >= MI_GX_DMA_MAX_PART) {
        part = MI_GX_DMA_MAX_PART;
    }
    src = sGXNDmaState.src;
    sGXNDmaState.length -= part;
    sGXNDmaState.src += part;
    if (sGXNDmaState.length == 0) {
        OS_DisableIrqMask(OS_IE_GXFIFO);
    }
    OS_ResetRequestIrqMask(OS_IE_GXFIFO);
    MIi_NDmaAsyncImmediate((sGXNDmaState.length == 0) ? MI_NDMA_MODE_SEND_IF : MI_NDMA_MODE_SEND, sGXNDmaState.ndmaNo,
                           (const void *)src, (void *)REG_GXFIFO_ADDR, 0, part,
                           (sGXNDmaState.length == 0) ? MIi_NDmaDMACallback : NULL, NULL, MI_NDMA_ENABLE);
}

// The end of the last part: restores what the FIFO interrupted for, and calls back
static void MIi_NDmaDMACallback(void *arg) {
    MINDmaCallback callback;
    void *callbackArg;

    OS_DisableIrqMask(OS_IE_GXFIFO);
    G3X_SetFifoIntrCond(sGXNDmaState.fifoIrqCondition);
    OS_SetIrqFunction(OS_IE_GXFIFO, sGXNDmaState.irqFunc);
    sGXNDmaState.isBusy = FALSE;
    callback = sGXNDmaState.callback;
    callbackArg = sGXNDmaState.arg;
    if (callback != NULL) {
        callback(callbackArg);
    }
}

void MI_SendNDmaGXCommandAsyncFast(u32 ndmaNo, const void *src, u32 length, MINDmaCallback callback, void *arg) {
    if (length == 0) {
        if (callback != NULL) {
            callback(arg);
        }
        return;
    }
    while (sGXNDmaState.isBusy) {
    }
    sGXNDmaState.isBusy = TRUE;
    sGXNDmaState.ndmaNo = ndmaNo;
    sGXNDmaState.callback = callback;
    sGXNDmaState.arg = arg;
    MI_WaitNDma(ndmaNo);
    MIi_NDmaAsyncImmediate(MI_NDMA_MODE_SEND_IF, ndmaNo, src, (void *)REG_GXFIFO_ADDR, 0, length,
                           MIi_NDmaDMAFastCallback, NULL, MI_NDMA_ENABLE);
}

// The end of the transfer: calls back
static void MIi_NDmaDMAFastCallback(void *arg) {
    void *callbackArg;
    MINDmaCallback callback;

    sGXNDmaState.isBusy = FALSE;
    callbackArg = sGXNDmaState.arg;
    callback = sGXNDmaState.callback;
    if (callback != NULL) {
        callback(callbackArg);
    }
}

#pragma section LTDMAIN end
