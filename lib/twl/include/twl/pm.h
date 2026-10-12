#ifndef POKEBW2_TWL_PM_H
#define POKEBW2_TWL_PM_H

#include "types.h"
#include "nitro/pm.h"

// TwlSDK's power management for the DSi, in the LTD autoload: the callbacks run when the console is about to exit the
// application, as when the power button is pressed. The functions keep their default names until the LTD autoload is
// analyzed; the comments give the SDK functions they implement, by their code

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

#endif // POKEBW2_TWL_PM_H
