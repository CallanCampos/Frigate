// NitroSDK libmath's math.c (the file name is a guess): MATH_QSort.
// The split from math_crc.c is certain; whether sha1_block.c is a separate file after it is not.
#include "nitro/math.h"

// The SDK wrote this in assembly: an iterative quicksort whose swaps use SWP and SWPB, with its stack of ranges
// either the caller's buffer or room made on the stack, sized for the element count.
asm void MATH_QSort(void *head, u32 num, u32 width, MATHCompareFunc comp, void *stackBuffer) {
    stmdb sp!, {r4-r11, lr}
    cmp r1, #1
    ble done
    ldr r4, [sp, #0x24]
    mov r11, r3
    mov r8, r2
    cmp r4, #0
    bne have_stack
    clz r2, r1
    rsb r2, r2, #0x20
    mov r2, r2, lsl #3
    sub sp, sp, r2
    mov r4, sp
    str r2, [sp, #-4]!
have_stack:
    sub r1, r1, #1
    mla r1, r1, r8, r0
    mov r5, r4
    str r0, [r4], #4
    str r1, [r4], #4
    clz r2, r8
    rsb r2, r2, #0x20
    str r2, [sp, #-4]!
next_range:
    cmp r4, r5
    beq finish
    ldr r7, [r4, #-4]
    ldr r6, [r4, #-8]!
    sub r2, r7, r6
    cmp r2, r8
    bne partition
    mov r0, r6
    mov r1, r7
    blx r11
    cmp r0, #0
    ble next_range
    mov r0, r8
    tst r0, #3
    beq swap_pair_words
swap_pair_bytes:
    ldrb r1, [r6]
    subs r0, r0, #1
    swpb r1, r1, [r7]
    add r7, r7, #1
    strb r1, [r6], #1
    bne swap_pair_bytes
    b next_range
swap_pair_words:
    ldr r1, [r6]
    subs r0, r0, #4
    swp r1, r1, [r7]
    add r7, r7, #4
    str r1, [r6], #4
    bne swap_pair_words
    b next_range
partition:
    ldr r3, [sp]
    sub r2, r7, r6
    mov r2, r2, lsr r3
    mla r2, r2, r8, r6
    mov r3, r6
    mov r0, r8
    mov r2, r2
    tst r0, #3
    beq swap_mid_words
swap_mid_bytes:
    ldrb r1, [r2]
    subs r0, r0, #1
    swpb r1, r1, [r3]
    add r3, r3, #1
    strb r1, [r2], #1
    bne swap_mid_bytes
    b scan_start
swap_mid_words:
    ldr r1, [r2]
    subs r0, r0, #4
    swp r1, r1, [r3]
    add r3, r3, #4
    str r1, [r2], #4
    bne swap_mid_words
scan_start:
    mov r9, r6
    mov r10, r7
    add r9, r9, r8
scan_up:
    cmp r9, r7
    bge scan_down
    mov r1, r6
    mov r0, r9
    blx r11
    cmp r0, #0
    addlt r9, r9, r8
    blt scan_up
scan_down:
    mov r1, r6
    mov r0, r10
    blx r11
    cmp r0, #0
    subgt r10, r10, r8
    bgt scan_down
    cmp r9, r10
    bge place_pivot
    mov r2, r9
    mov r3, r10
    mov r0, r8
    tst r0, #3
    beq swap_scan_words
swap_scan_bytes:
    ldrb r1, [r2]
    subs r0, r0, #1
    swpb r1, r1, [r3]
    add r3, r3, #1
    strb r1, [r2], #1
    bne swap_scan_bytes
    b scan_advance
swap_scan_words:
    ldr r1, [r2]
    subs r0, r0, #4
    swp r1, r1, [r3]
    add r3, r3, #4
    str r1, [r2], #4
    bne swap_scan_words
scan_advance:
    add r9, r9, r8
    sub r10, r10, r8
    cmp r9, r10
    ble scan_up
place_pivot:
    mov r2, r6
    mov r3, r10
    mov r0, r8
    tst r0, #3
    beq swap_pivot_words
swap_pivot_bytes:
    ldrb r1, [r2]
    subs r0, r0, #1
    swpb r1, r1, [r3]
    add r3, r3, #1
    strb r1, [r2], #1
    bne swap_pivot_bytes
    b push_ranges
swap_pivot_words:
    ldr r1, [r2]
    subs r0, r0, #4
    swp r1, r1, [r3]
    add r3, r3, #4
    str r1, [r2], #4
    bne swap_pivot_words
push_ranges:
    sub r2, r10, r6
    sub r3, r7, r10
    cmp r2, r3
    ble push_right_first
    sub r2, r10, r8
    cmp r6, r2
    strlt r6, [r4], #4
    strlt r2, [r4], #4
    add r2, r10, r8
    cmp r2, r7
    strlt r2, [r4], #4
    strlt r7, [r4], #4
    b next_range
push_right_first:
    add r2, r10, r8
    cmp r2, r7
    strlt r2, [r4], #4
    strlt r7, [r4], #4
    sub r2, r10, r8
    cmp r6, r2
    strlt r6, [r4], #4
    strlt r2, [r4], #4
    b next_range
finish:
    add sp, sp, #4
    sub r4, r4, #4
    cmp r4, sp
    ldreq r0, [sp]
    addeq r0, r0, #4
    addeq sp, sp, r0
done:
    ldmia sp!, {r4-r11, lr}
    bx lr
}
