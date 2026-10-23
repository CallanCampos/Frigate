#include "dsprot/dsprot_internal.h"

// The game's copy of DS Protect's entry points (overlay 337): four functions that the game calls through the stubs
// DSProt_CallRunChecks1 to 4. Each verifies the code of two stubs, runs the checks they stand for with the pair of
// functions the game picked and the arguments, and calls the first of the pair if the codes add up, the second if not.
// The ROM doesn't name this file; it is named for what it does.

DSProtCallback data_ov337_02182444[2];
u32 data_ov337_02182440;

static const u32 sEncryptedKeyMulti[6] = { 0x08a27510, 0xe47ab3c3, 0x5a289302, 0xeaa6cac8, 0xe00d75d5, 0xe2d2fe01 };

const u32 DSProt_DecryptCodeEntryMulti = (u32)(DSProt_DecryptCodeWithKeyMulti + 0x2200);

// What the runner returns when a check failed or its code was changed: the result of the second function if a check
// has run it already, else the result of running it now
static inline void *DSProt_CheckFailed(DSProtCheckArgs *args, void *arg0, void *arg1) {
    if (args->result != NULL) {
        return args->result;
    }
    return args->tamper(arg0, arg1);
}

// Checks the first 37 words of a decryption wrapper before it is called
static inline BOOL DSProt_CheckWrapperMulti(const void *function) {
    u32 i;
    const u32 *code;
    u32 checksum;

    code = function;
    i = 0x25;
    checksum = 0;
    do {
        checksum ^= (*code >> i) | (*code << (32 - i));
        code++;
    } while (--i != 0);
    return checksum == DSPROT_CHECKSUM;
}

// Runs the checks of a list that ends at its first zero, each of which gets the arguments and returns a code that is
// added up, and calls the first function of the pair the game picked if the total is a multiple of `divisor`, the
// second if it isn't or a check failed
static inline void *DSProt_RunQueueMulti(u32 *check, void *arg0, void *arg1, u32 divisor) {
    DSProtCheckArgs args;
    u32 total;
    u32 (*func)(DSProtCheckArgs *);
    u32 value;
    // DS Protect keeps the addresses of the pair and of its index biased by 0x2200 in the literals and removes the
    // bias at run time, as a signed value
    DSProtCallback *callbacks = (DSProtCallback *)((s32)(data_ov337_02182444 + 0x880) - 0x2200);
    u32 index = *(u32 *)((s32)(&data_ov337_02182440 + 0x880) - 0x2200);

    args.nop = callbacks[index];
    args.tamper = callbacks[index ^ 1];
    args.arg0 = arg0;
    args.arg1 = arg1;
    args.result = NULL;
    args.failed = 0;
    total = 0x30eb87;
    do {
        func = (u32 (*)(DSProtCheckArgs *))(*check - 0x2200);
        if (!DSProt_CheckWrapperMulti(func)) {
            return DSProt_CheckFailed(&args, arg0, arg1);
        }
        value = func(&args);
        if (value == 0 && args.failed == 0) {
            return DSProt_CheckFailed(&args, arg0, arg1);
        }
        total += value;
        check++;
    } while (*check != 0);
    if (total % divisor == 0) {
        return args.nop(arg0, arg1);
    }
    if (args.failed != 0) {
        return args.result;
    }
    return args.tamper(arg0, arg1);
}

// Runs the two checks of the stubs `DSProt_CallCheckFlashcartA` and `DSProt_CallCheckFlashcartAStub`, each of which
// gets the arguments and returns a code that is added up
void *DSProt_RunChecks1(void *arg0, void *arg1) {
    u32 checks[32];

    // The order of these stores, the list's end first, sets the registers of the runner's first loads
    checks[2] = 0;
    checks[0] = (u32)(DSProt_CallCheckFlashcartA + 0x2200);
    checks[1] = (u32)(DSProt_CallCheckFlashcartAStub + 0x2200);
    return DSProt_RunQueueMulti(checks, arg0, arg1, 241);
}

// Runs the two checks of the stubs `DSProt_CallCheckFlashcartB` and `DSProt_CallCheckFlashcartBStub`, each of which
// gets the arguments and returns a code that is added up
void *DSProt_RunChecks2(void *arg0, void *arg1) {
    u32 checks[32];

    // The order of these stores, the list's end first, sets the registers of the runner's first loads
    checks[2] = 0;
    checks[0] = (u32)(DSProt_CallCheckFlashcartB + 0x2200);
    checks[1] = (u32)(DSProt_CallCheckFlashcartBStub + 0x2200);
    return DSProt_RunQueueMulti(checks, arg0, arg1, 251);
}

