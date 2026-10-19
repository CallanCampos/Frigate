#include "dwc_util_irq.h"
#include "dwc_util_shared.h"
#include "nitro/hw.h"
#include "nitro/os.h"

// The game's interrupt mask and VBlank handler
typedef struct DWCUtilIrq {
    OSIrqFunction prevVBlank;
    u32 prevMask;
} DWCUtilIrq;

static DWCUtilIrq sIrq;

void func_ov182_021bcaac(void) {
    sIrq.prevMask = reg_OS_IE;
    OS_SetIrqMask(OS_IE_TIMER0 | OS_IE_TIMER1 | OS_IE_FIFO_RECV);
    OS_EnableIrqMask(OS_IE_V_BLANK);
    sIrq.prevVBlank = OS_GetIrqFunction(OS_IE_V_BLANK);
    OS_SetIrqFunction(OS_IE_V_BLANK, func_ov182_021bcb1c);
    OS_ResetRequestIrqMask(OS_IE_V_BLANK);
    OS_EnableIrq();
    OS_EnableInterrupts();
}

void func_ov182_021bcaf8(void) {
    OS_DisableIrq();
    OS_SetIrqMask(sIrq.prevMask);
    OS_SetIrqFunction(OS_IE_V_BLANK, sIrq.prevVBlank);
}

void func_ov182_021bcb1c(void) {
    func_ov182_021bd19c(1);
    OS_SetIrqCheckFlag(OS_IE_V_BLANK);
}
