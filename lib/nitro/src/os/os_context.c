// NitroSDK OS: thread contexts (os_context.c, the SDK's name for the module). ARM code. OS_SaveContext and
// OS_LoadContext are written in assembly in the SDK, since a context holds the registers that a thread needs to
// continue, which C cannot save and load.
#include "nitro/cp.h"
#include "nitro/os.h"

// clang-format off

// NitroSDK's OS_InitContext: sets up a context that starts running at newpc (an address with the Thumb bit set for Thumb
// code) with the stack at newsp. The SVC mode's stack is that address, and the stack of the thread's mode is 0x40 bytes
// under it, aligned to 8. The mode is SYS, with the Thumb state bit set when the code is Thumb. This is assembly as well:
// in C the stores come in another order, with newpc + 4 in another register (tried the 13 zero stores one by one and as
// a loop, newpc + 4 in a local, and the test of the Thumb bit on pc_plus4 read back from the context)
asm void OS_InitContext(OSContext *context, u32 newpc, u32 newsp) {
    add r1, r1, #4
    str r1, [r0, #0x40]
    str r2, [r0, #0x44]
    sub r2, r2, #0x40
    tst r2, #4
    subne r2, r2, #4
    str r2, [r0, #0x38]
    ands r1, r1, #1
    movne r1, #0x3f
    moveq r1, #0x1f
    str r1, [r0, #0x0]
    mov r1, #0
    str r1, [r0, #0x4]
    str r1, [r0, #0x8]
    str r1, [r0, #0xc]
    str r1, [r0, #0x10]
    str r1, [r0, #0x14]
    str r1, [r0, #0x18]
    str r1, [r0, #0x1c]
    str r1, [r0, #0x20]
    str r1, [r0, #0x24]
    str r1, [r0, #0x28]
    str r1, [r0, #0x2c]
    str r1, [r0, #0x30]
    str r1, [r0, #0x34]
    str r1, [r0, #0x3c]
    bx lr
}

// NitroSDK's OS_SaveContext: stores the registers of the caller (its SVC mode's stack too, and the divider and square
// root unit's), so that OS_LoadContext continues after the call, with 1 as the result in place of the 0 that the call
// itself returns
asm BOOL OS_SaveContext(OSContext *context) {
    stmdb sp!, {r0, lr}
    add r0, r0, #0x48
    ldr r1, =CP_SaveContext
    blx r1
    ldmia sp!, {r0, lr}
    add r1, r0, #0
    mrs r2, cpsr
    str r2, [r1], #4
    mov r0, #0xd3
    msr cpsr_c, r0
    str sp, [r1, #0x40]
    msr cpsr_c, r2
    mov r0, #1
    stmia r1, {r0-r12, sp, lr}
    add r0, pc, #8
    str r0, [r1, #0x3c]
    mov r0, #0
    bx lr
}

// NitroSDK's OS_LoadContext: continues from a context. In the SVC mode it loads the registers of the thread's mode and
// returns to the context's PC and CPSR with SUBS PC. Before that it waits for the divider (the busy bit of DIVCNT) to
// finish, going by the flags that hw_isDSi leaves (its BEQ has no compare before it, as in the original)
asm void func_0207abbc(OSContext *context) {
    stmdb sp!, {r0, lr}
    add r0, r0, #0x48
    ldr r1, =CPi_RestoreContext
    blx r1
    ldmia sp!, {r0, lr}
    mrs r1, cpsr
    bic r1, r1, #0x1f
    orr r1, r1, #0xd3
    msr cpsr_c, r1
    ldr r1, [r0], #4
    msr spsr_fsxc, r1
    ldr sp, [r0, #0x40]
    ldr lr, [r0, #0x3c]
    ldmia r0, {r0-r12, sp, lr}^
    mov r0, r0
    stmdb sp!, {r0-r3, r12, lr}
    bl hw_isDSi
    beq @done
    ldr r0, =0x4000280
@wait:
    ldr r1, [r0, #0]
    ands r1, r1, #0x8000
    bne @wait
@done:
    ldmia sp!, {r0-r3, r12, lr}
    subs pc, lr, #4
}
// clang-format on
