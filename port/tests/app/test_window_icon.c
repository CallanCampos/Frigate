// Tests of the window's icon on Linux and Windows (port/app/window_icon.h): that the bitmap the build made of the
// version's app icon (port/tools/gen_window_icon.c, from port/assets/branding) is the version's, with its plate filling
// the square, and that giving it to a window goes through. The colours are the artwork's (its README.md): Black 2's
// icon is a dark plate with an ice-blue rig and a snow-white hull, and White 2's is a light plate with a steel-blue rig
// and an ink hull. The window is one of SDL's dummy video driver, which needs no display. On Linux and Windows, SDL
// converts the icon and, having nowhere to show it, says so, which the program logs; on macOS the window takes no
// icon, and the call returns at once.
#include "app/window_icon.h"
#include <stdio.h>
#include <stdlib.h>

#if defined(BLACK2)
#define RIG 0x9dc0e0
#define HULL 0xf1f5f9
#define PLATE_IS_DARK 1
#elif defined(WHITE2)
#define RIG 0x4a7fb0
#define HULL 0x10151c
#define PLATE_IS_DARK 0
#else
#error "BLACK2 or WHITE2 must be defined"
#endif

// How far a channel may be from the artwork's colour: the bitmap's pixels inside a sail or the hull are averages of
// pixels of one colour, so only rounding moves them
#define TOLERANCE 3

static int sChecks;
static int sFailures;

static void Expect(const char *what, int ok) {
    sChecks++;
    if (!ok) {
        sFailures++;
        printf("FAIL %s\n", what);
    }
}

static const unsigned char *Pixel(const SDL_Surface *icon, int x, int y) {
    return (const unsigned char *)icon->pixels + (size_t)y * (size_t)icon->pitch + (size_t)x * 4;
}

// Whether an opaque pixel of the icon has the colour 0xRRGGBB, within TOLERANCE
static int HasColour(const SDL_Surface *icon, unsigned colour) {
    for (int y = 0; y < icon->h; y++) {
        for (int x = 0; x < icon->w; x++) {
            const unsigned char *p = Pixel(icon, x, y);
            if (p[3] == 255 && abs(p[0] - (int)(colour >> 16)) <= TOLERANCE &&
                abs(p[1] - (int)((colour >> 8) & 0xff)) <= TOLERANCE && abs(p[2] - (int)(colour & 0xff)) <= TOLERANCE) {
                return 1;
            }
        }
    }
    return 0;
}

static void TestBitmap(void) {
    SDL_Surface *icon = port_window_icon_surface();
    Expect("the surface is made", icon != NULL);
    if (icon == NULL) {
        printf("SDL: %s\n", SDL_GetError());
        return;
    }
    int n = port_window_icon_size;
    Expect("the size is a window icon's", n >= 16 && n <= 256);
    Expect("the surface is the bitmap's size", icon->w == n && icon->h == n);
    Expect("the surface is RGBA", icon->format == SDL_PIXELFORMAT_RGBA32);

    // The plate fills the square: its rounded corners leave the corners transparent, and its straight edges reach a
    // pixel from the bitmap's
    Expect("top left corner transparent", Pixel(icon, 0, 0)[3] == 0);
    Expect("top right corner transparent", Pixel(icon, n - 1, 0)[3] == 0);
    Expect("bottom left corner transparent", Pixel(icon, 0, n - 1)[3] == 0);
    Expect("bottom right corner transparent", Pixel(icon, n - 1, n - 1)[3] == 0);
    Expect("left edge opaque", Pixel(icon, 1, n / 2)[3] == 255);
    Expect("right edge opaque", Pixel(icon, n - 2, n / 2)[3] == 255);
    Expect("top edge opaque", Pixel(icon, n / 2, 1)[3] == 255);
    Expect("bottom edge opaque", Pixel(icon, n / 2, n - 2)[3] == 255);

    // The version's plate, above the ship, and the version's rig and hull
    const unsigned char *plate = Pixel(icon, n / 2, 2);
    if (PLATE_IS_DARK) {
        Expect("the plate is Black 2's, dark", plate[0] < 64 && plate[1] < 64 && plate[2] < 64);
    } else {
        Expect("the plate is White 2's, light", plate[0] > 200 && plate[1] > 200 && plate[2] > 200);
    }
    Expect("the rig is the version's colour", HasColour(icon, RIG));
    Expect("the hull is the version's colour", HasColour(icon, HULL));

    // Each surface is a copy of its own
    SDL_Surface *other = port_window_icon_surface();
    Expect("a second surface", other != NULL && other->pixels != icon->pixels);
    SDL_DestroySurface(other);
    SDL_DestroySurface(icon);
}

static void TestWindow(void) {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        Expect("SDL's dummy video driver starts", 0);
        printf("SDL_Init: %s\n", SDL_GetError());
        return;
    }
    SDL_Window *window = SDL_CreateWindow("window icon test", 64, 64, SDL_WINDOW_HIDDEN);
    Expect("a window", window != NULL);
    if (window != NULL) {
        port_window_icon_set(window);
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

int main(void) {
    TestBitmap();
    TestWindow();
    printf("%d checks, %d failed\n", sChecks, sFailures);
    return sFailures != 0;
}
