// TwlSDK's mi_ndma.c, in the LTD autoload: the DSi's four new DMA channels. Each transfer is a fill or a copy whose
// address modes its mode gives, with the channel's settings. The ROM names none of it: the fills, copies, waits and
// stops of a channel keep the names they had here before the file was decompiled, and the other functions are named
// for what they do
#include "nitro/mi.h"
#include "nitro/os.h"
#include "twl/mi.h"

// The arbitration between the channels
#define reg_MI_NDMAGCNT (*(vu32 *)0x04004100)

// The camera's data register
#define MI_CAMERA_DATA_ADDR 0x04004204

static u32 MIi_GetNDmaModeBits(u32 mode);
static void MIi_NDma(u32 mode, u32 ndmaNo, const void *src, void *dest, u32 data, u32 size,
                     const MINDmaSettings *config, u32 timing, u32 enable);
static void MIi_WaitNDmaDone(u32 ndmaNo);

MINDmaSettings MIi_NDmaConfig[MI_NDMA_MAX_NUM + 1];

#pragma define_section LTDMAIN ".ltdmain"                                                                              \
                               ".ltdmain.bss"                                                                          \
                               ".ltdmain" abs32 RWX
#pragma section LTDMAIN begin

// Writes of a channel's registers, each a one-line static inline: the transfers match only so, in the shape of
// cybervisi0n/libntr's reconstruction
static inline void MIi_WriteNDmaSource(u32 ndmaNo, u32 src) {
    MI_NDMA_REG(ndmaNo, MI_NDMA_SAD) = src;
}

static inline void MIi_WriteNDmaDest(u32 ndmaNo, u32 dest) {
    MI_NDMA_REG(ndmaNo, MI_NDMA_DAD) = dest;
}

static inline void MIi_WriteNDmaTransferWords(u32 ndmaNo, u32 size) {
    MI_NDMA_REG(ndmaNo, MI_NDMA_TCNT) = size;
}

static inline void MIi_WriteNDmaStartWords(u32 ndmaNo, u32 size) {
    MI_NDMA_REG(ndmaNo, MI_NDMA_WCNT) = size;
}

static inline void MIi_WriteNDmaBlockInterval(u32 ndmaNo, u32 intervalTimer, u32 prescaler) {
    MI_NDMA_REG(ndmaNo, MI_NDMA_BCNT) = intervalTimer | prescaler;
}

static inline void MIi_WriteNDmaFillValue(u32 ndmaNo, u32 data) {
    MI_NDMA_REG(ndmaNo, MI_NDMA_FDATA) = data;
}

static inline void MIi_WriteNDmaControl(u32 ndmaNo, u32 control) {
    MI_NDMA_REG(ndmaNo, MI_NDMA_CNT) = control;
}

// The address modes of each kind of transfer
static u32 MIi_GetNDmaModeBits(u32 mode) {
    switch (mode) {
    case 0:
        return MI_NDMA_SRC_FILLDATA;
    case 1:
        return 0;
    case 2:
    case 8:
        return MI_NDMA_DEST_FIX;
    case 9:
        return MI_NDMA_IF_ENABLE | MI_NDMA_DEST_FIX;
    case 3:
        return MI_NDMA_SRC_FIX;
    case 4:
        return MI_NDMA_SRC_FIX | MI_NDMA_DEST_FIX;
    case 10:
        return MI_NDMA_IF_ENABLE | MI_NDMA_CONTINUOUS | MI_NDMA_SRC_FIX | MI_NDMA_DEST_RELOAD;
    case 5:
        return MI_NDMA_CONTINUOUS | MI_NDMA_DEST_RELOAD;
    case 6:
        return MI_NDMA_IF_ENABLE | MI_NDMA_CONTINUOUS | MI_NDMA_DEST_RELOAD;
    case 7:
        return MI_NDMA_CONTINUOUS | MI_NDMA_DEST_FIX;
    }
    return 0;
}

