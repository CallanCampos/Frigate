// NitroSDK's waits on the host (os_system.c and os_irqHandler.c, and the BIOS's SVC_WaitByLoop). The CPSR functions of
// os_system.c are in core.c, with the rest of the CPU's state.
//
// The busy loops of the DS (OS_SpinWait under OS_SpinWaitSysCycles, and the BIOS's WaitByLoop) are modelled
// ones here: each moves the virtual clock by its length, and the events and interrupts on the way come as they would.
// A no-op would hang their callers, which wait for state that an interrupt delivers (PMi_SendPxiDataSync, PM_Init,
// OSi_DoLockByWord, CARD's pull-out check).
//
// OS_WaitIrq is the decomp's lib/nitro/src/os/os_irqHandler.c (whose ITCM handlers, os_irqHandler_itcm.c's asm
// functions, core.c replaces) and OS_WaitInterrupt the decomp's os_system.c, with OS_SetIrqCheckFlag's word reached
// through the host's DTCM: os.h's HW_INTR_CHECK_BUF truncates its 64-bit address (see os_irqTable.c).
// port/hal/os/replaced.txt keeps os_irqHandler.c out of the game's sources.
#include "hal/os/os_internal.h"
#include "nitro/os.h"
#include "nitro/svc.h"
#include "twl/scfg.h"

// TwlSDK's OS_SpinWaitSysCycles: waits for some cycles of the system clock. The SDK counts them in the ARM9's cycles,
// twice as many (four times at the DSi's speed), and takes the call's own 16 of those off
void OS_SpinWaitSysCycles(u32 cycles) {
    u32 shift = ((reg_SCFG_CLK & SCFG_CLK_CPU_2X) == 1) ? 2 : 1;
    u32 cpuCycles = cycles << shift;

    if (cpuCycles > 16) {
        port_osi_spin((cpuCycles - 16) >> shift);
    }
}

// The BIOS's WaitByLoop: count turns of a loop of four of the ARM9's cycles each (GBATEK, "BIOS Halt Functions"), so
// two of the system clock's
void SVC_WaitByLoop(u32 count) {
    port_osi_spin((u64)count * 2);
}

// NitroSDK's OS_WaitInterrupt: waits for one of the interrupts, after forgetting the ones that happened if clear is
// set. It halts the CPU with IME on, and lets the IRQ through for a moment every time the CPU wakes
void OS_WaitInterrupt(BOOL clear, u32 interrupts) {
    vu32 *check = port_osi_irq_check_word();
    u32 irq = OS_DisableInterrupts();
    BOOL ime = OS_EnableIrq();

    if (clear) {
        *check &= ~interrupts;
    }
    while (!(*check & interrupts)) {
        OS_Halt();
        OS_EnableInterrupts();
        OS_DisableInterrupts();
    }
    *check &= ~interrupts;
    OS_RestoreIrq(ime);
    OS_RestoreInterrupts(irq);
}

// NitroSDK's OS_WaitVBlankIntr
void OS_WaitVBlankIntr(void) {
    SVC_WaitByLoop(1);
    OS_WaitIrq(TRUE, OS_IE_V_BLANK);
}

// NitroSDK's OS_GetBootType: what the system left in the shared memory (port_os_run sets the boot from a card)
u32 OS_GetBootType(void) {
    return *(u16 *)HW_BOOT_CHECK_INFO_BUF;
}

// NitroSDK's OS_WaitIrq: sleeps until one of the interrupts has happened, after forgetting the interrupts that already
// did if clear is set. The IRQ handler's tail wakes every thread that sleeps here after each interrupt
void OS_WaitIrq(BOOL clear, u32 interrupts) {
    vu32 *check = port_osi_irq_check_word();
    u32 irq = OS_DisableInterrupts();

    if (clear) {
        *check &= ~interrupts;
    }
    OS_RestoreInterrupts(irq);
    while (!(interrupts & *check)) {
        OS_SleepThread(&OSi_IrqThreadQueue);
    }
}
