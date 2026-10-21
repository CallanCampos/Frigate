#ifndef POKEBW2_DWC_SB_CRYPT_H
#define POKEBW2_DWC_SB_CRYPT_H

#include "dwc/nonport.h"

// The stream cipher of GameSpy's server browser (sb_crypt.c), in overlay 11, that the master server encrypts its
// lists with: a permutation of the bytes, which every byte shuffles a little more, and five indexes

typedef struct GOACryptState {
    gsi_u8 cards[256];  // a permutation of 0 to 255
    gsi_u8 rotor;       // moves on by one
    gsi_u8 ratchet;     // moves erratically
    gsi_u8 avalanche;   // depends on the data
    gsi_u8 last_plain;  // the last byte of plain text
    gsi_u8 last_cipher; // the last byte of cipher text
} GOACryptState;

// Sets the cipher up from a key, or as the hash's fixed permutation for a key of no bytes
void GOACryptInit(GOACryptState *state, const gsi_u8 *key, gsi_u8 keysize);
void GOAHashInit(GOACryptState *state);
// Decrypts one byte, and a buffer in place
gsi_u8 GOADecryptByte(GOACryptState *state, gsi_u8 b);
void GOADecrypt(GOACryptState *state, gsi_u8 *bp, int len);

#endif // POKEBW2_DWC_SB_CRYPT_H
