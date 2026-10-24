// TwlSDK's DSi part of os_interrupt.c (ARM9 main), in the LTD autoload: the callback of a new DMA channel's interrupt,
// which main's OSi_EnterNDmaCallback calls. The file name is inferred from main's half; the function's name is ours
#include "nitro/os.h"
#include "twl/os.h"

#pragma define_section LTDMAIN ".ltdmain"                                                                              \
                               ".ltdmain.bss"                                                                          \
                               ".ltdmain" abs32 RWX
#pragma section LTDMAIN begin

void OSi_EnterNDmaCallbackCore(u32 ndmaNo, OSIrqCallback callback, void *arg) {
    u32 mask = 1 << (OS_IE_NDMA0_SHIFT + ndmaNo);
    u32 enabled;

    (OSi_IrqCallbackInfo + OSi_IRQCALLBACK_NO_NDMA0 + ndmaNo)->func = callback;
    (OSi_IrqCallbackInfo + OSi_IRQCALLBACK_NO_NDMA0 + ndmaNo)->arg = arg;
    enabled = OS_EnableIrqMask(mask);
    (OSi_IrqCallbackInfo + OSi_IRQCALLBACK_NO_NDMA0 + ndmaNo)->enable = mask & enabled;
}

#pragma section LTDMAIN end
