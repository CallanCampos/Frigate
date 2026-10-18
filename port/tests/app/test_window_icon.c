// Tests of the window's icon on Linux and Windows (port/app/window_icon.h): that the bitmaps the build made
// (port/tools/gen_window_icon.c, from port/assets/branding) are the version's, the 32 px one the window icon drawn for
// that size pixel for pixel (PORT_WINDOW_ICON_PNG, which SDL decodes here) and the 64 px one, its alternate for a
// display at 200%, the master scaled, both with the plate filling the square, and that giving the icon to a window goes
// through. The colours are the artwork's (its README.md): Black 2's icon is a dark plate with an ice-blue rig and a
// snow-white hull, and White 2's is a light plate with a steel-blue rig and an ink hull. The window is one of SDL's
// dummy video driver, which needs no display. On Linux and Windows, SDL converts the icon and, having nowhere to show
// it, says so, which the program logs; on macOS the window takes no icon, and the call returns at once.
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

// Expect, for a check of the bitmap named what
static void ExpectOf(const char *what, const char *check, int ok) {
    char name[128];

    snprintf(name, sizeof(name), "%s: %s", what, check);
    Expect(name, ok);
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

// The checks of one of the icon's bitmaps, named what: the plate fills the square, its rounded corners leaving the
// corners transparent and its straight edges reaching a pixel from the bitmap's, and the plate, the rig and the hull
// are the version's
static void CheckBitmap(const char *what, const SDL_Surface *icon, int n) {
    ExpectOf(what, "the surface is its size", icon->w == n && icon->h == n);
    ExpectOf(what, "the surface is RGBA", icon->format == SDL_PIXELFORMAT_RGBA32);
    if (icon->w != n || icon->h != n) {
        return;
    }
    ExpectOf(what, "top left corner transparent", Pixel(icon, 0, 0)[3] == 0);
    ExpectOf(what, "top right corner transparent", Pixel(icon, n - 1, 0)[3] == 0);
    ExpectOf(what, "bottom left corner transparent", Pixel(icon, 0, n - 1)[3] == 0);
    ExpectOf(what, "bottom right corner transparent", Pixel(icon, n - 1, n - 1)[3] == 0);
    ExpectOf(what, "left edge opaque", Pixel(icon, 1, n / 2)[3] == 255);
    ExpectOf(what, "right edge opaque", Pixel(icon, n - 2, n / 2)[3] == 255);
    ExpectOf(what, "top edge opaque", Pixel(icon, n / 2, 1)[3] == 255);
    ExpectOf(what, "bottom edge opaque", Pixel(icon, n / 2, n - 2)[3] == 255);

    // The version's plate, above the ship, and the version's rig and hull
    const unsigned char *plate = Pixel(icon, n / 2, 2);
    if (PLATE_IS_DARK) {
        ExpectOf(what, "the plate is Black 2's, dark", plate[0] < 64 && plate[1] < 64 && plate[2] < 64);
    } else {
        ExpectOf(what, "the plate is White 2's, light", plate[0] > 200 && plate[1] > 200 && plate[2] > 200);
    }
    ExpectOf(what, "the rig is the version's colour", HasColour(icon, RIG));
    ExpectOf(what, "the hull is the version's colour", HasColour(icon, HULL));
}

// Whether the bitmap's pixels are the PNG's, pixel for pixel (a transparent pixel's colour aside)
static int SameAsPng(const SDL_Surface *icon, const char *path) {
    SDL_Surface *loaded = SDL_LoadPNG(path);
    SDL_Surface *png = loaded == NULL ? NULL : SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    int same = png != NULL && png->w == icon->w && png->h == icon->h;

    if (png == NULL) {
        printf("%s: %s\n", path, SDL_GetError());
    }
    for (int y = 0; same && y < icon->h; y++) {
        for (int x = 0; same && x < icon->w; x++) {
            const unsigned char *a = Pixel(icon, x, y);
            const unsigned char *b = Pixel(png, x, y);
            same = a[3] == b[3] && (a[3] == 0 || (a[0] == b[0] && a[1] == b[1] && a[2] == b[2]));
        }
    }
    SDL_DestroySurface(png);
    SDL_DestroySurface(loaded);
    return same;
}

static void TestBitmap(void) {
    SDL_Surface *icon = port_window_icon_surface();
    Expect("the surface is made", icon != NULL);
    if (icon == NULL) {
        printf("SDL: %s\n", SDL_GetError());
        return;
    }
    int n = port_window_icon_size;
    Expect("the size is a window icon's at 100%", n == 32);
    CheckBitmap("at 100%", icon, n);
    Expect("at 100%: the window icon drawn for it, pixel for pixel", SameAsPng(icon, PORT_WINDOW_ICON_PNG));

    // The alternate for 200%, which SDL lists after the surface itself
    int count = 0;
    SDL_Surface **images = SDL_GetSurfaceImages(icon, &count);
    Expect("an alternate image", images != NULL && count == 2 && images[0] == icon);
    if (images != NULL && count == 2) {
        CheckBitmap("at 200%", images[1], n * 2);
    }
    SDL_free(images);

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
