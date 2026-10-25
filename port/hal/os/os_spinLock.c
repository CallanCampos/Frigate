// NitroSDK's spin locks (os_spinLock.c) on the host: the decomp's lib/nitro/src/os/os_spinLock.c, with the lock-ID
// pool's two assembly functions in C and without the handshake with the ARM7. The lock words stay in the HAL's copy of
// the shared memory (HW_INIT_LOCK_BUF and the card's and cartridge's words), where the host's ARM7 side would look.
// On one host thread a lock that another processor holds can't be released while the ARM9 spins, but nothing on the
// host's ARM7 side takes these locks, so OS_LockByWord never spins.
#include "hal/os/os_internal.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/svc.h"

// The bits of EXMEMCNT that give the cartridge and the card to the ARM7, and the DSi's card slot status (the card's
// control register is in nitro/hw.h)
#define EXMEMCNT_CARTRIDGE_ARM7 0x0080
#define EXMEMCNT_CARD_ARM7 0x0800
#define reg_SCFG_MC (*(vu16 *)PORT_HW_ADDR(0x04004010))

static s32 OSi_DoLockByWord(u16 lockID, OSLockWord *lockp, void (*ctrl)(void), BOOL disableFiq);
static s32 OSi_DoUnlockByWord(u16 lockID, OSLockWord *lockp, void (*ctrl)(void), BOOL disableFiq);
static s32 OSi_DoTryLockByWord(u16 lockID, OSLockWord *lockp, void (*ctrl)(void), BOOL disableFiq);
static void OSi_AllocateCartridgeBus(void);
static void OSi_FreeCartridgeBus(void);
static void OSi_AllocateCardBus(void);
static void OSi_FreeCardBus(void);

// Whether OS_InitLock has run
static BOOL isInitialized;

void port_osi_reset_locks(void) {
    isInitialized = FALSE;
}

// NitroSDK's OSi_SyncWithOtherProc: a handshake through four bytes of shared memory with the other processor, which
// runs the opposite half. The host's ARM7 side is ready from the start and needs no handshake, so this returns at once
// (with the bytes as the handshake leaves them on the ARM9's side)
void OSi_SyncWithOtherProc(int c, vu8 *p) {
    if (c == 0) {
        p[3] = 1;
    } else {
        p[2] = 1;
    }
}

// NitroSDK's OS_InitLock: sets up the lock words and gives the card and the cartridge to the ARM7, then synchronizes
// with it
void OS_InitLock(void) {
    if (isInitialized) {
        return;
    }
    isInitialized = TRUE;
    *(u32 *)HW_INIT_LOCK_BUF = 0xffffffff;
    *(u32 *)(HW_INIT_LOCK_BUF + 4) = 0xffff0000;
    MI_CpuClear32((void *)(HW_INIT_LOCK_BUF + 0x10), 0x28);
    reg_MI_EXMEMCNT = (u16)(reg_MI_EXMEMCNT | EXMEMCNT_CARD_ARM7);
    reg_MI_EXMEMCNT = (u16)(reg_MI_EXMEMCNT | EXMEMCNT_CARTRIDGE_ARM7);
    OSi_SyncWithOtherProc(0, (vu8 *)(HW_INIT_LOCK_BUF + 0x40));
    OSi_SyncWithOtherProc(1, (vu8 *)(HW_INIT_LOCK_BUF + 0x40));
}

// NitroSDK's OSi_DoLockByWord: tries to lock until it does
static s32 OSi_DoLockByWord(u16 lockID, OSLockWord *lockp, void (*ctrl)(void), BOOL disableFiq) {
    s32 lastLockFlag;

    while ((lastLockFlag = OSi_DoTryLockByWord(lockID, lockp, ctrl, disableFiq)) > 0) {
        SVC_WaitByLoop(0x400);
    }
    return lastLockFlag;
}

s32 OS_LockByWord(u16 lockID, OSLockWord *lockp, void (*ctrl)(void)) {
    return OSi_DoLockByWord(lockID, lockp, ctrl, FALSE);
}

static s32 OSi_DoUnlockByWord(u16 lockID, OSLockWord *lockp, void (*ctrl)(void), BOOL disableFiq) {
    u32 irq;

    if (lockID != lockp->ownerID) {
        return OS_UNLOCK_ERROR;
    }
    if (disableFiq) {
        irq = OS_DisableInterrupts_IrqAndFiq();
    } else {
        irq = OS_DisableInterrupts();
    }
    lockp->ownerID = 0;
    if (ctrl != NULL) {
        ctrl();
    }
    lockp->lockFlag = 0;
    if (disableFiq) {
        OS_RestoreInterrupts_IrqAndFiq(irq);
    } else {
        OS_RestoreInterrupts(irq);
    }
    return OS_UNLOCK_SUCCESS;
}

s32 OS_UnlockByWord(u16 lockID, OSLockWord *lockp, void (*ctrl)(void)) {
    return OSi_DoUnlockByWord(lockID, lockp, ctrl, FALSE);
}

