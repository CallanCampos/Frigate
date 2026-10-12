#ifndef POKEBW2_TWL_PHOTO_SIGN_H
#define POKEBW2_TWL_PHOTO_SIGN_H

#include "types.h"

// The DSi's photo signature, in the LTD autoload (photo_sign.c), which the SSP JPEG encoder and decoder call. A
// signature is a random 12-byte nonce followed by the 16-byte MAC that the ARM7 computes over the photo with it

#define PHOTO_SIGN_NONCE_SIZE 12
#define PHOTO_SIGN_MAC_SIZE 16

// What the ARM7 and the functions return
enum {
    PHOTO_SIGN_RESULT_SUCCESS = 1,
    PHOTO_SIGN_RESULT_MISMATCH = 2,
    PHOTO_SIGN_RESULT_BUSY = 4,
};

// Sets up the ARM7's side once
void PhotoSign_Init(void);
// Signs data, whose signature block is zeroed: writes a new nonce and the MAC into sig
int PhotoSign_Sign(u8 *sig, const u8 *data, u32 size);
// Computes the MAC of data with sig's nonce and compares it with sig's; PHOTO_SIGN_RESULT_MISMATCH when they differ
int PhotoSign_Verify(const u8 *sig, const u8 *data, u32 size);

#endif // POKEBW2_TWL_PHOTO_SIGN_H
