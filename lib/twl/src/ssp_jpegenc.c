#include "types.h"
#include "gfl/std.h"
#include "nitro/os.h"
#include "ssp_exifenc.h"
#include "ssp_private.h"
#include "twl/ssp.h"

// TwlSDK's JPEG encoder (SSP_StartJpegEncoder), linked into the Xtransceiver's overlay. The file name is a guess after
// TwlSDK's ssp/jpegenc.h. It converts the image to YCbCr planes, transforms and quantizes it in 8x8 blocks with a
// fixed point AAN DCT, and writes a baseline JPEG with the standard Huffman tables, after the EXIF data of
// ssp_exifenc.c or, for the EXIF thumbnail, a bare header. The internal names are descriptive guesses.

// The thumbnail that SSP_JPEG_THUMBNAIL adds to the EXIF data
#define THUMBNAIL_WIDTH 160
#define THUMBNAIL_HEIGHT 120
// Its quality is capped
#define THUMBNAIL_MAX_QUALITY 70
// The work memory it needs: the thumbnail's RGB555 pixels, and its JPEG data in a buffer as large
#define THUMBNAIL_BUFFER_SIZE (THUMBNAIL_WIDTH * THUMBNAIL_HEIGHT * 2)

// EXIF tags that JpegEnc_Sign looks for
#define EXIF_TAG_EXIF_IFD 0x8769
#define EXIF_TAG_MAKER_NOTE 0x927c

// Option bits
#define SSP_JPEG_THUMBNAIL 0x1
#define SSP_JPEG_YUV422 0x100

// Sampling factors
enum {
    SSP_JPEG_444 = 1,
    SSP_JPEG_420,
    SSP_JPEG_422,
};

// A Huffman table in the encoder's work memory: the counts of codes of each length and the values, as the DHT segment
// holds them, and the code and code length of each value
typedef struct {
    u32 unused[257];
    struct {
        u8 bits[32];
        u8 values[256];
        u16 codes[256];
        u32 unused2;
        u8 sizes[256];
        u32 unused3;
    } spec;
    u32 unused4;
} JpegEncHuffTable;

// The code lengths and codes of a table's values, in order, while JpegEnc_SetupHuffmanTable builds it
typedef struct {
    u32 sizes[256];
    u32 codes[256];
} JpegEncHuffWork;

// The start of the encoder's work memory, which the coefficients and the YCbCr planes follow
typedef struct {
    s32 divisors[2][64];
    u8 quant[2][64];
    JpegEncHuffTable dcLum;
    JpegEncHuffTable dcChrom;
    JpegEncHuffTable acLum;
    JpegEncHuffTable acChrom;
    JpegEncHuffWork huffWork;
} JpegEncWork;

// The quantized DCT coefficients of one component, 64 for each block
typedef struct {
    s16 *coefs;
    u32 blocksX;
    u32 blocksY;
    s32 lastDc;
} JpegEncComponent;

// Converts the source pixels to the Y, Cb and Cr planes
typedef void (*JpegEncConvertFunc)(const void *src, u32 width, u32 height, u8 *dst);

static void JpegEnc_ResetBits(void);
static BOOL JpegEnc_PutBits(u8 *dst, u32 *pos, u32 value, u32 count);
static void JpegEnc_ForwardDct(const u8 *block, const s32 *divisors, s16 *coefs);
static void JpegEnc_SetupHuffmanTable(JpegEncHuffWork *work, JpegEncHuffTable *table, u32 type);
static BOOL JpegEnc_WriteHuffmanTable(const JpegEncHuffTable *table, u8 *dst, u32 *pos);
static u32 JpegEnc_GetHuffmanTableLength(const JpegEncHuffTable *table);
static BOOL JpegEnc_EncodeBlock(const JpegEncHuffTable *dcTable, const JpegEncHuffTable *acTable,
                                JpegEncComponent *component, u8 *dst, u32 *pos, u32 x, u32 y);
static void JpegEnc_TransformH2V2(const u8 *src, const s32 *divisors, JpegEncComponent *component, u32 width);
static void JpegEnc_TransformH2V1(const u8 *src, const s32 *divisors, JpegEncComponent *component, u32 width);
static void JpegEnc_TransformH1V1(const u8 *src, const s32 *divisors, JpegEncComponent *component, u32 width);
static u32 JpegEnc_GetWorkSize(u32 width, u32 height, u32 sampling);
static BOOL JpegEnc_Sign(u8 *data, u32 size);
static void JpegEnc_MakeThumbnail(const u16 *src, u32 width, u32 height, u16 *dst, u32 dstWidth, u32 dstHeight,
                                  u32 option);
static void JpegEnc_ConvertRGB555(const void *src, u32 width, u32 height, u8 *dst);
static void JpegEnc_ConvertYUV422(const void *src, u32 width, u32 height, u8 *dst);
static u32 JpegEnc_Encode(const void *src, u8 *dst, u32 limit, JpegEncWork *work, u32 width, u32 height, u32 quality,
                          u32 sampling, BOOL isThumbnail, const u8 *thumbnail, u32 thumbnailSize,
                          JpegEncConvertFunc convert);
static BOOL JpegEnc_CheckParameters(u32 width, u32 height, u32 sampling);

// The bit masks, by bit count
static const u32 sJpegEncBitMasks[17] = {
    0x0000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
    0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000, 0x8000,
};

// The SOS segment of the three components
static u8 sJpegEncSosHeader[14] = {
    0xff, 0xda, 0x00, 0x0c, 0x03, 0x01, 0x00, 0x02, 0x11, 0x03, 0x11, 0x00, 0x3f, 0x00
};

// The SOF0 segment, whose size and sampling are written in, and the start of the DQT segment
static u8 sJpegEncSofHeader[23] = {
    0xff, 0xc0, 0x00, 0x11, 0x08, 0x01, 0xe0, 0x02, 0x80, 0x03, 0x01, 0x11,
    0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, 0xff, 0xdb, 0x00, 0x84,
};

