// TwlSDK's shared WRAM manager (mi_sharedWram.c, TwlSDK's mi_sharedwram.o, which aletheia's match of Black 1's code to
// libmi.TWL.HYB.a gives with its functions' names): in ARM9 main, the call into the LTD autoload on a DSi; in TwlSDK's
// LTDMAIN section, which the LTD autoload's .ltdmain holds, the DSi part, which dsd lists as
// lib/twl/src/mi/mi_sharedWram_twl.c and config/merged_sources.toml links from this file's object: the WRAM A, B and C
// banks' registers, and the bookkeeping of which processor has each bank, or has reserved it, which the ARM7 asks for
// through the PXI FIFO. The functions and the tables of functions have TwlSDK's names; the other statics have names of
// ours. The ARM9's own requests besides the slot allocation and the release are not linked. The DSi part's statics are
// in its section, where each has a section of its own and a literal of its own, as the original reaches them.
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "nitro/svc.h"
#include "twl/mi.h"

// Each bank's state: the processor that has it, and the one that has reserved it (bits 8 and 9), MI_WRAM_FREE for
// none
#define MI_WRAM_FREE 3
#define MI_WRAM_STATE_PROC_MASK 0x3
#define MI_WRAM_STATE_RESERVE_SHIFT 8
#define MI_WRAM_STATE_RESERVE_MASK 0x300

#define MI_WRAM_BANK_ENABLE 0x80

// Where each WRAM is mapped in the ARM9's memory: its start in units of a slot
#define reg_MI_MBK6 (*(vu32 *)0x04004054)
#define reg_MI_MBK7 (*(vu32 *)0x04004058)
#define reg_MI_MBK8 (*(vu32 *)0x0400405c)
#define HW_WRAM_AREA 0x03000000

// The ARM7's requests to the manager, a word each: the request, the WRAM, the processor and the parameters
#define MI_WRAM_PXI_ALLOC 1
#define MI_WRAM_PXI_ALLOC_SLOT 2
#define MI_WRAM_PXI_FREE 3
#define MI_WRAM_PXI_FREE_SLOT 4
#define MI_WRAM_PXI_SWITCH 5
#define MI_WRAM_PXI_SWITCH_SLOT 6
#define MI_WRAM_PXI_RESERVE 7
#define MI_WRAM_PXI_RESERVE_SLOT 8
#define MI_WRAM_PXI_CANCEL 9
#define MI_WRAM_PXI_CANCEL_SLOT 10
#define MI_WRAM_PXI_GET_RESERVER 11
#define MI_WRAM_PXI_GET_FREE_SLOTS 14
#define PXI_FIFO_TAG_MI 18

// The lock word that the two processors share for the manager
#define MI_WRAM_LOCK_WORD ((OSLockWord *)0x02fff680)

static u8 MIi_GetWramBank_A(int bank);
static u8 MIi_GetWramBank_B(int bank);
static u8 MIi_GetWramBank_C(int bank);
static u32 MIi_GetAddress_A(int bank);
static u32 MIi_GetAddress_B(int bank);
static u32 MIi_GetAddress_C(int bank);
static void MIi_CallbackForPxi(PXIFifoTag tag, u32 data, BOOL err);
static u32 MIi_DoAllocWram(MIWramPos wram, MIWramSize size, MIWramProc proc);
static u32 MIi_DoAllocWramSlot(MIWramPos wram, int slot, MIWramSize size, MIWramProc proc);
static int MIi_DoFreeWram(MIWramPos wram, MIWramProc proc);
static int MIi_DoFreeWramSlot(MIWramPos wram, int slot, MIWramSize size, MIWramProc proc);
static int MIi_DoSwitchWram(MIWramPos wram, MIWramProc from, MIWramProc to);
static int MIi_DoSwitchWramSlot(MIWramPos wram, int slot, MIWramSize size, MIWramProc from, MIWramProc to);
static u32 MIi_DoReserveWram(MIWramPos wram, MIWramSize size, MIWramProc proc);
static u32 MIi_DoReserveWramSlot(MIWramPos wram, int slot, MIWramSize size, MIWramProc proc);
static int MIi_DoCancelWram(MIWramPos wram, MIWramProc proc);
static int MIi_DoCancelWramSlot(MIWramPos wram, int slot, MIWramSize size, MIWramProc proc);
static u32 MIi_DoGetAllocatableWramSlot(MIWramPos wram, MIWramProc proc);
static void MIi_SendToPxi(BOOL err, u32 data);
static void MIi_TakeApartData(u32 data, u32 *command, MIWramPos *wram, MIWramProc *proc, u32 *param);
static void MI_LockWramManager(u16 *lockId);
static void MI_UnlockWramManager(u16 lockId);
static u32 MIi_LockWramManagerDI(u16 *lockId);
static void MIi_UnlockWramManagerRI(u16 lockId, u32 enabled);

