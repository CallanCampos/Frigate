// The cartridge library's common part (libctrdg, TwlSDK's ctrdg/common/src/ctrdg.c by its place in the library;
// ntrtwl's reconstruction of NitroSDK has the same file): taking the bus of slot 2 for the ARM9, saying whether the
// cartridge is still the one that was there at the start, and the word to the ARM7. The names are NitroSDK's, as that
// reconstruction has them, except CTRDGi_IsExistedAtInit and ctrdgWork, which are ours
#include "nitro/ctrdg.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "nitro/svc.h"

// The wait states of slot 2 are in the bits of the memory control register (reg_MI_EXMEMCNT)
#define REG_MI_EXMEMCNT_ROM1st_MASK 0x000c
#define REG_MI_EXMEMCNT_ROM1st_SHIFT 2
#define REG_MI_EXMEMCNT_ROM2nd_MASK 0x0010
#define REG_MI_EXMEMCNT_ROM2nd_SHIFT 4

// A cartridge's ROM is at the start of slot 2's area. A GBA cartridge's header has a fixed byte of 0x96, and a game
// code
#define CTRDG_IS_ROM_CODE 0x96

// The header of a GBA cartridge's ROM, of which the library reads a few fields
typedef struct {
    u8 unk_00[0xac];
    u32 gameCode;
    u8 makerCode[2];
    u8 isRomCode; // CTRDG_IS_ROM_CODE
    u8 unk_b3[0xb];
    u16 moduleID; // What the start noted in CTRDGModuleInfo.moduleID
} CTRDGHeader;

#define CTRDG_AGB_HEADER ((const CTRDGHeader *)HW_CTRDG_ROM)

// How the library took the bus: whether this processor had it already, and the interrupts as they were
typedef struct {
    u32 locked;
    u32 irq;
} CTRDGLockByProc;

// The wait states of the ROM, as the bits of the register give them: the first access takes 10, 8, 6 or 18 cycles and
// the second 6 or 4
typedef enum {
    CTRDG_ROMCYCLE1_10,
    CTRDG_ROMCYCLE1_8,
    CTRDG_ROMCYCLE1_6,
    CTRDG_ROMCYCLE1_18,
} MICartridgeRomCycle1st;

typedef enum {
    CTRDG_ROMCYCLE2_6,
    CTRDG_ROMCYCLE2_4,
} MICartridgeRomCycle2nd;

// The wait states of the ROM as they were before the library set the slowest for its reads
typedef struct {
    MICartridgeRomCycle1st c1;
    MICartridgeRomCycle2nd c2;
} CTRDGRomCycle;

// The library's state, of which this file uses the lock ID: CTRDGi_InitCommon clears the last four bytes as a word, and
// then sets the ID in their second half. The struct is ours, for what the SDK keeps as CTRDGi_EnableFlag and the
// CTRDGWork CTRDGi_Work
static struct {
    u32 unk_00;
    u16 subpInitialized;
    u16 lockID;
} ctrdgWork;

static BOOL CTRDGi_IsExistedAtInit(void);
static void CTRDGi_ChangeLatestAccessCycle(CTRDGRomCycle *cycle);
static void CTRDGi_RestoreAccessCycle(const CTRDGRomCycle *cycle);
static void CTRDGi_LockByProcessor(u16 lockID, CTRDGLockByProc *info);
static void CTRDGi_UnlockByProcessor(u16 lockID, CTRDGLockByProc *info);

// Whether a cartridge was there when the console started
static BOOL CTRDGi_IsExistedAtInit(void) {
    return *(vu16 *)HW_CTRDG_MODULE_INFO_BUF != 0xffff;
}

void CTRDGi_InitCommon(void) {
    u32 zero = 0;

    SVC_CpuSet(&zero, &ctrdgWork.subpInitialized, MI_DMA_SRC_FIX | MI_DMA_32BIT_BUS | 1);
    ctrdgWork.lockID = OS_GetLockID();
}

