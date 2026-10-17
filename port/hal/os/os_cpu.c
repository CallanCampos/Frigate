// The parts of NitroSDK's OS that only drive the ARM946E-S's coprocessor 15 or the DS's memory map, which a host has no
// use for: the caches (os_cache.c), the memory protection unit (os_protectionRegion.c, os_protectionUnit.c), the TCM
// address (os_tcm.c) and the exception vector (os_exception.c). All of them are assembly in the SDK, or install
// assembly.
//
// The caches: the host's memory is coherent between the CPU and what the HAL does for DMA, so flushing, storing and
// invalidating do nothing. The protection unit: the DS uses it to catch stray accesses, which the host's own memory
// protection and the sanitizer builds catch. The exception vector: the host reports faults itself (port/hal/trap.c).
#include "hal/os/os_internal.h"
#include "nitro/os.h"
#include <stdint.h>

// NitroSDK's DC_FlushAll, DC_InvalidateRange, DC_StoreRange, DC_FlushRange, DC_WaitWriteBufferEmpty,
// IC_InvalidateAll and IC_InvalidateRange
void DC_FlushAll(void) {
}

void DC_InvalidateRange(void *addr, u32 size) {
}

void DC_StoreRange(const void *addr, u32 size) {
}

void DC_FlushRange(const void *addr, u32 size) {
}

void DC_WaitWriteBufferEmpty(void) {
}

void IC_InvalidateAll(void) {
}

void IC_InvalidateRange(void *addr, u32 size) {
}

// NitroSDK's OS_EnableProtectionUnit, OS_DisableProtectionUnit and OS_SetProtectionRegion
void OS_EnableProtectionUnit(void) {
}

void OS_DisableProtectionUnit(void) {
}

void OS_SetProtectionRegion(int regionNo, u32 address, u32 size) {
}

// NitroSDK's OS_GetDTCMAddress: where the data TCM is mapped. The host's DTCM is a block of host memory; this gives the
// low 32 bits of its address, which is what os.h's HW_DTCM gives too, so that the SDK's u32 range checks
// (card_rom.c) compare like with like
u32 OS_GetDTCMAddress(void) {
    return (u32)(uintptr_t)port_osi_dtcm();
}

// NitroSDK's OS_InitException: installs the SDK's handler in the exception vector, unless a debugger's is there, and
// clears the game's own handler. The host installs none, since port/hal/trap.c reports faults, and nothing in the
// program sets a handler of its own (OS_SetUserExceptionHandler isn't linked)
void OS_InitException(void) {
}
