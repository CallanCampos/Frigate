// TwlSDK's sound extensions for the DSi (SNDEX), ARM9 side (library code in the DSi-only LTD autoload). The ARM9 sends
// the ARM7 one request at a time over the PXI tag 0x18, as words of a command and a value, and the ARM7 answers with a
// word of the same command and a code; a request's callback, or the thread that waits on a message queue, gets the
// result. Only the code that the game keeps is here: the setup, the request that sets the microphone's filter and the
// two that begin and end the exclusive state (the shutter sound's pre- and post-processing), with what they share. The
// ROM names none of it; the file name and those of the private types and data are descriptive guesses, and the
// functions, commands and codes have the names of libntr's reconstruction of TwlSDK's headers (sndex.h,
// sndex_common.h) where the code shows them, and the statics those of the TwlSDK symbols that Black 1's decompilation
// (aletheia) recovered from libsndex.TWL.HYB.a's sndex.o at the same addresses (twl/sndex.h says what the public
// ones do).

#include "types.h"
#include "nitro/os.h"
#include "nitro/pm.h"
#include "nitro/pxi.h"
#include "nitro/svc.h"
#include "twl/pm.h"
#include "twl/sndex.h"

// The priority of the library's callbacks in the lists of the callbacks that run before the console sleeps or exits
#define SNDEX_PM_PRIORITY 0xfffd

// How long a wait spins between two looks at the state, in cycles of the system clock
#define SNDEX_SPIN_CYCLES 0x82dc

// The commands of the words (bits 16 to 23), as far as the code shows them. The ARM7 answers a command with its own
// number. The answers to 1 to 5 and 0xfe carry a value (bits 0 to 7), which goes to where the request pointed at, a
// byte for 3 and 4 and a word for the others; the requests that send them were left out of the ROM
#define SNDEX_COMMAND_NONE 0
#define SNDEX_PXI_COMMAND_SET_VOLUME 0x84             // a value of at most 31, or at most 7 with bit 6 of the value set
#define SNDEX_PXI_COMMAND_SET_IIRFILTER_TARGET 0xc1   // the filter request: the target, five coefficients and the end
#define SNDEX_PXI_COMMAND_SET_IIRFILTER_PARAM_N0 0xc2 // to 0xc6, for the five coefficients (SNDEXIirFilterParam)
#define SNDEX_PXI_COMMAND_SET_IIRFILTER_PARAM_N1 0xc3
#define SNDEX_PXI_COMMAND_SET_IIRFILTER_PARAM_N2 0xc4
#define SNDEX_PXI_COMMAND_SET_IIRFILTER_PARAM_D1 0xc5
#define SNDEX_PXI_COMMAND_SET_IIRFILTER_PARAM_D2 0xc6
#define SNDEX_PXI_COMMAND_SET_IIRFILTER 0xc7 // the one that the callback waits for
#define SNDEX_PXI_COMMAND_PRE_PROC_SHUTTER 0xfb
#define SNDEX_PXI_COMMAND_POST_PROC_SHUTTER 0xfc
#define SNDEX_PXI_COMMAND_PRESS_VOLSWITCH 0xfd // sent by the ARM7 of its own

// The codes of the answers (bits 8 to 15)
enum {
    SNDEX_PXI_RESULT_SUCCESS = 0,
    SNDEX_PXI_RESULT_INVALID_PARAM = 2,
    SNDEX_PXI_RESULT_EXCLUSIVE = 3,
    SNDEX_PXI_RESULT_ILLEGAL_STATE = 4,
    SNDEX_PXI_RESULT_DEVICE_ERROR = 5, // for the command 4, the byte to write is cleared
};

// The library does one request at a time, and is in the request state from the moment that one starts to that of its
// answer. The exclusive state follows the answer to the request 0xfb, and lasts until the request 0xfc is answered;
// every request but those two is refused in it
typedef enum SndexState {
    SNDEX_STATE_UNINITIALIZED,
    SNDEX_STATE_INITIALIZING,
    SNDEX_STATE_IDLE,
    SNDEX_STATE_REQUEST,
    SNDEX_STATE_EXCLUSIVE,
    SNDEX_STATE_EXCLUSIVE_ENDING,
} SndexState;

// The notification that the ARM7 sends of its own: the code of its word, and the argument that was set with it
typedef void (*SndexNotifyCallback)(u32 code, void *arg);

