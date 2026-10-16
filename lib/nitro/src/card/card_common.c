// NitroSDK's CARD library, the part that ROM and backup commands share: initialization, the lock of the card's
// resources, the thread that runs the commands' tasks, and the cache handling around the ARM7's DMA. The file name is
// a guess, descriptive from the SDK's structure (card_common.c); the library has no file name string.
#include "nitro/card.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "twl/card.h"

// What CARD_LockRom waits with: an event, signaled once the card is locked, and the vertical-blank alarm that polls for
// it
typedef struct {
    OSEvent event;
    OSVAlarm alarm;
    BOOL (*condition)(void *arg);
    void *arg;
    // Not read or written, but CARD_LockRom's frame has the room for it
    u32 unk_3c;
} CARDRomLockPoll;

static void CARDi_PollRomLock(void *arg);
static BOOL CARDi_TryLockCard(void *arg);
static BOOL CARD_IsEnabled(void);
static void CARD_Enable(BOOL enable);
static void CARDi_LockResource(u16 lockId, int target);
static void CARDi_UnlockResource(u16 lockId, int target);
static int CARDi_InitResourceLock(void);
static void CARDi_InitCommand(void);
static void CARDi_OldTypeTaskThread(void *arg); // swan's cart_read_thread_proc

// The first node of the list of hooks has only its link in use. The statics lie in memory in reverse of the order here
static CARDHookContext sCardHookRoot;
CARDiCommon cardi_common;
static CARDiCommandArg cardi_arg;
static BOOL sCardEnabled;

// The interface for the DS's DMA channels
static const CARDDmaInterface sCardDmaInterface = { MIi_CardDmaCopy32, MI_StopDma };

// Calls the condition each frame, at the first line of the vertical blank, until it holds
static void CARDi_PollRomLock(void *arg) {
    CARDRomLockPoll *poll = arg;
    if (poll->condition(poll->arg)) {
        OS_SignalEvent(&poll->event, 1);
    } else {
        OS_SetVAlarm(&poll->alarm, 192, 263, CARDi_PollRomLock, poll);
    }
}

// Whether the card's lock was free, and is the caller's now
static BOOL CARDi_TryLockCard(void *arg) {
    return OS_TryLockCard(*(u16 *)arg) == 0;
}

// CARD_Init: sets up the library's work, starts the thread that runs the commands' tasks and, on a ROM boot, enables
// the card
void CARD_Init(void) {
    CARDiCommon *p = &cardi_common;
    if (p->flag == 0) {
        p->flag = CARD_STAT_INIT;
        if (OS_GetBootType() == OS_BOOTTYPE_ROM) {
            MI_CpuCopy8((void *)HW_ROM_HEADER_BUF, (void *)HW_CARD_ROM_HEADER, HW_CARD_ROM_HEADER_SIZE);
        }
        p->src = 0;
        p->dst = 0;
        p->len = 0;
        p->dma = (u32)-1;
        p->dmaInterface = NULL;
        p->dcInvalidateThreshold = 0x2400;
        p->icInvalidateThreshold = 0x400;
        cardi_rom_base = 0;
        p->priority = 4;
        CARDi_InitResourceLock();
        p->callback = NULL;
        p->callbackArg = NULL;
        OS_InitThreadQueue(&p->busyQueue);
        OS_CreateThread(&p->thread, CARDi_OldTypeTaskThread, NULL, p->stack + sizeof(p->stack), sizeof(p->stack),
                        p->priority);
        OS_WakeupThreadDirect(&p->thread);
        CARDi_InitCommand();
        CARDi_InitRom();
        if (OS_GetBootType() == OS_BOOTTYPE_ROM) {
            CARD_Enable(TRUE);
        }
        CARD_InitPulledOutCallback();
    }
}

static BOOL CARD_IsEnabled(void) {
    return sCardEnabled;
}

void CARD_CheckEnabled(void) {
    if (!CARD_IsEnabled()) {
        OS_Terminate();
    }
}

static void CARD_Enable(BOOL enable) {
    sCardEnabled = enable;
}

CARDResult CARD_GetResultCode(void) {
    return cardi_common.cmd->result;
}

const ROMHeader *CARD_GetRomHeader(void) {
    return (const ROMHeader *)HW_CARD_ROM_HEADER;
}

const ROMHeader *CARD_GetOwnRomHeader(void) {
    return (const ROMHeader *)HW_ROM_HEADER_BUF;
}

const void *CARD_GetOwnRomHeaderTWL(void) {
    return (const void *)HW_TWL_ROM_HEADER_BUF;
}

// Locks the card's ROM for a thread by the ID it got from the OS, polling for the card's lock each frame
void CARD_LockRom(u16 lockId) {
    CARDRomLockPoll poll;
    CARDi_LockResource(lockId, CARD_TARGET_ROM);
    OS_InitEvent(&poll.event);
    OS_CreateVAlarm(&poll.alarm);
    poll.condition = CARDi_TryLockCard;
    poll.arg = &lockId;
    CARDi_PollRomLock(&poll);
    OS_WaitEventEx(&poll.event, 1, OS_EVENT_MODE_AND, 1);
}

