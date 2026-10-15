#include "twl/photo_sign.h"
#include "types.h"
#include "nitro/math.h"
#include "nitro/mi.h"
#include "nitro/os.h"
#include "photo_sign_private.h"

// The DSi's photo signature, which the SSP JPEG encoder and decoder of overlay 257 call: a signature is a random nonce
// and the MAC that the ARM7 computes over the photo with it (photo_sign_client.c). The nonces are drawn from random
// numbers that the ARM7 supplies, after it is seeded once from the console's entropy. ARM code in the DSi-only LTD
// autoload. The ROM names none of it; the file's name and those of its functions and data are descriptive guesses.
// Its .bss is reached from its own base, so it was a file of its own, built with -ipa file like the library's others.

static void PhotoSign_SignalEvent(u32 result, void *arg);
static void PhotoSign_InitRandom(void);
static int PhotoSign_GetRandom(u8 *dst, u32 size);

// The block of random bytes from the ARM7, which writes it, so it has a cache line of its own: aligned to it and as
// large as it (32 bytes, of which a block uses 16), which puts photo_sign_util.c's .bss 16 bytes further on
static u8 sRandomBuffer[32] ATTRIBUTE_ALIGN(32);
static BOOL sRandomInitialized;
static BOOL sRandomUnk04;
// How many bytes of the block are left, at its end
static u32 sRandomRemaining;
static u8 sMac[PHOTO_SIGN_MAC_SIZE];

// The MAC's callback: wakes the signing thread with the result
static void PhotoSign_SignalEvent(u32 result, void *arg) {
    func_0207c864(arg, result);
}

// Seeds the ARM7's random numbers once with the digest of the console's entropy, then overwrites the entropy and the
// digest with random numbers of their own
static void PhotoSign_InitRandom(void) {
    MATHRandContext32 rand;
    u32 entropy[8];
    u32 digest[MATH_SHA1_DIGEST_SIZE / sizeof(u32)];
    u32 i;

    clock_init();
    sRandomRemaining = 0;
    sRandomInitialized = TRUE;
    MATH_InitRand32(&rand, ((u64)(u32)&rand << 32) | (u32)&rand);
    OS_GetLowEntropyData(entropy);
    MATH_CalcSHA1((u8 *)digest, entropy, sizeof(entropy));
    for (i = 0; i < NELEMS(entropy); i++) {
        entropy[i] = MATH_Rand32(&rand, 0);
    }
    PhotoSign_SeedRandom(digest);
    for (i = 0; i < NELEMS(digest); i++) {
        digest[i] = MATH_Rand32(&rand, 0);
    }
}

// Fills dst with random bytes, using up the rest of the last block from the ARM7 first
static int PhotoSign_GetRandom(u8 *dst, u32 size) {
    u32 count;
    int result;

    if (!sRandomInitialized) {
        PhotoSign_InitRandom();
    }
    if (sRandomRemaining != 0) {
        count = sRandomRemaining;
        if (count > size) {
            count = size;
        }
        sRandomRemaining -= count;
        sys_memcpy(sRandomBuffer + sRandomRemaining, dst, count);
        size -= count;
        dst += count;
    }
    while (size >= PHOTO_SIGN_RANDOM_BLOCK_SIZE) {
        result = PhotoSign_GetRandomBlock(sRandomBuffer);
        if (result != PHOTO_SIGN_RESULT_SUCCESS) {
            return result;
        }
        sys_memcpy(sRandomBuffer, dst, PHOTO_SIGN_RANDOM_BLOCK_SIZE);
        dst += PHOTO_SIGN_RANDOM_BLOCK_SIZE;
        size -= PHOTO_SIGN_RANDOM_BLOCK_SIZE;
    }
    if (size != 0) {
        result = PhotoSign_GetRandomBlock(sRandomBuffer);
        if (result != PHOTO_SIGN_RESULT_SUCCESS) {
            return result;
        }
        sRandomRemaining = PHOTO_SIGN_RANDOM_BLOCK_SIZE - size;
        sys_memcpy(sRandomBuffer + sRandomRemaining, dst, size);
    }
    return PHOTO_SIGN_RESULT_SUCCESS;
}

int PhotoSign_Sign(u8 *sig, const u8 *data, u32 size) {
    OSEvent event;
    int result;

    PhotoSign_Lock();
    if (!sRandomInitialized) {
        PhotoSign_InitRandom();
    }
    func_0207c7f4(&event);
    result = PhotoSign_GetRandom(sig, PHOTO_SIGN_NONCE_SIZE);
    if (result != PHOTO_SIGN_RESULT_SUCCESS) {
        // BUG: The lock stays taken, so other threads wait for it forever
#ifdef BUGFIX
        PhotoSign_Unlock();
#endif
        return result;
    }
    do {
        if (!sRandomUnk04) {
            PhotoSign_SendHeader(PHOTO_SIGN_FIFO_TAG, PHOTO_SIGN_COMMAND_UNK_8, 1);
        }
        result = PhotoSign_StartMac(sig, data, size, sMac, PhotoSign_SignalEvent, &event);
        if (result == PHOTO_SIGN_RESULT_SUCCESS) {
            result = func_0207c800(&event, 0xffffffff, OS_EVENT_MODE_OR, 0xffffffff);
            if (result == PHOTO_SIGN_RESULT_SUCCESS) {
                sys_memcpy(sMac, sig + PHOTO_SIGN_NONCE_SIZE, PHOTO_SIGN_MAC_SIZE);
            }
        }
    } while (result == PHOTO_SIGN_RESULT_BUSY);
    if (!sRandomUnk04) {
        PhotoSign_SendHeader(PHOTO_SIGN_FIFO_TAG, PHOTO_SIGN_COMMAND_UNK_8, 0);
    }
    PhotoSign_Unlock();
    return result;
}

int PhotoSign_Verify(const u8 *sig, const u8 *data, u32 size) {
    OSEvent event;
    int result;

    PhotoSign_Lock();
    if (!sRandomInitialized) {
        PhotoSign_InitRandom();
    }
    func_0207c7f4(&event);
    do {
        if (!sRandomUnk04) {
            PhotoSign_SendHeader(PHOTO_SIGN_FIFO_TAG, PHOTO_SIGN_COMMAND_UNK_8, 1);
        }
        result = PhotoSign_StartMac(sig, data, size, sMac, PhotoSign_SignalEvent, &event);
        if (result == PHOTO_SIGN_RESULT_SUCCESS) {
            result = func_0207c800(&event, 0xffffffff, OS_EVENT_MODE_OR, 0xffffffff);
            if (result == PHOTO_SIGN_RESULT_SUCCESS &&
                sys_memcmp(sMac, sig + PHOTO_SIGN_NONCE_SIZE, PHOTO_SIGN_MAC_SIZE) != 0) {
                result = PHOTO_SIGN_RESULT_MISMATCH;
            }
        }
    } while (result == PHOTO_SIGN_RESULT_BUSY);
    if (!sRandomUnk04) {
        PhotoSign_SendHeader(PHOTO_SIGN_FIFO_TAG, PHOTO_SIGN_COMMAND_UNK_8, 0);
    }
    PhotoSign_Unlock();
    return result;
}