// The SOI marker and the same segments, for the thumbnail, which has no EXIF data
static u8 sJpegEncThumbnailHeader[25] = {
    0xff, 0xd8, 0xff, 0xc0, 0x00, 0x11, 0x08, 0x01, 0xe0, 0x02, 0x80, 0x03, 0x01,
    0x11, 0x00, 0x02, 0x11, 0x01, 0x03, 0x11, 0x01, 0xff, 0xdb, 0x00, 0x84,
};

// A standard Huffman table of the JPEG specification (K.3), as the DHT segment holds it: the number of codes of each
// length, and the values in code order. They are copied with MI_CpuCopy32, and aligned like TwlSDK's DMA buffers
typedef struct {
    u8 bits[16];
    u8 values[12];
} JpegEncDcHuffSpec;

typedef struct {
    u8 bits[16];
    u8 values[162];
} JpegEncAcHuffSpec;

static JpegEncDcHuffSpec sJpegEncDcChromSpec ATTRIBUTE_ALIGN(32) = {
    { 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },
};
static JpegEncDcHuffSpec sJpegEncDcLumSpec ATTRIBUTE_ALIGN(32) = {
    { 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 },
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 },
};

// The standard quantization tables of the JPEG specification (K.1), for quality 50
static u8 sJpegEncLumQuantTable[64] = {
    16, 11,  10,  16, 24, 40, 51, 61, 12,  12,  14,  19,  26, 58, 60, 55,  14,  13,  16,  24, 40, 57,
    69, 56,  14,  17, 22, 29, 51, 87, 80,  62,  18,  22,  37, 56, 68, 109, 103, 77,  24,  35, 55, 64,
    81, 104, 113, 92, 49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98,  112, 100, 103, 99,
};
static u8 sJpegEncChromQuantTable[64] = {
    17, 18, 24, 47, 99, 99, 99, 99, 18, 21, 26, 66, 99, 99, 99, 99, 24, 26, 56, 99, 99, 99,
    99, 99, 47, 66, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
    99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99,
};

// The index in a block of each coefficient in zigzag order
static u8 sJpegEncZigzag[64] = {
    0,  1,  8,  16, 9,  2,  3,  10, 17, 24, 32, 25, 18, 11, 4,  5,  12, 19, 26, 33, 40, 48,
    41, 34, 27, 20, 13, 6,  7,  14, 21, 28, 35, 42, 49, 56, 57, 50, 43, 36, 29, 22, 15, 23,
    30, 37, 44, 51, 58, 59, 52, 45, 38, 31, 39, 46, 53, 60, 61, 54, 47, 55, 62, 63,
};

static JpegEncAcHuffSpec sJpegEncAcChromSpec ATTRIBUTE_ALIGN(32) = {
    { 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 },
    {
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22,
        0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1,
        0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36,
        0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a,
        0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a,
        0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba,
        0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
        0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa,
    },
};
static JpegEncAcHuffSpec sJpegEncAcLumSpec ATTRIBUTE_ALIGN(32) = {
    { 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d },
    {
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71,
        0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72,
        0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36, 0x37,
        0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83,
        0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3,
        0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
        0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
        0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa,
    },
};

// The AAN DCT's scale factors, 8 * 2^21 / (s(u) * s(v)) with s(0) = 1 and s(k) = cos(k pi / 16) * sqrt(2), which
// JpegEnc_Encode divides by the quantizer
static s32 sJpegEncAanScales[64] = {
    0x200, 0x171, 0x188, 0x1b3, 0x200, 0x28c, 0x3b2, 0x740, 0x171, 0x10a, 0x11b, 0x13a, 0x171, 0x1d6, 0x2aa, 0x53a,
    0x188, 0x11b, 0x12c, 0x14d, 0x188, 0x1f3, 0x2d4, 0x58c, 0x1b3, 0x13a, 0x14d, 0x172, 0x1b3, 0x22a, 0x325, 0x62a,
    0x200, 0x171, 0x188, 0x1b3, 0x200, 0x28c, 0x3b2, 0x740, 0x28c, 0x1d6, 0x1f3, 0x22a, 0x28c, 0x33d, 0x4b4, 0x93a,
    0x3b2, 0x2aa, 0x2d4, 0x325, 0x3b2, 0x4b4, 0x6d4, 0xd65, 0x740, 0x53a, 0x58c, 0x62a, 0x740, 0x93a, 0xd65, 0x1a46,
};

// The bit writer: the bits left in the byte being written, the byte, and the size of the output buffer
static u32 sJpegEncLimit;
static u32 sJpegEncBitsLeft;
static u32 sJpegEncBits;

static void JpegEnc_ResetBits(void) {
    sJpegEncBits = 0;
    sJpegEncBitsLeft = 8;
}

// Writes the low count bits of value, from the highest, with a 0 stuffed after each 0xff byte
static BOOL JpegEnc_PutBits(u8 *dst, u32 *pos, u32 value, u32 count) {
    for (; count != 0; count--) {
        if (value & sJpegEncBitMasks[count]) {
            sJpegEncBits |= sJpegEncBitMasks[sJpegEncBitsLeft];
        }
        if (--sJpegEncBitsLeft == 0) {
            dst[*pos] = sJpegEncBits;
            if (sJpegEncBits == 0xff) {
                if (*pos + 2 >= sJpegEncLimit) {
                    return FALSE;
                }
                dst[*pos + 1] = 0;
                *pos += 2;
            } else {
                if (++*pos >= sJpegEncLimit) {
                    return FALSE;
                }
            }
            sJpegEncBits = 0;
            sJpegEncBitsLeft = 8;
        }
    }
    return TRUE;
}

