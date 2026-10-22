// NitroSDK's NVRAM library (the file name is a guess): the ARM9's access to the console's NVRAM, which is the flash of
// the firmware. Reading, writing and verifying are calls that send the ARM7 their commands through the PXI FIFO and
// finish in the function that gets its answers. The write sends a page at a time, and after each polls the flash's
// status register on an alarm until the flash is done. The public functions have the SDK's names, known from how the
// code works but not from the ROM; the statics are named by their addresses, and the static inlines' names are ours.
#include "nitro/nvram.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"

// A command is a series of words with the NVRAM's tag: the first has NVRAM_PXI_START, the command in bits 8 to 14 and
// 8 bits of data, the last has NVRAM_PXI_END, and each of the others carries its index in bits 16 to 22 and 16 bits of
// data
#define NVRAM_PXI_START 0x02000000
#define NVRAM_PXI_END 0x01000000
#define NVRAM_PXI_INDEX(index) ((index) << 16)
#define NVRAM_PXI_COMMAND(command) ((command) << 8)
#define NVRAM_PXI_COMMAND_WRITE_ENABLE 0x20
#define NVRAM_PXI_COMMAND_WRITE_DISABLE 0x21
#define NVRAM_PXI_COMMAND_READ_STATUS 0x22
#define NVRAM_PXI_COMMAND_READ 0x23
#define NVRAM_PXI_COMMAND_PAGE_WRITE 0x25
// Sent when the status register shows its error bit, after which the write ends in an error
#define NVRAM_PXI_COMMAND_CLEAR_ERROR 0x2d

// What the ARM7 answers with in bits 0 to 7 of a word, which repeats the command in bits 8 to 14
#define NVRAM_PXI_RESULT_MASK 0xff
#define NVRAM_PXI_RESULT_SUCCESS 0
#define NVRAM_PXI_RESULT_ILLEGAL_PARAMETER 2
#define NVRAM_PXI_RESULT_ILLEGAL_STATUS 3
#define NVRAM_PXI_RESULT_BUSY 4

// The flash's status register: the bit set while it works, and the bit of an error
#define NVRAM_STATUS_WRITE_IN_PROGRESS 0x01
#define NVRAM_STATUS_ERROR 0x20

// A flash page is 256 bytes, which a write may not cross. The cache line of 32 bytes is the unit of what the ARM7
// writes into the buffers
#define NVRAM_PAGE_SIZE 0x100
#define NVRAM_BUFFER_SIZE 0x20

// What the poll waits between two reads of the status register
#define NVRAM_POLL_MSEC 1

typedef enum NVRAMInitState {
    NVRAM_INIT_NONE,
    NVRAM_INIT_RUNNING,
    NVRAM_INIT_DONE,
} NVRAMInitState;

// The call that runs, if any
typedef enum NVRAMCommand {
    NVRAM_COMMAND_NONE,
    NVRAM_COMMAND_READ,
    NVRAM_COMMAND_WRITE,
    NVRAM_COMMAND_VERIFY,
} NVRAMCommand;

// How far the write has got: what the ARM7 answers to, or the alarm that stands in for it
typedef enum NVRAMWritePhase {
    NVRAM_WRITE_ENABLE_SENT,
    NVRAM_WRITE_PAGE_SENT,
    NVRAM_WRITE_STATUS_SENT,
    NVRAM_WRITE_DISABLE_SENT,
    NVRAM_WRITE_CLEAR_ERROR_SENT,
    NVRAM_WRITE_POLL_WAIT,
} NVRAMWritePhase;

typedef struct NVRAMWork {
    NVRAMCommand command;
    NVRAMCallback callback;
    void *callbackArg;
    // Fires NVRAM_POLL_MSEC after a request that the ARM7 refused as busy, or a page that was sent, and calls the
    // answer function as if the ARM7 had answered with a result in its argument
    OSAlarm alarm;
    // The tick at which the last page was sent
    u64 pageTick;
    // The phase of a write
    u32 phase;
    // What remains to be read, written or verified, from where, and to or from where
    u32 address;
    u32 size;
    u8 *buf;
    // The threads waiting for the call to end
    OSThreadQueue queue;
} NVRAMWork;