// clang-format off
// TwlSDK's section for the DSi-only code and data that the LTD autoload's .ltdmain holds
#pragma define_section LTDMAIN ".ltdmain" ".ltdmain.bss" ".ltdmain" abs32 RWX
// clang-format on

// The banks' states, and where the reply to a request of the ARM9's goes, with the flag that it came, in the LTD
// autoload's .ltdmain.bss. The linker lays them out in the reverse of the order of these definitions
#pragma section LTDMAIN begin
static u16 sWramState[3][8];
static BOOL sRequestBusy;
static u32 *sReplyData;
static BOOL *sReplyDone;
static BOOL sWramManagerInitialized;
#pragma section LTDMAIN end

// On a DSi, sets up the manager in the LTD autoload
void MI_InitWramManager(void) {
    if (OS_IsRunOnTwl()) {
        MIi_InitWramManager();
    }
}

// The DSi part, in the LTD autoload
#pragma section LTDMAIN begin

// Each table has a section of its own, which the linker places in the order of these definitions, the ROM's
static MIWramProc (*const MIi_GetWramBankMasterFuncs[])(int bank) = {
    MI_GetWramBankMaster_A,
    MI_GetWramBankMaster_B,
    MI_GetWramBankMaster_C,
};
static u32 (*const MIi_GetWramMapStartFuncs[])(void) = {
    MI_GetWramMapStart_A,
    MI_GetWramMapStart_B,
    MI_GetWramMapStart_C,
};
static void (*const MIi_SetWramBankEnableFuncs[])(int bank, MIWramEnable enable) = {
    MIi_SetWramBankEnable_A,
    MIi_SetWramBankEnable_B,
    MIi_SetWramBankEnable_C,
};
static void (*const MIi_SetWramBankMasterFuncs[])(int bank, MIWramProc proc) = {
    MIi_SetWramBankMaster_A,
    MIi_SetWramBankMaster_B,
    MIi_SetWramBankMaster_C,
};

static u8 MIi_GetWramBank_A(int bank) {
    return reg_MBK_A[bank];
}

static u8 MIi_GetWramBank_B(int bank) {
    return reg_MBK_B[bank];
}

static u8 MIi_GetWramBank_C(int bank) {
    return reg_MBK_C[bank];
}

void MIi_SetWramBankMaster_A(int bank, MIWramProc proc) {
    reg_MBK_A[bank] = (reg_MBK_A[bank] & ~1) | proc;
}

void MIi_SetWramBankMaster_B(int bank, MIWramProc proc) {
    reg_MBK_B[bank] = (reg_MBK_B[bank] & ~3) | proc;
}

void MIi_SetWramBankMaster_C(int bank, MIWramProc proc) {
    reg_MBK_C[bank] = (reg_MBK_C[bank] & ~3) | proc;
}

void MIi_SetWramBankMaster(MIWramPos wram, int bank, MIWramProc proc) {
    MIi_SetWramBankMasterFuncs[wram](bank, proc);
}

void MIi_SetWramBankEnable_A(int bank, MIWramEnable enable) {
    vu8 *cnt = &reg_MBK_A[bank];
    *cnt = (*cnt & ~MI_WRAM_BANK_ENABLE) | (enable ? MI_WRAM_BANK_ENABLE : 0);
}

