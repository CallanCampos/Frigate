#include "types.h"
#include "gfl/areaman.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"

// Marks a valid window
#define BMPWIN_MAGIC 0x7fb2

struct BmpWinSys {
    HeapID heapId;
    u16 unused;
};

struct BmpWin {
    u16 magic : 15;
    u16 colorMode : 1;
    u8 bg;
    u8 x;
    u8 y;
    // The size of the bitmap, and of the area of the screen, in tiles
    u8 sizeX;
    u8 sizeY;
    u8 width;
    u8 height;
    u8 palette;
    u16 charPos;
    GFLBitmap *bitmap;
};

static BmpWin *BmpWin_CreateCore(u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette, u32 charPos, u8 colorMode);

static BmpWinSys *sBmpWinSys;

void BmpWin_InitAllocator(HeapID heapId) {
    sBmpWinSys = GFL_HeapAllocate(heapId, sizeof(BmpWinSys), FALSE, "bmp_win.c", 79);
    sBmpWinSys->heapId = heapId;
}

void BmpWin_FreeAllocator(void) {
    if (sBmpWinSys != NULL) {
        GFL_HeapFree(sBmpWinSys);
        sBmpWinSys = NULL;
    }
}

BmpWin *BmpWin_CreateDynamic(u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette, u8 fromEnd) {
    u8 colorMode;
    u32 size;
    u32 pos;

    if (GFL_BGSysGetBGColorPaletteMode(bg) == GX_BG_COLORMODE_16) {
        size = width * height;
        colorMode = GX_BG_COLORMODE_16;
    } else {
        size = width * height * 2;
        colorMode = GX_BG_COLORMODE_256;
    }
    pos = GFL_BGSysAllocChar(bg, size * BGSYS_TILE_SIZE_16, fromEnd);
    if (pos != AREAMAN_FAIL) {
        return BmpWin_CreateCore(bg, x, y, width, height, palette, pos, colorMode);
    }
    return NULL;
}

BmpWin *BmpWin_CreateStatic(u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette, u32 charPos) {
    u8 colorMode;
    u32 size;

    if (GFL_BGSysGetBGColorPaletteMode(bg) == GX_BG_COLORMODE_16) {
        size = width * height;
        colorMode = GX_BG_COLORMODE_16;
    } else {
        size = width * height * 2;
        colorMode = GX_BG_COLORMODE_256;
    }
    if (GFL_BGSysAllocCharAt(bg, charPos, size)) {
        return BmpWin_CreateCore(bg, x, y, width, height, palette, charPos, colorMode);
    }
    return NULL;
}

static BmpWin *BmpWin_CreateCore(u8 bg, u8 x, u8 y, u8 width, u8 height, u8 palette, u32 charPos, u8 colorMode) {
    u32 tileSize = BGSYS_TILE_SIZE_16;
    BmpWin *window;

    if (colorMode != GX_BG_COLORMODE_16) {
        tileSize = BGSYS_TILE_SIZE_256;
    }
    window = GFL_HeapAllocate(sBmpWinSys->heapId, sizeof(BmpWin), FALSE, "bmp_win.c", 213);

    window->magic = BMPWIN_MAGIC;
    window->bg = bg;
    window->x = x;
    window->y = y;
    window->sizeX = width;
    window->sizeY = height;
    window->width = width;
    window->height = height;
    window->palette = palette;
    window->charPos = charPos;
    window->colorMode = colorMode;
    window->bitmap = GFL_BitmapCreate(width, height, tileSize, sBmpWinSys->heapId);
    return window;
}

void BmpWin_Free(BmpWin *window) {
    u32 size;

    if (window->colorMode == GX_BG_COLORMODE_16) {
        size = window->sizeX * window->sizeY;
    } else {
        size = window->sizeX * window->sizeY * 2;
    }
    GFL_BGSysFreeCharMemory(window->bg, window->charPos, size * BGSYS_TILE_SIZE_16);
    GFL_BitmapFree(window->bitmap);
    GFL_HeapFree(window);
}

void BmpWin_FlushChar(BmpWin *window) {
    u32 size = window->sizeX * window->sizeY * GFL_BGSysGetBGBytesPerTile(window->bg);

    GFL_BGSysLoadChar(window->bg, GFL_BitmapGetPixelData(window->bitmap), size, window->charPos);
}

