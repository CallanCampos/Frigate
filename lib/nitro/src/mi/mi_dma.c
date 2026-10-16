// NitroSDK's mi_dma.c: DMA fills and copies, synchronous and with a completion callback, and the checks on them. The
// SDK writes the registers in ITCM (MIi_DmaSetParameters).
#include "nitro/mi.h"
#include "nitro/os.h"

// Waiting for a transfer or stopping one leaves DMA0 with a dummy transfer of one word, from address 0 to address 0
#define MI_DMA0_DUMMY_CNT (MI_DMA_ENABLE | MI_DMA_SRC_FIX | MI_DMA_DEST_FIX | 1)

// The 16 MiB areas of the address space that DMA0 may not read from: the I/O registers and everything from the
// cartridge on
#define MI_ADDR_AREA_MASK 0xff000000
#define MI_ADDR_AREA_IOREG 0x04000000
#define MI_ADDR_AREA_CARTRIDGE 0x08000000

void MIi_DmaFill32(u32 dmaNo, void *dest, u32 data, u32 size, BOOL dmaEnable) {
    vu32 *dmaCntp;

    if (size == 0) {
        return;
    }
    dmaCntp = &((vu32 *)REG_DMA0SAD_ADDR)[dmaNo * 3 + 2];
    while (*dmaCntp & MI_DMA_ENABLE) {
    }
    if (dmaEnable) {
        MIi_DmaSetParameters(dmaNo, data, (u32)dest, MI_DMA_ENABLE | MI_DMA_32BIT_BUF | MI_DMA_SRC_FIX | (size >> 2),
                             MI_DMA_SET_WAIT | MI_DMA_SET_FILL32);
    } else {
        MIi_DmaSetParameters(dmaNo, data, (u32)dest, MI_DMA_32BIT_BUF | MI_DMA_SRC_FIX | (size >> 2),
                             MI_DMA_SET_WAIT | MI_DMA_SET_NO_DMA0_FIX | MI_DMA_SET_FILL32);
    }
    while (*dmaCntp & MI_DMA_ENABLE) {
    }
}

void MIi_DmaCopy32(u32 dmaNo, const void *src, void *dest, u32 size, BOOL dmaEnable) {
    vu32 *dmaCntp;

    if (size == 0) {
        return;
    }
    MIi_CheckDma0SourceAddress(dmaNo, (u32)src, size, MI_DMA_SRC_INC);
    dmaCntp = &((vu32 *)REG_DMA0SAD_ADDR)[dmaNo * 3 + 2];
    while (*dmaCntp & MI_DMA_ENABLE) {
    }
    if (dmaEnable) {
        MIi_DmaSetParameters(dmaNo, (u32)src, (u32)dest, MI_DMA_ENABLE | MI_DMA_32BIT_BUF | (size >> 2),
                             MI_DMA_SET_WAIT);
    } else {
        MIi_DmaSetParameters(dmaNo, (u32)src, (u32)dest, MI_DMA_32BIT_BUF | (size >> 2),
                             MI_DMA_SET_WAIT | MI_DMA_SET_NO_DMA0_FIX);
    }
    while (*dmaCntp & MI_DMA_ENABLE) {
    }
}

void MIi_DmaCopy16(u32 dmaNo, const void *src, void *dest, u32 size, BOOL dmaEnable) {
    vu32 *dmaCntp;

    if (size == 0) {
        return;
    }
    MIi_CheckDma0SourceAddress(dmaNo, (u32)src, size, MI_DMA_SRC_INC);
    dmaCntp = &((vu32 *)REG_DMA0SAD_ADDR)[dmaNo * 3 + 2];
    while (*dmaCntp & MI_DMA_ENABLE) {
    }
    if (dmaEnable) {
        MIi_DmaSetParameters(dmaNo, (u32)src, (u32)dest, MI_DMA_ENABLE | (size >> 1), MI_DMA_SET_WAIT);
    } else {
        MIi_DmaSetParameters(dmaNo, (u32)src, (u32)dest, size >> 1, MI_DMA_SET_WAIT | MI_DMA_SET_NO_DMA0_FIX);
    }
    while (*dmaCntp & MI_DMA_ENABLE) {
    }
}

void MIi_DmaFill32Async(u32 dmaNo, void *dest, u32 data, u32 size, MIDmaCallback callback, void *arg, BOOL dmaEnable) {
    if (size == 0) {
        if (callback != NULL) {
            callback(arg);
        }
        return;
    }
    MI_WaitDma(dmaNo);
    if (callback != NULL) {
        OSi_EnterDmaCallback(dmaNo, callback, arg);
        if (dmaEnable) {
            MIi_DmaSetParameters(dmaNo, data, (u32)dest,
                                 MI_DMA_ENABLE | MI_DMA_IF_ENABLE | MI_DMA_32BIT_BUF | MI_DMA_SRC_FIX | (size >> 2),
                                 MI_DMA_SET_FILL32);
        } else {
            MIi_DmaSetParameters(dmaNo, data, (u32)dest,
                                 MI_DMA_IF_ENABLE | MI_DMA_32BIT_BUF | MI_DMA_SRC_FIX | (size >> 2),
                                 MI_DMA_SET_NO_DMA0_FIX | MI_DMA_SET_FILL32);
        }
    } else {
        if (dmaEnable) {
            MIi_DmaSetParameters(dmaNo, data, (u32)dest,
                                 MI_DMA_ENABLE | MI_DMA_32BIT_BUF | MI_DMA_SRC_FIX | (size >> 2), MI_DMA_SET_FILL32);
        } else {
            MIi_DmaSetParameters(dmaNo, data, (u32)dest, MI_DMA_32BIT_BUF | MI_DMA_SRC_FIX | (size >> 2),
                                 MI_DMA_SET_NO_DMA0_FIX | MI_DMA_SET_FILL32);
        }
    }
}

