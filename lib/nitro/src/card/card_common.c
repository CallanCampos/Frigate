// NitroSDK's CARD library, the part that ROM and backup commands share: initialization, the lock of the card's
// resources, the thread that runs the commands' tasks, and the cache handling around the ARM7's DMA. The file name is
// a guess, descriptive from the SDK's structure (card_common.c); the library has no file name string.
#include "nitro/card.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "twl/card.h"

// The size of the ROM header that CARD copies (nitro/hw.h has the addresses)
#define HW_CARD_ROM_HEADER_SIZE 0x160

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

static void func_0206ed94(void *arg);
static BOOL func_0206edcc(void *arg);
static BOOL func_0206eea4(void);
static void func_0206eec0(BOOL enable);
static void func_0206ef74(u16 lockId, int target);
static void func_0206efc8(u16 lockId, int target);
static int func_0206f0a8(void);
static void func_0206f0c0(void);
static void cart_read_thread_proc(void *arg);

// The first node of the list of hooks has only its link in use. The statics lie in memory in reverse of the order here
static CARDHookContext sCardHookRoot;
CARDiCommon cardi_common;
static CARDiCommandArg cardi_arg;
static BOOL sCardEnabled;

// The interface for the DS's DMA channels
static const CARDDmaInterface sCardDmaInterface = { func_0207917c, dma_end };

// Calls the condition each frame, at the first line of the vertical blank, until it holds
static void func_0206ed94(void *arg) {
    CARDRomLockPoll *poll = arg;
    if (poll->condition(poll->arg)) {
        func_0207c864(&poll->event, 1);
    } else {
        OS_SetVAlarm(&poll->alarm, 192, 263, func_0206ed94, poll);
    }
}

// Whether the card's lock was free, and is the caller's now
static BOOL func_0206edcc(void *arg) {
    return func_0207a1a8(*(u16 *)arg) == 0;
}

