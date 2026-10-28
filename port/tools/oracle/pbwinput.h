// Input files ("pbwinput 1"): what the player holds on each frame, for the melonDS oracle (port/tools/oracle) and, for
// replays, the port itself. The format is in port/tools/oracle/README.md. This file and pbwinput.c are plain C11 with
// no dependency on melonDS or the decomp's headers, so that the port can compile them as they are.
//
// A file counts frames one of two ways, which its "frames" line says:
// - Game frames (the default): passes of the game's main loop. Game frame N is the pass after GFL's frame count
//   (sVBlankCount, src/system/gfl_use.c) has gone up N times since power-on; it goes up by one after each pass's VBlank
//   wait (GFL_VBlankUpdate), so this is the pass during which the count is N, until a soft reset restarts the count.
//   A video frame the console sleeps through (no VBlank) counts as one game frame, so that a later line can open the
//   lid. The DS drops frames under load and the port doesn't, so only game frames give both programs the same input
//   at the same moment of the game (port/docs/PLAN.md §6).
// - Video frames: frame N ends with the N-th VBlank since power-on (on the port, since the game's main()).
// Both programs apply a state at a VBlank, before the interrupt is taken, as pbw_input_frame_at_vblank says; the game
// reads it at the start of its next pass (GCTX_HIDUpdate). Legacy files hold frame 1 from power-on; an explicit
// initial header supplies a separate preboot state and holds until the first committed frame change.
//
// A file also sets the real-time clock: its "time" line is the time at power-on, and its "clock" line says whether the
// clock runs from there (the default) or stays pinned there for the whole run on both programs (PbwInputClock), for a
// screen that shows the time, such as the field menu's. And its "seed" lines give the random seeds the DS's game built
// from its console's entropy, which the port's game takes in their place, so that random outcomes (a battle's rolls, a
// wild Pokémon) are the DS's on both programs.
#ifndef PORT_TOOLS_ORACLE_PBWINPUT_H
#define PORT_TOOLS_ORACLE_PBWINPUT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// The buttons, with NitroSDK's PAD_* values, the same as port/hal/input/port_input.h's PORT_INPUT_*
#define PBW_INPUT_A 0x0001u
#define PBW_INPUT_B 0x0002u
#define PBW_INPUT_SELECT 0x0004u
#define PBW_INPUT_START 0x0008u
#define PBW_INPUT_RIGHT 0x0010u
#define PBW_INPUT_LEFT 0x0020u
#define PBW_INPUT_UP 0x0040u
#define PBW_INPUT_DOWN 0x0080u
#define PBW_INPUT_R 0x0100u
#define PBW_INPUT_L 0x0200u
#define PBW_INPUT_X 0x0400u
#define PBW_INPUT_Y 0x0800u
#define PBW_INPUT_DEBUG 0x2000u
#define PBW_INPUT_ALL 0x2fffu

// The touch screen's size in pixels: a touch is at x 0-255, y 0-191 of the bottom screen
#define PBW_INPUT_TOUCH_WIDTH 256
#define PBW_INPUT_TOUCH_HEIGHT 192

// The real-time clock at power-on for a run whose file has no "time" line and that isn't given one
#define PBW_INPUT_DEFAULT_TIME "2012-10-07 12:00:00"

// The longest frame number a file may name, so that every frame fits in 32 bits with room to add
#define PBW_INPUT_MAX_FRAME 100000000u

typedef struct PbwInputState {
    uint32_t buttons;  // PBW_INPUT_* bits of the buttons held
    uint8_t lidClosed; // nonzero while the lid is closed
    uint8_t touching;  // nonzero while the pen is down, at touchX, touchY
    uint8_t touchX;
    uint8_t touchY;
} PbwInputState;

// A state and the frame it starts on; it holds until the next change
typedef struct PbwInputChange {
    uint32_t frame;
    PbwInputState state;
} PbwInputChange;

// What a file's frame numbers count (its "frames" line)
typedef enum PbwInputFrames {
    PBW_INPUT_GAME_FRAMES = 0,  // passes of the game's main loop, by GFL's frame count (the default)
    PBW_INPUT_VIDEO_FRAMES = 1, // video frames since power-on
} PbwInputFrames;

// Whether the real-time clock runs from the time at power-on (its "clock" line)
typedef enum PbwInputClock {
    // It runs with the console's own time (the default): on the DS the video frames the game takes, which are more than
    // the port's where the DS's passes take longer, so the two programs' clocks drift apart against the game's count
    PBW_INPUT_CLOCK_RUNNING = 0,
    // It stays at the time at power-on for the whole run on both programs, so that a screen that shows the clock (the
    // field menu's bottom screen) shows the same time on both at the same count
    PBW_INPUT_CLOCK_PINNED = 1,
} PbwInputClock;

