#include "types.h"
#include "nitro/os.h"
#include "twl/dsp.h"
#include "twl/scfg.h"

// TwlSDK's DSP library, the register interface (guessed file name, after the SDK's dsp_if.c): the DSP's power, its
// reset and the ports of SCFG and of the DSP's own registers, through which the other DSP files talk to it. ARM code in
// the DSi-only LTD autoload. The ROM names none of it; the functions have the SDK's names where twl/dsp.h knows them,
// and keep their default names otherwise.

static void func_02704f9c(void);
static BOOL func_02704fe4(int index);

// Powers the DSP: its clock, then its reset line, then the reset of its port
void DSP_PowerOnCore(void) {
    reg_SCFG_RST &= ~SCFG_RST_DSP;
    reg_SCFG_CLK = (u16)((reg_SCFG_CLK & (u16)~SCFG_CLK_DSP) | SCFG_CLK_DSP);
    OS_SpinWaitSysCycles(2);
    reg_SCFG_RST = (u16)((reg_SCFG_RST & (u16)~SCFG_RST_DSP) | SCFG_RST_DSP);
    DSP_ResetOnCore();
}

void DSP_PowerOffCore(void) {
    reg_SCFG_RST &= ~SCFG_RST_DSP;
    reg_SCFG_CLK &= ~SCFG_CLK_DSP;
}

void DSP_ResetOnCore(void) {
    if (reg_DSP_PCFG & DSP_PCFG_RESET) {
        return;
    }
    reg_DSP_PCFG |= DSP_PCFG_RESET;
    while (reg_DSP_PSTS & DSP_PSTS_RESETTING) {
    }
}

void DSP_ResetOffExCore(u16 mask) {
    while (reg_DSP_PSTS & DSP_PSTS_RESETTING) {
    }
    func_02704f9c();
    reg_DSP_PCFG |= (u16)(mask << DSP_PCFG_REPLY_IRQ_SHIFT);
    reg_DSP_PCFG &= ~DSP_PCFG_RESET;
}

// Called in reset: clears the reply interrupts and the semaphores, and reads the replies to empty them
static void func_02704f9c(void) {
    if (!(reg_DSP_PCFG & DSP_PCFG_RESET)) {
        return;
    }
    reg_DSP_PCFG &= ~DSP_PCFG_REPLY_IRQ_MASK;
    reg_DSP_PSEM = 0;
    reg_DSP_PCLEAR = 0xffff;
    (void)reg_DSP_REP(0);
    (void)reg_DSP_REP(1);
    (void)reg_DSP_REP(2);
}

// Whether the DSP took command index
static BOOL func_02704fe4(int index) {
    BOOL empty = TRUE;
    if (reg_DSP_PSTS & ((1 << index) << DSP_PSTS_COMMAND_SHIFT)) {
        empty = FALSE;
    }
    return empty;
}

BOOL DSP_RecvDataIsReadyCore(int index) {
    BOOL ready = TRUE;
    if (!(reg_DSP_PSTS & ((1 << index) << DSP_PSTS_REPLY_SHIFT))) {
        ready = FALSE;
    }
    return ready;
}

void DSP_SendDataCore(int index, u16 data) {
    while (!func_02704fe4(index)) {
    }
    reg_DSP_CMD(index) = data;
}

u16 DSP_RecvDataCore(int index) {
    while (!DSP_RecvDataIsReadyCore(index)) {
    }
    return reg_DSP_REP(index);
}

void func_02705088(u16 memory, u16 dspAddr, const u16 *src, int count, u16 flags) {
    OSIntrMode enabled = OS_DisableInterrupts();
    u16 autoIncrement = (flags & DSP_WRITE_FIXED_ADDRESS) ? 0 : DSP_PCFG_AUTO_INCREMENT;
    reg_DSP_PADR = dspAddr;
    reg_DSP_PCFG = (reg_DSP_PCFG & ~(DSP_PCFG_MEMORY_MASK | DSP_PCFG_AUTO_INCREMENT)) | memory | autoIncrement;
    if (flags & DSP_WRITE_FIXED_SOURCE) {
        while (count-- > 0) {
            while (reg_DSP_PSTS & DSP_PSTS_WRITE_FULL) {
            }
            reg_DSP_PDATA = *src;
        }
    } else {
        while (count-- > 0) {
            while (reg_DSP_PSTS & DSP_PSTS_WRITE_FULL) {
            }
            reg_DSP_PDATA = *src++;
        }
    }
    OS_RestoreInterrupts(enabled);
}

void func_02705150(u16 memory, u16 *dst, u16 dspAddr, int count, u16 flags) {
    OSIntrMode enabled = OS_DisableInterrupts();
    u16 autoIncrement = (flags & DSP_READ_FIXED_ADDRESS) ? 0 : DSP_PCFG_AUTO_INCREMENT;
    u16 length;

    switch (flags & DSP_READ_BURST_MASK) {
    case DSP_READ_BURST_1:
        length = DSP_PCFG_READ_1;
        count = 1;
        break;
    case DSP_READ_BURST_8:
        length = DSP_PCFG_READ_8;
        count = 8;
        break;
    case DSP_READ_BURST_16:
        length = DSP_PCFG_READ_16;
        count = 16;
        break;
    default:
        length = DSP_PCFG_READ_CONTINUOUS;
        break;
    }
    reg_DSP_PADR = dspAddr;
    reg_DSP_PCFG = (reg_DSP_PCFG & ~(DSP_PCFG_MEMORY_MASK | DSP_PCFG_READ_LENGTH_MASK | DSP_PCFG_AUTO_INCREMENT)) |
                   memory | length | autoIncrement | DSP_PCFG_READ_START;
    if (flags & DSP_READ_FIXED_DESTINATION) {
        while (count-- > 0) {
            while (!(reg_DSP_PSTS & DSP_PSTS_READ_READY)) {
            }
            *dst = reg_DSP_PDATA;
        }
    } else {
        while (count-- > 0) {
            while (!(reg_DSP_PSTS & DSP_PSTS_READ_READY)) {
            }
            *dst++ = reg_DSP_PDATA;
        }
    }
    reg_DSP_PCFG &= ~DSP_PCFG_READ_START;
    OS_RestoreInterrupts(enabled);
}

u16 DSP_GetSemaphoreCore(void) {
    return reg_DSP_SEM;
}

void DSP_ClearSemaphoreCore(u16 mask) {
    reg_DSP_PCLEAR = mask;
}

void DSP_MaskSemaphoreCore(u16 mask) {
    reg_DSP_PMASK = mask;
}
