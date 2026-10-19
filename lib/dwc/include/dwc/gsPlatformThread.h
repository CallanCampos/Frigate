#ifndef POKEBW2_DWC_GSPLATFORMTHREAD_H
#define POKEBW2_DWC_GSPLATFORMTHREAD_H

#include "types.h"
#include "nitro/os.h"

// GameSpy's threads (gsPlatformThread.c), in overlay 11: counters changed with the interrupts off, and the critical
// sections, which are NitroSDK's mutexes

typedef OSMutex GSICriticalSection;

// Adds one to or takes one from a counter, and returns its new value
s32 gsiInterlockedIncrement(s32 *value);
s32 gsiInterlockedDecrement(s32 *value);
void gsiInitializeCriticalSection(GSICriticalSection *section);
void gsiEnterCriticalSection(GSICriticalSection *section);
void gsiLeaveCriticalSection(GSICriticalSection *section);
// Does nothing: a mutex needs no freeing
void gsiDeleteCriticalSection(GSICriticalSection *section);

#endif // POKEBW2_DWC_GSPLATFORMTHREAD_H
