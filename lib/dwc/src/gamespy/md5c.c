#include "dwc/ext.h"
#include "dwc/md5.h"

// GameSpy's MD5 (md5c.c): RSA's reference MD5, from RFC 1321, whose names the static functions carry. MD5Transform
// keeps its address, with its name in the comment: NitroWiFi's SSL has a function of that name in this overlay

// RFC 1321's MD5Transform, and its helpers
static void func_ov011_02171af0(gsi_u32 state[4], const gsi_u8 block[64]);
static void Encode(gsi_u8 *output, gsi_u32 *input, gsi_u32 length);
static void Decode(gsi_u32 *output, const gsi_u8 *input, gsi_u32 length);
static void MD5_memcpy(void *output, const void *input, gsi_u32 length);
static void MD5_memset(void *output, int value, gsi_u32 length);
// Writes a digest as hex digits
static void MD5Print(gsi_u8 digest[16], char *output);

static gsi_u8 PADDING[64] = { 0x80 };

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define FF(a, b, c, d, x, s, ac)                                                                                       \
    {                                                                                                                  \
        (a) += F((b), (c), (d)) + (x) + (gsi_u32)(ac);                                                                 \
        (a) = ROTATE_LEFT((a), (s));                                                                                   \
        (a) += (b);                                                                                                    \
    }
#define GG(a, b, c, d, x, s, ac)                                                                                       \
    {                                                                                                                  \
        (a) += G((b), (c), (d)) + (x) + (gsi_u32)(ac);                                                                 \
        (a) = ROTATE_LEFT((a), (s));                                                                                   \
        (a) += (b);                                                                                                    \
    }
#define HH(a, b, c, d, x, s, ac)                                                                                       \
    {                                                                                                                  \
        (a) += H((b), (c), (d)) + (x) + (gsi_u32)(ac);                                                                 \
        (a) = ROTATE_LEFT((a), (s));                                                                                   \
        (a) += (b);                                                                                                    \
    }
#define II(a, b, c, d, x, s, ac)                                                                                       \
    {                                                                                                                  \
        (a) += I((b), (c), (d)) + (x) + (gsi_u32)(ac);                                                                 \
        (a) = ROTATE_LEFT((a), (s));                                                                                   \
        (a) += (b);                                                                                                    \
    }

void func_ov011_021719fc(HashContext *context) {
    func_ov011_021719fcInline(context);
}

void func_ov011_02171a24(HashContext *context, const void *data, gsi_u32 inputLen) {
    const gsi_u8 *input = data;
    gsi_u32 i;
    gsi_u32 index;
    gsi_u32 partLen;

    index = (context->count[0] >> 3) & 0x3F;
    if ((context->count[0] += (inputLen << 3)) < (inputLen << 3)) {
        context->count[1]++;
    }
    context->count[1] += (inputLen >> 29);
    partLen = 64 - index;
    if (inputLen >= partLen) {
        MD5_memcpy(&context->buffer[index], input, partLen);
        func_ov011_02171af0(context->state, context->buffer);
        for (i = partLen; i + 63 < inputLen; i += 64) {
            func_ov011_02171af0(context->state, &input[i]);
        }
        index = 0;
    } else {
        i = 0;
    }
    MD5_memcpy(&context->buffer[index], &input[i], inputLen - i);
}

void func_ov011_02171a9c(gsi_u8 digest[16], HashContext *context) {
    gsi_u8 bits[8];
    gsi_u32 index;
    gsi_u32 padLen;

    Encode(bits, context->count, 8);
    index = (context->count[0] >> 3) & 0x3f;
    padLen = (index < 56) ? (56 - index) : (120 - index);
    func_ov011_02171a24(context, PADDING, padLen);
    func_ov011_02171a24(context, bits, 8);
    Encode(digest, context->state, 16);
    MD5_memset(context, 0, sizeof(*context));
}

