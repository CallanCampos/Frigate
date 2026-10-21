// NitroSDK's system call library (libsyscall.a) as this game has it: the 2 KB at the start of ARM9 main, which the
// card's secure area holds. Nintendo gives each game its own build of the library: the BIOS call stubs (Thumb, each
// one SWI, which the SDK wrote in assembly and are MWCC asm functions here) lie at places of their own among filler
// bytes, so Black 2 and White 2 have them in different orders. The file name is descriptive, since the library's
// object name isn't known; the names are NitroSDK's, with swan's in nitro/svc.h.
//
// The file isn't linked: the filler bytes are the base ROM's, which no source gives and which aren't written here, and
// MWCC starts each function at a multiple of 4 bytes, where most of the stubs start 2 bytes past one.
#include "nitro/svc.h"

#ifdef BLACK2

// SVC_UnpackBits: BIOS call 0x10 (0x02004064)
// clang-format off
asm void SVC_UnpackBits(const void *src, void *dest, const void *param) {
    swi 0x10
    bx lr
}

// clang-format on

// SVC_UncompressLZ8: BIOS call 0x11 (0x020040d0)
// clang-format off
asm void SVC_UncompressLZ8(const void *src, void *dest) {
    swi 0x11
    bx lr
}

// clang-format on

// SVC_GetCRC16: BIOS call 0x0e (0x0200413a)
// clang-format off
asm u16 SVC_GetCRC16(u16 start, const void *data, u32 length) {
    swi 0xe
    bx lr
}

// clang-format on

// SVC_CpuSet: BIOS call 0x0b (0x02004190)
// clang-format off
asm void SVC_CpuSet(const void *src, void *dest, u32 mode) {
    swi 0xb
    bx lr
}

// clang-format on

// SVC_UncompressRL8: BIOS call 0x14 (0x020041f0)
// clang-format off
asm void SVC_UncompressRL8(const void *src, void *dest) {
    swi 0x14
    bx lr
}

// clang-format on

// SVC_UncompressLZ16FromDevice: BIOS call 0x12 (0x0200426e)
// clang-format off
asm s32 SVC_UncompressLZ16FromDevice(const void *src, void *dest, const void *param, const void *callbacks) {
    swi 0x12
    bx lr
}

// clang-format on

// SVC_Div: BIOS call 0x09, which leaves the quotient in r0 (0x020042e4)
// clang-format off
asm s32 SVC_Div(s32 number, s32 denom) {
    swi 0x9
    bx lr
}

// clang-format on

// SVC_WaitVBlankIntr: BIOS call 0x05, with r2 = 0 (0x02004352)
// clang-format off
asm void SVC_WaitVBlankIntr(void) {
    mov r2, #0x0
    swi 0x5
    bx lr
}

// clang-format on

// SVC_IsMmemExpanded: BIOS call 0x0f (0x020043ca)
// clang-format off
asm BOOL SVC_IsMmemExpanded(void) {
    swi 0xf
    bx lr
}

// clang-format on

// SVC_UncompressHuffmanFromDevice: BIOS call 0x13 (0x02004424)
// clang-format off
asm s32 SVC_UncompressHuffmanFromDevice(const void *src, void *dest, u8 *tree, const void *callbacks) {
    swi 0x13
    bx lr
}

// clang-format on

// SVC_SoftReset: BIOS call 0x00 (0x0200448a)
// clang-format off
asm void SVC_SoftReset(void) {
    swi 0x0
    bx lr
}

// clang-format on

// SVC_WaitByLoop: BIOS call 0x03 (0x020044f6)
// clang-format off
asm void SVC_WaitByLoop(u32 cycles) {
    swi 0x3
    bx lr
}

// clang-format on

// SVC_Sqrt: BIOS call 0x0d (0x02004582)
// clang-format off
asm u16 SVC_Sqrt(u32 value) {
    swi 0xd
    bx lr
}

// clang-format on

// SVC_CpuSetFast: BIOS call 0x0c (0x020045f4)
// clang-format off
asm void SVC_CpuSetFast(const void *src, void *dest, u32 mode) {
    swi 0xc
    bx lr
}

// clang-format on

// SVC_Halt: BIOS call 0x06 (0x02004666)
// clang-format off
asm void SVC_Halt(void) {
    swi 0x6
    bx lr
}

// clang-format on

// SVC_DivRem: BIOS call 0x09, returning the remainder that it leaves in r1 (0x020046ba)
// clang-format off
asm s32 SVC_DivRem(s32 number, s32 denom) {
    swi 0x9
    mov r0, r1
    bx lr
}

// clang-format on

