#ifndef POKEBW2_TWL_PM_H
#define POKEBW2_TWL_PM_H

#include "types.h"
#include "nitro/pm.h"

// TwlSDK's power management for the DSi, in the LTD autoload: the callbacks run when the console is about to exit the
// application, as when the power button is pressed. The PM_ functions and the types have the names of the TWL-SDK
// Function Reference Manual (PM_SetExitCallbackInfo, PM_GetExitFactor, the exit callback lists) from their code, and
// libntr's where the manual has none; the functions of pm.c's DSi part have TwlSDK's names, which aletheia's match of
// Black 1's code to TwlSDK's pm.o gives

// The exit callbacks are the general PM callbacks, which the SDK's header names so with macros
#define PMExitCallback PMGenCallback
#define PMExitCallbackInfo PMGenCallbackInfo

// Why the system is exiting: none, the power button, a low battery, or the application (a forced power-off)
typedef enum {
    PM_EXIT_FACTOR_NONE = 0,
    PM_EXIT_FACTOR_PWSW = 1,
    PM_EXIT_FACTOR_BATTERY = 2,
    PM_EXIT_FACTOR_USER = 3
} PMExitFactor;

static inline void PM_SetExitCallbackInfo(PMExitCallbackInfo *info, PMExitCallback callback, void *arg) {
    info->callback = callback;
    info->arg = arg;
}

void PM_AppendPreExitCallback(PMExitCallbackInfo *info);
void PM_SetAutoExit(BOOL enabled);
BOOL PM_GetAutoExit(void);
void PM_ReadyToExit(void);
// Inserts the callback, by its priority, into the post-processing list, the callbacks that run when the application is
// ready to exit (PMi_ExecutePostExitCallbackList), and PM_DeletePostExitCallback takes it out. NWM and SNDEX pass
// priorities beyond the manual's -255 to 255, which the function does not check: it may be libntr's
// PMi_InsertPostExitCallbackEx
void PMi_InsertPostExitCallbackEx(PMExitCallbackInfo *info, int priority);
void PM_DeletePostExitCallback(PMExitCallbackInfo *info);
// Why the system is exiting (PMi_ExitType), which NWM checks
PMExitFactor PM_GetExitFactor(void);

// The functions of pm.c's DSi part that its main part calls on a DSi: the exit request, which starts the exit
// sequence and runs the pre-exit callbacks, the DSi's battery level, and the stop of a new DMA channel, the DSi's part
// of PMi_PreSleepForDma
void PMi_ProceedToExit(PMExitFactor exitType);
u32 PMi_GetBatteryLevelCore(u16 *level);
void PMi_PreSleepForNdma(u32 ndmaNo);
// Runs the callbacks of the post-processing list, as the power-off does, and as OS_ResetSystem and the card's
// pull-out do on a DSi
void PMi_ExecutePostExitCallbackList(void);

#endif // POKEBW2_TWL_PM_H
