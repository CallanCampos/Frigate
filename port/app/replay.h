// Input replays (--input FILE): the buttons, lid and touch an input file holds on each frame (the "pbwinput 1" format,
// port/tools/oracle/README.md), given to the game in place of the window's input, so that a run reaches the same
// moments of the game as the melonDS oracle given the same file (PLAN.md §6, T4). Frames count as the file says: game
// frames by GFL's frame count (port/hal/gfl/gfl_frame.h), or video frames. The port's console never sleeps through a
// VBlank (port/hal/pm wakes it at once), so every frame comes through port_replay_vblank. Plain C types only.
#ifndef PORT_APP_REPLAY_H
#define PORT_APP_REPLAY_H

#include <stdint.h>

// Loads the file. Returns 0, after printing why, when it can't be read or parsed
int port_replay_open(const char *path);
// Whether a file is loaded
int port_replay_active(void);
// The real-time clock at power-on for the replay, "YYYY-MM-DD HH:MM:SS": the file's "time" line, or the format's
// default when it has none, never the host's time, so that the oracle and the port start from the same moment
const char *port_replay_time(void);
// Whether the file pins the clock at that time for the whole run ("clock pinned"), as the oracle then does too
int port_replay_clock_pinned(void);
// The file's "version" line, "b2_us" or "w2_us", or NULL when it has none
const char *port_replay_version(void);
// What the file's frames count, "game" or "video", or NULL without a file
const char *port_replay_frames(void);

// Gives the game frame 1's state, which holds from power-on; before the game starts
void port_replay_start(void);
// At each VBlank, before the interrupt is taken (a frame hook), with or without a file: moves the game clock on and,
// with a file, gives the game the state of the frame whose pass reads the keys next. videoFrame is the VBlank's number
// since the game's main() started, from 1
void port_replay_vblank(uint32_t videoFrame);
// The game frame whose input the game reads after the last VBlank given to port_replay_vblank (1 before the first):
// frames.tsv's game_frame, as pbw-oracle counts it
uint32_t port_replay_game_frame(void);

void port_replay_close(void);

#endif // PORT_APP_REPLAY_H
