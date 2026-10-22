// NitroSDK's mi_memory.c: the CPU copies, fills and compare of the MI library. The SDK wrote them all in assembly
// (MI_Zero36B, the last, in Thumb), so each is an MWCC asm function. The names are the SDK's, and mi.h gives swan's.
#include "nitro/mi.h"

// clang-format off
#pragma thumb off

// Fills size bytes with a halfword at a time
asm void MIi_CpuClear16(u16 value, void *dest, u32 size) {
    mov r3, #0
@loop:
    cmp r3, r2
    strlth r0, [r1, r3]
    addlt r3, r3, #2
    blt @loop
    bx lr
}

// Copies size bytes a halfword at a time
asm void MIi_CpuCopy16(const void *src, void *dest, u32 size) {
    mov r12, #0
@loop:
    cmp r12, r2
    ldrlth r3, [r0, r12]
    strlth r3, [r1, r12]
    addlt r12, r12, #2
    blt @loop
    bx lr
}

// Fills size bytes with a word at a time
asm void MIi_CpuClear32(u32 value, void *dest, u32 size) {
    add r12, r1, r2
@loop:
    cmp r1, r12
    stmltia r1!, {r0}
    blt @loop
    bx lr
}

// Copies size bytes a word at a time
asm void MIi_CpuCopy32(const void *src, void *dest, u32 size) {
    add r12, r1, r2
@loop:
    cmp r1, r12
    ldmltia r0!, {r2}
    stmltia r1!, {r2}
    blt @loop
    bx lr
}

// Sends size bytes, a word at a time, to one address, such as a FIFO port
asm void MIi_CpuSend32(const void *src, volatile void *dest, u32 size) {
    add r12, r0, r2
@loop:
    cmp r0, r12
    ldmltia r0!, {r2}
    strlt r2, [r1]
    blt @loop
    bx lr
}

// Fills size bytes, a multiple of 4, with a word, 32 bytes at a time while it can
asm void MIi_CpuClearFast(u32 value, void *dest, u32 size) {
    stmdb sp!, {r4, r5, r6, r7, r8, r9}
    add r9, r1, r2
    mov r12, r2, lsr #5
    add r12, r1, r12, lsl #5
    mov r2, r0
    mov r3, r2
    mov r4, r2
    mov r5, r2
    mov r6, r2
    mov r7, r2
    mov r8, r2
@block_loop:
    cmp r1, r12
    stmltia r1!, {r0, r2, r3, r4, r5, r6, r7, r8}
    blt @block_loop
@word_loop:
    cmp r1, r9
    stmltia r1!, {r0}
    blt @word_loop
    ldmia sp!, {r4, r5, r6, r7, r8, r9}
    bx lr
}

// Copies size bytes, a multiple of 4, 32 bytes at a time while it can
asm void MIi_CpuCopyFast(const void *src, void *dest, u32 size) {
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10}
    add r10, r1, r2
    mov r12, r2, lsr #5
    add r12, r1, r12, lsl #5
@block_loop:
    cmp r1, r12
    ldmltia r0!, {r2, r3, r4, r5, r6, r7, r8, r9}
    stmltia r1!, {r2, r3, r4, r5, r6, r7, r8, r9}
    blt @block_loop
@word_loop:
    cmp r1, r10
    ldmltia r0!, {r2}
    stmltia r1!, {r2}
    blt @word_loop
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10}
    bx lr
}

// MI_Copy32B (swan's gfxUploadFogTable): copies 32 bytes, as big as the fog table
asm void MI_Copy32B(const void *src, void *dest) {
    ldmia r0!, {r2, r3, r12}
    stmia r1!, {r2, r3, r12}
    ldmia r0!, {r2, r3, r12}
    stmia r1!, {r2, r3, r12}
    ldmia r0!, {r2, r3}
    stmia r1!, {r2, r3}
    bx lr
}

