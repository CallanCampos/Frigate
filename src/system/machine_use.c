#include "types.h"
#include "gfl/ui.h"
#include "nitro/ctrdg.h"
#include "nitro/fs.h"
#include "nitro/fx.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nitro/mic.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/rtc.h"
#include "nnsys/gfd.h"
#include "system/dsi.h"
#include "system/machine_use.h"
#include "system/main.h"
#include "twl/pm.h"
#include "twl/scfg.h"

// The game's setup of the console and the SDK, with what the main loop does for them each frame. The file name is a
// guess after gfl_use.c, which follows it, as the ROM does not embed it; its statics have a base of their own, so it
// is not main's file. SystemInit and SystemUpdateVBlank are names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
//
// The calls to the DSi's power management go to the LTD autoload, out of reach of a branch, so the linker adds a
// veneer after each calling function: the `bx pc` stubs and ARM jumps after SystemInit, Machine_ProcessExitRequest and
// OnExitRequest (swan's dsi276952D, dsi2769595, dsi27694DD and dsi2769589) are those, not functions of this file

// The DSi's system configuration: the extensions it enables for the game, and the copy that the boot code keeps
#define reg_SCFG_EXT (*(vu32 *)0x04004008)
#define SCFG_EXT_COPY (*(vu32 *)0x02fffdf0)
#define SCFG_EXT_VRAM 0x2000

#define VRAM_TRANSFER_TASK_COUNT 48

static void OnExitRequest(void *arg);

static BOOL sExitRequested;
static PMExitCallbackInfo sExitCallbackInfo;
static NNSGfdVramTransferTask sVramTransferTasks[VRAM_TRANSFER_TASK_COUNT];

void SystemInit(void) {
    u32 tableSize;
    void *table;
    int amp;

    OS_Init();
    OS_InitTick();
    OS_InitAlarm();
    RTC_Init();
    GX_Init();
    FX_Init();

    if (OS_GetConsoleType() & OS_CONSOLE_DEV_MASK) {
        SCFG_SetCpuSpeed(SCFG_CPU_SPEED_2X);
        reg_SCFG_EXT &= ~SCFG_EXT_VRAM;
        SCFG_EXT_COPY &= ~SCFG_EXT_VRAM;
    }

    (void)OS_EnableIrq();
    FS_Init(1);
    tableSize = FS_LoadTable(NULL, 0);
    table = OS_AllocFromArenaLo(OS_ARENA_MAIN, tableSize, 4);
    FS_LoadTable(table, tableSize);

    NNS_GfdInitVramTransferManager(sVramTransferTasks, VRAM_TRANSFER_TASK_COUNT);

    if (isRunningOnDSi()) {
        sExitRequested = FALSE;
        PM_SetExitCallbackInfo(&sExitCallbackInfo, OnExitRequest, NULL);
        PM_AppendPreExitCallback(&sExitCallbackInfo);
    }

    PM_GetAmp(&amp);
    if (amp == PM_AMP_ON) {
        PM_SetAmp(PM_AMP_OFF);
    }
}

void Machine_ProcessExitRequest(void) {
    if (sExitRequested && !GCTX_HIDIsSoftResetBlocked(2) && !PM_GetAutoExit()) {
        MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
        PM_ReadyToExit();
    }
}

void SystemUpdateVBlank(void) {
    NNS_GfdDoVramTransfer();
}

void CTRDG_Init(void) {
    CTRDG_DummyInit();
}

// Called when the DSi is about to exit the game: keeps it from exiting until Machine_ProcessExitRequest lets it
static void OnExitRequest(void *arg) {
    PM_SetAutoExit(FALSE);
    sExitRequested = TRUE;
}