// SVC_UncompressRL16FromDevice: BIOS call 0x15 (0x02004726)
// clang-format off
asm s32 SVC_UncompressRL16FromDevice(const void *src, void *dest, const void *param, const void *callbacks) {
    swi 0x15
    bx lr
}

// clang-format on

// SVC_WaitIntr: BIOS call 0x04 (0x0200478e)
// clang-format off
asm void SVC_WaitIntr(BOOL clear, u32 irqFlags) {
    swi 0x4
    bx lr
}

// clang-format on

#else

// SVC_GetCRC16: BIOS call 0x0e (0x0200406a)
// clang-format off
asm u16 SVC_GetCRC16(u16 start, const void *data, u32 length) {
    swi 0xe
    bx lr
}

// clang-format on

// SVC_WaitIntr: BIOS call 0x04 (0x020040e4)
// clang-format off
asm void SVC_WaitIntr(BOOL clear, u32 irqFlags) {
    swi 0x4
    bx lr
}

// clang-format on

// SVC_Halt: BIOS call 0x06 (0x02004164)
// clang-format off
asm void SVC_Halt(void) {
    swi 0x6
    bx lr
}

// clang-format on

// SVC_IsMmemExpanded: BIOS call 0x0f (0x020041cc)
// clang-format off
asm BOOL SVC_IsMmemExpanded(void) {
    swi 0xf
    bx lr
}

// clang-format on

// SVC_UncompressRL8: BIOS call 0x14 (0x02004224)
// clang-format off
asm void SVC_UncompressRL8(const void *src, void *dest) {
    swi 0x14
    bx lr
}

// clang-format on

// SVC_WaitByLoop: BIOS call 0x03 (0x02004280)
// clang-format off
asm void SVC_WaitByLoop(u32 cycles) {
    swi 0x3
    bx lr
}

// clang-format on

// SVC_CpuSet: BIOS call 0x0b (0x020042e6)
// clang-format off
asm void SVC_CpuSet(const void *src, void *dest, u32 mode) {
    swi 0xb
    bx lr
}

// clang-format on

// SVC_SoftReset: BIOS call 0x00 (0x02004350)
// clang-format off
asm void SVC_SoftReset(void) {
    swi 0x0
    bx lr
}

// clang-format on

// SVC_Sqrt: BIOS call 0x0d (0x020043a8)
// clang-format off
asm u16 SVC_Sqrt(u32 value) {
    swi 0xd
    bx lr
}

// clang-format on

// SVC_UnpackBits: BIOS call 0x10 (0x02004414)
// clang-format off
asm void SVC_UnpackBits(const void *src, void *dest, const void *param) {
    swi 0x10
    bx lr
}

// clang-format on

// SVC_WaitVBlankIntr: BIOS call 0x05, with r2 = 0 (0x02004484)
// clang-format off
asm void SVC_WaitVBlankIntr(void) {
    mov r2, #0x0
    swi 0x5
    bx lr
}

// clang-format on

// SVC_UncompressLZ16FromDevice: BIOS call 0x12 (0x020044ea)
// clang-format off
asm s32 SVC_UncompressLZ16FromDevice(const void *src, void *dest, const void *param, const void *callbacks) {
    swi 0x12
    bx lr
}

// clang-format on

// SVC_DivRem: BIOS call 0x09, returning the remainder that it leaves in r1 (0x02004554)
// clang-format off
asm s32 SVC_DivRem(s32 number, s32 denom) {
    swi 0x9
    mov r0, r1
    bx lr
}

// clang-format on

// SVC_UncompressRL16FromDevice: BIOS call 0x15 (0x020045ae)
// clang-format off
asm s32 SVC_UncompressRL16FromDevice(const void *src, void *dest, const void *param, const void *callbacks) {
    swi 0x15
    bx lr
}

// clang-format on

// SVC_UncompressHuffmanFromDevice: BIOS call 0x13 (0x02004632)
// clang-format off
asm s32 SVC_UncompressHuffmanFromDevice(const void *src, void *dest, u8 *tree, const void *callbacks) {
    swi 0x13
    bx lr
}

// clang-format on

// SVC_UncompressLZ8: BIOS call 0x11 (0x020046aa)
// clang-format off
asm void SVC_UncompressLZ8(const void *src, void *dest) {
    swi 0x11
    bx lr
}

// clang-format on

// SVC_Div: BIOS call 0x09, which leaves the quotient in r0 (0x02004712)
// clang-format off
asm s32 SVC_Div(s32 number, s32 denom) {
    swi 0x9
    bx lr
}

// clang-format on

// SVC_CpuSetFast: BIOS call 0x0c (0x02004778)
// clang-format off
asm void SVC_CpuSetFast(const void *src, void *dest, u32 mode) {
    swi 0xc
    bx lr
}

// clang-format on

#endif
