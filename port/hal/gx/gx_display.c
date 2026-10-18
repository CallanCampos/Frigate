// What the two screens show (port/hal/gx/gx_display.h): which 2D engine feeds the top screen and which the bottom, and
// whether the LCDs and the engines are powered, read from POWCNT1 as the game's GX calls left it, for the screens'
// line drawing (port/gfx/screens.c). Every screen the game shows goes through this choice.
#include "hal/gx/gx_display.h"
#include "hal/hw/hw.h"
#include <stdint.h>
#include <string.h>

#define REG_POWCNT1 0x04000304u

void port_gx_display(PortGxDisplay *display) {
    uint16_t powcnt;

    memcpy(&powcnt, port_hw_pointer(REG_POWCNT1), sizeof(powcnt));
    display->lcdOn = powcnt & 1;
    display->topEngine = (powcnt >> 15) & 1 ? 0 : 1;
    display->enginePowered[0] = (powcnt >> 1) & 1;
    display->enginePowered[1] = (powcnt >> 9) & 1;
}
