// The DS's two screens as the port draws them (PLAN.md §4.5, §4.6). At the start of each visible line the OS core's
// scanline hook (port/hal/os/port_os.h) draws that line of both 2D engines with the CPU reference renderer
// (port/gfx/ref/ds2d.h), from the registers, palettes, OAM and VRAM as the game left them (port_vram_fill_source), and
// sends each engine's line to the screen that POWCNT1 gives it (port/hal/gx/gx_display.h); engine A's line also goes
// to display capture (port/gfx/capture.h). The program presents the screens at each VBlank, when all 192 lines of the
// frame are drawn. Plain C types only.
#ifndef PORT_GFX_SCREENS_H
#define PORT_GFX_SCREENS_H

#include <stdint.h>

#define PORT_SCREEN_WIDTH 256
#define PORT_SCREEN_HEIGHT 192

enum {
    PORT_SCREEN_TOP = 0,
    PORT_SCREEN_BOTTOM = 1,
};

// Sets the OS core's scanline hook to this module's line drawing; called once before the game runs
void port_screens_attach(void);

// Called at the start of each frame, before line 0 is drawn (before the 3D frame and line 0's capture), when not NULL:
// the digest stream's look at what the VRAM banks hold of 3D as the frame starts (port/app/digest_stream.c), as
// pbw-oracle looks before melonDS's line 0. NULL unless --digest sets it; port_screens_attach leaves it as it is. The
// game starts inside line 0, so the first frame may have no call
extern void (*port_screens_frame_start_hook)(void);

// Draws line 0-191 of both engines into the screens, as the scanline hook does
void port_screens_draw_line(int line);

// A screen's pixels, 192 rows of 256, as 0xFFRRGGBB (SDL's ARGB8888): the last frame drawn, or the lines drawn so far
// of the frame being drawn
const uint32_t *port_screens_pixels(int screen);

#endif // PORT_GFX_SCREENS_H
