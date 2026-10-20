// TwlSDK's shared WRAM manager, the DSi part of mi_sharedWram.c in the LTD autoload (ARM9 main has the NITRO part):
// the WRAM A, B and C banks' registers, and the bookkeeping of which processor has each bank, or has reserved it,
// which the ARM7 asks for through the PXI FIFO. The register functions and the public functions have TwlSDK's names as
// recalled, not shown by the code; the cores of the requests, the PXI handling and the locks are named for what they
// do. The ARM9's own requests besides the slot allocation and the release are not linked. The file is built with
// -ipa function (file_flags in lib/twl/library.toml): the original reaches each static through a literal of its own.
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

static u8 MIi_GetWramBankCnt_A(int bank);
static u8 MIi_GetWramBankCnt_B(int bank);
static u8 MIi_GetWramBankCnt_C(int bank);
static u32 MIi_GetWramBankAddr_A(int bank);
static u32 MIi_GetWramBankAddr_B(int bank);
static u32 MIi_GetWramBankAddr_C(int bank);
static void MIi_WramPxiCallback(u32 tag, u32 data, BOOL err);
static u32 MIi_AllocWramCore(MIWram wram, int count, MIWramProc proc);
static u32 MIi_AllocWramSlotCore(MIWram wram, int slot, int count, MIWramProc proc);
static int MIi_FreeWramCore(MIWram wram, MIWramProc proc);
static int MIi_FreeWramSlotCore(MIWram wram, int slot, int count, MIWramProc proc);
static int MIi_SwitchWramCore(MIWram wram, MIWramProc from, MIWramProc to);
static int MIi_SwitchWramSlotCore(MIWram wram, int slot, int count, MIWramProc from, MIWramProc to);
static u32 MIi_ReserveWramCore(MIWram wram, int count, MIWramProc proc);
static u32 MIi_ReserveWramSlotCore(MIWram wram, int slot, int count, MIWramProc proc);
static int MIi_CancelWramCore(MIWram wram, MIWramProc proc);
static int MIi_CancelWramSlotCore(MIWram wram, int slot, int count, MIWramProc proc);
static u32 MIi_GetFreeWramSlotsCore(MIWram wram, MIWramProc proc);
static void MIi_SendWramPxiReply(BOOL err, u32 data);
static void MIi_UnpackWramPxiCommand(u32 data, u32 *command, MIWram *wram, MIWramProc *proc, u32 *param);
static void MIi_LockWram(u16 *lockId);
static void MIi_UnlockWram(u16 lockId);
static u32 MIi_EnterWramCritical(u16 *lockId);
static void MIi_LeaveWramCritical(u16 lockId, u32 enabled);

// The banks' states, and where the reply to a request of the ARM9's goes, with the flag that it came. MWCC lays the
// .bss out in the reverse of the order of these definitions
static u16 sWramState[3][8];
static BOOL sRequestBusy;
static u32 *sReplyData;
static BOOL *sReplyDone;
static BOOL sWramManagerInitialized;

#pragma define_section LTDMAIN ".ltdmain"                                                                              \
                               ".ltdmain.bss"                                                                          \
                               ".ltdmain" abs32 RWX
#pragma section LTDMAIN begin

// The order of these definitions sets the order of the tables in the object
static void (*const sSetWramBankEnableFuncs[])(int bank, BOOL enable) = {
    MI_SetWramBankEnable_A,
    MI_SetWramBankEnable_B,
    MI_SetWramBankEnable_C,
};
static void *(*const sGetWramMapStartFuncs[])(void) = {
    MI_GetWramMapStart_A,
    MI_GetWramMapStart_B,
    MI_GetWramMapStart_C,
};
static MIWramProc (*const sGetWramBankMasterFuncs[])(int bank) = {
    MI_GetWramBankMaster_A,
    MI_GetWramBankMaster_B,
    MI_GetWramBankMaster_C,
};
static void (*const sSetWramBankMasterFuncs[])(int bank, MIWramProc proc) = {
    MI_SetWramBankMaster_A,
    MI_SetWramBankMaster_B,
    MI_SetWramBankMaster_C,
};

static u8 MIi_GetWramBankCnt_A(int bank) {
    return reg_MBK_A[bank];
}