// Transforms an 8x8 block of samples with the AAN DCT in 8 bit fixed point, and quantizes the coefficients
static void JpegEnc_ForwardDct(const u8 *block, const s32 *divisors, s16 *coefs) {
    s32 tmp0, tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
    s32 tmp10, tmp11, tmp12, tmp13;
    s32 tmp10b, tmp11b, tmp12b;
    s32 z1, z3, z11, z13, z5;
    s32 workspace[64];
    const u8 *in;
    s32 *ws;
    const s32 *div;
    s16 *out;
    int i, k;

    for (i = 0; i < 8; i++) {
        in = block + i;
        ws = &workspace[i];
        tmp0 = in[0] + in[56] - 256;
        tmp1 = in[8] + in[48] - 256;
        tmp2 = in[16] + in[40] - 256;
        tmp3 = in[24] + in[32] - 256;
        tmp4 = in[24] - in[32];
        tmp5 = in[16] - in[40];
        tmp6 = in[8] - in[48];
        tmp7 = in[0] - in[56];

        tmp10 = tmp0 + tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;
        tmp13 = tmp0 - tmp3;
        ws[0] = (tmp10 + tmp11) << 8;
        ws[32] = (tmp10 - tmp11) << 8;
        z1 = (tmp12 + tmp13) * 181;
        ws[16] = (tmp13 << 8) + z1;
        ws[48] = (tmp13 << 8) - z1;

        tmp10 = tmp4 + tmp5;
        tmp11 = tmp5 + tmp6;
        tmp12 = tmp6 + tmp7;
        z5 = tmp10 + tmp12;
        z3 = tmp11 * 181;
        z11 = (tmp7 << 8) + z3;
        z13 = (tmp7 << 8) - z3;
        ws[8] = z11 + z5 * 98 + tmp12 * 139;
        ws[40] = z13 + z5 * 237 - tmp12 * 334;
        ws[24] = z13 - z5 * 237 + tmp12 * 334;
        ws[56] = z11 - z5 * 98 - tmp12 * 139;
    }

    for (i = 0, k = 0; i < 8; i++, k += 8) {
        ws = &workspace[k];
        div = &divisors[i * 8];
        out = &coefs[k];
        tmp0 = ws[0] + ws[7];
        tmp7 = ws[0] - ws[7];
        tmp1 = ws[1] + ws[6];
        tmp6 = ws[1] - ws[6];
        tmp2 = ws[2] + ws[5];
        tmp5 = ws[2] - ws[5];
        tmp3 = ws[3] + ws[4];
        tmp4 = ws[3] - ws[4];

        tmp10 = tmp0 + tmp3;
        tmp11 = tmp1 + tmp2;
        tmp12 = tmp1 - tmp2;
        tmp13 = tmp0 - tmp3;
        z1 = (tmp12 + tmp13) * 181;
        tmp10b = tmp4 + tmp5;
        tmp11b = tmp5 + tmp6;
        tmp12b = tmp6 + tmp7;
        z5 = tmp10b + tmp12b;
        z3 = tmp11b * 181;
        z11 = (tmp7 << 8) + z3;
        z13 = (tmp7 << 8) - z3;
        out[0] = (div[0] * ((tmp10 + tmp11 + 0x80) >> 8) + 0x800) >> 12;
        out[1] = (div[1] * ((z11 + z5 * 98 + tmp12b * 139 + 0x8000) >> 16) + 0x800) >> 12;
        out[2] = (div[2] * (((tmp13 << 8) + z1 + 0x8000) >> 16) + 0x800) >> 12;
        out[3] = (div[3] * ((z13 - z5 * 237 + tmp12b * 334 + 0x8000) >> 16) + 0x800) >> 12;
        out[4] = (div[4] * ((tmp10 - tmp11 + 0x80) >> 8) + 0x800) >> 12;
        out[5] = (div[5] * ((z13 + z5 * 237 - tmp12b * 334 + 0x8000) >> 16) + 0x800) >> 12;
        out[6] = (div[6] * (((tmp13 << 8) - z1 + 0x8000) >> 16) + 0x800) >> 12;
        out[7] = (div[7] * ((z11 - z5 * 98 - tmp12b * 139 + 0x8000) >> 16) + 0x800) >> 12;
    }
}

// Copies one of the standard tables into a Huffman table, and derives the code of each value from it (JPEG
// specification, C.1 to C.3); type is 0 and 1 for the luminance and chrominance DC tables, 2 and 3 for the AC tables
static void JpegEnc_SetupHuffmanTable(JpegEncHuffWork *work, JpegEncHuffTable *table, u32 type) {
    u32 i, j, p;
    u16 code = 0;
    u32 size;
    u32 value;

    sys_memset32_fast(0, &table->spec, sizeof(table->spec));
    switch (type) {
    case 0:
        sys_memcpy32_fast(sJpegEncDcLumSpec.bits, table->spec.bits, 16);
        break;
    case 1:
        sys_memcpy32_fast(sJpegEncDcChromSpec.bits, table->spec.bits, 16);
        break;
    case 2:
        sys_memcpy32_fast(sJpegEncAcLumSpec.bits, table->spec.bits, 16);
        break;
    case 3:
        sys_memcpy32_fast(sJpegEncAcChromSpec.bits, table->spec.bits, 16);
        break;
    }
    switch (type) {
    case 0:
        sys_memcpy32_fast(sJpegEncDcLumSpec.values, table->spec.values, sizeof(sJpegEncDcLumSpec.values));
        break;
    case 1:
        sys_memcpy32_fast(sJpegEncDcChromSpec.values, table->spec.values, sizeof(sJpegEncDcChromSpec.values));
        break;
    case 2:
        sys_memcpy_fast(sJpegEncAcLumSpec.values, table->spec.values, sizeof(sJpegEncAcLumSpec.values));
        break;
    case 3:
        sys_memcpy_fast(sJpegEncAcChromSpec.values, table->spec.values, sizeof(sJpegEncAcChromSpec.values));
        break;
    }

    i = 0;
    sys_memset32_fast(0, work->sizes, sizeof(work->sizes));
    p = 0;
    while (i < 16) {
        for (j = 0; j < table->spec.bits[i]; j++) {
            work->sizes[p] = i + 1;
            p++;
        }
        work->sizes[p] = 0;
        i++;
    }

    sys_memset32_fast(0, work->codes, sizeof(work->codes));
    p = 0;
    size = work->sizes[0];
    while (work->sizes[p] != 0) {
        while (work->sizes[p] == size) {
            work->codes[p] = code;
            code++;
            p++;
        }
        size++;
        code <<= 1;
    }

    for (p = 0; p < 256; p++) {
        if (work->sizes[p] != 0) {
            value = table->spec.values[p];
            table->spec.codes[value] = work->codes[p];
            table->spec.sizes[value] = work->sizes[p];
        }
    }
}