// NitroSDK's OSi_DoTryLockByWord: swaps the ID into the flag, and if the flag was clear it has the lock
static s32 OSi_DoTryLockByWord(u16 lockID, OSLockWord *lockp, void (*ctrl)(void), BOOL disableFiq) {
    u32 irq;
    s32 lastLockFlag;

    if (disableFiq) {
        irq = OS_DisableInterrupts_IrqAndFiq();
    } else {
        irq = OS_DisableInterrupts();
    }
    lastLockFlag = MI_SwapWord(lockID, &lockp->lockFlag);
    if (lastLockFlag == 0) {
        if (ctrl != NULL) {
            ctrl();
        }
        lockp->ownerID = lockID;
    }
    if (disableFiq) {
        OS_RestoreInterrupts_IrqAndFiq(irq);
    } else {
        OS_RestoreInterrupts(irq);
    }
    return lastLockFlag;
}

s32 OS_TryLockByWord(u16 lockID, OSLockWord *lockp, void (*ctrl)(void)) {
    return OSi_DoTryLockByWord(lockID, lockp, ctrl, FALSE);
}

// NitroSDK's OS_UnlockCartridge, which gives the cartridge back to the ARM7
s32 OS_UnlockCartridge(u16 lockID) {
    return OSi_DoUnlockByWord(lockID, (OSLockWord *)HW_CTRDG_LOCK_BUF, OSi_FreeCartridgeBus, TRUE);
}

// NitroSDK's OS_UnLockCartridge, the old spelling of OS_UnlockCartridge (a jump to it in the SDK's assembly)
s32 OS_UnLockCartridge(u16 lockID) {
    return OS_UnlockCartridge(lockID);
}

// NitroSDK's OS_TryLockCartridge, which takes the cartridge for the ARM9
s32 OS_TryLockCartridge(u16 lockID) {
    return OSi_DoTryLockByWord(lockID, (OSLockWord *)HW_CTRDG_LOCK_BUF, OSi_AllocateCartridgeBus, TRUE);
}

// NitroSDK's OSi_AllocateCartridgeBus and OSi_FreeCartridgeBus
static void OSi_AllocateCartridgeBus(void) {
    reg_MI_EXMEMCNT = (u16)(reg_MI_EXMEMCNT & ~EXMEMCNT_CARTRIDGE_ARM7);
}

static void OSi_FreeCartridgeBus(void) {
    reg_MI_EXMEMCNT = (u16)(reg_MI_EXMEMCNT | EXMEMCNT_CARTRIDGE_ARM7);
}

s32 OS_LockCard(u16 lockID) {
    return OS_LockByWord(lockID, (OSLockWord *)HW_CARD_LOCK_BUF, OSi_AllocateCardBus);
}

s32 OS_UnlockCard(u16 lockID) {
    return OS_UnlockByWord(lockID, (OSLockWord *)HW_CARD_LOCK_BUF, OSi_FreeCardBus);
}

// NitroSDK's OS_UnLockCard, the old spelling of OS_UnlockCard (a jump to it in the SDK's assembly)
s32 OS_UnLockCard(u16 lockID) {
    return OS_UnlockCard(lockID);
}

s32 OS_TryLockCard(u16 lockID) {
    return OS_TryLockByWord(lockID, (OSLockWord *)HW_CARD_LOCK_BUF, OSi_AllocateCardBus);
}

// NitroSDK's OSi_AllocateCardBus, which takes the card for the ARM9 (and on the DSi, in a card slot status, sets a bit
// of the card's control), and OSi_FreeCardBus
static void OSi_AllocateCardBus(void) {
    if (((reg_SCFG_MC & 0xc) >> 2) == 2) {
        reg_MI_MCCNT1 |= REG_MI_MCCNT1_RESB_MASK;
    }
    reg_MI_EXMEMCNT = (u16)(reg_MI_EXMEMCNT & ~EXMEMCNT_CARD_ARM7);
}

static void OSi_FreeCardBus(void) {
    reg_MI_EXMEMCNT = (u16)(reg_MI_EXMEMCNT | EXMEMCNT_CARD_ARM7);
}

u16 OS_ReadOwnerOfLockWord(OSLockWord *lockp) {
    return lockp->ownerID;
}

// The ID pool: two words of the shared memory whose set bits are the free IDs from 0x40 to 0x7f, the first bit of the
// first word being 0x40. NitroSDK's OS_GetLockID and OS_ReleaseLockID, assembly in the SDK: the first free ID is the
// first set bit from the top (CLZ), which the ID takes by clearing it
s32 OS_GetLockID(void) {
    u32 *pool = (u32 *)HW_INIT_LOCK_BUF;
    s32 base = 0x40;
    u32 bit;

    if (*pool == 0) {
        pool++;
        base = 0x60;
        if (*pool == 0) {
            return OS_LOCK_ID_ERROR;
        }
    }
    bit = (u32)__builtin_clz(*pool);
    *pool &= ~(0x80000000u >> bit);
    return base + (s32)bit;
}

void OS_ReleaseLockID(u16 lockId) {
    u32 *pool = (u32 *)HW_INIT_LOCK_BUF;

    if (lockId >= 0x60) {
        pool++;
        lockId -= 0x60;
    } else {
        lockId -= 0x40;
    }
    *pool |= 0x80000000u >> lockId;
}