void MIi_SetWramBankEnable_B(int bank, MIWramEnable enable) {
    vu8 *cnt = &reg_MBK_B[bank];
    *cnt = (*cnt & ~MI_WRAM_BANK_ENABLE) | (enable ? MI_WRAM_BANK_ENABLE : 0);
}

void MIi_SetWramBankEnable_C(int bank, MIWramEnable enable) {
    vu8 *cnt = &reg_MBK_C[bank];
    *cnt = (*cnt & ~MI_WRAM_BANK_ENABLE) | (enable ? MI_WRAM_BANK_ENABLE : 0);
}

void MIi_SetWramBankEnable(MIWramPos wram, int bank, MIWramEnable enable) {
    MIi_SetWramBankEnableFuncs[wram](bank, enable);
}

MIWramProc MI_GetWramBankMaster_A(int bank) {
    return reg_MBK_A[bank] & 1;
}

MIWramProc MI_GetWramBankMaster_B(int bank) {
    return reg_MBK_B[bank] & 3;
}

MIWramProc MI_GetWramBankMaster_C(int bank) {
    return reg_MBK_C[bank] & 3;
}

MIWramProc MI_GetWramBankMaster(MIWramPos wram, int bank) {
    return MIi_GetWramBankMasterFuncs[wram](bank);
}

u32 MI_GetWramMapStart_A(void) {
    return (((reg_MI_MBK6 & 0xff0) >> 4) << 16) + HW_WRAM_AREA;
}

u32 MI_GetWramMapStart_B(void) {
    return (((reg_MI_MBK7 & 0xff8) >> 3) << 15) + HW_WRAM_AREA;
}

u32 MI_GetWramMapStart_C(void) {
    return (((reg_MI_MBK8 & 0xff8) >> 3) << 15) + HW_WRAM_AREA;
}

u32 MI_GetWramMapStart(MIWramPos wram) {
    return MIi_GetWramMapStartFuncs[wram]();
}

// The address of a bank: WRAM A's are 0x10000 bytes, B's and C's 0x8000
static u32 MIi_GetAddress_A(int bank) {
    return (bank << 16) + MI_GetWramMapStart_A();
}

static u32 MIi_GetAddress_B(int bank) {
    return (bank << 15) + MI_GetWramMapStart_B();
}

static u32 MIi_GetAddress_C(int bank) {
    return (bank << 15) + MI_GetWramMapStart_C();
}

