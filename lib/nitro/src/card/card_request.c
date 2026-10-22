// NitroSDK's CARD library, the commands for the ARM7: sending a command through the FIFO and waiting for the ARM7 to
// answer it. The file name is NitroSDK's card_request.c, which has these two functions in ntrtwl's reconstruction of
// NitroSDK; the binary can't tell whether they are a file of their own or belong to card_common.c or card_backup.c.
#include "nitro/card.h"
#include "nitro/os.h"
#include "nitro/pxi.h"

// The cycles to wait between two looks at whether the ARM7 is ready for the first command
#define CARD_ARM7_READY_SPIN 50

// CARDi_OnFifoRecv: called with each word the ARM7 sends. One with the error bit is the ARM7's acknowledgment of the
// command, which wakes the thread that waits for it
void CARDi_OnFifoRecv(PXIFifoTag fifoTag, u32 data, BOOL err) {
    if (fifoTag == PXI_FIFO_TAG_FS && err) {
        cardi_common.flag &= ~CARD_STAT_REQ;
        OS_WakeupThreadDirect(cardi_common.curThread);
    }
}

// CARDi_Request: sends a command for the command block to the ARM7 and waits for its acknowledgment, sending it again
// while the answer is a timeout, up to retry times. The first command also starts the ARM7 side. Returns whether the
// block holds a success
BOOL CARDi_Request(CARDiCommon *p, int reqType, int retry) {
    if ((p->flag & CARD_STAT_INIT_CMD) == 0) {
        p->flag |= CARD_STAT_INIT_CMD;
        while (!PXI_IsCallbackReady(PXI_FIFO_TAG_FS, PXI_PROC_ARM7)) {
            OS_SpinWaitSysCycles(CARD_ARM7_READY_SPIN);
        }
        CARDi_Request(p, CARD_REQ_INIT, 1);
    }
    DC_FlushRange(p->cmd, sizeof(CARDiCommandArg));
    DC_WaitWriteBufferEmpty();
    p->curThread = OS_GetCurrentThread();
    do {
        p->flag |= CARD_STAT_REQ;
        while (PXI_SendWordByFifo(PXI_FIFO_TAG_FS, reqType, TRUE) < 0) {
        }
        if (reqType == CARD_REQ_INIT) {
            u32 arg = (u32)p->cmd;
            while (PXI_SendWordByFifo(PXI_FIFO_TAG_FS, arg, TRUE) < 0) {
            }
        }
        {
            OSIntrMode irq = OS_DisableInterrupts();
            while ((p->flag & CARD_STAT_REQ) != 0) {
                OS_SleepThread(NULL);
            }
            OS_RestoreInterrupts(irq);
        }
        DC_InvalidateRange(p->cmd, sizeof(CARDiCommandArg));
    } while (p->cmd->result == CARD_RESULT_TIMEOUT && --retry > 0);
    return p->cmd->result == CARD_RESULT_SUCCESS;
}
