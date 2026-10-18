// The window's icon on Linux and Windows (port/app/window_icon.h), from the bitmap the build made of the version's app
// icon (port/tools/gen_window_icon.c)
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

SDL_Surface *port_window_icon_surface(void) {
    int size = port_window_icon_size;
    SDL_Surface *icon = SDL_CreateSurface(size, size, SDL_PIXELFORMAT_RGBA32);

    if (icon == NULL) {
        return NULL;
    }
    // Copied, so that nothing SDL does to the surface can reach the constant pixels
    for (int y = 0; y < size; y++) {
        memcpy((unsigned char *)icon->pixels + (size_t)y * (size_t)icon->pitch,
               port_window_icon_pixels + (size_t)y * (size_t)size * 4, (size_t)size * 4);
    }
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