// Writes the code counts and values of a table, as the DHT segment holds them
static BOOL JpegEnc_WriteHuffmanTable(const JpegEncHuffTable *table, u8 *dst, u32 *pos) {
    u32 i;
    u32 count = 0;

    if (*pos + 16 >= sJpegEncLimit) {
        return FALSE;
    }
    for (i = 0; i < 16; i++) {
        dst[*pos] = table->spec.bits[i];
        count += table->spec.bits[i];
        (*pos)++;
    }
    if (*pos + count >= sJpegEncLimit) {
        return FALSE;
    }
    for (i = 0; i < count; i++) {
        dst[*pos] = table->spec.values[i];
        (*pos)++;
    }
    return TRUE;
}

// The length of a table in the DHT segment, with its class and ID
static u32 JpegEnc_GetHuffmanTableLength(const JpegEncHuffTable *table) {
    u32 i;
    u32 count = 0;

    for (i = 0; i < 16; i++) {
        count += table->spec.bits[i];
    }
    return count + 17;
}

// Encodes the block at x, y of a component
static BOOL JpegEnc_EncodeBlock(const JpegEncHuffTable *dcTable, const JpegEncHuffTable *acTable,
                                JpegEncComponent *component, u8 *dst, u32 *pos, u32 x, u32 y) {
    u32 k;
    s32 temp, temp2;
    s16 *block;
    u32 nbits;
    u32 run;
    u32 i;

    block = &component->coefs[(y * component->blocksX + x) * 64];
    temp = block[0] - component->lastDc;
    component->lastDc = block[0];
    if (temp >= 0) {
        temp2 = temp;
    } else {
        temp = -temp;
        temp2 = ~temp;
    }
    nbits = 0;
    while (temp != 0) {
        nbits++;
        temp >>= 1;
    }
    if (!JpegEnc_PutBits(dst, pos, dcTable->spec.codes[nbits], dcTable->spec.sizes[nbits])) {
        return FALSE;
    }
    if (nbits != 0 && !JpegEnc_PutBits(dst, pos, temp2, nbits)) {
        return FALSE;
    }

    run = 0;
    for (k = 1; k < 64; k++) {
        if (block[sJpegEncZigzag[k]] != 0) {
            while (run >= 16) {
                if (!JpegEnc_PutBits(dst, pos, acTable->spec.codes[0xf0], acTable->spec.sizes[0xf0])) {
                    return FALSE;
                }
                run -= 16;
            }
            temp = block[sJpegEncZigzag[k]];
            if (temp >= 0) {
                temp2 = temp;
            } else {
                temp = -temp;
                temp2 = ~temp;
            }
            nbits = 0;
            while (temp != 0) {
                temp >>= 1;
                nbits++;
            }
            i = (run << 4) | nbits;
            if (!JpegEnc_PutBits(dst, pos, acTable->spec.codes[i], acTable->spec.sizes[i])) {
                return FALSE;
            }
            if (nbits != 0 && !JpegEnc_PutBits(dst, pos, temp2, nbits)) {
                return FALSE;
            }
            run = 0;
        } else {
            run++;
        }
    }
    if (run != 0 && !JpegEnc_PutBits(dst, pos, acTable->spec.codes[0], acTable->spec.sizes[0])) {
        return FALSE;
    }
    return TRUE;
}

// Transforms a chrominance plane subsampled 2:1 both ways
static void JpegEnc_TransformH2V2(const u8 *src, const s32 *divisors, JpegEncComponent *component, u32 width) {
    u32 n;
    u32 x, y;
    s16 *coefs;
    u32 blocksX;
    u32 blocksY;
    u8 block[64];
    const u8 *row0;
    const u8 *row1;
    u32 offset;
    u32 i;

    coefs = component->coefs;
    blocksX = component->blocksX;
    blocksY = component->blocksY;
    for (y = 0, n = 0; y < blocksY; y++) {
        for (x = 0; x < blocksX; x++) {
            offset = x * 16 + (y << 4) * width;
            for (i = 0; i < 64; i += 8) {
                row0 = src + offset;
                row1 = src + width + offset;
                block[i + 0] = (row0[0] + row0[1] + row1[0] + row1[1]) >> 2;
                block[i + 1] = (row0[2] + row0[3] + row1[2] + row1[3]) >> 2;
                block[i + 2] = (row0[4] + row0[5] + row1[4] + row1[5]) >> 2;
                block[i + 3] = (row0[6] + row0[7] + row1[6] + row1[7]) >> 2;
                block[i + 4] = (row0[8] + row0[9] + row1[8] + row1[9]) >> 2;
                block[i + 5] = (row0[10] + row0[11] + row1[10] + row1[11]) >> 2;
                block[i + 6] = (row0[12] + row0[13] + row1[12] + row1[13]) >> 2;
                block[i + 7] = (row0[14] + row0[15] + row1[14] + row1[15]) >> 2;
                offset += width * 2;
            }
            JpegEnc_ForwardDct(block, divisors, &coefs[n * 64]);
            n++;
        }
    }
}

// Transforms a chrominance plane subsampled 2:1 horizontally
static void JpegEnc_TransformH2V1(const u8 *src, const s32 *divisors, JpegEncComponent *component, u32 width) {
    u32 n;
    u32 x, y;
    s16 *coefs;
    u32 blocksX;
    u32 blocksY;
    u8 block[64];
    const u8 *row;
    u32 offset;
    u32 i;

    coefs = component->coefs;
    blocksX = component->blocksX;
    blocksY = component->blocksY;
    for (y = 0, n = 0; y < blocksY; y++) {
        for (x = 0; x < blocksX; x++) {
            offset = x * 16 + (y << 3) * width;
            for (i = 0; i < 64; i += 8) {
                row = src + offset;
                block[i + 0] = (row[0] + row[1]) >> 1;
                block[i + 1] = (row[2] + row[3]) >> 1;
                block[i + 2] = (row[4] + row[5]) >> 1;
                block[i + 3] = (row[6] + row[7]) >> 1;
                block[i + 4] = (row[8] + row[9]) >> 1;
                block[i + 5] = (row[10] + row[11]) >> 1;
                block[i + 6] = (row[12] + row[13]) >> 1;
                block[i + 7] = (row[14] + row[15]) >> 1;
                offset += width;
            }
            JpegEnc_ForwardDct(block, divisors, &coefs[n * 64]);
            n++;
        }
    }
}