// The request in progress
typedef struct SndexRequest {
    SNDEXCallback callback; // 0x0
    void *arg;              // 0x4
    void *result;           // 0x8: where the answer's value goes, if the request has one
    u8 command;             // 0xc
} SndexRequest;

// The callback of the notification, which nothing in the game sets
typedef struct SndexNotify {
    SndexNotifyCallback callback; // 0x0
    void *arg;                    // 0x4
    u32 unk_8[2];                 // 0x8: never read or written in the ROM, but the object is 0x10 bytes
} SndexNotify;

// The library's state, in the bss. It is separate statics, not one struct: SNDEXi_SetIirFilter clears its two flags
// before it reloads its result only when they are two objects. -ipa file reads and writes them from one base, and
// lays them out by its sort by size, so they are defined in the order that gives the ROM's layout, not in address
// order (sVolume, sFilterBusy, sVolumeRetries, sState, sVolumeCallbackRetries, sVolumePending, sFilterEndSent,
// sExitInfo, sNotify, sSleepInfo, sRequest). The statics of the volume belong to the code that sets it before the
// console sleeps or exits (ResetTempVolume) while sVolumePending is set, which nothing in the game does: it sends the
// command 0x84 with sVolume, and again, up to five times, from the callback of its answer
static u8 sVolume;
static BOOL sFilterEndSent; // the last word of the filter request is on its way
static BOOL sFilterBusy;    // the filter request is in progress
static int sVolumeRetries;  // of ResetTempVolume
// Volatile because the PXI callback, which runs in the FIFO's interrupt, changes it while a thread looks at it
static volatile SndexState sState;
static u32 sVolumeCallbackRetries; // of the callback of the volume request
static BOOL sVolumePending;        // the volume is waiting to be set
static SndexRequest sRequest;
static SndexNotify sNotify;
static PMExitCallbackInfo sExitInfo;
static PMSleepCallbackInfo sSleepInfo;

// The volume stays an int: the code compares it signed (libntr's sndex.h, of another SDK revision, has u8)
static SNDEXResult SNDEXi_SetVolumeAsync(int volume, SNDEXCallback callback, void *arg, BOOL byLevel);
static SNDEXResult SNDEXi_SetVolumeExAsync(int volume, SNDEXCallback callback, void *arg);
static SNDEXResult SNDEXi_SetIirFilterAsync(SNDEXIirFilterTarget target, const SNDEXIirFilterParam *pParam,
                                            SNDEXCallback callback, void *arg);
static void CommonCallback(PXIFifoTag tag, u32 data, BOOL err);
static void SyncCallback(SNDEXResult result, void *arg);
static BOOL SendCommand(u32 command, u8 value);
static BOOL SendCommandEx(u32 command, u16 value);
static SNDEXResult CheckState(void);
static void ReplyCallback(SNDEXResult result);
static void SetSndexWork(SNDEXCallback callback, void *arg, void *result, u8 command);
static void SndexSleepAndExitCallback(void *arg);
static void WaitIirFilterSetting(void);
static void WaitResetSoundCallback(SNDEXResult result, void *arg);
static void ResetTempVolume(void);

// TwlSDK's SNDEXi_Init, by its code: sets the library up, once
void SNDEXi_Init(void) {
    OSIntrMode irq = OS_DisableInterrupts();

    if (sState != SNDEX_STATE_UNINITIALIZED) {
        OS_RestoreInterrupts(irq);
        return;
    }
    sState = SNDEX_STATE_INITIALIZING;
    OS_RestoreInterrupts(irq);

    SetSndexWork(NULL, NULL, NULL, SNDEX_COMMAND_NONE);
    PXI_Init();
    while (!PXI_IsCallbackReady(PXI_FIFO_TAG_SNDEX, PXI_PROC_ARM7)) {
        SVC_WaitByLoop(10);
    }
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_SNDEX, CommonCallback);

    PM_SetExitCallbackInfo(&sExitInfo, SndexSleepAndExitCallback, NULL);
    PMi_InsertPostExitCallbackEx(&sExitInfo, SNDEX_PM_PRIORITY);
    PM_SetSleepCallbackInfo(&sSleepInfo, SndexSleepAndExitCallback, NULL);
    PMi_InsertPreSleepCallbackEx(&sSleepInfo, SNDEX_PM_PRIORITY);
    sState = SNDEX_STATE_IDLE;
}