// Reads the ROM's identifying values and compares them with what the start noted
BOOL CTRDG_IsExisting(void) {
    BOOL result = TRUE;
    CTRDGLockByProc lockInfo;
    CTRDGRomCycle cycle;
    CTRDGModuleInfo *info = CTRDG_MODULE_INFO;
    const CTRDGHeader *header = CTRDG_AGB_HEADER;

    if (!CTRDGi_IsExistedAtInit()) {
        return FALSE;
    }
    if (info->detectPullOut == 1) {
        return FALSE;
    }
    CTRDGi_LockByProcessor(ctrdgWork.lockID, &lockInfo);
    CTRDGi_ChangeLatestAccessCycle(&cycle);
    if ((header->isRomCode == CTRDG_IS_ROM_CODE && info->moduleID != header->moduleID) ||
        (header->isRomCode != CTRDG_IS_ROM_CODE && info->moduleID != *(u16 *)(HW_CTRDG_ROM + 0x1fffe)) ||
        (info->gameCode != header->gameCode && info->isAgbCartridge)) {
        info->detectPullOut = 1;
        result = FALSE;
    }
    CTRDGi_RestoreAccessCycle(&cycle);
    CTRDGi_UnlockByProcessor(ctrdgWork.lockID, &lockInfo);
    return result;
}

// The wait states of the ROM, in the register: the first access and the second
static inline MICartridgeRomCycle1st MI_GetCartridgeRomCycle1st(void) {
    return (reg_MI_EXMEMCNT & REG_MI_EXMEMCNT_ROM1st_MASK) >> REG_MI_EXMEMCNT_ROM1st_SHIFT;
}

static inline MICartridgeRomCycle2nd MI_GetCartridgeRomCycle2nd(void) {
    return (reg_MI_EXMEMCNT & REG_MI_EXMEMCNT_ROM2nd_MASK) >> REG_MI_EXMEMCNT_ROM2nd_SHIFT;
}

static inline void MI_SetCartridgeRomCycle1st(MICartridgeRomCycle1st cycle) {
    reg_MI_EXMEMCNT = (reg_MI_EXMEMCNT & ~REG_MI_EXMEMCNT_ROM1st_MASK) | (cycle << REG_MI_EXMEMCNT_ROM1st_SHIFT);
}

static inline void MI_SetCartridgeRomCycle2nd(MICartridgeRomCycle2nd cycle) {
    reg_MI_EXMEMCNT = (reg_MI_EXMEMCNT & ~REG_MI_EXMEMCNT_ROM2nd_MASK) | (cycle << REG_MI_EXMEMCNT_ROM2nd_SHIFT);
}

// Saves the wait states of the ROM and sets the slowest ones
static void CTRDGi_ChangeLatestAccessCycle(CTRDGRomCycle *cycle) {
    cycle->c1 = MI_GetCartridgeRomCycle1st();
    cycle->c2 = MI_GetCartridgeRomCycle2nd();
    MI_SetCartridgeRomCycle1st(CTRDG_ROMCYCLE1_18);
    MI_SetCartridgeRomCycle2nd(CTRDG_ROMCYCLE2_6);
}

// Puts the wait states back
static void CTRDGi_RestoreAccessCycle(const CTRDGRomCycle *cycle) {
    MI_SetCartridgeRomCycle1st(cycle->c1);
    MI_SetCartridgeRomCycle2nd(cycle->c2);
}

// Takes the cartridge bus with interrupts off, waiting for it if the ARM7 has it. The ID with the main processor's flag
// means that it was this processor's already, and nothing is locked
static void CTRDGi_LockByProcessor(u16 lockID, CTRDGLockByProc *info) {
    while (TRUE) {
        info->irq = OS_DisableInterrupts();
        info->locked = OS_ReadOwnerOfLockWord((OSLockWord *)HW_CTRDG_LOCK_BUF) & 0x40;
        if (info->locked != 0) {
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
static void CTRDGi_UnlockByProcessor(u16 lockID, CTRDGLockByProc *info) {
    if (info->locked == 0) {
        OS_UnLockCartridge(lockID);
    }
    OS_RestoreInterrupts(info->irq);
}

void CTRDGi_SendtoPxi(u32 data) {
    while (PXI_SendWordByFifo(PXI_FIFO_TAG_CTRDG, data, FALSE) != 0) {
        SVC_WaitByLoop(1);
    }
}
