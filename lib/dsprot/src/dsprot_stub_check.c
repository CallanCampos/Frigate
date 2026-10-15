#include "dsprot/dsprot_internal.h"

// The checks that the stubs of DS Protect's other two checks are intact. The ROM doesn't name this file; it is named
// for what it does.

// Whether DSProt_CallCheckEmulator still begins as every DSProt_Call* stub does
u32 DSProt_CheckEmulatorStub(void) {
    const u8 *code = (const u8 *)((u32)(DSProt_CallCheckEmulator + 0x2200) - 0x4400);

    if (code[0x2200] != 0x0f) {
        return 0xa99f;
    }
    if (code[0x2201] != 0xc0) {
        return 0xa99f;
    }
    if (code[0x2202] != 0x8f) {
        return 0xa99f;
    }
    if (code[0x2203] != 0xe1) {
        return 0xa99f;
    }
    if (code[0x2204] != 0x0c) {
        return 0xa99f;
    }
    if (code[0x2205] != 0xc0) {
        return 0xa99f;
    }
    if (code[0x2206] != 0x1c) {
        return 0xa99f;
    }
    if (code[0x2207] != 0xe0) {
        return 0xa99f;
    }
    if (code[0x2208] != 0x00) {
        return 0xa99f;
    }
    if (code[0x2209] != 0xc0) {
        return 0xa99f;
    }
    if (code[0x220a] != 0xa0) {
        return 0xa99f;
    }
    if (code[0x220b] != 0x03) {
        return 0xa99f;
    }
    if (code[0x220c] != 0x8c) {
        return 0xa99f;
    }
    if (code[0x220d] != 0xc0) {
        return 0xa99f;
    }
    if (code[0x220e] != 0x8c) {
        return 0xa99f;
    }
    if (code[0x220f] != 0x12) {
        return 0xa99f;
    }
    return 0xa2dd;
}

// Whether DSProt_CallCheckFlashcart still begins as every DSProt_Call* stub does
u32 DSProt_CheckFlashcartStub(void) {
    const u8 *code = (const u8 *)((u32)(DSProt_CallCheckFlashcart + 0x2200) - 0x4400);

    if (code[0x2200] != 0x0f) {
        return 0xa99f;
    }
    if (code[0x2201] != 0xc0) {
        return 0xa99f;
    }
    if (code[0x2202] != 0x8f) {
        return 0xa99f;
    }
    if (code[0x2203] != 0xe1) {
        return 0xa99f;
    }
    if (code[0x2204] != 0x0c) {
        return 0xa99f;
    }
    if (code[0x2205] != 0xc0) {
        return 0xa99f;
    }
    if (code[0x2206] != 0x1c) {
        return 0xa99f;
    }
    if (code[0x2207] != 0xe0) {
        return 0xa99f;
    }
    if (code[0x2208] != 0x00) {
        return 0xa99f;
    }
    if (code[0x2209] != 0xc0) {
        return 0xa99f;
    }
    if (code[0x220a] != 0xa0) {
        return 0xa99f;
    }
    if (code[0x220b] != 0x03) {
        return 0xa99f;
    }
    if (code[0x220c] != 0x8c) {
        return 0xa99f;
    }
    if (code[0x220d] != 0xc0) {
        return 0xa99f;
    }
    if (code[0x220e] != 0x8c) {
        return 0xa99f;
    }
    if (code[0x220f] != 0x12) {
        return 0xa99f;
    }
    return 0xa2dd;
}

