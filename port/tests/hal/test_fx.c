// Unit tests of port/hal's host versions of the SDK's assembly in libfx's matrix files, libmath (MATH_QSort,
// MATH_CountLeadingZerosFunc, sha1_block) and libcrypto (CRYPTO_RC4Encrypt), on known vectors, with no ROM: the
// standard SHA-1 and RC4 test vectors, overlapping matrices, and results recorded from the ROM's code for what only
// the original's order of work decides (MATH_QSort's order of equal elements and its comparison calls, RC4 writing
// over its own state). port/tests/asm_oracle/test_fx.py compares the same functions with the ROM's code on random
// inputs. Exits 0 when every check passes, 1 otherwise.
#include "nitro/crypto/rc4.h"
#include "nitro/fx.h"
#include "nitro/math.h"
#include <stdio.h>
#include <string.h>

// What a test leaves in the words around a matrix, which must still be there after it
#define SENTINEL ((fx32)0x5aa5a55a)

static int sFailures;

static void Check(int ok, const char *what) {
    if (!ok) {
        printf("FAIL %s\n", what);
        sFailures++;
    }
}

static void CheckWords(const char *what, const fx32 *actual, const fx32 *expected, int count) {
    int i;

    for (i = 0; i < count; i++) {
        if (actual[i] != expected[i]) {
            printf("FAIL %s: word %d is %#x, expected %#x\n", what, i, (u32)actual[i], (u32)expected[i]);
            sFailures++;
            return;
        }
    }
}

static void CheckBytes(const char *what, const u8 *actual, const u8 *expected, u32 size) {
    u32 i;

    for (i = 0; i < size; i++) {
        if (actual[i] != expected[i]) {
            printf("FAIL %s: byte %u is %#04x, expected %#04x\n", what, i, actual[i], expected[i]);
            sFailures++;
            return;
        }
    }
}

// A buffer of 18 words: a sentinel, the 16 words a matrix can take, and a sentinel
typedef struct {
    fx32 before;
    fx32 words[16];
    fx32 after;
} MatrixBuffer;

static void FillBuffer(MatrixBuffer *buffer) {
    int i;

    buffer->before = SENTINEL;
    for (i = 0; i < 16; i++) {
        buffer->words[i] = SENTINEL;
    }
    buffer->after = SENTINEL;
}

// Checks the first count words of the buffer, and that the words after them and around the buffer are untouched
static void CheckMatrix(const char *what, const MatrixBuffer *buffer, const fx32 *expected, int count) {
    int i;

    CheckWords(what, buffer->words, expected, count);
    for (i = count; i < 16; i++) {
        Check(buffer->words[i] == SENTINEL, what);
    }
    Check(buffer->before == SENTINEL && buffer->after == SENTINEL, what);
}

