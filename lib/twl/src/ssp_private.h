#ifndef POKEBW2_TWL_SSP_PRIVATE_H
#define POKEBW2_TWL_SSP_PRIVATE_H

#include "types.h"

// What TwlSDK's SSP files share: the DSi's photo signature, which the encoder writes into a JPEG's maker note and the
// decoder checks, and the EXIF reader's state that the decoder reads

// The size of the signature block in the maker note
#define SSP_SIGNATURE_SIZE 0x1c

// The signature functions, in the LTD autoload, which keep their default names until it is analyzed. The first sets
// up the ARM7's side once. The second signs data, whose signature block is zeroed, writing the signature into sig; the
// third computes the same signature and compares it with sig. Both return 1 on success
void func_027076c4(void);
int func_02707a8c(u8 *sig, const u8 *data, u32 size);
int func_02707ba4(const u8 *sig, const u8 *data, u32 size);

// ssp_exifdec.c
// Whether SSP_StartJpegDecoder checks the signature of a JPEG
extern BOOL data_ov257_021b6240;

// TwlSDK's SSP_SetJpegDecoderSignMode, which the game doesn't call, so the link dropped it; it was compiled with the
// EXIF reader, whose layout of its data shows that its code refers to the mode
static inline void SSP_SetJpegDecoderSignMode(BOOL signMode) {
    data_ov257_021b6240 = signMode;
}

// Reads the EXIF data of the APP1 segment at *pos into the EXIF reader's state, and sets *pos to the next segment, or
// with option bit 0 to the thumbnail. Returns 0 for the thumbnail, 1 for the next segment, 2 on an error
int func_ov257_021acc20(u8 *data, u32 size, s32 *pos, u32 option);

#endif // POKEBW2_TWL_SSP_PRIVATE_H
