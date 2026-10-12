#include "types.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "photo_sign_private.h"

// The ARM9's side of the ARM7's photo signature commands: it sets up the FIFO tag, sends the commands, and takes the
// ARM7's results, which a thread waits for or a callback gets. ARM code in the DSi-only LTD autoload. The ROM names
// none of it; the file's name and those of its functions and data are descriptive guesses. Its .bss is reached from
// its own base, so it was a file of its own, built with -ipa file like the library's other files.

// The MAC length the ARM7 is given, (PHOTO_SIGN_MAC_SIZE - 2) / 2
#define MAC_LENGTH_CODE 7

// The MAC command's parameters, which are sent to the ARM7 as they are
typedef struct {
    u8 nonce[PHOTO_SIGN_NONCE_SIZE];
    u32 unk_0c;
    const void *data;
    u32 dataSize;
    u32 payloadSize;
    u32 macLength;
    u8 *mac;
} MacParam;

static void PhotoSign_FifoCallback(u32 fifoTag, u32 data, BOOL err);
static int PhotoSign_WaitResult(void);

static OSMessage sMessages[1];
static OSMessageQueue sQueue;
static void *sCallbackArg;
static PhotoSignCallback sCallback;
static BOOL sInitialized;

// Takes the words the ARM7 sends: a MAC's result goes to the callback and ends the command, another result to the
// thread that waits for it
static void PhotoSign_FifoCallback(u32 fifoTag, u32 data, BOOL err) {
    u32 command;

    if (err) {
        return;
    }
    command = (data & PHOTO_SIGN_FIFO_COMMAND_MASK) >> PHOTO_SIGN_FIFO_COMMAND_SHIFT;
    data &= PHOTO_SIGN_FIFO_DATA_MASK;
    switch (command) {
    case PHOTO_SIGN_COMMAND_MAC_RESULT:
        if (sCallback != NULL) {
            sCallback(data, sCallbackArg);
        }
        PhotoSign_Unlock();
        break;
    case PHOTO_SIGN_COMMAND_RESULT:
        OS_SendMessage(&sQueue, (OSMessage)data, OS_MESSAGE_NOBLOCK);
        break;
    default:
        PhotoSign_SendCommandData(PHOTO_SIGN_COMMAND_RESULT, 6);
        break;
    }
}

// Waits for the ARM7's result of a command
static int PhotoSign_WaitResult(void) {
    OSMessage result;

    OS_ReceiveMessage(&sQueue, &result, OS_MESSAGE_BLOCK);
    return (int)result;
}

void PhotoSign_Init(void) {
    if (sInitialized) {
        return;
    }
    func_0207acc4(&sQueue, sMessages, NELEMS(sMessages));
    PhotoSign_InitLock();
    PXI_Init();
    func_0207c9fc(PHOTO_SIGN_FIFO_TAG, PhotoSign_FifoCallback);
    while (!func_0207ca48(PHOTO_SIGN_FIFO_TAG, MI_PROCESSOR_ARM7)) {
    }
    sInitialized = TRUE;
}

int PhotoSign_SeedRandom(const void *seed) {
    int result;

    if (!PhotoSign_TryLock()) {
        return PHOTO_SIGN_RESULT_BUSY;
    }
    PhotoSign_SendCommand(PHOTO_SIGN_COMMAND_SEED, seed, MATH_SHA1_DIGEST_SIZE);
    result = PhotoSign_WaitResult();
    PhotoSign_Unlock();
    return result;
}

// The command carries the buffer's address, which the ARM7 writes to
int PhotoSign_GetRandomBlock(void *buffer) {
    int result;

    if (!PhotoSign_TryLock()) {
        return PHOTO_SIGN_RESULT_BUSY;
    }
    PhotoSign_InvalidateCache(buffer, PHOTO_SIGN_RANDOM_BLOCK_SIZE);
    PhotoSign_SendCommand(PHOTO_SIGN_COMMAND_RANDOM, &buffer, sizeof(buffer));
    result = PhotoSign_WaitResult();
    PhotoSign_Unlock();
    return result;
}

// The lock taken here is released when the ARM7's result comes
int PhotoSign_StartMac(const u8 *nonce, const void *data, u32 size, u8 *mac, PhotoSignCallback callback, void *arg) {
    MacParam param;

    if (!PhotoSign_TryLock()) {
        return PHOTO_SIGN_RESULT_BUSY;
    }
    sCallback = callback;
    sCallbackArg = arg;
    sys_memcpy(nonce, param.nonce, PHOTO_SIGN_NONCE_SIZE);
    param.data = data;
    param.dataSize = size;
    param.payloadSize = 0;
    param.macLength = MAC_LENGTH_CODE;
    param.mac = mac;
    PhotoSign_FlushCache(data, size);
    PhotoSign_InvalidateCache(mac, PHOTO_SIGN_MAC_SIZE);
    PhotoSign_SendCommand(PHOTO_SIGN_COMMAND_MAC, &param, sizeof(param));
    return PHOTO_SIGN_RESULT_SUCCESS;
}