static void TestMatrixStores(void) {
    static const fx32 identity22[] = { 0x1000, 0, 0, 0x1000 };
    static const fx32 scaling22[] = { 0x7fffffff, 0, 0, -5 };
    static const fx32 rot22[] = { 0xe00, -0x7fffffff - 1, -0x7fffffff - 1, 0xe00 };
    static const fx32 identity33[] = { 0x1000, 0, 0, 0, 0x1000, 0, 0, 0, 0x1000 };
    static const fx32 scaling33[] = { 3, 0, 0, 0, -4, 0, 0, 0, 5 };
    static const fx32 rotX33[] = { 0x1000, 0, 0, 0, 0xddb, 0x800, 0, -0x800, 0xddb };
    static const fx32 rotY33[] = { 0xddb, 0, -0x800, 0, 0x1000, 0, 0x800, 0, 0xddb };
    static const fx32 rotZ33[] = { 0xddb, 0x800, 0, -0x800, 0xddb, 0, 0, 0, 0x1000 };
    static const fx32 identity43[] = { 0x1000, 0, 0, 0, 0x1000, 0, 0, 0, 0x1000, 0, 0, 0 };
    static const fx32 scale43[] = { 0x2000, 0, 0, 0, 0x3000, 0, 0, 0, -0x4000, 0, 0, 0 };
    static const fx32 rotY43[] = { -0x1000, 0, -1, 0, 0x1000, 0, 1, 0, -0x1000, 0, 0, 0 };
    static const fx32 identity44[] = { 0x1000, 0, 0, 0, 0, 0x1000, 0, 0, 0, 0, 0x1000, 0, 0, 0, 0, 0x1000 };
    MatrixBuffer buffer;

    FillBuffer(&buffer);
    MTX_Identity22_((MtxFx22 *)buffer.words);
    CheckMatrix("MTX_Identity22_", &buffer, identity22, 4);
    FillBuffer(&buffer);
    MTX_Scale22_((MtxFx22 *)buffer.words, 0x7fffffff, -5);
    CheckMatrix("MTX_Scale22_", &buffer, scaling22, 4);
    // The sine FX32_MIN negates to itself
    FillBuffer(&buffer);
    MTX_Rot22_((MtxFx22 *)buffer.words, -0x7fffffff - 1, 0xe00);
    CheckMatrix("MTX_Rot22_", &buffer, rot22, 4);

    FillBuffer(&buffer);
    MTX_Identity33_((MtxFx33 *)buffer.words);
    CheckMatrix("MTX_Identity33_", &buffer, identity33, 9);
    FillBuffer(&buffer);
    MTX_Scale33_((MtxFx33 *)buffer.words, 3, -4, 5);
    CheckMatrix("MTX_Scale33_", &buffer, scaling33, 9);
    // 30 degrees
    FillBuffer(&buffer);
    MTX_RotX33_((MtxFx33 *)buffer.words, 0x800, 0xddb);
    CheckMatrix("MTX_RotX33_", &buffer, rotX33, 9);
    FillBuffer(&buffer);
    MTX_RotY33_((MtxFx33 *)buffer.words, 0x800, 0xddb);
    CheckMatrix("MTX_RotY33_", &buffer, rotY33, 9);
    FillBuffer(&buffer);
    MTX_RotZ33_((MtxFx33 *)buffer.words, 0x800, 0xddb);
    CheckMatrix("MTX_RotZ33_", &buffer, rotZ33, 9);

    FillBuffer(&buffer);
    MTX_Identity43_((MtxFx43 *)buffer.words);
    CheckMatrix("MTX_Identity43_", &buffer, identity43, 12);
    FillBuffer(&buffer);
    MTX_Scale43_((MtxFx43 *)buffer.words, 0x2000, 0x3000, -0x4000);
    CheckMatrix("MTX_Scale43_", &buffer, scale43, 12);
    // 180 degrees, with a sine of 1 left by rounding
    FillBuffer(&buffer);
    MTX_RotY43_((MtxFx43 *)buffer.words, 1, -0x1000);
    CheckMatrix("MTX_RotY43_", &buffer, rotY43, 12);

    FillBuffer(&buffer);
    MTX_Identity44_((MtxFx44 *)buffer.words);
    CheckMatrix("MTX_Identity44_", &buffer, identity44, 16);
}

