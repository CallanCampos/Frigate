// NitroSDK OS: the interrupt handler's file (os_irqHandler.c). Its assembly handlers, OS_IrqHandler (swan's
// CPU_IRQInvoke) and OS_IrqHandler_ThreadSwitch, are in ITCM, in os_irqHandler_itcm.c; the one function of it in ARM9
// main is OS_WaitIrq, which is ARM code (the library's default). The file is complete in both versions.
#include "nitro/os.h"

// NitroSDK's OS_WaitIrq: sleeps until one of the interrupts has happened, after forgetting the
// interrupts that already did if clear is set
void OS_WaitIrq(BOOL clear, OSIrqMask interrupts) {
    OSIntrMode irq = OS_DisableInterrupts();

    if (clear) {
        *(vu32 *)HW_INTR_CHECK_BUF &= ~interrupts;
    }
    OS_RestoreInterrupts(irq);
    while (!(interrupts & *(vu32 *)HW_INTR_CHECK_BUF)) {
        OS_SleepThread(&OSi_IrqThreadQueue);
    }
}
