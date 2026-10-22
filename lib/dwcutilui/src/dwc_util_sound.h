#ifndef POKEBW2_DWCUTIL_DWC_UTIL_SOUND_H
#define POKEBW2_DWCUTIL_DWC_UTIL_SOUND_H

#include "types.h"

// The connection utility's sound: the sound archive of its files, sound/sound_data.sdat, with one player. The sound
// effects are the archive's sequence archive 0.
// The ROM does not embed this source file's name; dwc_util_sound.c is descriptive.
// Starts the sound and ends it, around the main loop
void DWCUtilSound_Init(void);
void DWCi_SNDlEnd(void);
// Starts a sound effect
void DWCUtilSound_PlaySe(int seq);
// Sets the volume and the pitch of the tracks of a mask of the sound effect that plays
void DWCUtilSound_SetVolume(int volume);
void DWCUtilSound_SetTrackPitch(u16 trackMask, int pitch);
// Stops the sound effect
void DWCUtilSound_StopSe(void);

#endif // POKEBW2_DWCUTIL_DWC_UTIL_SOUND_H