static void TestMatrixCopies(void) {
    static const fx32 to4x4[] = { 1, 2, 3, 0, 4, 5, 6, 0, 7, 8, 9, 0, 0, 0, 0, 0x1000 };
    static const fx32 copy43To44[] = { 1, 2, 3, 0, 4, 5, 6, 0, 7, 8, 9, 0, 10, 11, 12, 0x1000 };
    static const fx32 copy44To43[] = { 1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15 };
    // In place, each row is stored over source words that are still to be read, as on the DS
    static const fx32 to4x4InPlace[] = { 1, 2, 3, 0, 0, 5, 6, 0, 6, 0, 9, 0, 0, 0, 0, 0x1000 };
    static const fx32 copy43To44InPlace[] = { 1, 2, 3, 0, 0, 5, 6, 0, 6, 0, 9, 0, 0, 9, 0, 0x1000 };
    static const fx32 copy44To43InPlace[] = { 1, 2, 3, 5, 6, 7, 9, 10, 11, 13, 14, 15, 13, 14, 15, 16 };
    MatrixBuffer source;
    MatrixBuffer buffer;
    int i;

    FillBuffer(&source);
    for (i = 0; i < 16; i++) {
        source.words[i] = i + 1;
    }

    FillBuffer(&buffer);
    MTX_Copy33To44_((const MtxFx33 *)source.words, (MtxFx44 *)buffer.words);
    CheckMatrix("MTX_Copy33To44_", &buffer, to4x4, 16);
    FillBuffer(&buffer);
    MTX_Copy43To44_((const MtxFx43 *)source.words, (MtxFx44 *)buffer.words);
    CheckMatrix("MTX_Copy43To44_", &buffer, copy43To44, 16);
    FillBuffer(&buffer);
    MTX_Copy44To43_((const MtxFx44 *)source.words, (MtxFx43 *)buffer.words);
    CheckMatrix("MTX_Copy44To43_", &buffer, copy44To43, 12);

    buffer = source;
    MTX_Copy33To44_((const MtxFx33 *)buffer.words, (MtxFx44 *)buffer.words);
    CheckMatrix("MTX_Copy33To44_ in place", &buffer, to4x4InPlace, 16);
    buffer = source;
    MTX_Copy43To44_((const MtxFx43 *)buffer.words, (MtxFx44 *)buffer.words);
    CheckMatrix("MTX_Copy43To44_ in place", &buffer, copy43To44InPlace, 16);
    buffer = source;
    MTX_Copy44To43_((const MtxFx44 *)buffer.words, (MtxFx43 *)buffer.words);
    CheckMatrix("MTX_Copy44To43_ in place", &buffer, copy44To43InPlace, 16);
}

static void TestCountLeadingZeros(void) {
    static const u32 values[] = { 0, 1, 2, 3, 0x8000, 0xffff, 0x10000, 0x7fffffff, 0x80000000, 0xffffffff };
    static const u32 counts[] = { 32, 31, 30, 30, 16, 16, 15, 1, 0, 0 };
    u32 i;

    for (i = 0; i < NELEMS(values); i++) {
        if (MATH_CountLeadingZerosFunc(values[i]) != counts[i]) {
            printf("FAIL MATH_CountLeadingZerosFunc(%#x) is %u, expected %u\n", values[i],
                   MATH_CountLeadingZerosFunc(values[i]), counts[i]);
            sFailures++;
        }
    }
}

static void Sha1Start(MATHSHA1Context *context) {
    memset(context, 0xa5, sizeof(*context));
    context->state[0] = 0x67452301;
    context->state[1] = 0xefcdab89;
    context->state[2] = 0x98badcfe;
    context->state[3] = 0x10325476;
    context->state[4] = 0xc3d2e1f0;
}

// Puts size bytes of a message in the block, and zeros after them
static void Sha1SetBlock(MATHSHA1Context *context, const char *text, u32 size) {
    memset(context->block.bytes, 0, sizeof(context->block.bytes));
    memcpy(context->block.bytes, text, size);
}

// Puts the message's length in bits at the end of the block, as its last block ends
static void Sha1SetLength(MATHSHA1Context *context, u32 bits) {
    context->block.bytes[60] = (u8)(bits >> 24);
    context->block.bytes[61] = (u8)(bits >> 16);
    context->block.bytes[62] = (u8)(bits >> 8);
    context->block.bytes[63] = (u8)bits;
}

