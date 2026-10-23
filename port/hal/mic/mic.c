// NitroSDK's microphone on the host (mic.c, which port/hal/replaced.txt keeps out of the game's sources for its
// assembly wait, MicWaitBusy), and the host ARM7's side of its PXI tag (MIC), whose microphone hears silence.
//
// The ARM9's functions keep the decomp's interface and protocol: a command's first word has the command in bits 8 to
// 14 and a byte of data, each word after it 16 bits (the start, 0x41, carries the type, the buffer, its size and the
// rate in ARM7 cycles a sample; the stop is 0x42); the ARM7 answers with the command and a result in the low byte, and
// sends 0x51 when a buffer that doesn't loop is full. The answer's callback ends the call from the interrupt, as on the
// DS, and the synchronous functions' waits are modelled busy loops. The buffer's address travels in two 16-bit
// halves, which hold only its low 32 bits on a 64-bit host; the ARM7's side here takes the whole pointer that the
// ARM9's side noted for the command it sent (both are this file). MIC_GetLastSamplingAddress gives the host pointer,
// where the DS reads the 32-bit word that the ARM7 keeps in the shared memory. That word still gets an address, since
// OS_GetLowEntropyData mixes it into the seeds the game draws (gf_standard.c's buildSeed, which a battle's random
// numbers start from), but not the host's: the last sample's place in the buffer over a fixed DS main memory address
// (MIC_SHARED_ADDRESS_BASE), so that a run's seeds don't depend on where the host put the buffer.
//
// The sampler runs on the virtual clock: at each VBlank it writes the samples of the time since the last one, at the
// rate, as silence (the middle of the range for the unsigned types, 0 for the signed ones; 12-bit samples fill the
// high bits of a halfword). A buffer that loops starts again from its start without a word; one that doesn't stops
// the sampling when it is full and the full callback is told (MIC_PXI_COMMAND_BUFFER_FULL). The DSi's MICEX path of
// the Limited functions is never taken: a host console isn't a DSi.
#include "nitro/mic.h"
#include "hal/os/port_os.h"
#include "hal/pxi/arm7.h"
#include "hal/pxi/pxi_bus.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "twl/mic.h"

#define MIC_PXI_START 0x02000000
#define MIC_PXI_END 0x01000000
#define MIC_PXI_INDEX(index) ((index) << 16)
#define MIC_PXI_COMMAND(command) ((command) << 8)
#define MIC_PXI_COMMAND_SAMPLING_ONCE 0x40
#define MIC_PXI_COMMAND_AUTO_ON 0x41
#define MIC_PXI_COMMAND_AUTO_OFF 0x42
#define MIC_PXI_COMMAND_BUFFER_FULL 0x51
#define MIC_PXI_COMMAND_MASK 0x7f00
#define MIC_PXI_RESULT_MASK 0xff

#define MIC_PXI_RESULT_SUCCESS 0
#define MIC_PXI_RESULT_INVALID_COMMAND 1
#define MIC_PXI_RESULT_INVALID_PARAMETER 2
#define MIC_PXI_RESULT_ILLEGAL_STATUS 3
#define MIC_PXI_RESULT_BUSY 4

#define MIC_PXI_TYPE_8BIT 0
#define MIC_PXI_TYPE_12BIT 1
#define MIC_PXI_TYPE_SIGNED_8BIT 2
#define MIC_PXI_TYPE_SIGNED_12BIT 3
#define MIC_PXI_TYPE_12BIT_FILTER_OFF 5
#define MIC_PXI_TYPE_SIGNED_12BIT_FILTER_OFF 7
#define MIC_PXI_TYPE_SIGNED 0x2
#define MIC_PXI_TYPE_12BIT_MASK 0x1
#define MIC_PXI_TYPE_LOOP 0x10

#define MIC_ALIGN_MASK 0x1f

// What the shared memory's last sampling address counts from (see above): the DS's main memory
#define MIC_SHARED_ADDRESS_BASE 0x02000000

// The cycles of each turn of MicWaitBusy's loop: a load, a compare and a branch
#define MIC_WAIT_CYCLES 8

// The ARM9's side: the SDK's statics, and the buffer of the start command it sent last (see above)
static u16 micInitialized;
static MICWork micWork;
static void *sSentBuffer;

// The ARM7's side: the command it collects, and the sampling: the buffer and its end, where the next sample goes, the
// type, the rate, and the virtual clock's cycle the next sample is due at
static struct {
    PortArm7SpiCommand command;
    BOOL sampling;
    u8 *buffer;
    u8 *end;
    u8 *next;
    u8 *last;
    u32 type;
    u32 rate;
    u64 due;
} sMic;

