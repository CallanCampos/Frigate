// NitroSDK's exclusive use of the video memory banks (os_vramExclusive.c) on the host: the decomp's
// lib/nitro/src/os/os_vramExclusive.c, by its names, with its count-leading-zeros helper, OsCountZeroBits (assembly in
// the SDK, CLZ), in C. A bank is taken for a lock ID, and given back by the same ID.
#include "hal/os/os_internal.h"
#include "nitro/os.h"

// The banks A to I
#define OS_VRAM_BANK_KINDS 9
#define OS_VRAM_BANK_ID_ALL 0x1ff

// The ID of the one who has each bank, and the banks that are taken, a bit each
static u16 OSi_vramLockId[OS_VRAM_BANK_KINDS];
static u32 OSi_vramExclusive;

// NitroSDK's OsCountZeroBits: counts the zero bits at the top of a word, 32 for 0 as CLZ gives
static u32 OsCountZeroBits(u32 value) {
    return value == 0 ? 32 : (u32)__builtin_clz(value);
}

void port_osi_reset_vram_exclusive(void) {
    OSi_InitVramExclusive();
}

void OSi_InitVramExclusive(void) {
    int i;

    OSi_vramExclusive = 0;
    for (i = 0; i < OS_VRAM_BANK_KINDS; i++) {
        OSi_vramLockId[i] = 0;
    }
}

// NitroSDK's OSi_UnlockVram: gives back the banks of the mask that the lock ID has
void OSi_UnlockVram(u16 bank, u16 lockId) {
    u32 enabled = OS_DisableInterrupts();
    u32 unlocking = bank & OSi_vramExclusive & OS_VRAM_BANK_ID_ALL;

    for (;;) {
        int index = 31 - (int)OsCountZeroBits(unlocking);

        if (index < 0) {
            break;
        }
        unlocking &= ~(1u << index);
        if (OSi_vramLockId[index] == lockId) {
            OSi_vramLockId[index] = 0;
            OSi_vramExclusive &= ~(1u << index);
        }
    }
    OS_RestoreInterrupts(enabled);
}
