// The cartridge library's common part (libctrdg, TwlSDK's ctrdg/common/src/ctrdg.c by its place in the library: the
// file name is a guess): taking the bus of slot 2 for the ARM9, saying whether the cartridge is still the one that was
// there at the start, and the word to the ARM7. The names are descriptive
#include "nitro/ctrdg.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "nitro/svc.h"

// The wait states of slot 2 are in the bits of the memory control register
#define REG_MI_EXMEMCNT (*(vu16 *)0x04000204)
#define MI_EXMEMCNT_ROM_1ST_MASK 0x000c
#define MI_EXMEMCNT_ROM_1ST_SHIFT 2
#define MI_EXMEMCNT_ROM_2ND_MASK 0x0010
#define MI_EXMEMCNT_ROM_2ND_SHIFT 4

// A cartridge's ROM is at the start of slot 2's area. A GBA cartridge's header has a fixed byte of 0x96, and a game
// code
#define CTRDG_ROM_ADDR 0x08000000
#define CTRDG_AGB_FIXED_VALUE 0x96

// The header of a GBA cartridge's ROM, of which the library reads a few fields
typedef struct {
    u8 unk_00[0xac];
    u32 gameCode;
    u8 unk_b0[2];
    u8 fixedValue; // CTRDG_AGB_FIXED_VALUE
    u8 unk_b3[0xb];
    u16 unk_be; // What the start noted in CTRDGModuleInfo.romCheck
} CTRDGAgbHeader;

#define CTRDG_AGB_HEADER ((const CTRDGAgbHeader *)CTRDG_ROM_ADDR)

// How the library took the bus: whether this processor had it already, and the interrupts as they were
typedef struct {
    u32 hadBus;
    u32 irq;
} CTRDGLockInfo;

// The wait states of the ROM, as the bits of the register give them: the first access takes 10, 8, 6 or 18 cycles and
// the second 6 or 4
typedef enum {
    CTRDG_ROMCYCLE1_10,
    CTRDG_ROMCYCLE1_8,
    CTRDG_ROMCYCLE1_6,
    CTRDG_ROMCYCLE1_18,
} CTRDGRomCycle1st;

typedef enum {
    CTRDG_ROMCYCLE2_6,
    CTRDG_ROMCYCLE2_4,
} CTRDGRomCycle2nd;

// The wait states of the ROM as they were before the library set the slowest for its reads
typedef struct {
    CTRDGRomCycle1st first;
    CTRDGRomCycle2nd second;
} CTRDGRomCycle;

// The library's state, of which this file uses the lock ID: CTRDGi_InitCommon clears the last four bytes as a word, and
// then sets the ID in their second half
static struct {
    u32 unk_00;
    u16 unk_04;
    u16 lockID;
} ctrdgWork;

static BOOL CTRDGi_IsExistedAtInit(void);
static void CTRDGi_ChangeLatestAccessCycle(CTRDGRomCycle *cycle);
static void CTRDGi_RestoreAccessCycle(const CTRDGRomCycle *cycle);
static void CTRDGi_LockByProcessor(u16 lockID, CTRDGLockInfo *info);
static void CTRDGi_UnlockByProcessor(u16 lockID, CTRDGLockInfo *info);

// Whether a cartridge was there when the console started
static BOOL CTRDGi_IsExistedAtInit(void) {
    return *(vu16 *)HW_CTRDG_MODULE_INFO_BUF != 0xffff;
}

void CTRDGi_InitCommon(void) {
    u32 zero = 0;

    SVC_CpuSet(&zero, &ctrdgWork.unk_04, SVC_CPUSET_FILL | SVC_CPUSET_32BIT | 1);
    ctrdgWork.lockID = OS_GetLockID();
}

