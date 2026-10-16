// NitroSDK OS: setting and changing the interrupt handlers and masks (os_interrupt.c). OS_SetIrqFunction and the
// callbacks of DMA channels and timers share os_irqTable.c's table.
#include "nitro/os.h"
#include "twl/os.h"

// The callbacks of the timers, which come after those of the DMA channels
#define OSi_TIMER_CALLBACK_INFO (data_0214c178 + OSi_IRQCALLBACK_NO_TIMER0)

// NitroSDK's OS_InitIrqTable: nobody waits for an interrupt yet
void func_02079d10(void) {
    OS_InitThreadQueue(&data_02fe00a0);
}

// NitroSDK's OS_SetIrqFunction, under swan's name. The interrupts of DMA channels and timers get a callback in
// OSi_IrqCallbackInfo, which their handlers call; the others go in the table of handlers
void CPU_SetIRQFunc(u32 interrupts, OSIrqFunction function) {
    int n;
    OSIrqCallbackInfo *info;

    for (n = 0; n < 32; n++) {
        if (interrupts & 1) {
            info = NULL;
            if (n >= OS_IE_DMA0_SHIFT && n <= OS_IE_DMA0_SHIFT + 3) {
                info = &data_0214c178[OSi_IRQCALLBACK_NO_DMA0 + n - OS_IE_DMA0_SHIFT];
            } else if (n >= OS_IE_NDMA0_SHIFT && n <= OS_IE_NDMA0_SHIFT + 3) {
                info = &data_0214c178[OSi_IRQCALLBACK_NO_NDMA0 + n - OS_IE_NDMA0_SHIFT];
            } else if (n >= OS_IE_TIMER0_SHIFT && n <= OS_IE_TIMER0_SHIFT + 3) {
                info = &data_0214c178[OSi_IRQCALLBACK_NO_TIMER0 + n - OS_IE_TIMER0_SHIFT];
            } else {
                data_02fe0020[n] = function;
            }
            if (info != NULL) {
                info->func = (OSIrqCallback)function;
                info->arg = NULL;
                info->enable = TRUE;
            }
        }
        interrupts >>= 1;
    }
}

// NitroSDK's OS_GetIrqFunction: the function of the first interrupt of the mask (not looking at the new DMA channels)
OSIrqFunction func_02079d88(u32 interrupts) {
    int n;
    OSIrqFunction *table = data_02fe0020;

    for (n = 0; n < 32; n++) {
        if (interrupts & 1) {
            if (n >= OS_IE_DMA0_SHIFT && n <= OS_IE_DMA0_SHIFT + 3) {
                return (OSIrqFunction)data_0214c178[OSi_IRQCALLBACK_NO_DMA0 + n - OS_IE_DMA0_SHIFT].func;
            }
            if (n >= OS_IE_TIMER0_SHIFT && n <= OS_IE_TIMER0_SHIFT + 3) {
                return (OSIrqFunction)data_0214c178[OSi_IRQCALLBACK_NO_TIMER0 + n - OS_IE_TIMER0_SHIFT].func;
            }
            return *table;
        }
        interrupts >>= 1;
        table++;
    }
    return NULL;
}

// NitroSDK's OSi_EnterDmaCallback, under swan's name: calls the function with the argument when the DMA channel's
// interrupt happens, enabling it for that if it was not enabled
void irq_setDMACallback(u32 dmaNo, OSIrqCallback callback, void *arg) {
    u32 mask = 1 << (dmaNo + OS_IE_DMA0_SHIFT);
    u32 enabled;

    data_0214c178[OSi_IRQCALLBACK_NO_DMA0 + dmaNo].func = callback;
    data_0214c178[OSi_IRQCALLBACK_NO_DMA0 + dmaNo].arg = arg;
    enabled = CPU_EnableInterrupts(mask);
    data_0214c178[OSi_IRQCALLBACK_NO_DMA0 + dmaNo].enable = mask & enabled;
}

// NitroSDK's OSi_EnterNDmaCallback, which the DSi's OS code in the LTD autoload implements
void func_02079e18(u32 ndmaNo, OSIrqCallback callback, void *arg) {
    func_027692b8(ndmaNo, callback, arg);
}

void OSi_EnterTimerCallback(int timerNo, OSIrqCallback callback, void *arg) {
    OSi_TIMER_CALLBACK_INFO[timerNo].func = callback;
    OSi_TIMER_CALLBACK_INFO[timerNo].arg = arg;
    CPU_EnableInterrupts(1 << (timerNo + OS_IE_TIMER0_SHIFT));
    OSi_TIMER_CALLBACK_INFO[timerNo].enable = TRUE;
}

// NitroSDK's OS_SetIrqMask, OS_EnableIrqMask, OS_DisableIrqMask and OS_ResetRequestIrqMask, under swan's names. Each
// keeps IME off while it changes IE or acknowledges IF
u32 CPU_SetInterrupts(u32 mask) {
    BOOL ime = OS_DisableIrq();
    u32 prev = reg_OS_IE;

    reg_OS_IE = mask;
    OS_RestoreIrq(ime);
    return prev;
}

u32 CPU_EnableInterrupts(u32 mask) {
    BOOL ime = OS_DisableIrq();
    u32 prev = reg_OS_IE;

    reg_OS_IE = prev | mask;
    OS_RestoreIrq(ime);
    return prev;
}

u32 CPU_DisableInterrupts(u32 mask) {
    BOOL ime = OS_DisableIrq();
    u32 prev = reg_OS_IE;

    reg_OS_IE = prev & ~mask;
    OS_RestoreIrq(ime);
    return prev;
}

u32 CPU_AcknowledgeInterrupts(u32 mask) {
    BOOL ime = OS_DisableIrq();
    u32 prev = reg_OS_IF;

    reg_OS_IF = mask;
    OS_RestoreIrq(ime);
    return prev;
}

// Puts the magic numbers that the exception handler checks at the top and at the end of the IRQ stack. The original
// subtracts the stack's size from the address in code, 8 bytes more than this constant, which looks like a symbol that
// the linker defines (SDK_IRQ_STACKSIZE)
void OS_SetIrqStackChecker(void) {
    *(u32 *)(HW_IRQ_STACK - 4) = 0xfddb597d;
    *(u32 *)HW_IRQ_STACK_END = 0x7bf9dd5b;
}
