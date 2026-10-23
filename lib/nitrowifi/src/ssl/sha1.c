#include "nitro/mi.h"
#include "ssl_internal.h"

// SSL's own SHA-1 (FIPS 180), written from the asm. The ROM names none of it; the names are NitroWiFi's own, as
// ntrtwl's reconstruction of NitroWiFi has them. char_from_long and long_from_char are statics of the same names as
// md5.c's, as in NitroWiFi.

#define SHA1_BLOCK_SIZE 64

#define SHA1_ROL(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

#define SHA1_F1(b, c, d) (((b) & ((c) ^ (d))) ^ (d))
#define SHA1_F2(b, c, d) ((b) ^ (c) ^ (d))
#define SHA1_F3(b, c, d) (((b) & ((c) | (d))) | ((c) & (d)))
#define SHA1_F4(b, c, d) ((b) ^ (c) ^ (d))

#define SHA1_K1 0x5a827999
#define SHA1_K2 0x6ed9eba1
#define SHA1_K3 0x8f1bbcdc
#define SHA1_K4 0xca62c1d6

// One round, with the five words' roles rotated by the caller rather than moved
#define SHA1_ROUND(a, b, c, d, e, f, x, k)                                                                             \
    do {                                                                                                               \
        e += SHA1_ROL(a, 5) + f(b, c, d) + (x) + (k);                                                                  \
        b = SHA1_ROL(b, 30);                                                                                           \
    } while (0)

// The first byte of a pad run is 0x80; the rest are the zero-initialized tail
static u8 sSha1Padding[SHA1_BLOCK_SIZE] = { 0x80 };

static u32 R(int t, u32 *w);
static void char_from_long(u8 *dst, const u32 *src, u32 len);
static void long_from_char(u32 *dst, const u8 *src, u32 len);
static void CPSi_sha1_block(CPSSha1Ctx *context, const u8 *block);

// The next word of the 16-word circular message schedule, replacing the one it displaces
static u32 R(int t, u32 *w) {
    u32 value = w[(t + 13) & 0xf] ^ w[t ^ 8] ^ w[(t + 2) & 0xf] ^ w[t];
    value = SHA1_ROL(value, 1);
    w[t] = value;
    return value;
}

// Packs len/4 words into len big-endian bytes
static void char_from_long(u8 *dst, const u32 *src, u32 len) {
    u32 i;

    len >>= 2;
    for (i = 0; i < len; i++) {
        u32 value = *src++;
        *dst++ = (u8)(value >> 24);
        *dst++ = (u8)(value >> 16);
        *dst++ = (u8)(value >> 8);
        *dst++ = (u8)value;
    }
}

// Unpacks len big-endian bytes into len/4 words
static void long_from_char(u32 *dst, const u8 *src, u32 len) {
    u32 i;

    for (i = 0; i < len; i += 4) {
        *dst++ = ((u32)src[i] << 24) | ((u32)src[i + 1] << 16) | ((u32)src[i + 2] << 8) | src[i + 3];
    }
}

