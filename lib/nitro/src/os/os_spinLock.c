// NitroSDK OS: the spin locks that the two processors share (os_spinLock.c), the locks of the cartridge and the card,
// and the pool of lock IDs. The first functions are Thumb, the pool's two are ARM.
#include "nitro/mi.h"
#include "nitro/os.h"

// The bits of EXMEMCNT that give the cartridge and the card to the ARM7, and the DSi's card slot status (the card's
// control register is in nitro/hw.h)
#define EXMEMCNT_CARTRIDGE_ARM7 0x0080
#define EXMEMCNT_CARD_ARM7 0x0800
#define reg_SCFG_MC (*(vu16 *)0x04004010)

static s32 func_0207a040(u16 lockID, OSLockWord *lockp, void (*ctrl)(void), BOOL disableFiq);
static s32 func_0207a078(u16 lockID, OSLockWord *lockp, void (*ctrl)(void), BOOL disableFiq);
static s32 func_0207a0cc(u16 lockID, OSLockWord *lockp, void (*ctrl)(void), BOOL disableFiq);
static void func_0207a158(void);
static void func_0207a168(void);
static void func_0207a1bc(void);
static void func_0207a1f4(void);

// Whether OS_InitLock has run
static BOOL sLockInitialized;

// NitroSDK's OSi_SyncWithOtherProc: a handshake through four bytes of shared memory that the other processor runs
// the opposite half of. With c 0 it writes a counter into p[0] until the other side echoes it into p[1], or says in
// p[2] that it has seen enough, and then tells it in p[3] that it knows. With c 1 it echoes p[0] into p[1] until the
// echoed counters add up to 0x300, says it is done in p[2] and waits for p[3]
void func_02079f5c(int c, vu8 *p) {
    int i;
    int sum;

    if (c == 0) {
        p[2] = 0;
        i = 0;
        do {
            p[0] = 0x80 | (i & 0xf);
            while (p[0] != p[1] && p[2] == 0) {
                WaitByLoop(0x400);
            }
            i++;
        } while (p[2] == 0);
        p[3] = 1;
    } else {
        p[1] = 0;
        sum = 0;
        while (sum < 0x300) {
            if (p[1] != p[0]) {
                p[1] = p[0];
                sum += p[1];
            } else {
                WaitByLoop(0x400);
            }
        }
        p[3] = 0;
        p[2] = 1;
        while (p[3] == 0) {
            WaitByLoop(0x400);
        }
    }
}

// NitroSDK's OS_InitLock
void func_02079fe4(void) {
    if (sLockInitialized) {
        return;
    }
    sLockInitialized = TRUE;
    *(u32 *)HW_INIT_LOCK_BUF = 0xffffffff;
    *(u32 *)(HW_INIT_LOCK_BUF + 4) = 0xffff0000;
    MI_CpuClear32((void *)(HW_INIT_LOCK_BUF + 0x10), 0x28);
    reg_MI_EXMEMCNT = (u16)(reg_MI_EXMEMCNT | EXMEMCNT_CARD_ARM7);
    reg_MI_EXMEMCNT = (u16)(reg_MI_EXMEMCNT | EXMEMCNT_CARTRIDGE_ARM7);
    func_02079f5c(0, (vu8 *)(HW_INIT_LOCK_BUF + 0x40));
    func_02079f5c(1, (vu8 *)(HW_INIT_LOCK_BUF + 0x40));
}

// NitroSDK's OSi_DoLockByWord: tries to lock until it does
static s32 func_0207a040(u16 lockID, OSLockWord *lockp, void (*ctrl)(void), BOOL disableFiq) {
    s32 lastLockFlag;

    while ((lastLockFlag = func_0207a0cc(lockID, lockp, ctrl, disableFiq)) > 0) {
        WaitByLoop(0x400);
    }
    return lastLockFlag;
}

// NitroSDK's OS_LockByWord
s32 func_0207a06c(u16 lockID, OSLockWord *lockp, void (*ctrl)(void)) {
    return func_0207a040(lockID, lockp, ctrl, FALSE);
}

// NitroSDK's OSi_DoUnlockByWord
static s32 func_0207a078(u16 lockID, OSLockWord *lockp, void (*ctrl)(void), BOOL disableFiq) {
    u32 irq;

    if (lockID != lockp->ownerID) {
        return OS_UNLOCK_ERROR;
    }
    if (disableFiq) {
        irq = CPU_IRQFIQDisable();
    } else {
        irq = CPU_IRQDisable();
    }
    lockp->ownerID = 0;
    if (ctrl != NULL) {
        ctrl();
    }
    lockp->lockFlag = 0;
    if (disableFiq) {
        CPU_SetIRQFIQMask(irq);
    } else {
        CPU_SetIRQMask(irq);
    }
    return OS_UNLOCK_SUCCESS;
}

// NitroSDK's OS_UnlockByWord
s32 func_0207a0c0(u16 lockID, OSLockWord *lockp, void (*ctrl)(void)) {
    return func_0207a078(lockID, lockp, ctrl, FALSE);
}