// The .bss lays them out in the reverse of this order: the state of the initialization, the buffer, which is 32-byte
// aligned as the ARM7 writes into it and the cache has to be invalidated for it, and the work
static NVRAMWork sNvramWork;
static u8 sNvramBuffer[NVRAM_BUFFER_SIZE] ATTRIBUTE_ALIGN(32);
static NVRAMInitState sNvramInitState;

static void func_02079944(PXIFifoTag tag, u32 data, BOOL err);
static void func_020792f4(u32 data, BOOL err);
static void func_02079458(u32 data, BOOL err);
static void func_020797d4(u32 data, BOOL err);
static BOOL func_02079930(void);
static void func_0207997c(NVRAMResult result, void *arg);
static void func_02079994(void *arg);
static BOOL func_020799a4(const u8 *a, const u8 *b, u32 size);
static BOOL func_020799c8(void);
static BOOL func_020799e4(void);
static BOOL func_02079a00(u8 *status);
static BOOL func_02079a5c(u32 address, u32 size, void *buf);
static BOOL func_02079b08(u32 address, u16 size, const void *src);
static BOOL func_02079b94(void);

// Calls back with the result, ending the call, and wakes the threads that wait for it
static inline void NVRAMi_EndCommand(NVRAMResult result) {
    NVRAMCallback callback;

    sNvramWork.command = NVRAM_COMMAND_NONE;
    sNvramWork.phase = 0;
    callback = sNvramWork.callback;
    if (callback) {
        sNvramWork.callback = NULL;
        callback(result, sNvramWork.callbackArg);
    }
    OS_WakeupThread(&sNvramWork.queue);
}

// Fires the alarm (OS_SetAlarm) that stands in for an answer of the ARM7, after the poll time
static inline void NVRAMi_SetAlarm(u32 pxiResult) {
    OS_SetAlarm(&sNvramWork.alarm, OS_MilliSecondsToTicks(NVRAM_POLL_MSEC), func_02079994, (void *)pxiResult);
}

void NVRAM_Init(void) {
    OSIntrMode enabled;

    enabled = OS_DisableInterrupts();
    if (sNvramInitState != NVRAM_INIT_NONE) {
        OS_RestoreInterrupts(enabled);
        return;
    }
    sNvramInitState = NVRAM_INIT_RUNNING;
    OS_RestoreInterrupts(enabled);
    PXI_Init();
    while (!PXI_IsCallbackReady(PXI_FIFO_TAG_NVRAM, PXI_PROC_ARM7)) {
    }
    PXI_SetFifoRecvCallback(PXI_FIFO_TAG_NVRAM, func_02079944);
    MI_CpuClearFast(&sNvramWork, sizeof(sNvramWork));
    if (!OS_IsTickAvailable()) {
        OS_InitTick();
    }
    if (!OS_IsAlarmAvailable()) {
        OS_InitAlarm();
    }
    OS_CreateAlarm(&sNvramWork.alarm);
    OS_InitThreadQueue(&sNvramWork.queue);
    sNvramInitState = NVRAM_INIT_DONE;
}

NVRAMResult NVRAM_ReadDataBytesAsync(u32 address, u32 size, void *buf, NVRAMCallback callback, void *arg) {
    OSIntrMode enabled;

    if (!func_02079930()) {
        return NVRAM_RESULT_ILLEGAL_STATUS;
    }
    enabled = OS_DisableInterrupts();
    if (sNvramWork.command != NVRAM_COMMAND_NONE) {
        OS_RestoreInterrupts(enabled);
        return NVRAM_RESULT_BUSY;
    }
    sNvramWork.command = NVRAM_COMMAND_READ;
    OS_RestoreInterrupts(enabled);
    sNvramWork.callback = callback;
    sNvramWork.callbackArg = arg;
    sNvramWork.phase = 0;
    enabled = OS_DisableInterrupts();
    if (func_02079a5c(address, size, buf) == TRUE) {
        OS_RestoreInterrupts(enabled);
        return NVRAM_RESULT_SUCCESS;
    }
    sNvramWork.command = NVRAM_COMMAND_NONE;
    OS_RestoreInterrupts(enabled);
    OS_WakeupThread(&sNvramWork.queue);
    return NVRAM_RESULT_SEND_ERROR;
}