void MIi_InitWramManager(void) {
    int i;

    if (sWramManagerInitialized) {
        return;
    }
    sWramManagerInitialized = TRUE;

    for (i = 0; i < 4; i++) {
        switch (MIi_GetWramBank_A(i) & 1) {
        case MI_WRAM_ARM9:
            sWramState[MI_WRAM_A][i] = (sWramState[MI_WRAM_A][i] & ~MI_WRAM_STATE_RESERVE_MASK) |
                                       (MI_WRAM_ARM9 << MI_WRAM_STATE_RESERVE_SHIFT);
            sWramState[MI_WRAM_A][i] = (sWramState[MI_WRAM_A][i] & ~MI_WRAM_STATE_PROC_MASK) | MI_WRAM_ARM9;
            break;
        case MI_WRAM_ARM7:
            sWramState[MI_WRAM_A][i] = (sWramState[MI_WRAM_A][i] & ~MI_WRAM_STATE_RESERVE_MASK) |
                                       (MI_WRAM_ARM7 << MI_WRAM_STATE_RESERVE_SHIFT);
            sWramState[MI_WRAM_A][i] = (sWramState[MI_WRAM_A][i] & ~MI_WRAM_STATE_PROC_MASK) | MI_WRAM_ARM7;
            break;
        }
    }
    for (i = 0; i < 8; i++) {
        switch (MIi_GetWramBank_B(i) & 3) {
        case MI_WRAM_ARM9:
            sWramState[MI_WRAM_B][i] = (sWramState[MI_WRAM_B][i] & ~MI_WRAM_STATE_RESERVE_MASK) |
                                       (MI_WRAM_ARM9 << MI_WRAM_STATE_RESERVE_SHIFT);
            sWramState[MI_WRAM_B][i] = (sWramState[MI_WRAM_B][i] & ~MI_WRAM_STATE_PROC_MASK) | MI_WRAM_ARM9;
            break;
        case MI_WRAM_ARM7:
            sWramState[MI_WRAM_B][i] = (sWramState[MI_WRAM_B][i] & ~MI_WRAM_STATE_RESERVE_MASK) |
                                       (MI_WRAM_ARM7 << MI_WRAM_STATE_RESERVE_SHIFT);
            sWramState[MI_WRAM_B][i] = (sWramState[MI_WRAM_B][i] & ~MI_WRAM_STATE_PROC_MASK) | MI_WRAM_ARM7;
            break;
        default:
            sWramState[MI_WRAM_B][i] =
                (sWramState[MI_WRAM_B][i] & ~MI_WRAM_STATE_RESERVE_MASK) | (MI_WRAM_DSP << MI_WRAM_STATE_RESERVE_SHIFT);
            sWramState[MI_WRAM_B][i] = (sWramState[MI_WRAM_B][i] & ~MI_WRAM_STATE_PROC_MASK) | MI_WRAM_DSP;
            break;
        }
    }
    for (i = 0; i < 8; i++) {
        switch (MIi_GetWramBank_C(i) & 3) {
        case MI_WRAM_ARM9:
            sWramState[MI_WRAM_C][i] = (sWramState[MI_WRAM_C][i] & ~MI_WRAM_STATE_RESERVE_MASK) |
                                       (MI_WRAM_ARM9 << MI_WRAM_STATE_RESERVE_SHIFT);
            sWramState[MI_WRAM_C][i] = (sWramState[MI_WRAM_C][i] & ~MI_WRAM_STATE_PROC_MASK) | MI_WRAM_ARM9;
            break;
        case MI_WRAM_ARM7:
            sWramState[MI_WRAM_C][i] = (sWramState[MI_WRAM_C][i] & ~MI_WRAM_STATE_RESERVE_MASK) |
                                       (MI_WRAM_ARM7 << MI_WRAM_STATE_RESERVE_SHIFT);
            sWramState[MI_WRAM_C][i] = (sWramState[MI_WRAM_C][i] & ~MI_WRAM_STATE_PROC_MASK) | MI_WRAM_ARM7;
            break;
        default:
            sWramState[MI_WRAM_C][i] =
                (sWramState[MI_WRAM_C][i] & ~MI_WRAM_STATE_RESERVE_MASK) | (MI_WRAM_DSP << MI_WRAM_STATE_RESERVE_SHIFT);
            sWramState[MI_WRAM_C][i] = (sWramState[MI_WRAM_C][i] & ~MI_WRAM_STATE_PROC_MASK) | MI_WRAM_DSP;
            break;
        }
    }
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_MI, MIi_CallbackForPxi);
}

// A request of the ARM7's, which is answered with the result, or the reply to one of the ARM9's
static void MIi_CallbackForPxi(PXIFifoTag tag, u32 data, BOOL err) {
    u32 command;
    u32 param;
    MIWramPos wram;
    MIWramProc proc;
    u32 result;

    if (!err) {
        result = 0;
        MIi_TakeApartData(data, &command, &wram, &proc, &param);
        switch (command) {
        case MI_WRAM_PXI_ALLOC:
            result = MIi_DoAllocWram(wram, (u8)(param >> 8), proc);
            break;
        case MI_WRAM_PXI_ALLOC_SLOT:
            result = MIi_DoAllocWramSlot(wram, (u8)(param >> 8), (u8)param, proc);
            break;
        case MI_WRAM_PXI_FREE:
            result = MIi_DoFreeWram(wram, proc);
            break;
        case MI_WRAM_PXI_FREE_SLOT:
            result = MIi_DoFreeWramSlot(wram, (u8)(param >> 8), (u8)param, proc);
            break;
        case MI_WRAM_PXI_SWITCH:
            result = MIi_DoSwitchWram(wram, proc, (param >> 12) & 3);
            break;
        case MI_WRAM_PXI_SWITCH_SLOT:
            result = MIi_DoSwitchWramSlot(wram, (param >> 8) & 0xf, (u8)param, proc, (param >> 12) & 3);
            break;
        case MI_WRAM_PXI_RESERVE:
            result = MIi_DoReserveWram(wram, (u8)param, proc);
            break;
        case MI_WRAM_PXI_RESERVE_SLOT:
            result = MIi_DoReserveWramSlot(wram, (u8)(param >> 8), (u8)param, proc);
            break;
        case MI_WRAM_PXI_CANCEL:
            result = MIi_DoCancelWram(wram, proc);
            break;
        case MI_WRAM_PXI_CANCEL_SLOT:
            result = MIi_DoCancelWramSlot(wram, (u8)(param >> 8), (u8)param, proc);
            break;
        case MI_WRAM_PXI_GET_RESERVER:
            result = (sWramState[wram][param] & MI_WRAM_STATE_RESERVE_MASK) >> MI_WRAM_STATE_RESERVE_SHIFT;
            break;
        case MI_WRAM_PXI_GET_FREE_SLOTS:
            result = (u8)MIi_DoGetAllocatableWramSlot(wram, proc);
            break;
        }
        MIi_SendToPxi(TRUE, result);
    } else {
        if (sReplyData != NULL) {
            *sReplyData = data;
        }
        if (sReplyDone != NULL) {
            *sReplyDone = TRUE;
        }
        sRequestBusy = FALSE;
    }
}

