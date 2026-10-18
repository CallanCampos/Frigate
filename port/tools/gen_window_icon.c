// Makes the window's icon on Linux and Windows at build time (port/cmake/app.cmake runs it): the version's window icon
// (port/assets/branding/window-black2-32.png or window-white2-32.png, Frigate's ship drawn pixel by pixel for 32 px on
// a plate that fills the square) and its app icon master (icon-black2-1024.png or icon-white2-1024.png) as two RGBA
// bitmaps in a C file, which port/app/window_icon.c gives the window: the first for 100% display scale, the second,
// twice its size, for 200%. The program then needs no PNG decoder and carries 20 KiB of pixels instead of the PNGs.
//
//   gen_window_icon WINDOW.png MASTER.png OUT.c
//
// The window icon is copied as it is: it is drawn for its size, whole pixels and all. The master is on Apple's macOS
// icon grid, its plate inside a transparent margin, and window icons on Linux and Windows fill their square, so the
// margin is trimmed: the larger bitmap is the smallest square of SIZE * k master pixels (k whole) centred on what the
// master draws, and each of its pixels is the average of k x k of the master's, weighted by their alpha so that the
// transparent margin doesn't darken the plate's edge. SDL decodes both (SDL_LoadPNG, SDL 3.4).
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>

// The smallest and largest window icon the program takes; the larger bitmap is twice its size
#define MIN_SIZE 16
#define MAX_SIZE 128

// The rectangle of the master's pixels whose alpha isn't 0, as [x0, x1) x [y0, y1); 0 when there is none
static int DrawnBounds(const SDL_Surface *master, int *x0, int *y0, int *x1, int *y1) {
    *x0 = master->w;
    *y0 = master->h;
    *x1 = 0;
    *y1 = 0;
    for (int y = 0; y < master->h; y++) {
        const uint8_t *row = (const uint8_t *)master->pixels + (size_t)y * (size_t)master->pitch;
        for (int x = 0; x < master->w; x++) {
            if (row[x * 4 + 3] != 0) {
                *x0 = SDL_min(*x0, x);
                *y0 = SDL_min(*y0, y);
                *x1 = SDL_max(*x1, x + 1);
                *y1 = SDL_max(*y1, y + 1);
            }
        }
    }
    return *x1 > *x0;
}

// The bitmap's pixel at (x, y): the alpha-weighted average of the k x k master pixels from (left + x * k, top + y * k),
// where a pixel outside the master counts as transparent. RGBA, straight alpha, as the master
static void BoxAverage(const SDL_Surface *master, int left, int top, int k, int x, int y, uint8_t out[4]) {
    uint64_t alpha = 0;
    uint64_t colour[3] = { 0, 0, 0 };

    for (int my = top + y * k; my < top + (y + 1) * k; my++) {
        for (int mx = left + x * k; mx < left + (x + 1) * k; mx++) {
            if (mx < 0 || my < 0 || mx >= master->w || my >= master->h) {
                continue;
            }
            const uint8_t *pixel = (const uint8_t *)master->pixels + (size_t)my * (size_t)master->pitch + mx * 4;
            alpha += pixel[3];
            for (int c = 0; c < 3; c++) {
                colour[c] += (uint64_t)pixel[c] * pixel[3];
            }
        }
    }
    for (int c = 0; c < 3; c++) {
        out[c] = alpha == 0 ? 0 : (uint8_t)((colour[c] + alpha / 2) / alpha);
    }
    out[3] = (uint8_t)((alpha + (uint64_t)(k * k) / 2) / (uint64_t)(k * k));
}

// A PNG as RGBA (SDL_PIXELFORMAT_RGBA32, straight alpha), or NULL after saying why
static SDL_Surface *LoadRgba(const char *path) {
    SDL_Surface *loaded = SDL_LoadPNG(path);
    if (loaded == NULL) {
        fprintf(stderr, "gen_window_icon: %s: %s\n", path, SDL_GetError());
        return NULL;
    }
    SDL_Surface *rgba = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (rgba == NULL) {
        fprintf(stderr, "gen_window_icon: %s: %s\n", path, SDL_GetError());
    }
    return rgba;
}

