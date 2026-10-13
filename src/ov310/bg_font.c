#include "types.h"
#include "app/research_radar/bg_font.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "system/printsys.h"

struct BGFont {
    BGFontSetup setup;
    Font *font;
    MsgData *msgData;
    BmpWin *window;
    BOOL visible;
};

static int BGFont_GetCenteredX(BGFont *bgFont, const StrBuf *str);

BGFont *BGFont_Create(const BGFontSetup *setup, Font *font, MsgData *msgData, HeapID heapId) {
    BGFont *bgFont = GFL_HeapAllocate(heapId, sizeof(BGFont), FALSE, "bg_font.c", 55);

    bgFont->setup = *setup;
    bgFont->font = font;
    bgFont->msgData = msgData;
    bgFont->window = BmpWin_CreateDynamic(setup->window.bg, setup->window.x, setup->window.y, setup->window.width,
                                          setup->window.height, setup->window.palette, FALSE);
    bgFont->visible = TRUE;
    return bgFont;
}

void BGFont_Delete(BGFont *bgFont) {
    BmpWin_Free(bgFont->window);
    GFL_HeapFree(bgFont);
}

void BGFont_PrintMsg(BGFont *bgFont, u32 strId) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(bgFont->msgData, strId);

    BGFont_PrintStr(bgFont, str);
    GFL_HeapFree(str);
}

void BGFont_PrintStr(BGFont *bgFont, const StrBuf *str) {
    u16 color = ((bgFont->setup.window.letterColor & 0x1f) << 10) | ((bgFont->setup.window.shadowColor & 0x1f) << 5) |
                (bgFont->setup.window.backColor & 0x1f);
    GFLBitmap *bitmap = BmpWin_GetBitmap(bgFont->window);
    int x;
    int y;

    GFL_BitmapFill(bitmap, bgFont->setup.window.backColor);
    if (bgFont->setup.centered) {
        x = bgFont->setup.window.textX + BGFont_GetCenteredX(bgFont, str);
        y = bgFont->setup.window.textY;
    } else {
        x = bgFont->setup.window.textX;
        y = bgFont->setup.window.textY;
    }
    GFL_TextRendererDrawToBitmapEx(bitmap, x, y, str, bgFont->font, color);
    if (bgFont->visible) {
        BmpWin_Transfer(bgFont->window);
    }
}

void BGFont_SetVisible(BGFont *bgFont, BOOL visible) {
    if (!bgFont->visible && visible) {
        BmpWin_Transfer(bgFont->window);
    } else if (bgFont->visible && !visible) {
        BmpWin *window = bgFont->window;

        BmpWin_ClearScreen(window);
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window));
    }
    bgFont->visible = visible;
}

void BGFont_SetPalette(BGFont *bgFont, u8 palette) {
    BmpWin_SetPalette(bgFont->window, palette);
    GFL_BGSysSetScrPaletteNo(BmpWin_GetBGIndex(bgFont->window), BmpWin_GetPosX(bgFont->window),
                             BmpWin_GetPosY(bgFont->window), BmpWin_GetWidth1(bgFont->window),
                             BmpWin_GetHeight2(bgFont->window), palette);
    if (bgFont->visible) {
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(bgFont->window));
    }
}

static int BGFont_GetCenteredX(BGFont *bgFont, const StrBuf *str) {
    int width = BmpWin_GetSizeX(bgFont->window) * 8;
    int x = (width - (int)GFL_FontGetBlockWidth(str, bgFont->font, 0)) * 0.5f;

    if (x < 0) {
        x = 0;
    }
    return x;
}
