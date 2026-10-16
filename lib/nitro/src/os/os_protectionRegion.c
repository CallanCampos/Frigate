// NitroSDK OS: the memory protection unit's regions (os_protectionRegion.c). ARM code. Each region's register is
// reached through its own function, since only an instruction can name a coprocessor register; the functions are in
// tables that the region numbers index.
#include "nitro/os.h"

static void func_0207b7d0(int regionNo, u32 param);
static asm void mprotect_region0(u32 param);
static asm void func_0207b7f4(u32 param);
static asm void func_0207b7fc(u32 param);
static asm void func_0207b804(u32 param);
static asm void func_0207b80c(u32 param);
static asm void func_0207b814(u32 param);
static asm void func_0207b81c(u32 param);
static asm void func_0207b824(u32 param);
static asm u32 func_0207b82c(void);
static asm u32 func_0207b834(void);
static asm u32 func_0207b83c(void);
static asm u32 func_0207b844(void);
static asm u32 func_0207b84c(void);
static asm u32 func_0207b854(void);
static asm u32 func_0207b85c(void);
static asm u32 func_0207b864(void);

// The setters of the eight regions' registers, then their getters (the ones for NitroSDK's
// OS_GetProtectionRegionParam, which nothing links)
static void (*OSi_SetPRFuncs[8])(u32 param) = {
    mprotect_region0, func_0207b7f4, func_0207b7fc, func_0207b804,
    func_0207b80c,    func_0207b814, func_0207b81c, func_0207b824,
};
static u32 (*OSi_GetPRFuncs[8])(void) = {
    func_0207b82c, func_0207b834, func_0207b83c, func_0207b844,
    func_0207b84c, func_0207b854, func_0207b85c, func_0207b864,
};

// NitroSDK's OS_SetProtectionRegionParam: writes the register of a region (address, size and enable bit) through the
// function for it
static void func_0207b7d0(int regionNo, u32 param) {
    OSi_SetPRFuncs[regionNo](param);
}

// clang-format off
// Assembly in the SDK: the setters and getters of coprocessor 15's registers c6,c0 to c6,c7, the regions'
// registers
static asm void mprotect_region0(u32 param) {
    mcr p15, 0, r0, c6, c0, 0
    bx lr
}

static asm void func_0207b7f4(u32 param) {
    mcr p15, 0, r0, c6, c1, 0
    bx lr
}

static asm void func_0207b7fc(u32 param) {
    mcr p15, 0, r0, c6, c2, 0
    bx lr
}

static asm void func_0207b804(u32 param) {
    mcr p15, 0, r0, c6, c3, 0
    bx lr
}

static asm void func_0207b80c(u32 param) {
    mcr p15, 0, r0, c6, c4, 0
    bx lr
}

static asm void func_0207b814(u32 param) {
    mcr p15, 0, r0, c6, c5, 0
    bx lr
}

static asm void func_0207b81c(u32 param) {
    mcr p15, 0, r0, c6, c6, 0
    bx lr
}

static asm void func_0207b824(u32 param) {
    mcr p15, 0, r0, c6, c7, 0
    bx lr
}

static asm u32 func_0207b82c(void) {
    mrc p15, 0, r0, c6, c0, 0
    bx lr
}

static asm u32 func_0207b834(void) {
    mrc p15, 0, r0, c6, c1, 0
    bx lr
}

static asm u32 func_0207b83c(void) {
    mrc p15, 0, r0, c6, c2, 0
    bx lr
}

static asm u32 func_0207b844(void) {
    mrc p15, 0, r0, c6, c3, 0
    bx lr
}

static asm u32 func_0207b84c(void) {
    mrc p15, 0, r0, c6, c4, 0
    bx lr
}

static asm u32 func_0207b854(void) {
    mrc p15, 0, r0, c6, c5, 0
    bx lr
}

static asm u32 func_0207b85c(void) {
    mrc p15, 0, r0, c6, c6, 0
    bx lr
}

static asm u32 func_0207b864(void) {
    mrc p15, 0, r0, c6, c7, 0
    bx lr
}
// clang-format on

// NitroSDK's OS_SetProtectionRegion, under swan's name: sets a region to the address, rounded down to a multiple of
// the size, and enables it. The size is the register's size field in place, as OS_PROTECTION_SIZE_* give it
void SetMemoryProtectionRegion(int regionNo, u32 address, u32 size) {
    func_0207b7d0(regionNo, (address & (0xfffff000 << ((size - OS_PROTECTION_SIZE_4KB) >> 1))) | size | 1);
}
