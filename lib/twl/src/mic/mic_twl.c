// TwlSDK's microphone library (libspi), the DSi's limited sampling in the LTD autoload: the same PXI commands as
// NitroSDK's MIC_StartAutoSampling and MIC_StopAutoSampling (nitro/spi/mic.c), with commands of their own and a check
// of the sampling rate, which the Limited functions call when the console runs in its DSi mode. ARM code. The ROM names
// none of it; the file's name is a guess (the SDK's may be mic_ex.c or micex.c), and the functions have the names of
// libntr's micex.h where they are known. The synchronous ones wait for the answer on a message queue.

#include "types.h"
#include "nitro/mic.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "twl/mic.h"

// A command is a series of words with the microphone's tag, as in nitro/spi/mic.c
#define SPI_PXI_START_BIT 0x02000000
#define SPI_PXI_END_BIT 0x01000000
#define MIC_PXI_INDEX(index) ((index) << 16)
#define MIC_PXI_COMMAND(command) ((command) << 8)
// The DSi's commands, which start and stop the sampling that is limited to the sampling rates below
#define MICEX_PXI_COMMAND_LIMITED_ON 0x44
#define MICEX_PXI_COMMAND_LIMITED_OFF 0x45

// The type that the ARM7 gets, as in nitro/spi/mic.c, with bit 4 for sampling in a loop. The code clears bit 5, which
// no type sets
#define MIC_PXI_TYPE_8BIT 0
#define MIC_PXI_TYPE_12BIT 1
#define MIC_PXI_TYPE_SIGNED_8BIT 2
#define MIC_PXI_TYPE_SIGNED_12BIT 3
#define MIC_PXI_TYPE_12BIT_FILTER_OFF 5
#define MIC_PXI_TYPE_SIGNED_12BIT_FILTER_OFF 7
#define MIC_PXI_TYPE_LOOP 0x10
#define MIC_PXI_TYPE_UNK_20 0x20

#define MIC_ALIGN_MASK 0x1f

static MICResult MICEXi_StopLimitedSamplingAsync(MICCallback callback, void *arg);
static void func_02703f60(MICResult result, void *arg);
static BOOL func_02703f7c(void *buffer, u32 size, u32 rate, u8 type);
static BOOL func_02704088(void);

MICResult MICEXi_StartLimitedSampling(const MICAutoParam *param) {
    MICResult result;
    OSMessage messages[1];
    OSMessageQueue queue;

    if (OS_GetCurrentThread() == NULL) {
        return MIC_RESULT_ILLEGAL_STATUS;
    }
    OS_InitMessageQueue(&queue, messages, 1);
    result = MICEXi_StartLimitedSamplingAsync(param, func_02703f60, &queue);
    if (result == MIC_RESULT_SUCCESS) {
        OS_ReceiveMessage(&queue, (OSMessage *)&result, OS_MESSAGE_BLOCK);
    }
    return result;
}

MICResult MICEXi_StartLimitedSamplingAsync(const MICAutoParam *param, MICCallback callback, void *arg) {
    MICWork *work = MICi_GetSysWork();
    u32 type;
    u8 mode;
    OSIntrMode enabled;

    if (param->size == 0) {
        return MIC_RESULT_ILLEGAL_PARAMETER;
    }
    if (((uptr)param->buffer & MIC_ALIGN_MASK) || (param->size & MIC_ALIGN_MASK)) {
        return MIC_RESULT_ILLEGAL_PARAMETER;
    }

    switch (param->type) {
    case MIC_SAMPLING_TYPE_8BIT:
        type = MIC_PXI_TYPE_8BIT;
        break;
    case MIC_SAMPLING_TYPE_12BIT:
        type = MIC_PXI_TYPE_12BIT;
        break;
    case MIC_SAMPLING_TYPE_SIGNED_8BIT:
        type = MIC_PXI_TYPE_SIGNED_8BIT;
        break;
    case MIC_SAMPLING_TYPE_SIGNED_12BIT:
        type = MIC_PXI_TYPE_SIGNED_12BIT;
        break;
    case MIC_SAMPLING_TYPE_12BIT_FILTER_OFF:
        type = MIC_PXI_TYPE_12BIT_FILTER_OFF;
        break;
    case MIC_SAMPLING_TYPE_SIGNED_12BIT_FILTER_OFF:
        type = MIC_PXI_TYPE_SIGNED_12BIT_FILTER_OFF;
        break;
    default:
        return MIC_RESULT_ILLEGAL_PARAMETER;
    }

    // The sampling rates that the DSi allows, in ARM7 clocks between two
    // samples: 47.7, 32.8, 23.8, 16.4, 15.9, 11.9, 10.9 and 8.2 kHz
    switch (param->rate) {
    case 0x2bf:
    case 0x3ff:
    case 0x57f:
    case 0x800:
    case 0x83f:
    case 0xb00:
    case 0xbff:
    case MIC_SAMPLING_RATE_8180:
        break;
    default:
        return MIC_RESULT_ILLEGAL_PARAMETER;
    }

    mode = param->loop_enable ? (type | MIC_PXI_TYPE_LOOP) : (type & ~MIC_PXI_TYPE_LOOP);
    mode = mode & ~MIC_PXI_TYPE_UNK_20;

    enabled = OS_DisableInterrupts();
    if (work->lock) {
        OS_RestoreInterrupts(enabled);
        return MIC_RESULT_BUSY;
    }
    work->lock = TRUE;
    OS_RestoreInterrupts(enabled);

    work->callback = callback;
    work->callbackArg = arg;
    work->full = param->full_callback;
    work->fullArg = param->full_arg;
    if (func_02703f7c(param->buffer, param->size, param->rate, mode)) {
        return MIC_RESULT_SUCCESS;
    }
    work->lock = FALSE;
    return MIC_RESULT_SEND_ERROR;
}

