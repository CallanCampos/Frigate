#ifndef POKEBW2_DWCUTIL_SOUND_H
#define POKEBW2_DWCUTIL_SOUND_H

#include "types.h"

// The connection utility's sound: the sound archive of its files, sound/sound_data.sdat, with one player. The sound
// effects are the archive's sequence archive 0.
// The file is the DWC utility library's sound.o (libdwcutil.TWL.HYB.a), as aletheia's map of Black 1's SDK code to that
// library's objects (layout/symbols_sdk.txt) shows it.
// Starts the sound and ends it, around the main loop
void DWCi_SNDlInit(void);
void DWCi_SNDlEnd(void);
// Starts a sound effect
void DWCi_SNDlPlay(int seq);
// Sets the volume and the pitch of the tracks of a mask of the sound effect that plays
void DWCi_SNDlSetVolume(int volume);
void DWCi_SNDlSetPitch(u16 trackMask, int pitch);
// Stops the sound effect
void DWCi_SNDlStop(void);

#endif // POKEBW2_DWCUTIL_SOUND_H
