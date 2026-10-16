// NitroSDK OS: the ARM946E-S's cache operations (os_cache.c, the SDK's name for the module). ARM code.
// All of them are assembly in the SDK, since only a CP15 instruction cleans or invalidates a cache line.
#include "nitro/os.h"

// clang-format off

// NitroSDK's DC_FlushAll: for each of the four segments and 32 lines of the data cache, cleans the
// line to memory (after draining the write buffer) and invalidates it
asm void DC_FlushAll(void) {
    mov ip, #0
    mov r1, #0
@segment:
    mov r0, #0
@line:
    orr r2, r1, r0
    mcr p15, 0, ip, c7, c10, 4
    mcr p15, 0, r2, c7, c14, 2
    add r0, r0, #0x20
    cmp r0, #0x400
    blt @line
    add r1, r1, #0x40000000
    cmp r1, #0
    bne @segment
    bx lr
}

// NitroSDK's DC_InvalidateRange: invalidates the lines of the data cache that hold the range
asm void DC_InvalidateRange(void *addr, u32 size) {
    add r1, r1, r0
    bic r0, r0, #0x1f
@loop:
    mcr p15, 0, r0, c7, c6, 1
    add r0, r0, #0x20
    cmp r0, r1
    blt @loop
    bx lr
}

// NitroSDK's DC_StoreRange: cleans the lines that hold the range to memory
asm void DC_StoreRange(const void *addr, u32 size) {
    add r1, r1, r0
    bic r0, r0, #0x1f
@loop:
    mcr p15, 0, r0, c7, c10, 1
    add r0, r0, #0x20
    cmp r0, r1
    blt @loop
    bx lr
}

// NitroSDK's DC_FlushRange: cleans and invalidates the lines that hold the range, draining the write
// buffer before each
asm void DC_FlushRange(const void *addr, u32 size) {
    mov ip, #0
    add r1, r1, r0
    bic r0, r0, #0x1f
@loop:
    mcr p15, 0, ip, c7, c10, 4
    mcr p15, 0, r0, c7, c14, 1
    add r0, r0, #0x20
    cmp r0, r1
    blt @loop
    bx lr
}

// NitroSDK's DC_WaitWriteBufferEmpty
asm void DC_WaitWriteBufferEmpty(void) {
    mov r0, #0
    mcr p15, 0, r0, c7, c10, 4
    bx lr
}

// NitroSDK's IC_InvalidateAll
asm void IC_InvalidateAll(void) {
    mov r0, #0
    mcr p15, 0, r0, c7, c5, 0
    bx lr
}

// NitroSDK's IC_InvalidateRange: invalidates the lines of the instruction cache that hold the range
asm void IC_InvalidateRange(void *addr, u32 size) {
    add r1, r1, r0
    bic r0, r0, #0x1f
@loop:
    mcr p15, 0, r0, c7, c5, 1
    add r0, r0, #0x20
    cmp r0, r1
    blt @loop
    bx lr
}
// clang-format on
