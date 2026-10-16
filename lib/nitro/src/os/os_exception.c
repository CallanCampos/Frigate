// NitroSDK OS: the exception handler (os_exception.c). OS_InitException puts the handler in the exception vector of
// the shared main memory; when the CPU takes a data abort or an undefined instruction, the handler logs the registers
// and calls the handler that the game set. The handler and the functions it calls are assembly in the SDK, since they
// run on the registers as the exception left them.
#include "nitro/os.h"

// The exception vector in the shared main memory, and the mask that gives its address in the DS's main memory (the
// same word, 0x027ffd9c). A handler that a debugger has put in the vector lies in the area from 0x02600000 to
// 0x02800000. The stack the handler uses is the end of ITCM
#define HW_EXCP_VECTOR_BUF 0x02fffd9c
#define HW_EXCP_VECTOR_MAIN_MASK 0xff7fffff
#define OSi_DEBUGGER_HANDLER_START 0x02600000
#define OSi_DEBUGGER_HANDLER_END 0x02800000
#define OSi_EXCEPTION_STACK_TOP 0x02000000

// The registers at the exception: an OSContext, then the first word of the BIOS's frame, a word that nothing sets
// and the low bit of the stack pointer that the handler was entered with
typedef struct OSiExContext {
    OSContext context;
    u32 unk_64;
    u32 unk_68;
    u32 spLowBit;
} OSiExContext;

typedef void (*OSiExceptionHandler)(OSiExContext *context, void *arg);

static asm void OSi_ExceptionHandler(void);
static asm void OSi_GetAndDisplayContext(void); // swan's AbortUndefExcHandler
static asm void OSi_SetExContext(void);         // swan's exceptions_log_regs
static void OSi_DisplayExContext(void);         // swan's exceptions_call_abort_undef_callback

static OSiExceptionHandler OSi_ExceptionUserHandler; // swan's g_ExcUndefAbortCallback
static u32 OSi_DebuggerHandler;
static void *OSi_ExceptionUserArg; // swan's g_ExcUndefAbortCallbackData
// Nothing reads these 32 bytes, which lie between the variables above and the registers' buffer
u8 data_0214c4dc[0x20];
static OSiExContext OSi_ExContext; // swan's g_ExcRegDump

// NitroSDK's OS_InitException: keeps the handler of a debugger in the vector, calling it before ours
// would run, and otherwise installs ours there and, on a DS, in the main memory's copy of the vector
void OS_InitException(void) {
    u32 *vector = (u32 *)HW_EXCP_VECTOR_BUF;
    u32 handler = *vector;

    if (handler < OSi_DEBUGGER_HANDLER_START || handler >= OSi_DEBUGGER_HANDLER_END) {
        handler = 0;
    }
    OSi_DebuggerHandler = handler;
    if (OSi_DebuggerHandler == 0) {
        *vector = (u32)OSi_ExceptionHandler;
        if (!hw_isDSi()) {
            *(u32 *)((u32)vector & HW_EXCP_VECTOR_MAIN_MASK) = (u32)OSi_ExceptionHandler;
        }
    }
    OSi_ExceptionUserHandler = NULL;
}

#pragma thumb off
// clang-format off
// NitroSDK's OSi_ExceptionHandler, swan's NativeExceptionHandler: calls the debugger's handler first, if there is one,
// moves to the stack at the end of ITCM and handles a data abort or an undefined instruction. Then it loops forever; the
// code after the loop, which returns, is never reached
static asm void OSi_ExceptionHandler(void) {
    ldr ip, =OSi_DebuggerHandler
    ldr ip, [ip, #0]
    cmp ip, #0
    movne lr, pc
    bxne ip
    ldr ip, =OSi_EXCEPTION_STACK_TOP
    stmdb ip!, {r0, r1, r2, r3, sp, lr}
    and r0, sp, #1
    mov sp, ip
    mrs r1, cpsr
    and r1, r1, #0x1f
    teq r1, #0x17
    bne @notAbort
    bl OSi_GetAndDisplayContext
    b @done
@notAbort:
    teq r1, #0x1b
    bne @done
    bl OSi_GetAndDisplayContext
@done:
    ldr ip, =OSi_DebuggerHandler
    ldr ip, [ip, #0]
    cmp ip, #0
@spin:
    beq @spin
@wait:
    mov r0, r0
    b @wait
    ldmia sp!, {r0, r1, r2, r3, ip, lr}
    mov sp, ip
    bx lr
}

// NitroSDK's OSi_GetAndDisplayContext (a guess): logs the registers and calls the game's handler, keeping r0 (the low
// bit of the stack pointer) across the two calls
static asm void OSi_GetAndDisplayContext(void) {
    stmdb sp!, {r0, lr}
    bl OSi_SetExContext
    bl OSi_DisplayExContext
    ldmia sp!, {r0, lr}
    bx lr
}

// NitroSDK's OSi_SetExContext (a guess): gets the registers of the mode that took the exception into OSi_ExContext,
// from the BIOS's frame at ip, r4 to r11 as they are and the mode's own sp and lr, which it reads by switching to the
// mode with IRQs off
static asm void OSi_SetExContext(void) {
    ldr r1, =OSi_ExContext
    str r0, [r1, #0x6c]
    ldr r0, [ip, #0]
    str r0, [r1, #4]
    ldr r0, [ip, #4]
    str r0, [r1, #8]
    ldr r0, [ip, #8]
    str r0, [r1, #0xc]
    ldr r0, [ip, #0xc]
    str r0, [r1, #0x10]
    ldr r2, [ip, #0x10]
    bic r2, r2, #1
    add r0, r1, #0x14
    stmia r0, {r4, r5, r6, r7, r8, r9, r10, r11}
    ldr r0, [r2, #0]
    str r0, [r1, #0x64]
    ldr r3, [r2, #4]
    str r3, [r1, #0]
    ldr r0, [r2, #8]
    str r0, [r1, #0x34]
    ldr r0, [r2, #0xc]
    str r0, [r1, #0x40]
    mrs r0, cpsr
    orr r3, r3, #0x80
    bic r3, r3, #0x20
    msr cpsr_fsxc, r3
    str sp, [r1, #0x38]
    str lr, [r1, #0x3c]
    mrs r2, spsr
    msr cpsr_fsxc, r0
    bx lr
}
// clang-format on

// NitroSDK's OSi_DisplayExContext (a guess): calls the game's handler, if it set one, with the registers and its
// argument, in system mode on the current stack and with the protection unit switched on for the call and off after
// it. The assembly is the SDK's own, in a C function whose prologue and early return the compiler makes
static void OSi_DisplayExContext(void) {
    if (OSi_ExceptionUserHandler == NULL) {
        return;
    }
    // clang-format off
    asm {
        mrs r2, cpsr
        mov r0, sp
        ldr r1, =0x9f
        msr cpsr_fsxc, r1
        mov r1, sp
        mov sp, r0
        stmdb sp!, {r1, r2}
        bl OS_EnableProtectionUnit
        ldr r0, =OSi_ExContext
        ldr r1, =OSi_ExceptionUserArg
        ldr r1, [r1, #0]
        ldr ip, =OSi_ExceptionUserHandler
        ldr ip, [ip, #0]
        ldr lr, =@return
        bx ip
    @return:
        bl OS_DisableProtectionUnit
        ldmia sp!, {r1, r2}
        mov sp, r1
        msr cpsr_fsxc, r2
    }
// clang-format on
}
#pragma thumb on
