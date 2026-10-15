#ifndef POKEBW2_NITROWIFI_SSL_INTERNAL_H
#define POKEBW2_NITROWIFI_SSL_INTERNAL_H

#include "types.h"

// SSL's state and helpers, shared by its files

// The C library's string functions the main module links, under their own names (ssl.c's certificate and session
// matching needs them; sys_memcpy and sys_memset, with the reversed argument order, cover the rest)
int memcmp(const void *a, const void *b, u32 size);
int strcmp(const char *a, const char *b);
u32 strlen(const char *s);

// rc4.c: RC4's state, with pokediamond's field layout (CPSi_rc4_init writes s right after i and j, with no padding)
typedef struct CPSRc4Context {
    u8 i;
    u8 j;
    u8 s[256];
} CPSRc4Context;

void CPSi_rc4_init(CPSRc4Context *context, const u8 *key, int keyLength);
void CPSi_rc4_crypt(CPSRc4Context *context, void *buf, int length);

// md5.c: MD5 (RFC 1321). No swan name for the object or any of its functions; named for the algorithm.
typedef struct MD5Context {
    u32 state[4];
    u32 count[2]; // The message's length so far, in bits, as a 64-bit little-endian pair
    u8 buffer[64];
} MD5Context;

void MD5Init(MD5Context *context);
void MD5Update(MD5Context *context, const u8 *input, u32 inputLen);
void MD5Final(MD5Context *context, u8 digest[16]);

// sha1.c: SHA-1 (FIPS 180). No swan name for the object or any of its functions; named for the algorithm.
typedef struct SHA1Context {
    u32 state[5];
    u32 count[2]; // The total bits hashed so far: count[0] is the high word, count[1] the low one SHA1Update adds to
    u8 buffer[64];
} SHA1Context;

void SHA1Init(SHA1Context *context);
void SHA1Update(SHA1Context *context, const u8 *data, u32 len);
void SHA1Final(SHA1Context *context, u8 *digest);
// Finishes the hash like SHA1Final, but pads with 44 zero bytes and never appends the bit count. Only ever called
// right after a 20-byte SHA1Update, so the zero padding completes exactly one block; used to extend a digest into a
// longer MAC/PRNG output rather than to hash a message (name a guess)
void SHA1FinalNoLength(SHA1Context *context, u8 *digest);

// biginteger.c: a multi-precision integer library, each value a little-endian array of 16-bit digits with a digit
// count given by every call (no header, no sign byte). No swan name for the object or any of its functions; named
// for what each does. A multiplier pointer equal to BIGINT_SQUARE tells BigInti_ModExpStep to square its value
// instead of multiplying it by another bignum
#define BIGINT_SQUARE ((const u16 *)1)

s32 BigInt_GetSign(const u16 *digits, int n);
void BigInt_Add(u16 *dst, const u16 *a, const u16 *b, int n);
void BigInt_Negate(u16 *digits, int n);
void BigInt_Sub(u16 *dst, const u16 *a, const u16 *b, int n);
void BigInt_Mul(u16 *dst, const u16 *a, const u16 *b, int n);
void BigInt_Div(u16 *quotient, const u16 *dividend, const u16 *divisor, u16 *remainder, int n, u16 *workBuf);
void BigInt_ModExp(u16 *dst, const u16 *base, const u16 *exponent, int n, const u16 *modulus);
void BigInt_ModExpFast(u16 *dst, const u16 *base, const u16 *exponent, int n, const u16 *modulus);
void BigInt_BytesToDigits(u16 *dst, const u8 *src, int nbytes, int n);
// n is the source digit count passed by callers; the original implementation does not use it.
void BigInt_DigitsToBytes(u8 *dst, const u16 *src, int nbytes, int n);

#endif // POKEBW2_NITROWIFI_SSL_INTERNAL_H