// Gives proc the first free run of banks of the size (in 32 KB units) that nobody else has reserved, and returns its
// address
static u32 MIi_DoAllocWram(MIWramPos wram, MIWramSize size, MIWramProc proc) {
    int found;
    int bankCount;
    int i;
    int start;

    if (wram == MI_WRAM_A) {
        size /= 2;
    }
    found = 0;
    bankCount = (wram == MI_WRAM_A) ? 4 : 8;
    for (i = 0; i < bankCount; i++) {
        u16 state = sWramState[wram][i];
        MIWramProc reserver = (state & MI_WRAM_STATE_RESERVE_MASK) >> MI_WRAM_STATE_RESERVE_SHIFT;
        MIWramProc owner = state & MI_WRAM_STATE_PROC_MASK;
        if (owner == MI_WRAM_FREE && (reserver == proc || reserver == MI_WRAM_FREE)) {
            if (found == 0) {
                start = i;
            }
            if (++found >= size) {
                break;
            }
        } else {
            found = 0;
        }
    }
    if (found < size) {
        return 0;
    }

    for (i = 0; i < size; i++) {
        sWramState[wram][start + i] = (sWramState[wram][start + i] & ~MI_WRAM_STATE_PROC_MASK) | proc;
        MIi_SetWramBankMaster(wram, start + i, proc);
        MIi_SetWramBankEnable(wram, start + i, MI_WRAM_ENABLE);
    }
    if (wram == MI_WRAM_A) {
        return MIi_GetAddress_A(start);
    } else if (wram == MI_WRAM_B) {
        return MIi_GetAddress_B(start);
    } else if (wram == MI_WRAM_C) {
        return MIi_GetAddress_C(start);
    }
    return 0;
}

u32 MI_AllocWramSlot(MIWramPos wram, int slot, MIWramSize size, MIWramProc proc) {
    u16 lockId;
    u32 enabled = MIi_LockWramManagerDI(&lockId);
    u32 address = MIi_DoAllocWramSlot(wram, slot, size, proc);
    MIi_UnlockWramManagerRI(lockId, enabled);
    return address;
}

// Gives proc the banks of the size from slot if they are all free, and returns their address
static u32 MIi_DoAllocWramSlot(MIWramPos wram, int slot, MIWramSize size, MIWramProc proc) {
    int end;
    int i;

    if (wram == MI_WRAM_A) {
        size /= 2;
    }
    for (i = slot, end = slot + size; i < end; i++) {
        u16 state = sWramState[wram][i];
        MIWramProc reserver = (state & MI_WRAM_STATE_RESERVE_MASK) >> MI_WRAM_STATE_RESERVE_SHIFT;
        MIWramProc owner = state & MI_WRAM_STATE_PROC_MASK;
        if (owner != MI_WRAM_FREE || (reserver != proc && reserver != MI_WRAM_FREE)) {
            return 0;
        }
    }

    for (i = slot; i < end; i++) {
        sWramState[wram][i] = (sWramState[wram][i] & ~MI_WRAM_STATE_PROC_MASK) | proc;
        MIi_SetWramBankMaster(wram, i, proc);
        MIi_SetWramBankEnable(wram, i, MI_WRAM_ENABLE);
    }
    if (wram == MI_WRAM_A) {
        return MIi_GetAddress_A(slot);
    } else if (wram == MI_WRAM_B) {
        return MIi_GetAddress_B(slot);
    } else if (wram == MI_WRAM_C) {
        return MIi_GetAddress_C(slot);
    }
    return 0;
}