// The answer to a read
static void func_020792f4(u32 data, BOOL err) {
    NVRAMResult result = NVRAM_RESULT_FATAL_ERROR;
    u16 status = data & NVRAM_PXI_RESULT_MASK;

    if (err == FALSE && status == NVRAM_PXI_RESULT_SUCCESS) {
        result = NVRAM_RESULT_SUCCESS;
    } else if (err == TRUE) {
        result = NVRAM_RESULT_SEND_ERROR;
    } else {
        switch (status) {
        case NVRAM_PXI_RESULT_ILLEGAL_PARAMETER:
            result = NVRAM_RESULT_ILLEGAL_PARAMETER;
            break;
        case NVRAM_PXI_RESULT_ILLEGAL_STATUS:
            result = NVRAM_RESULT_ILLEGAL_STATUS;
            break;
        case NVRAM_PXI_RESULT_BUSY:
            result = NVRAM_RESULT_BUSY;
            break;
        }
    }
    NVRAMi_EndCommand(result);
}

NVRAMResult NVRAM_ReadDataBytes(u32 address, u32 size, void *buf) {
    OSMessageQueue queue;
    NVRAMResult result;
    OSMessage message[1];
    OSIntrMode enabled;

    if (OS_GetCurrentThread() == NULL) {
        return NVRAM_RESULT_ILLEGAL_STATUS;
    }
    OS_InitMessageQueue(&queue, message, 1);
    while (TRUE) {
        enabled = OS_DisableInterrupts();
        result = NVRAM_ReadDataBytesAsync(address, size, buf, func_0207997c, &queue);
        if (result == NVRAM_RESULT_BUSY) {
            OS_SleepThread(&sNvramWork.queue);
        } else if (result == NVRAM_RESULT_SEND_ERROR) {
            OS_Sleep(1);
        } else {
            OS_RestoreInterrupts(enabled);
            break;
        }
        OS_RestoreInterrupts(enabled);
    }
    if (result == NVRAM_RESULT_SUCCESS) {
        if (!OS_ReceiveMessage(&queue, (OSMessage *)&result, OS_MESSAGE_BLOCK)) {
            OS_Terminate();
        }
    }
    return result;
}

NVRAMResult NVRAM_WriteDataBytesAsync(u32 address, u32 size, const void *src, NVRAMCallback callback, void *arg) {
    OSIntrMode enabled;

    if (!func_02079930()) {
        return NVRAM_RESULT_ILLEGAL_STATUS;
    }
    enabled = OS_DisableInterrupts();
    if (sNvramWork.command != NVRAM_COMMAND_NONE) {
        OS_RestoreInterrupts(enabled);
        return NVRAM_RESULT_BUSY;
    }
    sNvramWork.command = NVRAM_COMMAND_WRITE;
    OS_RestoreInterrupts(enabled);
    sNvramWork.callback = callback;
    sNvramWork.callbackArg = arg;
    sNvramWork.address = address;
    sNvramWork.size = size;
    sNvramWork.buf = (u8 *)src;
    sNvramWork.phase = NVRAM_WRITE_ENABLE_SENT;
    enabled = OS_DisableInterrupts();
    if (func_020799c8() == TRUE) {
        OS_RestoreInterrupts(enabled);
        return NVRAM_RESULT_SUCCESS;
    }
    sNvramWork.command = NVRAM_COMMAND_NONE;
    OS_RestoreInterrupts(enabled);
    OS_WakeupThread(&sNvramWork.queue);
    return NVRAM_RESULT_SEND_ERROR;
}

