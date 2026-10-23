#ifndef POKEBW2_TWL_SSP_PRIVATE_H
#define POKEBW2_TWL_SSP_PRIVATE_H

#include "types.h"
#include "twl/photo_sign.h"

// What TwlSDK's SSP files share: the DSi's photo signature, which the encoder writes into a JPEG's maker note and the
// decoder checks, and the EXIF reader's state that the decoder reads

// The size of the signature block in the maker note
#define SSP_SIGNATURE_SIZE 0x1c

// The signature functions are the LTD autoload's photo_sign.c (twl/photo_sign.h)

// ssp_exifdec.c
// Whether SSP_StartJpegDecoder checks the signature of a JPEG
extern BOOL data_ov257_021b6240;

// TwlSDK's SSP_SetJpegDecoderSignMode, which the game doesn't call, so the link dropped it; it was compiled with the
// EXIF reader, whose layout of its data shows that its code refers to the mode. It returns the previous mode
static inline BOOL SSP_SetJpegDecoderSignMode(BOOL mode) {
    BOOL old = data_ov257_021b6240;
    data_ov257_021b6240 = mode;
    return old;
}

// TwlSDK's SSP_ExifDecode (libntr's exifdec.h): reads the EXIF data of the APP1 segment at *cur into the EXIF reader's
// state, and sets *cur to the next segment, or with option bit 0 to the thumbnail. Returns 0 for the thumbnail, 1 for
// the next segment, 2 on an error. libntr's option is an int, but ssp_jpegdec.c's calls compile as in the ROM only
// with the u32 that SSP_StartJpegDecoder takes
s32 SSP_ExifDecode(u8 *src, u32 src_size, int *cur, u32 option);

#endif // POKEBW2_TWL_SSP_PRIVATE_H
