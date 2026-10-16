// NitroSDK OS: the callbacks of the interrupts that DMA channels, the DSi's new DMA channels and timers raise
// (os_irqTable.c). Their handlers in DTCM's table all call OSi_IrqCallback with their index in OSi_IrqCallbackInfo.
#include "nitro/os.h"

static void OSi_IrqCallback(int index);

// The interrupt bit that each callback is for: DMA0 to 3, then NDMA0 to 3, then Timer0 to 3
static u16 OSi_IrqCallbackInfoIndex[OSi_IRQCALLBACK_NUM] = {
    OS_IE_DMA0_SHIFT,   OS_IE_DMA0_SHIFT + 1,   OS_IE_DMA0_SHIFT + 2,   OS_IE_DMA0_SHIFT + 3,
    OS_IE_NDMA0_SHIFT,  OS_IE_NDMA0_SHIFT + 1,  OS_IE_NDMA0_SHIFT + 2,  OS_IE_NDMA0_SHIFT + 3,
    OS_IE_TIMER0_SHIFT, OS_IE_TIMER0_SHIFT + 1, OS_IE_TIMER0_SHIFT + 2, OS_IE_TIMER0_SHIFT + 3,
};

// NitroSDK's OSi_IrqCallbackInfo: the callback and argument of each, which OSi_IrqCallback clears when it runs it
OSIrqCallbackInfo data_0214c178[OSi_IRQCALLBACK_NUM];

// NitroSDK's OS_IrqDummy: the handler of the interrupts that nothing handles
void func_02079c24(void) {
}

static void OSi_IrqCallback(int index) {
    u32 mask = 1 << OSi_IrqCallbackInfoIndex[index];
    OSIrqCallback func = data_0214c178[index].func;

    data_0214c178[index].func = NULL;
    if (func != NULL) {
        func(data_0214c178[index].arg);
    }
    OS_SetIrqCheckFlag(mask);
    if (data_0214c178[index].enable == FALSE) {
        CPU_DisableInterrupts(mask);
    }
}

// NitroSDK's OSi_IrqDma0 to 3
void func_02079c80(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_DMA0);
}

void func_02079c8c(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_DMA0 + 1);
}

void func_02079c98(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_DMA0 + 2);
}

void func_02079ca4(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_DMA0 + 3);
}

// NitroSDK's OSi_IrqTimer0 to 3
void func_02079cb0(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_TIMER0);
}

void func_02079cbc(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_TIMER0 + 1);
}

void func_02079cc8(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_TIMER0 + 2);
}

void func_02079cd4(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_TIMER0 + 3);
}

// NitroSDK's OSi_IrqNDma0 to 3
void func_02079ce0(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_NDMA0);
}

void func_02079cec(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_NDMA0 + 1);
}

void func_02079cf8(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_NDMA0 + 2);
}

void func_02079d04(void) {
    OSi_IrqCallback(OSi_IRQCALLBACK_NO_NDMA0 + 3);
}