// clang-format off
asm u32 DSProt_CallCheckEmulatorStub(void) {
    orr ip, pc, pc
    ands ip, ip, ip
    moveq ip, #0
    addne ip, ip, #0x8c
    ldr ip, [ip, #0x14]
    sub ip, ip, #0
    stmdb sp!, {r0, r1, r2, r3, r4}
    mov r1, ip, lsr #0x1a
    eor r3, r3, r3
    mov ip, ip, lsl #6
    mov ip, ip, lsr #6
    mov r0, ip
@1:
    ldr r2, [r0, #0]
    mov r4, r2, lsr #0x18
    cmp r4, #0xea
    cmpne r4, #0xeb
    eorne r3, r3, r2, ror #0x11
    addne r3, r3, r2, ror #0x1c
    eorne r3, r3, r2, ror r1
    subs r1, r1, #1
    add r0, r0, #4
    bne @1
    mov r0, #0x2000000
    orr r1, r0, #0x800
    mov r0, #0x4000000
    orr r0, r0, #0x4000
    ldr r0, [r0, #0]
    ands r0, r0, #1
    orrne r1, r1, #0x4000
    cmp ip, r1
    strge r3, [pc, #0x18]
    movlt ip, lr
    ldmgeia sp!, {r0, r1, r2, r3, r4}
    ldmltia sp!, {r5, r6, r7, r8, r9}
    stmgedb sp!, {ip}
    orr ip, pc, pc
    ldmia sp!, {pc}
    dcd DSProt_Work + 1
    dcd DSProt_Work + 3
    dcd DSProt_CheckEmulatorStub + 0x2200
    dcd DSProt_Work + 0x2200 + 0x110
    dcd 0
    dcd DSProt_Dispatch + (26 << 26)
}
// clang-format on

// clang-format off
asm u32 DSProt_CallCheckFlashcartStub(void) {
    orr ip, pc, pc
    ands ip, ip, ip
    moveq ip, #0
    addne ip, ip, #0x8c
    ldr ip, [ip, #0x14]
    sub ip, ip, #0
    stmdb sp!, {r0, r1, r2, r3, r4}
    mov r1, ip, lsr #0x1a
    eor r3, r3, r3
    mov ip, ip, lsl #6
    mov ip, ip, lsr #6
    mov r0, ip
@1:
    ldr r2, [r0, #0]
    mov r4, r2, lsr #0x18
    cmp r4, #0xea
    cmpne r4, #0xeb
    eorne r3, r3, r2, ror #0x11
    addne r3, r3, r2, ror #0x1c
    eorne r3, r3, r2, ror r1
    subs r1, r1, #1
    add r0, r0, #4
    bne @1
    mov r0, #0x2000000
    orr r1, r0, #0x800
    mov r0, #0x4000000
    orr r0, r0, #0x4000
    ldr r0, [r0, #0]
    ands r0, r0, #1
    orrne r1, r1, #0x4000
    cmp ip, r1
    strge r3, [pc, #0x18]
    movlt ip, lr
    ldmgeia sp!, {r0, r1, r2, r3, r4}
    ldmltia sp!, {r5, r6, r7, r8, r9}
    stmgedb sp!, {ip}
    orr ip, pc, pc
    ldmia sp!, {pc}
    dcd DSProt_Work + 1
    dcd DSProt_Work + 3
    dcd DSProt_CheckFlashcartStub + 0x2200
    dcd DSProt_Work + 0x2200 + 0x110
    dcd 0
    dcd DSProt_Dispatch + (26 << 26)
}
// clang-format on

// Hand-written, run from .ctor when the overlay is loaded: has DSProt_Decrypt decrypt this file's stubs
// clang-format off
asm void DSProt_DecryptStubCheck(void) {
    orr r0, pc, #0
    adds r0, r0, #4
    bne DSProt_Decrypt
    dcd DSProt_CallCheckEmulatorStub + 0x2200
    dcd DSProt_Work + 0x2200 + 0x94
    dcd DSProt_CallCheckFlashcartStub + 0x2200
    dcd DSProt_Work + 0x2200 + 0x94
    dcd 0
    dcd 0
}
// clang-format on

#pragma define_section ctor ".ctor"                                                                                    \
                            ".ctor"                                                                                    \
                            ".ctor" abs32 RW
__declspec(ctor) static void (*const sDecryptStubCheck)(void) = DSProt_DecryptStubCheck;