// NitroSDK's OSi_DoTryLockByWord: swaps the ID into the flag, and if the flag was clear it has the lock
static s32 func_0207a0cc(u16 lockID, OSLockWord *lockp, void (*ctrl)(void), BOOL disableFiq) {
    u32 irq;
    s32 lastLockFlag;

    if (disableFiq) {
        irq = CPU_IRQFIQDisable();
    } else {
        irq = CPU_IRQDisable();
    }
    lastLockFlag = memswap(lockID, &lockp->lockFlag);
    if (lastLockFlag == 0) {
        if (ctrl != NULL) {
            ctrl();
        }
        lockp->ownerID = lockID;
    }
    if (disableFiq) {
        CPU_SetIRQFIQMask(irq);
    } else {
        CPU_SetIRQMask(irq);
    }
    return lastLockFlag;
}

// NitroSDK's OS_TryLockByWord
s32 func_0207a114(u16 lockID, OSLockWord *lockp, void (*ctrl)(void)) {
    return func_0207a0cc(lockID, lockp, ctrl, FALSE);
}

// NitroSDK's OS_UnlockCartridge, which gives the cartridge back to the ARM7
s32 func_0207a120(u16 lockID) {
    return func_0207a078(lockID, (OSLockWord *)HW_CTRDG_LOCK_BUF, func_0207a168, TRUE);
}

// NitroSDK's OS_UnLockCartridge
s32 func_0207a138(u16 lockID) {
    return func_0207a120(lockID);
}

// NitroSDK's OS_TryLockCartridge, which takes the cartridge for the ARM9
s32 func_0207a140(u16 lockID) {
    return func_0207a0cc(lockID, (OSLockWord *)HW_CTRDG_LOCK_BUF, func_0207a158, TRUE);
}

// NitroSDK's OSi_AllocateCartridgeBus and OSi_FreeCartridgeBus
static void func_0207a158(void) {
    reg_MI_EXMEMCNT = (u16)(reg_MI_EXMEMCNT & ~EXMEMCNT_CARTRIDGE_ARM7);
}

static void func_0207a168(void) {
    reg_MI_EXMEMCNT = (u16)(reg_MI_EXMEMCNT | EXMEMCNT_CARTRIDGE_ARM7);
}

// NitroSDK's OS_LockCard
s32 func_0207a178(u16 lockID) {
    return func_0207a06c(lockID, (OSLockWord *)HW_CARD_LOCK_BUF, func_0207a1bc);
}

// NitroSDK's OS_UnlockCard
s32 func_0207a18c(u16 lockID) {
    return func_0207a0c0(lockID, (OSLockWord *)HW_CARD_LOCK_BUF, func_0207a1f4);
}

// NitroSDK's OS_UnLockCard
s32 func_0207a1a0(u16 lockID) {
    return func_0207a18c(lockID);
}

// NitroSDK's OS_TryLockCard
s32 func_0207a1a8(u16 lockID) {
    return func_0207a114(lockID, (OSLockWord *)HW_CARD_LOCK_BUF, func_0207a1bc);
}

// NitroSDK's OSi_AllocateCardBus, which takes the card for the ARM9 (and on the DSi, in a card slot status, sets a bit
// of the card's control), and OSi_FreeCardBus
static void func_0207a1bc(void) {
    if (((reg_SCFG_MC & 0xc) >> 2) == 2) {
        reg_MI_MCCNT1 |= 0x20000000;
    }
    reg_MI_EXMEMCNT = (u16)(reg_MI_EXMEMCNT & ~EXMEMCNT_CARD_ARM7);
}

static void func_0207a1f4(void) {
    reg_MI_EXMEMCNT = (u16)(reg_MI_EXMEMCNT | EXMEMCNT_CARD_ARM7);
}

// NitroSDK's OS_ReadOwnerOfLockWord
u16 cart_mutex_get_status(const OSLockWord *lockp) {
    return lockp->ownerID;
}

// The ID pool: two words of the shared memory whose set bits are the free IDs from 0x40 to 0x7f, the first bit of the
// first word being 0x40. NitroSDK's OS_GetLockID and OS_ReleaseLockID, under swan's names
#pragma thumb off
// clang-format off
// Hand-written in assembly in the original: the -3 for no free ID comes from a literal pool, not an MVN, and sits
// between the compare and the conditional return
asm s32 cart_key_create(void) {
    ldr r3, =HW_INIT_LOCK_BUF
    ldr r1, [r3, #0]
    clz r2, r1
    cmp r2, #32
    movne r0, #0x40
    bne @found
    add r3, r3, #4
    ldr r1, [r3, #0]
    clz r2, r1
    cmp r2, #32
    ldr r0, =OS_LOCK_ID_ERROR
    bxeq lr
    mov r0, #0x60
@found:
    add r0, r0, r2
    mov r1, #0x80000000
    mov r1, r1, lsr r2
    ldr r2, [r3, #0]
    bic r2, r2, r1
    str r2, [r3, #0]
    bx lr
}

// Hand-written in assembly in the original as well: it works on the ID in place, with conditions
asm void cart_key_release(u16 lockId) {
    ldr r3, =HW_INIT_LOCK_BUF
    cmp r0, #0x60
    addpl r3, r3, #4
    subpl r0, r0, #0x60
    submi r0, r0, #0x40
    mov r1, #0x80000000
    mov r1, r1, lsr r0
    ldr r2, [r3, #0]
    orr r2, r2, r1
    str r2, [r3, #0]
    bx lr
}
// clang-format on
#pragma thumb on
