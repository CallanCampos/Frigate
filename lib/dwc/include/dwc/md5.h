#ifndef POKEBW2_DWC_MD5_H
#define POKEBW2_DWC_MD5_H

#include "types.h"

// GameSpy's MD5 (md5c.c), in overlay 11: RSA's reference MD5 (RFC 1321), with a function that gives the digest of some
// bytes as 32 hex digits. The ROM has no names for the file or its functions: the file is named after GameSpy's, as
// pret/pokerevo's Wii build has it (asm/SDK/DWC/gamespy/md5c.s), and so are the functions. MD5Init, MD5Update and
// MD5Final keep their addresses, with their names in the comments: NitroWiFi's SSL (lib/nitrowifi/src/ssl/md5.c) has
// functions of the same names in this overlay

// The state of a hash (RFC 1321's MD5_CTX): the four words of the state, the count of bits hashed and the bytes of
// the block not hashed yet
typedef struct HashContext {
    u32 state[4];
    u32 count[2];
    u8 buffer[64];
} HashContext;

// MD5Init, MD5Update and MD5Final, which gives the 16 bytes of the digest
void func_ov011_021719fc(HashContext *context);
void func_ov011_02171a24(HashContext *context, const void *data, u32 size);
void func_ov011_02171a9c(u8 *digest, HashContext *context);
// MD5Init's body, which the DWC utility's own copy of RFC 1321's MD5 (lib/dwcutil/src/dwcutil_ac.c) also has
static inline void func_ov011_021719fcInline(HashContext *context) {
    context->count[0] = context->count[1] = 0;
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
}
// Writes the digest of size bytes as 32 hex digits and an end
void MD5Digest(const u8 *data, u32 size, char *output);

#endif // POKEBW2_DWC_MD5_H
