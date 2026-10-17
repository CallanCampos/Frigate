// What the two screens show, for the presentation: which 2D engine feeds which screen, and the power of the LCDs and of
// the engines, from POWCNT1 (0x04000304, GBATEK "DS Power Control"). The rest of the display state is in the engines'
// registers, which the renderer reads through port_vram_fill_source. Plain C types only
#ifndef PORT_HAL_GX_GX_DISPLAY_H
#define PORT_HAL_GX_GX_DISPLAY_H

typedef struct PortGxDisplay {
    // Bit 0: both LCDs on
    int lcdOn;
    // Bit 15, the display swap: engine A (0) goes to the top screen when set, as GX_Init leaves it
    // (GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB)), else engine B (1) does
    int topEngine;
    // Bits 1 and 9: the 2D engines A and B. GBATEK doesn't say what an engine's screen shows while it is off
    int enginePowered[2];
} PortGxDisplay;

void port_gx_display(PortGxDisplay *display);

#endif // PORT_HAL_GX_GX_DISPLAY_H
