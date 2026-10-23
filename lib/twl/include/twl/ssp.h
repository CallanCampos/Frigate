#ifndef POKEBW2_TWL_SSP_H
#define POKEBW2_TWL_SSP_H

#include "types.h"

// TwlSDK's JPEG encoder and decoder (the SSP library), linked into the Xtransceiver's overlay. The functions and the
// sampling values have the names of the TWL-SDK Function Reference Manual (SSP_StartJpegEncoder) and libntr's
// ssp_jpeg_type.h

#define SSP_JPEG_RGB555 0x4

// The encoder's sampling of the chrominance: 4:4:4 (1x1), 4:2:0 (2x2) or 4:2:2 (2x1)
#define SSP_JPEG_OUTPUT_YUV444 1
#define SSP_JPEG_OUTPUT_YUV420 2
#define SSP_JPEG_OUTPUT_YUV422 3

u32 SSP_GetJpegEncoderBufferSize(u32 width, u32 height, u32 sampling, u32 option);
u32 SSP_StartJpegEncoder(const void *src, u8 *dst, u32 limit, u8 *work, u32 width, u32 height, u32 quality,
                         u32 sampling, u32 option);
BOOL SSP_StartJpegDecoder(u8 *data, u32 size, void *dst, s16 *width, s16 *height, u32 option);

#endif // POKEBW2_TWL_SSP_H
