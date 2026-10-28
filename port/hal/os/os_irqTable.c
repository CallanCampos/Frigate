// NitroSDK's interrupt table and the handlers of the interrupts that have callbacks (os_irqTable.c), on the host.
//
// Upstream now defines OS_IRQTable and OSi_IrqThreadQueue in DTCM C. The host still owns the IRQ callback path
// and uses its whole-address DTCM flag below. The table agrees with the recovered SDK initializer: OS_IrqDummy everywhere but the four timers (OSi_IrqTimer0 to 3,
// entries 3 to 6), the four DMA channels (OSi_IrqDma0 to 3, entries 8 to 11) and the DSi's four new DMA channels
// (OSi_IrqNDma0 to 3, entries 28 to 31).
//
// The rest is the decomp's lib/nitro/src/os/os_irqTable.c, which port/hal/os/replaced.txt keeps out of the game's
// sources, with one change: OSi_IrqCallback sets OS_SetIrqCheckFlag's word through the host's DTCM. The SDK's inline
// OS_SetIrqCheckFlag reaches it through HW_DTCM, which os.h defines as (u32)SDK_AUTOLOAD_DTCM_START: on a 64-bit host
// that truncates the address, and the first timer or DMA interrupt would fault. Once the host variant of os.h keeps
// HW_DTCM whole (port/tools/gen_host_tree.py), this file can go back to being the decomp's.
#include "hal/os/os_internal.h"
#include "nitro/os.h"
#include <string.h>

static void OSi_IrqCallback(int index);

// The interrupt bit that each callback is for: DMA0 to 3, then NDMA0 to 3, then Timer0 to 3
static u16 OSi_IrqCallbackInfoIndex[OSi_IRQCALLBACK_NUM] = {
    REG_OS_IE_D0_SHIFT,   REG_OS_IE_D0_SHIFT + 1,   REG_OS_IE_D0_SHIFT + 2,   REG_OS_IE_D0_SHIFT + 3,
    OS_IE_NDMA0_SHIFT,  OS_IE_NDMA0_SHIFT + 1,  OS_IE_NDMA0_SHIFT + 2,  OS_IE_NDMA0_SHIFT + 3,
    REG_OS_IE_T0_SHIFT, REG_OS_IE_T0_SHIFT + 1, REG_OS_IE_T0_SHIFT + 2, REG_OS_IE_T0_SHIFT + 3,
};

// NitroSDK's OSi_IrqCallbackInfo: the callback and argument of each, which OSi_IrqCallback clears when it runs it
OSIrqCallbackInfo OSi_IrqCallbackInfo[OSi_IRQCALLBACK_NUM];

// The table of the 32 interrupts' handlers and the queue of the threads that sleep in OS_WaitIrq, in DTCM on the DS
OSIrqFunction OS_IRQTable[32];
OSThreadQueue OSi_IrqThreadQueue;

// The table as the DS's DTCM starts with it
static const OSIrqFunction sInitialIRQTable[32] = {
    OS_IrqDummy, OS_IrqDummy, OS_IrqDummy, OSi_IrqTimer0, OSi_IrqTimer1, OSi_IrqTimer2, OSi_IrqTimer3, OS_IrqDummy,
    OSi_IrqDma0, OSi_IrqDma1, OSi_IrqDma2, OSi_IrqDma3,   OS_IrqDummy,   OS_IrqDummy,   OS_IrqDummy,   OS_IrqDummy,
    OS_IrqDummy, OS_IrqDummy, OS_IrqDummy, OS_IrqDummy,   OS_IrqDummy,   OS_IrqDummy,   OS_IrqDummy,   OS_IrqDummy,
    OS_IrqDummy, OS_IrqDummy, OS_IrqDummy, OS_IrqDummy,   OSi_IrqNDma0,  OSi_IrqNDma1,  OSi_IrqNDma2,  OSi_IrqNDma3,
};

void port_osi_reset_irq_table(void) {
    memcpy(OS_IRQTable, sInitialIRQTable, sizeof(OS_IRQTable));
    OSi_IrqThreadQueue.head = NULL;
    OSi_IrqThreadQueue.tail = NULL;
    memset(OSi_IrqCallbackInfo, 0, sizeof(OSi_IrqCallbackInfo));
}

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
    *port_osi_irq_check_word() |= mask;
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
