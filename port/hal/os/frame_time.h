// The host time each frame of a run costs, by part, for --frame-times (port/app/main.c): how long the program takes to
// make each of the DS's frames, the 16.715 ms it has at 59.8261 frames a second, and where that time goes. It touches
// no part of the game and nothing reads its numbers back, so a run with it makes the same frames as one without.
//
// The parts are timed where their work is: the 3D frame (port/gfx/screens.c, around port/gfx/ge/ge_hal.c's
// port_g3d_render_frame), the geometry engine running the logged commands (port_g3d_sync), the 2D engines' lines with
// their sources and display capture (port/gfx/screens.c), the ARM7's frame and the sound driver's alarms
// (port/hal/pxi/arm7.c, port/hal/os/core.c's timed work), the window's events and presentation, and the pacing wait
// (port/app/main.c). Parts never nest. What no part covers is "other": the game's own C on its fibers, the dispatcher,
// the interrupt handlers and the rest of the frame hooks (--run-dir's row, whose hashes of both screens land there).
// A frame ends with the program's frame hook, after its pacing: frame N runs from the end of frame N - 1's hook to the
// end of its own, the first from the start of the game's main(). port/tools/frame_times.py reads the file. Plain C
// types only, so that the renderers can include it.
#ifndef PORT_HAL_OS_FRAME_TIME_H
#define PORT_HAL_OS_FRAME_TIME_H

#include <stdint.h>

typedef enum PortFrameTimePart {
    PORT_FRAME_TIME_RENDER3D, // the 3D frame drawn before line 0, with every thread that draws it
    PORT_FRAME_TIME_GEOMETRY, // the geometry engine running the commands the game logged
    PORT_FRAME_TIME_RENDER2D, // the 2D engines' 192 lines and the sources each line is drawn from
    PORT_FRAME_TIME_CAPTURE,  // display capture, line by line
    PORT_FRAME_TIME_ARM7,     // the ARM7's frame (touch, the sound driver catching up, the microphone) and its alarms
    PORT_FRAME_TIME_PRESENT,  // the window's events and the frame's presentation
    PORT_FRAME_TIME_PACE,     // waiting until the frame's time on the DS's clock
    PORT_FRAME_TIME_PARTS
} PortFrameTimePart;

// Whether frames are being timed: set by port_frame_time_open, so that a part's timing costs a test of it otherwise
extern int port_frame_time_on;

// The host's monotonic clock in nanoseconds when frames are timed, else 0: the start of a part
uint64_t port_frame_time_begin(void);
// Adds the time since begin, a value port_frame_time_begin returned, to the frame's part
void port_frame_time_end(PortFrameTimePart part, uint64_t begin);

// Starts timing: the frames' rows are kept in memory and written to path by port_frame_time_close. 0 when path can't
// be written, which it tries at once
int port_frame_time_open(const char *path);
// Starts the first frame's time: just before the game's main() runs
void port_frame_time_start(void);
// Ends a frame, the program's frame number of which is frame, and starts the next
void port_frame_time_frame(uint32_t frame);
// Writes the rows, one a frame, and prints how long the frames took without the pacing wait (median, 95th percentile,
// most) to stderr. 0 when the file couldn't be written. A run that a trap, a crash or the watchdog ends writes nothing
int port_frame_time_close(void);

#endif // PORT_HAL_OS_FRAME_TIME_H
