#include "types.h"
#include "gfl/dwc_rap.h"
#include "gfl/net.h"
#include "gfl/sound.h"

// The BGM's volume while a Wi-Fi connection may be using the sound, as during voice chat. One function between the
// friend checks and net_err.c that belongs to neither by what it uses; the ROM has no string for the file, so the
// name is a guess.

void func_02011bdc(u16 trackMask, s32 volume) {
    if (!func_02042b20() || func_ov011_02151dec() == 0) {
        GFL_SndBGMSetVolume(trackMask, volume);
    }
}
