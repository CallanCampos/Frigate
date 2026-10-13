#include "vct_internal.h"

// Adaptive differential PCM for the voice chat, at 4, 3 and 2 bits a sample. Each codec keeps a predicted sample and
// an index into the table of step sizes, and the code of a sample says how far from the prediction the sample was
// and which way, to the precision of the step.

static const s8 sIndexAdjust2[4] = {-1, 1, -1, 1};
static const s8 sIndexAdjust3[8] = {-1, -1, 1, 2, -1, -1, 1, 2};
static const s8 sIndexAdjust4[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};
static const s16 sVctAdpcmStepTable[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,    28,    31,
    34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,   130,   143,
    157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,   494,   544,   598,   658,
    724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,  2272,  2499,  2749,  3024,
    3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,  9493,  10442, 11487, 12635, 13899,
    15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
};

static void DecodeAdpcm4(u8 *src, s16 *dst, u32 count, VCTAdpcmState *state);
static void EncodeAdpcm4(u8 *dst, s16 *src, u32 count, VCTAdpcmState *state);
static void DecodeAdpcm3(u8 *src, s16 *dst, u32 count, VCTAdpcmState *state);
static void EncodeAdpcm3(u8 *dst, s16 *src, u32 count, VCTAdpcmState *state);
static void DecodeAdpcm2(u8 *src, s16 *dst, u32 count, VCTAdpcmState *state);
static void EncodeAdpcm2(u8 *dst, s16 *src, u32 count, VCTAdpcmState *state);

// Moves the step index by the adjustment of a code, within the table
#define ADVANCE_STEP_INDEX(index, adjust) \
    do {                                  \
        (index) += (adjust);              \
        if ((index) < 0) {                \
            (index) = 0;                  \
        } else if ((index) > 88) {        \
            (index) = 88;                 \
        }                                 \
    } while (0)

static inline s16 DecodeSample4(VCTAdpcmState *state, int code)
{
    int predictor = state->predictor;
    int index = state->stepIndex;
    int step = sVctAdpcmStepTable[index];
    int diff = step >> 3;
    s16 sample;

    if (code & 4) {
        diff += step;
    }
    if (code & 2) {
        diff += step >> 1;
    }
    if (code & 1) {
        diff += step >> 2;
    }
    if (code & 8) {
        predictor -= diff;
        if (predictor < -0x8000) {
            predictor = -0x8000;
        }
    } else {
        predictor += diff;
        if (predictor > 0x7fff) {
            predictor = 0x7fff;
        }
    }
    index += sIndexAdjust4[code];
    if (index < 0) {
        index = 0;
    } else if (index > 88) {
        index = 88;
    }
    state->stepIndex = index;
    sample = predictor;
    state->predictor = sample;
    return sample;
}

static inline s16 DecodeSample3(VCTAdpcmState *state, int code)
{
    int index = state->stepIndex;
    int step = sVctAdpcmStepTable[index];
    int diff = step >> 2;
    int predictor = state->predictor;
    s16 sample;

    if (code & 2) {
        diff += step;
    }
    if (code & 1) {
        diff += step >> 1;
    }
    if (code & 4) {
        predictor -= diff;
        if (predictor < -0x8000) {
            predictor = -0x8000;
        }
    } else {
        predictor += diff;
        if (predictor > 0x7fff) {
            predictor = 0x7fff;
        }
    }
    ADVANCE_STEP_INDEX(index, sIndexAdjust3[code & 7]);
    state->stepIndex = index;
    sample = predictor;
    state->predictor = sample;
    return sample;
}

static inline s16 DecodeSample2(VCTAdpcmState *state, int code)
{
    int index = state->stepIndex;
    int diff = sVctAdpcmStepTable[index];
    int predictor = state->predictor;
    s16 sample;

    if (!(code & 1)) {
        diff = 0;
    }
    if (code & 2) {
        predictor -= diff;
        if (predictor < -0x8000) {
            predictor = -0x8000;
        }
    } else {
        predictor += diff;
        if (predictor > 0x7fff) {
            predictor = 0x7fff;
        }
    }
    ADVANCE_STEP_INDEX(index, sIndexAdjust2[code]);
    state->stepIndex = index;
    sample = predictor;
    state->predictor = sample;
    return sample;
}

