// The DS's two screens as the port draws them (port/gfx/screens.h): the OS core's scanline hook, which draws each
// visible line of both 2D engines from the state the game left (port/gfx/ref/ds2d.c), sends each line to the screen
// POWCNT1 gives its engine, feeds engine A's to display capture, and has the 3D frame drawn before line 0. Every
// screen of the game, raster effects included, is drawn here. With --frame-times, the 3D frame, the 2D lines and
// capture are timed here (port/hal/os/frame_time.h).
#include "gfx/screens.h"
#include "gfx/capture.h"
#include "gfx/ge/ge_hal.h"
#include "gfx/ref/ds2d.h"
#include "hal/gx/gx_display.h"
#include "hal/gx/vram.h"
#include "hal/os/frame_time.h"
#include "hal/os/port_os.h"
#include <string.h>

// What a screen shows while the LCDs are off (POWCNT1 bit 0 clear, as before GX_Init): no picture
#define COLOR_BLACK 0xff000000u
// What engine B's screen shows while the engine is off (POWCNT1 bit 9 clear). GBATEK doesn't say; melonDS measured
// white, and that engine A goes on drawing while it is off, which this follows (ds2d.h: melonDS is the oracle)
#define COLOR_WHITE 0xffffffffu

static uint32_t sScreens[2][PORT_SCREEN_HEIGHT * PORT_SCREEN_WIDTH];
// Each engine's source, kept from line to line: port_vram_fill_source leaves the 3D layer's line (bg0_3d_line) alone,
// which this file sets for engine A from the 3D renderer
static PortDs2dSource sSources[2];

static void Fill(uint32_t *line, uint32_t color) {
    for (int x = 0; x < PORT_SCREEN_WIDTH; x++) {
        line[x] = color;
    }
}

void port_screens_draw_line(int line) {
    PortGxDisplay display;

    if (line < 0 || line >= PORT_SCREEN_HEIGHT) {
        return;
    }
    port_gx_display(&display);
    // The 3D picture, drawn whole before line 0 from the list and registers of the last VBlank (port/gfx/ge/ge_hal.h);
    // engine A shows its lines as BG0
    if (line == 0) {
        uint64_t begin = port_frame_time_begin();
        port_g3d_render_frame();
        port_frame_time_end(PORT_FRAME_TIME_RENDER3D, begin);
    }
    sSources[0].bg0_3d_line = port_g3d_line(line);
    for (int engine = 0; engine < 2; engine++) {
        int screen = engine == display.topEngine ? PORT_SCREEN_TOP : PORT_SCREEN_BOTTOM;
        uint32_t *out = &sScreens[screen][line * PORT_SCREEN_WIDTH];
        uint64_t begin = port_frame_time_begin();

        // Each engine draws every line, whatever its screen shows, so that the state the renderer carries from line to
        // line (the affine reference points, mosaic, windows) moves as on the DS
        if (line == 0) {
            port_ds2d_begin_frame(engine);
        }
        port_vram_fill_source(&sSources[engine], engine);
        port_ds2d_render_line(&sSources[engine], engine, line, out);
        port_frame_time_end(PORT_FRAME_TIME_RENDER2D, begin);
        if (engine == 0) {
            begin = port_frame_time_begin();
            port_capture_draw_line(&sSources[0], line);
            port_frame_time_end(PORT_FRAME_TIME_CAPTURE, begin);
        }
        if (!display.lcdOn) {
            Fill(out, COLOR_BLACK);
        } else if (engine == 1 && !display.enginePowered[1]) {
            Fill(out, COLOR_WHITE);
        }
    }
}

void port_screens_attach(void) {
    memset(sSources, 0, sizeof(sSources));
    port_capture_reset();
    // What the screens show before the game draws a line: white, as the firmware leaves them (the engines on with
    // DISPCNT 0, which melonDS's direct boot gives too). The game starts inside line 0 (port/hal/os), so the first
    // frame shows this on line 0
    for (int screen = 0; screen < 2; screen++) {
        for (int line = 0; line < PORT_SCREEN_HEIGHT; line++) {
            Fill(&sScreens[screen][line * PORT_SCREEN_WIDTH], COLOR_WHITE);
        }
    }
    port_os_scanline_hook = port_screens_draw_line;
}

const uint32_t *port_screens_pixels(int screen) {
    return sScreens[screen == PORT_SCREEN_BOTTOM ? PORT_SCREEN_BOTTOM : PORT_SCREEN_TOP];
}