// The answer to a write, or the alarm that stands for one
static void func_02079458(u32 data, BOOL err) {
    NVRAMResult result = NVRAM_RESULT_FATAL_ERROR;
    u16 status = data & NVRAM_PXI_RESULT_MASK;
    u16 room = NVRAM_PAGE_SIZE - (u8)sNvramWork.address;
    u32 size = sNvramWork.size;
    u16 chunk;
    u8 flash;

    chunk = room > size ? size : room;
    switch (sNvramWork.phase) {
    case NVRAM_WRITE_ENABLE_SENT:
        if (err == FALSE && status == NVRAM_PXI_RESULT_SUCCESS) {
            if (func_02079b08(sNvramWork.address, chunk, sNvramWork.buf) == TRUE) {
                sNvramWork.phase = NVRAM_WRITE_PAGE_SENT;
                return;
            }
            result = NVRAM_RESULT_SEND_ERROR;
        } else if (err == TRUE) {
            result = NVRAM_RESULT_SEND_ERROR;
        } else if (status == NVRAM_PXI_RESULT_BUSY) {
            result = NVRAM_RESULT_BUSY;
        }
        break;
    case NVRAM_WRITE_PAGE_SENT:
        if (err == FALSE && status == NVRAM_PXI_RESULT_SUCCESS) {
            sNvramWork.address += chunk;
            sNvramWork.size -= chunk;
            sNvramWork.buf += chunk;
            sNvramWork.pageTick = OS_GetTick();
            sNvramWork.phase = NVRAM_WRITE_POLL_WAIT;
            NVRAMi_SetAlarm(NVRAM_PXI_RESULT_SUCCESS);
            return;
        } else if (err == TRUE) {
            result = NVRAM_RESULT_SEND_ERROR;
        } else {
            switch (status) {
            case NVRAM_PXI_RESULT_ILLEGAL_PARAMETER:
                result = NVRAM_RESULT_ILLEGAL_PARAMETER;
                break;
            case NVRAM_PXI_RESULT_ILLEGAL_STATUS:
                result = NVRAM_RESULT_ILLEGAL_STATUS;
                break;
            case NVRAM_PXI_RESULT_BUSY:
                if (func_02079b08(sNvramWork.address, chunk, sNvramWork.buf) == TRUE) {
                    return;
                }
                result = NVRAM_RESULT_SEND_ERROR;
                break;
            }
        }
        break;
    case NVRAM_WRITE_STATUS_SENT:
        if (err == FALSE && status == NVRAM_PXI_RESULT_SUCCESS) {
            flash = sNvramBuffer[0];
            if (flash & NVRAM_STATUS_ERROR) {
                if (func_02079b94() == TRUE) {
                    sNvramWork.phase = NVRAM_WRITE_CLEAR_ERROR_SENT;
                    return;
                }
                sNvramWork.phase = NVRAM_WRITE_CLEAR_ERROR_SENT;
                NVRAMi_SetAlarm(NVRAM_PXI_RESULT_BUSY);
                return;
            }
            if (!(flash & NVRAM_STATUS_WRITE_IN_PROGRESS)) {
                if (chunk != 0) {
                    if (func_020799c8() == TRUE) {
                        sNvramWork.phase = NVRAM_WRITE_ENABLE_SENT;
                        return;
                    }
                    result = NVRAM_RESULT_SEND_ERROR;
                    break;
                }
                if (func_020799e4() == TRUE) {
                    sNvramWork.phase = NVRAM_WRITE_DISABLE_SENT;
                    return;
                }
                sNvramWork.phase = NVRAM_WRITE_DISABLE_SENT;
                NVRAMi_SetAlarm(NVRAM_PXI_RESULT_BUSY);
                return;
            }
        } else if (err == FALSE && status != NVRAM_PXI_RESULT_BUSY) {
            break;
        }
        sNvramWork.phase = NVRAM_WRITE_POLL_WAIT;
        NVRAMi_SetAlarm(NVRAM_PXI_RESULT_SUCCESS);
        return;
    case NVRAM_WRITE_DISABLE_SENT:
        if (err == FALSE && status == NVRAM_PXI_RESULT_SUCCESS) {
            result = NVRAM_RESULT_SUCCESS;
            break;
        }
        if (err == FALSE && status != NVRAM_PXI_RESULT_BUSY) {
            break;
        }
        if (func_020799e4() == TRUE) {
            return;
        }
        sNvramWork.phase = NVRAM_WRITE_DISABLE_SENT;
        NVRAMi_SetAlarm(NVRAM_PXI_RESULT_BUSY);
        return;
    case NVRAM_WRITE_CLEAR_ERROR_SENT:
        if (err == FALSE && status == NVRAM_PXI_RESULT_SUCCESS) {
            result = NVRAM_RESULT_WRITE_ERROR;
            break;
        }
        if (err == FALSE && status != NVRAM_PXI_RESULT_BUSY) {
            break;
        }
        if (func_02079b94() == TRUE) {
            return;
        }
        sNvramWork.phase = NVRAM_WRITE_CLEAR_ERROR_SENT;
        NVRAMi_SetAlarm(NVRAM_PXI_RESULT_BUSY);
        return;
    case NVRAM_WRITE_POLL_WAIT:
        DC_InvalidateRange(sNvramBuffer, NVRAM_BUFFER_SIZE);
        if (func_02079a00(sNvramBuffer) == TRUE) {
            sNvramWork.phase = NVRAM_WRITE_STATUS_SENT;
            return;
        }
        sNvramWork.phase = NVRAM_WRITE_STATUS_SENT;
        sNvramBuffer[0] = NVRAM_STATUS_WRITE_IN_PROGRESS;
        DC_StoreRange(sNvramBuffer, NVRAM_BUFFER_SIZE);
        NVRAMi_SetAlarm(NVRAM_PXI_RESULT_SUCCESS);
        return;
    }
    NVRAMi_EndCommand(result);
}

