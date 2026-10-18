// The window's icon on Linux and Windows (port/app/window_icon.c): the version's app icon, Frigate's ship on Black 2's
// dark plate or White 2's light one (port/assets/branding), which the taskbar and the window's title bar show. On macOS
// the window sets none: the app's own icon (port/app/macos/make_icns.sh) is the one the Dock shows. The pixels are made
// at build time from the icon's master by port/tools/gen_window_icon.c (port/cmake/app.cmake), so the program carries
// no PNG decoder for them.
#ifndef PORT_APP_WINDOW_ICON_H
#define PORT_APP_WINDOW_ICON_H

#include <SDL3/SDL.h>

// The bitmap the build made: port_window_icon_size pixels square, RGBA (SDL_PIXELFORMAT_RGBA32) with straight alpha,
// rows top to bottom
extern const int port_window_icon_size;
extern const unsigned char port_window_icon_pixels[];

// The icon as a new surface, which the caller destroys. NULL, with SDL's error, when SDL can't make one
SDL_Surface *port_window_icon_surface(void);

// Gives the window its icon where the window takes one from the program, that is everywhere but macOS. A window that
// can't take it (a Wayland compositor without the icon protocol) goes on without one, and the program says so once
void port_window_icon_set(SDL_Window *window);

#endif // PORT_APP_WINDOW_ICON_H
