#ifndef POKEBW2_TWL_SSP_EXIFENC_H
#define POKEBW2_TWL_SSP_EXIFENC_H

#include "types.h"

// The EXIF writer of TwlSDK's JPEG encoder (ssp_exifenc.c), as the encoder (ssp_jpegenc.c) uses it

// Whether the encoder signs the image it writes, for the DSi's photo viewer. Its setter was dead-stripped, so it stays
// 0
extern BOOL ExifEnc_SignMode;

// Writes the SOI and the APP1 segment with the EXIF data of a width x height image, and its thumbnail if there is one,
// to dst; returns the number of bytes written
u32 ExifEnc_WriteHeader(u8 *dst, u32 width, u32 height, const u8 *thumbnail, u32 thumbnailSize);

#endif // POKEBW2_TWL_SSP_EXIFENC_H
