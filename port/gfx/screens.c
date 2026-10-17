// The DS's two screens as the port draws them (port/gfx/screens.h)
#include "gfx/screens.h"
#include "gfx/ref/ds2d.h"
#include "hal/gx/gx_display.h"
#include "hal/gx/vram.h"
#include "hal/os/port_os.h"
#include <string.h>

// What a screen shows while the LCDs are off (POWCNT1 bit 0 clear, as before GX_Init): no picture
#define COLOR_BLACK 0xff000000u
// What engine B's screen shows while the engine is off (POWCNT1 bit 9 clear). GBATEK doesn't say; melonDS measured
// white, and that engine A goes on drawing while it is off, which this follows (ds2d.h: melonDS is the oracle)
#define COLOR_WHITE 0xffffffffu

static uint32_t sScreens[2][PORT_SCREEN_HEIGHT * PORT_SCREEN_WIDTH];
// Each engine's source, kept so that the 3D layer's line (bg0_3d_line, which port_vram_fill_source leaves alone)
// stays NULL until the 3D renderer exists: BG0 in 3D mode then shows as transparent
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
    for (int engine = 0; engine < 2; engine++) {
        int screen = engine == display.topEngine ? PORT_SCREEN_TOP : PORT_SCREEN_BOTTOM;
        uint32_t *out = &sScreens[screen][line * PORT_SCREEN_WIDTH];

        // Each engine draws every line, whatever its screen shows, so that the state the renderer carries from line to
        // line (the affine reference points, mosaic, windows) moves as on the DS
        if (line == 0) {
            port_ds2d_begin_frame(engine);
        }
        port_vram_fill_source(&sSources[engine], engine);
        port_ds2d_render_line(&sSources[engine], engine, line, out);
        if (!display.lcdOn) {
            Fill(out, COLOR_BLACK);
        } else if (engine == 1 && !display.enginePowered[1]) {
            Fill(out, COLOR_WHITE);
        }
    }
}

void port_screens_attach(void) {
    memset(sSources, 0, sizeof(sSources));
    for (int screen = 0; screen < 2; screen++) {
        for (int line = 0; line < PORT_SCREEN_HEIGHT; line++) {
            Fill(&sScreens[screen][line * PORT_SCREEN_WIDTH], COLOR_BLACK);
        }
    }
    port_os_scanline_hook = port_screens_draw_line;
}

const uint32_t *port_screens_pixels(int screen) {
    return sScreens[screen == PORT_SCREEN_BOTTOM ? PORT_SCREEN_BOTTOM : PORT_SCREEN_TOP];
}
