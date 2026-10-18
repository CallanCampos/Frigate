#include "types.h"
#include "nitro/card.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "system/ir_check.h"

// The infrared chip's command for its ID, and the ID a genuine chip answers with
#define IR_COMMAND_ID 0x08
#define IR_ID 0xaa

// Where the bytes of a transfer come from and go to
typedef struct {
    u32 unk0;
    u8 *send;
    u8 *recv;
    u32 unkC;
} IrTransfer;

static u8 IrCheck_ReadId(void);
static void IrCheck_WaitBusy(void);
static void IrCheck_SetControl(u16 flags);
static u8 IrCheck_SendByte(IrTransfer *transfer);
static void IrCheck_RecvByte(IrTransfer *transfer);

static u16 sLockId = 0xfffd;
// MWCC leaves a function's 64-bit constant multiplies apart when it reads an object with an explicit alignment, as
// the original's IrCheck_ReadId does for its waits' 64 and 1000; lib/irc/src/irc_spi.c, the other copy of this
// driver, shows the same in IrcSpi_Recv and IrcSpi_Send. Which object was aligned, and to what, is a guess: the
// sections here start 32-aligned whatever it is, and that copy's data sits at 8- and 16-aligned addresses.
static IrTransfer sTransfer ATTRIBUTE_ALIGN(4);
static u8 sSendBuffer[0xd0];

// As the infrared library's IrcSpi_Wait60us (lib/irc/src/irc_spi.c), inlined here
static inline void IrCheck_Wait60us(void) {
    u64 start = OS_GetTick();

    while (OS_TicksToMicroSeconds(OS_GetTick() - start) < 60) {
    }
}

static u8 IrCheck_ReadId(void) {
    u8 id;

    sTransfer.send = sSendBuffer;
    sTransfer.recv = &id;
    sSendBuffer[0] = IR_COMMAND_ID;
    CARD_LockBackup(sLockId);
    OS_LockCard(sLockId);

    IrCheck_Wait60us();
    IrCheck_WaitBusy();
    IrCheck_SetControl(REG_MI_MCCNT0_MODE_MASK | 0x2);
    id = IrCheck_SendByte(&sTransfer);

    IrCheck_Wait60us();
    IrCheck_SetControl(0x2);
    IrCheck_RecvByte(&sTransfer);
    IrCheck_WaitBusy();

    OS_UnLockCard(sLockId);
    CARD_UnlockBackup(sLockId);
    return id;
}

static void IrCheck_WaitBusy(void) {
    while (reg_MI_MCCNT0 & REG_MI_MCCNT0_BUSY_MASK) {
    }
}

static void IrCheck_SetControl(u16 flags) {
    reg_MI_MCCNT0 = REG_MI_MCCNT0_E_MASK | REG_MI_MCCNT0_SEL_MASK | flags;
}

static u8 IrCheck_SendByte(IrTransfer *transfer) {
    u64 start;
    // Read from the data register into a volatile copy, which the original keeps on the stack
    vu16 data;

    start = OS_GetTick();
    while (OS_TicksToMicroSeconds(OS_GetTick() - start) < 50) {
    }
    reg_MI_MCD0 = *transfer->send;
    transfer->send++;
    IrCheck_WaitBusy();
    data = reg_MI_MCD0;
    return data;
}

static void IrCheck_RecvByte(IrTransfer *transfer) {
    u64 start;

    start = OS_GetTick();
    while (OS_TicksToMicroSeconds(OS_GetTick() - start) < 50) {
    }
    reg_MI_MCD0 = 0;
    IrCheck_WaitBusy();
    *transfer->recv = reg_MI_MCD0;
    transfer->recv++;
}

BOOL IrCheck_IsGenuineCard(void) {
    IrCheck_ReadId();
    if (IrCheck_ReadId() == IR_ID) {
        return TRUE;
    }
    return FALSE;
}