static void MicCommonCallback(PXIFifoTag tag, u32 data, BOOL err);
static BOOL MicStartAutoSampling(void *buffer, u32 size, u32 rate, u8 type);
static BOOL MicStopAutoSampling(void);
static void MicGetResultCallback(MICResult result, void *arg);
static void MicWaitBusy(void);
static MICResult MicWaitResult(MICResult sent);

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
    u32 enabled;
    int type;
    u8 mode;

    if ((uintptr_t)param->buffer & MIC_ALIGN_MASK) {
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
    mode = param->loop_enable ? (u8)(type | MIC_PXI_TYPE_LOOP) : (u8)type;

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

MICResult MIC_StartAutoSampling(const MICAutoParam *param) {
    return MicWaitResult(MIC_StartAutoSamplingAsync(param, MicGetResultCallback, NULL));
}

MICResult MIC_StopAutoSamplingAsync(MICCallback callback, void *arg) {
    u32 enabled = OS_DisableInterrupts();

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

MICResult MIC_StopAutoSampling(void) {
    return MicWaitResult(MIC_StopAutoSamplingAsync(MicGetResultCallback, NULL));
}

// The last sample written, as a host pointer (NULL before the first)
void *MIC_GetLastSamplingAddress(void) {
    return sMic.last;
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

MICWork *MICi_GetSysWork(void) {
    return &micWork;
}

// NitroSDK's MicCommonCallback: the ARM7's words with the microphone's tag
static void MicCommonCallback(PXIFifoTag tag, u32 data, BOOL err) {
    MICCallback callback;
    MICResult result;
    u16 command;
    u16 status;

    if (err) {
        micWork.lock = FALSE;
        callback = micWork.callback;
        if (callback) {
            micWork.callback = NULL;
            callback(MIC_RESULT_FATAL_ERROR, micWork.callbackArg);
        }
    }

    command = (data & MIC_PXI_COMMAND_MASK) >> 8;
    status = data & MIC_PXI_RESULT_MASK;
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
    case MIC_PXI_RESULT_BUSY:
        result = MIC_RESULT_BUSY;
        break;
    default:
        result = MIC_RESULT_FATAL_ERROR;
        break;
    }

    if (command == MIC_PXI_COMMAND_BUFFER_FULL) {
        if (micWork.full) {
            micWork.full(result, micWork.fullArg);
        }
        return;
    }
    if (command == MIC_PXI_COMMAND_SAMPLING_ONCE) {
        if (micWork.dst_buf) {
            *micWork.dst_buf = OS_GetSystemWork()->mic_sampling_data;
        }
    }

    micWork.lock = FALSE;
    callback = micWork.callback;
    if (callback) {
        micWork.callback = NULL;
        callback(result, micWork.callbackArg);
    }
}

#define MIC_PXI_SEND(data) PXI_SendWordByFifo(PXI_FIFO_TAG_MIC, (data), FALSE)

// Sends the command that starts the sampling, in seven words, with the buffer's low 32 bits; the ARM7's side takes the
// pointer from sSentBuffer
static BOOL MicStartAutoSampling(void *buffer, u32 size, u32 rate, u8 type) {
    u32 address = (u32)(uintptr_t)buffer;

    sSentBuffer = buffer;
    if (MIC_PXI_SEND(MIC_PXI_START | MIC_PXI_COMMAND(MIC_PXI_COMMAND_AUTO_ON) | type) < 0) {
        return FALSE;
    }
    if (MIC_PXI_SEND(MIC_PXI_INDEX(1) | (address >> 16)) < 0) {
        return FALSE;
    }
    if (MIC_PXI_SEND(MIC_PXI_INDEX(2) | (address & 0xffff)) < 0) {
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
    if (MIC_PXI_SEND(MIC_PXI_END | MIC_PXI_INDEX(6) | (rate & 0xffff)) < 0) {
        return FALSE;
    }
    return TRUE;
}

static BOOL MicStopAutoSampling(void) {
    if (MIC_PXI_SEND(MIC_PXI_START | MIC_PXI_END | MIC_PXI_COMMAND(MIC_PXI_COMMAND_AUTO_OFF)) < 0) {
        return FALSE;
    }
    return TRUE;
}

static void MicGetResultCallback(MICResult result, void *arg) {
    micWork.commonResult = result;
}

// NitroSDK's MicWaitBusy, assembly in the SDK: waits for the busy flag that the answer clears, as a modelled busy loop
static void MicWaitBusy(void) {
    while (micWork.lock == TRUE) {
        port_os_spin(MIC_WAIT_CYCLES);
    }
}

// The end of the synchronous functions: the result of the asynchronous call that sent the command, or the answer's.
// mic.c stores the call's result before it waits, which the answer's callback overwrites when it comes; on the host the
// answer comes inside the call, before that store, so the call's result is kept apart here
static MICResult MicWaitResult(MICResult sent) {
    if (sent != MIC_RESULT_SUCCESS) {
        micWork.commonResult = sent;
        return sent;
    }
    MicWaitBusy();
    return micWork.commonResult;
}

// The ARM7's side

static void Answer(u32 command, u32 result) {
    port_pxi_reply(PXI_FIFO_TAG_MIC, PORT_ARM7_SPI_ANSWER(command, result), FALSE);
}

static u32 SampleSize(u32 type) {
    return (type & MIC_PXI_TYPE_12BIT_MASK) ? 2 : 1;
}

// Writes the samples from next up to count of them, or to the end of the buffer
static u32 WriteSilence(u32 count) {
    u32 size = SampleSize(sMic.type);
    BOOL isSigned = (sMic.type & MIC_PXI_TYPE_SIGNED) != 0;
    u32 written = 0;

    while (written < count && (uintptr_t)(sMic.end - sMic.next) >= size) {
        if (size == 2) {
            u16 silence = isSigned ? 0 : 0x8000;

            sMic.next[0] = (u8)silence;
            sMic.next[1] = (u8)(silence >> 8);
        } else {
            sMic.next[0] = isSigned ? 0 : 0x80;
        }
        sMic.last = sMic.next;
        sMic.next += size;
        written++;
    }
    if (written != 0) {
        OS_GetSystemWork()->mic_last_address = MIC_SHARED_ADDRESS_BASE + (u32)(sMic.last - sMic.buffer);
    }
    return written;
}

static u32 StartSampling(const PortArm7SpiCommand *command) {
    u32 address = ((u32)command->data[1] << 16) | command->data[2];
    u32 size = ((u32)command->data[3] << 16) | command->data[4];
    u32 rate = ((u32)command->data[5] << 16) | command->data[6];
    u8 *buffer = sSentBuffer;

    if (sMic.sampling) {
        return MIC_PXI_RESULT_ILLEGAL_STATUS;
    }
    if (buffer == NULL || (u32)(uintptr_t)buffer != address || size == 0 || rate < MIC_SAMPLING_RATE_LIMIT) {
        return MIC_PXI_RESULT_INVALID_PARAMETER;
    }
    sMic.sampling = TRUE;
    sMic.buffer = buffer;
    sMic.end = buffer + size;
    sMic.next = buffer;
    sMic.type = command->data[0];
    sMic.rate = rate;
    sMic.due = port_os_cycles() + rate;
    return MIC_PXI_RESULT_SUCCESS;
}

// The MIC tag's callback: collects the ARM9's words into a command, then starts or stops the automatic sampling into
// the game's buffer (of silence), and answers as the DS's ARM7 answers mic.c
static void MicService(u32 tag, u32 data, BOOL err) {
    u32 command;

    if (err || !port_arm7_spi_collect(&sMic.command, data)) {
        return;
    }
    command = sMic.command.command;
    switch (command) {
    case MIC_PXI_COMMAND_AUTO_ON:
        Answer(command, sMic.command.words == 7 ? StartSampling(&sMic.command) : MIC_PXI_RESULT_INVALID_PARAMETER);
        break;
    case MIC_PXI_COMMAND_AUTO_OFF:
        if (!sMic.sampling) {
            Answer(command, MIC_PXI_RESULT_ILLEGAL_STATUS);
        } else {
            sMic.sampling = FALSE;
            Answer(command, MIC_PXI_RESULT_SUCCESS);
        }
        break;
    default:
        Answer(command, MIC_PXI_RESULT_INVALID_COMMAND);
        break;
    }
}

void port_mic7_start(void) {
    sMic.command.count = 0;
    sMic.sampling = FALSE;
    sMic.buffer = NULL;
    sMic.end = NULL;
    sMic.next = NULL;
    sMic.last = NULL;
    sSentBuffer = NULL;
    OS_GetSystemWork()->mic_last_address = 0;
    port_pxi_register_service(PXI_FIFO_TAG_MIC, MicService);
}

void port_mic7_frame(u64 cycles) {
    u32 count;

    if (!sMic.sampling || cycles < sMic.due) {
        return;
    }
    count = (u32)((cycles - sMic.due) / sMic.rate) + 1;
    sMic.due += (u64)count * sMic.rate;
    while (count > 0 && sMic.sampling) {
        count -= WriteSilence(count);
        if ((uintptr_t)(sMic.end - sMic.next) < SampleSize(sMic.type)) {
            if (sMic.type & MIC_PXI_TYPE_LOOP) {
                sMic.next = sMic.buffer;
            } else {
                sMic.sampling = FALSE;
                Answer(MIC_PXI_COMMAND_BUFFER_FULL, MIC_PXI_RESULT_SUCCESS);
            }
        }
    }
}
