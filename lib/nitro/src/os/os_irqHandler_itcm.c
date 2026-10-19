// The ITCM part of NitroSDK's os_irqHandler.c: the interrupt dispatcher and the thread switch that the handler it
// calls returns into. The SDK wrote both in assembly, so each is an MWCC asm function.
#include "nitro/os.h"

// clang-format off
// NitroSDK's OS_IrqHandler (swan's CPU_IRQInvoke): takes the interrupt with the lowest number of the enabled and
// pending ones, acknowledges it and calls its handler, with lr set to the thread switch below so that the handler
// returns into it
asm void OS_IrqHandler(void) {
    stmdb sp!, {lr}
    mov ip, #0x4000000
    add ip, ip, #0x210
    ldr r1, [ip, #-0x8]         // master enable (IME)
    cmp r1, #0
    ldmeqia sp!, {pc}
    ldmia ip, {r1, r2}          // enabled interrupts (IE) and pending ones (IF)
    ands r1, r1, r2
    ldmeqia sp!, {pc}
    mov r3, #0x80000000
@findMask:
    clz r0, r1
    bics r1, r1, r3, lsr r0
    bne @findMask
    mov r1, r3, lsr r0
    str r1, [ip, #0x4]          // acknowledge it in IF
    rsbs r0, r0, #0x1f
    ldr r1, =OS_IRQTable
    ldr r0, [r1, r0, lsl #0x2]
    ldr lr, =OS_IrqHandler_ThreadSwitch
    bx r0
}

// NitroSDK's OS_IrqHandler_ThreadSwitch, the rest of the dispatcher: moves the threads that slept in OS_WaitIrq back
// to ready, then switches to the highest-priority ready thread, saving the context of the thread that was interrupted
// into its own and loading the new thread's
asm void OS_IrqHandler_ThreadSwitch(void) {
    ldr ip, =OSi_IrqThreadQueue
    mov r3, #0
    ldr ip, [ip, #0]
    mov r2, #1
    cmp ip, #0
    beq @checkReschedule
@wakeQueue:
    str r2, [ip, #0x64]         // thread->state = OS_THREAD_STATE_READY
    str r3, [ip, #0x78]         // thread->queue = NULL
    str r3, [ip, #0x7c]         // thread->link.prev = NULL
    ldr r0, [ip, #0x80]
    str r3, [ip, #0x80]         // thread->link.next = NULL
    mov ip, r0
    cmp ip, #0
    bne @wakeQueue
    ldr ip, =OSi_IrqThreadQueue
    str r3, [ip, #0x0]
    str r3, [ip, #0x4]
    ldr ip, =OSi_ThreadInfo
    mov r1, #1
    strh r1, [ip, #0x0]         // isNeedRescheduling = TRUE
@checkReschedule:
    ldr ip, =OSi_ThreadInfo
    ldrh r1, [ip, #0x0]
    cmp r1, #0
    ldreq pc, [sp], #0x4
    mov r1, #0
    strh r1, [ip, #0x0]
    mov r3, #0xd2
    msr cpsr_c, r3              // IRQ mode, with IRQs and FIQs off
    add r2, ip, #0x8
    ldr r1, [r2, #0x0]          // OSi_ThreadInfo.list, the threads by priority
@findThread:
    cmp r1, #0
    ldrneh r0, [r1, #0x64]
    cmpne r0, #0x1
    ldrne r1, [r1, #0x68]
    bne @findThread
    cmp r1, #0
    bne @switchThread
@restoreMode:
    mov r3, #0x92
    msr cpsr_c, r3              // back to the mode of the interrupted code
    ldr pc, [sp], #0x4
@switchThread:
    ldr r0, [ip, #0x4]          // OSi_ThreadInfo.current, the thread that was interrupted
    cmp r1, r0
    beq @restoreMode
    ldr r3, [ip, #0xc]          // switchCallback
    cmp r3, #0
    beq @saveContext
    stmdb sp!, {r0, r1, ip}
    mov lr, pc
    bx r3
    ldmia sp!, {r0, r1, ip}
@saveContext:
    str r1, [ip, #0x4]          // OSi_ThreadInfo.current = the new thread
    mrs r2, spsr
    str r2, [r0, #0x0]!         // the old thread's context.cpsr = the interrupted code's
    stmdb sp!, {r0, r1}
    add r0, r0, #0x0
    add r0, r0, #0x48
    ldr r1, =CP_SaveContext     // save the divider's context into the old thread's
    blx r1
    ldmia sp!, {r0, r1}
    ldmib sp!, {r2, r3}
    stmib r0!, {r2, r3}
    ldmib sp!, {r2, r3, ip, lr}
    stmib r0, {r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, ip, sp, lr}^
    add r0, r0, #0x34
    stmib r0!, {lr}
    mov r3, #0xd3
    msr cpsr_c, r3              // SVC mode, with IRQs and FIQs off
    stmib r0!, {sp}
    stmdb sp!, {r1}
    add r0, r1, #0x0
    add r0, r0, #0x48
    ldr r1, =CPi_RestoreContext // restore the new thread's
    blx r1
    ldmia sp!, {r1}
    ldr sp, [r1, #0x44]         // the new thread's context.sp_svc
    mov r3, #0xd2
    msr cpsr_c, r3
    ldr r2, [r1, #0x0]!
    msr spsr_fc, r2
    ldr lr, [r1, #0x40]
    ldmib r1, {r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, ip, sp, lr}^
    mov r0, r0
    stmda sp!, {r0, r1, r2, r3, ip, lr}
    ldmia sp!, {pc}
}

// clang-format on
