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
static IrTransfer sTransfer;
static u8 sSendBuffer[0xd0];

static u8 IrCheck_ReadId(void) {
    u8 id;
    u64 start;

    sTransfer.send = sSendBuffer;
    sTransfer.recv = &id;
    sSendBuffer[0] = IR_COMMAND_ID;
    CARD_LockBackup(sLockId);
    OS_LockCard(sLockId);

    start = OS_GetTick();
    while (OS_TicksToMicroSeconds(OS_GetTick() - start) < 60) {
    }
    IrCheck_WaitBusy();
    IrCheck_SetControl(REG_MI_MCCNT0_MODE_MASK | 0x2);
    id = IrCheck_SendByte(&sTransfer);

    start = OS_GetTick();
    while (OS_TicksToMicroSeconds(OS_GetTick() - start) < 60) {
    }
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