NVRAMResult NVRAM_WriteDataBytes(u32 address, u32 size, const void *src) {
    OSMessageQueue queue;
    NVRAMResult result;
    OSMessage message[1];
    OSIntrMode enabled;

    if (OS_GetCurrentThread() == NULL) {
        return NVRAM_RESULT_ILLEGAL_STATUS;
    }
    OS_InitMessageQueue(&queue, message, 1);
    while (TRUE) {
        enabled = OS_DisableInterrupts();
        result = NVRAM_WriteDataBytesAsync(address, size, src, func_0207997c, &queue);
        if (result == NVRAM_RESULT_BUSY) {
            OS_SleepThread(&sNvramWork.queue);
        } else if (result == NVRAM_RESULT_SEND_ERROR) {
            OS_Sleep(1);
        } else {
            OS_RestoreInterrupts(enabled);
            break;
        }
        OS_RestoreInterrupts(enabled);
    }
    if (result == NVRAM_RESULT_SUCCESS) {
        if (!OS_ReceiveMessage(&queue, (OSMessage *)&result, OS_MESSAGE_BLOCK)) {
            OS_Terminate();
        }
    }
    return result;
}

NVRAMResult NVRAM_VerifyDataBytesAsync(u32 address, u32 size, const void *src, NVRAMCallback callback, void *arg) {
    OSIntrMode enabled;
    u32 chunk;

    if (!func_02079930()) {
        return NVRAM_RESULT_ILLEGAL_STATUS;
    }
    enabled = OS_DisableInterrupts();
    if (sNvramWork.command != NVRAM_COMMAND_NONE) {
        OS_RestoreInterrupts(enabled);
        return NVRAM_RESULT_BUSY;
    }
    sNvramWork.command = NVRAM_COMMAND_VERIFY;
    OS_RestoreInterrupts(enabled);
    sNvramWork.callback = callback;
    sNvramWork.callbackArg = arg;
    sNvramWork.address = address;
    sNvramWork.size = size;
    sNvramWork.buf = (u8 *)src;
    sNvramWork.phase = 0;
    chunk = NVRAM_BUFFER_SIZE;
    DC_InvalidateRange(sNvramBuffer, NVRAM_BUFFER_SIZE);
    enabled = OS_DisableInterrupts();
    if (size <= NVRAM_BUFFER_SIZE) {
        chunk = size;
    }
    if (func_02079a5c(address, chunk, sNvramBuffer) == TRUE) {
        OS_RestoreInterrupts(enabled);
        return NVRAM_RESULT_SUCCESS;
    }
    sNvramWork.command = NVRAM_COMMAND_NONE;
    OS_RestoreInterrupts(enabled);
    OS_WakeupThread(&sNvramWork.queue);
    return NVRAM_RESULT_SEND_ERROR;
}

