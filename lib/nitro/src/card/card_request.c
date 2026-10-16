// NitroSDK's CARD library, the commands for the ARM7: sending a command through the FIFO and waiting for the ARM7 to
// answer it. The file name is a guess, from NitroSDK's card_request.c, which these two functions came from; the binary
// can't tell whether they are a file of their own or belong to card_common.c or card_backup.c.
#include "nitro/card.h"
#include "nitro/os.h"
#include "nitro/pxi.h"

// The cycles to wait between two looks at whether the ARM7 is ready for the first command
#define CARD_ARM7_READY_SPIN 50

// CARDi_OnFifoRecv: called with each word the ARM7 sends. One with the error bit is the ARM7's acknowledgment of the
// command, which wakes the thread that waits for it
void func_0206f2e4(u32 fifoTag, u32 data, BOOL err) {
    if (fifoTag == PXI_FIFO_TAG_FS && err) {
        cardi_common.flag &= ~CARD_STAT_WAITFOR7ACK;
        scheduler_start_thread(cardi_common.curThread);
    }
}

// CARDi_Request: sends a command for the command block to the ARM7 and waits for its acknowledgment, sending it again
// while the answer is a timeout, up to retry times. The first command also starts the ARM7 side. Returns whether the
// block holds a success
BOOL func_0206f30c(CARDiCommon *p, int reqType, int retry) {
    if ((p->flag & CARD_STAT_INIT_CMD) == 0) {
        p->flag |= CARD_STAT_INIT_CMD;
        if (!func_0207ca48(PXI_FIFO_TAG_FS, PXI_PROC_ARM7)) {
            do {
                func_0207c160(CARD_ARM7_READY_SPIN);
            } while (!func_0207ca48(PXI_FIFO_TAG_FS, PXI_PROC_ARM7));
        }
        func_0206f30c(p, CARD_REQ_INIT, 1);
    }
    cp15_flushDC(p->cmd, sizeof(CARDiCommandArg));
    cp15_drainWriteBuffer();
    p->curThread = OS_GetCurrentThread();
    do {
        p->flag |= CARD_STAT_WAITFOR7ACK;
        while (PXI_SendWordByFifo(PXI_FIFO_TAG_FS, reqType, TRUE) < 0) {
        }
        if (reqType == CARD_REQ_INIT) {
            u32 arg = (u32)p->cmd;
            while (PXI_SendWordByFifo(PXI_FIFO_TAG_FS, arg, TRUE) < 0) {
            }
        }
        {
            u32 irq = CPU_IRQDisable();
            while ((p->flag & CARD_STAT_WAITFOR7ACK) != 0) {
                scheduler_yield(NULL);
            }
            CPU_SetIRQMask(irq);
        }
        cp15_invalidateDC(p->cmd, sizeof(CARDiCommandArg));
    } while (p->cmd->result == CARD_RESULT_TIMEOUT && --retry > 0);
    return p->cmd->result == CARD_RESULT_SUCCESS;
}
