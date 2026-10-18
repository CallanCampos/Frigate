// The sound driver's trace (--sound-trace FILE): the state of the host ARM7's sound driver (port/hal/snd7), which plays
// the game's music, sound effects and cries, after each run of its thread, in the format that pbw-oracle's
// --sound-trace writes for the DS's (port/tools/oracle/sndtrace.h), for port/tools/oracle/sound.py to compare with
// melonDS's. The program opens it from its options (port/app/main.c). Plain C types only.
#ifndef PORT_APP_SOUND_TRACE_H
#define PORT_APP_SOUND_TRACE_H

// Opens the file and starts the trace; returns 0, after printing why, when the file can't be written
int port_sound_trace_open(const char *path);
// Stops the trace and closes the file; returns 0 when a write failed
int port_sound_trace_close(void);

#endif // PORT_APP_SOUND_TRACE_H
