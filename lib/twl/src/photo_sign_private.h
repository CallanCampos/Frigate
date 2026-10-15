#ifndef POKEBW2_TWL_PHOTO_SIGN_PRIVATE_H
#define POKEBW2_TWL_PHOTO_SIGN_PRIVATE_H

#include "types.h"
#include "twl/photo_sign.h"

// What the photo signature's files share: the FIFO protocol with the ARM7, the ARM7's commands (photo_sign_client.c),
// and the FIFO, lock and cache helpers (photo_sign_util.c)

#define PHOTO_SIGN_FIFO_TAG 19

// A header word: a start bit, 4 bits of command and 16 of data. The words that follow it carry 3 bytes each
#define PHOTO_SIGN_FIFO_START_BIT 0x2000000
#define PHOTO_SIGN_FIFO_COMMAND_SHIFT 21
#define PHOTO_SIGN_FIFO_COMMAND_MASK 0x1e00000
#define PHOTO_SIGN_FIFO_DATA_MASK 0xffff
#define PHOTO_SIGN_FIFO_PAYLOAD_MASK 0xffffff

enum {
    // Seeds the ARM7's random numbers with a digest
    PHOTO_SIGN_COMMAND_SEED = 1,
    // Fills a 16-byte buffer with random numbers
    PHOTO_SIGN_COMMAND_RANDOM = 2,
    // From the ARM7: the result of a MAC, for the callback
    PHOTO_SIGN_COMMAND_MAC_RESULT = 6,
    // From the ARM7: the result of the other commands; to the ARM7: an unknown command
    PHOTO_SIGN_COMMAND_RESULT = 7,
    // Brackets the MACs of a signature, unless sRandomUnk04 is set
    PHOTO_SIGN_COMMAND_UNK_8 = 8,
    // Computes a MAC
    PHOTO_SIGN_COMMAND_MAC = 9,
};

// The random numbers come from the ARM7 16 bytes at a time
#define PHOTO_SIGN_RANDOM_BLOCK_SIZE 16

typedef void (*PhotoSignCallback)(u32 result, void *arg);

// photo_sign_client.c
// Seeds the ARM7's random numbers with a SHA-1 digest
int PhotoSign_SeedRandom(const void *seed);
// Has the ARM7 fill a 16-byte buffer with random bytes
int PhotoSign_GetRandomBlock(void *buffer);
// Starts the MAC of data with a nonce into mac, which ends with the callback
int PhotoSign_StartMac(const u8 *nonce, const void *data, u32 size, u8 *mac, PhotoSignCallback callback, void *arg);

// photo_sign_util.c
// Sends a header word, a command with its parameters, or a command with 16 bits of data
void PhotoSign_SendHeader(u32 fifoTag, u16 command, u16 data);
void PhotoSign_SendCommand(u32 command, const void *data, u32 size);
void PhotoSign_SendCommandData(u32 command, u32 data);
// The lock of the ARM7's commands, which the thread holding it may take again: set up, try, take (sleeping while
// another thread holds it) and release
void PhotoSign_InitLock(void);
BOOL PhotoSign_TryLock(void);
void PhotoSign_Lock(void);
void PhotoSign_Unlock(void);
// Writes data the ARM7 will read back to memory, and discards the cached copy of a buffer the ARM7 will write
void PhotoSign_FlushCache(const void *addr, u32 size);
void PhotoSign_InvalidateCache(void *addr, u32 size);

#endif // POKEBW2_TWL_PHOTO_SIGN_PRIVATE_H
