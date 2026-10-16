// NitroSDK OS: the interrupt handler's file (os_irqHandler.c). Its assembly handlers, OS_IrqHandler (swan's
// CPU_IRQInvoke) and OS_IrqHandler_ThreadSwitch, are in ITCM; the one function of it in ARM9 main is OS_WaitIrq, which
// is ARM code (the library's default).
#include "nitro/os.h"

// NitroSDK's OS_WaitIrq, under swan's name: sleeps until one of the interrupts has happened, after forgetting the
// interrupts that already did if clear is set
void irq_waitFor(BOOL clear, u32 interrupts) {
    u32 irq = CPU_IRQDisable();

    if (clear) {
        *(vu32 *)HW_INTR_CHECK_BUF &= ~interrupts;
    }
    CPU_SetIRQMask(irq);
    while (!(interrupts & *(vu32 *)HW_INTR_CHECK_BUF)) {
        scheduler_yield(&data_02fe00a0);
    }
}
