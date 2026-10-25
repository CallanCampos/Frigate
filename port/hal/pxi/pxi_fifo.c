// NitroSDK's PXI on the host (pxi_fifo.c and pxi_init.c, which port/hal/pxi/replaced.txt keeps out of the game's
// sources): the FIFO between the ARM9 and the ARM7, with the ARM7's side in host C (pxi_bus.h). The ARM9's functions
// keep the decomp's interface and behaviour; what changes is what is behind the registers:
//   - PXI_InitFifo doesn't wait for the ARM7 through reg_PXI_MAINP_INTF: the host's ARM7 boots right there
//     (port_arm7_boot), as the DS's has by the time its handshake ends;
//   - a word that PXI_SendWordByFifo sends goes to the ARM7's service of its tag at once, and the send FIFO is never
//     full or in error, since the ARM7 takes each word as it comes;
//   - the receive FIFO is a queue of the words that the services send, which raises the FIFO's interrupt
//     (OS_IE_FIFO_RECV) once PXI_InitFifo has enabled it, as its write of REG_PXI_FIFO_CNT_INIT does on the DS. The
//     FIFO's registers themselves are never read or written.
// PXIi_HandlerRecvFifoNotEmpty and the table of the tags each processor is ready for (OSSystemWork's
// pxiHandleChecker, which PXI_IsCallbackReady reads) are the DS's.
#include "hal/os/port_os.h"
#include "hal/pxi/arm7.h"
#include "hal/pxi/pxi_bus.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include <stdio.h>
#include <stdlib.h>

// The receive FIFO's depth. The DS's holds 16 words and its ARM7 waits while it is full; the host's ARM7 can't wait,
// so the queue is deep enough for every answer a frame brings (the sound driver's alarms are the most)
#define RECEIVE_CAPACITY 4096

// The ARM9's callback for each tag, and whether the FIFO was set up (the SDK's statics)
static PXIFifoCallback FifoRecvCallbackTable[PXI_MAX_FIFO_TAG];
static u16 FifoCtrlInit;

// The ARM7's service for each tag, and the words it sent that the ARM9 hasn't read, a ring
static PortPxiService sServices[PXI_MAX_FIFO_TAG];
static u32 sReceive[RECEIVE_CAPACITY];
static u32 sReceiveHead;
static u32 sReceiveCount;
// Whether the receive FIFO's interrupt is enabled, as PXI_InitFifo's REG_PXI_FIFO_CNT_INIT enables it
static BOOL sReceiveIrqEnabled;

static void Fatal(const char *message) {
    fprintf(stderr, "\nport: PXI: %s\n", message);
    fflush(stderr);
    abort();
}

// The ARM7's half of the table of ready tags, from the services that are registered
static void PublishArm7Tags(void) {
    u32 ready = 0;
    int tag;

    for (tag = 0; tag < PXI_MAX_FIFO_TAG; tag++) {
        if (sServices[tag] != NULL) {
            ready |= 1u << tag;
        }
    }
    OS_GetSystemWork()->pxiHandleChecker[MI_PROCESSOR_ARM7] = ready;
}

// The FIFO's interrupt, when the ARM9 enabled it and a word waits
static void RaiseReceiveIrq(void) {
    if (sReceiveIrqEnabled && sReceiveCount != 0) {
        port_os_request_irq(OS_IE_FIFO_RECV);
    }
}

void port_pxi_register_service(u32 tag, PortPxiService service) {
    if (tag >= PXI_MAX_FIFO_TAG) {
        Fatal("a service for a tag beyond the FIFO's 32");
    }
    sServices[tag] = service;
    PublishArm7Tags();
}

void port_pxi_reply(u32 tag, u32 data, BOOL err) {
    PXIFifoMessage message;

    if (sReceiveCount == RECEIVE_CAPACITY) {
        Fatal("the ARM9 doesn't read its receive FIFO: 4096 words of the ARM7 wait");
    }
    message.raw = 0;
    message.e.tag = tag;
    message.e.err = err ? 1 : 0;
    message.e.data = data & PORT_PXI_DATA_MASK;
    sReceive[(sReceiveHead + sReceiveCount) % RECEIVE_CAPACITY] = message.raw;
    sReceiveCount++;
    RaiseReceiveIrq();
}

u32 port_pxi_pending(void) {
    return sReceiveCount;
}

void port_pxi_reset(void) {
    int i;

    for (i = 0; i < PXI_MAX_FIFO_TAG; i++) {
        FifoRecvCallbackTable[i] = NULL;
        sServices[i] = NULL;
    }
    FifoCtrlInit = FALSE;
    sReceiveHead = 0;
    sReceiveCount = 0;
    sReceiveIrqEnabled = FALSE;
    OS_GetSystemWork()->pxiHandleChecker[MI_PROCESSOR_ARM9] = 0;
    OS_GetSystemWork()->pxiHandleChecker[MI_PROCESSOR_ARM7] = 0;
    port_pxi_forget_pointers();
    port_arm7_reset();
}

