#include "types.h"
#include "app/dwc_utility.h"
#include "dwc/utility.h"
#include "gfl/heap.h"
#include "gfl/net.h"
#include "gfl/overlay.h"
#include "gfl/proc.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "system/wih_dwc.h"

// The game's procs around NitroDWC's Wi-Fi Connection settings (dwc_raputil.c, from the string of its allocation),
// which the start menu and the Game Sync event open. The rest of the overlay is the library's

static void DwcRapUtil_StartUtility(HeapID heapId);
static BOOL DwcRapUtil_ProcInit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL DwcRapUtil_ProcMain(GameProc *proc, u32 *state, void *param, void *work);
static BOOL DwcRapUtil_ResetProcExit(GameProc *proc, u32 *state, void *param, void *work);
static BOOL DwcRapUtil_ProcExit(GameProc *proc, u32 *state, void *param, void *work);

const GameProcFunctions DWC_UTILITY_RESET_PROC_FUNCTIONS = {
    DwcRapUtil_ProcInit,
    DwcRapUtil_ProcMain,
    DwcRapUtil_ResetProcExit,
};

const GameProcFunctions DWC_UTILITY_PROC_FUNCTIONS = {
    DwcRapUtil_ProcInit,
    DwcRapUtil_ProcMain,
    DwcRapUtil_ProcExit,
};

// Runs the settings with interrupts off, which a DSi skips
static void DwcRapUtil_StartUtility(HeapID heapId) {
    u32 mask = CPU_IRQDisable();
    void *work = GFL_HeapAllocate(heapId, DWC_UTILITY_WORK_SIZE, TRUE, "dwc_raputil.c", 61);

    if (!hw_isDSi()) {
        DWC_StartUtility(work, DWC_LANGUAGE_ENGLISH, DWC_UTILITY_TOP_MENU_FOR_USA);
    }
    GFL_HeapFree(work);
    CPU_SetIRQMask(mask);
    OS_EnableIrq();
}

static BOOL DwcRapUtil_ProcInit(GameProc *proc, u32 *state, void *param, void *work) {
    GFL_HeapCreateChild(HEAPID_USER, HEAPID_DWC_UTILITY, 0x40100);
    func_020116c0(GFL_NET_TYPE_WIFI);
    GFL_OvlLoad(OVERLAY_ID(181));
    return TRUE;
}

static BOOL DwcRapUtil_ProcMain(GameProc *proc, u32 *state, void *param, void *work) {
    DwcRapUtil_StartUtility(HEAPID_DWC_UTILITY);
    return TRUE;
}

// Restarts the game, for the start menu
static BOOL DwcRapUtil_ResetProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    sys_reset(0);
    return TRUE;
}

static BOOL DwcRapUtil_ProcExit(GameProc *proc, u32 *state, void *param, void *work) {
    func_02011778(GFL_NET_TYPE_WIFI);
    GFL_OvlUnload(OVERLAY_ID(181));
    GFL_HeapDelete(HEAPID_DWC_UTILITY);
    downloadPlayConfigForWifi();
    return TRUE;
}