static u8 MIi_GetWramBankCnt_B(int bank) {
    return reg_MBK_B[bank];
}

static u8 MIi_GetWramBankCnt_C(int bank) {
    return reg_MBK_C[bank];
}

void MI_SetWramBankMaster_A(int bank, MIWramProc proc) {
    reg_MBK_A[bank] = (reg_MBK_A[bank] & ~1) | proc;
}

void MI_SetWramBankMaster_B(int bank, MIWramProc proc) {
    reg_MBK_B[bank] = (reg_MBK_B[bank] & ~3) | proc;
}

void MI_SetWramBankMaster_C(int bank, MIWramProc proc) {
    reg_MBK_C[bank] = (reg_MBK_C[bank] & ~3) | proc;
}

void MI_SetWramBankMaster(MIWram wram, int bank, MIWramProc proc) {
    sSetWramBankMasterFuncs[wram](bank, proc);
}

void MI_SetWramBankEnable_A(int bank, BOOL enable) {
    vu8 *cnt = &reg_MBK_A[bank];
    *cnt = (*cnt & ~MI_WRAM_BANK_ENABLE) | (enable ? MI_WRAM_BANK_ENABLE : 0);
}

void MI_SetWramBankEnable_B(int bank, BOOL enable) {
    vu8 *cnt = &reg_MBK_B[bank];
    *cnt = (*cnt & ~MI_WRAM_BANK_ENABLE) | (enable ? MI_WRAM_BANK_ENABLE : 0);
}

void MI_SetWramBankEnable_C(int bank, BOOL enable) {
    vu8 *cnt = &reg_MBK_C[bank];
    *cnt = (*cnt & ~MI_WRAM_BANK_ENABLE) | (enable ? MI_WRAM_BANK_ENABLE : 0);
}

