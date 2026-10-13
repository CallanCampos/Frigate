#include "nitro/cp.h"

// Save the math coprocessor registers, including the current operation modes.
asm void CP_SaveContext(CPContext *context) {
    ldr r1, [pc, #0x34]
    stmfd sp!, {r4}
    ldmia r1, {r2-r4, r12}
    stmia r0!, {r2-r4, r12}
    ldrh r12, [r1, #-16]
    add r1, r1, #40
    ldmia r1, {r2-r3}
    stmia r0!, {r2-r3}
    and r12, r12, #3
    ldrh r2, [r1, #-8]
    strh r12, [r0]
    and r2, r2, #1
    strh r2, [r0, #2]
    ldmfd sp!, {r4}
    bx lr
    dcd 0x04000290
}

// Restore the saved register values and modes.
asm void CPi_RestoreContext(const CPContext *context) {
    stmfd sp!, {r4}
    ldr r1, [pc, #0x2c]
    ldmia r0, {r2-r4, r12}
    stmia r1, {r2-r4, r12}
    ldrh r2, [r0, #24]
    ldrh r3, [r0, #26]
    strh r2, [r1, #-16]
    strh r3, [r1, #32]
    add r0, r0, #16
    add r1, r1, #40
    ldmia r0, {r2-r3}
    stmia r1, {r2-r3}
    ldmfd sp!, {r4}
    bx lr
    dcd 0x04000290
}
