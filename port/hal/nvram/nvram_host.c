// NitroSDK's NVRAM library on the host: the decomp's lib/nitro/src/nvram/nvram.c, compiled unchanged through this file
// (port/hal/nvram/replaced.txt keeps it out of the game's own sources), with the six functions that take a buffer
// renamed out of the way and wrapped, for two things a 64-bit host does differently:
//   - the commands carry the ARM9's buffers as 32-bit values, which hold only the low half of a host pointer, so each
//     wrapper notes the caller's buffer and the library's own (sNvramBuffer, where the flash's status and the verify's
//     reads go) for the host's ARM7 (port_pxi_note_pointer, nvram7.c);
//   - the synchronous functions receive the asynchronous call's result as a message into an NVRAMResult
//     (OS_ReceiveMessage(&queue, (OSMessage *)&result, ...)): a message is a pointer, 8 bytes on a host, so the
//     decomp's write would overrun the 4-byte result. Their host versions here are the decomp's, with the message
//     received whole and then turned into the result.
// The asynchronous wrappers call the decomp's functions. This file reaches two of nvram.c's statics, whose names a
// rename upstream would change: sNvramBuffer, to note it, and sNvramWork's thread queue, which the synchronous
// functions sleep on while another call runs. The callback that ends their asynchronous call is this file's own
// (SendResult): nvram.c's is static, and its name is still a placeholder.
#define NVRAM_ReadDataBytesAsync NVRAMi_DsReadDataBytesAsync
#define NVRAM_ReadDataBytes NVRAMi_DsReadDataBytes
#define NVRAM_WriteDataBytesAsync NVRAMi_DsWriteDataBytesAsync
#define NVRAM_WriteDataBytes NVRAMi_DsWriteDataBytes
#define NVRAM_VerifyDataBytesAsync NVRAMi_DsVerifyDataBytesAsync
#define NVRAM_VerifyDataBytes NVRAMi_DsVerifyDataBytes
#include "lib/nitro/src/nvram/nvram.c"
#undef NVRAM_ReadDataBytesAsync
#undef NVRAM_ReadDataBytes
#undef NVRAM_WriteDataBytesAsync
#undef NVRAM_WriteDataBytes
#undef NVRAM_VerifyDataBytesAsync
#undef NVRAM_VerifyDataBytes

#include "hal/pxi/pxi_bus.h"
#include <stdint.h>

// nvram.h's prototypes were renamed with the functions
NVRAMResult NVRAM_ReadDataBytesAsync(u32 address, u32 size, void *buf, NVRAMCallback callback, void *arg);
NVRAMResult NVRAM_ReadDataBytes(u32 address, u32 size, void *buf);
NVRAMResult NVRAM_WriteDataBytesAsync(u32 address, u32 size, const void *src, NVRAMCallback callback, void *arg);
NVRAMResult NVRAM_WriteDataBytes(u32 address, u32 size, const void *src);
NVRAMResult NVRAM_VerifyDataBytesAsync(u32 address, u32 size, const void *src, NVRAMCallback callback, void *arg);
NVRAMResult NVRAM_VerifyDataBytes(u32 address, u32 size, const void *src);

// The asynchronous calls that the synchronous functions wait for
typedef enum NVRAMHostCall {
    NVRAM_HOST_READ,
    NVRAM_HOST_WRITE,
    NVRAM_HOST_VERIFY,
} NVRAMHostCall;

static void NoteBuffers(const void *buffer, u32 size) {
    port_pxi_note_pointer(PXI_FIFO_TAG_NVRAM, sNvramBuffer, sizeof(sNvramBuffer));
    port_pxi_note_pointer(PXI_FIFO_TAG_NVRAM, buffer, size);
}

// The asynchronous call's callback for the synchronous functions: its result, as a message to the queue they wait on,
// as nvram.c's does
static void SendResult(NVRAMResult result, void *arg) {
    if (!OS_SendMessage((OSMessageQueue *)arg, (OSMessage)(uintptr_t)result, OS_MESSAGE_NOBLOCK)) {
        OS_Terminate();
    }
}

static NVRAMResult StartCall(NVRAMHostCall call, u32 address, u32 size, void *buffer, OSMessageQueue *queue) {
    switch (call) {
    case NVRAM_HOST_READ:
        return NVRAMi_DsReadDataBytesAsync(address, size, buffer, SendResult, queue);
    case NVRAM_HOST_WRITE:
        return NVRAMi_DsWriteDataBytesAsync(address, size, buffer, SendResult, queue);
    default:
        return NVRAMi_DsVerifyDataBytesAsync(address, size, buffer, SendResult, queue);
    }
}

// The decomp's NVRAM_ReadDataBytes, NVRAM_WriteDataBytes and NVRAM_VerifyDataBytes: start the call, sleeping while
// another runs and retrying a moment later when the FIFO refused it, then wait for its result as a message
static NVRAMResult WaitForCall(NVRAMHostCall call, u32 address, u32 size, void *buffer) {
    OSMessageQueue queue;
    NVRAMResult result;
    OSMessage message[1];
    OSMessage answer;
    u32 enabled;

    if (OS_GetCurrentThread() == NULL) {
        return NVRAM_RESULT_ILLEGAL_STATUS;
    }
    OS_InitMessageQueue(&queue, message, 1);
    while (TRUE) {
        enabled = OS_DisableInterrupts();
        result = StartCall(call, address, size, buffer, &queue);
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
        if (!OS_ReceiveMessage(&queue, &answer, OS_MESSAGE_BLOCK)) {
            OS_Terminate();
        }
        result = (NVRAMResult)(uintptr_t)answer;
    }
    return result;
}

NVRAMResult NVRAM_ReadDataBytesAsync(u32 address, u32 size, void *buf, NVRAMCallback callback, void *arg) {
    NoteBuffers(buf, size);
    return NVRAMi_DsReadDataBytesAsync(address, size, buf, callback, arg);
}

NVRAMResult NVRAM_ReadDataBytes(u32 address, u32 size, void *buf) {
    NoteBuffers(buf, size);
    return WaitForCall(NVRAM_HOST_READ, address, size, buf);
}

NVRAMResult NVRAM_WriteDataBytesAsync(u32 address, u32 size, const void *src, NVRAMCallback callback, void *arg) {
    NoteBuffers(src, size);
    return NVRAMi_DsWriteDataBytesAsync(address, size, src, callback, arg);
}

NVRAMResult NVRAM_WriteDataBytes(u32 address, u32 size, const void *src) {
    NoteBuffers(src, size);
    return WaitForCall(NVRAM_HOST_WRITE, address, size, (void *)src);
}

NVRAMResult NVRAM_VerifyDataBytesAsync(u32 address, u32 size, const void *src, NVRAMCallback callback, void *arg) {
    NoteBuffers(src, size);
    return NVRAMi_DsVerifyDataBytesAsync(address, size, src, callback, arg);
}

NVRAMResult NVRAM_VerifyDataBytes(u32 address, u32 size, const void *src) {
    NoteBuffers(src, size);
    return WaitForCall(NVRAM_HOST_VERIFY, address, size, (void *)src);
}
