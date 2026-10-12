#ifndef POKEBW2_NITRO_MATH_H
#define POKEBW2_NITRO_MATH_H

#include "types.h"

#define MATH_ABS(a) (((a) < 0) ? -(a) : (a))
#define MATH_CLAMP(x, low, high) (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define MATH_MAX(a, b) (((a) >= (b)) ? (a) : (b))
#define MATH_MIN(a, b) (((a) <= (b)) ? (a) : (b))

// NitroSDK's linear congruential random numbers, which are inline in the SDK

typedef struct {
    u64 x;
    u64 mul;
    u64 add;
} MATHRandContext32;

static inline void MATH_InitRand32(MATHRandContext32 *context, u64 seed) {
    context->x = seed;
    context->mul = (1566083941LL << 32) + 1812433253LL;
    context->add = 2531011;
}

// A random number below max, or any 32-bit number when max is 0
static inline u32 MATH_Rand32(MATHRandContext32 *context, u32 max) {
    context->x = context->mul * context->x + context->add;
    if (max == 0) {
        return (u32)(context->x >> 32);
    } else {
        return (u32)(((context->x >> 32) * max) >> 32);
    }
}

// CRC-16/CCITT through a table, and SHA-1
typedef struct {
    u16 table[256];
} MATHCRC16Table;

typedef struct {
    u8 data[0x60];
} MATHSHA1Context;

#define MATH_SHA1_DIGEST_SIZE 20

// The number of set bits, NitroSDK's MATH_CountPopulation (swan's name)
u8 countOneBits(u32 x);

// The number of leading zero bits: NitroSDK's MATH_CountLeadingZerosFunc (swan's name), which MATH_CountLeadingZeros
// calls from Thumb code, where CLZ doesn't exist
u32 countLeadingZeros(u32 x);

static inline u32 MATH_CountLeadingZeros(u32 x) {
    return countLeadingZeros(x);
}

// Quicksort of num elements of width bytes; stackBuffer may be NULL
typedef s32 (*MATHCompareFunc)(void *a, void *b);
void MATH_QSort(void *head, u32 num, u32 width, MATHCompareFunc comp, void *stackBuffer);

void MATH_CRC16CCITTInitTable(MATHCRC16Table *table, u16 poly);
u16 MATH_CalcCRC16CCITT(const MATHCRC16Table *table, const void *data, u32 size);
void MATH_SHA1Init(MATHSHA1Context *context);
void MATH_SHA1Update(MATHSHA1Context *context, const void *data, u32 size);
void MATH_SHA1GetHash(MATHSHA1Context *context, void *digest);
// The SHA-1 digest of size bytes, through the three above (MATH_CalcSHA1's code)
void func_02077720(u8 *digest, const void *data, u32 size);

#endif // POKEBW2_NITRO_MATH_H
