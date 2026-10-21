// TwlSDK's NWM, the DSi's wireless manager, ARM9 side, its setup: NWM_Init sets the library up with the system area
// that the game gives it. ARM code in the DSi-only LTD autoload; the file's name is ours

#include "types.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/pxi.h"
#include "nitro/wm.h"
#include "nwm_private.h"
#include "twl/nwm.h"

static NWMRetCode func_02702468(NWMSys *sys, u32 size, u32 dmaNo);

// The zero initializer gives the flag a second literal for its address, as the original has: func_02702468 reads and
// stores it through one literal and passes DC_StoreRange its address through another
u8 NWMi_SetUpDone = FALSE;

NWMRetCode NWM_Init(void *systemBuffer, u32 size, u32 dmaNo) {
    if (!func_027026b4()) {
        return NWM_RETCODE_UNAVAILABLE;
    }
    return func_02702468(systemBuffer, size, dmaNo);
}

static NWMRetCode func_02702468(NWMSys *sys, u32 size, u32 dmaNo) {
    u32 irq;
    int i;

    irq = OS_DisableInterrupts();
    if (WM_CheckInitialized() == 0) {
        OS_RestoreInterrupts(irq);
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    if (NWMi_SetUpDone) {
        OS_RestoreInterrupts(irq);
        return NWM_RETCODE_ILLEGAL_STATE;
    }
    if (sys == NULL) {
        OS_RestoreInterrupts(irq);
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (dmaNo > 3) {
        OS_RestoreInterrupts(irq);
        return NWM_RETCODE_INVALID_PARAM;
    }
    if (((u32)sys & 0x1f) != 0) {
        OS_RestoreInterrupts(irq);
        return NWM_RETCODE_INVALID_PARAM;
    }
    PXI_Init();
    if (!PXI_IsCallbackReady(PXI_FIFO_TAG_NWM, PXI_PROC_ARM7)) {
        OS_RestoreInterrupts(irq);
        return NWM_RETCODE_UNAVAILABLE;
    }
    DC_InvalidateRange(sys, size);
    MIi_DmaFill32(dmaNo, sys, 0, size, TRUE);
    NWMi_SystemArea = sys;
    DC_StoreRange(&NWMi_SystemArea, 4);
    NWMi_SystemArea->info = (NWMInfo *)(NWMi_SystemArea + 1);
    NWMi_SystemArea->status = (NWMStatus *)(NWMi_SystemArea->info + 1);
    NWMi_SystemArea->event = (NWMEvent *)(NWMi_SystemArea->status + 1);
    func_027026ec();
    NWMi_SystemArea->dmaNo = dmaNo;
    OS_InitMessageQueue(&NWMi_SystemArea->freeQueue, NWMi_SystemArea->freeMsgs, NWM_CMD_BUF_COUNT);
    for (i = 0; i < NWM_CMD_BUF_COUNT; i++) {
        NWMi_SystemArea->cmdBuf[i].cmd = NWM_CMD_BUF_FREE;
        DC_StoreRange(&NWMi_SystemArea->cmdBuf[i], 2);
        OS_SendMessage(&NWMi_SystemArea->freeQueue, &NWMi_SystemArea->cmdBuf[i], OS_MESSAGE_BLOCK);
    }
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_NWM, func_02702740);
    MI_CpuFill8(NWMi_SystemArea->unk_2120, 0, sizeof(NWMi_SystemArea->unk_2120));
    NWMi_SetUpDone = TRUE;
    NWMi_SystemArea->status->state = NWM_STATE_INITIALIZED;
    DC_StoreRange(NWMi_SystemArea, sizeof(NWMSys));
    DC_StoreRange(&NWMi_SetUpDone, 1);
    OS_RestoreInterrupts(irq);
    return NWM_RETCODE_SUCCESS;
}

NWMRetCode func_02702698(void) {
    return NWMi_SetUpDone ? NWM_RETCODE_SUCCESS : NWM_RETCODE_ILLEGAL_STATE;
}

// Whether the wireless is available and not turned off
BOOL func_027026b4(void) {
    if (OS_IsAvailableWireless() == TRUE && !OS_IsForceDisableWireless()) {
        return TRUE;
    }
    return FALSE;
}
