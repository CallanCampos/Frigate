#include "types.h"
#include "app/pokedex_search_bmp.h"
#include "app/pokedex_search_main.h"
#include "app/ui/frame_list.h"
#include "app/ui/print_msg.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "system/printsys.h"
#include "text/system/zknsearch_main.h"

// The windows and text of the Pokédex's search screen. The ROM doesn't name this file; pokedex_search_bmp.c is a guess
// after pokedex_search_main.c

#define COLOR_WHITE PRINT_COLOR(15, 2, 0)
#define COLOR_BLACK PRINT_COLOR(1, 2, 0)
#define COLOR_MESSAGE PRINT_COLOR(1, 2, 6)
#define COLOR_ITEM PRINT_COLOR(14, 15, 0)

// A window to create, as BmpWin_CreateDynamic takes it
typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} PokedexSearchWindowData;

static void PokedexSearchBmp_Print(PokedexSearchWork *wk, u32 window, u32 msg, u32 x, u32 y, u16 color, u32 align);
static void PokedexSearchBmp_PutWindow(PrintWindow *window);
static void PokedexSearchBmp_PutLabels(PokedexSearchWork *wk);
static void PokedexSearchBmp_PutMessage(PokedexSearchWork *wk, u32 msg);

static const PokedexSearchWindowData sPokedexSearchWindows[POKEDEX_SEARCH_WIN_COUNT] = {
    { 7, 1, 0, 30, 3, 15 }, { 7, 1, 3, 30, 3, 15 },  { 1, 3, 0, 10, 3, 15 },  { 1, 3, 3, 10, 3, 15 },
    { 1, 3, 6, 10, 3, 15 }, { 1, 3, 9, 10, 3, 15 },  { 1, 3, 13, 10, 3, 15 }, { 1, 19, 0, 10, 3, 15 },
    { 1, 23, 3, 2, 3, 15 }, { 1, 16, 6, 16, 3, 15 }, { 1, 19, 9, 10, 3, 15 }, { 1, 19, 18, 10, 3, 15 },
    { 0, 3, 8, 10, 2, 15 }, { 0, 2, 11, 12, 2, 15 }, { 0, 2, 14, 12, 2, 15 }, { 1, 1, 18, 30, 2, 5 },
};

void PokedexSearchBmp_Init(PokedexSearchWork *wk) {
    u32 i;

    BmpWin_InitAllocator(HEAPID_POKEDEX_SEARCH);
    for (i = 0; i < POKEDEX_SEARCH_WIN_COUNT; i++) {
        wk->windows[i].window = BmpWin_CreateDynamic(
            sPokedexSearchWindows[i].bg, sPokedexSearchWindows[i].x, sPokedexSearchWindows[i].y,
            sPokedexSearchWindows[i].width, sPokedexSearchWindows[i].height, sPokedexSearchWindows[i].palette, TRUE);
    }
    PokedexSearchBmp_PutLabels(wk);
}

void PokedexSearchBmp_Exit(PokedexSearchWork *wk) {
    u32 i;

    for (i = 0; i < POKEDEX_SEARCH_WIN_COUNT; i++) {
        BmpWin_Free(wk->windows[i].window);
    }
    BmpWin_FreeAllocator();
}

void PokedexSearchBmp_Update(PokedexSearchWork *wk) {
    u32 i;

    PrintQueue_Main(wk->printQueue);
    for (i = 0; i < POKEDEX_SEARCH_WIN_COUNT; i++) {
        PrintWindow_Flush(&wk->windows[i], wk->printQueue);
    }
}

static void PokedexSearchBmp_Print(PokedexSearchWork *wk, u32 window, u32 msg, u32 x, u32 y, u16 color, u32 align) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, msg);

    PrintStrAligned(&wk->windows[window], wk->printQueue, x, y, strbuf, wk->font, color, align);
    GFL_StrBufFree(strbuf);
}

static void PokedexSearchBmp_PutWindow(PrintWindow *window) {
    BmpWin_FlushMap(window->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window->window));
}