// MI_Copy36B: copies 36 bytes, a 3x3 matrix
asm void MI_Copy36B(const void *src, void *dest) {
    ldmia r0!, {r2, r3, r12}
    stmia r1!, {r2, r3, r12}
    ldmia r0!, {r2, r3, r12}
    stmia r1!, {r2, r3, r12}
    ldmia r0!, {r2, r3, r12}
    stmia r1!, {r2, r3, r12}
    bx lr
}

// MI_Copy48B: copies 48 bytes, a 4x3 matrix
asm void MI_Copy48B(const void *src, void *dest) {
    ldmia r0!, {r2, r3, r12}
    stmia r1!, {r2, r3, r12}
    ldmia r0!, {r2, r3, r12}
    stmia r1!, {r2, r3, r12}
    ldmia r0!, {r2, r3, r12}
    stmia r1!, {r2, r3, r12}
    ldmia r0!, {r2, r3, r12}
    stmia r1!, {r2, r3, r12}
    bx lr
}

// MI_Copy64B: copies 64 bytes, a 4x4 matrix
asm void MI_Copy64B(const void *src, void *dest) {
    ldmia r0!, {r2, r3, r12}
    stmia r1!, {r2, r3, r12}
    ldmia r0!, {r2, r3, r12}
    stmia r1!, {r2, r3, r12}
    ldmia r0!, {r2, r3, r12}
    stmia r1!, {r2, r3, r12}
    ldmia r0!, {r2, r3, r12}
    stmia r1!, {r2, r3, r12}
    ldmia r0, {r0, r2, r3, r12}
    stmia r1!, {r0, r2, r3, r12}
    bx lr
}

// MI_CpuFill8: fills size bytes with a byte, writing halfwords and words only: the ARM9 can't write a byte to
// VRAM, so an odd start or end is a halfword that keeps the byte next to it
asm void MI_CpuFill8(void *dest, u8 value, u32 size) {
    cmp r2, #0
    bxeq lr
    tst r0, #1
    beq @halfwords
    ldrh r12, [r0, #-1]
    and r12, r12, #0xff
    orr r3, r12, r1, lsl #8
    strh r3, [r0, #-1]
    add r0, r0, #1
    subs r2, r2, #1
    bxeq lr
@halfwords:
    cmp r2, #2
    blo @tail_byte
    orr r1, r1, r1, lsl #8
    tst r0, #2
    beq @words
    strh r1, [r0], #2
    subs r2, r2, #2
    bxeq lr
@words:
    orr r1, r1, r1, lsl #16
    bics r3, r2, #3
    beq @tail_halfword
    sub r2, r2, r3
    add r12, r3, r0
@word_loop:
    str r1, [r0], #4
    cmp r0, r12
    blo @word_loop
@tail_halfword:
    tst r2, #2
    strneh r1, [r0], #2
@tail_byte:
    tst r2, #1
    bxeq lr
    ldrh r3, [r0]
    and r3, r3, #0xff00
    and r1, r1, #0xff
    orr r1, r1, r3
    strh r1, [r0]
    bx lr
}

// MI_CpuFill: fills size bytes with a byte. Up to 8 bytes are an unrolled run that a computed jump enters; more are
// bytes up to a word boundary, then words, in blocks of 32 bytes where it can
asm void MI_CpuFill(void *dest, u32 value, u32 size) {
    cmp r2, #0
    bxeq lr
    cmp r2, #8
    bgt @large
    rsb r3, r2, #8
    add pc, pc, r3, lsl #2
    mov r0, r0
    strb r1, [r0], #1
    strb r1, [r0], #1
    strb r1, [r0], #1
    strb r1, [r0], #1
    strb r1, [r0], #1
    strb r1, [r0], #1
    strb r1, [r0], #1
    strb r1, [r0], #1
    bx lr
@large:
    orr r1, r1, r1, lsl #8
    orr r1, r1, r1, lsl #16
    tst r0, #1
    subne r2, r2, #1
    strneb r1, [r0], #1
    tst r0, #2
    subne r2, r2, #2
    strneh r1, [r0], #2
    tst r0, #4
    subne r2, r2, #4
    strne r1, [r0], #4
    cmp r2, #0x20
    blt @words
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10}
    mov r4, r1
    mov r5, r1
    mov r6, r1
    mov r7, r1
    mov r8, r1
    mov r9, r1
    mov r10, r1
    subs r2, r2, #0x20
@block_loop:
    stmgeia r0!, {r1, r4, r5, r6, r7, r8, r9, r10}
    subges r2, r2, #0x20
    bge @block_loop
    add r2, r2, #0x20
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10}
@words:
    cmp r2, #4
    blt @bytes
    subs r2, r2, #4
@word_loop:
    strge r1, [r0], #4
    subs r2, r2, #4
    bge @word_loop
    add r2, r2, #4
@bytes:
    subs r2, r2, #1
    strgeb r1, [r0], #1
    subges r2, r2, #1
    strgeb r1, [r0], #1
    subges r2, r2, #1
    strgeb r1, [r0], #1
    bx lr
}

