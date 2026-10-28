#include "types.h"
#include "app/hall_of_fame_pc_bmp.h"
#include "app/hall_of_fame_pc_main.h"
#include "app/ui/print_msg.h"
#include "constants/species.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "system/bmp_oam.h"
#include "system/printsys.h"
#include "system/wordset.h"
#include "text/system/btl_pokeparam_3.h"

// The Hall of Fame PC's text: the page's title, the page number and the chosen Pokémon's data. The ROM doesn't name
// this file; the name is a guess, after box2_bmp.c

#define HALL_OF_FAME_PC_BMP_COLOR_BLACK PRINT_COLOR(1, 2, 0)
#define HALL_OF_FAME_PC_BMP_COLOR_BLUE PRINT_COLOR(3, 4, 0)
#define HALL_OF_FAME_PC_BMP_COLOR_RED PRINT_COLOR(5, 6, 0)
#define HALL_OF_FAME_PC_BMP_COLOR_PAGE PRINT_COLOR(6, 8, 0)

static void HallOfFamePCBmp_PutWindow(PrintWindow *win);

void HallOfFamePCBmp_Init(HallOfFamePCWork *wk) {
    BmpWin_InitAllocator(HEAPID_HALL_OF_FAME_PC);
    wk->win[HALL_OF_FAME_PC_BMP_WIN_TITLE].window = BmpWin_CreateDynamic(3, 1, 0, 30, 2, 3, TRUE);
    wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO].window = BmpWin_CreateDynamic(5, 4, 7, 24, 15, 1, TRUE);
}

void HallOfFamePCBmp_Exit(HallOfFamePCWork *wk) {
    BmpWin_Free(wk->win[HALL_OF_FAME_PC_BMP_WIN_TITLE].window);
    BmpWin_Free(wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO].window);
    BmpWin_FreeAllocator();
}

void HallOfFamePCBmp_PrintUpdate(HallOfFamePCWork *wk) {
    u32 i;

    PrintQueue_Main(wk->printQueue);
    for (i = 0; i < HALL_OF_FAME_PC_BMP_WIN_MAX; i++) {
        PrintWindow_Flush(&wk->win[i], wk->printQueue);
    }
}

static void HallOfFamePCBmp_PutWindow(PrintWindow *win) {
    BmpWin_FlushMap(win->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(win->window));
}

void HallOfFamePCBmp_PutTitle(HallOfFamePCWork *wk) {
    HallOfFamePCTeam *team = &wk->teams[wk->page];

    GFL_BitmapFill(BmpWin_GetBitmap(wk->win[HALL_OF_FAME_PC_BMP_WIN_TITLE].window), 0);

    if (wk->param->mode == HALL_OF_FAME_PC_MODE_FIRST_CLEAR) {
        StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam3_Text_CompletedFirstAdventure);

        PrintStrAligned(&wk->win[HALL_OF_FAME_PC_BMP_WIN_TITLE], wk->printQueue, 0, 0, str, wk->font,
                        HALL_OF_FAME_PC_BMP_COLOR_BLACK, PRINT_ALIGN_LEFT);
        GFL_StrBufFree(str);
    } else {
        StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam3_Text_HallFameNo);

        WordSetNumber(wk->wordSet, 0, team->number, 4, 0, TRUE);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->expandBuf, str);
        PrintStrAligned(&wk->win[HALL_OF_FAME_PC_BMP_WIN_TITLE], wk->printQueue, 0, 0, wk->expandBuf, wk->font,
                        HALL_OF_FAME_PC_BMP_COLOR_BLACK, PRINT_ALIGN_LEFT);
        GFL_StrBufFree(str);
    }

    {
        StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam3_Text_20);

        WordSetNumber(wk->wordSet, 0, team->date.year, 2, 2, TRUE);
        WordSetNumber(wk->wordSet, 1, team->date.month, 2, 2, TRUE);
        WordSetNumber(wk->wordSet, 2, team->date.day, 2, 2, TRUE);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->expandBuf, str);
        PrintStrAligned(&wk->win[HALL_OF_FAME_PC_BMP_WIN_TITLE], wk->printQueue,
                        BmpWin_GetWidth1(wk->win[HALL_OF_FAME_PC_BMP_WIN_TITLE].window) * 8, 0, wk->expandBuf, wk->font,
                        HALL_OF_FAME_PC_BMP_COLOR_RED, PRINT_ALIGN_RIGHT);
        GFL_StrBufFree(str);
    }

    HallOfFamePCBmp_PutWindow(&wk->win[HALL_OF_FAME_PC_BMP_WIN_TITLE]);
}

