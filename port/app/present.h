// The program's window, presentation and input (port/app/present.c): the DS's two screens stacked in one SDL window,
// the keyboard, a gamepad and the mouse as the DS's buttons and touch screen, and PNG screenshots of the screens.
// Everything runs on the program's one thread, from its frame hook on the dispatcher's stack (port/app/main.c).
#ifndef PORT_APP_PRESENT_H
#define PORT_APP_PRESENT_H

// Opens the window with its renderer and the screens' texture, and gamepad input. Returns 0 and logs why when SDL
// can't
int port_present_open(void);
// Closes what port_present_open opened
void port_present_close(void);

// Handles the window's events, then reports the buttons held and the touch to the platform layer (port_input_set,
// port_tp_set_touch). Asks the OS core to stop when the window is closed
void port_present_poll(void);

// Shows the screens as the scanline hook last drew them (port/gfx/screens.h)
void port_present_frame(void);

// Writes the two screens, the top one above the bottom one, as a 256x384 PNG. Returns 0 and logs why when it can't.
// Needs no window
int port_present_screenshot(const char *path);

#endif // PORT_APP_PRESENT_H