static void PokedexSearchBmp_PutLabels(PokedexSearchWork *wk) {
    if (wk->param->criteria->regional == 0) {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_MODE, ZknsearchMain_Text_NationalPokedexSearchMode, 0, 4,
                               COLOR_WHITE, PRINT_ALIGN_LEFT);
    } else {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_MODE, ZknsearchMain_Text_UnovaPokedexSearchMode, 0, 4,
                               COLOR_WHITE, PRINT_ALIGN_LEFT);
    }
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_ORDER_LABEL, ZknsearchMain_Text_Order, 40, 4, COLOR_WHITE,
                           PRINT_ALIGN_CENTER);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_INITIAL_LABEL, ZknsearchMain_Text_Name, 40, 4, COLOR_WHITE,
                           PRINT_ALIGN_CENTER);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TYPE_LABEL, ZknsearchMain_Text_Type, 40, 4, COLOR_WHITE,
                           PRINT_ALIGN_CENTER);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_COLOR_LABEL, ZknsearchMain_Text_Color, 40, 4, COLOR_WHITE,
                           PRINT_ALIGN_CENTER);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_SHAPE_LABEL, ZknsearchMain_Text_Form, 40, 4, COLOR_WHITE,
                           PRINT_ALIGN_CENTER);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_START, ZknsearchMain_Text_Start, 40, 4, COLOR_WHITE,
                           PRINT_ALIGN_CENTER);
}

void PokedexSearchBmp_PutMain(PokedexSearchWork *wk) {
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_MODE]);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_ORDER_LABEL]);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_INITIAL_LABEL]);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_TYPE_LABEL]);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_COLOR_LABEL]);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_SHAPE_LABEL]);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_START]);
    PokedexSearchBmp_PutInfo(wk);

    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_ORDER].window), 0);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_ORDER, ZknsearchMain_Text_Number + wk->param->criteria->order, 40, 4,
                           COLOR_WHITE, PRINT_ALIGN_CENTER);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_ORDER]);

    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_INITIAL].window), 0);
    if (wk->param->criteria->initial == POKEDEX_SEARCH_NONE) {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_INITIAL, ZknsearchMain_Text_Empty_30, 8, 4, COLOR_WHITE,
                               PRINT_ALIGN_CENTER);
    } else {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_INITIAL, ZknsearchMain_Text_A + wk->param->criteria->initial, 8,
                               4, COLOR_WHITE, PRINT_ALIGN_CENTER);
    }
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_INITIAL]);

    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_TYPE].window), 0);
    if (wk->param->criteria->type1 == POKEDEX_SEARCH_NONE) {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TYPE, ZknsearchMain_Text_Empty_9, 64, 4, COLOR_WHITE,
                               PRINT_ALIGN_CENTER);
    } else if (wk->param->criteria->type2 == POKEDEX_SEARCH_NONE) {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TYPE,
                               PokedexSearchMain_GetTypeMsg(wk, wk->param->criteria->type1), 64, 4, COLOR_WHITE,
                               PRINT_ALIGN_CENTER);
    } else {
        // Two types, on either side of a separator
        StrBuf *strbuf;
        u8 width;
        u8 x;

        strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, ZknsearchMain_Text_Empty_10);
        PrintStrAligned(&wk->windows[POKEDEX_SEARCH_WIN_TYPE], wk->printQueue, 66, 4, strbuf, wk->font, COLOR_WHITE,
                        PRINT_ALIGN_CENTER);
        width = GFL_FontGetBlockWidth(strbuf, wk->font, 0);
        GFL_StrBufFree(strbuf);
        x = 64 - (u8)(width / 2);

        strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, PokedexSearchMain_GetTypeMsg(wk, wk->param->criteria->type1));
        PrintStrAligned(&wk->windows[POKEDEX_SEARCH_WIN_TYPE], wk->printQueue, x + 2, 4, strbuf, wk->font, COLOR_WHITE,
                        PRINT_ALIGN_RIGHT);
        GFL_StrBufFree(strbuf);
        x += width;

        strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, PokedexSearchMain_GetTypeMsg(wk, wk->param->criteria->type2));
        PrintStrAligned(&wk->windows[POKEDEX_SEARCH_WIN_TYPE], wk->printQueue, x + 2, 4, strbuf, wk->font, COLOR_WHITE,
                        PRINT_ALIGN_LEFT);
        GFL_StrBufFree(strbuf);
    }
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_TYPE]);

    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_COLOR].window), 0);
    if (wk->param->criteria->color == POKEDEX_SEARCH_NONE) {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_COLOR, ZknsearchMain_Text_Empty_31, 40, 4, COLOR_WHITE,
                               PRINT_ALIGN_CENTER);
    } else {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_COLOR, ZknsearchMain_Text_Red + wk->param->criteria->color, 40, 4,
                               COLOR_WHITE, PRINT_ALIGN_CENTER);
    }
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_COLOR]);
}

