#include "vct_adpcm.h"

// Adaptive differential PCM for the voice chat, at 4, 3 and 2 bits a sample. Each codec keeps a predicted sample and
// an index into the table of step sizes, and the code of a sample says how far from the prediction the sample was
// and which way, to the precision of the step.

static const s8 sIndexAdjust2[4] = { -1, 1, -1, 1 };
static const s8 sIndexAdjust3[8] = { -1, -1, 1, 2, -1, -1, 1, 2 };
static const s8 sIndexAdjust4[16] = { -1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8 };
static const s16 sVctAdpcmStepTable[89] = {
    7,    8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,    28,
    31,   34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
    130,  143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,   494,
    544,  598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272, 2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,
    9493, 10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767,
};

static void vct_decode_adpcm_32(u8 *src, s16 *dst, u32 count, AdpcmState *state);
static void vct_encode_adpcm_32(u8 *dst, s16 *src, u32 count, AdpcmState *state);
static void vct_decode_adpcm_24(u8 *src, s16 *dst, u32 count, AdpcmState *state);
static void vct_encode_adpcm_24(u8 *dst, s16 *src, u32 count, AdpcmState *state);
static void vct_decode_adpcm_16(u8 *src, s16 *dst, u32 count, AdpcmState *state);
static void vct_encode_adpcm_16(u8 *dst, s16 *src, u32 count, AdpcmState *state);

// Moves the step index by the adjustment of a code, within the table
#define ADVANCE_STEP_INDEX(index, adjust)                                                                              \
    do {                                                                                                               \
        (index) += (adjust);                                                                                           \
        if ((index) < 0) {                                                                                             \
            (index) = 0;                                                                                               \
        } else if ((index) > 88) {                                                                                     \
            (index) = 88;                                                                                              \
        }                                                                                                              \
    } while (0)

static inline s16 DecodeAdpcm4bit(AdpcmState *state, int code) {
    int predictor = state->prevSample;
    int index = state->prevIndex;
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
    state->prevIndex = index;
    sample = predictor;
    state->prevSample = sample;
    return sample;
}

static inline s16 DecodeAdpcm3bit(AdpcmState *state, int code) {
    int predictor = state->prevSample;
    int index = state->prevIndex;
    int step = sVctAdpcmStepTable[index];
    int diff = step >> 2;
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
    ADVANCE_STEP_INDEX(index, sIndexAdjust3[code]);
    state->prevIndex = index;
    sample = predictor;
    state->prevSample = sample;
    return sample;
}

static inline s16 DecodeAdpcm2bit(AdpcmState *state, int code) {
    int predictor = state->prevSample;
    int index = state->prevIndex;
    int diff = sVctAdpcmStepTable[index];
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
    state->prevIndex = index;
    sample = predictor;
    state->prevSample = sample;
    return sample;
}

static inline u8 EncodeAdpcm4bit(AdpcmState *state, s16 input) {
    // The order of these declarations sets the locals' registers
    int index = state->prevIndex;
    int predictor;
    int code;
    int vpdiff;
    int step;
    int diff = input - state->prevSample;

    if (diff < 0) {
        code = 8;
        diff = -diff;
    } else {
        code = 0;
    }
    step = sVctAdpcmStepTable[index];
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
    predictor = state->prevSample + vpdiff;
    if (predictor < -0x8000) {
        predictor = -0x8000;
    }
    if (predictor > 0x7fff) {
        predictor = 0x7fff;
    }
    ADVANCE_STEP_INDEX(index, sIndexAdjust4[code]);
    state->prevSample = predictor;
    state->prevIndex = index;
    return code;
}

static inline u8 EncodeAdpcm3Bit(AdpcmState *state, s16 input) {
    // The order of these declarations sets the locals' registers
    int predictor = state->prevSample;
    int vpdiff;
    int step;
    int index = state->prevIndex;
    int diff = input - predictor;
    int code;
    int sample;

    if (diff < 0) {
        code = 4;
        diff = -diff;
    } else {
        code = 0;
    }
    step = sVctAdpcmStepTable[index];
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
    sample = predictor + vpdiff;
    if (sample < -0x8000) {
        sample = -0x8000;
    }
    if (sample > 0x7fff) {
        sample = 0x7fff;
    }
    ADVANCE_STEP_INDEX(index, sIndexAdjust3[code]);
    state->prevSample = sample;
    state->prevIndex = index;
    return code;
}

static inline u8 EncodeAdpcm2bit(AdpcmState *state, s16 input) {
    // The order of these declarations sets the locals' registers, as in EncodeAdpcm3Bit
    int predictor = state->prevSample;
    int vpdiff;
    int step;
    int index = state->prevIndex;
    int diff = input - predictor;
    int code;
    int sample;

    if (diff < 0) {
        code = 2;
        diff = -diff;
    } else {
        code = 0;
    }
    step = sVctAdpcmStepTable[index];
    if (diff >= step) {
        code |= 1;
    }
    if (code & 1) {
        vpdiff = step;
    } else {
        vpdiff = 0;
    }
    if (code & 2) {
        vpdiff = -vpdiff;
    }
    sample = predictor + vpdiff;
    if (sample < -0x8000) {
        sample = -0x8000;
    }
    if (sample > 0x7fff) {
        sample = 0x7fff;
    }
    ADVANCE_STEP_INDEX(index, sIndexAdjust2[code]);
    state->prevSample = sample;
    state->prevIndex = index;
    return code;
}

