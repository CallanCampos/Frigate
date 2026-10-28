// NitroSDK OS: the callbacks of the interrupts that DMA channels, the DSi's new DMA channels and timers raise
// (os_irqTable.c). Their handlers in DTCM's table all call OSi_IrqCallback with their index in OSi_IrqCallbackInfo.
#include "nitro/os.h"

static void OSi_IrqCallback(int index);

// The handlers and the queue that OS_WaitIrq sleeps on are in DTCM, where the IRQ handler reaches them directly.
// clang-format off
#pragma define_section DTCM ".dtcm.data" ".dtcm.bss" ".dtcm.data" abs32 RWX
// clang-format on
#pragma section DTCM begin

OSIrqFunction OS_IRQTable[32] = {
    OS_IrqDummy, OS_IrqDummy, OS_IrqDummy, OSi_IrqTimer0, OSi_IrqTimer1, OSi_IrqTimer2, OSi_IrqTimer3, OS_IrqDummy,
    OSi_IrqDma0, OSi_IrqDma1, OSi_IrqDma2, OSi_IrqDma3,   OS_IrqDummy,   OS_IrqDummy,   OS_IrqDummy,   OS_IrqDummy,
    OS_IrqDummy, OS_IrqDummy, OS_IrqDummy, OS_IrqDummy,   OS_IrqDummy,   OS_IrqDummy,   OS_IrqDummy,   OS_IrqDummy,
    OS_IrqDummy, OS_IrqDummy, OS_IrqDummy, OS_IrqDummy,   OSi_IrqNDma0,  OSi_IrqNDma1,  OSi_IrqNDma2,  OSi_IrqNDma3,
};

OSThreadQueue OSi_IrqThreadQueue;

#pragma section DTCM end

// The interrupt bit that each callback is for: DMA0 to 3, then NDMA0 to 3, then Timer0 to 3
static u16 OSi_IrqCallbackInfoIndex[OSi_IRQCALLBACK_NUM] = {
    REG_OS_IE_D0_SHIFT, REG_OS_IE_D0_SHIFT + 1, REG_OS_IE_D0_SHIFT + 2, REG_OS_IE_D0_SHIFT + 3,
    OS_IE_NDMA0_SHIFT,  OS_IE_NDMA0_SHIFT + 1,  OS_IE_NDMA0_SHIFT + 2,  OS_IE_NDMA0_SHIFT + 3,
    REG_OS_IE_T0_SHIFT, REG_OS_IE_T0_SHIFT + 1, REG_OS_IE_T0_SHIFT + 2, REG_OS_IE_T0_SHIFT + 3,
};

// NitroSDK's OSi_IrqCallbackInfo: the callback and argument of each, which OSi_IrqCallback clears when it runs it
OSIrqCallbackInfo OSi_IrqCallbackInfo[OSi_IRQCALLBACK_NUM];

// NitroSDK's OS_IrqDummy: the handler of the interrupts that nothing handles
void OS_IrqDummy(void) {
}

static void OSi_IrqCallback(int index) {
    u32 mask = 1 << OSi_IrqCallbackInfoIndex[index];
    OSIrqCallback func = OSi_IrqCallbackInfo[index].func;

    OSi_IrqCallbackInfo[index].func = NULL;
    if (func != NULL) {
        func(OSi_IrqCallbackInfo[index].arg);
    }
    OS_SetIrqCheckFlag(mask);
    if (OSi_IrqCallbackInfo[index].enable == FALSE) {
        OS_DisableIrqMask(mask);
    }
}

// NitroSDK's OSi_IrqDma0 to 3
void OSi_IrqDma0(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_DMA0);
}

void OSi_IrqDma1(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_DMA0 + 1);
}

void OSi_IrqDma2(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_DMA0 + 2);
}

void OSi_IrqDma3(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_DMA0 + 3);
}

// NitroSDK's OSi_IrqTimer0 to 3
void OSi_IrqTimer0(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_TIMER0);
}

void OSi_IrqTimer1(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_TIMER0 + 1);
}

void OSi_IrqTimer2(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_TIMER0 + 2);
}

void OSi_IrqTimer3(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_TIMER0 + 3);
}

// NitroSDK's OSi_IrqNDma0 to 3
void OSi_IrqNDma0(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_NDMA0);
}

void OSi_IrqNDma1(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_NDMA0 + 1);
}

void OSi_IrqNDma2(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_NDMA0 + 2);
}

void OSi_IrqNDma3(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_NDMA0 + 3);
}
