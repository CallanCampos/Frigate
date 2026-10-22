#ifndef POKEBW2_DWC_MD5_H
#define POKEBW2_DWC_MD5_H

// GameSpy's MD5 (md5c.c), in overlay 11: RSA's reference MD5 (RFC 1321), with a function that gives the digest of some
// bytes as 32 hex digits. The ROM has no names for the file or its functions: the file is named after GameSpy's, as
// pret/pokerevo's Wii build has it (asm/SDK/DWC/gamespy/md5c.s), and so are the functions (NitroWiFi's SSL,
// lib/nitrowifi/src/ssl/md5.c, has the SDK's own names for its MD5: CPSi_md5_init, CPSi_md5_calc, CPSi_md5_result)

typedef unsigned char *POINTER;
typedef unsigned int UINT4;

// The state of a hash: the four words of the state, the count of bits hashed and the bytes of the block not hashed yet
typedef struct {
    UINT4 state[4];
    UINT4 count[2];
    unsigned char buffer[64];
} MD5_CTX;

// MD5Init, MD5Update and MD5Final, which gives the 16 bytes of the digest
void MD5Init(MD5_CTX *context);
void MD5Update(MD5_CTX *context, unsigned char *input, unsigned int inputLen);
void MD5Final(unsigned char digest[16], MD5_CTX *context);
// MD5Init's body, which the DWC utility's own copy of RFC 1321's MD5 (lib/dwcutil/src/dwcutil_ac.c) also has
static inline void MD5InitInline(MD5_CTX *context) {
    context->count[0] = context->count[1] = 0;
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
}
// Writes the digest of size bytes as 32 hex digits and an end
void MD5Digest(unsigned char *input, unsigned int len, char output[33]);

#endif // POKEBW2_DWC_MD5_H
