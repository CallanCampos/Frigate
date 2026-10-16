// NitroSDK OS: the processor's interrupt state and waiting (os_system.c, a guessed name). The functions that change
// the CPSR are assembly in the SDK, in ARM code; the others are Thumb.
#include "nitro/os.h"
#include "twl/scfg.h"

static asm void OSi_SpinWaitCpuCycles(u32 cycles);
static u16 *sys_get_boot_indicator_ptr(void);

#pragma thumb off
// clang-format off
// NitroSDK's OS_EnableInterrupts, OS_DisableInterrupts and OS_RestoreInterrupts. Each gives the
// IRQ bit that the CPSR had
asm u32 OS_EnableInterrupts(void) {
    mrs r0, cpsr
    bic r1, r0, #0x80
    msr cpsr_c, r1
    and r0, r0, #0x80
    bx lr
}

asm u32 OS_DisableInterrupts(void) {
    mrs r0, cpsr
    orr r1, r0, #0x80
    msr cpsr_c, r1
    and r0, r0, #0x80
    bx lr
}

asm u32 OS_RestoreInterrupts(u32 mask) {
    mrs r1, cpsr
    bic r2, r1, #0x80
    orr r2, r2, r0
    msr cpsr_c, r2
    and r0, r1, #0x80
    bx lr
}

// NitroSDK's OS_DisableInterrupts_IrqAndFiq and OS_RestoreInterrupts_IrqAndFiq: the same for both the IRQ and FIQ bits
asm u32 OS_DisableInterrupts_IrqAndFiq(void) {
    mrs r0, cpsr
    orr r1, r0, #0xc0
    msr cpsr_c, r1
    and r0, r0, #0xc0
    bx lr
}

asm u32 OS_RestoreInterrupts_IrqAndFiq(u32 mask) {
    mrs r1, cpsr
    bic r2, r1, #0xc0
    orr r2, r2, r0
    msr cpsr_c, r2
    and r0, r1, #0xc0
    bx lr
}

// NitroSDK's OS_GetCpsrIrq and OS_GetProcMode
asm u32 OS_GetCpsrIrq(void) {
    mrs r0, cpsr
    and r0, r0, #0x80
    bx lr
}

asm u32 OS_GetProcMode(void) {
    mrs r0, cpsr
    and r0, r0, #0x1f
    bx lr
}

// NitroSDK's OSi_SpinWaitCpuCycles: a loop of four CPU cycles per turn
static asm void OSi_SpinWaitCpuCycles(u32 cycles) {
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
    u32 shift = 1;

    if ((reg_SCFG_CLK & SCFG_CLK_CPU_2X) == 1) {
        shift = 2;
    }
    cycles <<= shift;
    if (cycles > 16) {
        OSi_SpinWaitCpuCycles(cycles - 16);
    }
}

// NitroSDK's OS_WaitInterrupt: waits for one of the interrupts, after forgetting the ones that happened if clear is
// set. It halts the CPU with IME on, and lets the IRQ through for a moment every time the CPU wakes
void OS_WaitInterrupt(BOOL clear, u32 interrupts) {
    u32 irq = OS_DisableInterrupts();
    BOOL ime = OS_EnableIrq();

    if (clear) {
        *(vu32 *)HW_INTR_CHECK_BUF &= ~interrupts;
    }
    while (!(*(vu32 *)HW_INTR_CHECK_BUF & interrupts)) {
        cp15_halt();
        OS_EnableInterrupts();
        OS_DisableInterrupts();
    }
    *(vu32 *)HW_INTR_CHECK_BUF &= ~interrupts;
    OS_RestoreIrq(ime);
    OS_RestoreInterrupts(irq);
}

// NitroSDK's OS_WaitVBlankIntr
void OS_WaitVBlankIntr(void) {
    WaitByLoop(1);
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
}

// NitroSDK's OS_GetBootType, and the address that it reads
u32 OS_GetBootType(void) {
    return *sys_get_boot_indicator_ptr();
}

static u16 *sys_get_boot_indicator_ptr(void) {
    return (u16 *)HW_BOOT_CHECK_INFO_BUF;
}