// A transfer that is waited for. Mode 0 fills with data, the others copy from src
static void MIi_NDma(u32 mode, u32 ndmaNo, const void *src, void *dest, u32 data, u32 size,
                     const MINDmaSettings *config, u32 timing, u32 enable) {
    if (size > 0) {
        u32 control;
        OSIntrMode enabled = OS_DisableInterrupts();

        MIi_WaitNDmaDone(ndmaNo);
        if (mode != 0) {
            MIi_WriteNDmaSource(ndmaNo, (u32)src);
        }
        MIi_WriteNDmaDest(ndmaNo, (u32)dest);
        MIi_WriteNDmaBlockInterval(ndmaNo, config->intervalTimer, config->prescaler);
        if (mode == 0) {
            MIi_WriteNDmaFillValue(ndmaNo, data);
        }
        if (timing == (u32)-1) {
            MIi_WriteNDmaStartWords(ndmaNo, size / 4);
        } else {
            MIi_WriteNDmaTransferWords(ndmaNo, size / 4);
            MIi_WriteNDmaStartWords(ndmaNo, config->wordCount == (u32)-1 ? size / 4 : config->wordCount);
        }
        control = config->blockWord | (enable & MI_NDMA_ENABLE);
        control |= MIi_GetNDmaModeBits(mode);
        control |= timing == (u32)-1 ? MI_NDMA_IMM : timing;
        MIi_WriteNDmaControl(ndmaNo, control);

        MIi_WaitNDmaDone(ndmaNo);
        OS_RestoreInterrupts(enabled);
    }
}

void MIi_NDmaAsync(u32 mode, u32 ndmaNo, const void *src, void *dest, u32 data, u32 size, MINDmaCallback callback,
                   void *arg, const MINDmaSettings *config, u32 timing, u32 enable) {
    if (size == 0) {
        if (callback != NULL) {
            callback(arg);
        }
    } else {
        u32 control;
        OSIntrMode enabled = OS_DisableInterrupts();

        MIi_WaitNDmaDone(ndmaNo);
        if (callback != NULL) {
            OSi_EnterNDmaCallback(ndmaNo, callback, arg);
        }
        if (mode != 0) {
            MIi_WriteNDmaSource(ndmaNo, (u32)src);
        }
        MIi_WriteNDmaDest(ndmaNo, (u32)dest);
        MIi_WriteNDmaBlockInterval(ndmaNo, config->intervalTimer, config->prescaler);
        if (mode == 0) {
            MIi_WriteNDmaFillValue(ndmaNo, data);
        }
        if (timing == (u32)-1) {
            MIi_WriteNDmaStartWords(ndmaNo, size / 4);
        } else {
            MIi_WriteNDmaTransferWords(ndmaNo, size / 4);
            MIi_WriteNDmaStartWords(ndmaNo, config->wordCount == (u32)-1 ? size / 4 : config->wordCount);
        }
        control = config->blockWord | (enable & MI_NDMA_ENABLE);
        control |= MIi_GetNDmaModeBits(mode);
        control |= timing == (u32)-1 ? MI_NDMA_IMM : timing;
        if (callback != NULL) {
            control |= MI_NDMA_IF_ENABLE;
        }
        MIi_WriteNDmaControl(ndmaNo, control);

        OS_RestoreInterrupts(enabled);
    }
}

void MI_SetUpNDma(void) {
    MI_SetNDmaGlobalControl(MI_NDMA_ROUND_ROBIN, 6 << 16);
    MI_ResetNDmaSettings();
}

void MI_NDmaFill(u32 ndmaNo, void *dest, u32 data, u32 size) {
    MIi_NDma(0, ndmaNo, NULL, dest, data, size, &MIi_NDmaConfig[ndmaNo], (u32)-1, MI_NDMA_ENABLE);
}

void MI_NDmaCopy(u32 ndmaNo, const void *src, void *dest, u32 size) {
    MIi_NDma(1, ndmaNo, src, dest, 0, size, &MIi_NDmaConfig[ndmaNo], (u32)-1, MI_NDMA_ENABLE);
}

void MI_NDmaFillAsync(u32 ndmaNo, void *dest, u32 data, u32 size, MINDmaCallback callback, void *arg) {
    MIi_NDmaAsyncImmediate(0, ndmaNo, NULL, dest, data, size, callback, arg, MI_NDMA_ENABLE);
}

void MI_NDmaCopyAsync(u32 ndmaNo, const void *src, void *dest, u32 size, MINDmaCallback callback, void *arg) {
    MIi_NDmaAsyncImmediate(1, ndmaNo, src, dest, 0, size, callback, arg, MI_NDMA_ENABLE);
}