// Decodes count bytes of 4-bit codes, two samples to a byte, low nibble first
static void vct_decode_adpcm_32(u8 *src, s16 *dst, u32 count, AdpcmState *state) {
    u32 i;

    for (i = 0; i < count; i++) {
        *dst++ = DecodeAdpcm4bit(state, *src & 0xf);
        *dst++ = DecodeAdpcm4bit(state, (*src >> 4) & 0xf);
        src++;
    }
}

// Encodes count samples to 4-bit codes
static void vct_encode_adpcm_32(u8 *dst, s16 *src, u32 count, AdpcmState *state) {
    u32 i;

    for (i = 0; i < count / 2; i++) {
        u8 low = EncodeAdpcm4bit(state, *src++);
        u8 high = EncodeAdpcm4bit(state, *src++);

        *dst++ = low | (u8)(high << 4);
    }
}

// Decodes groups of three bytes of 3-bit codes, eight samples to a group, the first sample in the top bits
static void vct_decode_adpcm_24(u8 *src, s16 *dst, u32 count, AdpcmState *state) {
    u32 i;

    for (i = 0; i < count / 3; i++) {
        u8 b0 = src[0];
        u8 b1 = src[1];
        u8 b2 = src[2];

        dst[0] = DecodeAdpcm3bit(state, b0 >> 5);
        dst[1] = DecodeAdpcm3bit(state, (b0 >> 2) & 7);
        dst[2] = DecodeAdpcm3bit(state, ((b0 << 1) & 7) | (b1 >> 7));
        dst[3] = DecodeAdpcm3bit(state, (b1 >> 4) & 7);
        dst[4] = DecodeAdpcm3bit(state, (b1 >> 1) & 7);
        dst[5] = DecodeAdpcm3bit(state, ((b1 << 2) & 7) | (b2 >> 6));
        dst[6] = DecodeAdpcm3bit(state, (b2 >> 3) & 7);
        dst[7] = DecodeAdpcm3bit(state, b2 & 7);
        src += 3;
        dst += 8;
    }
}

// Encodes count samples to 3-bit codes, eight samples to a group of three bytes, the first sample in the top bits. The
// codes of the third and sixth samples straddle two bytes.
static void vct_encode_adpcm_24(u8 *dst, s16 *src, u32 count, AdpcmState *state) {
    u32 i;

    for (i = 0; i < count / 8; i++) {
        u8 byte;
        u8 c2, c5;

        byte = EncodeAdpcm3Bit(state, *src++) << 5;
        byte |= EncodeAdpcm3Bit(state, *src++) << 2;
        c2 = EncodeAdpcm3Bit(state, *src++);
        *dst++ = byte | (c2 >> 1);
        byte = EncodeAdpcm3Bit(state, *src++) << 4;
        byte |= EncodeAdpcm3Bit(state, *src++) << 1;
        c5 = EncodeAdpcm3Bit(state, *src++);
        *dst++ = byte | (c2 << 7) | (c5 >> 2);
        byte = EncodeAdpcm3Bit(state, *src++) << 3;
        byte |= EncodeAdpcm3Bit(state, *src++);
        *dst++ = byte | (c5 << 6);
    }
}

// Decodes count bytes of 2-bit codes, four samples to a byte, the first in the low bits
static void vct_decode_adpcm_16(u8 *src, s16 *dst, u32 count, AdpcmState *state) {
    u32 i;

    for (i = 0; i < count; i++) {
        *dst++ = DecodeAdpcm2bit(state, *src & 3);
        *dst++ = DecodeAdpcm2bit(state, (*src >> 2) & 3);
        *dst++ = DecodeAdpcm2bit(state, (*src >> 4) & 3);
        *dst++ = DecodeAdpcm2bit(state, (*src >> 6) & 3);
        src++;
    }
}

// Encodes count samples to 2-bit codes
static void vct_encode_adpcm_16(u8 *dst, s16 *src, u32 count, AdpcmState *state) {
    u32 i;

    for (i = 0; i < count / 4; i++) {
        u8 byte = EncodeAdpcm2bit(state, *src++);

        byte |= EncodeAdpcm2bit(state, *src++) << 2;
        byte |= EncodeAdpcm2bit(state, *src++) << 4;
        byte |= EncodeAdpcm2bit(state, *src++) << 6;
        *dst++ = byte;
    }
}

// Decodes a packet of ADPCM: the state it starts from, then bytes of codes at the given number of bits a sample
void vct_decode_adpcm(u8 *packet, s16 *dst, u32 size, VCTCodec bits) {
    AdpcmState state;

    state.prevSample = ((AdpcmState *)packet)->prevSample;
    state.prevIndex = ((AdpcmState *)packet)->prevIndex;
    switch (bits) {
    case 2:
        vct_decode_adpcm_16(packet + 4, dst, size - 4, &state);
        break;
    case 3:
        vct_decode_adpcm_24(packet + 4, dst, size - 4, &state);
        break;
    case 4:
        vct_decode_adpcm_32(packet + 4, dst, size - 4, &state);
        break;
    }
}

// Encodes count samples to a packet of ADPCM, which starts with the state before the first sample
void vct_encode_adpcm(u8 *packet, s16 *src, u32 count, AdpcmState *state, VCTCodec bits) {
    AdpcmState *header = (AdpcmState *)packet;

    header->prevSample = state->prevSample;
    header->prevIndex = state->prevIndex;
    header->pad = 0;
    switch (bits) {
    case 2:
        vct_encode_adpcm_16(packet + 4, src, count, state);
        break;
    case 3:
        vct_encode_adpcm_24(packet + 4, src, count, state);
        break;
    case 4:
        vct_encode_adpcm_32(packet + 4, src, count, state);
        break;
    }
}
