#include "types.h"
#include "app/pokedex_list_bmp.h"
#include "app/pokedex_list_main.h"
#include "app/ui/frame_list.h"
#include "app/ui/print_msg.h"
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
#include "save/pokedex_savedata.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/game_start_3.h"

// The windows and text of the Pokédex's list. The ROM doesn't name this file; pokedex_list_bmp.c is a guess after the
// search screen's pokedex_search_bmp.c

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
} PokedexListWindowData;

static void PokedexListBmp_Print(PokedexListWork *wk, u32 window, u32 msg, u32 x, u32 y, u16 color, u32 align);
static void PokedexListBmp_PrintFormat(PokedexListWork *wk, u32 window, u32 msg, u32 x, u32 y, u16 color, u32 align);
static void PokedexListBmp_PutWindow(PrintWindow *window);
static void PokedexListBmp_LoadWindowBG(PokedexListWork *wk, u32 window);
static void PokedexListBmp_PutTitle(PokedexListWork *wk);
static void PokedexListBmp_PutModes(PokedexListWork *wk);

static const PokedexListWindowData sPokedexListWindows[POKEDEX_LIST_WIN_COUNT] = {
    { 5, 1, 0, 17, 3, 4 },  { 5, 2, 3, 13, 3, 4 },  { 5, 16, 3, 15, 3, 4 },
    { 0, 0, 12, 7, 3, 15 }, { 0, 0, 15, 7, 3, 15 }, { 0, 0, 18, 7, 3, 15 },
};

void PokedexListBmp_Init(PokedexListWork *wk) {
    u32 i;

    BmpWin_InitAllocator(HEAPID_POKEDEX_LIST);
    for (i = 0; i < POKEDEX_LIST_WIN_COUNT; i++) {
        wk->windows[i].window = BmpWin_CreateDynamic(
            sPokedexListWindows[i].bg, sPokedexListWindows[i].x, sPokedexListWindows[i].y, sPokedexListWindows[i].width,
            sPokedexListWindows[i].height, sPokedexListWindows[i].palette, TRUE);
    }
    PokedexListBmp_PutTitle(wk);
    PokedexListBmp_PutModes(wk);
}

void PokedexListBmp_Exit(PokedexListWork *wk) {
    u32 i;

    for (i = 0; i < POKEDEX_LIST_WIN_COUNT; i++) {
        BmpWin_Free(wk->windows[i].window);
    }
    BmpWin_FreeAllocator();
}

void PokedexListBmp_Update(PokedexListWork *wk) {
    u32 i;

    PrintQueue_Main(wk->printQueue);
    for (i = 0; i < POKEDEX_LIST_WIN_COUNT; i++) {
        PrintWindow_Flush(&wk->windows[i], wk->printQueue);
    }
}

static void PokedexListBmp_Print(PokedexListWork *wk, u32 window, u32 msg, u32 x, u32 y, u16 color, u32 align) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, msg);

    PrintStrAligned(&wk->windows[window], wk->printQueue, x, y, strbuf, wk->font, color, align);
    GFL_StrBufFree(strbuf);
}

// Prints a message with the word set's words
static void PokedexListBmp_PrintFormat(PokedexListWork *wk, u32 window, u32 msg, u32 x, u32 y, u16 color, u32 align) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, msg);

    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, strbuf);
    PrintStrAligned(&wk->windows[window], wk->printQueue, x, y, wk->strbuf, wk->font, color, align);
    GFL_StrBufFree(strbuf);
}

static void PokedexListBmp_PutWindow(PrintWindow *window) {
    BmpWin_FlushMap(window->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window->window));
}

// Copies the BG under a window of the top screen into it, for the text to go over
static void PokedexListBmp_LoadWindowBG(PokedexListWork *wk, u32 window) {
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
    arc = GFL_ArcSysCreateFileHandle(ARCID_POKEDEX_GRA, HEAPID_POKEDEX_LIST);
    screenFile = GFL_G2DIOReadNSCRArc(arc, 71, FALSE, &screenData, HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);
    screen = (u16 *)screenData->rawData;
    charFile = GFL_G2DIOReadBGNCGRArc(arc, 59, FALSE, &charData, HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);
    chars = charData->pRawData;
    GFL_ArcToolFree(arc);
    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            MIi_CpuCopy32(&chars[(screen[(posY + y) * 32 + posX + x] & 0x3ff) * 0x20], &pixels[(y * width + x) * 0x20],
                          0x20);
        }
    }
    GFL_HeapFree(charFile);
    GFL_HeapFree(screenFile);
}

static void PokedexListBmp_PutTitle(PokedexListWork *wk) {
    PokedexListBmp_LoadWindowBG(wk, POKEDEX_LIST_WIN_TITLE);
    if (PokeDex_GetMode(wk->param->pokedex) == DEX_NATIONAL) {
        PokedexListBmp_Print(wk, POKEDEX_LIST_WIN_TITLE, GameStart3_Text_NationalPokedex, 0, 4, COLOR_ITEM,
                             PRINT_ALIGN_LEFT);
    } else {
        PokedexListBmp_Print(wk, POKEDEX_LIST_WIN_TITLE, GameStart3_Text_UnovaPokedex, 0, 4, COLOR_ITEM,
                             PRINT_ALIGN_LEFT);
    }
    PokedexListBmp_PutWindow(&wk->windows[POKEDEX_LIST_WIN_TITLE]);
}