void MIi_DmaCopy32Async(u32 dmaNo, const void *src, void *dest, u32 size, MIDmaCallback callback, void *arg,
                        BOOL dmaEnable) {
    MIi_CheckDma0SourceAddress(dmaNo, (u32)src, size, MI_DMA_SRC_INC);
    if (size == 0) {
        if (callback != NULL) {
            callback(arg);
        }
        return;
    }
    MI_WaitDma(dmaNo);
    if (callback != NULL) {
        OSi_EnterDmaCallback(dmaNo, callback, arg);
        if (dmaEnable) {
            MIi_DmaSetParameters(dmaNo, (u32)src, (u32)dest,
                                 MI_DMA_ENABLE | MI_DMA_IF_ENABLE | MI_DMA_32BIT_BUF | (size >> 2), 0);
        } else {
            MIi_DmaSetParameters(dmaNo, (u32)src, (u32)dest, MI_DMA_IF_ENABLE | MI_DMA_32BIT_BUF | (size >> 2),
                                 MI_DMA_SET_NO_DMA0_FIX);
        }
    } else {
        if (dmaEnable) {
            MIi_DmaSetParameters(dmaNo, (u32)src, (u32)dest, MI_DMA_ENABLE | MI_DMA_32BIT_BUF | (size >> 2), 0);
        } else {
            MIi_DmaSetParameters(dmaNo, (u32)src, (u32)dest, MI_DMA_32BIT_BUF | (size >> 2), MI_DMA_SET_NO_DMA0_FIX);
        }
    }
}

void MI_WaitDma(u32 dmaNo) {
    u32 enabled = OS_DisableInterrupts();

    while (*(vu32 *)(REG_DMA0SAD_ADDR + 8 + dmaNo * 12) & MI_DMA_ENABLE) {
    }
    if (dmaNo == 0) {
        vu32 *regs = (vu32 *)(REG_DMA0SAD_ADDR + dmaNo * 12);
        regs[0] = 0;
        regs[1] = 0;
        regs[2] = MI_DMA0_DUMMY_CNT;
    }
    OS_RestoreInterrupts(enabled);
}

void MI_StopDma(u32 dmaNo) {
    u32 enabled = OS_DisableInterrupts();
    vu32 *dmaCntp = (vu32 *)(REG_DMA0SAD_ADDR + 8 + dmaNo * 12);

    *dmaCntp &= ~(MI_DMA_TIMING_MASK | MI_DMA_CONTINUOUS_ON);
    *dmaCntp &= ~MI_DMA_ENABLE;
    (void)*dmaCntp;
    (void)*dmaCntp;
    if (dmaNo == 0) {
        vu32 *regs = (vu32 *)(REG_DMA0SAD_ADDR + dmaNo * 12);
        regs[0] = 0;
        regs[1] = 0;
        regs[2] = MI_DMA0_DUMMY_CNT;
    }
    OS_RestoreInterrupts(enabled);
}

void MI_StopAllDma(void) {
    MI_StopDma(0);
    MI_StopDma(1);
    MI_StopDma(2);
    MI_StopDma(3);
}

void MIi_CheckAnotherAutoDMA(u32 dmaNo, u32 dmaType) {
    int i;

    for (i = 0; i < MI_DMA_MAX_NUM; i++) {
        u32 cnt;
        u32 type;

        if (i == dmaNo) {
            continue;
        }
        cnt = MI_DMA_REGS[i].cnt;
        if (!(cnt & MI_DMA_ENABLE)) {
            continue;
        }
        type = cnt & MI_DMA_TIMING_MASK;
        if (type == dmaType) {
            continue;
        }
        if (type == MI_DMA_TIMING_V_BLANK && dmaType == MI_DMA_TIMING_H_BLANK) {
            continue;
        }
        if (type == MI_DMA_TIMING_H_BLANK && dmaType == MI_DMA_TIMING_V_BLANK) {
            continue;
        }
        if (type == MI_DMA_TIMING_DISP || type == MI_DMA_TIMING_DISP_MMEM || type == MI_DMA_TIMING_CARD ||
            type == MI_DMA_TIMING_CARTRIDGE || type == MI_DMA_TIMING_GXFIFO || type == MI_DMA_TIMING_V_BLANK ||
            type == MI_DMA_TIMING_H_BLANK) {
            OS_Terminate();
        }
    }
}

void MIi_CheckDma0SourceAddress(u32 dmaNo, u32 src, u32 size, u32 dir) {
    if (dmaNo == 0) {
        u32 areaStart = src & MI_ADDR_AREA_MASK;
        u32 areaEnd = src;

        switch (dir) {
        case MI_DMA_SRC_INC:
            areaEnd = src + size;
            break;
        case MI_DMA_SRC_DEC:
            areaEnd = src - size;
            break;
        }
        areaEnd &= MI_ADDR_AREA_MASK;
        if (areaStart == MI_ADDR_AREA_IOREG || areaStart >= MI_ADDR_AREA_CARTRIDGE || areaEnd == MI_ADDR_AREA_IOREG ||
            areaEnd >= MI_ADDR_AREA_CARTRIDGE) {
            OS_Terminate();
        }
    }
}
