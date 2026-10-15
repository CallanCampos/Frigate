#include "dsprot/dsprot_internal.h"

// The checks that the stubs of the four detection checks in the game's copy of DS Protect (overlay 337) are intact,
// each with its own stub and codes. The ROM doesn't name this file; it is named for what it does.

const u32 DSProt_EncryptCodeEntryMulti = (u32)(DSProt_EncryptCodeWithKeyMulti + 0x2200);

// Whether DSProt_CallCheckEmulatorA still begins as every DSProt_Call* stub does
u32 DSProt_CheckEmulatorAStub(void) {
    const u8 *code = (const u8 *)((u32)(DSProt_CallCheckEmulatorA + 0x2200) - 0x4400);

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

// The same for DSProt_CallCheckEmulatorB
u32 DSProt_CheckEmulatorBStub(void) {
    const u8 *code = (const u8 *)((u32)(DSProt_CallCheckEmulatorB + 0x2200) - 0x4400);

    if (code[0x2200] != 0x0f) {
        return 0xb5b1;
    }
    if (code[0x2201] != 0xc0) {
        return 0xb5b1;
    }
    if (code[0x2202] != 0x8f) {
        return 0xb5b1;
    }
    if (code[0x2203] != 0xe1) {
        return 0xb5b1;
    }
    if (code[0x2204] != 0x0c) {
        return 0xb5b1;
    }
    if (code[0x2205] != 0xc0) {
        return 0xb5b1;
    }
    if (code[0x2206] != 0x1c) {
        return 0xb5b1;
    }
    if (code[0x2207] != 0xe0) {
        return 0xb5b1;
    }
    if (code[0x2208] != 0x00) {
        return 0xb5b1;
    }
    if (code[0x2209] != 0xc0) {
        return 0xb5b1;
    }
    if (code[0x220a] != 0xa0) {
        return 0xb5b1;
    }
    if (code[0x220b] != 0x03) {
        return 0xb5b1;
    }
    if (code[0x220c] != 0x8c) {
        return 0xb5b1;
    }
    if (code[0x220d] != 0xc0) {
        return 0xb5b1;
    }
    if (code[0x220e] != 0x8c) {
        return 0xb5b1;
    }
    if (code[0x220f] != 0x12) {
        return 0xb5b1;
    }
    return 0xbd3b;
}

// The same for DSProt_CallCheckFlashcartA
u32 DSProt_CheckFlashcartAStub(void) {
    const u8 *code = (const u8 *)((u32)(DSProt_CallCheckFlashcartA + 0x2200) - 0x4400);

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

// The same for DSProt_CallCheckFlashcartB
u32 DSProt_CheckFlashcartBStub(void) {
    const u8 *code = (const u8 *)((u32)(DSProt_CallCheckFlashcartB + 0x2200) - 0x4400);

    if (code[0x2200] != 0x0f) {
        return 0xb5b1;
    }
    if (code[0x2201] != 0xc0) {
        return 0xb5b1;
    }
    if (code[0x2202] != 0x8f) {
        return 0xb5b1;
    }
    if (code[0x2203] != 0xe1) {
        return 0xb5b1;
    }
    if (code[0x2204] != 0x0c) {
        return 0xb5b1;
    }
    if (code[0x2205] != 0xc0) {
        return 0xb5b1;
    }
    if (code[0x2206] != 0x1c) {
        return 0xb5b1;
    }
    if (code[0x2207] != 0xe0) {
        return 0xb5b1;
    }
    if (code[0x2208] != 0x00) {
        return 0xb5b1;
    }
    if (code[0x2209] != 0xc0) {
        return 0xb5b1;
    }
    if (code[0x220a] != 0xa0) {
        return 0xb5b1;
    }
    if (code[0x220b] != 0x03) {
        return 0xb5b1;
    }
    if (code[0x220c] != 0x8c) {
        return 0xb5b1;
    }
    if (code[0x220d] != 0xc0) {
        return 0xb5b1;
    }
    if (code[0x220e] != 0x8c) {
        return 0xb5b1;
    }
    if (code[0x220f] != 0x12) {
        return 0xb5b1;
    }
    return 0xbd3b;
}

// clang-format off
asm u32 DSProt_CallCheckEmulatorAStub(void) {
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
    dcd DSProt_WorkMulti + 1
    dcd DSProt_WorkMulti + 3
    dcd DSProt_CheckEmulatorAStub + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x110
    dcd 0
    dcd DSProt_DispatchMulti + (26 << 26)
}
// clang-format on

// clang-format off
asm u32 DSProt_CallCheckEmulatorBStub(void) {
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
    dcd DSProt_WorkMulti + 1
    dcd DSProt_WorkMulti + 3
    dcd DSProt_CheckEmulatorBStub + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x110
    dcd 0
    dcd DSProt_DispatchMulti + (26 << 26)
}
// clang-format on

// clang-format off
asm u32 DSProt_CallCheckFlashcartAStub(void) {
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
    dcd DSProt_WorkMulti + 1
    dcd DSProt_WorkMulti + 3
    dcd DSProt_CheckFlashcartAStub + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x110
    dcd 0
    dcd DSProt_DispatchMulti + (26 << 26)
}
// clang-format on

// clang-format off
asm u32 DSProt_CallCheckFlashcartBStub(void) {
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
    dcd DSProt_WorkMulti + 1
    dcd DSProt_WorkMulti + 3
    dcd DSProt_CheckFlashcartBStub + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x110
    dcd 0
    dcd DSProt_DispatchMulti + (26 << 26)
}
// clang-format on

// Hand-written, run by DSProt_Init: has DSProt_DecryptMulti decrypt this file's stubs
// clang-format off
asm void DSProt_DecryptStubCheckMulti(void) {
    orr r0, pc, #0
    adds r0, r0, #4
    bne DSProt_DecryptMulti
    dcd DSProt_CallCheckEmulatorAStub + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x94
    dcd DSProt_CallCheckEmulatorBStub + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x94
    dcd DSProt_CallCheckFlashcartAStub + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x94
    dcd DSProt_CallCheckFlashcartBStub + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x94
    dcd 0
    dcd 0
}

// clang-format on