void MI_SetWramBankEnable(MIWram wram, int bank, BOOL enable) {
    sSetWramBankEnableFuncs[wram](bank, enable);
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

MIWramProc MI_GetWramBankMaster(MIWram wram, int bank) {
    return sGetWramBankMasterFuncs[wram](bank);
}

void *MI_GetWramMapStart_A(void) {
    return (void *)((((reg_MI_MBK6 & 0xff0) >> 4) << 16) + HW_WRAM_AREA);
}

void *MI_GetWramMapStart_B(void) {
    return (void *)((((reg_MI_MBK7 & 0xff8) >> 3) << 15) + HW_WRAM_AREA);
}

void *MI_GetWramMapStart_C(void) {
    return (void *)((((reg_MI_MBK8 & 0xff8) >> 3) << 15) + HW_WRAM_AREA);
}

void *MI_GetWramMapStart(MIWram wram) {
    return sGetWramMapStartFuncs[wram]();
}

// The address of a bank: WRAM A's are 0x10000 bytes, B's and C's 0x8000
static u32 MIi_GetWramBankAddr_A(int bank) {
    return (bank << 16) + (u32)MI_GetWramMapStart_A();
}

static u32 MIi_GetWramBankAddr_B(int bank) {
    return (bank << 15) + (u32)MI_GetWramMapStart_B();
}

static u32 MIi_GetWramBankAddr_C(int bank) {
    return (bank << 15) + (u32)MI_GetWramMapStart_C();
}

void MIi_InitWramManagerCore(void) {
    int i;

    if (sWramManagerInitialized) {
        return;
    }
    sWramManagerInitialized = TRUE;

    for (i = 0; i < 4; i++) {
        switch (MIi_GetWramBankCnt_A(i) & 1) {
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
        switch (MIi_GetWramBankCnt_B(i) & 3) {
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
        switch (MIi_GetWramBankCnt_C(i) & 3) {
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
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_MI, MIi_WramPxiCallback);
}

// A request of the ARM7's, which is answered with the result, or the reply to one of the ARM9's
static void MIi_WramPxiCallback(u32 tag, u32 data, BOOL err) {
    u32 command;
    u32 param;
    MIWram wram;
    MIWramProc proc;
    u32 result;

    if (!err) {
        result = 0;
        MIi_UnpackWramPxiCommand(data, &command, &wram, &proc, &param);
        switch (command) {
        case MI_WRAM_PXI_ALLOC:
            result = MIi_AllocWramCore(wram, (u8)(param >> 8), proc);
            break;
        case MI_WRAM_PXI_ALLOC_SLOT:
            result = MIi_AllocWramSlotCore(wram, (u8)(param >> 8), (u8)param, proc);
            break;
        case MI_WRAM_PXI_FREE:
            result = MIi_FreeWramCore(wram, proc);
            break;
        case MI_WRAM_PXI_FREE_SLOT:
            result = MIi_FreeWramSlotCore(wram, (u8)(param >> 8), (u8)param, proc);
            break;
        case MI_WRAM_PXI_SWITCH:
            result = MIi_SwitchWramCore(wram, proc, (param >> 12) & 3);
            break;
        case MI_WRAM_PXI_SWITCH_SLOT:
            result = MIi_SwitchWramSlotCore(wram, (param >> 8) & 0xf, (u8)param, proc, (param >> 12) & 3);
            break;
        case MI_WRAM_PXI_RESERVE:
            result = MIi_ReserveWramCore(wram, (u8)param, proc);
            break;
        case MI_WRAM_PXI_RESERVE_SLOT:
            result = MIi_ReserveWramSlotCore(wram, (u8)(param >> 8), (u8)param, proc);
            break;
        case MI_WRAM_PXI_CANCEL:
            result = MIi_CancelWramCore(wram, proc);
            break;
        case MI_WRAM_PXI_CANCEL_SLOT:
            result = MIi_CancelWramSlotCore(wram, (u8)(param >> 8), (u8)param, proc);
            break;
        case MI_WRAM_PXI_GET_RESERVER:
            result = (sWramState[wram][param] & MI_WRAM_STATE_RESERVE_MASK) >> MI_WRAM_STATE_RESERVE_SHIFT;
            break;
        case MI_WRAM_PXI_GET_FREE_SLOTS:
            result = (u8)MIi_GetFreeWramSlotsCore(wram, proc);
            break;
        }
        MIi_SendWramPxiReply(TRUE, result);
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

// Gives proc the first run of count free banks that nobody else has reserved, and returns its address
static u32 MIi_AllocWramCore(MIWram wram, int count, MIWramProc proc) {
    int found;
    int bankCount;
    int i;
    int start;

    if (wram == MI_WRAM_A) {
        count /= 2;
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
            if (++found >= count) {
                break;
            }
        } else {
            found = 0;
        }
    }
    if (found < count) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        sWramState[wram][start + i] = (sWramState[wram][start + i] & ~MI_WRAM_STATE_PROC_MASK) | proc;
        MI_SetWramBankMaster(wram, start + i, proc);
        MI_SetWramBankEnable(wram, start + i, TRUE);
    }
    if (wram == MI_WRAM_A) {
        return MIi_GetWramBankAddr_A(start);
    } else if (wram == MI_WRAM_B) {
        return MIi_GetWramBankAddr_B(start);
    } else if (wram == MI_WRAM_C) {
        return MIi_GetWramBankAddr_C(start);
    }
    return 0;
}

u32 MI_AllocWramSlot(MIWram wram, int slot, int count, MIWramProc proc) {
    u16 lockId;
    u32 enabled = MIi_EnterWramCritical(&lockId);
    u32 address = MIi_AllocWramSlotCore(wram, slot, count, proc);
    MIi_LeaveWramCritical(lockId, enabled);
    return address;
}

// Gives proc the count banks from slot if they are all free, and returns their address
static u32 MIi_AllocWramSlotCore(MIWram wram, int slot, int count, MIWramProc proc) {
    int end;
    int i;

    if (wram == MI_WRAM_A) {
        count /= 2;
    }
    for (i = slot, end = slot + count; i < end; i++) {
        u16 state = sWramState[wram][i];
        MIWramProc reserver = (state & MI_WRAM_STATE_RESERVE_MASK) >> MI_WRAM_STATE_RESERVE_SHIFT;
        MIWramProc owner = state & MI_WRAM_STATE_PROC_MASK;
        if (owner != MI_WRAM_FREE || (reserver != proc && reserver != MI_WRAM_FREE)) {
            return 0;
        }
    }

    for (i = slot; i < end; i++) {
        sWramState[wram][i] = (sWramState[wram][i] & ~MI_WRAM_STATE_PROC_MASK) | proc;
        MI_SetWramBankMaster(wram, i, proc);
        MI_SetWramBankEnable(wram, i, TRUE);
    }
    if (wram == MI_WRAM_A) {
        return MIi_GetWramBankAddr_A(slot);
    } else if (wram == MI_WRAM_B) {
        return MIi_GetWramBankAddr_B(slot);
    } else if (wram == MI_WRAM_C) {
        return MIi_GetWramBankAddr_C(slot);
    }
    return 0;
}

int MI_FreeWram(MIWram wram, MIWramProc proc) {
    u16 lockId;
    u32 enabled = MIi_EnterWramCritical(&lockId);
    int freed = MIi_FreeWramCore(wram, proc);
    MIi_LeaveWramCritical(lockId, enabled);
    return freed;
}

// Frees every bank that proc has, and returns how many
static int MIi_FreeWramCore(MIWram wram, MIWramProc proc) {
    int freed = 0;
    int bankCount = (wram == MI_WRAM_A) ? 4 : 8;
    int i;

    for (i = 0; i < bankCount; i++) {
        u16 state = sWramState[wram][i];
        if ((state & MI_WRAM_STATE_PROC_MASK) == proc) {
            sWramState[wram][i] = (sWramState[wram][i] & ~MI_WRAM_STATE_PROC_MASK) | MI_WRAM_FREE;
            MI_SetWramBankMaster(wram, i, MI_WRAM_ARM7);
            MI_SetWramBankEnable(wram, i, FALSE);
            freed++;
        }
    }
    return freed;
}

int MI_FreeWramSlot(MIWram wram, int slot, int count, MIWramProc proc) {
    u16 lockId;
    u32 enabled = MIi_EnterWramCritical(&lockId);
    int freed = MIi_FreeWramSlotCore(wram, slot, count, proc);
    MIi_LeaveWramCritical(lockId, enabled);
    return freed;
}

// Frees the banks from slot that proc has, and returns how many
static int MIi_FreeWramSlotCore(MIWram wram, int slot, int count, MIWramProc proc) {
    int end;
    int freed;

    if (wram == MI_WRAM_A) {
        count /= 2;
    }
    freed = 0;
    end = slot + count;
    for (; slot < end; slot++) {
        u16 state = sWramState[wram][slot];
        if ((state & MI_WRAM_STATE_PROC_MASK) == proc) {
            sWramState[wram][slot] = (sWramState[wram][slot] & ~MI_WRAM_STATE_PROC_MASK) | MI_WRAM_FREE;
            MI_SetWramBankMaster(wram, slot, MI_WRAM_ARM7);
            MI_SetWramBankEnable(wram, slot, FALSE);
            freed++;
        }
    }
    return freed;
}

// Gives every bank that from has to to, and returns how many
static int MIi_SwitchWramCore(MIWram wram, MIWramProc from, MIWramProc to) {
    int switched = 0;
    int bankCount = (wram == MI_WRAM_A) ? 4 : 8;
    int i;

    for (i = 0; i < bankCount; i++) {
        u16 state = sWramState[wram][i];
        if ((state & MI_WRAM_STATE_PROC_MASK) == from) {
            sWramState[wram][i] = (sWramState[wram][i] & ~MI_WRAM_STATE_PROC_MASK) | to;
            MI_SetWramBankMaster(wram, i, to);
            MI_SetWramBankEnable(wram, i, TRUE);
            switched++;
        }
    }
    return switched;
}

// Gives the banks from slot that from has to to, and returns how many
static int MIi_SwitchWramSlotCore(MIWram wram, int slot, int count, MIWramProc from, MIWramProc to) {
    int switched;
    int end;

    if (wram == MI_WRAM_A) {
        count /= 2;
    }
    switched = 0;
    end = slot + count;
    for (; slot < end; slot++) {
        u16 state = sWramState[wram][slot];
        if ((state & MI_WRAM_STATE_PROC_MASK) == from) {
            sWramState[wram][slot] = (sWramState[wram][slot] & ~MI_WRAM_STATE_PROC_MASK) | to;
            MI_SetWramBankMaster(wram, slot, to);
            MI_SetWramBankEnable(wram, slot, TRUE);
            switched++;
        }
    }
    return switched;
}

// Reserves for proc the first run of count banks that are free and not reserved, and returns its address
static u32 MIi_ReserveWramCore(MIWram wram, int count, MIWramProc proc) {
    int found;
    int bankCount;
    int i;
    int start;

    if (wram == MI_WRAM_A) {
        count /= 2;
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
            if (++found >= count) {
                break;
            }
        } else {
            found = 0;
        }
    }
    if (found < count) {
        return 0;
    }

    for (i = 0; i < count; i++) {
        sWramState[wram][start + i] =
            (sWramState[wram][start + i] & ~MI_WRAM_STATE_RESERVE_MASK) | (proc << MI_WRAM_STATE_RESERVE_SHIFT);
    }
    if (wram == MI_WRAM_A) {
        return MIi_GetWramBankAddr_A(start);
    } else if (wram == MI_WRAM_B) {
        return MIi_GetWramBankAddr_B(start);
    } else if (wram == MI_WRAM_C) {
        return MIi_GetWramBankAddr_C(start);
    }
    return 0;
}

// Reserves for proc the count banks from slot if they are free and not reserved, and returns their address
static u32 MIi_ReserveWramSlotCore(MIWram wram, int slot, int count, MIWramProc proc) {
    int end;
    int i;

    if (wram == MI_WRAM_A) {
        count /= 2;
    }
    end = slot + count;
    for (i = slot; i < end; i++) {
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

    for (i = slot; i < end; i++) {
        sWramState[wram][i] =
            (sWramState[wram][i] & ~MI_WRAM_STATE_RESERVE_MASK) | (proc << MI_WRAM_STATE_RESERVE_SHIFT);
    }
    if (wram == MI_WRAM_A) {
        return MIi_GetWramBankAddr_A(slot);
    } else if (wram == MI_WRAM_B) {
        return MIi_GetWramBankAddr_B(slot);
    } else if (wram == MI_WRAM_C) {
        return MIi_GetWramBankAddr_C(slot);
    }
    return 0;
}

int MI_CancelWram(MIWram wram, MIWramProc proc) {
    u16 lockId;
    u32 enabled = MIi_EnterWramCritical(&lockId);
    int canceled = MIi_CancelWramCore(wram, proc);
    MIi_LeaveWramCritical(lockId, enabled);
    return canceled;
}

// Cancels every reservation of proc's, and returns how many
static int MIi_CancelWramCore(MIWram wram, MIWramProc proc) {
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
static int MIi_CancelWramSlotCore(MIWram wram, int slot, int count, MIWramProc proc) {
    int end;
    int canceled;

    if (wram == MI_WRAM_A) {
        count /= 2;
    }
    canceled = 0;
    end = slot + count;
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
static u32 MIi_GetFreeWramSlotsCore(MIWram wram, MIWramProc proc) {
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

BOOL MIi_IsWramSlotUsed(MIWram wram, int slot) {
    if ((sWramState[wram][slot] & MI_WRAM_STATE_PROC_MASK) != MI_WRAM_FREE) {
        return TRUE;
    }
    return FALSE;
}

static void MIi_SendWramPxiReply(BOOL err, u32 data) {
    while (PXI_SendWordByFifo(PXI_FIFO_TAG_MI, data, err) != PXI_FIFO_SUCCESS) {
        SVC_WaitByLoop(1);
    }
}

static void MIi_UnpackWramPxiCommand(u32 data, u32 *command, MIWram *wram, MIWramProc *proc, u32 *param) {
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

static void MIi_LockWram(u16 *lockId) {
    *lockId = OS_GetLockID();
    OS_LockByWord(*lockId, MI_WRAM_LOCK_WORD, NULL);
}

static void MIi_UnlockWram(u16 lockId) {
    OS_UnlockByWord(lockId, MI_WRAM_LOCK_WORD, NULL);
    OS_ReleaseLockID(lockId);
}

static u32 MIi_EnterWramCritical(u16 *lockId) {
    u32 enabled = OS_DisableInterrupts();
    MIi_LockWram(lockId);
    return enabled;
}

static void MIi_LeaveWramCritical(u16 lockId, u32 enabled) {
    MIi_UnlockWram(lockId);
    OS_RestoreInterrupts(enabled);
}

#pragma section LTDMAIN end
