// NitroSDK's card removal notification and shutdown. The source file name follows NitroSDK's CARD_pullOut.c, and the
// names are the SDK's.
#include "nitro/card.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/pxi.h"
#include "nitro/svc.h"
#include "twl/mi.h"
#include "twl/os.h"
#include "twl/pm.h"

// The commands of the card's PXI tag: the ARM7 sends PULLED_OUT and RESET_SLOT, and the ARM9 sends TERMINATE
#define CARD_PXI_COMMAND_MASK 0x3f
#define CARD_PXI_COMMAND_TERMINATE 0x01
#define CARD_PXI_COMMAND_RESET_SLOT 0x02
#define CARD_PXI_COMMAND_PULLED_OUT 0x11

typedef struct {
    u32 notificationCount;
    BOOL pulledOut;
    CARDPulledOutCallback callback;
} CARDPullOutState;

static CARDPullOutState sCardPullOut;

static void CARDi_PulledOutCallback(u32 tag, u32 data, BOOL err);
static void CARD_TerminateForPulledOut(void);
static void CARDi_SendtoPxi(u32 data, u32 wait);

void CARD_InitPulledOutCallback(void) {
    PXI_Init();
    sCardPullOut.notificationCount = 0;
    sCardPullOut.pulledOut = FALSE;
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_CARD, CARDi_PulledOutCallback);
    sCardPullOut.callback = NULL;
}

static void CARDi_PulledOutCallback(u32 tag, u32 data, BOOL err) {
    u32 command = data & CARD_PXI_COMMAND_MASK;
    if (command == CARD_PXI_COMMAND_PULLED_OUT) {
        if (!sCardPullOut.pulledOut) {
            BOOL terminate = TRUE;
            sCardPullOut.pulledOut = TRUE;
            CARDi_NotifyEvent(CARD_EVENT_PULLED_OUT, 0);
            if (sCardPullOut.callback != NULL) {
                terminate = sCardPullOut.callback();
            }
            if (terminate) {
                CARD_TerminateForPulledOut();
            }
        }
    } else if (command == CARD_PXI_COMMAND_RESET_SLOT) {
        ++sCardPullOut.notificationCount;
        sCardPullOut.pulledOut = FALSE;
        CARDi_NotifyEvent(CARD_EVENT_SLOT_RESET, 0);
    } else {
        OS_Terminate();
    }
}

void CARD_SetPulledOutCallback(CARDPulledOutCallback callback) {
    sCardPullOut.callback = callback;
}

BOOL CARD_IsPulledOut(void) {
    return sCardPullOut.pulledOut;
}

static void CARD_TerminateForPulledOut(void) {
    if (PAD_DetectFold()) {
        PM_ForceToPowerOff();
    }
    // The calls into the LTD autoload go through veneers, which the linker adds after this function
    if (OS_IsRunOnTwl()) {
        PMi_ExecutePostExitCallbackList();
    }
    CARDi_SendtoPxi(CARD_PXI_COMMAND_TERMINATE, 1);
    MI_StopAllDma();
    if (OS_IsRunOnTwl()) {
        MI_StopNDmaChannels();
    }
    OS_Terminate();
}

void CARDi_CheckPulledOutCore(u32 cardId) {
    // volatile: the boot card ID in the shared work is written by the ARM7, and the original reads it into a stack slot
    volatile u32 bootCardId = OS_GetSystemWork()->bootCardId;
    if (cardId != bootCardId) {
        u32 irq = OS_DisableInterrupts();
        CARDi_PulledOutCallback(PXI_FIFO_TAG_CARD, CARD_PXI_COMMAND_PULLED_OUT, FALSE);
        OS_RestoreInterrupts(irq);
    }
}

static void CARDi_SendtoPxi(u32 data, u32 wait) {
    while (PXI_SendWordByFifo(PXI_FIFO_TAG_CARD, data, FALSE) != 0) {
        SVC_WaitByLoop(wait);
    }
}
