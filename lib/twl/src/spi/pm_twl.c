// TwlSDK's DSi part of pm.c (ARM9 main), in the LTD autoload: the DSi's battery level, the stop of a new DMA channel
// before the sleep, and the exit sequence, with its callbacks, that the power button and the power-off start. The
// object is one with main's pm.c, whose statics PMi_ExitType and PMi_AutoExitFlag this part reaches at offsets of
// pm.c's first objects. The file name is inferred from main's half, as card_rom_twl.c's is; the PM_ functions have
// TwlSDK's names, the others are ours
#include "nitro/gx.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "twl/mi.h"
#include "twl/pm.h"

// The request that reads the battery level of the DSi
#define PM_REQ_BATTERY_LEVEL 0xf
// The requests that shut the DSi down and that restart it
#define PM_REQ_SHUTDOWN 0x13
#define PM_REQ_RESET 0x14
// The wait between two reads of the vertical blank count
#define PM_VBLANK_WAIT_CYCLES 0x32

// The vertical blank count of the shared memory, which the vertical blank interrupt increments
#define PM_VBLANK_COUNT (*(vu32 *)0x02fffc3c)
// The word of main memory that a restart after an exit notification clears
#define PM_RESET_PARAM (*(u32 *)0x02000300)
// The shared memory of the handshake with the ARM7 before the exit, and the byte that says the ARM9 is done
#define PM_EXIT_SYNC_WORK ((vu8 *)0x02fffff0)
#define PM_EXIT_SYNC_DONE (*(vu8 *)0x02fffff4)

static void PMi_SyncExitWithOtherProc(void);
static void PMi_ExecuteExit(BOOL reset);
static BOOL PMi_TryLockExit(void);

PMGenCallbackInfo PMi_BatteryLowCallbackInfo;
PMGenCallbackInfo *PMi_PostExitCallbackList;
// Set once the exit sequence has started
static BOOL PMi_ExitLocked;
// The callbacks that run when the exit sequence starts
static PMGenCallbackInfo *PMi_PreExitCallbackList;

#pragma define_section LTDMAIN ".ltdmain"                                                                              \
                               ".ltdmain.bss"                                                                          \
                               ".ltdmain" abs32 RWX
#pragma section LTDMAIN begin

u32 PMi_GetBatteryLevelCore(u16 *level) {
    u16 value;
    u32 result = PMi_ReadRegister(PM_REQ_BATTERY_LEVEL, 1, &value);

    if (result == 0 && level != NULL) {
        *level = value;
    }
    return result;
}

void PMi_StopNDma(u32 ndmaNo) {
    if (MI_NDMA_REG(ndmaNo, MI_NDMA_CNT) & MI_NDMA_IMM) {
        MI_WaitNDma(ndmaNo);
        return;
    }
    switch (MI_NDMA_REG(ndmaNo, MI_NDMA_CNT) & MI_NDMA_TIMING_MASK) {
    case MI_NDMA_TIMING_CARD:
        while (reg_MI_MCCNT1 & REG_MI_MCCNT1_START_MASK) {
        }
        break;
    case MI_NDMA_TIMING_DISP:
    case MI_NDMA_TIMING_DISP_MMEM:
        MI_WaitNDma(ndmaNo);
    case MI_NDMA_TIMING_CAMERA:
        MI_StopNDma(ndmaNo);
        break;
    default:
        if (!(MI_NDMA_REG(ndmaNo, MI_NDMA_CNT) & MI_NDMA_CONTINUOUS)) {
            MI_WaitNDma(ndmaNo);
        } else {
            MI_StopNDma(ndmaNo);
        }
        break;
    }
}

void PMi_ProcessExitRequest(u32 exitType) {
    if (PMi_TryLockExit()) {
        PMi_ExitType = exitType;
        PMi_CallCallbackList(PMi_PreExitCallbackList);
        if (PMi_AutoExitFlag) {
            PM_ReadyToExit();
        }
    }
}

void PM_ReadyToExit(void) {
    PMi_ExecuteExit(TRUE);
}

// Tells the ARM7 that the ARM9 is about to stop
static void PMi_SyncExitWithOtherProc(void) {
    PM_EXIT_SYNC_DONE = FALSE;
    OSi_SyncWithOtherProc(0, PM_EXIT_SYNC_WORK);
    OSi_SyncWithOtherProc(1, PM_EXIT_SYNC_WORK);
    OS_DisableInterrupts();
    PM_EXIT_SYNC_DONE = TRUE;
}

u32 PMi_GetExitType(void) {
    return PMi_ExitType;
}

void PM_AppendPreExitCallback(PMExitCallbackInfo *info) {
    PMi_InsertList(&PMi_PreExitCallbackList, info, 0xff, 0);
}

void PM_InsertPreExitCallback(PMExitCallbackInfo *info, int priority) {
    PMi_InsertList(&PMi_PostExitCallbackList, info, priority, 0);
}

void PM_DeletePreExitCallback(PMExitCallbackInfo *info) {
    PMi_DeleteList(&PMi_PostExitCallbackList, info);
}

void PMi_ExecutePostExitCallbackList(void) {
    PMi_CallCallbackList(PMi_PostExitCallbackList);
}

void PM_SetAutoExit(BOOL enabled) {
    PMi_AutoExitFlag = enabled;
}

BOOL PM_GetAutoExit(void) {
    return PMi_AutoExitFlag;
}

// Runs the post-exit callbacks, turns the displays off, waits for three vertical blanks, has the ARM7 restart or shut
// the system down until it answers, and stops the program
static void PMi_ExecuteExit(BOOL reset) {
    int i;
    u16 result;

    PMi_CallCallbackList(PMi_PostExitCallbackList);
    GX_DispOff();
    GXS_DispOff();
    MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
    for (i = 0; i < 3; i++) {
        u32 count = PM_VBLANK_COUNT;
        while (count == PM_VBLANK_COUNT) {
            OS_SpinWaitSysCycles(PM_VBLANK_WAIT_CYCLES);
        }
    }

    for (;;) {
        u8 request;
        if (reset) {
            request = PM_REQ_RESET;
        } else {
            request = PM_REQ_SHUTDOWN;
        }
        if (*(u32 *)HW_PM_EXIT_FLAG == 1) {
            request = PM_REQ_RESET;
            PM_RESET_PARAM = 0;
        }
        while (PMi_ReadRegister(request, 0, &result) != 0) {
            OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
        }
        if (result == 0) {
            break;
        }
        OS_SpinWaitSysCycles(PM_RETRY_CYCLES);
    }

    OS_DisableInterrupts();
    MI_StopAllDma();
    MI_StopNDmaChannels();
    PMi_SyncExitWithOtherProc();
    OSi_TerminateCore();
}

static BOOL PMi_TryLockExit(void) {
    u32 enabled = OS_DisableInterrupts();

    if (PMi_ExitLocked) {
        OS_RestoreInterrupts(enabled);
        return FALSE;
    }
    PMi_ExitLocked = TRUE;
    OS_RestoreInterrupts(enabled);
    return TRUE;
}

#pragma section LTDMAIN end