static inline u8 EncodeSample4(VCTAdpcmState *state, s16 input)
{
    int index = state->stepIndex;
    int step = sVctAdpcmStepTable[index];
    int predictor = state->predictor;
    int diff = input - predictor;
    int vpdiff;
    u8 code;

    if (diff < 0) {
        code = 8;
        diff = -diff;
    } else {
        code = 0;
    }
    if (diff >= step) {
        code |= 4;
        diff -= step;
    }
    if (diff >= step >> 1) {
        code |= 2;
        diff -= step >> 1;
    }
    if (diff >= step >> 2) {
        code |= 1;
    }
    vpdiff = step >> 3;
    if (code & 4) {
        vpdiff += step;
    }
    if (code & 2) {
        vpdiff += step >> 1;
    }
    if (code & 1) {
        vpdiff += step >> 2;
    }
    if (code & 8) {
        vpdiff = -vpdiff;
    }
    predictor += vpdiff;
    if (predictor < -0x8000) {
        predictor = -0x8000;
    }
    if (predictor > 0x7fff) {
        predictor = 0x7fff;
    }
    ADVANCE_STEP_INDEX(index, sIndexAdjust4[code]);
    state->predictor = predictor;
    state->stepIndex = index;
    return code;
}

static inline u8 EncodeSample3(VCTAdpcmState *state, s16 input)
{
    int index = state->stepIndex;
    int step = sVctAdpcmStepTable[index];
    int predictor = state->predictor;
    int diff = input - predictor;
    int vpdiff;
    u8 code;

    if (diff < 0) {
        code = 4;
        diff = -diff;
    } else {
        code = 0;
    }
    if (diff >= step) {
        code |= 2;
        diff -= step;
    }
    if (diff >= step >> 1) {
        code |= 1;
    }
    vpdiff = step >> 2;
    if (code & 2) {
        vpdiff += step;
    }
    if (code & 1) {
        vpdiff += step >> 1;
    }
    if (code & 4) {
        vpdiff = -vpdiff;
    }
    predictor += vpdiff;
    if (predictor < -0x8000) {
        predictor = -0x8000;
    }
    if (predictor > 0x7fff) {
        predictor = 0x7fff;
    }
    ADVANCE_STEP_INDEX(index, sIndexAdjust3[code]);
    state->predictor = predictor;
    state->stepIndex = index;
    return code;
}

static inline u8 EncodeSample2(VCTAdpcmState *state, s16 input)
{
    int index = state->stepIndex;
    int step = sVctAdpcmStepTable[index];
    int predictor = state->predictor;
    int diff = input - predictor;
    int vpdiff;
    u8 code;

    if (diff < 0) {
        code = 2;
        diff = -diff;
    } else {
        code = 0;
    }
    if (diff >= step) {
        code |= 1;
    }
    vpdiff = step;
    if (!(code & 1)) {
        vpdiff = 0;
    }
    if (code & 2) {
        vpdiff = -vpdiff;
    }
    predictor += vpdiff;
    if (predictor < -0x8000) {
        predictor = -0x8000;
    }
    if (predictor > 0x7fff) {
        predictor = 0x7fff;
    }
    ADVANCE_STEP_INDEX(index, sIndexAdjust2[code]);
    state->predictor = predictor;
    state->stepIndex = index;
    return code;
}

// Decodes count bytes of 4-bit codes, two samples to a byte, low nibble first
static void DecodeAdpcm4(u8 *src, s16 *dst, u32 count, VCTAdpcmState *state)
{
    u32 i;

    for (i = 0; i < count; i++) {
        *dst++ = DecodeSample4(state, *src & 0xf);
        *dst++ = DecodeSample4(state, (*src >> 4) & 0xf);
        src++;
    }
}

// Encodes count samples to 4-bit codes
static void EncodeAdpcm4(u8 *dst, s16 *src, u32 count, VCTAdpcmState *state)
{
    u32 i;

    for (i = 0; i < count / 2; i++) {
        u8 low = EncodeSample4(state, src[0]);
        u8 high = EncodeSample4(state, src[1]);

        *dst++ = low | (u8)(high << 4);
        src += 2;
    }
}

