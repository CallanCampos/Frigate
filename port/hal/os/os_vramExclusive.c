// NitroSDK's exclusive use of the video memory banks (os_vramExclusive.c) on the host: the decomp's
// lib/nitro/src/os/os_vramExclusive.c, with its count-leading-zeros helper (assembly in the SDK, CLZ) in C. A bank is
// taken for a lock ID, and given back by the same ID.
#include "hal/os/os_internal.h"
#include "nitro/os.h"

// The banks A to I
#define OSi_VRAM_BANK_NUM 9
#define OSi_VRAM_ALL_BANKS 0x1ff

// The ID of the one who has each bank, and the banks that are taken, a bit each
static u16 OSi_vramLockId[OSi_VRAM_BANK_NUM];
static u32 OSi_vramExclusive;

// NitroSDK's helper that counts the zero bits at the top of a word, 32 for 0 as CLZ gives
static u32 OSi_CountLeadingZeros(u32 value) {
    return value == 0 ? 32 : (u32)__builtin_clz(value);
}

void port_osi_reset_vram_exclusive(void) {
    OSi_InitVramExclusive();
}

void OSi_InitVramExclusive(void) {
    int i;

    OSi_vramExclusive = 0;
    for (i = 0; i < OSi_VRAM_BANK_NUM; i++) {
        OSi_vramLockId[i] = 0;
    }
}

// NitroSDK's OSi_UnlockVram: gives back the banks of the mask that the lock ID has
void OSi_UnlockVram(u16 bank, u16 lockId) {
    u32 enabled = OS_DisableInterrupts();
    u32 unlocking = bank & OSi_vramExclusive & OSi_VRAM_ALL_BANKS;

    for (;;) {
        int index = 31 - (int)OSi_CountLeadingZeros(unlocking);

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