// The most "seed" lines a file may have
#define PBW_INPUT_MAX_SEEDS 64

typedef struct PbwInput {
    PbwInputChange *changes; // in increasing frame order
    size_t count;
    size_t capacity;
    // The "time" line: the real-time clock's time at power-on, "YYYY-MM-DD HH:MM:SS", or "" when the file has none
    char time[20];
    // The "version" line: "b2_us" or "w2_us", or "" when the file has none
    char version[8];
    // The "frames" line: PBW_INPUT_GAME_FRAMES when the file has none
    PbwInputFrames frames;
    // The "clock" line: PBW_INPUT_CLOCK_RUNNING when the file has none
    PbwInputClock clock;
    // The "seed" lines, in the file's order: the seeds the DS's game built from its console's entropy
    // (src/gfl/gf_standard.c's buildSeed: GFL's at start-up, then each battle's), which the port's game takes in
    // place of the ones it builds, the n-th line for the n-th seed (port/hal/gfl/random_seed.h). melonDS builds the
    // DS's own, so the oracle reads them only to check the file
    uint64_t seeds[PBW_INPUT_MAX_SEEDS];
    size_t seedCount;
    // Optional preboot state. Legacy files hold frame 1 at boot; explicit initial separates boot from VBlank 1.
    PbwInputState initial;
    int hasInitial;
} PbwInput;

// Counts game frames from GFL's frame count as a program reads it at each VBlank
typedef struct PbwGameClock {
    uint32_t frames;    // the game frames gone by: the steps of the count by one, and the VBlanks slept through
    uint32_t lastCount; // the count at the last VBlank
} PbwGameClock;

// An empty input: nothing held on any frame, no time and no version, on game frames, the clock running
void pbw_input_init(PbwInput *input);
void pbw_input_free(PbwInput *input);

// Parses a file's text into input (initialised here). Returns 1, or 0 with a message naming the line in error
int pbw_input_parse(PbwInput *input, const char *text, size_t length, char *error, size_t errorSize);
// Reads and parses a file
int pbw_input_load(PbwInput *input, const char *path, char *error, size_t errorSize);

// The state of a frame (1 or more): the last change at or before it; before the first change, explicit initial
// holds when present, otherwise nothing is held. Explicit-initial frame rows require a committing newline; only an
// unterminated trailing numeric FRAME row is ignored, so killed recordings retain complete samples.
PbwInputState pbw_input_state(const PbwInput *input, uint32_t frame);
// Explicit initial state, or frame 1 for legacy files.
PbwInputState pbw_input_initial_state(const PbwInput *input);

// Adds a change at the end, for a recorder: frame must be after the last change's. A state equal to the one that holds
// already is dropped. Returns 0 when frame is out of order or memory runs out
int pbw_input_append(PbwInput *input, uint32_t frame, const PbwInputState *state);

// Writes input in the file format. Returns 1, or 0 on a write error
int pbw_input_write(const PbwInput *input, FILE *file);
// Streaming pieces: no flush; the caller commits complete lines and handles errors.
int pbw_input_write_header(const PbwInput *input, FILE *file);
int pbw_input_write_change(FILE *file, uint32_t frame, const PbwInputState *state);

// The state's columns as a file line writes them ("A+START -", "- 128,96"); returns 0 when they don't fit
int pbw_input_format_state(const PbwInputState *state, char *out, size_t size);

// A game clock at power-on, before the game's first pass
void pbw_game_clock_init(PbwGameClock *clock);

// The frame whose state a program applies at a VBlank, before the interrupt is taken: on the file's count, the frame
// whose pass reads the keys next. videoFrame is the VBlank's number since power-on, count GFL's frame count as it is
// then (before the game's VBlank work increments it). For game frames, every VBlank must come through here, in order,
// with one clock for the run; the count is read even before the game clears it, when it holds anything: the clock
// moves only on a step of exactly one
uint32_t pbw_input_frame_at_vblank(const PbwInput *input, PbwGameClock *clock, uint32_t videoFrame, uint32_t count);
// The same for a video frame without a VBlank (the console asleep): on game frames it counts as one
uint32_t pbw_input_frame_asleep(const PbwInput *input, PbwGameClock *clock, uint32_t videoFrame);

// Whether text is a time the format accepts: "YYYY-MM-DD HH:MM:SS" from 2000 to 2099, a real date
int pbw_input_valid_time(const char *text);

#ifdef __cplusplus
}
#endif

#endif // PORT_TOOLS_ORACLE_PBWINPUT_H
