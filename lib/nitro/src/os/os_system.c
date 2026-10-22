// NitroSDK OS: the processor's interrupt state and waiting (os_system.c, as ntrtwl's reconstruction of NitroSDK has
// it). The functions that change the CPSR are assembly in the SDK, in ARM code; the others are Thumb.
#include "nitro/os.h"
#include "nitro/svc.h"
#include "twl/scfg.h"

static asm void OS_SpinWait(u32 cycles);
static u16 *sys_get_boot_indicator_ptr(void);

#pragma thumb off
// clang-format off
// NitroSDK's OS_EnableInterrupts, assembly in the SDK like every function here that reads or changes the CPSR: clears
// the IRQ bit and gives the one the CPSR had
asm OSIntrMode OS_EnableInterrupts(void) {
    mrs r0, cpsr
    bic r1, r0, #0x80
    msr cpsr_c, r1
    and r0, r0, #0x80
    bx lr
}

// NitroSDK's OS_DisableInterrupts, assembly in the SDK: sets the IRQ bit and gives the one the CPSR had
asm OSIntrMode OS_DisableInterrupts(void) {
    mrs r0, cpsr
    orr r1, r0, #0x80
    msr cpsr_c, r1
    and r0, r0, #0x80
    bx lr
}

// NitroSDK's OS_RestoreInterrupts, assembly in the SDK: sets the IRQ bit to mask and gives the one the CPSR had
asm OSIntrMode OS_RestoreInterrupts(OSIntrMode mask) {
    mrs r1, cpsr
    bic r2, r1, #0x80
    orr r2, r2, r0
    msr cpsr_c, r2
    and r0, r1, #0x80
    bx lr
}

// NitroSDK's OS_DisableInterrupts_IrqAndFiq, assembly in the SDK: OS_DisableInterrupts for both the IRQ and FIQ bits
asm OSIntrMode OS_DisableInterrupts_IrqAndFiq(void) {
    mrs r0, cpsr
    orr r1, r0, #0xc0
    msr cpsr_c, r1
    and r0, r0, #0xc0
    bx lr
}

// NitroSDK's OS_RestoreInterrupts_IrqAndFiq, assembly in the SDK: OS_RestoreInterrupts for both bits
asm OSIntrMode OS_RestoreInterrupts_IrqAndFiq(OSIntrMode mask) {
    mrs r1, cpsr
    bic r2, r1, #0xc0
    orr r2, r2, r0
    msr cpsr_c, r2
    and r0, r1, #0xc0
    bx lr
}

// NitroSDK's OS_GetCpsrIrq, assembly in the SDK: the IRQ bit of the CPSR
asm OSIntrMode_Irq OS_GetCpsrIrq(void) {
    mrs r0, cpsr
    and r0, r0, #0x80
    bx lr
}

// NitroSDK's OS_GetProcMode, assembly in the SDK: the processor's mode, from the CPSR
asm OSProcMode OS_GetProcMode(void) {
    mrs r0, cpsr
    and r0, r0, #0x1f
    bx lr
}

// NitroSDK's OS_SpinWait, assembly in the SDK: a loop of four CPU cycles per turn
static asm void OS_SpinWait(u32 cycles) {
@loop:
    subs r0, r0, #4
    bhs @loop
    bx lr
}
// clang-format on
#pragma thumb on

// TwlSDK's OS_SpinWaitSysCycles: waits for some cycles of the system clock. The ARM9 runs at twice its rate, or at four
// times with the speed bit of SCFG_CLK set (the DSi's 134 MHz), and 16 of its cycles are the cost of the call
void OS_SpinWaitSysCycles(u32 cycles) {
    cycles <<= ((reg_SCFG_CLK & SCFG_CLK_CPU_2X) == 1) ? 2 : 1;
    if (cycles > 16) {
        OS_SpinWait(cycles - 16);
    }
}

// NitroSDK's OS_WaitInterrupt: waits for one of the interrupts, after forgetting the ones that happened if clear is
// set. It halts the CPU with IME on, and lets the IRQ through for a moment every time the CPU wakes
void OS_WaitInterrupt(BOOL clear, OSIrqMask interrupts) {
    OSIntrMode irq = OS_DisableInterrupts();
    BOOL ime = OS_EnableIrq();

    if (clear) {
        *(vu32 *)HW_INTR_CHECK_BUF &= ~interrupts;
    }
    while (!(*(vu32 *)HW_INTR_CHECK_BUF & interrupts)) {
        OS_Halt();
        OS_EnableInterrupts();
        OS_DisableInterrupts();
    }
    *(vu32 *)HW_INTR_CHECK_BUF &= ~interrupts;
    OS_RestoreIrq(ime);
    OS_RestoreInterrupts(irq);
}

// NitroSDK's OS_WaitVBlankIntr
void OS_WaitVBlankIntr(void) {
    SVC_WaitByLoop(1);
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
}

// NitroSDK's OS_GetBootType, and the address that it reads
u32 OS_GetBootType(void) {
    return *sys_get_boot_indicator_ptr();
}

static u16 *sys_get_boot_indicator_ptr(void) {
    return (u16 *)HW_BOOT_CHECK_INFO_BUF;
}
