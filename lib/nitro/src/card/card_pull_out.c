// Card removal notification and shutdown. The source file name follows NitroSDK's CARD_pullOut.c.
#include "nitro/card.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/pxi.h"
#include "twl/mi.h"
#include "twl/os.h"

typedef struct {
    u32 notificationCount;
    BOOL pulledOut;
    CARDPulledOutCallback callback;
} CARDPullOutState;

static CARDPullOutState sCardPullOut;

static void func_0206fefc(u32 tag, u32 data, BOOL err);
static void func_0206ff68(void);
static void func_0206fffc(u32 data, u32 wait);

void func_0206fed8(void) {
    PXI_Init();
    sCardPullOut.notificationCount = 0;
    sCardPullOut.pulledOut = FALSE;
    PXI_SetFifoRecvCallback(14, func_0206fefc);
    sCardPullOut.callback = NULL;
}

static void func_0206fefc(u32 tag, u32 data, BOOL err) {
    u32 command = data & 0x3f;
    if (command == 0x11) {
        if (!sCardPullOut.pulledOut) {
            BOOL terminate = TRUE;
            sCardPullOut.pulledOut = TRUE;
            CARDi_NotifyEvent(TRUE, FALSE);
            if (sCardPullOut.callback != NULL) {
                terminate = sCardPullOut.callback();
            }
            if (terminate) {
                func_0206ff68();
            }
        }
    } else if (command == 2) {
        ++sCardPullOut.notificationCount;
        sCardPullOut.pulledOut = FALSE;
        CARDi_NotifyEvent(2, 0);
    } else {
        sys_exit();
    }
}

void func_0206ff50(CARDPulledOutCallback callback) {
    sCardPullOut.callback = callback;
}

BOOL func_0206ff5c(void) {
    return sCardPullOut.pulledOut;
}

static void func_0206ff68(void) {
    if (PAD_DetectFold()) {
        PM_ForceToPowerOff();
    }
    // The calls into the LTD autoload go through veneers, which the linker adds after this function
    if (hw_isDSi()) {
        func_02769578();
    }
    func_0206fffc(1, 1);
    MI_StopAllDma();
    if (hw_isDSi()) {
        func_02768298();
    }
    sys_exit();
}

void func_0206ffcc(u32 cardId) {
    // volatile: the boot card ID in the shared header is written by the ARM7, and the original reads it into a stack
    // slot
    volatile u32 bootCardId = *(vu32 *)0x02fffc00;
    if (cardId != bootCardId) {
        u32 irq = OS_DisableInterrupts();
        func_0206fefc(14, 0x11, FALSE);
        OS_RestoreInterrupts(irq);
    }
}

static void func_0206fffc(u32 data, u32 wait) {
    while (PXI_SendWordByFifo(14, data, FALSE) != 0) {
        WaitByLoop(wait);
    }
}
