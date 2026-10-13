#include "types.h"
#include "app/ui/frame_list.h"
#include "app/ui/print_msg.h"
#include "app/zknlist_bmp.h"
#include "app/zknlist_main.h"
#include "constants/arc.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "nnsys/g2d.h"
#include "save/pokedex.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/game_start_3.h"

// The windows and text of the Pokédex's list. The ROM doesn't name this file; zknlist_bmp.c is a guess after the
// search screen's zknsearch_bmp.c

#define COLOR_ITEM PRINT_COLOR(14, 15, 0)
#define COLOR_BUTTON PRINT_COLOR(15, 2, 1)
#define COLOR_COUNT PRINT_COLOR(12, 13, 0)

// A window to create, as BmpWin_CreateDynamic takes it
typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} ZknListWindowData;

static void ZknListBmp_Print(ZknListWork *wk, u32 window, u32 msg, u32 x, u32 y, u16 color, u32 align);
static void ZknListBmp_PrintFormat(ZknListWork *wk, u32 window, u32 msg, u32 x, u32 y, u16 color, u32 align);
static void ZknListBmp_PutWindow(PrintWindow *window);
static void ZknListBmp_LoadWindowBG(ZknListWork *wk, u32 window);
static void ZknListBmp_PutTitle(ZknListWork *wk);
static void ZknListBmp_PutModes(ZknListWork *wk);

static const ZknListWindowData sZknListWindows[ZKNLIST_WIN_COUNT] = {
    { 5, 1, 0, 17, 3, 4 },  { 5, 2, 3, 13, 3, 4 },  { 5, 16, 3, 15, 3, 4 },
    { 0, 0, 12, 7, 3, 15 }, { 0, 0, 15, 7, 3, 15 }, { 0, 0, 18, 7, 3, 15 },
};

void ZknListBmp_Init(ZknListWork *wk) {
    u32 i;

    BmpWin_InitAllocator(HEAPID_ZUKAN_LIST);
    for (i = 0; i < ZKNLIST_WIN_COUNT; i++) {
        wk->windows[i].window =
            BmpWin_CreateDynamic(sZknListWindows[i].bg, sZknListWindows[i].x, sZknListWindows[i].y,
                                 sZknListWindows[i].width, sZknListWindows[i].height, sZknListWindows[i].palette, TRUE);
    }
    ZknListBmp_PutTitle(wk);
    ZknListBmp_PutModes(wk);
}

void ZknListBmp_Exit(ZknListWork *wk) {
    u32 i;

    for (i = 0; i < ZKNLIST_WIN_COUNT; i++) {
        BmpWin_Free(wk->windows[i].window);
    }
    BmpWin_FreeAllocator();
}

void ZknListBmp_Update(ZknListWork *wk) {
    u32 i;

    func_02021a3c(wk->printQueue);
    for (i = 0; i < ZKNLIST_WIN_COUNT; i++) {
        PrintWindow_Flush(&wk->windows[i], wk->printQueue);
    }
}

static void ZknListBmp_Print(ZknListWork *wk, u32 window, u32 msg, u32 x, u32 y, u16 color, u32 align) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, msg);

    PrintStrAligned(&wk->windows[window], wk->printQueue, x, y, strbuf, wk->font, color, align);
    GFL_StrBufFree(strbuf);
}

// Prints a message with the word set's words
static void ZknListBmp_PrintFormat(ZknListWork *wk, u32 window, u32 msg, u32 x, u32 y, u16 color, u32 align) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, msg);

    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, strbuf);
    PrintStrAligned(&wk->windows[window], wk->printQueue, x, y, wk->strbuf, wk->font, color, align);
    GFL_StrBufFree(strbuf);
}

static void ZknListBmp_PutWindow(PrintWindow *window) {
    BmpWin_FlushMap(window->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window->window));
}