void BmpWin_FlushMap(BmpWin *window) {
    u16 count = window->sizeX * window->sizeY;
    u16 tile = window->charPos;
    u16 palette = window->palette << 12;
    void *screen;
    int i;

    if (GFL_BGSysGetBGMode(window->bg) != BGMODE_AFFINE) {
        u16 *map = GFL_HeapAllocate(HEAPID_TAIL(sBmpWinSys->heapId), count * 2, FALSE, "bmp_win.c", 307);

        for (i = 0; i < count; i++) {
            map[i] = tile | palette;
            tile++;
        }
        screen = map;
    } else {
        u8 *map = GFL_HeapAllocate(HEAPID_TAIL(sBmpWinSys->heapId), count, FALSE, "bmp_win.c", 318);

        for (i = 0; i < count; i++) {
            map[i] = tile;
            tile++;
        }
        screen = map;
    }
    GFL_BGSysLoadScrArea(window->bg, window->x, window->y, window->width, window->height, screen, 0, 0, window->sizeX,
                         window->sizeY);
    GFL_HeapFree(screen);
}

void BmpWin_MakeFrameScreen(BmpWin *window, u16 frameChar, u8 palette) {
    u16 pal;

    if (GFL_BGSysGetBGMode(window->bg) != BGMODE_AFFINE) {
        pal = palette << 12;
    } else {
        pal = 0;
    }
    GFL_BGSysFillScrArea(window->bg, pal | frameChar, window->x - 1, window->y - 1, 1, 1, BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysFillScrArea(window->bg, pal | (frameChar + 1), window->x, window->y - 1, window->width, 1,
                         BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysFillScrArea(window->bg, pal | (frameChar + 2), window->x + window->width, window->y - 1, 1, 1,
                         BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysFillScrArea(window->bg, pal | (frameChar + 3), window->x - 1, window->y, 1, window->height,
                         BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysFillScrArea(window->bg, pal | (frameChar + 4), window->x + window->width, window->y, 1, window->height,
                         BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysFillScrArea(window->bg, pal | (frameChar + 5), window->x - 1, window->y + window->height, 1, 1,
                         BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysFillScrArea(window->bg, pal | (frameChar + 6), window->x, window->y + window->height, window->width, 1,
                         BGSYS_FILL_TILE_PALETTE);
    GFL_BGSysFillScrArea(window->bg, pal | (frameChar + 7), window->x + window->width, window->y + window->height, 1, 1,
                         BGSYS_FILL_TILE_PALETTE);
}

void BmpWin_ClearScreen(BmpWin *window) {
    GFL_BGSysFillScrArea(window->bg, 0, window->x, window->y, window->sizeX, window->sizeY, 0);
}

u8 BmpWin_GetBGIndex(BmpWin *window) {
    return window->bg;
}

u8 BmpWin_GetSizeX(BmpWin *window) {
    return window->sizeX;
}

u8 BmpWin_GetSizeY(BmpWin *window) {
    return window->sizeY;
}

u8 BmpWin_GetWidth1(BmpWin *window) {
    return window->width;
}

u8 BmpWin_GetHeight2(BmpWin *window) {
    return window->height;
}

u8 BmpWin_GetPosX(BmpWin *window) {
    return window->x;
}

u8 BmpWin_GetPosY(BmpWin *window) {
    return window->y;
}

u16 BmpWin_GetCharPos(BmpWin *window) {
    return window->charPos;
}

GFLBitmap *BmpWin_GetBitmap(BmpWin *window) {
    return window->bitmap;
}

u8 BmpWin_GetPalette(BmpWin *window) {
    return window->palette;
}

void BmpWin_SetPosX(BmpWin *window, u8 x) {
    window->x = x;
}

void BmpWin_SetPosY(BmpWin *window, u8 y) {
    window->y = y;
}

void BmpWin_SetHeight2(BmpWin *window, u8 height) {
    window->height = height;
}

void BmpWin_SetPalette(BmpWin *window, u8 palette) {
    window->palette = palette;
}
