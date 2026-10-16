// NitroSDK OS: the memory protection unit's regions (os_protectionRegion.c). ARM code. Each region's register is
// reached through its own function, since only an instruction can name a coprocessor register; the functions are in
// tables that the region numbers index.
#include "nitro/os.h"

static void OS_SetProtectionRegionParam(int regionNo, u32 param);
static asm void OSi_SetPRParam0(u32 param); // swan's mprotect_region0
static asm void OSi_SetPRParam1(u32 param);
static asm void OSi_SetPRParam2(u32 param);
static asm void OSi_SetPRParam3(u32 param);
static asm void OSi_SetPRParam4(u32 param);
static asm void OSi_SetPRParam5(u32 param);
static asm void OSi_SetPRParam6(u32 param);
static asm void OSi_SetPRParam7(u32 param);
static asm u32 OSi_GetPRParam0(void);
static asm u32 OSi_GetPRParam1(void);
static asm u32 OSi_GetPRParam2(void);
static asm u32 OSi_GetPRParam3(void);
static asm u32 OSi_GetPRParam4(void);
static asm u32 OSi_GetPRParam5(void);
static asm u32 OSi_GetPRParam6(void);
static asm u32 OSi_GetPRParam7(void);

// The setters of the eight regions' registers (swan's MEMORY_PROTECTION_SET_FUNCS), then their getters (the ones for
// NitroSDK's OS_GetProtectionRegionParam, which nothing links)
static void (*OSi_SetPRFuncs[8])(u32 param) = {
    OSi_SetPRParam0, OSi_SetPRParam1, OSi_SetPRParam2, OSi_SetPRParam3,
    OSi_SetPRParam4, OSi_SetPRParam5, OSi_SetPRParam6, OSi_SetPRParam7,
};
static u32 (*OSi_GetPRFuncs[8])(void) = {
    OSi_GetPRParam0, OSi_GetPRParam1, OSi_GetPRParam2, OSi_GetPRParam3,
    OSi_GetPRParam4, OSi_GetPRParam5, OSi_GetPRParam6, OSi_GetPRParam7,
};

// NitroSDK's OS_SetProtectionRegionParam: writes the register of a region (address, size and enable bit) through the
// function for it
static void OS_SetProtectionRegionParam(int regionNo, u32 param) {
    OSi_SetPRFuncs[regionNo](param);
}

// clang-format off
// The setters and getters of coprocessor 15's registers c6,c0 to c6,c7, the regions' registers. Each is assembly in
// the SDK, since only an instruction can name a coprocessor register

// Assembly in the SDK: writes region 0's register, c6,c0
static asm void OSi_SetPRParam0(u32 param) {
    mcr p15, 0, r0, c6, c0, 0
    bx lr
}

// Assembly in the SDK: writes region 1's register, c6,c1
static asm void OSi_SetPRParam1(u32 param) {
    mcr p15, 0, r0, c6, c1, 0
    bx lr
}

// Assembly in the SDK: writes region 2's register, c6,c2
static asm void OSi_SetPRParam2(u32 param) {
    mcr p15, 0, r0, c6, c2, 0
    bx lr
}

// Assembly in the SDK: writes region 3's register, c6,c3
static asm void OSi_SetPRParam3(u32 param) {
    mcr p15, 0, r0, c6, c3, 0
    bx lr
}

// Assembly in the SDK: writes region 4's register, c6,c4
static asm void OSi_SetPRParam4(u32 param) {
    mcr p15, 0, r0, c6, c4, 0
    bx lr
}

// Assembly in the SDK: writes region 5's register, c6,c5
static asm void OSi_SetPRParam5(u32 param) {
    mcr p15, 0, r0, c6, c5, 0
    bx lr
}

// Assembly in the SDK: writes region 6's register, c6,c6
static asm void OSi_SetPRParam6(u32 param) {
    mcr p15, 0, r0, c6, c6, 0
    bx lr
}

// Assembly in the SDK: writes region 7's register, c6,c7
static asm void OSi_SetPRParam7(u32 param) {
    mcr p15, 0, r0, c6, c7, 0
    bx lr
}

// Assembly in the SDK: reads region 0's register, c6,c0
static asm u32 OSi_GetPRParam0(void) {
    mrc p15, 0, r0, c6, c0, 0
    bx lr
}

// Assembly in the SDK: reads region 1's register, c6,c1
static asm u32 OSi_GetPRParam1(void) {
    mrc p15, 0, r0, c6, c1, 0
    bx lr
}

// Assembly in the SDK: reads region 2's register, c6,c2
static asm u32 OSi_GetPRParam2(void) {
    mrc p15, 0, r0, c6, c2, 0
    bx lr
}

// Assembly in the SDK: reads region 3's register, c6,c3
static asm u32 OSi_GetPRParam3(void) {
    mrc p15, 0, r0, c6, c3, 0
    bx lr
}

// Assembly in the SDK: reads region 4's register, c6,c4
static asm u32 OSi_GetPRParam4(void) {
    mrc p15, 0, r0, c6, c4, 0
    bx lr
}

// Assembly in the SDK: reads region 5's register, c6,c5
static asm u32 OSi_GetPRParam5(void) {
    mrc p15, 0, r0, c6, c5, 0
    bx lr
}

// Assembly in the SDK: reads region 6's register, c6,c6
static asm u32 OSi_GetPRParam6(void) {
    mrc p15, 0, r0, c6, c6, 0
    bx lr
}

// Assembly in the SDK: reads region 7's register, c6,c7
static asm u32 OSi_GetPRParam7(void) {
    mrc p15, 0, r0, c6, c7, 0
    bx lr
}
// clang-format on

// NitroSDK's OS_SetProtectionRegion: sets a region to the address, rounded down to a multiple of
// the size, and enables it. The size is the register's size field in place, as OS_PROTECTION_SIZE_* give it
void OS_SetProtectionRegion(int regionNo, u32 address, u32 size) {
    OS_SetProtectionRegionParam(regionNo,
                                (address & (0xfffff000 << ((size - OS_PROTECTION_SIZE_4KB) >> 1))) | size | 1);
}
