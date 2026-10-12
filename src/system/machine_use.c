#include "types.h"
#include "gfl/ui.h"
#include "nitro/hw.h"
#include "nitro/mi.h"
#include "nitro/mic.h"
#include "nitro/os.h"
#include "nitro/rtc.h"
#include "nnsys/gfd.h"
#include "system/dsi.h"
#include "system/machine_use.h"
#include "system/main.h"
#include "twl/pm.h"

// The game's setup of the console and the SDK, with what the main loop does for them each frame. The file name is a
// guess after gfl_use.c, which follows it, as the ROM does not embed it; its statics have a base of their own, so it
// is not main's file. SystemInit and SystemUpdateVBlank are names from swan
// (https://github.com/ds-pokemon-hacking/swan, GPL-3.0).
//
// The calls to the DSi's power management go to the LTD autoload, out of reach of a branch, so the linker adds a
// veneer after each calling function: the `bx pc` stubs and ARM jumps after SystemInit, func_02005430 and
// OnExitRequest (swan's dsi276952D, dsi2769595, dsi27694DD and dsi2769589) are those, not functions of this file

// The DSi's system configuration: the extensions it enables for the game, and the copy that the boot code keeps
#define reg_SCFG_EXT (*(vu32 *)0x04004008)
#define SCFG_EXT_COPY (*(vu32 *)0x02fffdf0)
#define SCFG_EXT_VRAM 0x2000

// The bits of the console type (func_0207ac24) that SystemInit checks
#define CONSOLE_TYPE_DEV_MASK 0x3000000

#define VRAM_TRANSFER_TASK_COUNT 48

static void OnExitRequest(void *arg);

static BOOL sExitRequested;
static PMExitCallbackInfo sExitCallbackInfo;
static NNSGfdVramTransferTask sVramTransferTasks[VRAM_TRANSFER_TASK_COUNT];

void SystemInit(void) {
    u32 tableSize;
    void *table;
    int amp;

    os_init();
    clock_init();
    func_0207bbf4();
    func_0207cb88();
    gfxInit();
    func_02074658();

    if (func_0207ac24() & CONSOLE_TYPE_DEV_MASK) {
        func_0207f6e8(1);
        reg_SCFG_EXT &= ~SCFG_EXT_VRAM;
        SCFG_EXT_COPY &= ~SCFG_EXT_VRAM;
    }

    (void)OS_EnableIrq();
    fs_init(1);
    tableSize = func_02072358(NULL, 0);
    table = mem_alloc_direct(OS_ARENA_MAIN, tableSize, 4);
    func_02072358(table, tableSize);

    NNS_GfdInitVramTransferManager(sVramTransferTasks, VRAM_TRANSFER_TASK_COUNT);

    if (isRunningOnDSi()) {
        sExitRequested = FALSE;
        PM_SetExitCallbackInfo(&sExitCallbackInfo, OnExitRequest, NULL);
        func_0276952c(&sExitCallbackInfo);
    }

    func_0207f0ac(&amp);
    if (amp == PM_AMP_ON) {
        func_0207efc4(PM_AMP_OFF);
    }
}

void func_02005430(void) {
    if (sExitRequested && !GCTX_HIDIsSoftResetBlocked(2) && !func_02769594()) {
        MI_SetMainMemoryPriority(MI_PROCESSOR_ARM7);
        func_027694dc();
    }
}

void SystemUpdateVBlank(void) {
    NNS_GfdDoVramTransfer();
}

void func_0200548c(void) {
    func_020827d0();
}

// Called when the DSi is about to exit the game: keeps it from exiting until func_02005430 lets it
static void OnExitRequest(void *arg) {
    func_02769588(FALSE);
    sExitRequested = TRUE;
}