// Transforms a plane at full resolution
static void JpegEnc_TransformH1V1(const u8 *src, const s32 *divisors, JpegEncComponent *component, u32 width) {
    u32 n;
    u32 x, y;
    s16 *coefs = component->coefs;
    u32 blocksX = component->blocksX;
    u32 blocksY = component->blocksY;
    const u8 *row0 = src;
    const u8 *row1 = src + width;
    const u8 *row2 = src + width * 2;
    const u8 *row3 = src + width * 3;
    const u8 *row4 = src + width * 4;
    const u8 *row5 = src + width * 5;
    const u8 *row6 = src + width * 6;
    const u8 *row7 = src + width * 7;
    u8 block[64];
    u32 i;

    for (y = 0, n = 0; y < blocksY; y++) {
        for (x = 0; x < blocksX; x++) {
            for (i = 0; i < 8; i++) {
                block[i] = *row0;
                row0++;
                block[i + 8] = *row1;
                row1++;
                block[i + 16] = *row2;
                row2++;
                block[i + 24] = *row3;
                row3++;
                block[i + 32] = *row4;
                row4++;
                block[i + 40] = *row5;
                row5++;
                block[i + 48] = *row6;
                row6++;
                block[i + 56] = *row7;
                row7++;
            }
            JpegEnc_ForwardDct(block, divisors, &coefs[n * 64]);
            n++;
        }
        row0 = row7;
        row1 = row0 + width;
        row2 = row1 + width;
        row3 = row2 + width;
        row4 = row3 + width;
        row5 = row4 + width;
        row6 = row5 + width;
        row7 = row6 + width;
    }
}

// The work memory that JpegEnc_Encode needs for an image
static u32 JpegEnc_GetWorkSize(u32 width, u32 height, u32 sampling) {
    u32 alignedWidth, alignedHeight;
    u32 hFactor = 1;
    u32 vFactor = 1;
    u32 blocksX, blocksY;
    u32 chromBlocksX, chromBlocksY;

    switch (sampling) {
    case SSP_JPEG_444:
        break;
    case SSP_JPEG_420:
        vFactor = 2;
        hFactor = 2;
        break;
    case SSP_JPEG_422:
        hFactor = 2;
        break;
    }
    alignedWidth = ~(hFactor * 8 - 1) & (width + (hFactor * 8 - 1));
    alignedHeight = ~(vFactor * 8 - 1) & (height + (vFactor * 8 - 1));
    blocksX = alignedWidth / 8;
    blocksY = alignedHeight / 8;
    chromBlocksX = blocksX / hFactor;
    chromBlocksY = blocksY / vFactor;
    return alignedWidth * alignedHeight * 3 + (blocksX * blocksY + chromBlocksX * chromBlocksY * 2) * 64 * sizeof(s16) +
           sizeof(JpegEncWork);
}

u32 SSP_GetJpegEncoderBufferSize(u32 width, u32 height, u32 sampling, u32 option) {
    u32 thumbnail = option & SSP_JPEG_THUMBNAIL;
    u32 size, thumbnailSize;

    if (thumbnail) {
        if (width < THUMBNAIL_WIDTH) {
            width = THUMBNAIL_WIDTH;
        }
        if (height < THUMBNAIL_HEIGHT) {
            height = THUMBNAIL_HEIGHT;
        }
    }
    size = JpegEnc_GetWorkSize(width, height, sampling);
    thumbnailSize = JpegEnc_GetWorkSize(THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, SSP_JPEG_444);
    if (size > thumbnailSize) {
        return size + (thumbnail ? THUMBNAIL_BUFFER_SIZE * 2 : 0);
    } else {
        return thumbnailSize + (thumbnail ? THUMBNAIL_BUFFER_SIZE * 2 : 0);
    }
}

// Reads a big-endian number
#define READ_BE16(p) ((u16)((p)[1] + ((p)[0] << 8)))
#define READ_BE32(p) (((p)[0] << 24) + ((p)[1] << 16) + ((p)[2] << 8) + (p)[3])

// Writes the signature of the image into the first entry of its EXIF maker note
static BOOL JpegEnc_Sign(u8 *data, u32 size) {
    u8 signature[SSP_SIGNATURE_SIZE];
    u32 pos, count, i;
    u32 limit = sJpegEncLimit;
    u16 tag;
    const u8 *entry;
    u32 offset;

    if (limit < 0x17) {
        return FALSE;
    }

    // IFD0, whose offset is at the end of the TIFF header, after the APP1 marker and length and "Exif"
    pos = data[0x13] + 12;
    if (pos + 2 > limit) {
        return FALSE;
    }
    count = READ_BE16(&data[pos]);
    pos += 2;
    if (pos + count * 12 > limit) {
        return FALSE;
    }
    tag = 0;
    for (i = 0; i < count; i++) {
        entry = &data[pos];
        tag = READ_BE16(entry);
        if (tag == EXIF_TAG_EXIF_IFD) {
            pos = READ_BE32(entry + 8) + 12;
            break;
        }
        pos += 12;
    }
    if (tag != EXIF_TAG_EXIF_IFD) {
        return FALSE;
    }

    if (pos + 2 > limit) {
        return FALSE;
    }
    count = READ_BE16(&data[pos]);
    pos += 2;
    if (pos + count * 12 > limit) {
        return FALSE;
    }
    tag = 0;
    for (i = 0; i < count; i++) {
        entry = &data[pos];
        tag = READ_BE16(entry);
        if (tag == EXIF_TAG_MAKER_NOTE) {
            pos = READ_BE32(entry + 8) + 12;
            break;
        }
        pos += 12;
    }
    if (tag != EXIF_TAG_MAKER_NOTE) {
        return FALSE;
    }

    // The maker note's first entry, tag 0x1000, holds the signature
    if (pos + 4 > limit) {
        return FALSE;
    }
    entry = &data[pos];
    if (entry[2] != 0x10 || entry[3] != 0x00) {
        return FALSE;
    }
    if (pos + 14 > limit) {
        return FALSE;
    }
    offset = READ_BE32(entry + 10) + 12;
    if (offset + SSP_SIGNATURE_SIZE > limit) {
        return FALSE;
    }
    sys_memset(&data[offset], 0, SSP_SIGNATURE_SIZE);
    if (hw_isDSi()) {
        PhotoSign_Init();
    }
    if (PhotoSign_Sign(signature, data, size) != TRUE) {
        return FALSE;
    }
    sys_memcpy(signature, &data[offset], SSP_SIGNATURE_SIZE);
    return TRUE;
}