// The answer to a verify, which compares what the ARM7 read into the buffer with the data and reads the next 32 bytes
static void func_020797d4(u32 data, BOOL err) {
    NVRAMResult result = NVRAM_RESULT_FATAL_ERROR;
    u16 status = data & NVRAM_PXI_RESULT_MASK;
    u32 remaining = sNvramWork.size;
    u32 next;
    u16 chunk;

    if (remaining > NVRAM_BUFFER_SIZE) {
        remaining = NVRAM_BUFFER_SIZE;
    }
    chunk = remaining;
    if (err == FALSE && status == NVRAM_PXI_RESULT_SUCCESS) {
        if (!func_020799a4(sNvramBuffer, sNvramWork.buf, chunk)) {
            result = NVRAM_RESULT_VERIFY_ERROR;
        } else {
            sNvramWork.address += chunk;
            sNvramWork.size -= chunk;
            sNvramWork.buf += chunk;
            if (sNvramWork.size != 0) {
                DC_InvalidateRange(sNvramBuffer, NVRAM_BUFFER_SIZE);
                next = NVRAM_BUFFER_SIZE;
                if (sNvramWork.size <= NVRAM_BUFFER_SIZE) {
                    next = sNvramWork.size;
                }
                if (func_02079a5c(sNvramWork.address, next, sNvramBuffer) == TRUE) {
                    return;
                }
                result = NVRAM_RESULT_SEND_ERROR;
            } else {
                result = NVRAM_RESULT_SUCCESS;
            }
        }
    } else if (err == TRUE) {
        result = NVRAM_RESULT_SEND_ERROR;
    } else {
        switch (status) {
        case NVRAM_PXI_RESULT_ILLEGAL_PARAMETER:
            result = NVRAM_RESULT_ILLEGAL_PARAMETER;
            break;
        case NVRAM_PXI_RESULT_ILLEGAL_STATUS:
            result = NVRAM_RESULT_ILLEGAL_STATUS;
            break;
        case NVRAM_PXI_RESULT_BUSY:
            DC_InvalidateRange(sNvramBuffer, NVRAM_BUFFER_SIZE);
            if (func_02079a5c(sNvramWork.address, chunk, sNvramBuffer) == TRUE) {
                return;
            }
            result = NVRAM_RESULT_SEND_ERROR;
            break;
        }
    }
    NVRAMi_EndCommand(result);
}

NVRAMResult NVRAM_VerifyDataBytes(u32 address, u32 size, const void *src) {
    OSMessageQueue queue;
    NVRAMResult result;
    OSMessage message[1];
    OSIntrMode enabled;

    if (OS_GetCurrentThread() == NULL) {
        return NVRAM_RESULT_ILLEGAL_STATUS;
    }
    OS_InitMessageQueue(&queue, message, 1);
    while (TRUE) {
        enabled = OS_DisableInterrupts();
        result = NVRAM_VerifyDataBytesAsync(address, size, src, func_0207997c, &queue);
        if (result == NVRAM_RESULT_BUSY) {
            OS_SleepThread(&sNvramWork.queue);
        } else if (result == NVRAM_RESULT_SEND_ERROR) {
            OS_Sleep(1);
        } else {
            OS_RestoreInterrupts(enabled);
            break;
        }
        OS_RestoreInterrupts(enabled);
    }
    if (result == NVRAM_RESULT_SUCCESS) {
        if (!OS_ReceiveMessage(&queue, (OSMessage *)&result, OS_MESSAGE_BLOCK)) {
            OS_Terminate();
        }
    }
    return result;
}

// Whether NVRAM_Init has finished
static BOOL func_02079930(void) {
    if (sNvramInitState == NVRAM_INIT_DONE) {
        return TRUE;
    }
    return FALSE;
}

// Gets the ARM7's words with the NVRAM's tag, and hands them to the function of the call that runs
static void func_02079944(PXIFifoTag tag, u32 data, BOOL err) {
    switch (sNvramWork.command) {
    case NVRAM_COMMAND_READ:
        func_020792f4(data, err);
        break;
    case NVRAM_COMMAND_WRITE:
        func_02079458(data, err);
        break;
    case NVRAM_COMMAND_VERIFY:
        func_020797d4(data, err);
        break;
    }
}

// The callback of the synchronous calls, which sends the result to the queue that they wait on
static void func_0207997c(NVRAMResult result, void *arg) {
    if (!OS_SendMessage((OSMessageQueue *)arg, (OSMessage)result, OS_MESSAGE_NOBLOCK)) {
        OS_Terminate();
    }
}

// The alarm's function, which stands in for an answer of the ARM7
static void func_02079994(void *arg) {
    func_02079944(PXI_FIFO_TAG_NVRAM, (u32)arg, FALSE);
}

