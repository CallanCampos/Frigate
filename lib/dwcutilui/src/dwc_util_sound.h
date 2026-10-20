#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SOUND_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SOUND_H

#include "types.h"

// The connection utility's sound: the sound archive of its files, sound/sound_data.sdat, with one player. The sound
// effects are the archive's sequence archive 0.
// The ROM does not embed this source file's name; dwc_util_sound.c is descriptive.
// Starts the sound and ends it, around the main loop
void func_ov182_021b8050(void);
void func_ov182_021b80d4(void);
// Starts a sound effect
void func_ov182_021b80f4(int seq);
// Sets the volume and the pitch of the tracks of a mask of the sound effect that plays
void func_ov182_021b810c(int volume);
void func_ov182_021b8120(u32 trackMask, int pitch);
// Stops the sound effect
void func_ov182_021b8138(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SOUND_H
