#include "ssl_internal.h"
#include "gfl/std.h"

// SSL's own SHA-1 (FIPS 180). No swan name for the object or any of its functions; everything here is named for the
// algorithm rather than copied from a reference implementation.

#define SHA1_BLOCK_SIZE 64

#define SHA1_ROL(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

#define SHA1_F1(b, c, d) (((b) & ((c) ^ (d))) ^ (d))
#define SHA1_F2(b, c, d) ((b) ^ (c) ^ (d))
#define SHA1_F3(b, c, d) (((b) & (c)) | ((d) & ((b) | (c))))
#define SHA1_F4(b, c, d) ((b) ^ (c) ^ (d))

#define SHA1_K1 0x5a827999
#define SHA1_K2 0x6ed9eba1
#define SHA1_K3 0x8f1bbcdc
#define SHA1_K4 0xca62c1d6

// The first byte of a pad run is 0x80; the rest are the zero-initialized tail
static u8 sSha1Padding[SHA1_BLOCK_SIZE] = { 0x80 };

static u32 Sha1ExpandWord(int t, u32 *w);
static void Sha1PutBigEndian(u8 *dst, const u32 *src, u32 len);
static void Sha1GetBigEndian(u32 *dst, const u8 *src, u32 len);
static void Sha1Transform(SHA1Context *context, const u8 *block);

// The next word of the 16-word circular message schedule, replacing the one it displaces
static u32 Sha1ExpandWord(int t, u32 *w) {
    u32 value = w[(t + 13) & 0xf] ^ w[t ^ 8] ^ w[(t + 2) & 0xf] ^ w[t];
    value = SHA1_ROL(value, 1);
    w[t] = value;
    return value;
}

// Packs len/4 words into len big-endian bytes
static void Sha1PutBigEndian(u8 *dst, const u32 *src, u32 len) {
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
static void Sha1GetBigEndian(u32 *dst, const u8 *src, u32 len) {
    u32 i;

    for (i = 0; i < len; i += 4) {
        *dst++ = ((u32)src[i] << 24) | ((u32)src[i + 1] << 16) | ((u32)src[i + 2] << 8) | src[i + 3];
    }
}

static void Sha1Transform(SHA1Context *context, const u8 *block) {
    u32 w[16];
    u32 a, b, c, d, e;
    int t;

    Sha1GetBigEndian(w, block, SHA1_BLOCK_SIZE);

    a = context->state[0];
    b = context->state[1];
    c = context->state[2];
    d = context->state[3];
    e = context->state[4];

    for (t = 0; t < 16; t++) {
        u32 temp = e + SHA1_F1(b, c, d) + w[t] + SHA1_K1 + SHA1_ROL(a, 5);
        e = d;
        d = c;
        c = SHA1_ROL(b, 30);
        b = a;
        a = temp;
    }

    for (t = 16; t < 20; t++) {
        u32 term = Sha1ExpandWord(t & 0xf, w);
        u32 temp = e + SHA1_F1(b, c, d) + term + SHA1_K1 + SHA1_ROL(a, 5);
        e = d;
        d = c;
        c = SHA1_ROL(b, 30);
        b = a;
        a = temp;
    }

    for (t = 20; t < 40; t++) {
        u32 term = Sha1ExpandWord(t & 0xf, w);
        u32 temp = e + SHA1_F2(b, c, d) + term + SHA1_K2 + SHA1_ROL(a, 5);
        e = d;
        d = c;
        c = SHA1_ROL(b, 30);
        b = a;
        a = temp;
    }

    for (t = 40; t < 60; t++) {
        u32 term = Sha1ExpandWord(t & 0xf, w);
        u32 temp = e + SHA1_F3(b, c, d) + term + SHA1_K3 + SHA1_ROL(a, 5);
        e = d;
        d = c;
        c = SHA1_ROL(b, 30);
        b = a;
        a = temp;
    }

    for (t = 60; t < 80; t++) {
        u32 term = Sha1ExpandWord(t & 0xf, w);
        u32 temp = e + SHA1_F4(b, c, d) + term + SHA1_K4 + SHA1_ROL(a, 5);
        e = d;
        d = c;
        c = SHA1_ROL(b, 30);
        b = a;
        a = temp;
    }

    context->state[0] += a;
    context->state[1] += b;
    context->state[2] += c;
    context->state[3] += d;
    context->state[4] += e;
}

void SHA1Init(SHA1Context *context) {
    sys_memset(context, 0, sizeof(SHA1Context));
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
    context->state[4] = 0xc3d2e1f0;
}

void SHA1Update(SHA1Context *context, const u8 *data, u32 len) {
    u32 index = (context->count[1] >> 3) & 0x3f;
    u32 bits = len << 3;
    u32 space;
    u32 i;

    context->count[1] += bits;
    if (context->count[1] < bits) {
        context->count[0]++;
    }
    context->count[0] += len >> 29;

    space = SHA1_BLOCK_SIZE - index;
    if (len >= space) {
        sys_memcpy(data, &context->buffer[index], space);
        Sha1Transform(context, context->buffer);
        for (i = space; i + 0x3f < len; i += SHA1_BLOCK_SIZE) {
            Sha1Transform(context, data + i);
        }
        index = 0;
    } else {
        i = 0;
    }

    sys_memcpy(data + i, &context->buffer[index], len - i);
}

void SHA1Final(SHA1Context *context, u8 *digest) {
    int index;
    u32 padLen;

    // Captures the bit count before the padding below changes it (digest is only used as scratch here; it is
    // overwritten with the real digest further down)
    Sha1PutBigEndian(digest, context->count, 8);

    index = (context->count[1] >> 3) & 0x3f;
    padLen = (index < 0x38) ? (0x38 - index) : (0x78 - index);
    SHA1Update(context, sSha1Padding, padLen);

    SHA1Update(context, digest, 8);

    Sha1PutBigEndian(digest, context->state, sizeof(context->state));
}

void SHA1FinalNoLength(SHA1Context *context, u8 *digest) {
    SHA1Update(context, &sSha1Padding[1], 0x2c);
    Sha1PutBigEndian(digest, context->state, sizeof(context->state));
}
