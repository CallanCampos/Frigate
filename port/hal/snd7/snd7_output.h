// The sound that the host ARM7's sound unit produces (port/hal/snd7), the game's music, sound effects and cries, for
// the host's audio (port/audio) and tests, and the driver's trace for the comparison with melonDS
// (port/app/sound_trace.c). Plain C types only, so that files outside the platform layer can include it.
//
// The sound unit makes one stereo sample each 1024 cycles of the virtual clock (port_os_cycles), as the DS's mixer
// does: every 512 ticks of its sound clock, which is half the 33,514,000 Hz bus clock, so 32,728.515625 samples a
// second. The samples follow game time, not the host's: they are made as the virtual clock passes them (at each
// VBlank, when the ARM9 sends the driver a command, and at the driver's next alarm, which the OS core runs as timed
// work), so the same run gives the same samples.
#ifndef PORT_HAL_SND7_SND7_OUTPUT_H
#define PORT_HAL_SND7_SND7_OUTPUT_H

#include <stdint.h>

#define PORT_SND7_CYCLES_PER_SAMPLE 1024u
// The rate as a whole number of samples a second, for a consumer that needs one (an SDL audio stream, a WAV header):
// 0.515625 below the exact rate, 16 parts per million
#define PORT_SND7_SAMPLE_RATE 32728
#define PORT_SND7_CHANNELS 2

// Where the samples go: count stereo frames, left then right, of signed 16 bits. Called at each VBlank with the frames
// made since the last (from the OS's frame hook, on the host's stack, never from a thread's fiber), from
// port_snd7_advance, and from port_snd7_set_output with the frames the last output hadn't had; the frames are only
// valid during the call. NULL drops them. One output at a time
typedef void (*PortSnd7Output)(const int16_t *frames, uint32_t count, void *user);
void port_snd7_set_output(PortSnd7Output output, void *user);

// The frames made since the driver started
uint64_t port_snd7_samples(void);

// For tests that drive the sound unit without the OS: the driver starts at a cycle of their own clock, and runs to
// another (as port_snd7_frame does at a VBlank, giving the output what it made)
void port_snd7_boot_at(uint64_t cycles);
void port_snd7_advance(uint64_t cycles);

// The driver's trace, to compare it with the DS's (port/tools/oracle/sndtrace.h, the program's --sound-trace): trace is
// called after each run of the driver's thread, periodic or not, wherever the virtual clock passed it (at a VBlank; in
// the ARM9's call that sent a word, on a thread's fiber; or at the driver's next alarm, as the OS core's timed work on
// the host's stack); NULL stops it. port_snd7_trace_record fills a record of PBW_SNDTRACE_RECORD_SIZE bytes with what
// the driver knows of the run just made: everything but the frame and GFL's frame count, which are the caller's
typedef void (*PortSnd7Trace)(int periodic, void *user);
void port_snd7_set_trace(PortSnd7Trace trace, void *user);
void port_snd7_trace_record(uint8_t *record, int periodic);

#endif // PORT_HAL_SND7_SND7_OUTPUT_H
