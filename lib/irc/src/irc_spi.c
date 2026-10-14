// Overlay 31, the last of its four files (see net_devirc.c): the SPI bus to the card's infrared chip, which it talks
// to through the backup SPI bus (as src/ov338/ir_check.c does). A transfer is a command byte, 1 to read and 2 to
// write, and then the bytes, with a pause of 60 microseconds between. Built at -O2,p as part of lib/irc (see its
// library.toml), whose name is ours.
#include "irc_spi.h"
#include "nitro/card.h"
#include "nitro/hw.h"
#include "nitro/os.h"

// The most bytes the chip answers with
#define IRC_RECV_MAX 0xb5

// Where the bytes of a transfer come from and go to
typedef struct {
    u32 unk0;
    const u8 *send;
    u8 *recv;
    u32 unkC;
} IrcSpiData;

static void IrcSpi_Wait60us(void);
static u8 IrcSpi_SendByte(IrcSpiData *transfer);
static void IrcSpi_RecvByte(IrcSpiData *transfer);

static u16 IrcLockId = 0xfffd;
static IrcSpiData IrcSpi;
static u8 IrcReadCmd[0xb8];
static u8 IrcWriteCmd[0xc8];

static void IrcSpi_Wait60us(void) {
    u64 start = clock();

    while (OS_TicksToMicroSeconds(clock() - start) < 60) {
    }
}

void IrcSpi_Init(void) {
    IrcSpi.unk0 = 1;
    if (IrcLockId == 0xfffd) {
        IrcLockId = cart_key_create();
    }
}

void IrcSpi_Release(void) {
    if (IrcLockId != OS_LOCK_ID_ERROR) {
        cart_key_release(IrcLockId);
        IrcLockId = 0xfffd;
    }
}

static u8 IrcSpi_SendByte(IrcSpiData *transfer) {
    u64 start;
    // Read from the data register into a volatile copy, which the original keeps on the stack
    vu16 data;

    start = clock();
    while (OS_TicksToMicroSeconds(clock() - start) < 50) {
    }
    reg_MI_MCD0 = *transfer->send;
    transfer->send++;
    while (reg_MI_MCCNT0 & REG_MI_MCCNT0_BUSY_MASK) {
    }
    data = reg_MI_MCD0;
    return data;
}

static void IrcSpi_RecvByte(IrcSpiData *transfer) {
    u64 start = clock();

    while (OS_TicksToMicroSeconds(clock() - start) < 50) {
    }
    reg_MI_MCD0 = 0;
    while (reg_MI_MCCNT0 & REG_MI_MCCNT0_BUSY_MASK) {
    }
    *transfer->recv = reg_MI_MCD0;
    transfer->recv++;
}

u8 IrcSpi_Recv(u8 *buf) {
    u8 size;
    int i;

    IrcSpi.send = IrcReadCmd;
    IrcSpi.recv = buf;
    IrcReadCmd[0] = 1;
    buf[0] = 0;
    func_0206ef4c(IrcLockId);
    func_0207a178(IrcLockId);
    while (reg_MI_MCCNT0 & REG_MI_MCCNT0_BUSY_MASK) {
    }
    reg_MI_MCCNT0 = REG_MI_MCCNT0_E_MASK | REG_MI_MCCNT0_SEL_MASK | REG_MI_MCCNT0_MODE_MASK | 0x2;
    IrcSpi_SendByte(&IrcSpi);
    IrcSpi_Wait60us();
    IrcSpi_RecvByte(&IrcSpi);
    IrcSpi_Wait60us();
    size = buf[0];
    if (size > IRC_RECV_MAX) {
        size = 0;
    }
    IrcSpi.send = NULL;
    IrcSpi.recv = buf;
    buf[0] = 0;
    if (size != 0) {
        for (i = 0; i < size; i++) {
            if (i == size - 1) {
                reg_MI_MCCNT0 = REG_MI_MCCNT0_E_MASK | REG_MI_MCCNT0_SEL_MASK | 0x2;
            }
            IrcSpi_RecvByte(&IrcSpi);
        }
    } else {
        reg_MI_MCCNT0 = REG_MI_MCCNT0_E_MASK | REG_MI_MCCNT0_SEL_MASK | 0x2;
        IrcSpi_RecvByte(&IrcSpi);
    }
    for (i = 0; i < size; i++) {
        buf[i] ^= IRC_XOR;
    }
    while (reg_MI_MCCNT0 & REG_MI_MCCNT0_BUSY_MASK) {
    }
    func_0207a1a0(IrcLockId);
    func_0206ef58(IrcLockId);
    return size;
}

void IrcSpi_Send(const u8 *data, int size) {
    int i;

    IrcSpi.send = IrcWriteCmd;
    IrcSpi.recv = NULL;
    IrcWriteCmd[0] = 2;
    func_0206ef4c(IrcLockId);
    func_0207a178(IrcLockId);
    while (reg_MI_MCCNT0 & REG_MI_MCCNT0_BUSY_MASK) {
    }
    reg_MI_MCCNT0 = REG_MI_MCCNT0_E_MASK | REG_MI_MCCNT0_SEL_MASK | REG_MI_MCCNT0_MODE_MASK | 0x2;
    IrcSpi_SendByte(&IrcSpi);
    IrcSpi_Wait60us();
    IrcSpi.send = data;
    for (i = 0; i < size; i++) {
        if (i == size - 1) {
            reg_MI_MCCNT0 = REG_MI_MCCNT0_E_MASK | REG_MI_MCCNT0_SEL_MASK | 0x2;
        }
        IrcSpi_SendByte(&IrcSpi);
    }
    while (reg_MI_MCCNT0 & REG_MI_MCCNT0_BUSY_MASK) {
    }
    func_0207a1a0(IrcLockId);
    func_0206ef58(IrcLockId);
}
