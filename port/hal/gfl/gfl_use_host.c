// GFL's startup and frame code on a host: the decomp's gfl_use.c, compiled as it is from its own path, so that the
// platform layer can read GFL's frame count (sVBlankCount), a static there, for input replays (port/app/replay.c).
// port/hal/gfl/replaced.txt keeps gfl_use.c out of the game's own sources.
#include "hal/gfl/gfl_frame.h"
#include "src/system/gfl_use.c"

uint32_t port_gfl_frame_count(void) {
    return sVBlankCount;
}