// Sends the command 0x84, with a value of at most 31, or at most 7 when byLevel is set
static SNDEXResult SNDEXi_SetVolumeAsync(int volume, SNDEXCallback callback, void *arg, BOOL byLevel) {
    SNDEXResult result;

    if (volume > (byLevel ? 7 : 31)) {
        return SNDEX_RESULT_INVALID_PARAM;
    }
    result = CheckState();
    if (result != SNDEX_RESULT_SUCCESS) {
        return result;
    }
    SetSndexWork(callback, arg, NULL, SNDEX_PXI_COMMAND_SET_VOLUME);
    if (!SendCommand(sRequest.command, (u8)((volume & 0x1f) | (byLevel << 6)))) {
        return SNDEX_RESULT_PXI_SEND_ERROR;
    }
    return SNDEX_RESULT_SUCCESS;
}

static SNDEXResult SNDEXi_SetVolumeExAsync(int volume, SNDEXCallback callback, void *arg) {
    if (!OS_IsRunOnTwl()) {
        return SNDEX_RESULT_ILLEGAL_STATE;
    }
    return SNDEXi_SetVolumeAsync(volume, callback, arg, FALSE);
}

// Sends the words of the filter request, one after the other, and gives the last the callback
static SNDEXResult SNDEXi_SetIirFilterAsync(SNDEXIirFilterTarget target, const SNDEXIirFilterParam *pParam,
                                            SNDEXCallback callback, void *arg) {
    SNDEXResult result;

    if (sFilterBusy) {
        return SNDEX_RESULT_EXCLUSIVE;
    }
    sFilterBusy = TRUE;
    if (!OSi_IsCodecTwlMode()) {
        sFilterBusy = FALSE;
        return SNDEX_RESULT_ILLEGAL_STATE;
    }
    result = CheckState();
    if (result != SNDEX_RESULT_SUCCESS) {
        sFilterBusy = FALSE;
        return result;
    }

    SetSndexWork(NULL, NULL, NULL, SNDEX_PXI_COMMAND_SET_IIRFILTER_TARGET);
    if (!SendCommandEx(sRequest.command, (u16)target)) {
        sFilterBusy = FALSE;
        return SNDEX_RESULT_PXI_SEND_ERROR;
    }
    // Each word waits until the answer to the last one is in and the library is idle again
    OS_SpinWaitSysCycles(SNDEX_SPIN_CYCLES);
    while (CheckState() != SNDEX_RESULT_SUCCESS) {
        OS_SpinWaitSysCycles(SNDEX_SPIN_CYCLES);
    }
    SetSndexWork(NULL, NULL, NULL, SNDEX_PXI_COMMAND_SET_IIRFILTER_PARAM_N0);
    if (!SendCommandEx(sRequest.command, pParam->n0)) {
        sFilterBusy = FALSE;
        return SNDEX_RESULT_PXI_SEND_ERROR;
    }
    OS_SpinWaitSysCycles(SNDEX_SPIN_CYCLES);
    while (CheckState() != SNDEX_RESULT_SUCCESS) {
        OS_SpinWaitSysCycles(SNDEX_SPIN_CYCLES);
    }
    SetSndexWork(NULL, NULL, NULL, SNDEX_PXI_COMMAND_SET_IIRFILTER_PARAM_N1);
    if (!SendCommandEx(sRequest.command, pParam->n1)) {
        sFilterBusy = FALSE;
        return SNDEX_RESULT_PXI_SEND_ERROR;
    }
    OS_SpinWaitSysCycles(SNDEX_SPIN_CYCLES);
    while (CheckState() != SNDEX_RESULT_SUCCESS) {
        OS_SpinWaitSysCycles(SNDEX_SPIN_CYCLES);
    }
    SetSndexWork(NULL, NULL, NULL, SNDEX_PXI_COMMAND_SET_IIRFILTER_PARAM_N2);
    if (!SendCommandEx(sRequest.command, pParam->n2)) {
        sFilterBusy = FALSE;
        return SNDEX_RESULT_PXI_SEND_ERROR;
    }
    OS_SpinWaitSysCycles(SNDEX_SPIN_CYCLES);
    while (CheckState() != SNDEX_RESULT_SUCCESS) {
        OS_SpinWaitSysCycles(SNDEX_SPIN_CYCLES);
    }
    SetSndexWork(NULL, NULL, NULL, SNDEX_PXI_COMMAND_SET_IIRFILTER_PARAM_D1);
    if (!SendCommandEx(sRequest.command, pParam->d1)) {
        sFilterBusy = FALSE;
        return SNDEX_RESULT_PXI_SEND_ERROR;
    }
    OS_SpinWaitSysCycles(SNDEX_SPIN_CYCLES);
    while (CheckState() != SNDEX_RESULT_SUCCESS) {
        OS_SpinWaitSysCycles(SNDEX_SPIN_CYCLES);
    }
    SetSndexWork(NULL, NULL, NULL, SNDEX_PXI_COMMAND_SET_IIRFILTER_PARAM_D2);
    if (!SendCommandEx(sRequest.command, pParam->d2)) {
        sFilterBusy = FALSE;
        return SNDEX_RESULT_PXI_SEND_ERROR;
    }
    OS_SpinWaitSysCycles(SNDEX_SPIN_CYCLES);
    while (CheckState() != SNDEX_RESULT_SUCCESS) {
        OS_SpinWaitSysCycles(SNDEX_SPIN_CYCLES);
    }
    SetSndexWork(callback, arg, NULL, SNDEX_PXI_COMMAND_SET_IIRFILTER);
    if (!SendCommandEx(sRequest.command, 0)) {
        sFilterBusy = FALSE;
        return SNDEX_RESULT_PXI_SEND_ERROR;
    }
    return SNDEX_RESULT_SUCCESS;
}