// CARD_Init: sets up the library's work, starts the thread that runs the commands' tasks and, on a ROM boot, enables
// the card
void cart_read_thread_init(void) {
    CARDiCommon *p = &cardi_common;
    if (p->flag == 0) {
        p->flag = CARD_STAT_INIT;
        if (sys_get_boot_indicator() == OS_BOOTTYPE_ROM) {
            sys_memcpy((void *)HW_ROM_HEADER_BUF, (void *)HW_CARD_ROM_HEADER, HW_CARD_ROM_HEADER_SIZE);
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
        func_0206f0a8();
        p->callback = NULL;
        p->callbackArg = NULL;
        OS_InitThreadQueue(&p->busyQueue);
        scheduler_init_thread(&p->thread, cart_read_thread_proc, NULL, p->stack + sizeof(p->stack), sizeof(p->stack),
                              p->priority);
        scheduler_start_thread(&p->thread);
        func_0206f0c0();
        func_0206fe54();
        if (sys_get_boot_indicator() == OS_BOOTTYPE_ROM) {
            func_0206eec0(TRUE);
        }
        func_0206fed8();
    }
}

static BOOL func_0206eea4(void) {
    return sCardEnabled;
}

void func_0206eeb0(void) {
    if (!func_0206eea4()) {
        sys_exit();
    }
}

static void func_0206eec0(BOOL enable) {
    sCardEnabled = enable;
}

CARDResult func_0206eecc(void) {
    return cardi_common.cmd->result;
}

const ROMHeader *func_0206eed8(void) {
    return (const ROMHeader *)HW_CARD_ROM_HEADER;
}

const ROMHeader *rom_get_header(void) {
    return (const ROMHeader *)HW_ROM_HEADER_BUF;
}

const void *func_0206eee8(void) {
    return (const void *)HW_TWL_ROM_HEADER_BUF;
}

// Locks the card's ROM for a thread by the ID it got from the OS, polling for the card's lock each frame
void func_0206eef0(u16 lockId) {
    CARDRomLockPoll poll;
    func_0206ef74(lockId, CARD_TARGET_ROM);
    func_0207c7f4(&poll.event);
    func_0207befc(&poll.alarm);
    poll.condition = func_0206edcc;
    poll.arg = &lockId;
    func_0206ed94(&poll);
    func_0207c800(&poll.event, 1, OS_EVENT_MODE_AND, 1);
}

void func_0206ef38(u16 lockId) {
    func_0207a18c(lockId);
    func_0206efc8(lockId, CARD_TARGET_ROM);
}

void func_0206ef4c(u16 lockId) {
    func_0206ef74(lockId, CARD_TARGET_BACKUP);
}

void func_0206ef58(u16 lockId) {
    if (!func_0206f890()) {
        func_0206f888();
    }
    func_0206efc8(lockId, CARD_TARGET_BACKUP);
}

// Takes the lock of the card's resources for an ID, waiting if another ID holds it. The same ID may take it again for
// the same target. The loop's test is a 0/1 value, which keeps it from being rotated
static void func_0206ef74(u16 lockId, int target) {
    CARDiCommon *p = &cardi_common;
    u32 irq = CPU_IRQDisable();
    if (p->lockOwner == lockId) {
        if (p->lockTarget != target) {
            sys_exit();
        }
    } else {
        while (p->lockOwner != OS_LOCK_ID_ERROR ? TRUE : FALSE) {
            scheduler_yield(&p->lockQueue);
        }
        p->lockOwner = lockId;
        p->lockTarget = target;
    }
    p->lockRef++;
    CPU_SetIRQMask(irq);
}

static void func_0206efc8(u16 lockId, int target) {
    CARDiCommon *p = &cardi_common;
    u32 irq = CPU_IRQDisable();
    if (p->lockOwner != lockId || p->lockRef == 0) {
        sys_exit();
    } else {
        if (p->lockTarget != target) {
            sys_exit();
        }
        if (--p->lockRef == 0) {
            p->lockOwner = OS_LOCK_ID_ERROR;
            p->lockTarget = 0;
            OS_WakeupThread(&p->lockQueue);
        }
    }
    CPU_SetIRQMask(irq);
}

// What the game may do with the card: everything on a ROM boot or a DS, and what the header of a DSi game allows
u32 func_0206f018(void) {
    u32 level = 0;
    if (sys_get_boot_indicator() == OS_BOOTTYPE_ROM) {
        level = CARD_ACCESS_LEVEL_FULL;
    } else if (!hw_isDSi()) {
        level = CARD_ACCESS_LEVEL_FULL;
    } else {
        const CARDTwlRomHeader *header = func_0206eee8();
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

BOOL func_0206f07c(void) {
    return func_0206f1c0(&cardi_common, FALSE, NULL, NULL);
}

BOOL func_0206f090(void) {
    return (cardi_common.flag & CARD_STAT_BUSY) == 0;
}

// Returns 0, which nothing reads: the original has the zero in r0 when it returns, where a void function puts the
// address of the work in r0
static int func_0206f0a8(void) {
    CARDiCommon *p = &cardi_common;
    p->lockOwner = OS_LOCK_ID_ERROR;
    p->lockRef = 0;
    p->lockTarget = 0;
    OS_InitThreadQueue(&p->lockQueue);
    return 0;
}

static void func_0206f0c0(void) {
    cardi_common.cmd = &cardi_arg;
    MI_CpuClearFast(&cardi_arg, sizeof(CARDiCommandArg));
    cp15_flushDC(&cardi_arg, sizeof(CARDiCommandArg));
    func_0207c9fc(PXI_FIFO_TAG_FS, func_0206f2e4);
}

// CARDi_NotifyEvent: calls each hook with the event. The walk keeps the link before the current hook and moves it on
// only if the hook is still there, so a hook may remove itself
void func_0206f0f0(u32 command, u32 value) {
    u32 irq = CPU_IRQDisable();
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
    CPU_SetIRQMask(irq);
}

// Runs a task: on the thread if async, otherwise here and now. An async task is done when its callback is called
BOOL func_0206f134(CARDiTask task, BOOL async) {
    CARDiCommon *p = &cardi_common;
    if (async) {
        func_0207a998(&p->thread, p->priority);
        p->task = task;
        p->flag |= CARD_STAT_TASK;
        scheduler_start_thread(&p->thread);
    } else {
        task(p);
        func_0206f224(p);
    }
    if (async) {
        return TRUE;
    }
    return p->cmd->result == CARD_RESULT_SUCCESS;
}

// CARDi_OldTypeTaskThread: sleeps until a task is set, then runs it
static void cart_read_thread_proc(void *arg) {
    CARDiCommon *p = &cardi_common;
    while (TRUE) {
        u32 irq = CPU_IRQDisable();
        for (;;) {
            if ((p->flag & CARD_STAT_TASK) != 0) {
                break;
            }
            scheduler_yield(NULL);
        }
        CPU_SetIRQMask(irq);
        func_0206f134(p->task, FALSE);
    }
}

// Waits until the last command is done. A command that starts takes over the thread, with the callback to call when
// it ends. Returns whether the command block holds a success
BOOL func_0206f1c0(CARDiCommon *p, BOOL start, MIDmaCallback callback, void *arg) {
    u32 irq = CPU_IRQDisable();
    while ((p->flag & CARD_STAT_BUSY) != 0) {
        scheduler_yield(&p->busyQueue);
    }
    if (start) {
        p->flag |= CARD_STAT_BUSY;
        p->callback = callback;
        p->callbackArg = arg;
    }
    CPU_SetIRQMask(irq);
    return p->cmd->result == CARD_RESULT_SUCCESS;
}

// Ends a command: lets the next one start, then calls the callback of the one that ended
void func_0206f224(CARDiCommon *p) {
    MIDmaCallback callback = p->callback;
    void *arg = p->callbackArg;
    u32 irq = CPU_IRQDisable();
    p->flag &= ~(CARD_STAT_BUSY | CARD_STAT_TASK | CARD_STAT_CANCEL);
    OS_WakeupThread(&p->busyQueue);
    CPU_SetIRQMask(irq);
    if (callback != NULL) {
        callback(arg);
    }
}

// The interface of a DMA channel for ROM reads: bit 4 of the number picks the DSi's new DMA, which only a DSi has.
// NULL for a channel that doesn't exist
const CARDDmaInterface *func_0206f25c(u32 dma) {
    const CARDDmaInterface *interface = NULL;
    BOOL twl = (dma & 0x10) != 0;
    dma &= ~0x10;
    if (dma <= 3) {
        if (!twl) {
            interface = &sCardDmaInterface;
        } else if (!hw_isDSi()) {
            sys_exit();
        } else {
            interface = &CARDi_NdmaInterface;
        }
    }
    return interface;
}

// Discards the instruction cache of a range, or all of it from the threshold on
void func_0206f298(void *addr, u32 len, u32 threshold) {
    if (len >= threshold) {
        cp15_invalidateICAll();
    } else {
        cp15_invalidateIC(addr, len);
    }
}

// Discards the data cache of a range, or all of it from the threshold on. The lines at the ends of a range that they
// share with other data are written back first
void func_0206f2ac(void *addr, u32 len, u32 threshold) {
    if (len >= threshold) {
        cp15_invalidateDCAll();
    } else {
        u32 offset = (u32)addr & 0x1f;
        if (offset != 0) {
            addr = (u8 *)addr - offset;
            cp15_cleanDC(addr, 0x20);
            cp15_cleanDC((u8 *)addr + len, 0x20);
            len += 0x20;
        }
        cp15_invalidateDC(addr, len);
        cp15_drainWriteBuffer();
    }
}
