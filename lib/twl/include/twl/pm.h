#ifndef POKEBW2_TWL_PM_H
#define POKEBW2_TWL_PM_H

#include "types.h"
#include "nitro/pm.h"

// TwlSDK's power management for the DSi, in the LTD autoload: the callbacks run when the console is about to exit the
// application, as when the power button is pressed. The functions keep their default names; the comments give the SDK
// functions they implement, by their code

typedef PMGenCallback PMExitCallback;
typedef PMGenCallbackInfo PMExitCallbackInfo;

static inline void PM_SetExitCallbackInfo(PMExitCallbackInfo *info, PMExitCallback callback, void *arg) {
    info->callback = callback;
    info->arg = arg;
}

void func_0276952c(PMExitCallbackInfo *info); // PM_AppendPreExitCallback
void func_02769588(BOOL enabled);             // PM_SetAutoExit
BOOL func_02769594(void);                     // PM_GetAutoExit
void func_027694dc(void);                     // PM_ReadyToExit
// Like func_0276952c, but with the callback's priority: inserts it into the list of the callbacks that run before the
// application exits, and func_02769568 takes it out
void func_02769550(PMExitCallbackInfo *info, int priority);
void func_02769568(PMExitCallbackInfo *info);
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
