// NitroSDK's microphone library (TwlSDK's libspi, spi/ARM9/src/mic.c): starts and stops the ARM7's automatic sampling
// of the microphone into a buffer, through the PXI FIFO. The file name is the SDK's, and the split is certain: the
// functions come in the SDK's order, and the .bss is its two statics. The names are the SDK's. The Limited functions
// hand over to the DSi's MICEX code in the LTD autoload when the console runs in its DSi mode, and otherwise call the
// plain ones (swan named them after these). The linker's veneers to that code (func_0207e92c, func_0207e950 and
// func_0207e970) follow their callers in the ROM.
#include "nitro/mic.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "twl/mic.h"

// A command is a series of words with the microphone's tag: the first has SPI_PXI_START_BIT and the command in bits 8
// to 14, the last has SPI_PXI_END_BIT, and each of the others carries its index in bits 16 to 22 and 16 bits of data
#define SPI_PXI_START_BIT 0x02000000
#define SPI_PXI_END_BIT 0x01000000
#define MIC_PXI_INDEX(index) ((index) << 16)
#define MIC_PXI_COMMAND(command) ((command) << 8)
// The commands, which the ARM7's answers repeat. This build never starts a single sampling, but its callback handles
// the answer
#define SPI_PXI_COMMAND_MIC_SAMPLING 0x40
#define SPI_PXI_COMMAND_MIC_AUTO_ON 0x41
#define SPI_PXI_COMMAND_MIC_AUTO_OFF 0x42
// Sent by the ARM7 when the buffer is full
#define SPI_PXI_COMMAND_MIC_BUFFER_FULL 0x51
#define SPI_PXI_RESULT_COMMAND_MASK 0x7f00
#define SPI_PXI_RESULT_DATA_MASK 0xff

// What the ARM7 answers with
#define MIC_PXI_RESULT_SUCCESS 0
#define MIC_PXI_RESULT_INVALID_COMMAND 1
#define MIC_PXI_RESULT_INVALID_PARAMETER 2
#define MIC_PXI_RESULT_ILLEGAL_STATUS 3
#define SPI_PXI_RESULT_EXCLUSIVE 4

// The type that the ARM7 gets, with bit 4 for sampling in a loop
#define SPI_MIC_SAMPLING_TYPE_8BIT 0
#define SPI_MIC_SAMPLING_TYPE_12BIT 1
#define SPI_MIC_SAMPLING_TYPE_S8BIT 2
#define SPI_MIC_SAMPLING_TYPE_S12BIT 3
#define MIC_PXI_TYPE_12BIT_FILTER_OFF 5
#define MIC_PXI_TYPE_SIGNED_12BIT_FILTER_OFF 7
#define SPI_MIC_SAMPLING_TYPE_LOOP_ON 0x10

#define MIC_ALIGN_MASK 0x1f

static u16 micInitialized;
static MICWork micWork;

static void MicCommonCallback(PXIFifoTag tag, u32 data, BOOL err);
static BOOL MicStartAutoSampling(void *buffer, u32 size, u32 rate, u8 type);
static BOOL MicStopAutoSampling(void);
static void MicGetResultCallback(MICResult result, void *arg);
static void MicWaitBusy(void);

void MIC_Init(void) {
    if (micInitialized) {
        return;
    }
    micInitialized = TRUE;
    micWork.lock = FALSE;
    micWork.callback = NULL;
    PXI_Init();
    while (!PXI_IsCallbackReady(PXI_FIFO_TAG_MIC, PXI_PROC_ARM7)) {
    }
    OS_GetSystemWork()->mic_last_address = 0;
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_MIC, MicCommonCallback);
}