// Scales the image down to the thumbnail, in RGB555 or YUV422 like the image
static void JpegEnc_MakeThumbnail(const u16 *src, u32 width, u32 height, u16 *dst, u32 dstWidth, u32 dstHeight,
                                  u32 option) {
    u32 prev = 0;
    u32 format = option & (SSP_JPEG_RGB555 | SSP_JPEG_YUV422);
    u32 stepX = (width << 4) / dstWidth;
    u32 srcX, srcY;
    s32 x, y;
    u32 pair;
    u32 pixel, u, v, luma;

    // From the last pixel back, so that each odd pixel is kept in prev until the even one before it
    for (y = dstHeight - 1; y >= 0; y--) {
        srcY = ((height * y) << 4) / dstHeight;
        srcX = width << 4;
        for (x = dstWidth - 1; x >= 0; x--) {
            srcX -= stepX;
            if (format != SSP_JPEG_RGB555 && format == SSP_JPEG_YUV422) {
                pixel = (srcX >> 4);
                pair = ((const u32 *)src)[((pixel & ~1) + width * (srcY >> 4)) >> 1];
                u = pair & 0xff00;
                v = pair & 0xff000000;
                if (pixel & 1) {
                    luma = (u8)(pair >> 16) & pair;
                } else {
                    luma = (u8)pair;
                }
                if (x & 1) {
                    prev = v | (luma | u);
                } else {
                    // The two pixels' Y, and their U and V averaged
                    u = (prev >> 1) + (u >> 1) + (v >> 1);
                    ((u32 *)dst)[(x + y * dstWidth) >> 1] =
                        luma | (((prev & 0xff) << 16) | (u & 0xff00ff00));
                }
            } else {
                dst[y * dstWidth + x] = src[width * (srcY >> 4) + (srcX >> 4)];
            }
        }
    }
}

// Converts RGB555 pixels to the Y, Cb and Cr planes, padded to whole blocks
static void JpegEnc_ConvertRGB555(const void *src, u32 width, u32 height, u8 *dst) {
    const u16 *pixels = src;
    u32 size;
    u8 *crPlane;
    u32 alignedWidth = (width + 7) & ~7;
    u8 *cbPlane;
    u8 *yPlane = dst;
    u32 alignedHeight = (height + 7) & ~7;
    u32 i, x;
    u32 color, r, g, b;
    u32 cb, cr;

    size = alignedWidth * alignedHeight;
    cbPlane = dst + size;
    crPlane = dst + size + size;

    for (i = 0; i < size; i++) {
        x = i % alignedWidth;
        color = x < width ? *pixels : 0;
        r = (color & 0x1f) << 3;
        g = (color & 0x3e0) >> 2;
        b = (color & 0x7c00) >> 7;
        yPlane[i] = (r * 0x4c9 + g * 0x964 + b * 0x1d3 + 0x800) >> 12;
        cb = (0x80800 - r * 0x2b3 - g * 0x54d + (b << 11)) >> 12;
        if (cb > 255) {
            cb = 255;
        }
        cbPlane[i] = cb;
        cr = ((r << 11) + 0x80800 - g * 0x6b3 - b * 0x14d) >> 12;
        if (cr > 255) {
            cr = 255;
        }
        crPlane[i] = cr;
        if (x < width) {
            pixels++;
        }
    }
}

// Splits YUV422 pixels into the Y, Cb and Cr planes, doubling the chrominance horizontally
static void JpegEnc_ConvertYUV422(const void *src, u32 width, u32 height, u8 *dst) {
    const u32 *pixels = src;
    u32 count = width * height / 2;
    u16 *crPlane;
    u16 *cbPlane;
    u16 *yPlane = (u16 *)dst;
    cbPlane = yPlane + count;
    crPlane = yPlane + count + count;
    u32 i;
    u32 pair;

    for (i = 0; i < count; i++) {
        pair = *pixels++;
        yPlane[i] = (pair & 0xff) | ((pair & 0xff0000) >> 8);
        cbPlane[i] = ((pair & 0xff00) >> 8) | (pair & 0xff00);
        crPlane[i] = (pair >> 24) | ((pair & 0xff000000) >> 16);
    }
}