static void CPSi_sha1_block(CPSSha1Ctx *context, const u8 *block) {
    u32 w[16];
    u32 a, b, c, d, e;
    int t;
    int i;

    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];
    long_from_char(w, block, SHA1_BLOCK_SIZE);

    t = 0;
    for (i = 0; i < 3; i++) {
        SHA1_ROUND(a, b, c, d, e, SHA1_F1, w[t + 0], SHA1_K1);
        SHA1_ROUND(e, a, b, c, d, SHA1_F1, w[t + 1], SHA1_K1);
        SHA1_ROUND(d, e, a, b, c, SHA1_F1, w[t + 2], SHA1_K1);
        SHA1_ROUND(c, d, e, a, b, SHA1_F1, w[t + 3], SHA1_K1);
        SHA1_ROUND(b, c, d, e, a, SHA1_F1, w[t + 4], SHA1_K1);
        t += 5;
    }
    SHA1_ROUND(a, b, c, d, e, SHA1_F1, w[15], SHA1_K1);
    SHA1_ROUND(e, a, b, c, d, SHA1_F1, R(0, w), SHA1_K1);
    SHA1_ROUND(d, e, a, b, c, SHA1_F1, R(1, w), SHA1_K1);
    SHA1_ROUND(c, d, e, a, b, SHA1_F1, R(2, w), SHA1_K1);
    SHA1_ROUND(b, c, d, e, a, SHA1_F1, R(3, w), SHA1_K1);

    t = 4;
    for (i = 0; i < 4; i++) {
        SHA1_ROUND(a, b, c, d, e, SHA1_F2, R(t, w), SHA1_K2);
        SHA1_ROUND(e, a, b, c, d, SHA1_F2, R(t + 1, w), SHA1_K2);
        t = (t + 2) & 0xf;
        SHA1_ROUND(d, e, a, b, c, SHA1_F2, R(t, w), SHA1_K2);
        SHA1_ROUND(c, d, e, a, b, SHA1_F2, R(t + 1, w), SHA1_K2);
        SHA1_ROUND(b, c, d, e, a, SHA1_F2, R(t + 2, w), SHA1_K2);
        t += 3;
    }

    for (i = 0; i < 4; i++) {
        SHA1_ROUND(a, b, c, d, e, SHA1_F3, R(t, w), SHA1_K3);
        SHA1_ROUND(e, a, b, c, d, SHA1_F3, R(t + 1, w), SHA1_K3);
        SHA1_ROUND(d, e, a, b, c, SHA1_F3, R(t + 2, w), SHA1_K3);
        t = (t + 3) & 0xf;
        SHA1_ROUND(c, d, e, a, b, SHA1_F3, R(t, w), SHA1_K3);
        SHA1_ROUND(b, c, d, e, a, SHA1_F3, R(t + 1, w), SHA1_K3);
        t += 2;
    }

    for (i = 0; i < 4; i++) {
        SHA1_ROUND(a, b, c, d, e, SHA1_F4, R(t, w), SHA1_K4);
        SHA1_ROUND(e, a, b, c, d, SHA1_F4, R(t + 1, w), SHA1_K4);
        SHA1_ROUND(d, e, a, b, c, SHA1_F4, R(t + 2, w), SHA1_K4);
        SHA1_ROUND(c, d, e, a, b, SHA1_F4, R(t + 3, w), SHA1_K4);
        t = (t + 4) & 0xf;
        SHA1_ROUND(b, c, d, e, a, SHA1_F4, R(t, w), SHA1_K4);
        t += 1;
    }

    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
}

void CPSi_sha1_init(CPSSha1Ctx *context) {
    MI_CpuFill8(context, 0, sizeof(CPSSha1Ctx));
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
    context->state[4] = 0xc3d2e1f0;
}

void CPSi_sha1_calc(CPSSha1Ctx *context, const u8 *data, u32 len) {
    u32 index;
    u32 space;
    u32 i;

    index = (context->count[1] >> 3) & 0x3f;
    if ((context->count[1] += len << 3) < (len << 3)) {
        context->count[0]++;
    }
    context->count[0] += len >> 29;

    space = SHA1_BLOCK_SIZE - index;
    if (len >= space) {
        MI_CpuCopy8(data, &context->buffer[index], space);
        index = 0;
        CPSi_sha1_block(context, context->buffer);
        for (i = space; i + 0x3f < len; i += SHA1_BLOCK_SIZE) {
            CPSi_sha1_block(context, data + i);
        }
    } else {
        i = 0;
    }

    MI_CpuCopy8(data + i, &context->buffer[index], len - i);
}

void CPSi_sha1_result(CPSSha1Ctx *context, u8 *digest) {
    int index;
    u32 padLen;

    // Captures the bit count before the padding below changes it (digest is only used as scratch here; it is
    // overwritten with the real digest further down)
    char_from_long(digest, context->count, 8);

    index = (context->count[1] >> 3) & 0x3f;
    padLen = (index < 0x38) ? (0x38 - index) : (0x78 - index);
    CPSi_sha1_calc(context, sSha1Padding, padLen);

    CPSi_sha1_calc(context, digest, 8);

    char_from_long(digest, context->state, sizeof(context->state));
}

void CPSi_sha1_result_prng(CPSSha1Ctx *context, u8 *digest) {
    CPSi_sha1_calc(context, &sSha1Padding[1], 0x2c);
    char_from_long(digest, context->state, sizeof(context->state));
}