// Copies the BG under a window of the top screen into it, for the text to go over
static void ZknListBmp_LoadWindowBG(ZknListWork *wk, u32 window) {
    NNSG2dCharacterData *charData;
    NNSG2dScreenData *screenData;
    void *charFile;
    u8 *chars;
    void *screenFile;
    u8 *pixels;
    u16 *screen;
    ArcTool *arc;
    u8 width, height;
    u8 posX, posY;
    u16 x, y;

    width = BmpWin_GetWidth1(wk->windows[window].window);
    height = BmpWin_GetHeight2(wk->windows[window].window);
    posX = BmpWin_GetPosX(wk->windows[window].window);
    posY = BmpWin_GetPosY(wk->windows[window].window);
    pixels = GFL_BitmapGetPixelData(BmpWin_GetBitmap(wk->windows[window].window));
    arc = GFL_ArcSysCreateFileHandle(ARCID_ZUKAN_GRA, HEAPID_ZUKAN_LIST);
    screenFile = GFL_G2DIOReadNSCRArc(arc, 71, FALSE, &screenData, HEAPID_ZUKAN_LIST | HEAPID_TAIL_BIT);
    screen = (u16 *)screenData->rawData;
    charFile = GFL_G2DIOReadBGNCGRArc(arc, 59, FALSE, &charData, HEAPID_ZUKAN_LIST | HEAPID_TAIL_BIT);
    chars = charData->rawData;
    GFL_ArcToolFree(arc);
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            sys_memcpy32(&chars[(screen[(posY + y) * 32 + posX + x] & 0x3ff) * 0x20], &pixels[(y * width + x) * 0x20],
                         0x20);
        }
    }
    GFL_HeapFree(charFile);
    GFL_HeapFree(screenFile);
}

static void ZknListBmp_PutTitle(ZknListWork *wk) {
    ZknListBmp_LoadWindowBG(wk, ZKNLIST_WIN_TITLE);
    if (func_0200d1f8(wk->param->pokedex) == 1) {
        ZknListBmp_Print(wk, ZKNLIST_WIN_TITLE, GameStart3_Text_NationalPokedex, 0, 4, COLOR_ITEM, PRINT_ALIGN_LEFT);
    } else {
        ZknListBmp_Print(wk, ZKNLIST_WIN_TITLE, GameStart3_Text_UnovaPokedex, 0, 4, COLOR_ITEM, PRINT_ALIGN_LEFT);
    }
    ZknListBmp_PutWindow(&wk->windows[ZKNLIST_WIN_TITLE]);
}

// The names of the Pokédexes on their buttons: without the National Pokédex, Unova's is on the second
static void ZknListBmp_PutModes(ZknListWork *wk) {
    BOOL national;
    BOOL habitat;

    if (wk->param->searched != FALSE) {
        return;
    }
    national = PokeDex_IsNationalObtained(wk->param->pokedex);
    habitat = PokeDex_IsHabitatListEnabled(wk->param->pokedex);
    if (national == TRUE && habitat == TRUE) {
        ZknListBmp_Print(wk, ZKNLIST_WIN_UNOVA, GameStart3_Text_Unova, 2, 7, COLOR_BUTTON, PRINT_ALIGN_LEFT);
        ZknListBmp_Print(wk, ZKNLIST_WIN_NATIONAL, GameStart3_Text_National, 2, 6, COLOR_BUTTON, PRINT_ALIGN_LEFT);
        ZknListBmp_Print(wk, ZKNLIST_WIN_HABITAT, GameStart3_Text_Habitat, 2, 5, COLOR_BUTTON, PRINT_ALIGN_LEFT);
        ZknListBmp_PutWindow(&wk->windows[ZKNLIST_WIN_UNOVA]);
        ZknListBmp_PutWindow(&wk->windows[ZKNLIST_WIN_NATIONAL]);
        ZknListBmp_PutWindow(&wk->windows[ZKNLIST_WIN_HABITAT]);
    } else if (national == FALSE && habitat == TRUE) {
        ZknListBmp_Print(wk, ZKNLIST_WIN_NATIONAL, GameStart3_Text_Unova, 2, 6, COLOR_BUTTON, PRINT_ALIGN_LEFT);
        ZknListBmp_Print(wk, ZKNLIST_WIN_HABITAT, GameStart3_Text_Habitat, 2, 5, COLOR_BUTTON, PRINT_ALIGN_LEFT);
        ZknListBmp_PutWindow(&wk->windows[ZKNLIST_WIN_NATIONAL]);
        ZknListBmp_PutWindow(&wk->windows[ZKNLIST_WIN_HABITAT]);
    }
}