// Decodes groups of three bytes of 3-bit codes, eight samples to a group, the first sample in the top bits
static void DecodeAdpcm3(u8 *src, s16 *dst, u32 count, VCTAdpcmState *state)
{
    u32 i;

    for (i = 0; i < count / 3; i++) {
        dst[0] = DecodeSample3(state, src[0] >> 5);
        dst[1] = DecodeSample3(state, (src[0] >> 2) & 7);
        dst[2] = DecodeSample3(state, ((src[0] << 1) & 7) | (src[1] >> 7));
        dst[3] = DecodeSample3(state, (src[1] >> 4) & 7);
        dst[4] = DecodeSample3(state, (src[1] >> 1) & 7);
        dst[5] = DecodeSample3(state, ((src[1] << 2) & 7) | (src[2] >> 6));
        dst[6] = DecodeSample3(state, (src[2] >> 3) & 7);
        dst[7] = DecodeSample3(state, src[2] & 7);
        src += 3;
        dst += 8;
    }
}

// Encodes count samples to 3-bit codes
static void EncodeAdpcm3(u8 *dst, s16 *src, u32 count, VCTAdpcmState *state)
{
    u32 i;

    for (i = 0; i < count / 8; i++) {
        u8 c0 = EncodeSample3(state, src[0]);
        u8 c1 = EncodeSample3(state, src[1]);
        u8 c2 = EncodeSample3(state, src[2]);
        u8 c3, c4, c5, c6, c7;

        dst[0] = (c0 << 5) | (c1 << 2) | (c2 >> 1);
        c3 = EncodeSample3(state, src[3]);
        c4 = EncodeSample3(state, src[4]);
        c5 = EncodeSample3(state, src[5]);
        dst[1] = (c2 << 7) | (c3 << 4) | (c4 << 1) | (c5 >> 2);
        c6 = EncodeSample3(state, src[6]);
        c7 = EncodeSample3(state, src[7]);
        dst[2] = (c5 << 6) | (c6 << 3) | c7;
        src += 8;
        dst += 3;
    }
}

// Decodes count bytes of 2-bit codes, four samples to a byte, the first in the low bits
static void DecodeAdpcm2(u8 *src, s16 *dst, u32 count, VCTAdpcmState *state)
{
    u32 i;

    for (i = 0; i < count; i++) {
        dst[0] = DecodeSample2(state, src[0] & 3);
        dst[1] = DecodeSample2(state, (src[0] >> 2) & 3);
        dst[2] = DecodeSample2(state, (src[0] >> 4) & 3);
        dst[3] = DecodeSample2(state, (src[0] >> 6) & 3);
        src++;
        dst += 4;
    }
}

// Encodes count samples to 2-bit codes
static void EncodeAdpcm2(u8 *dst, s16 *src, u32 count, VCTAdpcmState *state)
{
    u32 i;

    for (i = 0; i < count / 4; i++) {
        u8 c0 = EncodeSample2(state, src[0]);
        u8 c1 = EncodeSample2(state, src[1]);
        u8 c2 = EncodeSample2(state, src[2]);
        u8 c3 = EncodeSample2(state, src[3]);

        *dst++ = c0 | (c1 << 2) | (c2 << 4) | (c3 << 6);
        src += 4;
    }
}

// Decodes a packet of ADPCM: the state it starts from, then bytes of codes at the given number of bits a sample
void VCT_DecodeAdpcm(u8 *packet, s16 *dst, int size, int bits)
{
    VCTAdpcmState state;

    state.predictor = ((VCTAdpcmState *)packet)->predictor;
    state.stepIndex = ((VCTAdpcmState *)packet)->stepIndex;
    switch (bits) {
    case 2:
        DecodeAdpcm2(packet + 4, dst, size - 4, &state);
        break;
    case 3:
        DecodeAdpcm3(packet + 4, dst, size - 4, &state);
        break;
    case 4:
        DecodeAdpcm4(packet + 4, dst, size - 4, &state);
        break;
    }
}

// Encodes count samples to a packet of ADPCM, which starts with the state before the first sample
void VCT_EncodeAdpcm(u8 *packet, s16 *src, int count, VCTAdpcmState *state, int bits)
{
    VCTAdpcmState *header = (VCTAdpcmState *)packet;

    header->predictor = state->predictor;
    header->stepIndex = state->stepIndex;
    header->pad = 0;
    switch (bits) {
    case 2:
        EncodeAdpcm2(packet + 4, src, count, state);
        break;
    case 3:
        EncodeAdpcm3(packet + 4, src, count, state);
        break;
    case 4:
        EncodeAdpcm4(packet + 4, src, count, state);
        break;
    }
}