void PokedexSearchBmp_PutInfo(PokedexSearchWork *wk) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_INFO].window), 0);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_INFO, ZknsearchMain_Text_PleaseEnterSearchCriteria, 0, 4, COLOR_BLACK,
                           PRINT_ALIGN_LEFT);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_INFO]);
}

void PokedexSearchBmp_ClearInfo(PokedexSearchWork *wk) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_INFO].window), 0);
    BmpWin_FlushChar(wk->windows[POKEDEX_SEARCH_WIN_INFO].window);
}

void PokedexSearchBmp_PutOrder(PokedexSearchWork *wk) {
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_MODE]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_INFO].window), 0);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_INFO, ZknsearchMain_Text_PleaseEnterOrder, 0, 4, COLOR_BLACK,
                           PRINT_ALIGN_LEFT);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_INFO]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_TOP_LABEL].window), 0);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TOP_LABEL, ZknsearchMain_Text_Order, 40, 0, COLOR_WHITE,
                           PRINT_ALIGN_CENTER);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_TOP_LABEL]);
    PokedexSearchBmp_PutOrderValue(wk);
}

void PokedexSearchBmp_PutOrderValue(PokedexSearchWork *wk) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_TOP_VALUE].window), 0);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TOP_VALUE, ZknsearchMain_Text_Number + wk->param->criteria->order, 48,
                           0, COLOR_WHITE, PRINT_ALIGN_CENTER);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_TOP_VALUE]);
}

void PokedexSearchBmp_PutInitial(PokedexSearchWork *wk) {
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_MODE]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_INFO].window), 0);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_INFO, ZknsearchMain_Text_PleaseEnterFirstLetter, 0, 4, COLOR_BLACK,
                           PRINT_ALIGN_LEFT);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_INFO]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_TOP_LABEL].window), 0);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TOP_LABEL, ZknsearchMain_Text_Name, 40, 0, COLOR_WHITE,
                           PRINT_ALIGN_CENTER);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_TOP_LABEL]);
    PokedexSearchBmp_PutInitialValue(wk);
}

void PokedexSearchBmp_PutInitialValue(PokedexSearchWork *wk) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_TOP_VALUE].window), 0);
    if (wk->param->criteria->initial != POKEDEX_SEARCH_NONE) {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TOP_VALUE, ZknsearchMain_Text_A + wk->param->criteria->initial,
                               48, 0, COLOR_WHITE, PRINT_ALIGN_CENTER);
    } else {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TOP_VALUE, ZknsearchMain_Text_Empty_30, 48, 0, COLOR_WHITE,
                               PRINT_ALIGN_CENTER);
    }
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_TOP_VALUE]);
}

void PokedexSearchBmp_PutType(PokedexSearchWork *wk) {
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_MODE]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_INFO].window), 0);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_INFO, ZknsearchMain_Text_PleaseEnterType, 0, 4, COLOR_BLACK,
                           PRINT_ALIGN_LEFT);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_INFO]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_TOP_LABEL].window), 0);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TOP_LABEL, ZknsearchMain_Text_Type, 40, 0, COLOR_WHITE,
                           PRINT_ALIGN_CENTER);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_TOP_LABEL]);
    PokedexSearchBmp_PutTypeValue(wk);
}

