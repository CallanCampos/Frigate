#ifndef POKEBW2_DWC_CRYPT_H
#define POKEBW2_DWC_CRYPT_H

#include "types.h"

// The signing that the server's check of a Pokémon is answered with, in overlay 189 (ARM code beside nhttp_rap.c);
// what these do is read off nhttp_rap_evilcheck.c

typedef void *(*CryptAllocFunc)(u32 size);
typedef void (*CryptFreeFunc)(void *ptr);

// Gives the library the allocator that it takes its memory from; the third argument is stored and not used
void func_ov189_021a9778(CryptAllocFunc alloc, CryptFreeFunc free, u32 arg);
// Digests size bytes and signs the digest with the key into signature, and returns the library's result
int func_ov189_021a96cc(const void *data, u32 size, void *signature, const u8 *key);

#endif // POKEBW2_DWC_CRYPT_H