int MI_FreeWram(MIWramPos wram, MIWramProc proc) {
    u16 lockId;
    u32 enabled = MIi_LockWramManagerDI(&lockId);
    int freed = MIi_DoFreeWram(wram, proc);
    MIi_UnlockWramManagerRI(lockId, enabled);
    return freed;
}

// Frees every bank that proc has, and returns how many
static int MIi_DoFreeWram(MIWramPos wram, MIWramProc proc) {
    int freed = 0;
    int bankCount = (wram == MI_WRAM_A) ? 4 : 8;
    int i;

    for (i = 0; i < bankCount; i++) {
        u16 state = sWramState[wram][i];
        if ((state & MI_WRAM_STATE_PROC_MASK) == proc) {
            sWramState[wram][i] = (sWramState[wram][i] & ~MI_WRAM_STATE_PROC_MASK) | MI_WRAM_FREE;
            MIi_SetWramBankMaster(wram, i, MI_WRAM_ARM7);
            MIi_SetWramBankEnable(wram, i, MI_WRAM_DISABLE);
            freed++;
        }
    }
    return freed;
}

int MI_FreeWramSlot(MIWramPos wram, int slot, MIWramSize size, MIWramProc proc) {
    u16 lockId;
    u32 enabled = MIi_LockWramManagerDI(&lockId);
    int freed = MIi_DoFreeWramSlot(wram, slot, size, proc);
    MIi_UnlockWramManagerRI(lockId, enabled);
    return freed;
}

// Frees the banks from slot that proc has, and returns how many
static int MIi_DoFreeWramSlot(MIWramPos wram, int slot, MIWramSize size, MIWramProc proc) {
    int end;
    int freed;

    if (wram == MI_WRAM_A) {
        size /= 2;
    }
    freed = 0;
    end = slot + size;
    for (; slot < end; slot++) {
        u16 state = sWramState[wram][slot];
        if ((state & MI_WRAM_STATE_PROC_MASK) == proc) {
            sWramState[wram][slot] = (sWramState[wram][slot] & ~MI_WRAM_STATE_PROC_MASK) | MI_WRAM_FREE;
            MIi_SetWramBankMaster(wram, slot, MI_WRAM_ARM7);
            MIi_SetWramBankEnable(wram, slot, MI_WRAM_DISABLE);
            freed++;
        }
    }
    return freed;
}

// Gives every bank that from has to to, and returns how many
static int MIi_DoSwitchWram(MIWramPos wram, MIWramProc from, MIWramProc to) {
    int switched = 0;
    int bankCount = (wram == MI_WRAM_A) ? 4 : 8;
    int i;

    for (i = 0; i < bankCount; i++) {
        u16 state = sWramState[wram][i];
        if ((state & MI_WRAM_STATE_PROC_MASK) == from) {
            sWramState[wram][i] = (sWramState[wram][i] & ~MI_WRAM_STATE_PROC_MASK) | to;
            MIi_SetWramBankMaster(wram, i, to);
            MIi_SetWramBankEnable(wram, i, MI_WRAM_ENABLE);
            switched++;
        }
    }
    return switched;
}