// Sets the microphone's filter and waits for the answer
SNDEXResult SNDEXi_SetIirFilter(SNDEXIirFilterTarget target, const SNDEXIirFilterParam *pParam) {
    SNDEXResult result;
    OSMessage messages[1];
    OSMessageQueue queue;

    if (OS_GetCurrentThread() == NULL) {
        return SNDEX_RESULT_ILLEGAL_STATE;
    }
    OS_InitMessageQueue(&queue, messages, 1);
    result = SNDEXi_SetIirFilterAsync(target, pParam, SyncCallback, &queue);
    if (result == SNDEX_RESULT_SUCCESS) {
        OS_ReceiveMessage(&queue, (OSMessage *)&result, OS_MESSAGE_BLOCK);
    }
    sFilterEndSent = FALSE;
    sFilterBusy = FALSE;
    return result;
}

// The request 0xfb: waits for the answer, and leaves the library in the exclusive state
SNDEXResult SNDEXi_PreProcessForShutterSound(void) {
    SNDEXResult result;
    OSMessage messages[1];
    OSMessageQueue queue;

    if (!OSi_IsCodecTwlMode()) {
        return SNDEX_RESULT_ILLEGAL_STATE;
    }
    if (OS_GetCurrentThread() == NULL) {
        return SNDEX_RESULT_ILLEGAL_STATE;
    }
    // In the exclusive state the request goes on without starting; the second test of the state needs it to change
    // between the two reads
    if (sState != SNDEX_STATE_EXCLUSIVE) {
        result = CheckState();
        if (result != SNDEX_RESULT_SUCCESS) {
            return result;
        }
    } else if (sState == SNDEX_STATE_EXCLUSIVE_ENDING) {
        return SNDEX_RESULT_EXCLUSIVE;
    }

    OS_InitMessageQueue(&queue, messages, 1);
    SetSndexWork(SyncCallback, &queue, NULL, SNDEX_PXI_COMMAND_PRE_PROC_SHUTTER);
    if (!SendCommand(sRequest.command, 0)) {
        return SNDEX_RESULT_PXI_SEND_ERROR;
    }
    OS_ReceiveMessage(&queue, (OSMessage *)&result, OS_MESSAGE_BLOCK);
    return result;
}