// Reads the ROM's identifying values and compares them with what the start noted
BOOL CTRDG_IsExisting(void) {
    BOOL result = TRUE;
    CTRDGLockInfo lockInfo;
    CTRDGRomCycle cycle;
    CTRDGModuleInfo *info = CTRDG_MODULE_INFO;
    const CTRDGAgbHeader *header = CTRDG_AGB_HEADER;

    if (!CTRDGi_IsExistedAtInit()) {
        return FALSE;
    }
    if (info->pulledOut == 1) {
        return FALSE;
    }
    CTRDGi_LockByProcessor(ctrdgWork.lockID, &lockInfo);
    CTRDGi_ChangeLatestAccessCycle(&cycle);
    if ((header->fixedValue == CTRDG_AGB_FIXED_VALUE && info->romCheck != header->unk_be) ||
        (header->fixedValue != CTRDG_AGB_FIXED_VALUE && info->romCheck != *(u16 *)(CTRDG_ROM_ADDR + 0x1fffe)) ||
        (info->gameCode != header->gameCode && info->isAgbCartridge)) {
        info->pulledOut = 1;
        result = FALSE;
    }
    CTRDGi_RestoreAccessCycle(&cycle);
    CTRDGi_UnlockByProcessor(ctrdgWork.lockID, &lockInfo);
    return result;
}

// The wait states of the ROM, in the register: the first access and the second
static inline CTRDGRomCycle1st CTRDGi_GetRomCycle1st(void) {
    return (REG_MI_EXMEMCNT & MI_EXMEMCNT_ROM_1ST_MASK) >> MI_EXMEMCNT_ROM_1ST_SHIFT;
}

static inline CTRDGRomCycle2nd CTRDGi_GetRomCycle2nd(void) {
    return (REG_MI_EXMEMCNT & MI_EXMEMCNT_ROM_2ND_MASK) >> MI_EXMEMCNT_ROM_2ND_SHIFT;
}

static inline void CTRDGi_SetRomCycle1st(CTRDGRomCycle1st cycle) {
    REG_MI_EXMEMCNT = (REG_MI_EXMEMCNT & ~MI_EXMEMCNT_ROM_1ST_MASK) | (cycle << MI_EXMEMCNT_ROM_1ST_SHIFT);
}

static inline void CTRDGi_SetRomCycle2nd(CTRDGRomCycle2nd cycle) {
    REG_MI_EXMEMCNT = (REG_MI_EXMEMCNT & ~MI_EXMEMCNT_ROM_2ND_MASK) | (cycle << MI_EXMEMCNT_ROM_2ND_SHIFT);
}

// Saves the wait states of the ROM and sets the slowest ones
static void CTRDGi_ChangeLatestAccessCycle(CTRDGRomCycle *cycle) {
    cycle->first = CTRDGi_GetRomCycle1st();
    cycle->second = CTRDGi_GetRomCycle2nd();
    CTRDGi_SetRomCycle1st(CTRDG_ROMCYCLE1_18);
    CTRDGi_SetRomCycle2nd(CTRDG_ROMCYCLE2_6);
}

// Puts the wait states back
static void CTRDGi_RestoreAccessCycle(const CTRDGRomCycle *cycle) {
    CTRDGi_SetRomCycle1st(cycle->first);
    CTRDGi_SetRomCycle2nd(cycle->second);
}

// Takes the cartridge bus with interrupts off, waiting for it if the ARM7 has it. The ID with the main processor's flag
// means that it was this processor's already, and nothing is locked
static void CTRDGi_LockByProcessor(u16 lockID, CTRDGLockInfo *info) {
    while (TRUE) {
        info->irq = OS_DisableInterrupts();
        info->hadBus = OS_ReadOwnerOfLockWord((const OSLockWord *)HW_CTRDG_LOCK_BUF) & 0x40;
        if (info->hadBus != 0) {
            return;
        }
        if (OS_TryLockCartridge(lockID) == 0) {
            return;
        }
        OS_RestoreInterrupts(info->irq);
        SVC_WaitByLoop(1);
    }
}

// Gives the bus back unless it was held before, and the interrupts as they were
static void CTRDGi_UnlockByProcessor(u16 lockID, CTRDGLockInfo *info) {
    if (info->hadBus == 0) {
        OS_UnLockCartridge(lockID);
    }
    OS_RestoreInterrupts(info->irq);
}

void CTRDGi_SendtoPxi(u32 data) {
    while (PXI_SendWordByFifo(PXI_FIFO_TAG_CTRDG, data, FALSE) != 0) {
        SVC_WaitByLoop(1);
    }
}
