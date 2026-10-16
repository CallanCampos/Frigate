// NitroSDK's PXI (pxi_fifo.c): the FIFO between the ARM9 and the ARM7. Words carry a 5-bit tag, an error bit and 26
// bits of data; the tag picks the callback that runs for them when the ARM7 sends one.
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"

// How often to look at the ARM7's state before starting over, and how many calm rounds end the wait
#define PXI_SYNC_WAIT_COUNT 1000
#define PXI_SYNC_DONE_ROUNDS 4

// The function called for each tag, and whether the FIFO was set up
static PXIFifoCallback sFifoCallbacks[PXI_MAX_FIFO_TAG];
static u16 sFifoInitialized;

// The ARM7's state, which it sets in the bits of the register that the ARM9 reads
static inline u32 PXI_GetRemoteState(void) {
    return reg_PXI_MAINP_INTF & REG_PXI_MAINP_INTF_RECV_MASK;
}

// NitroSDK's PXI_InitFifo
void func_0207c954(void) {
    OSSystemWork *work = OS_GetSystemWork();
    u32 enabled = CPU_IRQDisable();

    if (!sFifoInitialized) {
        int i;
        int round;

        sFifoInitialized = TRUE;
        work->pxiSignalParam[MI_PROCESSOR_ARM9] = 0;
        for (i = 0; i < PXI_MAX_FIFO_TAG; i++) {
            sFifoCallbacks[i] = NULL;
        }

        reg_PXI_FIFO_CNT = REG_PXI_FIFO_CNT_INIT;
        CPU_AcknowledgeInterrupts(OS_IE_FIFO_RECV);
        CPU_SetIRQFunc(OS_IE_FIFO_RECV, func_0207cad4);
        CPU_EnableInterrupts(OS_IE_FIFO_RECV);

        // Hand the ARM7's state back to it until it has said 0 for a few rounds
        round = 0;
        while (TRUE) {
            u32 state = PXI_GetRemoteState();
            int wait;

            reg_PXI_MAINP_INTF = state << REG_PXI_MAINP_INTF_SEND_SHIFT;
            if (state == 0 && round > PXI_SYNC_DONE_ROUNDS) {
                break;
            }
            wait = PXI_SYNC_WAIT_COUNT;
            while ((reg_PXI_MAINP_INTF & REG_PXI_MAINP_INTF_RECV_MASK) == state) {
                if (wait <= 0) {
                    round = 0;
                    break;
                }
                wait--;
            }
            round++;
        }
    }
    CPU_SetIRQMask(enabled);
}

// NitroSDK's PXI_SetFifoRecvCallback
void func_0207c9fc(u32 fifoTag, PXIFifoCallback callback) {
    OSSystemWork *work = OS_GetSystemWork();
    u32 enabled = CPU_IRQDisable();

    sFifoCallbacks[fifoTag] = callback;
    if (callback != NULL) {
        work->pxiSignalParam[MI_PROCESSOR_ARM9] |= 1 << fifoTag;
    } else {
        work->pxiSignalParam[MI_PROCESSOR_ARM9] &= ~(1 << fifoTag);
    }
    CPU_SetIRQMask(enabled);
}

// NitroSDK's PXI_IsCallbackReady
BOOL func_0207ca48(u32 fifoTag, u32 processor) {
    OSSystemWork *work = OS_GetSystemWork();

    return (work->pxiSignalParam[processor] & (1 << fifoTag)) != 0;
}

// Puts a word in the send FIFO, unless the FIFO is in error (the error is acknowledged) or full
static inline int PXIi_SendRaw(u32 raw) {
    u32 enabled;

    if (reg_PXI_FIFO_CNT & REG_PXI_FIFO_CNT_ERR_MASK) {
        reg_PXI_FIFO_CNT |= REG_PXI_FIFO_CNT_ERR_ACK;
        return PXI_FIFO_FAIL_SEND_ERR;
    }
    enabled = CPU_IRQDisable();
    if (reg_PXI_FIFO_CNT & REG_PXI_FIFO_CNT_SEND_FULL_MASK) {
        CPU_SetIRQMask(enabled);
        return PXI_FIFO_FAIL_SEND_FULL;
    }
    reg_PXI_SEND_FIFO = raw;
    CPU_SetIRQMask(enabled);
    return PXI_FIFO_SUCCESS;
}

// NitroSDK's PXI_SendWordByFifo: sends a word with the tag, unless the FIFO is in error or full
int PXI_SendWordByFifo(u32 fifoTag, u32 data, BOOL err) {
    PXIFifoMessage message;

    message.e.tag = fifoTag;
    message.e.err = err;
    message.e.data = data;

    return PXIi_SendRaw(message.raw);
}

// Takes a word from the receive FIFO, unless the FIFO is in error (the error is acknowledged) or empty
static inline int PXIi_ReceiveRaw(u32 *raw) {
    u32 enabled;

    if (reg_PXI_FIFO_CNT & REG_PXI_FIFO_CNT_ERR_MASK) {
        reg_PXI_FIFO_CNT |= REG_PXI_FIFO_CNT_ERR_ACK;
        return PXI_FIFO_FAIL_RECV_ERR;
    }
    enabled = CPU_IRQDisable();
    if (reg_PXI_FIFO_CNT & REG_PXI_FIFO_CNT_RECV_EMPTY_MASK) {
        CPU_SetIRQMask(enabled);
        return PXI_FIFO_FAIL_RECV_EMPTY;
    }
    *raw = reg_PXI_RECV_FIFO;
    CPU_SetIRQMask(enabled);
    return PXI_FIFO_SUCCESS;
}

// NitroSDK's PXIi_HandlerRecvFifoNotEmpty: reads the words the ARM7 sent and calls the callback of each tag. A word
// for a tag without a callback is sent back with the error bit set
void func_0207cad4(void) {
    PXIFifoMessage message;
    int result;

    while (TRUE) {
        result = PXIi_ReceiveRaw(&message.raw);

        if (result == PXI_FIFO_FAIL_RECV_EMPTY) {
            break;
        }
        if (result == PXI_FIFO_FAIL_RECV_ERR) {
            continue;
        }
        if (message.e.tag == PXI_FIFO_TAG_EX) {
            continue;
        }
        if (sFifoCallbacks[message.e.tag] != NULL) {
            sFifoCallbacks[message.e.tag](message.e.tag, message.e.data, message.e.err);
        } else if (!message.e.err) {
            message.e.err = TRUE;
            PXIi_SendRaw(message.raw);
        }
    }
}