// Gives the banks from slot that from has to to, and returns how many
static int MIi_DoSwitchWramSlot(MIWramPos wram, int slot, MIWramSize size, MIWramProc from, MIWramProc to) {
    int switched;
    int end;

    if (wram == MI_WRAM_A) {
        size /= 2;
    }
    switched = 0;
    end = slot + size;
    for (; slot < end; slot++) {
        u16 state = sWramState[wram][slot];
        if ((state & MI_WRAM_STATE_PROC_MASK) == from) {
            sWramState[wram][slot] = (sWramState[wram][slot] & ~MI_WRAM_STATE_PROC_MASK) | to;
            MIi_SetWramBankMaster(wram, slot, to);
            MIi_SetWramBankEnable(wram, slot, MI_WRAM_ENABLE);
            switched++;
        }
    }
    return switched;
}

// Reserves for proc the first run of banks of the size that are free and not reserved, and returns its address
static u32 MIi_DoReserveWram(MIWramPos wram, MIWramSize size, MIWramProc proc) {
    int found;
    int bankCount;
    int i;
    int start;

    if (wram == MI_WRAM_A) {
        size /= 2;
    }
    found = 0;
    bankCount = (wram == MI_WRAM_A) ? 4 : 8;
    for (i = 0; i < bankCount; i++) {
        u16 state = sWramState[wram][i];
        MIWramProc reserver = (state & MI_WRAM_STATE_RESERVE_MASK) >> MI_WRAM_STATE_RESERVE_SHIFT;
        MIWramProc owner = state & MI_WRAM_STATE_PROC_MASK;
        if (owner == MI_WRAM_FREE && reserver == MI_WRAM_FREE) {
            if (found == 0) {
                start = i;
            }
            if (++found >= size) {
                break;
            }
        } else {
            found = 0;
        }
    }
    if (found < size) {
        return 0;
    }

    for (i = 0; i < size; i++) {
        sWramState[wram][start + i] =
            (sWramState[wram][start + i] & ~MI_WRAM_STATE_RESERVE_MASK) | (proc << MI_WRAM_STATE_RESERVE_SHIFT);
    }
    if (wram == MI_WRAM_A) {
        return MIi_GetAddress_A(start);
    } else if (wram == MI_WRAM_B) {
        return MIi_GetAddress_B(start);
    } else if (wram == MI_WRAM_C) {
        return MIi_GetAddress_C(start);
    }
    return 0;
}

// Reserves for proc the banks of the size from slot if they are free and not reserved, and returns their address
static u32 MIi_DoReserveWramSlot(MIWramPos wram, int slot, MIWramSize size, MIWramProc proc) {
    int banks = (wram == MI_WRAM_A) ? size / 2 : size;
    int i;

    for (i = slot; i < slot + banks; i++) {
        // BUG: Each pass checks the first bank, not bank i
#ifdef BUGFIX
        MIWramProc owner = sWramState[wram][i] & MI_WRAM_STATE_PROC_MASK;
        MIWramProc reserver = (sWramState[wram][i] & MI_WRAM_STATE_RESERVE_MASK) >> MI_WRAM_STATE_RESERVE_SHIFT;
#else
        MIWramProc owner = sWramState[wram][slot] & MI_WRAM_STATE_PROC_MASK;
        MIWramProc reserver = (sWramState[wram][slot] & MI_WRAM_STATE_RESERVE_MASK) >> MI_WRAM_STATE_RESERVE_SHIFT;
#endif
        if (owner != MI_WRAM_FREE || reserver != MI_WRAM_FREE) {
            return 0;
        }
    }

    for (i = slot; i < slot + banks; i++) {
        sWramState[wram][i] =
            (sWramState[wram][i] & ~MI_WRAM_STATE_RESERVE_MASK) | (proc << MI_WRAM_STATE_RESERVE_SHIFT);
    }
    if (wram == MI_WRAM_A) {
        return MIi_GetAddress_A(slot);
    } else if (wram == MI_WRAM_B) {
        return MIi_GetAddress_B(slot);
    } else if (wram == MI_WRAM_C) {
        return MIi_GetAddress_C(slot);
    }
    return 0;
}

int MI_CancelWram(MIWramPos wram, MIWramProc proc) {
    u16 lockId;
    u32 enabled = MIi_LockWramManagerDI(&lockId);
    int canceled = MIi_DoCancelWram(wram, proc);
    MIi_UnlockWramManagerRI(lockId, enabled);
    return canceled;
}

