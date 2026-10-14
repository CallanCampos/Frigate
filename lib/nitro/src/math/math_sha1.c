// NitroSDK's libmath SHA-1 and the HMAC over it (the file name is a guess; see math_md5.c for why it is apart from
// MD5). sha1_block, the compression, is hand-written ARM in sha1_block.c; the block function is called through a pointer
// that sha1_set_masked can swap for a variant that clears words 6 and 14 of the block. Names are the SDK's as pret's
// pokediamond and pmd-sky use them, or descriptive.
#include "nitro/math.h"
#include "nitro/mi.h"

#define SHA1_BLOCK_SIZE 64

#define BSWAP32(x) (((u8)((x) >> 24)) | (((x) >> 8) & 0x0000ff00) | (((x) << 8) & 0x00ff0000) | (((x) << 24) & 0xff000000))

static void (*sSHA1BlockFunc)(MATHSHA1Context *context) = sha1_block;
static u32 sSHA1Masked;

// The padding that ends a message: a one bit, then zeros
static const u8 SHA1_PADDING[12] = { 0x80 };

static void sha1_block_masked(MATHSHA1Context *context);
static void sha1_zero_fill(MATHSHA1Context *context, u32 value, u32 size);
static u32 sha1_set_masked(u32 masked);
static void hmac_generic(void *out, const void *data, u32 dataSize, const void *key, u32 keySize,
                         const MATHHashInfo *info);

// Runs sha1_block with words 6 and 14 of the block cleared, and restores them after
static void sha1_block_masked(MATHSHA1Context *context) {
    u32 *words = context->block.words;
    u32 word6 = words[6];
    u32 word14 = words[14];

    words[6] = 0;
    words[14] = 0;
    sha1_block(context);
    words[6] = word6;
    words[14] = word14;
}

// Fills size bytes of the message with value, as MATH_SHA1Update does the bytes it is given
static void sha1_zero_fill(MATHSHA1Context *context, u32 value, u32 size) {
    if (size == 0) {
        return;
    }
    do {
        u32 chunk = SHA1_BLOCK_SIZE - context->blockPos;

        if (chunk > size) {
            chunk = size;
        }
        sys_memset(&context->block.bytes[context->blockPos], value, chunk);
        size -= chunk;
        context->blockPos += chunk;
        if (context->blockPos >= SHA1_BLOCK_SIZE) {
            sSHA1BlockFunc(context);
            context->blockPos = 0;
            context->blockCount++;
            if (context->blockCount == 0) {
                context->blockCountHigh++;
            }
        }
    } while (size != 0);
}

// Chooses whether the block function clears words 6 and 14, returning the choice before
static u32 sha1_set_masked(u32 masked) {
    u32 old = sSHA1Masked;

    sSHA1Masked = masked;
    if (masked) {
        sSHA1BlockFunc = sha1_block_masked;
    } else {
        sSHA1BlockFunc = sha1_block;
    }
    return old;
}

void MATH_SHA1Init(MATHSHA1Context *context) {
    context->blockCount = 0;
    context->blockCountHigh = 0;
    context->blockPos = 0;
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
    context->state[4] = 0xc3d2e1f0;
}

void MATH_SHA1Update(MATHSHA1Context *context, const void *data, u32 size) {
    const u8 *src = data;

    if (size == 0) {
        return;
    }
    do {
        u32 chunk = SHA1_BLOCK_SIZE - context->blockPos;

        if (chunk > size) {
            chunk = size;
        }
        sys_memcpy(src, &context->block.bytes[context->blockPos], chunk);
        src += chunk;
        size -= chunk;
        context->blockPos += chunk;
        if (context->blockPos >= SHA1_BLOCK_SIZE) {
            sSHA1BlockFunc(context);
            context->blockPos = 0;
            context->blockCount++;
            if (context->blockCount == 0) {
                context->blockCountHigh++;
            }
        }
    } while (size != 0);
}

void MATH_SHA1GetHash(MATHSHA1Context *context, void *digest) {
    u32 length[2];
    u32 bits;
    u32 room;

    bits = (context->blockCount << 9) + (context->blockPos << 3);
    length[1] = BSWAP32(bits);
    bits = (context->blockCountHigh << 9) + (context->blockCount >> 23);
    length[0] = BSWAP32(bits);
    MATH_SHA1Update(context, &SHA1_PADDING[0], 1);
    room = SHA1_BLOCK_SIZE - context->blockPos;
    if (room < 8) {
        MATH_SHA1Update(context, &SHA1_PADDING[1], room);
    }
    sha1_zero_fill(context, 0, 56 - context->blockPos);
    MATH_SHA1Update(context, length, 8);
    context->state[0] = BSWAP32(context->state[0]);
    context->state[1] = BSWAP32(context->state[1]);
    context->state[2] = BSWAP32(context->state[2]);
    context->state[3] = BSWAP32(context->state[3]);
    context->state[4] = BSWAP32(context->state[4]);
    sys_memcpy(context, digest, 20);
}

void MATH_CalcSHA1(u8 *digest, const void *data, u32 size) {
    MATHSHA1Context context;

    MATH_SHA1Init(&context);
    MATH_SHA1Update(&context, data, size);
    MATH_SHA1GetHash(&context, digest);
}

void MATH_CalcHMACSHA1(void *digest, const void *data, u32 dataSize, const void *key, u32 keySize) {
    MATHSHA1Context context;
    u8 hash[MATH_SHA1_DIGEST_SIZE];
    MATHHashInfo info = { MATH_SHA1_DIGEST_SIZE, SHA1_BLOCK_SIZE };

    info.context = &context;
    info.digest = hash;
    info.init = (void (*)(void *))MATH_SHA1Init;
    info.update = (void (*)(void *, const void *, u32))MATH_SHA1Update;
    info.getHash = (void (*)(void *, void *))MATH_SHA1GetHash;
    hmac_generic(digest, data, dataSize, key, keySize, &info);
}

static void hmac_generic(void *out, const void *data, u32 dataSize, const void *key, u32 keySize,
                         const MATHHashInfo *info) {
    u8 keyHash[64];
    u8 innerPad[64];
    u8 outerPad[64];
    const u8 *keyBytes = key;
    u32 i;

    if (out == NULL || data == NULL || dataSize == 0 || key == NULL || keySize == 0 || info == NULL) {
        return;
    }
    if (keySize > info->blockSize) {
        info->init(info->context);
        info->update(info->context, key, keySize);
        keyBytes = keyHash;
        info->getHash(info->context, keyHash);
        keySize = info->digestSize;
    }
    for (i = 0; i < keySize; i++) {
        innerPad[i] = keyBytes[i] ^ 0x36;
    }
    for (; i < info->blockSize; i++) {
        innerPad[i] = 0x36;
    }
    info->init(info->context);
    info->update(info->context, innerPad, info->blockSize);
    info->update(info->context, data, dataSize);
    info->getHash(info->context, info->digest);
    for (i = 0; i < keySize; i++) {
        outerPad[i] = keyBytes[i] ^ 0x5c;
    }
    for (; i < info->blockSize; i++) {
        outerPad[i] = 0x5c;
    }
    info->init(info->context);
    info->update(info->context, outerPad, info->blockSize);
    info->update(info->context, info->digest, info->digestSize);
    info->getHash(info->context, out);
}