BOOL MI_IsNDmaBusy(u32 ndmaNo) {
    return (MI_NDMA_REG(ndmaNo, MI_NDMA_CNT) & MI_NDMA_ENABLE) >> 31;
}

void MI_WaitNDma(u32 ndmaNo) {
    OSIntrMode enabled = OS_DisableInterrupts();
    while (MI_NDMA_REG(ndmaNo, MI_NDMA_CNT) & MI_NDMA_ENABLE) {
    }
    OS_RestoreInterrupts(enabled);
}

void MI_StopNDma(u32 ndmaNo) {
    OSIntrMode enabled = OS_DisableInterrupts();
    MI_NDMA_REG(ndmaNo, MI_NDMA_CNT) &= ~MI_NDMA_ENABLE;
    // The channel stops after two more reads of its control
    MI_NDMA_REG(ndmaNo, MI_NDMA_CNT);
    MI_NDMA_REG(ndmaNo, MI_NDMA_CNT);
    OS_RestoreInterrupts(enabled);
}

void MI_StopNDmaChannels(void) {
    MI_StopNDma(0);
    MI_StopNDma(1);
    MI_StopNDma(2);
    MI_StopNDma(3);
}

static void MIi_WaitNDmaDone(u32 ndmaNo) {
    while (MI_IsNDmaBusy(ndmaNo) == TRUE) {
    }
}

void MI_SetNDmaGlobalControl(u32 mode, u32 cycle) {
    if (mode == MI_NDMA_ROUND_ROBIN) {
        reg_MI_NDMAGCNT = cycle | MI_NDMA_ROUND_ROBIN;
    } else {
        reg_MI_NDMAGCNT = 0;
    }
}

void MI_ResetNDmaSettings(void) {
    MINDmaSettings *config = MIi_NDmaConfig;
    int i;

    for (i = 0; i <= MI_NDMA_MAX_NUM; i++) {
        config->intervalTimer = 1;
        config->prescaler = 0;
        config->blockWord = MI_NDMA_BWORD_16;
        config->wordCount = (u32)-1;
        config++;
    }
}

void MI_CopyNDmaSettings(u32 ndmaNo, MINDmaSettings *config) {
    MI_CpuCopy32(&MIi_NDmaConfig[ndmaNo], config, sizeof(MINDmaSettings));
}

void MIi_StartCardNDma(u32 ndmaNo, const void *src, void *dest) {
    OSIntrMode enabled = OS_DisableInterrupts();
    MINDmaSettings *config = &MIi_NDmaConfig[ndmaNo];
    MIi_WaitNDmaDone(ndmaNo);
    MI_NDMA_REG(ndmaNo, MI_NDMA_SAD) = (u32)src;
    MI_NDMA_REG(ndmaNo, MI_NDMA_DAD) = (u32)dest;
    MI_NDMA_REG(ndmaNo, MI_NDMA_TCNT) = 1;
    MI_NDMA_REG(ndmaNo, MI_NDMA_WCNT) = 1;
    MI_NDMA_REG(ndmaNo, MI_NDMA_CNT) =
        config->blockWord | MI_NDMA_ENABLE | MI_NDMA_CONTINUOUS | MI_NDMA_TIMING_CARD | MI_NDMA_SRC_FIX;
    OS_RestoreInterrupts(enabled);
}

void MIi_CardNDmaRecv(u32 ndmaNo, const void *src, void *dest, u32 size) {
    MIi_StartCardNDma(ndmaNo, src, dest);
}

void MIi_CameraNDmaRecv(u32 ndmaNo, void *dest, u32 unitWords, u32 length, BOOL continuous, MINDmaCallback callback,
                        void *arg) {
    MINDmaSettings config;
    u32 mode = 10;

    if (!continuous) {
        mode = 3;
    }
    MI_CopyNDmaSettings(ndmaNo, &config);
    config.intervalTimer = 2;
    config.prescaler = 0;
    config.blockWord = MI_NDMA_BWORD_16;
    config.wordCount = unitWords;
    MIi_NDmaAsync(mode, ndmaNo, (const void *)MI_CAMERA_DATA_ADDR, dest, 0, length, callback, arg, &config,
                  MI_NDMA_TIMING_CAMERA, MI_NDMA_ENABLE);
}

#pragma section LTDMAIN end