// Whether the two ranges are the same
static BOOL func_020799a4(const u8 *a, const u8 *b, u32 size) {
    u32 i;

    for (i = 0; i < size; i++) {
        if (a[i] != b[i]) {
            return FALSE;
        }
    }
    return TRUE;
}

// Sends the command that enables writing
static BOOL func_020799c8(void) {
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM,
                           NVRAM_PXI_START | NVRAM_PXI_END | NVRAM_PXI_COMMAND(NVRAM_PXI_COMMAND_WRITE_ENABLE),
                           FALSE) < 0) {
        return FALSE;
    }
    return TRUE;
}

// Sends the command that disables writing
static BOOL func_020799e4(void) {
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM,
                           NVRAM_PXI_START | NVRAM_PXI_END | NVRAM_PXI_COMMAND(NVRAM_PXI_COMMAND_WRITE_DISABLE),
                           FALSE) < 0) {
        return FALSE;
    }
    return TRUE;
}

// Sends the command that reads the status register to the address of status, which is the byte that the ARM7 writes
static BOOL func_02079a00(u8 *status) {
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM,
                           NVRAM_PXI_START | NVRAM_PXI_COMMAND(NVRAM_PXI_COMMAND_READ_STATUS) | ((u32)status >> 24),
                           FALSE) < 0) {
        return FALSE;
    }
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM, NVRAM_PXI_INDEX(1) | (((u32)status >> 8) & 0xffff), FALSE) < 0) {
        return FALSE;
    }
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM, NVRAM_PXI_END | NVRAM_PXI_INDEX(2) | (((u32)status << 8) & 0xff00),
                           FALSE) < 0) {
        return FALSE;
    }
    return TRUE;
}

// Sends the command that reads size bytes of the NVRAM from address to buf
static BOOL func_02079a5c(u32 address, u32 size, void *buf) {
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM,
                           NVRAM_PXI_START | NVRAM_PXI_COMMAND(NVRAM_PXI_COMMAND_READ) | ((address >> 16) & 0xff),
                           FALSE) < 0) {
        return FALSE;
    }
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM, NVRAM_PXI_INDEX(1) | (address & 0xffff), FALSE) < 0) {
        return FALSE;
    }
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM, NVRAM_PXI_INDEX(2) | (size >> 16), FALSE) < 0) {
        return FALSE;
    }
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM, NVRAM_PXI_INDEX(3) | (size & 0xffff), FALSE) < 0) {
        return FALSE;
    }
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM, NVRAM_PXI_INDEX(4) | ((u32)buf >> 16), FALSE) < 0) {
        return FALSE;
    }
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM, NVRAM_PXI_END | NVRAM_PXI_INDEX(5) | ((u32)buf & 0xffff), FALSE) < 0) {
        return FALSE;
    }
    return TRUE;
}

// Sends the command that writes size bytes from src to the page of the NVRAM at address
static BOOL func_02079b08(u32 address, u16 size, const void *src) {
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM,
                           NVRAM_PXI_START | NVRAM_PXI_COMMAND(NVRAM_PXI_COMMAND_PAGE_WRITE) | ((address >> 16) & 0xff),
                           FALSE) < 0) {
        return FALSE;
    }
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM, NVRAM_PXI_INDEX(1) | (address & 0xffff), FALSE) < 0) {
        return FALSE;
    }
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM, NVRAM_PXI_INDEX(2) | size, FALSE) < 0) {
        return FALSE;
    }
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM, NVRAM_PXI_INDEX(3) | ((u32)src >> 16), FALSE) < 0) {
        return FALSE;
    }
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM, NVRAM_PXI_END | NVRAM_PXI_INDEX(4) | ((u32)src & 0xffff), FALSE) < 0) {
        return FALSE;
    }
    return TRUE;
}

// Sends the command that clears the flash's error
static BOOL func_02079b94(void) {
    if (PXI_SendWordByFifo(PXI_FIFO_TAG_NVRAM,
                           NVRAM_PXI_START | NVRAM_PXI_END | NVRAM_PXI_COMMAND(NVRAM_PXI_COMMAND_CLEAR_ERROR),
                           FALSE) < 0) {
        return FALSE;
    }
    return TRUE;
}