// Encodes an image, after the EXIF data or as a thumbnail; returns its size, or 0 if it didn't fit in limit bytes
static u32 JpegEnc_Encode(const void *src, u8 *dst, u32 limit, JpegEncWork *work, u32 width, u32 height, u32 quality,
                          u32 sampling, BOOL isThumbnail, const u8 *thumbnail, u32 thumbnailSize,
                          JpegEncConvertFunc convert) {
    JpegEncComponent components[3];
    u32 pos;
    u32 yOffset, cbOffset, crOffset;
    u32 chromBlocksX, chromBlocksY;
    u32 blocksX, blocksY;
    u32 mcusX, mcusY;
    u32 hFactor = 1;
    u32 vFactor = 1;
    u32 q;
    u32 i;
    u32 x, y;
    u32 dhtLength;
    u32 coefOffset;
    u32 size;
    u8 (*quant)[64];
    JpegEncHuffTable *dcLum;
    JpegEncHuffTable *dcChrom;
    JpegEncHuffTable *acLum;
    JpegEncHuffTable *acChrom;
    JpegEncHuffWork *huffWork;

    switch (sampling) {
    case SSP_JPEG_444:
        break;
    case SSP_JPEG_420:
        vFactor = 2;
        hFactor = 2;
        break;
    case SSP_JPEG_422:
        hFactor = 2;
        break;
    }
    sJpegEncLimit = limit;
    mcusX = width / (hFactor * 8);
    mcusY = height / (vFactor * 8);
    blocksX = width / 8;
    blocksY = height / 8;
    quant = work->quant;
    dcLum = &work->dcLum;
    dcChrom = &work->dcChrom;
    acLum = &work->acLum;
    acChrom = &work->acChrom;
    huffWork = &work->huffWork;
    chromBlocksX = blocksX / hFactor;
    chromBlocksY = blocksY / vFactor;
    coefOffset = sizeof(JpegEncWork) + blocksX * blocksY * 128;
    components[0].coefs = (s16 *)((u8 *)work + sizeof(JpegEncWork));
    components[0].blocksX = blocksX;
    components[0].blocksY = blocksY;
    components[0].lastDc = 0;
    components[1].coefs = (s16 *)((u8 *)work + coefOffset);
    components[1].blocksX = chromBlocksX;
    components[1].blocksY = chromBlocksY;
    components[1].lastDc = 0;
    components[2].coefs = (s16 *)((u8 *)work + (coefOffset + chromBlocksX * chromBlocksY * 128));
    components[2].blocksX = chromBlocksX;
    components[2].blocksY = chromBlocksY;
    components[2].lastDc = 0;
    yOffset = coefOffset + chromBlocksX * chromBlocksY * 128 * 2;
    size = width * height;
    cbOffset = yOffset + size;
    crOffset = yOffset + size * 2;

    // Scale the quantization tables as IJG's libjpeg does
    if (quality < 50) {
        quality = 5000 / quality;
    } else {
        quality = 200 - quality * 2;
    }
    for (i = 0; i < 64; i++) {
        q = (sJpegEncLumQuantTable[i] * quality + 50) / 100;
        if (q == 0) {
            q = 1;
        }
        if (q > 255) {
            q = 255;
        }
        quant[0][i] = q;
        work->divisors[0][i] = sJpegEncAanScales[i] / (s32)q;
        q = (sJpegEncChromQuantTable[i] * quality + 50) / 100;
        if (q == 0) {
            q = 1;
        }
        if (q > 255) {
            q = 255;
        }
        quant[1][i] = q;
        work->divisors[1][i] = sJpegEncAanScales[i] / (s32)q;
    }

    if (convert != NULL) {
        convert(src, width, height, (u8 *)work + yOffset);
    }
    JpegEnc_TransformH1V1((u8 *)work + yOffset, work->divisors[0], &components[0], width);
    if (sampling == SSP_JPEG_444) {
        JpegEnc_TransformH1V1((u8 *)work + cbOffset, work->divisors[1], &components[1], width);
    } else if (sampling == SSP_JPEG_420) {
        JpegEnc_TransformH2V2((u8 *)work + cbOffset, work->divisors[1], &components[1], width);
    } else {
        JpegEnc_TransformH2V1((u8 *)work + cbOffset, work->divisors[1], &components[1], width);
    }
    if (sampling == SSP_JPEG_444) {
        JpegEnc_TransformH1V1((u8 *)work + crOffset, work->divisors[1], &components[2], width);
    } else if (sampling == SSP_JPEG_420) {
        JpegEnc_TransformH2V2((u8 *)work + crOffset, work->divisors[1], &components[2], width);
    } else {
        JpegEnc_TransformH2V1((u8 *)work + crOffset, work->divisors[1], &components[2], width);
    }

    // SOI, or the EXIF data, and SOF0
    if (isThumbnail) {
        if (sizeof(sJpegEncThumbnailHeader) >= sJpegEncLimit) {
            return 0;
        }
        sys_memcpy(sJpegEncThumbnailHeader, dst, sizeof(sJpegEncThumbnailHeader));
        dst[7] = THUMBNAIL_HEIGHT >> 8;
        dst[8] = THUMBNAIL_HEIGHT & 0xff;
        dst[9] = THUMBNAIL_WIDTH >> 8;
        dst[10] = THUMBNAIL_WIDTH & 0xff;
        dst[13] = (hFactor << 4) | vFactor;
        pos = sizeof(sJpegEncThumbnailHeader);
    } else {
        pos = ExifEnc_WriteHeader(dst, width, height, thumbnail, thumbnailSize);
        if (pos + sizeof(sJpegEncSofHeader) >= sJpegEncLimit) {
            return 0;
        }
        sys_memcpy(sJpegEncSofHeader, &dst[pos], sizeof(sJpegEncSofHeader));
        dst[pos + 5] = height >> 8;
        dst[pos + 6] = height;
        dst[pos + 7] = width >> 8;
        dst[pos + 8] = width;
        dst[pos + 11] = (hFactor << 4) | vFactor;
        pos += sizeof(sJpegEncSofHeader);
    }

    // DQT
    if (pos + 130 >= sJpegEncLimit) {
        return 0;
    }
    dst[pos++] = 0;
    for (i = 0; i < 64; i++) {
        dst[pos] = quant[0][sJpegEncZigzag[i]];
        pos++;
    }
    dst[pos++] = 1;
    for (i = 0; i < 64; i++) {
        dst[pos] = quant[1][sJpegEncZigzag[i]];
        pos++;
    }

    // DHT
    JpegEnc_SetupHuffmanTable(huffWork, dcLum, 0);
    JpegEnc_SetupHuffmanTable(huffWork, acLum, 2);
    JpegEnc_SetupHuffmanTable(huffWork, dcChrom, 1);
    JpegEnc_SetupHuffmanTable(huffWork, acChrom, 3);
    dhtLength = 2 + JpegEnc_GetHuffmanTableLength(dcLum) + JpegEnc_GetHuffmanTableLength(dcChrom) +
                JpegEnc_GetHuffmanTableLength(acLum) + JpegEnc_GetHuffmanTableLength(acChrom);
    if (pos + 4 >= sJpegEncLimit) {
        return 0;
    }
    dst[pos] = 0xff;
    dst[pos + 1] = 0xc4;
    dst[pos + 2] = dhtLength >> 8;
    dst[pos + 3] = dhtLength;
    pos += 4;
    dst[pos++] = 0x00;
    if (!JpegEnc_WriteHuffmanTable(dcLum, dst, &pos)) {
        return 0;
    }
    dst[pos++] = 0x01;
    if (!JpegEnc_WriteHuffmanTable(dcChrom, dst, &pos)) {
        return 0;
    }
    dst[pos++] = 0x10;
    if (!JpegEnc_WriteHuffmanTable(acLum, dst, &pos)) {
        return 0;
    }
    dst[pos++] = 0x11;
    if (!JpegEnc_WriteHuffmanTable(acChrom, dst, &pos)) {
        return 0;
    }

    // SOS and the scan
    for (i = 0; i < 3; i++) {
        components[i].lastDc = 0;
    }
    JpegEnc_ResetBits();
    if (pos + sizeof(sJpegEncSosHeader) > sJpegEncLimit) {
        return 0;
    }
    for (i = 0; i < sizeof(sJpegEncSosHeader); i++) {
        dst[pos] = sJpegEncSosHeader[i];
        pos++;
    }
    for (y = 0; y < mcusY; y++) {
        for (x = 0; x < mcusX; x++) {
            if (sampling == SSP_JPEG_444) {
                if (!JpegEnc_EncodeBlock(dcLum, acLum, &components[0], dst, &pos, x, y)) {
                    return 0;
                }
            } else if (sampling == SSP_JPEG_420) {
                if (!JpegEnc_EncodeBlock(dcLum, acLum, &components[0], dst, &pos, x * 2, y * 2)) {
                    return 0;
                }
                if (!JpegEnc_EncodeBlock(dcLum, acLum, &components[0], dst, &pos, x * 2 + 1, y * 2)) {
                    return 0;
                }
                if (!JpegEnc_EncodeBlock(dcLum, acLum, &components[0], dst, &pos, x * 2, y * 2 + 1)) {
                    return 0;
                }
                if (!JpegEnc_EncodeBlock(dcLum, acLum, &components[0], dst, &pos, x * 2 + 1, y * 2 + 1)) {
                    return 0;
                }
            } else {
                if (!JpegEnc_EncodeBlock(dcLum, acLum, &components[0], dst, &pos, x * 2, y)) {
                    return 0;
                }
                if (!JpegEnc_EncodeBlock(dcLum, acLum, &components[0], dst, &pos, x * 2 + 1, y)) {
                    return 0;
                }
            }
            if (!JpegEnc_EncodeBlock(dcChrom, acChrom, &components[1], dst, &pos, x, y)) {
                return 0;
            }
            if (!JpegEnc_EncodeBlock(dcChrom, acChrom, &components[2], dst, &pos, x, y)) {
                return 0;
            }
        }
    }
    if (sJpegEncBitsLeft != 8 && !JpegEnc_PutBits(dst, &pos, 0, 7)) {
        return 0;
    }

    // EOI
    if (pos + 2 >= sJpegEncLimit) {
        return 0;
    }
    dst[pos] = 0xff;
    dst[pos + 1] = 0xd9;
    pos += 2;
    if (isThumbnail) {
        // The thumbnail is padded to an even size
        if ((pos & 1) == 1) {
            dst[pos] = 0;
            pos++;
        }
        return pos;
    }
    if (ExifEnc_SignMode == TRUE && hw_isDSi() == TRUE) {
        if (JpegEnc_Sign(dst, pos)) {
            return pos;
        }
        return 0;
    }
    return pos;
}

