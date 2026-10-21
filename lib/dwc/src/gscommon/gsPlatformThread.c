#include "dwc/gsPlatformThread.h"

// GameSpy's threads (gsPlatformThread.c). The ROM names neither the file nor its functions; both are the later SDK's

gsi_i32 gsiInterlockedIncrement(gsi_i32 *value) {
    gsi_u32 mask;
    gsi_i32 result;

    mask = OS_DisableInterrupts_IrqAndFiq();
    result = *value + 1;
    *value = result;
    OS_RestoreInterrupts_IrqAndFiq(mask);
    return result;
}

gsi_i32 gsiInterlockedDecrement(gsi_i32 *value) {
    gsi_u32 mask;
    gsi_i32 result;

    mask = OS_DisableInterrupts_IrqAndFiq();
    result = *value - 1;
    *value = result;
    OS_RestoreInterrupts_IrqAndFiq(mask);
    return result;
}

void gsiInitializeCriticalSection(GSICriticalSection *section) {
    OS_InitMutex(section);
}

void gsiEnterCriticalSection(GSICriticalSection *section) {
    OS_LockMutex(section);
}

void gsiLeaveCriticalSection(GSICriticalSection *section) {
    OS_UnlockMutex(section);
}

void gsiDeleteCriticalSection(GSICriticalSection *section) {
}
