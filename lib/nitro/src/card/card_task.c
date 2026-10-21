// NitroSDK's CARD library, its second file in ARM9 main: the hooks that hear of the card's events, the thread that runs
// the commands' tasks, the DMA interface of a ROM read and the cache handling around the ARM7's DMA. It is apart from
// card_common.c because the hooks' list head opens a .bss of its own, after card_common.c's work (CARDi_NotifyEvent
// reads it at offset 0 through two literals), and the DMA interface is its .rodata. The file name is a guess,
// descriptive; the library has no file name string.
#include "nitro/card.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "twl/card.h"

// The first node of the list of hooks has only its link in use
static CARDHookContext sCardHookRoot = { NULL };

// The interface for the DS's DMA channels
static const CARDDmaInterface sCardDmaInterface = { MIi_CardDmaCopy32, MI_StopDma };

// CARDi_NotifyEvent: calls each hook with the event. The walk keeps the link before the current hook and moves it on
// only if the hook is still there, so a hook may remove itself
void CARDi_NotifyEvent(u32 command, u32 value) {
    OSIntrMode irq = OS_DisableInterrupts();
    CARDHookContext *prev = &sCardHookRoot;
    CARDHookContext *hook = sCardHookRoot.next;
    for (; hook != NULL; hook = prev->next) {
        if (hook->callback != NULL) {
            hook->callback(hook->userdata, command, value);
        }
        if (prev->next == hook) {
            prev = prev->next;
        }
    }
    OS_RestoreInterrupts(irq);
}

// Runs a task: on the thread if async, otherwise here and now. An async task is done when its callback is called
BOOL CARDi_ExecuteOldTypeTask(CARDiTask task, BOOL async) {
    CARDiCommon *p = &cardi_common;
    if (async) {
        OS_SetThreadPriority(&p->thread, p->priority);
        p->task = task;
        p->flag |= CARD_STAT_TASK;
        OS_WakeupThreadDirect(&p->thread);
    } else {
        task(p);
        CARDi_EndTask(p);
    }
    if (async) {
        return TRUE;
    }
    return p->cmd->result == CARD_RESULT_SUCCESS;
}

// Sleeps until a task is set, then runs it
void CARDi_OldTypeTaskThread(void *arg) {
    CARDiCommon *p = &cardi_common;
    while (TRUE) {
        OSIntrMode irq = OS_DisableInterrupts();
        for (;;) {
            if ((p->flag & CARD_STAT_TASK) != 0) {
                break;
            }
            OS_SleepThread(NULL);
        }
        OS_RestoreInterrupts(irq);
        CARDi_ExecuteOldTypeTask(p->task, FALSE);
    }
}

// Waits until the last command is done. A command that starts takes over the thread, with the callback to call when
// it ends. Returns whether the command block holds a success
BOOL CARDi_WaitForTask(CARDiCommon *p, BOOL start, MIDmaCallback callback, void *arg) {
    OSIntrMode irq = OS_DisableInterrupts();
    while ((p->flag & CARD_STAT_BUSY) != 0) {
        OS_SleepThread(&p->busyQueue);
    }
    if (start) {
        p->flag |= CARD_STAT_BUSY;
        p->callback = callback;
        p->callbackArg = arg;
    }
    OS_RestoreInterrupts(irq);
    return p->cmd->result == CARD_RESULT_SUCCESS;
}

// Ends a command: lets the next one start, then calls the callback of the one that ended
void CARDi_EndTask(CARDiCommon *p) {
    MIDmaCallback callback = p->callback;
    void *arg = p->callbackArg;
    OSIntrMode irq = OS_DisableInterrupts();
    p->flag &= ~(CARD_STAT_BUSY | CARD_STAT_TASK | CARD_STAT_CANCEL);
    OS_WakeupThread(&p->busyQueue);
    OS_RestoreInterrupts(irq);
    if (callback != NULL) {
        callback(arg);
    }
}

// The interface of a DMA channel for ROM reads: bit 4 of the number picks the DSi's new DMA, which only a DSi has.
// NULL for a channel that doesn't exist
const CARDDmaInterface *CARDi_GetDmaInterface(u32 dma) {
    const CARDDmaInterface *interface = NULL;
    BOOL twl = (dma & 0x10) != 0;
    dma &= ~0x10;
    if (dma <= 3) {
        if (!twl) {
            interface = &sCardDmaInterface;
        } else if (!OS_IsRunOnTwl()) {
            OS_Terminate();
        } else {
            interface = &CARDi_NdmaInterface;
        }
    }
    return interface;
}

// Discards the instruction cache of a range, or all of it from the threshold on
void CARDi_ICInvalidateSmart(void *addr, u32 len, u32 threshold) {
    if (len >= threshold) {
        IC_InvalidateAll();
    } else {
        IC_InvalidateRange(addr, len);
    }
}

// Discards the data cache of a range, or all of it from the threshold on. The lines at the ends of a range that they
// share with other data are written back first
void CARDi_DCInvalidateSmart(void *addr, u32 len, u32 threshold) {
    if (len >= threshold) {
        DC_FlushAll();
    } else {
        u32 offset = (u32)addr & 0x1f;
        if (offset != 0) {
            addr = (u8 *)addr - offset;
            DC_StoreRange(addr, 0x20);
            DC_StoreRange((u8 *)addr + len, 0x20);
            len += 0x20;
        }
        DC_InvalidateRange(addr, len);
        DC_WaitWriteBufferEmpty();
    }
}
