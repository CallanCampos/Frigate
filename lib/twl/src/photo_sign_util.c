#include "types.h"
#include "nitro/os.h"
#include "nitro/pxi.h"
#include "photo_sign_private.h"

// What the photo signature's other files use: sending words to the ARM7 over the FIFO, the lock of its commands, and
// the cache maintenance of the buffers it reads and writes. ARM code in the DSi-only LTD autoload. The ROM names none
// of it; the file's name and those of its functions and data are descriptive guesses, and so is the boundary: its
// .bss is reached from its own base, so the lock was a file of its own, but whether the FIFO and cache helpers were
// in the same file the code doesn't show.

// Below these sizes the caches are flushed or invalidated by range, above them whole
#define DC_RANGE_MAX 0x2400
#define IC_RANGE_MAX 0x400
#define CACHE_LINE_MASK 0x1f

// A lock that the thread holding it may take again. The ARM7's result releases it in the FIFO's interrupt, so the
// owner is volatile: PhotoSign_TryLock reads it again after testing it
typedef struct {
    OSThread *volatile owner;
    s32 count;
    OSThreadQueue queue;
} PhotoSignLock;

static void PhotoSign_SendWord(u32 fifoTag, u32 data);
static void PhotoSign_SendData(u32 fifoTag, const u8 *data, u32 size);

static PhotoSignLock sLock;

void PhotoSign_SendHeader(u32 fifoTag, u16 command, u16 data) {
    while (PXI_SendWordByFifo(fifoTag,
                              ((command << PHOTO_SIGN_FIFO_COMMAND_SHIFT) & PHOTO_SIGN_FIFO_COMMAND_MASK) |
                                  PHOTO_SIGN_FIFO_START_BIT | (data & PHOTO_SIGN_FIFO_DATA_MASK),
                              FALSE) < 0) {
    }
}

// Sends a word of payload
static void PhotoSign_SendWord(u32 fifoTag, u32 data) {
    while (PXI_SendWordByFifo(fifoTag, data & PHOTO_SIGN_FIFO_PAYLOAD_MASK, FALSE) < 0) {
    }
}

// Sends data 3 bytes to a word, the first in the high byte
static void PhotoSign_SendData(u32 fifoTag, const u8 *data, u32 size) {
    unsigned int i;

    for (i = 0; size - i >= 3; i += 3) {
        PhotoSign_SendWord(fifoTag, (data[i] << 16) | (data[i + 1] << 8) | data[i + 2]);
    }
    if (size - i == 2) {
        PhotoSign_SendWord(fifoTag, (data[i] << 16) | (data[i + 1] << 8));
    } else if (size - i == 1) {
        PhotoSign_SendWord(fifoTag, data[i] << 16);
    }
}

void PhotoSign_SendCommand(u32 command, const void *data, u32 size) {
    PhotoSign_SendHeader(PHOTO_SIGN_FIFO_TAG, command, 0);
    PhotoSign_SendData(PHOTO_SIGN_FIFO_TAG, data, size);
}

void PhotoSign_SendCommandData(u32 command, u32 data) {
    PhotoSign_SendHeader(PHOTO_SIGN_FIFO_TAG, command, data);
}

void PhotoSign_InitLock(void) {
    OS_InitThreadQueue(&sLock.queue);
    sLock.owner = NULL;
    sLock.count = 0;
}

BOOL PhotoSign_TryLock(void) {
    OSIntrMode enabled = OS_DisableInterrupts();
    OSThread *current = OSi_ThreadInfo.current;
    BOOL result;

    if (sLock.owner == NULL) {
        sLock.owner = current;
        sLock.count = 1;
        result = TRUE;
    } else if (sLock.owner == current) {
        sLock.count++;
        result = TRUE;
    } else {
        result = FALSE;
    }
    OS_RestoreInterrupts(enabled);
    return result;
}

void PhotoSign_Lock(void) {
    OSIntrMode enabled = OS_DisableInterrupts();
    OSThread *current = OSi_ThreadInfo.current;

    while (TRUE) {
        OSThread *owner = sLock.owner;

        if (owner == NULL) {
            sLock.owner = current;
            sLock.count = 1;
            break;
        } else if (owner == current) {
            sLock.count++;
            break;
        }
        OS_SleepThread(&sLock.queue);
    }
    OS_RestoreInterrupts(enabled);
}

void PhotoSign_Unlock(void) {
    OSIntrMode enabled = OS_DisableInterrupts();

    if (sLock.owner != NULL && --sLock.count == 0) {
        sLock.owner = NULL;
        OS_WakeupThread(&sLock.queue);
    }
    OS_RestoreInterrupts(enabled);
}

void PhotoSign_FlushCache(const void *addr, u32 size) {
    if (size < DC_RANGE_MAX) {
        DC_FlushRange(addr, size);
    } else {
        DC_FlushAll();
    }
}

// Writes back the lines the buffer shares at its ends before discarding it
void PhotoSign_InvalidateCache(void *addr, u32 size) {
    u32 end;

    if (size < IC_RANGE_MAX) {
        IC_InvalidateRange(addr, size);
    } else {
        IC_InvalidateAll();
    }
    if (size < DC_RANGE_MAX) {
        end = (u32)addr + size;
        if ((u32)addr & CACHE_LINE_MASK) {
            DC_StoreRange(addr, 1);
        }
        if (end & CACHE_LINE_MASK) {
            DC_StoreRange((void *)(end - 1), 1);
        }
        DC_InvalidateRange(addr, size);
    } else {
        DC_FlushAll();
    }
}
