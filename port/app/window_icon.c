// The window's icon on Linux and Windows (port/app/window_icon.h), from the bitmaps the build made of the version's
// icon (port/tools/gen_window_icon.c): the one drawn for 32 px, and the master scaled to 64 px for a display at 200%.
// It touches no part of the game: it is the program's look in the taskbar.
#include "app/window_icon.h"
#include <string.h>

// Whether the window takes its icon from the program. On macOS the app's icon is the Dock's, and an icon set here would
// replace it with this small bitmap while the program runs. The code below is compiled everywhere all the same, so that
// a build on any platform checks it
#ifdef __APPLE__
#define WINDOW_TAKES_ICON 0
#else
#define WINDOW_TAKES_ICON 1
#endif

// A new surface of size x size RGBA pixels, copied, so that nothing SDL does to the surface can reach the constant ones
static SDL_Surface *BitmapSurface(const unsigned char *pixels, int size) {
    SDL_Surface *surface = SDL_CreateSurface(size, size, SDL_PIXELFORMAT_RGBA32);

    if (surface == NULL) {
        return NULL;
    }
    for (int y = 0; y < size; y++) {
        memcpy((unsigned char *)surface->pixels + (size_t)y * (size_t)surface->pitch,
               pixels + (size_t)y * (size_t)size * 4, (size_t)size * 4);
    }
    return surface;
}

SDL_Surface *port_window_icon_surface(void) {
    SDL_Surface *icon = BitmapSurface(port_window_icon_pixels, port_window_icon_size);
    SDL_Surface *large = BitmapSurface(port_window_icon_pixels_2x, port_window_icon_size * 2);

    // The icon takes its own reference to the alternate image, which goes with it
    if (icon == NULL || large == NULL || !SDL_AddSurfaceAlternateImage(icon, large)) {
        SDL_DestroySurface(icon);
        SDL_DestroySurface(large);
        return NULL;
    }
    SDL_DestroySurface(large);
    return icon;
}

void port_window_icon_set(SDL_Window *window) {
    if (!WINDOW_TAKES_ICON) {
        return;
    }
    SDL_Surface *icon = port_window_icon_surface();
    if (icon == NULL || !SDL_SetWindowIcon(window, icon)) {
        SDL_Log("port: the window has no icon: %s", SDL_GetError());
    }
    SDL_DestroySurface(icon);
}