// Writes size x size pixels of source as the array name, four pixels a line: each the alpha-weighted average of the
// k x k block at (left + x * k, top + y * k), so with k 1 the pixels as they are (a transparent one as 0, 0, 0, 0)
static void WriteBitmap(FILE *out, const char *name, int size, const SDL_Surface *source, int left, int top, int k) {
    fprintf(out, "const unsigned char %s[%d * %d * 4] = {\n", name, size, size);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            uint8_t pixel[4];
            BoxAverage(source, left, top, k, x, y, pixel);
            fprintf(out, "%s0x%02x, 0x%02x, 0x%02x, 0x%02x,%s", x % 4 == 0 ? "    " : " ", pixel[0], pixel[1], pixel[2],
                    pixel[3], x % 4 == 3 ? "\n" : "");
        }
    }
    fprintf(out, "};\n");
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: gen_window_icon WINDOW.png MASTER.png OUT.c\n");
        return 2;
    }
    const char *windowPath = argv[1];
    const char *masterPath = argv[2];
    const char *outPath = argv[3];

    SDL_Surface *window = LoadRgba(windowPath);
    if (window == NULL) {
        return 1;
    }
    int size = window->w;
    if (window->h != size || size < MIN_SIZE || size > MAX_SIZE) {
        fprintf(stderr, "gen_window_icon: %s must be square, %d to %d pixels, not %d x %d\n", windowPath, MIN_SIZE,
                MAX_SIZE, window->w, window->h);
        SDL_DestroySurface(window);
        return 1;
    }
    SDL_Surface *master = LoadRgba(masterPath);
    if (master == NULL) {
        SDL_DestroySurface(window);
        return 1;
    }

    // The master's square to scale to twice the window icon's size: whole k x k blocks, centred on what it draws
    int x0, y0, x1, y1;
    if (!DrawnBounds(master, &x0, &y0, &x1, &y1)) {
        fprintf(stderr, "gen_window_icon: %s is transparent everywhere\n", masterPath);
        SDL_DestroySurface(window);
        SDL_DestroySurface(master);
        return 1;
    }
    int large = size * 2;
    int extent = SDL_max(x1 - x0, y1 - y0);
    int k = (extent + large - 1) / large;
    int side = large * k;
    int left = (x0 + x1 - side) / 2;
    int top = (y0 + y1 - side) / 2;

    FILE *out = fopen(outPath, "w");
    if (out == NULL) {
        fprintf(stderr, "gen_window_icon: can't write %s\n", outPath);
        SDL_DestroySurface(window);
        SDL_DestroySurface(master);
        return 1;
    }
    fprintf(out, "// The window's icon (port/app/window_icon.h), RGBA with straight alpha, made by\n");
    fprintf(out, "// port/tools/gen_window_icon.c. Don't edit. At 100%%, %d x %d, from\n// %s;\n", size, size,
            windowPath);
    fprintf(out, "// at 200%%, %d x %d, from %s\n// (its %d x %d pixels from %d, %d, %d to a pixel).\n", large, large,
            masterPath, side, side, left, top, k);
    fprintf(out, "#include \"app/window_icon.h\"\n\n");
    fprintf(out, "const int port_window_icon_size = %d;\n", size);
    // The window icon as it is: blocks of one pixel
    WriteBitmap(out, "port_window_icon_pixels", size, window, 0, 0, 1);
    WriteBitmap(out, "port_window_icon_pixels_2x", large, master, left, top, k);
    SDL_DestroySurface(window);
    SDL_DestroySurface(master);
    int failed = ferror(out);
    if (fclose(out) != 0 || failed) {
        fprintf(stderr, "gen_window_icon: can't write %s\n", outPath);
        remove(outPath);
        return 1;
    }
    return 0;
}