static void TestSha1Block(void) {
    static const char twoBlocks[] = "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
    static const u32 empty[] = { 0xda39a3ee, 0x5e6b4b0d, 0x3255bfef, 0x95601890, 0xafd80709 };
    static const u32 abc[] = { 0xa9993e36, 0x4706816a, 0xba3e2571, 0x7850c26c, 0x9cd0d89d };
    static const u32 long448[] = { 0x84983e44, 0x1c3bd26e, 0xbaae4aa1, 0xf95129e5, 0xe54670f1 };
    MATHSHA1Context context;
    MATHSHA1Context before;

    // Each message ends with a one bit, zeros and its length
    Sha1Start(&context);
    Sha1SetBlock(&context, "", 0);
    context.block.bytes[0] = 0x80;
    Sha1SetLength(&context, 0);
    sha1_block(&context);
    CheckWords("sha1_block of the empty message", (const fx32 *)context.state, (const fx32 *)empty, 5);

    Sha1Start(&context);
    Sha1SetBlock(&context, "abc", 3);
    context.block.bytes[3] = 0x80;
    Sha1SetLength(&context, 24);
    before = context;
    sha1_block(&context);
    CheckWords("sha1_block of \"abc\"", (const fx32 *)context.state, (const fx32 *)abc, 5);
    // Only the state changes
    Check(memcmp(&context.block, &before.block, sizeof(before.block)) == 0 && context.blockPos == before.blockPos &&
              context.blockCount == before.blockCount && context.blockCountHigh == before.blockCountHigh,
          "sha1_block changes only the state");

    // 56 bytes take two blocks, the second only the length
    Sha1Start(&context);
    Sha1SetBlock(&context, twoBlocks, 56);
    context.block.bytes[56] = 0x80;
    sha1_block(&context);
    Sha1SetBlock(&context, "", 0);
    Sha1SetLength(&context, 448);
    sha1_block(&context);
    CheckWords("sha1_block of two blocks", (const fx32 *)context.state, (const fx32 *)long448, 5);
}

// FNV-1a over size bytes, to compare a state with one recorded from the ROM
static u32 HashBytes(const void *data, u32 size) {
    const u8 *bytes = data;
    u32 hash = 2166136261;
    u32 i;

    for (i = 0; i < size; i++) {
        hash = (hash ^ bytes[i]) * 16777619;
    }
    return hash;
}

// Sets up a context with RC4's key schedule, for the standard vectors
static void Rc4SetKey(CRYPTORC4Context *context, const char *key) {
    u32 length = (u32)strlen(key);
    u8 j = 0;
    int i;

    context->i = 0;
    context->j = 0;
    for (i = 0; i < 256; i++) {
        context->s[i] = (u8)i;
    }
    for (i = 0; i < 256; i++) {
        u8 t = context->s[i];

        j = (u8)(j + t + (u8)key[i % length]);
        context->s[i] = context->s[j];
        context->s[j] = t;
    }
}