void HallOfFamePCBmp_PutPage(HallOfFamePCWork *wk) {
    StrBuf *str;
    u8 slashWidth;
    u8 x;

    if (wk->pageMax == 1) {
        return;
    }

    GFL_BitmapFill(wk->pageBitmap, 0);

    // The slash in the middle, the page before it and the number of pages after it
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam3_Text_Empty);
    slashWidth = GFL_FontGetBlockWidth(str, wk->font, 0);
    x = (40 - slashWidth) / 2;
    GFL_TextRendererDrawToBitmapEx(wk->pageBitmap, x, 4, str, wk->font, HALL_OF_FAME_PC_BMP_COLOR_PAGE);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam3_Text_Empty_2);
    WordSetNumber(wk->wordSet, 0, wk->page + 1, 2, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->expandBuf, str);
    GFL_TextRendererDrawToBitmapEx(wk->pageBitmap, x - GFL_FontGetBlockWidth(wk->expandBuf, wk->font, 0), 4,
                                   wk->expandBuf, wk->font, HALL_OF_FAME_PC_BMP_COLOR_PAGE);
    WordSetNumber(wk->wordSet, 0, wk->pageMax, 2, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->expandBuf, str);
    GFL_TextRendererDrawToBitmapEx(wk->pageBitmap, x + slashWidth, 4, wk->expandBuf, wk->font,
                                   HALL_OF_FAME_PC_BMP_COLOR_PAGE);
    GFL_StrBufFree(str);

    BmpOam_ActorBmpTrans(wk->pageActor);
}

void HallOfFamePCBmp_PutPokeInfo(HallOfFamePCWork *wk) {
    HallOfFamePokemon *poke = &wk->teams[wk->page].pokemon[wk->pokePos];
    StrBuf *str;
    u32 i;
    u32 width;

    GFL_BitmapFill(BmpWin_GetBitmap(wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO].window), 0);

    // The species and its sex, which the names of both Nidoran have
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam3_Text_Empty_3);
    WordSet_LoadSpeciesName(wk->wordSet, 0, poke->species);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->expandBuf, str);
    PrintStrAligned(&wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO], wk->printQueue, 0, 0, wk->expandBuf, wk->font,
                    HALL_OF_FAME_PC_BMP_COLOR_BLACK, PRINT_ALIGN_LEFT);
    GFL_StrBufFree(str);

    if (poke->sex == 0) {
        if (poke->species != SPECIES_NIDORAN_M) {
            str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam3_Text_Empty_4);
            PrintStrAligned(&wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO], wk->printQueue, 64, 0, str, wk->font,
                            HALL_OF_FAME_PC_BMP_COLOR_BLUE, PRINT_ALIGN_LEFT);
            GFL_StrBufFree(str);
        }
    } else if (poke->sex == 1) {
        if (poke->species != SPECIES_NIDORAN_F) {
            str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam3_Text_Empty_5);
            PrintStrAligned(&wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO], wk->printQueue, 64, 0, str, wk->font,
                            HALL_OF_FAME_PC_BMP_COLOR_RED, PRINT_ALIGN_LEFT);
            GFL_StrBufFree(str);
        }
    }

    // The level
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam3_Text_L);
    width = GFL_FontGetBlockWidth(str, wk->smallFont, 0);
    PrintStrAligned(&wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO], wk->printQueue, 80, 4, str, wk->smallFont,
                    HALL_OF_FAME_PC_BMP_COLOR_BLACK, PRINT_ALIGN_LEFT);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam3_Text_Empty_6);
    WordSetNumber(wk->wordSet, 0, poke->level, 3, 0, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->expandBuf, str);
    PrintStrAligned(&wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO], wk->printQueue, 80 + width, 0, wk->expandBuf, wk->font,
                    HALL_OF_FAME_PC_BMP_COLOR_BLACK, PRINT_ALIGN_LEFT);
    GFL_StrBufFree(str);

    // The nickname and the trainer's name
    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam3_Text_Name);
    GFL_WordSetSetStrbuf(wk->wordSet, 0, poke->nickname, 0, 1, 2);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->expandBuf, str);
    PrintStrAligned(&wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO], wk->printQueue, 0, 24, wk->expandBuf, wk->font,
                    HALL_OF_FAME_PC_BMP_COLOR_BLACK, PRINT_ALIGN_LEFT);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam3_Text_Ot);
    GFL_WordSetSetStrbuf(wk->wordSet, 0, poke->trainerName, 0, 1, 2);
    GFL_WordSetFormatStrbuf(wk->wordSet, wk->expandBuf, str);
    PrintStrAligned(&wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO], wk->printQueue, 0, 48, wk->expandBuf, wk->font,
                    HALL_OF_FAME_PC_BMP_COLOR_BLACK, PRINT_ALIGN_LEFT);
    GFL_StrBufFree(str);

    // The moves, in two columns
    for (i = 0; i < 4; i++) {
        str = GFL_MsgDataLoadStrbufNew(wk->msgData, BtlPokeparam3_Text_Empty_7 + i);
        loadMoveNameToStrbuf(wk->wordSet, 0, poke->moves[i]);
        GFL_WordSetFormatStrbuf(wk->wordSet, wk->expandBuf, str);
        PrintStrAligned(&wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO], wk->printQueue, (i & 1) * 96, 88 + (i >> 1) * 16,
                        wk->expandBuf, wk->font, HALL_OF_FAME_PC_BMP_COLOR_BLACK, PRINT_ALIGN_LEFT);
        GFL_StrBufFree(str);
    }

    HallOfFamePCBmp_PutWindow(&wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO]);
}

void HallOfFamePCBmp_ClearPokeInfo(HallOfFamePCWork *wk) {
    BmpWin_ClearScreen(wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO].window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO].window));
    GFL_BitmapFill(BmpWin_GetBitmap(wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO].window), 0);
    BmpWin_FlushChar(wk->win[HALL_OF_FAME_PC_BMP_WIN_INFO].window);
}
