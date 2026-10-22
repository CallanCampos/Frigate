// NitroSDK OS: exclusive use of the video memory banks (os_vramExclusive.c). A bank is taken for a lock ID, and
// given back by the same ID. The count-zero-bits helper is assembly in the SDK, in ARM code; the rest is Thumb.
#include "nitro/os.h"

// The banks A to I
#define OS_VRAM_BANK_KINDS 9
#define OS_VRAM_BANK_ID_ALL 0x1ff

static u32 OsCountZeroBits(u32 value);

// The ID of the one who has each bank, and the banks that are taken, a bit each
static u16 OSi_vramLockId[OS_VRAM_BANK_KINDS];
static u32 OSi_vramExclusive;

#pragma thumb off
// clang-format off
// NitroSDK's OsCountZeroBits: counts the zero bits at the top of a word. It is assembly in the SDK
static asm u32 OsCountZeroBits(u32 value) {
    clz r0, r0
    bx lr
}
// clang-format on
#pragma thumb on

void OSi_InitVramExclusive(void) {
    int i;

    OSi_vramExclusive = 0;
    for (i = 0; i < OS_VRAM_BANK_KINDS; i++) {
        OSi_vramLockId[i] = 0;
    }
}

// NitroSDK's OSi_UnlockVram: gives back the banks of the mask that the lock ID has
void OSi_UnlockVram(u16 bank, u16 lockId) {
    OSIntrMode enabled = OS_DisableInterrupts();
    u32 unlocking = bank & OSi_vramExclusive & OS_VRAM_BANK_ID_ALL;

    for (;;) {
        int index = 31 - OsCountZeroBits(unlocking);

        if (index < 0) {
            break;
        }
        unlocking &= ~(1 << index);
        if (OSi_vramLockId[index] == lockId) {
            OSi_vramLockId[index] = 0;
            OSi_vramExclusive &= ~(1 << index);
        }
    }
    OS_RestoreInterrupts(enabled);
}