static void TestRc4(void) {
    static const u8 plaintext[] = { 0xbb, 0xf3, 0x16, 0xe8, 0xd9, 0x40, 0xaf, 0x0a, 0xd3 };
    static const u8 pedia[] = { 0x10, 0x21, 0xbf, 0x04, 0x20 };
    static const u8 attack[] = { 0x45, 0xa0, 0x1f, 0x64, 0x5f, 0xc3, 0x5b, 0x38, 0x35, 0x52, 0x54, 0x4b, 0x9b, 0xf5 };
    CRYPTORC4Context context;
    CRYPTORC4Context before;
    u8 out[64];
    u8 keystream[32];
    u8 buffer[33];
    u8 expected[33];
    int n;

    Rc4SetKey(&context, "Key");
    CRYPTO_RC4Encrypt(&context, "Plaintext", 9, out);
    CheckBytes("CRYPTO_RC4Encrypt, Key and Plaintext", out, plaintext, sizeof(plaintext));
    Check(context.i == 9, "CRYPTO_RC4Encrypt advances i");

    Rc4SetKey(&context, "Wiki");
    CRYPTO_RC4Encrypt(&context, "pedia", 5, out);
    CheckBytes("CRYPTO_RC4Encrypt, Wiki and pedia", out, pedia, sizeof(pedia));

    // In two calls, the state carries over
    Rc4SetKey(&context, "Secret");
    CRYPTO_RC4Encrypt(&context, "Attack", 6, out);
    CRYPTO_RC4Encrypt(&context, " at dawn", 8, &out[6]);
    CheckBytes("CRYPTO_RC4Encrypt, Secret and Attack at dawn in two calls", out, attack, sizeof(attack));

    // In place
    Rc4SetKey(&context, "Secret");
    memcpy(out, "Attack at dawn", 14);
    CRYPTO_RC4Encrypt(&context, out, 14, out);
    CheckBytes("CRYPTO_RC4Encrypt in place", out, attack, sizeof(attack));

    // A length of 0, or above 0x80000000, changes nothing
    memset(out, 0x77, sizeof(out));
    before = context;
    CRYPTO_RC4Encrypt(&context, "Attack at dawn", 0, out);
    CRYPTO_RC4Encrypt(&context, "Attack at dawn", 0xffffffff, out);
    CRYPTO_RC4Encrypt(&context, "Attack at dawn", 0x80000001, out);
    Check(memcmp(&context, &before, sizeof(context)) == 0 && out[0] == 0x77, "CRYPTO_RC4Encrypt of nothing");

    // With the output one byte after the input, each byte read is the one written before it, so byte n + 1 ends as
    // the first byte with the first n + 1 bytes of the key stream
    Rc4SetKey(&context, "Key");
    memset(keystream, 0, sizeof(keystream));
    CRYPTO_RC4Encrypt(&context, keystream, sizeof(keystream), keystream);
    for (n = 0; n < 33; n++) {
        buffer[n] = (u8)(0x31 * n + 7);
    }
    expected[0] = buffer[0];
    for (n = 0; n < 32; n++) {
        expected[n + 1] = expected[n] ^ keystream[n];
    }
    Rc4SetKey(&context, "Key");
    CRYPTO_RC4Encrypt(&context, buffer, 32, &buffer[1]);
    CheckBytes("CRYPTO_RC4Encrypt one byte ahead", buffer, expected, sizeof(expected));

    // With the output over the state, two bytes after s[0], each byte lands on the s[i] that the next pass has already
    // loaded, which it uses as loaded. The context after, as the ROM's code leaves it: i, j and a hash of all of it
    memset(&context, 0, sizeof(context));
    Rc4SetKey(&context, "Key");
    CRYPTO_RC4Encrypt(&context, "Plaintext", 9, &context.s[2]);
    Check(context.i == 9 && context.j == 222 && HashBytes(&context, sizeof(context)) == 0x129b4604,
          "CRYPTO_RC4Encrypt into its own state");
}

// The comparisons a sort made, as the indices of the two elements: the first 64, and a hash of all of them (FNV-1a
// over the indices)
static const u8 *sSortBase;
static u32 sSortWidth;
static u8 sCalls[64][2];
static int sCallCount;
static u32 sCallHash;

static void RecordCall(void *a, void *b) {
    u32 first = (u32)(((u8 *)a - sSortBase) / sSortWidth);
    u32 second = (u32)(((u8 *)b - sSortBase) / sSortWidth);

    if (sCallCount < 64) {
        sCalls[sCallCount][0] = (u8)first;
        sCalls[sCallCount][1] = (u8)second;
    }
    sCallCount++;
    sCallHash = (sCallHash ^ first) * 16777619;
    sCallHash = (sCallHash ^ second) * 16777619;
}

static s32 CompareWordKeys(void *a, void *b) {
    u32 x;
    u32 y;

    RecordCall(a, b);
    memcpy(&x, a, sizeof(x));
    memcpy(&y, b, sizeof(y));
    return x < y ? -0x10000 : x > y ? 0x10000 : 0;
}

static s32 CompareByteKeys(void *a, void *b) {
    RecordCall(a, b);
    return *(u8 *)a - *(u8 *)b;
}

static s32 CompareNever(void *a, void *b) {
    RecordCall(a, b);
    return -1;
}

static void StartSort(const void *base, u32 width) {
    sSortBase = base;
    sSortWidth = width;
    sCallCount = 0;
    sCallHash = 2166136261;
}

static void CheckCalls(const char *what, const u8 (*expected)[2], int count) {
    int i;

    if (sCallCount != count) {
        printf("FAIL %s: %d comparisons, expected %d\n", what, sCallCount, count);
        sFailures++;
        return;
    }
    for (i = 0; i < count; i++) {
        if (sCalls[i][0] != expected[i][0] || sCalls[i][1] != expected[i][1]) {
            printf("FAIL %s: comparison %d is (%d, %d), expected (%d, %d)\n", what, i, sCalls[i][0], sCalls[i][1],
                   expected[i][0], expected[i][1]);
            sFailures++;
            return;
        }
    }
}

