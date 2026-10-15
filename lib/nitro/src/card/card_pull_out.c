// Card removal notification and shutdown. The source file name follows NitroSDK's CARD_pullOut.c.
#include "nitro/card.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/pxi.h"

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
    func_0207c9fc(14, func_0206fefc);
    sCardPullOut.callback = NULL;
}

static void func_0206fefc(u32 tag, u32 data, BOOL err) {
    u32 command = data & 0x3f;
    if (command == 0x11) {
        if (!sCardPullOut.pulledOut) {
            BOOL terminate = TRUE;
            sCardPullOut.pulledOut = TRUE;
            func_0206f0f0(TRUE, FALSE);
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
        func_0206f0f0(2, 0);
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
        func_0207ef8c();
    }
    if (hw_isDSi()) {
        func_0206ffac();
    }
    func_0206fffc(1, 1);
    dma_endAll();
    if (hw_isDSi()) {
        func_0206ffbc();
    }
    sys_exit();
}

// The NOP is the ROM's alignment halfword before the following ARM entry point.
asm void func_0206ffac(void) {
    bx pc
    nop
}

#pragma thumb off
asm void func_0206ffb0(void) {
    ldr ip, [pc, #0]
    bx ip
    dcd 0x02769579
}
#pragma thumb on

// As above, the NOP fills the remaining halfword before ARM code.
asm void func_0206ffbc(void) {
    bx pc
    nop
}

#pragma thumb off
asm void func_0206ffc0(void) {
    ldr ip, [pc, #0]
    bx ip
    dcd 0x02768299
}
#pragma thumb on

void func_0206ffcc(u32 cardId) {
    volatile u32 bootCardId = *(vu32 *)0x02fffc00;
    if (cardId != bootCardId) {
        u32 irq = CPU_IRQDisable();
        func_0206fefc(14, 0x11, FALSE);
        CPU_SetIRQMask(irq);
    }
}

static void func_0206fffc(u32 data, u32 wait) {
    while (PXI_SendWordByFifo(14, data, FALSE) != 0) {
        WaitByLoop(wait);
    }
}