static void func_ov011_02171af0(gsi_u32 state[4], const gsi_u8 block[64]) {
    gsi_u32 a = state[0];
    gsi_u32 b = state[1];
    gsi_u32 c = state[2];
    gsi_u32 d = state[3];
    gsi_u32 x[16];

    Decode(x, block, 64);

    FF(a, b, c, d, x[0], S11, 0xd76aa478);
    FF(d, a, b, c, x[1], S12, 0xe8c7b756);
    FF(c, d, a, b, x[2], S13, 0x242070db);
    FF(b, c, d, a, x[3], S14, 0xc1bdceee);
    FF(a, b, c, d, x[4], S11, 0xf57c0faf);
    FF(d, a, b, c, x[5], S12, 0x4787c62a);
    FF(c, d, a, b, x[6], S13, 0xa8304613);
    FF(b, c, d, a, x[7], S14, 0xfd469501);
    FF(a, b, c, d, x[8], S11, 0x698098d8);
    FF(d, a, b, c, x[9], S12, 0x8b44f7af);
    FF(c, d, a, b, x[10], S13, 0xffff5bb1);
    FF(b, c, d, a, x[11], S14, 0x895cd7be);
    FF(a, b, c, d, x[12], S11, 0x6b901122);
    FF(d, a, b, c, x[13], S12, 0xfd987193);
    FF(c, d, a, b, x[14], S13, 0xa679438e);
    FF(b, c, d, a, x[15], S14, 0x49b40821);

    GG(a, b, c, d, x[1], S21, 0xf61e2562);
    GG(d, a, b, c, x[6], S22, 0xc040b340);
    GG(c, d, a, b, x[11], S23, 0x265e5a51);
    GG(b, c, d, a, x[0], S24, 0xe9b6c7aa);
    GG(a, b, c, d, x[5], S21, 0xd62f105d);
    GG(d, a, b, c, x[10], S22, 0x2441453);
    GG(c, d, a, b, x[15], S23, 0xd8a1e681);
    GG(b, c, d, a, x[4], S24, 0xe7d3fbc8);
    GG(a, b, c, d, x[9], S21, 0x21e1cde6);
    GG(d, a, b, c, x[14], S22, 0xc33707d6);
    GG(c, d, a, b, x[3], S23, 0xf4d50d87);
    GG(b, c, d, a, x[8], S24, 0x455a14ed);
    GG(a, b, c, d, x[13], S21, 0xa9e3e905);
    GG(d, a, b, c, x[2], S22, 0xfcefa3f8);
    GG(c, d, a, b, x[7], S23, 0x676f02d9);
    GG(b, c, d, a, x[12], S24, 0x8d2a4c8a);

    HH(a, b, c, d, x[5], S31, 0xfffa3942);
    HH(d, a, b, c, x[8], S32, 0x8771f681);
    HH(c, d, a, b, x[11], S33, 0x6d9d6122);
    HH(b, c, d, a, x[14], S34, 0xfde5380c);
    HH(a, b, c, d, x[1], S31, 0xa4beea44);
    HH(d, a, b, c, x[4], S32, 0x4bdecfa9);
    HH(c, d, a, b, x[7], S33, 0xf6bb4b60);
    HH(b, c, d, a, x[10], S34, 0xbebfbc70);
    HH(a, b, c, d, x[13], S31, 0x289b7ec6);
    HH(d, a, b, c, x[0], S32, 0xeaa127fa);
    HH(c, d, a, b, x[3], S33, 0xd4ef3085);
    HH(b, c, d, a, x[6], S34, 0x4881d05);
    HH(a, b, c, d, x[9], S31, 0xd9d4d039);
    HH(d, a, b, c, x[12], S32, 0xe6db99e5);
    HH(c, d, a, b, x[15], S33, 0x1fa27cf8);
    HH(b, c, d, a, x[2], S34, 0xc4ac5665);

    II(a, b, c, d, x[0], S41, 0xf4292244);
    II(d, a, b, c, x[7], S42, 0x432aff97);
    II(c, d, a, b, x[14], S43, 0xab9423a7);
    II(b, c, d, a, x[5], S44, 0xfc93a039);
    II(a, b, c, d, x[12], S41, 0x655b59c3);
    II(d, a, b, c, x[3], S42, 0x8f0ccc92);
    II(c, d, a, b, x[10], S43, 0xffeff47d);
    II(b, c, d, a, x[1], S44, 0x85845dd1);
    II(a, b, c, d, x[8], S41, 0x6fa87e4f);
    II(d, a, b, c, x[15], S42, 0xfe2ce6e0);
    II(c, d, a, b, x[6], S43, 0xa3014314);
    II(b, c, d, a, x[13], S44, 0x4e0811a1);
    II(a, b, c, d, x[4], S41, 0xf7537e82);
    II(d, a, b, c, x[11], S42, 0xbd3af235);
    II(c, d, a, b, x[2], S43, 0x2ad7d2bb);
    II(b, c, d, a, x[9], S44, 0xeb86d391);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

    MD5_memset(x, 0, sizeof(x));
}

static void Encode(gsi_u8 *output, gsi_u32 *input, gsi_u32 length) {
    gsi_u32 i;
    gsi_u32 j;

    for (i = 0, j = 0; j < length; i++, j += 4) {
        output[j] = (gsi_u8)(input[i] & 0xff);
        output[j + 1] = (gsi_u8)((input[i] >> 8) & 0xff);
        output[j + 2] = (gsi_u8)((input[i] >> 16) & 0xff);
        output[j + 3] = (gsi_u8)((input[i] >> 24) & 0xff);
    }
}

static void Decode(gsi_u32 *output, const gsi_u8 *input, gsi_u32 length) {
    gsi_u32 i;
    gsi_u32 j;

    for (i = 0, j = 0; j < length; i++, j += 4) {
        output[i] = ((gsi_u32)input[j]) | (((gsi_u32)input[j + 1]) << 8) | (((gsi_u32)input[j + 2]) << 16) |
                    (((gsi_u32)input[j + 3]) << 24);
    }
}

static void MD5_memcpy(void *output, const void *input, gsi_u32 length) {
    memcpy(output, input, length);
}

static void MD5_memset(void *output, int value, gsi_u32 length) {
    memset(output, value, length);
}

// Writes the digest as 32 hex digits and an end
static void MD5Print(gsi_u8 digest[16], char *output) {
    static const char hexDigits[] = "0123456789abcdef";
    gsi_u32 i;

    for (i = 0; i < 16; i++) {
        output[i * 2] = hexDigits[digest[i] / 16];
        output[i * 2 + 1] = hexDigits[digest[i] % 16];
    }
    output[32] = '\0';
}

void MD5Digest(const gsi_u8 *input, gsi_u32 length, char *output) {
    HashContext context;
    gsi_u8 digest[16];

    func_ov011_021719fc(&context);
    func_ov011_02171a24(&context, input, length);
    func_ov011_02171a9c(digest, &context);
    MD5Print(digest, output);
}