void PokedexSearchBmp_PutTypeValue(PokedexSearchWork *wk) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_TOP_VALUE].window), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_TOP_VALUE2].window), 0);
    if (wk->param->criteria->type1 != POKEDEX_SEARCH_NONE) {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TOP_VALUE,
                               PokedexSearchMain_GetTypeMsg(wk, wk->param->criteria->type1), 48, 0, COLOR_WHITE,
                               PRINT_ALIGN_CENTER);
    } else {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TOP_VALUE, ZknsearchMain_Text_Empty_9, 48, 0, COLOR_WHITE,
                               PRINT_ALIGN_CENTER);
    }
    if (wk->param->criteria->type2 != POKEDEX_SEARCH_NONE) {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TOP_VALUE2,
                               PokedexSearchMain_GetTypeMsg(wk, wk->param->criteria->type2), 48, 0, COLOR_WHITE,
                               PRINT_ALIGN_CENTER);
    } else {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TOP_VALUE2, ZknsearchMain_Text_Empty_9, 48, 0, COLOR_WHITE,
                               PRINT_ALIGN_CENTER);
    }
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_TOP_VALUE]);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_TOP_VALUE2]);
}

void PokedexSearchBmp_PutColor(PokedexSearchWork *wk) {
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_MODE]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_INFO].window), 0);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_INFO, ZknsearchMain_Text_PleaseEnterColor, 0, 4, COLOR_BLACK,
                           PRINT_ALIGN_LEFT);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_INFO]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_TOP_LABEL].window), 0);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TOP_LABEL, ZknsearchMain_Text_Color, 40, 0, COLOR_WHITE,
                           PRINT_ALIGN_CENTER);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_TOP_LABEL]);
    PokedexSearchBmp_PutColorValue(wk);
}

void PokedexSearchBmp_PutColorValue(PokedexSearchWork *wk) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_TOP_VALUE].window), 0);
    if (wk->param->criteria->color != POKEDEX_SEARCH_NONE) {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TOP_VALUE, ZknsearchMain_Text_Red + wk->param->criteria->color,
                               48, 0, COLOR_WHITE, PRINT_ALIGN_CENTER);
    } else {
        PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TOP_VALUE, ZknsearchMain_Text_Empty_31, 48, 0, COLOR_WHITE,
                               PRINT_ALIGN_CENTER);
    }
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_TOP_VALUE]);
}

void PokedexSearchBmp_PutShape(PokedexSearchWork *wk) {
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_MODE]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_INFO].window), 0);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_INFO, ZknsearchMain_Text_PleaseEnterForm, 0, 4, COLOR_BLACK,
                           PRINT_ALIGN_LEFT);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_INFO]);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_TOP_LABEL].window), 0);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_TOP_LABEL, ZknsearchMain_Text_Form, 40, 0, COLOR_WHITE,
                           PRINT_ALIGN_CENTER);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_TOP_LABEL]);
}

void PokedexSearchBmp_PrintItem(PokedexSearchWork *wk, PrintWindow *window, StrBuf *strbuf) {
    PrintQueue *queue = FrameList_GetPrintQueue(wk->frameList);

    PrintStrAligned(window, queue, BmpWin_GetSizeX(window->window) * 8 / 2, 4, strbuf, wk->font, COLOR_ITEM,
                    PRINT_ALIGN_CENTER);
}

void PokedexSearchBmp_FlushItem(PokedexSearchWork *wk, PrintWindow *window) {
    BmpWin_FlushChar(window->window);
}

static void PokedexSearchBmp_PutMessage(PokedexSearchWork *wk, u32 msg) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->windows[POKEDEX_SEARCH_WIN_MESSAGE].window), 6);
    PokedexSearchBmp_Print(wk, POKEDEX_SEARCH_WIN_MESSAGE, msg, 120, 0, COLOR_MESSAGE, PRINT_ALIGN_CENTER);
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_MESSAGE]);
}

void PokedexSearchBmp_PutSearching(PokedexSearchWork *wk) {
    PokedexSearchBmp_PutMessage(wk, ZknsearchMain_Text_Searching);
}

void PokedexSearchBmp_PutFound(PokedexSearchWork *wk) {
    PokedexSearchBmp_PutMessage(wk, ZknsearchMain_Text_PokemonFound);
}

void PokedexSearchBmp_PutNotFound(PokedexSearchWork *wk) {
    PokedexSearchBmp_PutMessage(wk, ZknsearchMain_Text_NoPokemonFound);
}

void PokedexSearchBmp_PutStart(PokedexSearchWork *wk) {
    PokedexSearchBmp_PutWindow(&wk->windows[POKEDEX_SEARCH_WIN_START]);
}