// The request 0xfc, which ends the exclusive state: the callback gets the answer
SNDEXResult SNDEXi_PostProcessForShutterSound(SNDEXCallback callback, void *arg) {
    SNDEXResult result;

    if (!OSi_IsCodecTwlMode() || sState != SNDEX_STATE_EXCLUSIVE) {
        return SNDEX_RESULT_ILLEGAL_STATE;
    }
    result = SNDEX_RESULT_PXI_SEND_ERROR;
    sState = SNDEX_STATE_EXCLUSIVE_ENDING;
    SetSndexWork(callback, arg, NULL, SNDEX_PXI_COMMAND_POST_PROC_SHUTTER);
    if (SendCommand(sRequest.command, 0)) {
        result = SNDEX_RESULT_SUCCESS;
    }
    return result;
}

// The PXI callback, which gets the ARM7's words: the answers to the request in progress, and the notifications that it
// sends of its own
static void CommonCallback(PXIFifoTag tag, u32 data, BOOL err) {
    u8 command = (u8)((data & 0xff0000) >> 16);
    u8 answer = (u8)((data & 0xff00) >> 8);
    u8 value = (u8)(data & 0xff);

    if (command == SNDEX_PXI_COMMAND_PRESS_VOLSWITCH) {
        if (CheckState() != SNDEX_RESULT_SUCCESS) {
            return;
        }
        SetSndexWork(NULL, NULL, NULL, SNDEX_PXI_COMMAND_PRESS_VOLSWITCH);
    }
    if (tag != PXI_FIFO_TAG_SNDEX || err == TRUE ||
        (sState != SNDEX_STATE_REQUEST && sState != SNDEX_STATE_EXCLUSIVE && sState != SNDEX_STATE_EXCLUSIVE_ENDING) ||
        sRequest.command != command) {
        ReplyCallback(SNDEX_RESULT_FATAL_ERROR);
        return;
    }

    switch (answer) {
    case SNDEX_PXI_RESULT_SUCCESS:
        switch (command) {
        case 1:
            if (sRequest.result != NULL) {
                *(u32 *)sRequest.result = value;
            }
            break;
        case 2:
            if (sRequest.result != NULL) {
                *(u32 *)sRequest.result = value;
            }
            break;
        case 3:
        case 4:
            if (sRequest.result != NULL) {
                *(u8 *)sRequest.result = value;
            }
            break;
        case 5:
            if (sRequest.result != NULL) {
                *(u32 *)sRequest.result = value;
            }
            break;
        case SNDEX_PXI_COMMAND_PRESS_VOLSWITCH:
            if (sNotify.callback != NULL) {
                sNotify.callback(answer, sNotify.arg);
            }
            break;
        case 0xfe:
            if (sRequest.result != NULL) {
                *(u32 *)sRequest.result = value;
            }
            break;
        }
        ReplyCallback(SNDEX_RESULT_SUCCESS);
        break;
    case SNDEX_PXI_RESULT_INVALID_PARAM:
        ReplyCallback(SNDEX_RESULT_INVALID_PARAM);
        break;
    case SNDEX_PXI_RESULT_EXCLUSIVE:
        ReplyCallback(SNDEX_RESULT_EXCLUSIVE);
        break;
    case SNDEX_PXI_RESULT_ILLEGAL_STATE:
        ReplyCallback(SNDEX_RESULT_ILLEGAL_STATE);
        break;
    case SNDEX_PXI_RESULT_DEVICE_ERROR:
        if (command == 4) {
            if (sRequest.result != NULL) {
                *(u8 *)sRequest.result = 0;
            }
        }
        ReplyCallback(SNDEX_RESULT_DEVICE_ERROR);
        break;
    default:
        ReplyCallback(SNDEX_RESULT_FATAL_ERROR);
        break;
    }
}

// The callback of the synchronous requests: sends the result to the thread that waits
static void SyncCallback(SNDEXResult result, void *arg) {
    OS_SendMessage(arg, (OSMessage)result, OS_MESSAGE_NOBLOCK);
}

// Sends a word with a command and a value of one byte. FALSE, and the state back to idle, when the FIFO refuses it
static BOOL SendCommand(u32 command, u8 value) {
    OSIntrMode irq = OS_DisableInterrupts();

    if (PXI_SendWordByFifo(PXI_FIFO_TAG_SNDEX, ((command << 16) & 0xff0000) | (value & 0xff), FALSE) < 0) {
        sState = SNDEX_STATE_IDLE;
        OS_RestoreInterrupts(irq);
        return FALSE;
    }
    OS_RestoreInterrupts(irq);
    return TRUE;
}

