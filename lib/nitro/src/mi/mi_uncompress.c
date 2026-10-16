// NitroSDK's mi_uncompress.c: the LZ77 decompressor, MI_UncompressLZ8, which writes bytes with SWPB, as the ARM9 needs
// to for VRAM. The SDK wrote it in assembly, so it is an MWCC asm function; the file's other decoders (run length,
// Huffman, the 16-bit LZ77 and the filters) were left out by the linker. The code accepts the DS's LZ77 header (type
// 0x10) and the extended format, whose copy lengths run to 0x10110 (type 0x11).
#include "nitro/mi.h"

// clang-format off
asm void sys_uncomp_lz1x(const void *src, void *dest) {
    stmdb sp!, {r4, r5, r6, r7, lr}
    // r2 counts the bytes left to write, from the header; r7 is set for the extended format
    ldr r5, [r0], #4
    mov r2, r5, lsr #8
    mov r7, #0
    tst r5, #0xf
    movne r7, #1
@flag_byte:
    // A flag byte tells eight blocks apart, the top bit first: a literal byte, or a reference to bytes already written
    cmp r2, #0
    ble @done
    ldrb lr, [r0], #1
    mov r4, #8
@flag_bit:
    subs r4, r4, #1
    blt @flag_byte
    tst lr, #0x80
    bne @reference
    ldrb r6, [r0], #1
    swpb r6, r6, [r1]
    add r1, r1, #1
    sub r2, r2, #1
    b @next_bit
@reference:
    // r6 becomes the base of the length, to which a high nibble is added. It is 3 in the plain format. In the extended
    // one it is 1 when the first byte's high nibble is 2 or more, and otherwise (0 or 1) the length takes two or three
    // bytes, with a base of 0x11 or 0x111
    ldrb r5, [r0]
    cmp r7, #0
    moveq r6, #3
    beq @got_length
    tst r5, #0xe0
    movne r6, #1
    bne @got_length
    add r0, r0, #1
    and r6, r5, #0xf
    mov r6, r6, lsl #4
    tst r5, #0x10
    beq @one_extra_byte
    mov r6, r6, lsl #8
    ldrb r5, [r0], #1
    add r6, r6, r5, lsl #4
    add r6, r6, #0x100
@one_extra_byte:
    add r6, r6, #0x11
    ldrb r5, [r0]
@got_length:
    // The length is the base plus the high nibble of the last byte of it; the offset is that byte's low nibble and the
    // next byte, plus 1
    add r3, r6, r5, asr #4
    add r0, r0, #1
    and r5, r5, #0xf
    mov r12, r5, lsl #8
    ldrb r6, [r0], #1
    orr r5, r6, r12
    add r12, r5, #1
    sub r2, r2, r3
@copy_loop:
    ldrb r5, [r1, -r12]
    swpb r5, r5, [r1]
    add r1, r1, #1
    subs r3, r3, #1
    bgt @copy_loop
@next_bit:
    cmp r2, #0
    movgt lr, lr, lsl #1
    bgt @flag_bit
    b @flag_byte
@done:
    ldmia sp!, {r4, r5, r6, r7, lr}
    bx lr
}
// clang-format on
