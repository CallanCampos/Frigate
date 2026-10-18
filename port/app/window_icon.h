// The window's icon on Linux and Windows (port/app/window_icon.c): the version's icon, Frigate's ship on Black 2's dark
// plate or White 2's light one (port/assets/branding), which the taskbar and the window's title bar show. On macOS the
// window sets none: the app's own icon (port/app/macos/make_icns.sh) is the one the Dock shows. The pixels are made at
// build time by port/tools/gen_window_icon.c (port/cmake/app.cmake): the 32 px icon drawn pixel by pixel for the window
// (window-<version>-32.png), and the app icon's master scaled to twice that, so the program carries no PNG decoder.
#ifndef PORT_APP_WINDOW_ICON_H
#define PORT_APP_WINDOW_ICON_H

#include <SDL3/SDL.h>

// The bitmaps the build made, RGBA (SDL_PIXELFORMAT_RGBA32) with straight alpha, rows top to bottom: the icon at 100%
// display scale, port_window_icon_size pixels square, drawn for that size, and at 200%, twice the size, scaled from the
// master
extern const int port_window_icon_size;
extern const unsigned char port_window_icon_pixels[];
extern const unsigned char port_window_icon_pixels_2x[];

// The icon as a new surface of the 100% bitmap, with the 200% one as its alternate image
// (SDL_AddSurfaceAlternateImage), which SDL gives a display scaled above 100%. The caller destroys it, alternate and
// all. NULL, with SDL's error, when SDL can't make one
SDL_Surface *port_window_icon_surface(void);

// Gives the window its icon where the window takes one from the program, that is everywhere but macOS. A window that
// can't take it (a Wayland compositor without the icon protocol) goes on without one, and the program says so once
void port_window_icon_set(SDL_Window *window);

#endif // PORT_APP_WINDOW_ICON_H