MICResult MICEXi_StopLimitedSampling(void) {
    MICResult result;
    OSMessage messages[1];
    OSMessageQueue queue;

    if (OS_GetCurrentThread() == NULL) {
        return MIC_RESULT_ILLEGAL_STATUS;
    }
    OS_InitMessageQueue(&queue, messages, 1);
    result = MICEXi_StopLimitedSamplingAsync(func_02703f60, &queue);
    if (result == MIC_RESULT_SUCCESS) {
        OS_ReceiveMessage(&queue, (OSMessage *)&result, OS_MESSAGE_BLOCK);
    }
    return result;
}

// TwlSDK's MICEXi_StopLimitedSamplingAsync: sends the stop, and the callback gets the answer
static MICResult MICEXi_StopLimitedSamplingAsync(MICCallback callback, void *arg) {
    MICWork *work = MICi_GetSysWork();
    OSIntrMode enabled = OS_DisableInterrupts();

    if (work->lock) {
        OS_RestoreInterrupts(enabled);
        return MIC_RESULT_BUSY;
    }
    work->lock = TRUE;
    OS_RestoreInterrupts(enabled);

    work->callback = callback;
    work->callbackArg = arg;
    if (func_02704088()) {
        return MIC_RESULT_SUCCESS;
    }
    work->lock = FALSE;
    return MIC_RESULT_SEND_ERROR;
}

// The callback of the synchronous calls: hands the result to the message queue that waits for it
static void func_02703f60(MICResult result, void *arg) {
    OS_SendMessage(arg, (OSMessage)result, OS_MESSAGE_NOBLOCK);
}

// Sends one word of a command to the ARM7; negative when the FIFO is full
#define MIC_PXI_SEND(data) PXI_SendWordByFifo(PXI_FIFO_TAG_MIC, (data), FALSE)

// Sends the command that starts the sampling, in seven words
static BOOL func_02703f7c(void *buffer, u32 size, u32 rate, u8 type) {
    if (MIC_PXI_SEND(SPI_PXI_START_BIT | MIC_PXI_COMMAND(MICEX_PXI_COMMAND_LIMITED_ON) | type) < 0) {
        return FALSE;
    }
    if (MIC_PXI_SEND(MIC_PXI_INDEX(1) | ((u32)buffer >> 16)) < 0) {
        return FALSE;
    }
    if (MIC_PXI_SEND(MIC_PXI_INDEX(2) | ((u32)buffer & 0xffff)) < 0) {
        return FALSE;
    }
    if (MIC_PXI_SEND(MIC_PXI_INDEX(3) | (size >> 16)) < 0) {
        return FALSE;
    }
    if (MIC_PXI_SEND(MIC_PXI_INDEX(4) | (size & 0xffff)) < 0) {
        return FALSE;
    }
    if (MIC_PXI_SEND(MIC_PXI_INDEX(5) | (rate >> 16)) < 0) {
        return FALSE;
    }
    if (MIC_PXI_SEND(SPI_PXI_END_BIT | MIC_PXI_INDEX(6) | (rate & 0xffff)) < 0) {
        return FALSE;
    }
    return TRUE;
}

// Sends the command that stops the sampling
static BOOL func_02704088(void) {
    if (MIC_PXI_SEND(SPI_PXI_START_BIT | SPI_PXI_END_BIT | MIC_PXI_COMMAND(MICEX_PXI_COMMAND_LIMITED_OFF)) < 0) {
        return FALSE;
    }
    return TRUE;
}