MICResult MIC_StartAutoSamplingAsync(const MICAutoParam *param, MICCallback callback, void *arg) {
    OSIntrMode enabled;
    int type;
    u8 mode;

    if ((u32)param->buffer & MIC_ALIGN_MASK) {
        return MIC_RESULT_ILLEGAL_PARAMETER;
    }
    if (param->size & MIC_ALIGN_MASK) {
        return MIC_RESULT_ILLEGAL_PARAMETER;
    }
    if (param->size == 0) {
        return MIC_RESULT_ILLEGAL_PARAMETER;
    }
    if (param->rate < MIC_SAMPLING_RATE_LIMIT) {
        return MIC_RESULT_ILLEGAL_PARAMETER;
    }
    switch (param->type) {
    case MIC_SAMPLING_TYPE_8BIT:
        type = SPI_MIC_SAMPLING_TYPE_8BIT;
        break;
    case MIC_SAMPLING_TYPE_12BIT:
        type = SPI_MIC_SAMPLING_TYPE_12BIT;
        break;
    case MIC_SAMPLING_TYPE_SIGNED_8BIT:
        type = SPI_MIC_SAMPLING_TYPE_S8BIT;
        break;
    case MIC_SAMPLING_TYPE_SIGNED_12BIT:
        type = SPI_MIC_SAMPLING_TYPE_S12BIT;
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
    mode = param->loop_enable ? (u8)(type | SPI_MIC_SAMPLING_TYPE_LOOP_ON) : (u8)type;

    enabled = OS_DisableInterrupts();
    if (micWork.lock) {
        OS_RestoreInterrupts(enabled);
        return MIC_RESULT_BUSY;
    }
    micWork.lock = TRUE;
    OS_RestoreInterrupts(enabled);

    micWork.callback = callback;
    micWork.callbackArg = arg;
    micWork.full = param->full_callback;
    micWork.fullArg = param->full_arg;
    if (MicStartAutoSampling(param->buffer, param->size, param->rate, mode)) {
        return MIC_RESULT_SUCCESS;
    }
    micWork.lock = FALSE;
    return MIC_RESULT_SEND_ERROR;
}

// The synchronous start: waits for the answer
MICResult MIC_StartAutoSampling(const MICAutoParam *param) {
    micWork.commonResult = MIC_StartAutoSamplingAsync(param, MicGetResultCallback, NULL);
    if (micWork.commonResult == MIC_RESULT_SUCCESS) {
        MicWaitBusy();
    }
    return micWork.commonResult;
}

MICResult MIC_StopAutoSamplingAsync(MICCallback callback, void *arg) {
    OSIntrMode enabled = OS_DisableInterrupts();

    if (micWork.lock) {
        OS_RestoreInterrupts(enabled);
        return MIC_RESULT_BUSY;
    }
    micWork.lock = TRUE;
    OS_RestoreInterrupts(enabled);

    micWork.callback = callback;
    micWork.callbackArg = arg;
    if (MicStopAutoSampling()) {
        return MIC_RESULT_SUCCESS;
    }
    micWork.lock = FALSE;
    return MIC_RESULT_SEND_ERROR;
}

// The synchronous stop: waits for the answer
MICResult MIC_StopAutoSampling(void) {
    micWork.commonResult = MIC_StopAutoSamplingAsync(MicGetResultCallback, NULL);
    if (micWork.commonResult == MIC_RESULT_SUCCESS) {
        MicWaitBusy();
    }
    return micWork.commonResult;
}

void *MIC_GetLastSamplingAddress(void) {
    return (void *)OS_GetSystemWork()->mic_last_address;
}

MICResult MIC_StartLimitedSamplingAsync(const MICAutoParam *param, MICCallback callback, void *arg) {
    if (OSi_IsCodecTwlMode() == TRUE) {
        return MICEXi_StartLimitedSamplingAsync(param, callback, arg);
    }
    return MIC_StartAutoSamplingAsync(param, callback, arg);
}

MICResult MIC_StartLimitedSampling(const MICAutoParam *param) {
    if (OSi_IsCodecTwlMode() == TRUE) {
        return MICEXi_StartLimitedSampling(param);
    }
    return MIC_StartAutoSampling(param);
}

MICResult MIC_StopLimitedSampling(void) {
    if (OSi_IsCodecTwlMode() == TRUE) {
        return MICEXi_StopLimitedSampling();
    }
    return MIC_StopAutoSampling();
}

// Gets the ARM7's words with the microphone's tag
static void MicCommonCallback(PXIFifoTag tag, u32 data, BOOL err) {
    MICCallback callback;
    MICResult result;
    u16 command;
    u16 status;

    if (err) {
        if (micWork.lock) {
            micWork.lock = FALSE;
        }
        callback = micWork.callback;
        if (callback) {
            micWork.callback = NULL;
            callback(MIC_RESULT_FATAL_ERROR, micWork.callbackArg);
        }
    }

    command = (data & SPI_PXI_RESULT_COMMAND_MASK) >> 8;
    status = data & SPI_PXI_RESULT_DATA_MASK;
    switch (status) {
    case MIC_PXI_RESULT_SUCCESS:
        result = MIC_RESULT_SUCCESS;
        break;
    case MIC_PXI_RESULT_INVALID_COMMAND:
        result = MIC_RESULT_INVALID_COMMAND;
        break;
    case MIC_PXI_RESULT_INVALID_PARAMETER:
        result = MIC_RESULT_ILLEGAL_PARAMETER;
        break;
    case MIC_PXI_RESULT_ILLEGAL_STATUS:
        result = MIC_RESULT_ILLEGAL_STATUS;
        break;
    case SPI_PXI_RESULT_EXCLUSIVE:
        result = MIC_RESULT_BUSY;
        break;
    default:
        result = MIC_RESULT_FATAL_ERROR;
        break;
    }

    if (command == SPI_PXI_COMMAND_MIC_BUFFER_FULL) {
        if (micWork.full) {
            micWork.full(result, micWork.fullArg);
        }
        return;
    }
    if (command == SPI_PXI_COMMAND_MIC_SAMPLING) {
        if (micWork.dst_buf) {
            *micWork.dst_buf = OS_GetSystemWork()->mic_sampling_data;
        }
    }

    if (micWork.lock) {
        micWork.lock = FALSE;
    }
    callback = micWork.callback;
    if (callback) {
        micWork.callback = NULL;
        callback(result, micWork.callbackArg);
    }
}

// Sends one word of a command to the ARM7; negative when the FIFO is full
#define MIC_PXI_SEND(data) PXI_SendWordByFifo(PXI_FIFO_TAG_MIC, (data), FALSE)

// Sends the command that starts the sampling, in seven words
static BOOL MicStartAutoSampling(void *buffer, u32 size, u32 rate, u8 type) {
    if (MIC_PXI_SEND(SPI_PXI_START_BIT | MIC_PXI_COMMAND(SPI_PXI_COMMAND_MIC_AUTO_ON) | type) < 0) {
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
static BOOL MicStopAutoSampling(void) {
    if (MIC_PXI_SEND(SPI_PXI_START_BIT | SPI_PXI_END_BIT | MIC_PXI_COMMAND(SPI_PXI_COMMAND_MIC_AUTO_OFF)) < 0) {
        return FALSE;
    }
    return TRUE;
}

// The callback of the synchronous calls
static void MicGetResultCallback(MICResult result, void *arg) {
    micWork.commonResult = result;
}

#pragma thumb off
// MicWaitBusy: the SDK wrote it in assembly, in ARM (it keeps the address in ip, which MWCC doesn't use for this)
// clang-format off
static asm void MicWaitBusy(void) {
    ldr ip, =micWork
@loop:
    ldr r0, [ip]
    cmp r0, #1
    beq @loop
    bx lr
}
// clang-format on
#pragma thumb on

MICWork *MICi_GetSysWork(void) {
    return &micWork;
}