// The ARM7's side of a word that the ARM9 sends: its service, or the DS's ARM7's answer for a tag it has no callback
// for, which is the word with the error bit, unless it had it already
static void DeliverToArm7(u32 raw) {
    PXIFifoMessage message;

    message.raw = raw;
    if (sServices[message.e.tag] != NULL) {
        sServices[message.e.tag](message.e.tag, message.e.data, message.e.err);
    } else if (!message.e.err) {
        port_pxi_reply(message.e.tag, message.e.data, TRUE);
    }
}

void PXI_Init(void) {
    PXI_InitFifo();
}

// NitroSDK's PXI_InitFifo: no callbacks, the receive FIFO's interrupt to PXIi_HandlerRecvFifoNotEmpty, and the ARM7's
// boot where the DS waits for the ARM7 to answer through reg_PXI_MAINP_INTF
void PXI_InitFifo(void) {
    OSSystemWork *work = OS_GetSystemWork();
    u32 enabled = OS_DisableInterrupts();

    if (!FifoCtrlInit) {
        int i;

        FifoCtrlInit = TRUE;
        work->pxiHandleChecker[MI_PROCESSOR_ARM9] = 0;
        for (i = 0; i < PXI_MAX_FIFO_TAG; i++) {
            FifoRecvCallbackTable[i] = NULL;
        }

        OS_ResetRequestIrqMask(OS_IE_FIFO_RECV);
        OS_SetIrqFunction(OS_IE_FIFO_RECV, PXIi_HandlerRecvFifoNotEmpty);
        OS_EnableIrqMask(OS_IE_FIFO_RECV);
        sReceiveIrqEnabled = TRUE;

        port_arm7_boot();
        PublishArm7Tags();
        RaiseReceiveIrq();
    }
    OS_RestoreInterrupts(enabled);
}

void PXI_SetFifoRecvCallback(int fifoTag, PXIFifoCallback callback) {
    OSSystemWork *work = OS_GetSystemWork();
    u32 enabled = OS_DisableInterrupts();

    port_pxi_observe_arena();
    FifoRecvCallbackTable[fifoTag] = callback;
    if (callback != NULL) {
        work->pxiHandleChecker[MI_PROCESSOR_ARM9] |= 1u << fifoTag;
    } else {
        work->pxiHandleChecker[MI_PROCESSOR_ARM9] &= ~(1u << fifoTag);
    }
    OS_RestoreInterrupts(enabled);
}

BOOL PXI_IsCallbackReady(int fifoTag, PXIProc processor) {
    OSSystemWork *work = OS_GetSystemWork();

    return (work->pxiHandleChecker[processor] & (1u << fifoTag)) != 0;
}

// NitroSDK's PXI_SendWordByFifo: the ARM7 takes the word at once, so the FIFO is never full. Then the interrupts that
// the ARM7's answer raised are taken, if the ARM9 can take them now
int PXI_SendWordByFifo(int fifoTag, u32 data, BOOL err) {
    PXIFifoMessage message;

    message.raw = 0;
    message.e.tag = fifoTag;
    message.e.err = err ? 1 : 0;
    message.e.data = data;
    DeliverToArm7(message.raw);
    port_os_poll_irq();
    return PXI_FIFO_SUCCESS;
}

// Takes a word from the receive FIFO, unless it is empty
static int PXIi_ReceiveRaw(u32 *raw) {
    u32 enabled = OS_DisableInterrupts();

    if (sReceiveCount == 0) {
        OS_RestoreInterrupts(enabled);
        return PXI_FIFO_FAIL_RECV_EMPTY;
    }
    *raw = sReceive[sReceiveHead];
    sReceiveHead = (sReceiveHead + 1) % RECEIVE_CAPACITY;
    sReceiveCount--;
    OS_RestoreInterrupts(enabled);
    return PXI_FIFO_SUCCESS;
}

// NitroSDK's PXIi_HandlerRecvFifoNotEmpty: reads the words the ARM7 sent and calls the callback of each tag. A word for
// a tag without a callback is sent back with the error bit set
void PXIi_HandlerRecvFifoNotEmpty(void) {
    PXIFifoMessage message;

    while (PXIi_ReceiveRaw(&message.raw) == PXI_FIFO_SUCCESS) {
        if (message.e.tag == PXI_FIFO_TAG_EX) {
            continue;
        }
        if (FifoRecvCallbackTable[message.e.tag] != NULL) {
            FifoRecvCallbackTable[message.e.tag](message.e.tag, message.e.data, message.e.err);
        } else if (!message.e.err) {
            message.e.err = TRUE;
            DeliverToArm7(message.raw);
        }
    }
}
