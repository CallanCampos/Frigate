#ifndef POKEBW2_NITROWIFI_SSL_INTERNAL_H
#define POKEBW2_NITROWIFI_SSL_INTERNAL_H

#include "types.h"
#include "nitrowifi/cps.h"

// SSL's state and helpers, shared by its files

// SSL's signature, "[SDK+UBIQUITOUS:SSL]", in the main module (ssl_version.c), which CPS_SetSsl refers to so that the
// link keeps it
extern const char SSLi_Signature[];

// The C library's string functions the main module links, under their own names (ssl.c's certificate and session
// matching and biginteger.c need them; MI_CpuCopy8, with the source first, and MI_CpuFill8 cover the rest). They are
// not taken from <string.h>, whose declarations make MWCC treat strcmp and strlen as the library's own, which changes
// the registers around their calls (docs/matching.md, "a header named `string.h`")
void *memmove(void *dest, const void *src, u32 size);
int memcmp(const void *a, const void *b, u32 size);
int strcmp(const char *a, const char *b);
u32 strlen(const char *s);

// rc4.c: RC4, over a CPSRc4Ctx (cps.h)
void CPSi_rc4_init(CPSRc4Ctx *context, const u8 *key, int keyLength);
void CPSi_rc4_crypt(CPSRc4Ctx *context, void *buf, int length);

// md5.c: MD5 (RFC 1321), over a CPSMd5Ctx (cps.h). No swan name for the object or any of its functions; they carry
// NitroWiFi's own names.
void CPSi_md5_init(CPSMd5Ctx *context);
// inputLen counts bytes; input is borrowed synchronously, with partial blocks copied into the context.
void CPSi_md5_calc(CPSMd5Ctx *context, const u8 *input, u32 inputLen);
// Requires 16 writable output bytes separate from context. Uses output as length scratch and mutates context;
// it does not reset the context for another message. These storage requirements are not checked.
void CPSi_md5_result(CPSMd5Ctx *context, u8 digest[16]);

// sha1.c: SHA-1 (FIPS 180), over a CPSSha1Ctx (cps.h). No swan name for the object or any of its functions; they
// carry NitroWiFi's own names.
void CPSi_sha1_init(CPSSha1Ctx *context);
// len counts bytes; data is borrowed synchronously, with partial blocks copied into the context.
void CPSi_sha1_calc(CPSSha1Ctx *context, const u8 *data, u32 len);
// Requires 20 writable output bytes separate from context. Uses output as length scratch and mutates context;
// it does not reset the context for another message. These storage requirements are not checked.
void CPSi_sha1_result(CPSSha1Ctx *context, u8 *digest);
// Finishes the hash like CPSi_sha1_result, but pads with 44 zero bytes and never appends the bit count. Only ever
// called right after a 20-byte CPSi_sha1_calc, so the zero padding completes exactly one block; used to extend a digest
// into a longer MAC/PRNG output rather than to hash a message (name a guess) Also mutates context and writes 20 output
// bytes; the output extent is not checked.
void CPSi_sha1_result_prng(CPSSha1Ctx *context, u8 *digest);

// biginteger.c: a multi-precision integer library, each value a little-endian array of 16-bit digits with a digit
// count given by every call (no header, no sign byte). No swan name for the object or any of its functions; named
// for what each does. A multiplier pointer equal to BIGINT_SQUARE tells CPSi_big_montmult to square its value
// instead of multiplying it by another bignum
// n specifies fixed capacity in 16-bit digits, not bytes; operations do not grow the caller's arrays.
#define BIGINT_SQUARE ((const u16 *)1)

s32 CPSi_big_sign(const u16 *digits, int n);
void CPSi_big_add(u16 *dst, const u16 *a, const u16 *b, int n);
void CPSi_big_negate(u16 *digits, int n);
void CPSi_big_sub(u16 *dst, const u16 *a, const u16 *b, int n);
// Writes only the low n digits of the product. Clearing dst first prevents general overlap with the inputs.
void CPSi_big_mult(u16 *dst, const u16 *a, const u16 *b, int n);
// For positive n, inputs and supplied outputs have n digits; scratch has 3*n writable digits separate from them.
// If both inputs have n significant digits, the original also reads workBuf[3*n], which it does not initialize.
// A divisor with fewer than n significant digits avoids that extra read. This does not validate arithmetic inputs.
// Either output may be NULL or reuse the dividend; final copies write quotient first, then remainder.
void CPSi_big_div(u16 *quotient, const u16 *dividend, const u16 *divisor, u16 *remainder, int n, u16 *workBuf);
// Both exponentiation functions allocate and free their own temporary storage; allocation failure leaves dst
// unchanged and is not reported through their void return values.
void CPSi_big_power(u16 *dst, const u16 *base, const u16 *exponent, int n, const u16 *modulus);
void CPSi_big_montpower(u16 *dst, const u16 *base, const u16 *exponent, int n, const u16 *modulus);
// Converts big-endian bytes to little-endian digits. Clears n destination digits, then writes ceil(nbytes/2)
// digits; callers must ensure that count fits n and supply nbytes readable bytes. No capacity check is performed.
void CPSi_big_from_char(u16 *dst, const u8 *src, int nbytes, int n);
// n is the source digit count passed by callers; the original implementation does not use it.
// Converts little-endian digits to nbytes big-endian bytes. Callers must supply ceil(nbytes/2) actual readable
// source digits and nbytes writable output bytes; the ignored n argument does not validate either extent.
void CPSi_char_from_big(u8 *dst, const u16 *src, int nbytes, int n);

#endif // POKEBW2_NITROWIFI_SSL_INTERNAL_H