// Runs the two checks of the stubs `DSProt_CallCheckEmulatorA` and `DSProt_CallCheckEmulatorAStub`, each of which gets
// the arguments and returns a code that is added up
void *DSProt_RunChecks3(void *arg0, void *arg1) {
    u32 checks[32];

    // The order of these stores, the list's end first, sets the registers of the runner's first loads
    checks[2] = 0;
    checks[0] = (u32)(DSProt_CallCheckEmulatorA + 0x2200);
    checks[1] = (u32)(DSProt_CallCheckEmulatorAStub + 0x2200);
    return DSProt_RunQueueMulti(checks, arg0, arg1, 241);
}

// Runs the two checks of the stubs `DSProt_CallCheckEmulatorB` and `DSProt_CallCheckEmulatorBStub`, each of which gets
// the arguments and returns a code that is added up
void *DSProt_RunChecks4(void *arg0, void *arg1) {
    u32 checks[32];

    // The order of these stores, the list's end first, sets the registers of the runner's first loads
    checks[2] = 0;
    checks[0] = (u32)(DSProt_CallCheckEmulatorB + 0x2200);
    checks[1] = (u32)(DSProt_CallCheckEmulatorBStub + 0x2200);
    return DSProt_RunQueueMulti(checks, arg0, arg1, 251);
}

// Hand-written, as every DSProt_Call* is: checksums DSProt_DispatchMulti from the address and word count packed in its
// last word, then has DSProt_DispatchMulti decrypt DSProt_RunChecks1, run it and encrypt it again
// clang-format off
asm void *DSProt_CallRunChecks1(void *arg0, void *arg1) {
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
    dcd DSProt_RunChecks1 + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x184
    dcd 0
    dcd DSProt_DispatchMulti + (26 << 26)
}
// clang-format on

// Hand-written, as every DSProt_Call* is: checksums DSProt_DispatchMulti from the address and word count packed in its
// last word, then has DSProt_DispatchMulti decrypt DSProt_RunChecks3, run it and encrypt it again
// clang-format off
asm void *DSProt_CallRunChecks3(void *arg0, void *arg1) {
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
    dcd DSProt_RunChecks3 + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x184
    dcd 0
    dcd DSProt_DispatchMulti + (26 << 26)
}
// clang-format on

// Hand-written, as every DSProt_Call* is: checksums DSProt_DispatchMulti from the address and word count packed in its
// last word, then has DSProt_DispatchMulti decrypt DSProt_RunChecks2, run it and encrypt it again
// clang-format off
asm void *DSProt_CallRunChecks2(void *arg0, void *arg1) {
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
    dcd DSProt_RunChecks2 + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x184
    dcd 0
    dcd DSProt_DispatchMulti + (26 << 26)
}
// clang-format on

// Hand-written, as every DSProt_Call* is: checksums DSProt_DispatchMulti from the address and word count packed in its
// last word, then has DSProt_DispatchMulti decrypt DSProt_RunChecks4, run it and encrypt it again
// clang-format off
asm void *DSProt_CallRunChecks4(void *arg0, void *arg1) {
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
    dcd DSProt_RunChecks4 + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x184
    dcd 0
    dcd DSProt_DispatchMulti + (26 << 26)
}
// clang-format on

// Hand-written, called by the game before the first of these: has the other objects' Decrypt* functions decrypt their
// functions, then DSProt_DecryptMulti decrypt this file's, and writes `bx lr` and r2 to r10 over its first ten words so
// that a second call does nothing
// clang-format off
asm void DSProt_Init(void) {
    stmdb sp!, {lr}
    bl DSProt_DecryptDetectMulti
    bl DSProt_DecryptDispatchMulti
    bl DSProt_DecryptStubCheckMulti
    bl DSProt_DecryptCipherMulti
    ldmia sp!, {lr}
    ldr r0, =DSProt_Init + 0x2200
    ldr r1, =0xe12fff1e
    sub r0, r0, #0x2200
    stmia r0!, {r1, r2, r3, r4, r5, r6, r7, r8, r9, r10}
    orr r0, pc, #0
    adds r0, r0, #4
    bne DSProt_DecryptMulti
    dcd DSProt_CallRunChecks1 + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x94
    dcd DSProt_CallRunChecks3 + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x94
    dcd DSProt_CallRunChecks2 + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x94
    dcd DSProt_CallRunChecks4 + 0x2200
    dcd DSProt_WorkMulti + 0x2200 + 0x94
    dcd 0
    dcd 0
    dcd sEncryptedKeyMulti + 0x2200
}

// clang-format on