// MI_CpuCopy8: copies size bytes, writing halfwords and words only (see MI_CpuFill8), whatever the alignment of the two
// sides
asm void MI_CpuCopy8(const void *src, void *dest, u32 size) {
    cmp r2, #0
    bxeq lr
    tst r1, #1
    beq @dest_even
    ldrh r12, [r1, #-1]
    and r12, r12, #0xff
    tst r0, #1
    ldrneh r3, [r0, #-1]
    movne r3, r3, lsr #8
    ldreqh r3, [r0]
    orr r3, r12, r3, lsl #8
    strh r3, [r1, #-1]
    add r0, r0, #1
    add r1, r1, #1
    subs r2, r2, #1
    bxeq lr
@dest_even:
    eor r12, r1, r0
    tst r12, #1
    beq @same_parity
    bic r0, r0, #1
    ldrh r12, [r0], #2
    mov r3, r12, lsr #8
    subs r2, r2, #2
    blo @shift_tail
@shift_loop:
    ldrh r12, [r0], #2
    orr r12, r3, r12, lsl #8
    strh r12, [r1], #2
    mov r3, r12, lsr #0x10
    subs r2, r2, #2
    bhs @shift_loop
@shift_tail:
    tst r2, #1
    bxeq lr
    ldrh r12, [r1]
    and r12, r12, #0xff00
    orr r12, r12, r3
    strh r12, [r1]
    bx lr
@same_parity:
    tst r12, #2
    beq @same_word_offset
    bics r3, r2, #1
    beq @tail_byte
    sub r2, r2, r3
    add r12, r3, r1
@halfword_loop:
    ldrh r3, [r0], #2
    strh r3, [r1], #2
    cmp r1, r12
    blo @halfword_loop
    b @tail_byte
@same_word_offset:
    cmp r2, #2
    blo @tail_byte
    tst r1, #2
    beq @words
    ldrh r3, [r0], #2
    strh r3, [r1], #2
    subs r2, r2, #2
    bxeq lr
@words:
    bics r3, r2, #3
    beq @tail_halfword
    sub r2, r2, r3
    add r12, r3, r1
@word_loop:
    ldr r3, [r0], #4
    str r3, [r1], #4
    cmp r1, r12
    blo @word_loop
@tail_halfword:
    tst r2, #2
    ldrneh r3, [r0], #2
    strneh r3, [r1], #2
@tail_byte:
    tst r2, #1
    bxeq lr
    ldrh r2, [r1]
    ldrh r0, [r0]
    and r2, r2, #0xff00
    and r0, r0, #0xff
    orr r0, r2, r0
    strh r0, [r1]
    bx lr
}

// Compares size bytes, returning the difference of the first that differ, or 0
asm int MI_CpuComp8(const void *a, const void *b, u32 size) {
    add r12, r0, r2
    cmp r0, r12
    bhs @equal
@loop:
    ldrb r3, [r0], #1
    ldrb r2, [r1], #1
    subs r2, r3, r2
    movne r0, r2
    bxne lr
    cmp r0, r12
    blo @loop
@equal:
    mov r0, #0
    bx lr
}

// MI_CpuCopy: copies size bytes forwards by words, whatever the alignment of the two sides. Up to 8 bytes are an
// unrolled run that a computed jump enters. Otherwise the source is brought to a word with bytes, and the destination's
// offset in its word chooses one of four loops: 32 bytes at a time, then 4, with the words shifted to fit
asm void MI_CpuCopy(const void *src, void *dest, u32 size) {
    cmp r2, #8
    bgt @large
    rsb r3, r2, #8
    add pc, pc, r3, lsl #3
    mov r0, r0
    ldrb r3, [r0], #1
    strb r3, [r1], #1
    ldrb r3, [r0], #1
    strb r3, [r1], #1
    ldrb r3, [r0], #1
    strb r3, [r1], #1
    ldrb r3, [r0], #1
    strb r3, [r1], #1
    ldrb r3, [r0], #1
    strb r3, [r1], #1
    ldrb r3, [r0], #1
    strb r3, [r1], #1
    ldrb r3, [r0], #1
    strb r3, [r1], #1
    ldrb r3, [r0], #1
    strb r3, [r1], #1
    bx lr
@large:
    tst r0, #1
    subne r2, r2, #1
    ldrneb r3, [r0], #1
    strneb r3, [r1], #1
    tst r0, #2
    subne r2, r2, #2
    ldrneb r3, [r0], #1
    strneb r3, [r1], #1
    ldrneb r3, [r0], #1
    strneb r3, [r1], #1
    and r3, r1, #3
    bic r1, r1, #3
    cmp r3, #0
    beq @dest_offset_0
    cmp r3, #1
    beq @dest_offset_1
    cmp r3, #2
    beq @dest_offset_2
    b @dest_offset_3
@dest_offset_0:
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #0x20
@dest_offset_0_block_loop:
    ldmgeia r0!, {r4, r5, r6, r7, r8, r9, r10, lr}
    stmgeia r1!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subges r2, r2, #0x20
    bge @dest_offset_0_block_loop
    add r2, r2, #0x20
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #4
@dest_offset_0_word_loop:
    ldrge r3, [r0], #4
    strge r3, [r1], #4
    subges r2, r2, #4
    bge @dest_offset_0_word_loop
    add r2, r2, #4
    b @tail
@dest_offset_1:
    ldr r12, [r1]
    mov r12, r12, lsl #0x18
    mov r12, r12, lsr #0x18
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #0x20
@dest_offset_1_block_loop:
    ldmgeia r0!, {r4, r5, r6, r7, r8, r9, r10, lr}
    movge r3, r4, lsr #0x18
    orrge r4, r12, r4, lsl #8
    movge r12, r5, lsr #0x18
    orrge r5, r3, r5, lsl #8
    movge r3, r6, lsr #0x18
    orrge r6, r12, r6, lsl #8
    movge r12, r7, lsr #0x18
    orrge r7, r3, r7, lsl #8
    movge r3, r8, lsr #0x18
    orrge r8, r12, r8, lsl #8
    movge r12, r9, lsr #0x18
    orrge r9, r3, r9, lsl #8
    movge r3, r10, lsr #0x18
    orrge r10, r12, r10, lsl #8
    movge r12, lr, lsr #0x18
    orrge lr, r3, lr, lsl #8
    stmgeia r1!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subges r2, r2, #0x20
    bge @dest_offset_1_block_loop
    add r2, r2, #0x20
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #4
@dest_offset_1_word_loop:
    ldrge r3, [r0], #4
    orrge r12, r12, r3, lsl #8
    strge r12, [r1], #4
    movge r12, r3, lsr #0x18
    subges r2, r2, #4
    bge @dest_offset_1_word_loop
    add r2, r2, #4
    sub r0, r0, #1
    add r2, r2, #1
    b @tail
@dest_offset_2:
    ldr r12, [r1]
    mov r12, r12, lsl #0x10
    mov r12, r12, lsr #0x10
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #0x20
@dest_offset_2_block_loop:
    ldmgeia r0!, {r4, r5, r6, r7, r8, r9, r10, lr}
    movge r3, r4, lsr #0x10
    orrge r4, r12, r4, lsl #16
    movge r12, r5, lsr #0x10
    orrge r5, r3, r5, lsl #16
    movge r3, r6, lsr #0x10
    orrge r6, r12, r6, lsl #16
    movge r12, r7, lsr #0x10
    orrge r7, r3, r7, lsl #16
    movge r3, r8, lsr #0x10
    orrge r8, r12, r8, lsl #16
    movge r12, r9, lsr #0x10
    orrge r9, r3, r9, lsl #16
    movge r3, r10, lsr #0x10
    orrge r10, r12, r10, lsl #16
    movge r12, lr, lsr #0x10
    orrge lr, r3, lr, lsl #16
    stmgeia r1!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subges r2, r2, #0x20
    bge @dest_offset_2_block_loop
    add r2, r2, #0x20
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #4
@dest_offset_2_word_loop:
    ldrge r3, [r0], #4
    orrge r12, r12, r3, lsl #16
    strge r12, [r1], #4
    movge r12, r3, lsr #0x10
    subges r2, r2, #4
    bge @dest_offset_2_word_loop
    add r2, r2, #4
    sub r0, r0, #2
    add r2, r2, #2
    b @tail
@dest_offset_3:
    ldr r12, [r1]
    mov r12, r12, lsl #8
    mov r12, r12, lsr #8
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #0x20
@dest_offset_3_block_loop:
    ldmgeia r0!, {r4, r5, r6, r7, r8, r9, r10, lr}
    movge r3, r4, lsr #8
    orrge r4, r12, r4, lsl #24
    movge r12, r5, lsr #8
    orrge r5, r3, r5, lsl #24
    movge r3, r6, lsr #8
    orrge r6, r12, r6, lsl #24
    movge r12, r7, lsr #8
    orrge r7, r3, r7, lsl #24
    movge r3, r8, lsr #8
    orrge r8, r12, r8, lsl #24
    movge r12, r9, lsr #8
    orrge r9, r3, r9, lsl #24
    movge r3, r10, lsr #8
    orrge r10, r12, r10, lsl #24
    movge r12, lr, lsr #8
    orrge lr, r3, lr, lsl #24
    stmgeia r1!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subges r2, r2, #0x20
    bge @dest_offset_3_block_loop
    add r2, r2, #0x20
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #4
@dest_offset_3_word_loop:
    ldrge r3, [r0], #4
    orrge r12, r12, r3, lsl #24
    strge r12, [r1], #4
    movge r12, r3, lsr #8
    subges r2, r2, #4
    bge @dest_offset_3_word_loop
    add r2, r2, #4
    sub r0, r0, #3
    add r2, r2, #3
    b @tail
@tail:
    tst r2, #4
    ldrneb r3, [r0], #1
    strneb r3, [r1], #1
    ldrneb r3, [r0], #1
    strneb r3, [r1], #1
    ldrneb r3, [r0], #1
    strneb r3, [r1], #1
    ldrneb r3, [r0], #1
    strneb r3, [r1], #1
    tst r2, #2
    ldrneb r3, [r0], #1
    strneb r3, [r1], #1
    ldrneb r3, [r0], #1
    strneb r3, [r1], #1
    tst r2, #1
    ldrneb r3, [r0], #1
    strneb r3, [r1], #1
    bx lr
}

// MI_CpuMove: copies size bytes, even when the ranges overlap. Where the destination is below the source it is
// MI_CpuCopy; otherwise it copies backwards from the ends, in the same four ways
asm void MI_CpuMove(const void *src, void *dest, u32 size) {
    cmp r2, #0
    subnes r3, r0, r1
    bxeq lr
    bgt MI_CpuCopy ; EXTERNAL
    add r1, r1, r2
    add r0, r0, r2
    cmp r2, #8
    bgt @large
    rsb r3, r2, #8
    add pc, pc, r3, lsl #3
    mov r0, r0
    ldrb r3, [r0, #-1]!
    strb r3, [r1, #-1]!
    ldrb r3, [r0, #-1]!
    strb r3, [r1, #-1]!
    ldrb r3, [r0, #-1]!
    strb r3, [r1, #-1]!
    ldrb r3, [r0, #-1]!
    strb r3, [r1, #-1]!
    ldrb r3, [r0, #-1]!
    strb r3, [r1, #-1]!
    ldrb r3, [r0, #-1]!
    strb r3, [r1, #-1]!
    ldrb r3, [r0, #-1]!
    strb r3, [r1, #-1]!
    ldrb r3, [r0, #-1]!
    strb r3, [r1, #-1]!
    bx lr
@large:
    tst r0, #2
    subne r2, r2, #2
    ldrneb r3, [r0, #-1]!
    strneb r3, [r1, #-1]!
    ldrneb r3, [r0, #-1]!
    strneb r3, [r1, #-1]!
    tst r0, #1
    subne r2, r2, #1
    ldrneb r3, [r0, #-1]!
    strneb r3, [r1, #-1]!
    and r3, r1, #3
    bic r1, r1, #3
    cmp r3, #0
    beq @end_offset_0
    cmp r3, #1
    beq @end_offset_1
    cmp r3, #2
    beq @end_offset_2
    b @end_offset_3
@end_offset_0:
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #0x20
@end_offset_0_block_loop:
    ldmgedb r0!, {r4, r5, r6, r7, r8, r9, r10, lr}
    stmgedb r1!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subges r2, r2, #0x20
    bge @end_offset_0_block_loop
    add r2, r2, #0x20
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #4
@end_offset_0_word_loop:
    ldrge r3, [r0, #-4]!
    strge r3, [r1, #-4]!
    subges r2, r2, #4
    bge @end_offset_0_word_loop
    add r2, r2, #4
    b @tail
@end_offset_1:
    ldr r12, [r1]
    mov r12, r12, lsr #8
    mov r12, r12, lsl #8
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #0x20
@end_offset_1_block_loop:
    ldmgedb r0!, {r4, r5, r6, r7, r8, r9, r10, lr}
    movge r3, lr, lsl #8
    orrge lr, r12, lr, lsr #24
    movge r12, r10, lsl #8
    orrge r10, r3, r10, lsr #24
    movge r3, r9, lsl #8
    orrge r9, r12, r9, lsr #24
    movge r12, r8, lsl #8
    orrge r8, r3, r8, lsr #24
    movge r3, r7, lsl #8
    orrge r7, r12, r7, lsr #24
    movge r12, r6, lsl #8
    orrge r6, r3, r6, lsr #24
    movge r3, r5, lsl #8
    orrge r5, r12, r5, lsr #24
    movge r12, r4, lsl #8
    orrge r4, r3, r4, lsr #24
    stmgeda r1!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subges r2, r2, #0x20
    bge @end_offset_1_block_loop
    add r2, r2, #0x20
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #4
@end_offset_1_word_loop:
    ldrge r3, [r0, #-4]!
    orrge r12, r12, r3, lsr #24
    strge r12, [r1], #-4
    movge r12, r3, lsl #8
    subges r2, r2, #4
    bge @end_offset_1_word_loop
    add r2, r2, #4
    add r1, r1, #4
    add r0, r0, #3
    add r2, r2, #3
    b @tail
@end_offset_2:
    ldr r12, [r1]
    mov r12, r12, lsr #0x10
    mov r12, r12, lsl #0x10
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #0x20
@end_offset_2_block_loop:
    ldmgedb r0!, {r4, r5, r6, r7, r8, r9, r10, lr}
    movge r3, lr, lsl #0x10
    orrge lr, r12, lr, lsr #16
    movge r12, r10, lsl #0x10
    orrge r10, r3, r10, lsr #16
    movge r3, r9, lsl #0x10
    orrge r9, r12, r9, lsr #16
    movge r12, r8, lsl #0x10
    orrge r8, r3, r8, lsr #16
    movge r3, r7, lsl #0x10
    orrge r7, r12, r7, lsr #16
    movge r12, r6, lsl #0x10
    orrge r6, r3, r6, lsr #16
    movge r3, r5, lsl #0x10
    orrge r5, r12, r5, lsr #16
    movge r12, r4, lsl #0x10
    orrge r4, r3, r4, lsr #16
    stmgeda r1!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subges r2, r2, #0x20
    bge @end_offset_2_block_loop
    add r2, r2, #0x20
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #4
@end_offset_2_word_loop:
    ldrge r3, [r0, #-4]!
    orrge r12, r12, r3, lsr #16
    strge r12, [r1], #-4
    movge r12, r3, lsl #0x10
    subges r2, r2, #4
    bge @end_offset_2_word_loop
    add r2, r2, #4
    add r1, r1, #4
    add r0, r0, #2
    add r2, r2, #2
    b @tail
@end_offset_3:
    ldr r12, [r1]
    mov r12, r12, lsr #0x18
    mov r12, r12, lsl #0x18
    stmdb sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #0x20
@end_offset_3_block_loop:
    ldmgedb r0!, {r4, r5, r6, r7, r8, r9, r10, lr}
    movge r3, lr, lsl #0x18
    orrge lr, r12, lr, lsr #8
    movge r12, r10, lsl #0x18
    orrge r10, r3, r10, lsr #8
    movge r3, r9, lsl #0x18
    orrge r9, r12, r9, lsr #8
    movge r12, r8, lsl #0x18
    orrge r8, r3, r8, lsr #8
    movge r3, r7, lsl #0x18
    orrge r7, r12, r7, lsr #8
    movge r12, r6, lsl #0x18
    orrge r6, r3, r6, lsr #8
    movge r3, r5, lsl #0x18
    orrge r5, r12, r5, lsr #8
    movge r12, r4, lsl #0x18
    orrge r4, r3, r4, lsr #8
    stmgeda r1!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subges r2, r2, #0x20
    bge @end_offset_3_block_loop
    add r2, r2, #0x20
    ldmia sp!, {r4, r5, r6, r7, r8, r9, r10, lr}
    subs r2, r2, #4
@end_offset_3_word_loop:
    ldrge r3, [r0, #-4]!
    orrge r12, r12, r3, lsr #8
    strge r12, [r1], #-4
    movge r12, r3, lsl #0x18
    subges r2, r2, #4
    bge @end_offset_3_word_loop
    add r2, r2, #4
    add r1, r1, #4
    add r0, r0, #1
    add r2, r2, #1
    b @tail
@tail:
    tst r2, #4
    ldrneb r3, [r0, #-1]!
    strneb r3, [r1, #-1]!
    ldrneb r3, [r0, #-1]!
    strneb r3, [r1, #-1]!
    ldrneb r3, [r0, #-1]!
    strneb r3, [r1, #-1]!
    ldrneb r3, [r0, #-1]!
    strneb r3, [r1, #-1]!
    tst r2, #2
    ldrneb r3, [r0, #-1]!
    strneb r3, [r1, #-1]!
    ldrneb r3, [r0, #-1]!
    strneb r3, [r1, #-1]!
    tst r2, #1
    ldrneb r3, [r0, #-1]!
    strneb r3, [r1, #-1]!
    bx lr

}

#pragma thumb on

// Clears 36 bytes
asm void MI_Zero36B(void *dest) {
    mov r1, #0
    mov r2, #0
    mov r3, #0
    stmia r0!, {r1, r2, r3}
    stmia r0!, {r1, r2, r3}
    stmia r0!, {r1, r2, r3}
    bx lr
}
// clang-format on