static BOOL JpegEnc_CheckParameters(u32 width, u32 height, u32 sampling) {
    if (width == 0 || width > 0xffff || height == 0 || height > 0xffff) {
        return FALSE;
    }
    switch (sampling) {
    case SSP_JPEG_444:
        if ((width & 7) != 0 || (height & 7) != 0) {
            return FALSE;
        }
        break;
    case SSP_JPEG_420:
        if ((width & 15) != 0 || (height & 15) != 0) {
            return FALSE;
        }
        break;
    case SSP_JPEG_422:
        if ((width & 15) != 0 || (height & 7) != 0) {
            return FALSE;
        }
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

u32 SSP_StartJpegEncoder(const void *src, u8 *dst, u32 limit, u8 *work, u32 width, u32 height, u32 quality,
                         u32 sampling, u32 option) {
    u8 *thumbnailBuffer = NULL;
    u32 thumbnailSize = 0;
    BOOL thumbnail = option & SSP_JPEG_THUMBNAIL;
    JpegEncConvertFunc convert;
    u32 thumbnailQuality;

    if (!JpegEnc_CheckParameters(width, height, sampling)) {
        return 0;
    }
    if ((option & (SSP_JPEG_RGB555 | SSP_JPEG_YUV422)) != SSP_JPEG_RGB555 &&
        (option & (SSP_JPEG_RGB555 | SSP_JPEG_YUV422)) == SSP_JPEG_YUV422) {
        convert = JpegEnc_ConvertYUV422;
    } else {
        convert = JpegEnc_ConvertRGB555;
    }

    if (thumbnail) {
        thumbnailQuality = quality;
        thumbnailBuffer = work;
        work += THUMBNAIL_BUFFER_SIZE * 2;
        JpegEnc_MakeThumbnail(src, width, height, (u16 *)(thumbnailBuffer + THUMBNAIL_BUFFER_SIZE), THUMBNAIL_WIDTH,
                              THUMBNAIL_HEIGHT, option);
        if (quality > THUMBNAIL_MAX_QUALITY) {
            thumbnailQuality = THUMBNAIL_MAX_QUALITY;
        }
        thumbnailSize = JpegEnc_Encode(thumbnailBuffer + THUMBNAIL_BUFFER_SIZE, thumbnailBuffer, THUMBNAIL_BUFFER_SIZE,
                                       (JpegEncWork *)work, THUMBNAIL_WIDTH, THUMBNAIL_HEIGHT, thumbnailQuality,
                                       SSP_JPEG_422, TRUE, NULL, 0, convert);
        if (thumbnailSize == 0) {
            return 0;
        }
    }
    return JpegEnc_Encode(src, dst, limit, (JpegEncWork *)work, width, height, quality, sampling, FALSE,
                          thumbnail ? thumbnailBuffer : NULL, thumbnailSize, convert);
}