void CARD_UnlockRom(u16 lockId) {
    OS_UnlockCard(lockId);
    CARDi_UnlockResource(lockId, CARD_TARGET_ROM);
}

void CARD_LockBackup(u16 lockId) {
    CARDi_LockResource(lockId, CARD_TARGET_BACKUP);
}

void CARD_UnlockBackup(u16 lockId) {
    if (!CARD_TryWaitBackupAsync()) {
        CARD_WaitBackupAsync();
    }
    CARDi_UnlockResource(lockId, CARD_TARGET_BACKUP);
}

// Takes the lock of the card's resources for an ID, waiting if another ID holds it. The same ID may take it again for
// the same target. The loop's test is a 0/1 value, which keeps it from being rotated
static void CARDi_LockResource(u16 lockId, int target) {
    CARDiCommon *p = &cardi_common;
    u32 irq = OS_DisableInterrupts();
    if (p->lockOwner == lockId) {
        if (p->lockTarget != target) {
            OS_Terminate();
        }
    } else {
        while (p->lockOwner != OS_LOCK_ID_ERROR ? TRUE : FALSE) {
            OS_SleepThread(&p->lockQueue);
        }
        p->lockOwner = lockId;
        p->lockTarget = target;
    }
    p->lockRef++;
    OS_RestoreInterrupts(irq);
}

static void CARDi_UnlockResource(u16 lockId, int target) {
    CARDiCommon *p = &cardi_common;
    u32 irq = OS_DisableInterrupts();
    if (p->lockOwner != lockId || p->lockRef == 0) {
        OS_Terminate();
    } else {
        if (p->lockTarget != target) {
            OS_Terminate();
        }
        if (--p->lockRef == 0) {
            p->lockOwner = OS_LOCK_ID_ERROR;
            p->lockTarget = 0;
            OS_WakeupThread(&p->lockQueue);
        }
    }
    OS_RestoreInterrupts(irq);
}

// What the game may do with the card: everything on a ROM boot or a DS, and what the header of a DSi game allows
u32 CARDi_GetAccessLevel(void) {
    u32 level = 0;
    if (OS_GetBootType() == OS_BOOTTYPE_ROM) {
        level = CARD_ACCESS_LEVEL_FULL;
    } else if (!OS_IsRunOnTwl()) {
        level = CARD_ACCESS_LEVEL_FULL;
    } else {
        const CARDTwlRomHeader *header = CARD_GetOwnRomHeaderTWL();
        BOOL granted = FALSE;
        if (header->accessControl.gameCardNtrMode) {
            level |= CARD_ACCESS_LEVEL_ROM;
            granted = TRUE;
        } else if (header->accessControl.gameCardPowerOn) {
            granted = TRUE;
        }
        if (granted) {
            if (header->accessControl.backupRead) {
                level |= CARD_ACCESS_LEVEL_BACKUP_READ;
            }
            if (header->accessControl.backupWrite) {
                level |= CARD_ACCESS_LEVEL_BACKUP_WRITE;
            }
        }
    }
    return level;
}

BOOL CARDi_WaitAsync(void) {
    return CARDi_WaitForTask(&cardi_common, FALSE, NULL, NULL);
}

BOOL CARDi_TryWaitAsync(void) {
    return (cardi_common.flag & CARD_STAT_BUSY) == 0;
}

// Returns 0, which nothing reads: the original has the zero in r0 when it returns, where a void function puts the
// address of the work in r0
static int CARDi_InitResourceLock(void) {
    CARDiCommon *p = &cardi_common;
    p->lockOwner = OS_LOCK_ID_ERROR;
    p->lockRef = 0;
    p->lockTarget = 0;
    OS_InitThreadQueue(&p->lockQueue);
    return 0;
}

static void CARDi_InitCommand(void) {
    cardi_common.cmd = &cardi_arg;
    MI_CpuClearFast(&cardi_arg, sizeof(CARDiCommandArg));
    DC_FlushRange(&cardi_arg, sizeof(CARDiCommandArg));
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_FS, CARDi_OnFifoRecv);
}

// CARDi_NotifyEvent: calls each hook with the event. The walk keeps the link before the current hook and moves it on
// only if the hook is still there, so a hook may remove itself
void CARDi_NotifyEvent(u32 command, u32 value) {
    u32 irq = OS_DisableInterrupts();
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
static void CARDi_OldTypeTaskThread(void *arg) {
    CARDiCommon *p = &cardi_common;
    while (TRUE) {
        u32 irq = OS_DisableInterrupts();
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
    u32 irq = OS_DisableInterrupts();
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
    u32 irq = OS_DisableInterrupts();
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
