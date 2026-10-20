#ifndef POKEBW2_TWL_PM_H
#define POKEBW2_TWL_PM_H

#include "types.h"
#include "nitro/pm.h"

// TwlSDK's power management for the DSi, in the LTD autoload: the callbacks run when the console is about to exit the
// application, as when the power button is pressed. The functions have the names of the SDK functions they implement,
// from their code; the ones still named by address are not known

typedef PMGenCallback PMExitCallback;
typedef PMGenCallbackInfo PMExitCallbackInfo;

static inline void PM_SetExitCallbackInfo(PMExitCallbackInfo *info, PMExitCallback callback, void *arg) {
    info->callback = callback;
    info->arg = arg;
}

void PM_AppendPreExitCallback(PMExitCallbackInfo *info);
void PM_SetAutoExit(BOOL enabled);
BOOL PM_GetAutoExit(void);
void PM_ReadyToExit(void);
// Like PM_AppendPreExitCallback, but with the callback's priority: inserts it into the list of the callbacks that run
// before the application exits, and PM_DeletePreExitCallback takes it out
void PM_InsertPreExitCallback(PMExitCallbackInfo *info, int priority);
void PM_DeletePreExitCallback(PMExitCallbackInfo *info);
// Why the system is exiting (PMi_ExitType), which NWM checks
u32 PMi_GetExitType(void);

// The part of TwlSDK's pm.c that main's pm.c calls on a DSi: the exit request, which starts the exit sequence and runs
// the pre-exit callbacks, the DSi's battery level, and the stop of a new DMA channel, the DSi's part of PMi_StopDma
void PMi_ProcessExitRequest(u32 exitType);
u32 PMi_GetBatteryLevelCore(u16 *level);
void PMi_StopNDma(u32 ndmaNo);
// Runs the callbacks of PM_InsertPreExitCallback's list, as the power-off does, and as OS_ResetSystem and the card's
// pull-out do on a DSi
void PMi_ExecutePostExitCallbackList(void);

// The objects of the LTD autoload that main's pm.c reaches: the callbacks run when the system is forced to power off,
// and the one for a low battery
extern PMGenCallbackInfo *PMi_PostExitCallbackList;
extern PMGenCallbackInfo PMi_BatteryLowCallbackInfo;

#endif // POKEBW2_TWL_PM_H