// Sends a word with a command and a value of two bytes
static BOOL SendCommandEx(u32 command, u16 value) {
    OSIntrMode irq = OS_DisableInterrupts();

    if (PXI_SendWordByFifo(PXI_FIFO_TAG_SNDEX, ((command << 16) & 0xff0000) | (value & 0xffff), FALSE) < 0) {
        sState = SNDEX_STATE_IDLE;
        OS_RestoreInterrupts(irq);
        return FALSE;
    }
    if (command == SNDEX_PXI_COMMAND_SET_IIRFILTER) {
        sFilterEndSent = TRUE;
    }
    OS_RestoreInterrupts(irq);
    return TRUE;
}

// Starts a request if the library is idle, and puts it in the request state. It is not set up in the first two states,
// and busy in the request and exclusive ones
static SNDEXResult CheckState(void) {
    OSIntrMode irq = OS_DisableInterrupts();

    switch (sState) {
    case SNDEX_STATE_UNINITIALIZED:
    case SNDEX_STATE_INITIALIZING:
        OS_RestoreInterrupts(irq);
        return SNDEX_RESULT_BEFORE_INIT;
    case SNDEX_STATE_REQUEST:
    case SNDEX_STATE_EXCLUSIVE:
    case SNDEX_STATE_EXCLUSIVE_ENDING:
        OS_RestoreInterrupts(irq);
        return SNDEX_RESULT_EXCLUSIVE;
    default:
        sState = SNDEX_STATE_REQUEST;
        OS_RestoreInterrupts(irq);
        return SNDEX_RESULT_SUCCESS;
    }
}

// Ends the request in progress: clears it, goes back to idle (or to the exclusive state, after the request 0xfb) and
// calls its callback with the result
static void ReplyCallback(SNDEXResult result) {
    OSIntrMode irq = OS_DisableInterrupts();
    u8 command = sRequest.command;
    SNDEXCallback callback = sRequest.callback;
    void *arg = sRequest.arg;

    if (command == SNDEX_PXI_COMMAND_SET_IIRFILTER) {
        sFilterEndSent = FALSE;
        sFilterBusy = FALSE;
    }
    SetSndexWork(NULL, NULL, NULL, SNDEX_COMMAND_NONE);
    if (command == SNDEX_PXI_COMMAND_PRE_PROC_SHUTTER) {
        sState = SNDEX_STATE_EXCLUSIVE;
    } else {
        sState = SNDEX_STATE_IDLE;
    }
    OS_RestoreInterrupts(irq);
    if (callback != NULL) {
        callback(result, arg);
    }
}

// Sets up the request in progress
static void SetSndexWork(SNDEXCallback callback, void *arg, void *result, u8 command) {
    sRequest.callback = callback;
    sRequest.arg = arg;
    sRequest.result = result;
    sRequest.command = command;
}

// The callback that runs before the console sleeps or exits
static void SndexSleepAndExitCallback(void *arg) {
    WaitIirFilterSetting();
    ResetTempVolume();
}

// Waits until the last word of the filter request has been answered
static void WaitIirFilterSetting(void) {
    while (sFilterEndSent) {
        OS_SpinWaitSysCycles(SNDEX_SPIN_CYCLES);
        PXIi_HandlerRecvFifoNotEmpty();
    }
}

// The callback of the volume request: sends it again if it failed, up to five times, and ends the wait otherwise
static void WaitResetSoundCallback(SNDEXResult result, void *arg) {
    if (result != SNDEX_RESULT_SUCCESS && sVolumeCallbackRetries < 5) {
        SNDEXi_SetVolumeExAsync(sVolume, WaitResetSoundCallback, NULL);
        sVolumeCallbackRetries++;
    } else {
        sVolumePending = FALSE;
    }
}

// Sends the volume request, if one is pending, and waits until its callback is done with it
static void ResetTempVolume(void) {
    if (sVolumePending) {
        while (SNDEXi_SetVolumeExAsync(sVolume, WaitResetSoundCallback, NULL) != SNDEX_RESULT_SUCCESS &&
               sVolumeRetries < 5) {
            sVolumeRetries++;
        }
        while (sVolumePending) {
            OS_SpinWaitSysCycles(SNDEX_SPIN_CYCLES);
            PXIi_HandlerRecvFifoNotEmpty();
        }
    }
}