// Cancels every reservation of proc's, and returns how many
static int MIi_DoCancelWram(MIWramPos wram, MIWramProc proc) {
    int canceled = 0;
    int bankCount = (wram == MI_WRAM_A) ? 4 : 8;
    int i;

    for (i = 0; i < bankCount; i++) {
        if (proc == (sWramState[wram][i] & MI_WRAM_STATE_RESERVE_MASK) >> MI_WRAM_STATE_RESERVE_SHIFT) {
            sWramState[wram][i] =
                (sWramState[wram][i] & ~MI_WRAM_STATE_RESERVE_MASK) | (MI_WRAM_FREE << MI_WRAM_STATE_RESERVE_SHIFT);
            canceled++;
        }
    }
    return canceled;
}

// Cancels proc's reservations of the banks from slot, and returns how many
static int MIi_DoCancelWramSlot(MIWramPos wram, int slot, MIWramSize size, MIWramProc proc) {
    int end;
    int canceled;

    if (wram == MI_WRAM_A) {
        size /= 2;
    }
    canceled = 0;
    end = slot + size;
    for (; slot < end; slot++) {
        u16 state = sWramState[wram][slot];
        if (proc == (state & MI_WRAM_STATE_RESERVE_MASK) >> MI_WRAM_STATE_RESERVE_SHIFT) {
            sWramState[wram][slot] =
                (state & ~MI_WRAM_STATE_RESERVE_MASK) | (MI_WRAM_FREE << MI_WRAM_STATE_RESERVE_SHIFT);
            canceled++;
        }
    }
    return canceled;
}

// The banks that proc could be given: a bit per bank that is free and not reserved for another
static u32 MIi_DoGetAllocatableWramSlot(MIWramPos wram, MIWramProc proc) {
    u32 slots = 0;
    int bankCount = (wram == MI_WRAM_A) ? 4 : 8;
    int i;

    for (i = 0; i < bankCount; i++) {
        u16 state = sWramState[wram][i];
        MIWramProc reserver = (state & MI_WRAM_STATE_RESERVE_MASK) >> MI_WRAM_STATE_RESERVE_SHIFT;
        MIWramProc owner = state & MI_WRAM_STATE_PROC_MASK;
        if (owner == MI_WRAM_FREE && (reserver == proc || reserver == MI_WRAM_FREE)) {
            slots |= 1 << i;
        }
    }
    return slots;
}

BOOL MI_IsWramSlotUsed(MIWramPos wram, int slot) {
    if ((sWramState[wram][slot] & MI_WRAM_STATE_PROC_MASK) != MI_WRAM_FREE) {
        return TRUE;
    }
    return FALSE;
}

static void MIi_SendToPxi(BOOL err, u32 data) {
    while (PXI_SendWordByFifo(PXI_FIFO_TAG_MI, data, err) != PXI_FIFO_SUCCESS) {
        SVC_WaitByLoop(1);
    }
}

static void MIi_TakeApartData(u32 data, u32 *command, MIWramPos *wram, MIWramProc *proc, u32 *param) {
    if (command != NULL) {
        *command = (data >> 20) & 0xf;
    }
    if (wram != NULL) {
        *wram = (data >> 18) & 3;
    }
    if (proc != NULL) {
        *proc = (data >> 16) & 3;
    }
    if (param != NULL) {
        *param = data & 0xffff;
    }
}

static void MI_LockWramManager(u16 *lockId) {
    *lockId = OS_GetLockID();
    OS_LockByWord(*lockId, MI_WRAM_LOCK_WORD, NULL);
}

static void MI_UnlockWramManager(u16 lockId) {
    OS_UnlockByWord(lockId, MI_WRAM_LOCK_WORD, NULL);
    OS_ReleaseLockID(lockId);
}

static u32 MIi_LockWramManagerDI(u16 *lockId) {
    OSIntrMode enabled = OS_DisableInterrupts();
    MI_LockWramManager(lockId);
    return enabled;
}

static void MIi_UnlockWramManagerRI(u16 lockId, u32 enabled) {
    MI_UnlockWramManager(lockId);
    OS_RestoreInterrupts(enabled);
}

#pragma section LTDMAIN end