void ZknListBmp_PutCounts(ZknListWork *wk) {
    u32 seen;
    u32 caught;

    if (func_0200d1f8(wk->param->pokedex) == 1) {
        seen = PokeDex_GetSeenNoNational(wk->param->pokedex);
        caught = PokeDex_GetCaughtNoNational(wk->param->pokedex);
    } else {
        seen = PokeDex_GetSeenNoUnova(wk->param->pokedex, HEAPID_ZUKAN_LIST | HEAPID_TAIL_BIT);
        caught = PokeDex_GetCaughtNoUnova(wk->param->pokedex, HEAPID_ZUKAN_LIST | HEAPID_TAIL_BIT);
    }
    ZknListBmp_LoadWindowBG(wk, ZKNLIST_WIN_SEEN);
    ZknListBmp_LoadWindowBG(wk, ZKNLIST_WIN_CAUGHT);
    ZknListBmp_Print(wk, ZKNLIST_WIN_SEEN, GameStart3_Text_Seen, 72, 4, COLOR_COUNT, PRINT_ALIGN_RIGHT);
    WordSetNumber(wk->wordSet, 0, seen, 3, 0, TRUE);
    ZknListBmp_PrintFormat(wk, ZKNLIST_WIN_SEEN, GameStart3_Text_Empty, 104, 4, COLOR_COUNT, PRINT_ALIGN_RIGHT);
    ZknListBmp_Print(wk, ZKNLIST_WIN_CAUGHT, GameStart3_Text_Obtained, 88, 4, COLOR_COUNT, PRINT_ALIGN_RIGHT);
    WordSetNumber(wk->wordSet, 0, caught, 3, 0, TRUE);
    ZknListBmp_PrintFormat(wk, ZKNLIST_WIN_CAUGHT, GameStart3_Text_Empty, 120, 4, COLOR_COUNT, PRINT_ALIGN_RIGHT);
    ZknListBmp_PutWindow(&wk->windows[ZKNLIST_WIN_SEEN]);
    ZknListBmp_PutWindow(&wk->windows[ZKNLIST_WIN_CAUGHT]);
}

void ZknListBmp_PutSearchCount(ZknListWork *wk) {
    ZknListBmp_LoadWindowBG(wk, ZKNLIST_WIN_CAUGHT);
    ZknListBmp_Print(wk, ZKNLIST_WIN_CAUGHT, GameStart3_Text_SearchResults, 88, 4, COLOR_COUNT, PRINT_ALIGN_RIGHT);
    WordSetNumber(wk->wordSet, 0, wk->param->count, 3, 0, TRUE);
    ZknListBmp_PrintFormat(wk, ZKNLIST_WIN_CAUGHT, GameStart3_Text_Empty, 120, 4, COLOR_COUNT, PRINT_ALIGN_RIGHT);
    ZknListBmp_PutWindow(&wk->windows[ZKNLIST_WIN_CAUGHT]);
}

void ZknListBmp_PrintItem(ZknListWork *wk, PrintWindow *window, StrBuf *name, u32 number) {
    PrintQueue *queue = FrameList_GetPrintQueue(wk->frameList);
    StrBuf *strbuf;

    // The list holds national numbers
    if (func_0200d1f8(wk->param->pokedex) != 1) {
        number = wk->regionalNumbers[number];
    }
    strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, GameStart3_Text_Empty_4);
    WordSetNumber(wk->wordSet, 0, number, 3, 2, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, strbuf);
    GFL_StrBufFree(strbuf);
    PrintStrAligned(window, queue, 0, 4, wk->strbuf, wk->font, COLOR_ITEM, PRINT_ALIGN_LEFT);
    PrintStrAligned(window, queue, 28, 4, name, wk->font, COLOR_ITEM, PRINT_ALIGN_LEFT);
}
