#include "dsprot/dsprot.h"
#include "dsprot/dsprot_internal.h"
#include "nitro/mi.h"
#include "nitro/os.h"

// DS Protect's entry point in overlay 165. DSProt_CallRunChecks runs the four checks of the other objects and calls
// the game's callback if they pass, and DSProt_CallCrash wipes the stack and stops the game when they don't. The ROM
// doesn't name this file; it is named for what it does.

static const u32 sEncryptedKey[6] = { 0x08a27510, 0xe47ab3c3, 0x5a289302, 0xeaa6cac8, 0xe00d75d5, 0xe2d2fe00 };

// Hand-written: clears 0x100 bytes of the stack, then jumps to OS_Terminate with the return address cleared, so that
// the game stops without a trace of where it came from
// clang-format off
asm void DSProt_Crash(void) {
    mov r10, pc
    orrs r10, r10, r10
    eorne lr, lr, lr
    subs r10, r10, #8
    ldr r9, =DSProt_ClearMemory + 0x2200
    mov r0, sp
    sub r9, r9, #0x2200
    mov r1, #0x100
    blx r9
    ldr lr, =OS_Terminate + 0x2200
    mov r0, r10
    mov r1, #0x1000
    sub lr, lr, #0x2200
    bx r9
}
// clang-format on

void DSProt_ClearMemory(void *dest, u32 size) {
    MIi_CpuClear32(0, dest, size);
}

// Hand-written, as every DSProt_Call* is: verifies the code from the address packed in its last word, then has
// DSProt_Dispatch decrypt DSProt_Crash, run it and encrypt it again
// clang-format off
asm void *DSProt_CallCrash(void *arg0, void *arg1) {
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
    dcd DSProt_Crash + 0x2200
    dcd DSProt_Work + 0x2200 + 0x38
    dcd 0
    dcd DSProt_Dispatch + (26 << 26)
}
// clang-format on

// Checks the first 37 words of a decryption wrapper before it is called.
static inline BOOL DSProt_CheckWrapper(const void *function) {
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

// Adds the checks' encoded results and invokes the callback only when the total has the expected remainder.
static inline void *DSProt_RunQueue(u32 *check, DSProtCallback callback, void *arg0, void *arg1) {
    u32 total;
    u32 (*func)(void *);
    u32 value;

    total = 0x30eb87;
    do {
        func = (u32 (*)(void *))(*check - 0x2200);
        if (!DSProt_CheckWrapper(func)) {
            return DSProt_CallCrash(NULL, NULL);
        }
        value = func(NULL);
        if (value == 0) {
            return DSProt_CallCrash(NULL, NULL);
        }
        total += value;
        check++;
    } while (*check != 0);
    if (total % 241 == 0) {
        if (callback) {
            return callback(arg0, arg1);
        } else {
            return NULL;
        }
    } else {
        return DSProt_CallCrash(NULL, NULL);
    }
}

// Runs the emulator and flashcart checks and their integrity checks.
void *DSProt_RunChecks(DSProtCallback callback, void *arg0, void *arg1) {
    u32 checks[5];

    checks[0] = (u32)(DSProt_CallCheckEmulatorStub + 0x2200);
    checks[1] = (u32)(DSProt_CallCheckEmulator + 0x2200);
    checks[2] = (u32)(DSProt_CallCheckFlashcart + 0x2200);
    checks[3] = (u32)(DSProt_CallCheckFlashcartStub + 0x2200);
    checks[4] = 0;
    return DSProt_RunQueue(checks, callback, arg0, arg1);
}

// Hand-written, as every DSProt_Call* is: verifies DSProt_Dispatch, then has it decrypt DSProt_RunChecks, run it
// and encrypt it again
// clang-format off
asm void *DSProt_CallRunChecks(DSProtCallback callback, void *arg0, void *arg1) {
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
    dcd DSProt_RunChecks + 0x2200
    dcd DSProt_Work + 0x2200 + 0x110
    dcd 0
    dcd DSProt_Dispatch + (26 << 26)
}
// clang-format on

// Hand-written, run from .ctor when the overlay is loaded: has DSProt_Decrypt decrypt this file's encrypted code, the
// pairs of a function and its length that follow, and erase this code
// clang-format off
asm void DSProt_DecryptRun(void) {
    orr r0, pc, #0
    adds r0, r0, #4
    bne DSProt_Decrypt
    dcd DSProt_CallRunChecks + 0x2200
    dcd DSProt_Work + 0x2200 + 0x94
    dcd DSProt_CallCrash + 0x2200
    dcd DSProt_Work + 0x2200 + 0x94
    dcd 0
    dcd 0
    dcd sEncryptedKey + 0x2200
}
// clang-format on

#pragma define_section ctor ".ctor"                                                                                    \
                            ".ctor"                                                                                    \
                            ".ctor" abs32 RW
// The loader, rather than C code, references this registration.
__attribute__((used)) __declspec(ctor) static void (*const sDecryptRun)(void) = DSProt_DecryptRun;
