// The BIOS's CpuSet (NitroSDK's SVC_CpuSet, a Thumb stub that runs SWI 0x0b, with no C in the decomp), on the host.
// Its only caller is the cartridge library's CTRDGi_InitCommon, which clears its work with it, so it lives with the
// cartridge slot; a module for the BIOS calls would take it.
//
// GBATEK, "BIOS Memory Copy": the mode's bits 0-20 are the number of units, bit 24 fills (the source's first unit is
// written to every unit of the destination) instead of copying, and bit 26 makes the units words instead of halfwords.
// The BIOS aligns the addresses down to the unit.
#include "nitro/svc.h"

#define SVC_CPUSET_COUNT_MASK 0x001fffff

void SVC_CpuSet(const void *src, void *dest, u32 mode) {
    u32 count = mode & SVC_CPUSET_COUNT_MASK;
    BOOL fill = (mode & MI_DMA_SRC_FIX) != 0;
    u32 i;

    if (mode & MI_DMA_32BIT_BUS) {
        const vu32 *from = (const vu32 *)((uintptr_t)src & ~(uintptr_t)3);
        vu32 *to = (vu32 *)((uintptr_t)dest & ~(uintptr_t)3);
        u32 value = *from;

        for (i = 0; i < count; i++) {
            to[i] = fill ? value : from[i];
        }
    } else {
        const vu16 *from = (const vu16 *)((uintptr_t)src & ~(uintptr_t)1);
        vu16 *to = (vu16 *)((uintptr_t)dest & ~(uintptr_t)1);
        u16 value = *from;

        for (i = 0; i < count; i++) {
            to[i] = fill ? value : from[i];
        }
    }
}
