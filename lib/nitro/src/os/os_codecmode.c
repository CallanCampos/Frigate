// NitroSDK OS: whether the game runs with the DSi's codec in TWL mode (os_codecmode.c, a guessed name; TwlSDK's
// OSi_IsCodecTwlMode). It is the ROM header's codec flag, on a DSi.
#include "nitro/os.h"

// The flags byte of the TWL ROM header that the DSi's system puts in main memory, whose bit 0 is the codec mode
#define OSi_CODEC_FLAGS (HW_TWL_ROM_HEADER_BUF + 0x1bf)
#define OSi_CODEC_FLAGS_TWL_MODE 0x01

static BOOL OSi_CodecModeChecked;
static BOOL OSi_CodecIsTwlMode;

BOOL func_0207c210(void) {
    if (!OSi_CodecModeChecked) {
        OSi_CodecIsTwlMode = hw_isDSi() == TRUE && (*(u8 *)OSi_CODEC_FLAGS & OSi_CODEC_FLAGS_TWL_MODE) == TRUE;
    }
    OSi_CodecModeChecked = TRUE;
    return OSi_CodecIsTwlMode;
}
