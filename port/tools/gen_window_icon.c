// Makes the window's icon on Linux and Windows at build time (port/cmake/app.cmake runs it): the version's app icon
// master (port/assets/branding/icon-black2-1024.png or icon-white2-1024.png, Frigate's ship on its plate) as a small
// RGBA bitmap in a C file, which port/app/window_icon.c gives the window. The program then needs no PNG decoder and
// carries 16 KiB of pixels instead of the master.
//
//   gen_window_icon MASTER.png OUT.c SIZE
//
// The master is on Apple's macOS icon grid, its plate inside a transparent margin. Window icons on Linux and Windows
// fill their square, so the margin is trimmed: the bitmap is the smallest square of SIZE * k master pixels (k whole)
// centred on what the master draws, and each of its pixels is the average of k x k of the master's, weighted by their
// alpha so that the transparent margin doesn't darken the plate's edge. SDL decodes the master (SDL_LoadPNG, SDL 3.4).
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// The smallest and largest bitmap the program's window takes
#define MIN_SIZE 16
#define MAX_SIZE 256

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

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: gen_window_icon MASTER.png OUT.c SIZE\n");
        return 2;
    }
    const char *masterPath = argv[1];
    const char *outPath = argv[2];
    int size = atoi(argv[3]);
    if (size < MIN_SIZE || size > MAX_SIZE) {
        fprintf(stderr, "gen_window_icon: SIZE must be %d to %d, not %s\n", MIN_SIZE, MAX_SIZE, argv[3]);
        return 2;
    }

    SDL_Surface *loaded = SDL_LoadPNG(masterPath);
    if (loaded == NULL) {
        fprintf(stderr, "gen_window_icon: %s: %s\n", masterPath, SDL_GetError());
        return 1;
    }
    SDL_Surface *master = SDL_ConvertSurface(loaded, SDL_PIXELFORMAT_RGBA32);
    SDL_DestroySurface(loaded);
    if (master == NULL) {
        fprintf(stderr, "gen_window_icon: %s: %s\n", masterPath, SDL_GetError());
        return 1;
    }

    // The square to scale: whole k x k blocks, centred on what the master draws
    int x0, y0, x1, y1;
    if (!DrawnBounds(master, &x0, &y0, &x1, &y1)) {
        fprintf(stderr, "gen_window_icon: %s is transparent everywhere\n", masterPath);
        SDL_DestroySurface(master);
        return 1;
    }
    int extent = SDL_max(x1 - x0, y1 - y0);
    int k = (extent + size - 1) / size;
    int side = size * k;
    int left = (x0 + x1 - side) / 2;
    int top = (y0 + y1 - side) / 2;

    FILE *out = fopen(outPath, "w");
    if (out == NULL) {
        fprintf(stderr, "gen_window_icon: can't write %s\n", outPath);
        SDL_DestroySurface(master);
        return 1;
    }
    fprintf(out, "// The window's icon, %d x %d RGBA with straight alpha (port/app/window_icon.h), made from\n", size,
            size);
    fprintf(out, "// %s\n", masterPath);
    fprintf(out, "// (its %d x %d pixels from %d, %d, %d to a pixel) by port/tools/gen_window_icon.c. Don't edit.\n",
            side, side, left, top, k);
    fprintf(out, "#include \"app/window_icon.h\"\n\n");
    fprintf(out, "const int port_window_icon_size = %d;\n", size);
    fprintf(out, "const unsigned char port_window_icon_pixels[%d * %d * 4] = {\n", size, size);
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            uint8_t pixel[4];
            BoxAverage(master, left, top, k, x, y, pixel);
            fprintf(out, "%s0x%02x, 0x%02x, 0x%02x, 0x%02x,%s", x % 4 == 0 ? "    " : " ", pixel[0], pixel[1], pixel[2],
                    pixel[3], x % 4 == 3 ? "\n" : "");
        }
    }
    fprintf(out, "};\n");
    SDL_DestroySurface(master);
    int failed = ferror(out);
    if (fclose(out) != 0 || failed) {
        fprintf(stderr, "gen_window_icon: can't write %s\n", outPath);
        remove(outPath);
        return 1;
    }
    return 0;
}
