// The host's audio output (port/audio): the sound the host ARM7's sound unit makes (port/hal/snd7, snd7_output.h),
// played on the default playback device through an SDL3 audio stream, and/or written to a WAV file.
//
// The sound unit makes its samples in step with game time, on the game's thread at each VBlank, and queues them; the
// device's thread only takes them from the stream, which SDL resamples to the device's rate. The game's pace and the
// device's clock drift apart (and the stream's rate, 32,728 Hz, is 16 ppm below the exact 32,728.515625), so the
// stream's speed follows how much is queued, within half a percent, and a queue that grows past a second (the game ran
// ahead of real time) is emptied. None of that touches the samples themselves, which a WAV dump gets as they are made.
#ifndef PORT_AUDIO_AUDIO_H
#define PORT_AUDIO_AUDIO_H

#include <stdbool.h>

// Opens the default playback device. False, with a message, when there is none, and the game goes on silently
bool port_audio_open(void);

// Starts writing every sample to a WAV file at path (16-bit stereo PCM at PORT_SND7_SAMPLE_RATE): the program's
// --audio-dump PATH, with or without a device (--headless plays nothing). False, with a message, when the file can't
// be made
bool port_audio_dump_open(const char *path);

// Closes the device and finishes the WAV file's header. The program calls it once its run ended
void port_audio_close(void);

#endif // PORT_AUDIO_AUDIO_H
