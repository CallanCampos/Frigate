// The host's touch screen, for the program's input (port/hal/tp). The program reports where the player touches the
// DS's bottom screen, from the mouse or a touch screen through the inverse of its screen layout, and the host's ARM7
// (tp7.c) samples it whenever the game asks, as the DS's ARM7 samples the touch panel. Plain C types only, so that
// files without the decomp's headers (port/app, port/gfx) can include it.
#ifndef PORT_HAL_TP_TP_HOST_H
#define PORT_HAL_TP_TP_HOST_H

// Whether the bottom screen is touched, and where, in its pixels (x 0-255, y 0-191; others are clamped to the
// screen). The state holds until the next call; set it before the frame's hooks end, at the latest in the program's
// own frame hook, so that the frame's samples have it
void port_tp_set_touch(int touching, int x, int y);

// The state as last set
void port_tp_get_touch(int *touching, int *x, int *y);

#endif // PORT_HAL_TP_TP_HOST_H