static void TestQSort(void) {
    // Recorded from the ROM's MATH_QSort: twelve 8-byte elements (key, index) sorted by key, and ten 3-byte elements
    // (key, index, 0xa0 + index) at an odd address, a byte at a time
    static const u32 keys[12] = { 3, 1, 3, 2, 1, 3, 2, 1, 0, 3, 2, 2 };
    static const u32 sortedIndices[12] = { 8, 7, 4, 1, 6, 11, 10, 3, 9, 5, 0, 2 };
    static const u8 calls[36][2] = {
        { 1, 0 }, { 2, 0 }, { 11, 0 }, { 3, 0 },   { 4, 0 }, { 5, 0 }, { 10, 0 }, { 6, 0 }, { 7, 0 },
        { 8, 0 }, { 9, 0 }, { 9, 0 },  { 10, 11 }, { 1, 0 }, { 8, 0 }, { 2, 0 },  { 7, 0 }, { 3, 0 },
        { 6, 0 }, { 5, 0 }, { 4, 0 },  { 3, 0 },   { 2, 0 }, { 0, 1 }, { 4, 3 },  { 8, 3 }, { 5, 3 },
        { 7, 3 }, { 6, 3 }, { 6, 3 },  { 7, 8 },   { 4, 3 }, { 5, 3 }, { 4, 3 },  { 3, 3 }, { 4, 5 },
    };
    static const u8 keys3[10] = { 5, 0, 5, 2, 2, 7, 1, 5, 0, 2 };
    static const u8 sorted3[30] = { 0, 8, 0xa8, 0, 1, 0xa1, 1, 6, 0xa6, 2, 3, 0xa3, 2, 9, 0xa9,
                                    2, 4, 0xa4, 5, 2, 0xa2, 5, 7, 0xa7, 5, 0, 0xa0, 7, 5, 0xa5 };
    static const u8 calls3[25][2] = {
        { 1, 0 }, { 2, 0 }, { 9, 0 }, { 8, 0 }, { 3, 0 }, { 7, 0 }, { 6, 0 }, { 5, 0 }, { 4, 0 },
        { 3, 0 }, { 2, 0 }, { 0, 1 }, { 4, 3 }, { 5, 3 }, { 9, 3 }, { 6, 3 }, { 8, 3 }, { 7, 3 },
        { 8, 3 }, { 7, 3 }, { 8, 9 }, { 4, 3 }, { 6, 3 }, { 5, 3 }, { 5, 6 },
    };
    static const u8 headAtZero[1][2] = { { 0, 0 } };
    // Recorded from the ROM's MATH_QSort too: the order of 64 elements (key, index) with keys from 0 to 3, and the
    // number and hash of the comparisons it made, which show the order in which it sorts the parts
    static const u8 sorted64[64] = { 36, 18, 37, 32, 41, 20, 23, 28, 22, 43, 0,  42, 54, 2,  57, 47,
                                     45, 10, 1,  29, 61, 30, 52, 40, 27, 49, 44, 31, 11, 8,  13, 7,
                                     6,  3,  4,  15, 9,  17, 24, 25, 21, 5,  51, 55, 56, 58, 19, 59,
                                     50, 14, 48, 35, 38, 62, 63, 39, 60, 12, 33, 16, 46, 53, 34, 26 };
    u32 elements64[64][2];
    u32 elements[12][2];
    u32 sortedWords[12][2];
    u8 bytes[32];
    u32 stack[2 * 4 + 2];
    u32 values[100];
    u32 seed;
    u32 i;

    for (i = 0; i < 12; i++) {
        elements[i][0] = keys[i];
        elements[i][1] = i;
        sortedWords[i][0] = keys[sortedIndices[i]];
        sortedWords[i][1] = sortedIndices[i];
    }
    StartSort(elements, 8);
    MATH_QSort(elements, 12, 8, CompareWordKeys, NULL);
    CheckWords("MATH_QSort of 8-byte elements", (const fx32 *)elements, (const fx32 *)sortedWords, 24);
    CheckCalls("MATH_QSort of 8-byte elements", calls, 36);

    // The same with a stack buffer of the size the SDK asks for, 8 bytes per bit of the count (4 here): the ranges
    // stay in it and nothing is written after it
    for (i = 0; i < 12; i++) {
        elements[i][0] = keys[i];
        elements[i][1] = i;
    }
    memset(stack, 0xee, sizeof(stack));
    StartSort(elements, 8);
    MATH_QSort(elements, 12, 8, CompareWordKeys, stack);
    CheckWords("MATH_QSort with a stack buffer", (const fx32 *)elements, (const fx32 *)sortedWords, 24);
    CheckCalls("MATH_QSort with a stack buffer", calls, 36);
    Check(stack[8] == 0xeeeeeeee && stack[9] == 0xeeeeeeee, "MATH_QSort writes no further than its stack buffer");

    memset(bytes, 0x55, sizeof(bytes));
    for (i = 0; i < 10; i++) {
        bytes[1 + 3 * i] = keys3[i];
        bytes[2 + 3 * i] = (u8)i;
        bytes[3 + 3 * i] = (u8)(0xa0 + i);
    }
    StartSort(&bytes[1], 3);
    MATH_QSort(&bytes[1], 10, 3, CompareByteKeys, NULL);
    CheckBytes("MATH_QSort of 3-byte elements", &bytes[1], sorted3, sizeof(sorted3));
    CheckCalls("MATH_QSort of 3-byte elements", calls3, 25);
    Check(bytes[0] == 0x55 && bytes[31] == 0x55, "MATH_QSort writes only the array");

    // Counts of 0x80000000 or more are negative to the SDK and sort nothing; with a width of 0, the one range is a
    // pair of elements, compared once
    StartSort(elements, 8);
    MATH_QSort(elements, 0x80000000, 8, CompareWordKeys, NULL);
    MATH_QSort(elements, 1, 8, CompareWordKeys, NULL);
    CheckCalls("MATH_QSort of a negative count", headAtZero, 0);
    StartSort(elements, 1);
    MATH_QSort(elements, 5, 0, CompareNever, NULL);
    CheckCalls("MATH_QSort with a width of 0", headAtZero, 1);

    // The keys of the 64 elements come from a linear congruential generator
    seed = 2024;
    for (i = 0; i < 64; i++) {
        seed = seed * 1103515245 + 12345;
        elements64[i][0] = (seed >> 16) % 4;
        elements64[i][1] = i;
    }
    StartSort(elements64, 8);
    MATH_QSort(elements64, 64, 8, CompareWordKeys, NULL);
    for (i = 0; i < 64; i++) {
        if (elements64[i][1] != sorted64[i]) {
            printf("FAIL MATH_QSort of 64 elements: element %u is %u, expected %u\n", i, elements64[i][1], sorted64[i]);
            sFailures++;
            break;
        }
    }
    Check(sCallCount == 303 && sCallHash == 0xd62ccd8d, "MATH_QSort of 64 elements makes the ROM's comparisons");

    // A hundred values from the same generator end in ascending order
    seed = 12345;
    for (i = 0; i < NELEMS(values); i++) {
        seed = seed * 1103515245 + 12345;
        values[i] = seed >> 8;
    }
    StartSort(values, 4);
    MATH_QSort(values, NELEMS(values), 4, CompareWordKeys, NULL);
    for (i = 1; i < NELEMS(values); i++) {
        Check(values[i - 1] <= values[i], "MATH_QSort of 100 values");
    }
}

int main(void) {
    TestMatrixStores();
    TestMatrixCopies();
    TestCountLeadingZeros();
    TestSha1Block();
    TestRc4();
    TestQSort();
    if (sFailures != 0) {
        printf("%d checks failed\n", sFailures);
        return 1;
    }
    printf("all checks passed\n");
    return 0;
}
