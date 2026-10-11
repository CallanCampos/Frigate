#ifndef POKEBW2_DWC_CRYPT_H
#define POKEBW2_DWC_CRYPT_H

#include "types.h"

// The signature check that the server's answer to a Pokémon's check goes through, in overlay 189 (ARM code beside
// nhttp_rap.c): an RSA public key with the exponent 65537 and a SHA-1 digest

typedef void *(*CryptAllocFunc)(u32 size);
typedef void (*CryptFreeFunc)(void *ptr);

// Gives the library the allocator that it takes its memory from; the third argument is stored and not used
void Crypt_SetAllocator(CryptAllocFunc alloc, CryptFreeFunc free, u32 arg);
// Digests size bytes and checks that the signature, of 128 bytes, holds the digest when decrypted with the key, whose
// modulus is 128 bytes; TRUE when it does
int Crypt_VerifySignature(const void *data, u32 size, void *signature, const u8 *key);

#endif // POKEBW2_DWC_CRYPT_H