// The names of the Pokédexes on their buttons: without the National Pokédex, Unova's is on the second
static void PokedexListBmp_PutModes(PokedexListWork *wk) {
    BOOL national;
    BOOL habitat;

    if (wk->param->searched != FALSE) {
        return;
    }
    national = PokeDex_IsNationalObtained(wk->param->pokedex);
    habitat = PokeDex_IsHabitatListEnabled(wk->param->pokedex);
    if (national == TRUE && habitat == TRUE) {
        PokedexListBmp_Print(wk, POKEDEX_LIST_WIN_UNOVA, GameStart3_Text_Unova, 2, 7, COLOR_BUTTON, PRINT_ALIGN_LEFT);
        PokedexListBmp_Print(wk, POKEDEX_LIST_WIN_NATIONAL, GameStart3_Text_National, 2, 6, COLOR_BUTTON,
                             PRINT_ALIGN_LEFT);
        PokedexListBmp_Print(wk, POKEDEX_LIST_WIN_HABITAT, GameStart3_Text_Habitat, 2, 5, COLOR_BUTTON,
                             PRINT_ALIGN_LEFT);
        PokedexListBmp_PutWindow(&wk->windows[POKEDEX_LIST_WIN_UNOVA]);
        PokedexListBmp_PutWindow(&wk->windows[POKEDEX_LIST_WIN_NATIONAL]);
        PokedexListBmp_PutWindow(&wk->windows[POKEDEX_LIST_WIN_HABITAT]);
    } else if (national == FALSE && habitat == TRUE) {
        PokedexListBmp_Print(wk, POKEDEX_LIST_WIN_NATIONAL, GameStart3_Text_Unova, 2, 6, COLOR_BUTTON,
                             PRINT_ALIGN_LEFT);
        PokedexListBmp_Print(wk, POKEDEX_LIST_WIN_HABITAT, GameStart3_Text_Habitat, 2, 5, COLOR_BUTTON,
                             PRINT_ALIGN_LEFT);
        PokedexListBmp_PutWindow(&wk->windows[POKEDEX_LIST_WIN_NATIONAL]);
        PokedexListBmp_PutWindow(&wk->windows[POKEDEX_LIST_WIN_HABITAT]);
    }
}

void PokedexListBmp_PutCounts(PokedexListWork *wk) {
    u32 seen;
    u32 caught;

    if (PokeDex_GetMode(wk->param->pokedex) == DEX_NATIONAL) {
        seen = PokeDex_GetSeenNoNational(wk->param->pokedex);
        caught = PokeDex_GetCaughtNoNational(wk->param->pokedex);
    } else {
        seen = PokeDex_GetSeenNoUnova(wk->param->pokedex, HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);
        caught = PokeDex_GetCaughtNoUnova(wk->param->pokedex, HEAPID_POKEDEX_LIST | HEAPID_TAIL_BIT);
    }
    PokedexListBmp_LoadWindowBG(wk, POKEDEX_LIST_WIN_SEEN);
    PokedexListBmp_LoadWindowBG(wk, POKEDEX_LIST_WIN_CAUGHT);
    PokedexListBmp_Print(wk, POKEDEX_LIST_WIN_SEEN, GameStart3_Text_Seen, 72, 4, COLOR_COUNT, PRINT_ALIGN_RIGHT);
    WordSetNumber(wk->wordSet, 0, seen, 3, 0, TRUE);
    PokedexListBmp_PrintFormat(wk, POKEDEX_LIST_WIN_SEEN, GameStart3_Text_Empty, 104, 4, COLOR_COUNT,
                               PRINT_ALIGN_RIGHT);
    PokedexListBmp_Print(wk, POKEDEX_LIST_WIN_CAUGHT, GameStart3_Text_Obtained, 88, 4, COLOR_COUNT, PRINT_ALIGN_RIGHT);
    WordSetNumber(wk->wordSet, 0, caught, 3, 0, TRUE);
    PokedexListBmp_PrintFormat(wk, POKEDEX_LIST_WIN_CAUGHT, GameStart3_Text_Empty, 120, 4, COLOR_COUNT,
                               PRINT_ALIGN_RIGHT);
    PokedexListBmp_PutWindow(&wk->windows[POKEDEX_LIST_WIN_SEEN]);
    PokedexListBmp_PutWindow(&wk->windows[POKEDEX_LIST_WIN_CAUGHT]);
}

void PokedexListBmp_PutSearchCount(PokedexListWork *wk) {
    PokedexListBmp_LoadWindowBG(wk, POKEDEX_LIST_WIN_CAUGHT);
    PokedexListBmp_Print(wk, POKEDEX_LIST_WIN_CAUGHT, GameStart3_Text_SearchResults, 88, 4, COLOR_COUNT,
                         PRINT_ALIGN_RIGHT);
    WordSetNumber(wk->wordSet, 0, wk->param->count, 3, 0, TRUE);
    PokedexListBmp_PrintFormat(wk, POKEDEX_LIST_WIN_CAUGHT, GameStart3_Text_Empty, 120, 4, COLOR_COUNT,
                               PRINT_ALIGN_RIGHT);
    PokedexListBmp_PutWindow(&wk->windows[POKEDEX_LIST_WIN_CAUGHT]);
}

void PokedexListBmp_PrintItem(PokedexListWork *wk, PrintWindow *window, StrBuf *name, u32 number) {
    PrintQueue *queue = FrameList_GetPrintQueue(wk->frameList);
    StrBuf *strbuf;

    // The list holds national numbers
    if (PokeDex_GetMode(wk->param->pokedex) != DEX_NATIONAL) {
        number = wk->regionalNumbers[number];
    }
    strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, GameStart3_Text_Empty_4);
    WordSetNumber(wk->wordSet, 0, number, 3, 2, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->strbuf, strbuf);
    GFL_StrBufFree(strbuf);
    PrintStrAligned(window, queue, 0, 4, wk->strbuf, wk->font, COLOR_ITEM, PRINT_ALIGN_LEFT);
    PrintStrAligned(window, queue, 28, 4, name, wk->font, COLOR_ITEM, PRINT_ALIGN_LEFT);
}
