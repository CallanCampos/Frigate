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
// Reads a word (at 0x14) of main's bss (PMi_IsInit)
BOOL func_02769520(void);

// The part of TwlSDK's pm.c that main's pm.c calls on a DSi. The exit request starts TwlSDK's exit sequence
// (PMi_ExitRequest, by the type in the work and the callbacks it runs), the battery level is the DSi's, and the stop of
// a new DMA channel is the DSi's part of PMi_StopDma
void func_02769498(u32 exitType);
u32 func_027693dc(u16 *level);
void func_02769410(u32 ndmaNo);

// The objects of the LTD autoload that main's pm.c reaches: the callbacks run when the system is forced to power off,
// and the one for a low battery
typedef struct {
    PMGenCallback callback;
    void *arg;
} PMBatteryLowCallbackInfo;

extern PMGenCallbackInfo *PMi_PostExitCallbackList;
extern PMBatteryLowCallbackInfo PMi_BatteryLowCallbackInfo;

#endif // POKEBW2_TWL_PM_H
