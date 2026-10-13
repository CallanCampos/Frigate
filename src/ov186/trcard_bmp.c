#include "types.h"
#include "app/trainer_card.h"
#include "constants/arc.h"
#include "constants/text_banks.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "save/playtime.h"
#include "save/timesig_savedata.h"
#include "system/bmp_winframe.h"
#include "system/game_data.h"
#include "system/gf_font.h"
#include "system/pms_draw.h"
#include "system/printsys.h"
#include "system/str_tool.h"
#include "system/union_view.h"
#include "system/wordset.h"

// The trainer card screen's windows and what is printed in them: the front's fields and the rows of the back's list.
// The ROM has no name for this file; trcard_bmp.c is a guess

// The rows of the back's list
enum {
    TRCARD_ROW_TRAINER_ID,
    TRCARD_ROW_PLAY_TIME,
    TRCARD_ROW_START_LABEL,
    TRCARD_ROW_START_DATE,
    TRCARD_ROW_CLEAR_LABEL,
    TRCARD_ROW_CLEAR_DATE,
    TRCARD_ROW_6,
    TRCARD_ROW_7,
    TRCARD_ROW_8,
    TRCARD_ROW_9,
    TRCARD_ROW_10,
    TRCARD_ROW_11,
    TRCARD_ROW_12,
    TRCARD_ROW_13,
    TRCARD_ROW_14,
    TRCARD_ROW_PASS_POWERS,
    TRCARD_ROW_16,
    TRCARD_ROW_17,
    TRCARD_ROW_18,
    TRCARD_ROW_PWT_WINS,
    TRCARD_ROW_AVENUE_NAME,
    TRCARD_ROW_AVENUE,
    TRCARD_ROW_PAL_PARK,
    TRCARD_ROW_23,
};

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} TrCardBmpWinDef;

static void TrCardBmp_Print(BmpWin *win, TrainerCardWork *wk, int x, int y, StrBuf *strbuf, int color);
static void TrCardBmp_FormatAvenueLabel(TrainerCardWork *wk, StrBuf *dest, int label);
static void TrCardBmp_DrawRow(TrainerCardWork *wk, BmpWin *win, u32 row, int y, int color);
static void TrCardBmp_AddRow(TrainerCardWork *wk, int row);
static void TrCardBmp_PrintNumber(TrainerCardWork *wk, BmpWin *win, int x, int margin, int y, StrBuf *tmp, u32 value,
                                  u8 digits, u32 pad, int color);
static void TrCardBmp_PrintMoney(TrainerCardWork *wk, BmpWin *win, int x, int margin, int y, StrBuf *tmp, u32 value,
                                 u8 digits, u32 pad, int format, int color);
static void TrCardBmp_PrintStartDate(TrainerCardWork *wk, BmpWin *win, int x, int margin, int y, StrBuf *tmp, u32 pad,
                                     int color);
static void TrCardBmp_PrintClearDate(TrainerCardWork *wk, BmpWin *win, int x, int margin, int y, StrBuf *tmp, u32 pad,
                                     int color);
static void TrCardBmp_PrintSeenCount(TrainerCardWork *wk, BmpWin *win, int x, int margin, int y, StrBuf *tmp, u32 value,
                                     u32 pad, int color);
static void TrCardBmp_PrintTimeNumber(TrainerCardWork *wk, BmpWin *win, int x, int margin, int y, StrBuf *tmp,
                                      u32 value, u8 digits, u32 pad, int a9, int a10, int color);
static void TrCardBmp_PrintRight(TrainerCardWork *wk, BmpWin *win, int x, int margin, int y, StrBuf *strbuf);
static void TrCardBmp_PrintPlayTime(TrainerCardWork *wk, BmpWin *win, int y, const TrainerCardData *data,
                                    StrBuf *tmp, int color);

// The text's colors: normally, and while the list is dragged
static const u16 sTrCardBmpColors[] = { 0x0440, 0x1060 };

// The front's windows that TrCardBmp_Flush shows
static const u8 sTrCardBmpFrontWins[] = { TRUE, TRUE, TRUE, TRUE, TRUE, TRUE, TRUE };

static const TrCardBmpWinDef sTrCardBmpWinDefs[TRCARD_WIN_COUNT] = {
    { 7, 3, 4, 17, 2, 15 },  { 7, 3, 6, 17, 2, 15 },  { 7, 3, 8, 17, 2, 15 }, { 7, 3, 10, 17, 2, 15 },
    { 7, 3, 13, 27, 2, 15 }, { 7, 3, 15, 25, 4, 15 }, { 7, 2, 2, 28, 8, 15 }, { 4, 2, 1, 27, 4, 14 },
};

void TrCardBmp_Init(TrainerCardWork *wk) {
    const TrCardBmpWinDef *defs = sTrCardBmpWinDefs;
    u8 i;

    BmpWin_InitAllocator(wk->heapId);
    for (i = 0; i < TRCARD_WIN_COUNT; i++) {
        wk->win[i] =
            BmpWin_CreateDynamic(defs[i].bg, defs[i].x, defs[i].y, defs[i].width, defs[i].height, defs[i].palette, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->win[i]), 0);
    }
    GFL_BGSysFillChar(7, 0, 1, 0);
    LoadSysMsgBox(4, 49, 13, wk->frameType, wk->heapId);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x1c0, 0x20, wk->heapId);
    GFL_BGSysQueueScrLoad(7);
    GFL_BGSysQueueScrLoad(4);
    wk->printQueue = func_02021998(wk->heapId);
}

void TrCardBmp_Exit(TrainerCardWork *wk) {
    u16 i;

    func_02021a18(wk->printQueue);
    for (i = 0; i < TRCARD_WIN_COUNT; i++) {
        BmpWin_Free(wk->win[i]);
    }
    BmpWin_FreeAllocator();
}

// The front: the name, the Pokédex count, the union room appearance, the personality, the money and the greeting
void TrCardBmp_DrawFront(TrainerCardWork *wk, BmpWin **win, const TrainerCardData *data) {
    u8 i;
    StrBuf *tmp;

    for (i = 0; i < 5; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(win[i]), 0);
        if (i != TRCARD_WIN_POKEDEX || (i == TRCARD_WIN_POKEDEX && data->unk04_2)) {
            GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win[i]), 0, 0, wk->labels[i], wk->font);
        }
    }
    tmp = wk->tmpBuf;
    GFL_StrBufLoadString(wk->nameBuf, data->name);
    TrCardBmp_PrintRight(wk, win[TRCARD_WIN_NAME], 136, 8, 0, wk->nameBuf);
    if (data->unk04_2) {
        TrCardBmp_PrintSeenCount(wk, win[TRCARD_WIN_POKEDEX], 136, 8, 0, tmp, data->seenCount, 1, 0);
    }
    TrCardBmp_DrawUnionView(wk, data->unk06, FALSE);
    TrCardBmp_DrawPersonality(wk, data->unk48, FALSE);
    TrCardBmp_PrintMoney(wk, win[TRCARD_WIN_MONEY], 216, 8, 0, tmp, data->money, 7, 1, 1, 0);
    PMSDraw_SetBackColor(wk->pmsDraw, 0);
    PMSDraw_Print(wk->pmsDraw, win[TRCARD_WIN_GREETING], &wk->data->greeting, 0);
    if (wk->side != 0) {
        PMSDraw_SetVisible(wk->pmsDraw, 0, FALSE);
    }
}

// The name of the union room appearance's trainer class
void TrCardBmp_DrawUnionView(TrainerCardWork *wk, u32 view, BOOL flush) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_TRAINER_CLASS_NAMES, wk->heapId);
    StrBuf *strbuf;
    u32 width;

    if (func_020091d0(getTimeSigSaveBlock(wk->param->gameData))) {
        strbuf = GFL_MsgDataLoadStrbufNew(msgData, func_0202b5fc(view));
    } else {
        strbuf = GFL_MsgDataLoadStrbufNew(msgData, 0);
    }
    width = GFL_FontGetBlockWidth(strbuf, wk->font, 0);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->win[TRCARD_WIN_UNION]), 0);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(wk->win[TRCARD_WIN_UNION]), 128 - width, 0, strbuf, wk->font);
    GFL_StrBufFree(strbuf);
    GFL_MsgDataFree(msgData);
    if (flush) {
        BmpWin_FlushMap(wk->win[TRCARD_WIN_UNION]);
        BmpWin_FlushChar(wk->win[TRCARD_WIN_UNION]);
        GFL_BGSysQueueScrLoad(7);
    }
}

void TrCardBmp_DrawPersonality(TrainerCardWork *wk, u32 personality, BOOL flush) {
    StrBuf *strbuf = GFL_MsgDataLoadStrbufNew(wk->msgData, personality + 35);
    u32 width = GFL_FontGetBlockWidth(strbuf, wk->font, 0);

    GFL_BitmapFill(BmpWin_GetBitmap(wk->win[TRCARD_WIN_PERSONALITY]), 0);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(wk->win[TRCARD_WIN_PERSONALITY]), 0, 0,
                                 wk->labels[TRCARD_WIN_PERSONALITY], wk->font);
    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(wk->win[TRCARD_WIN_PERSONALITY]), 128 - width, 0, strbuf, wk->font);
    GFL_StrBufFree(strbuf);
    if (flush) {
        BmpWin_FlushMap(wk->win[TRCARD_WIN_PERSONALITY]);
        BmpWin_FlushChar(wk->win[TRCARD_WIN_PERSONALITY]);
        GFL_BGSysQueueScrLoad(7);
    }
}

// Shows the windows of the side shown
void TrCardBmp_Flush(TrainerCardWork *wk, BmpWin **win) {
    int i;

    if (wk->side != 0) {
        BmpWin_FlushMap(win[TRCARD_WIN_LIST]);
        BmpWin_FlushChar(win[TRCARD_WIN_LIST]);
    } else {
        for (i = 0; i <= TRCARD_WIN_GREETING; i++) {
            if (sTrCardBmpFrontWins[i]) {
                BmpWin_FlushMap(win[i]);
                BmpWin_FlushChar(win[i]);
            }
        }
    }
    GFL_BGSysQueueScrLoad(7);
    GFL_BGSysQueueScrLoad(4);
}

void TrCardBmp_DrawBack(TrainerCardWork *wk, BmpWin **win, TrainerCardData *data) {
    TrCardBmp_DrawList(wk, wk->scroll, FALSE, 0);
}

static void TrCardBmp_Print(BmpWin *win, TrainerCardWork *wk, int x, int y, StrBuf *strbuf, int color) {
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(win), x, y, strbuf, wk->font, sTrCardBmpColors[color]);
}

// A label that names the player's Join Avenue
static void TrCardBmp_FormatAvenueLabel(TrainerCardWork *wk, StrBuf *dest, int label) {
    StrBuf *name = GFL_StrBufCreate(21, wk->heapId);

    GFL_StrBufLoadFixedString(name, wk->ownData->avenueName, 21);
    func_0202437c(wk->wordSet, 7, name, 0, 0, 0);
    GFL_WordSetFormatStrbuf(wk->wordSet, dest, wk->labels[label]);
    GFL_StrBufFree(name);
}

static void TrCardBmp_DrawRow(TrainerCardWork *wk, BmpWin *win, u32 row, int y, int color) {
    TrainerCardData *data = wk->data;
    TrainerCardData *own = wk->ownData;
    StrBuf *tmp = wk->tmpBuf;

    switch (row) {
    case TRCARD_ROW_TRAINER_ID:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[5], color);
        TrCardBmp_PrintNumber(wk, win, 96, 0, y, tmp, data->trainerId, 5, 2, color);
        break;
    case TRCARD_ROW_PLAY_TIME:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[6], color);
        TrCardBmp_PrintPlayTime(wk, win, y, wk->data, wk->numBuf, color);
        break;
    case TRCARD_ROW_START_LABEL:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[7], color);
        break;
    case TRCARD_ROW_START_DATE:
        TrCardBmp_PrintStartDate(wk, win, 216, 0, y, tmp, 0, color);
        break;
    case TRCARD_ROW_CLEAR_LABEL:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[8], color);
        break;
    case TRCARD_ROW_CLEAR_DATE:
        if (data->clearMonth != 0) {
            TrCardBmp_PrintClearDate(wk, win, 216, 0, y, tmp, 0, color);
        } else {
            u32 width = GFL_FontGetBlockWidth(wk->noClearBuf, wk->font, 0);

            TrCardBmp_Print(win, wk, 216 - width, y, wk->noClearBuf, color);
        }
        break;
    case TRCARD_ROW_6:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[9], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, data->unk38, 9, 1, color);
        break;
    case TRCARD_ROW_7:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[10], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, data->unk3C, 9, 1, color);
        break;
    case TRCARD_ROW_8:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[11], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, data->unk40, 9, 1, color);
        break;
    case TRCARD_ROW_9:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[12], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, data->unk44, 9, 1, color);
        break;
    case TRCARD_ROW_10:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[13], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, data->unk4C, 9, 1, color);
        break;
    case TRCARD_ROW_11:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[14], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, data->unk50, 9, 1, color);
        break;
    case TRCARD_ROW_12:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[15], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, data->unk54, 9, 1, color);
        break;
    case TRCARD_ROW_13:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[16], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, data->unk58, 5, 1, color);
        break;
    case TRCARD_ROW_14:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[17], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, data->unk5C, 4, 1, color);
        break;
    case TRCARD_ROW_PASS_POWERS:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[18], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, data->passPowerCount, 2, 1, color);
        break;
    case TRCARD_ROW_16:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[22], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, own->unk684, 5, 1, color);
        break;
    case TRCARD_ROW_17:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[23], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, own->unk688, 4, 1, color);
        break;
    case TRCARD_ROW_18:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[19], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, data->unk60, 4, 1, color);
        break;
    case TRCARD_ROW_PWT_WINS:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[24], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, own->pwtWins, 5, 1, color);
        break;
    case TRCARD_ROW_AVENUE_NAME:
        TrCardBmp_FormatAvenueLabel(wk, tmp, 25);
        TrCardBmp_Print(win, wk, 0, y, tmp, color);
        break;
    case TRCARD_ROW_AVENUE:
        TrCardBmp_FormatAvenueLabel(wk, tmp, 26);
        TrCardBmp_Print(win, wk, 0, y, tmp, color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, own->unk68A, 4, 1, color);
        break;
    case TRCARD_ROW_PAL_PARK:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[20], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, data->palParkHighScore, 3, 1, color);
        break;
    case TRCARD_ROW_23:
        TrCardBmp_Print(win, wk, 0, y, wk->labels[21], color);
        TrCardBmp_PrintNumber(wk, win, 216, 8, y, tmp, data->unk64, 4, 1, color);
        break;
    }
}

static void TrCardBmp_AddRow(TrainerCardWork *wk, int row) {
    wk->rows[wk->rowCount] = row;
    wk->rowCount++;
}

// The rows of the back's list: the first thirteen always, and the others once the card has something to show there
void TrCardBmp_InitRows(TrainerCardWork *wk, TrainerCardData *shown, TrainerCardData *own) {
    int i;

    for (i = 0; i < 13; i++) {
        TrCardBmp_AddRow(wk, i);
    }
    if (shown->unk68_0) {
        TrCardBmp_AddRow(wk, TRCARD_ROW_14);
        TrCardBmp_AddRow(wk, TRCARD_ROW_13);
    }
    if (shown->unk68_1) {
        TrCardBmp_AddRow(wk, TRCARD_ROW_PASS_POWERS);
    }
    if (shown->unk68_2) {
        TrCardBmp_AddRow(wk, TRCARD_ROW_18);
    }
    if (shown->unk68_3) {
        TrCardBmp_AddRow(wk, TRCARD_ROW_PAL_PARK);
    }
    if (shown->unk68_4) {
        TrCardBmp_AddRow(wk, TRCARD_ROW_23);
    }
    if (own->unk690_0) {
        TrCardBmp_AddRow(wk, TRCARD_ROW_16);
    }
    if (own->unk690_1) {
        TrCardBmp_AddRow(wk, TRCARD_ROW_17);
    }
    if (own->unk690_2) {
        TrCardBmp_AddRow(wk, TRCARD_ROW_PWT_WINS);
    }
    if (own->unk690_3) {
        TrCardBmp_AddRow(wk, TRCARD_ROW_AVENUE_NAME);
        TrCardBmp_AddRow(wk, TRCARD_ROW_AVENUE);
    }
}

// The five rows of the list that show when it is scrolled by scroll, which is 0 or less
void TrCardBmp_DrawList(TrainerCardWork *wk, int scroll, BOOL flush, int color) {
    BmpWin *win = wk->win[TRCARD_WIN_LIST];
    int i;
    int y;
    int top;

    GFL_BitmapFill(BmpWin_GetBitmap(win), 0);
    y = scroll % TRCARD_ROW_HEIGHT;
    top = -(scroll / TRCARD_ROW_HEIGHT);
    for (i = 0; i < 5; i++) {
        TrCardBmp_DrawRow(wk, win, wk->rows[top + i], y + i * TRCARD_ROW_HEIGHT, color);
    }
    if (flush) {
        BmpWin_FlushChar(win);
        BmpWin_FlushMap(win);
        GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(win));
    }
}

// Prints a number that ends margin pixels before x
static void TrCardBmp_PrintNumber(TrainerCardWork *wk, BmpWin *win, int x, int margin, int y, StrBuf *tmp, u32 value,
                                  u8 digits, u32 pad, int color) {
    u32 width;

    GFL_WordSetFormatNumber(tmp, value, digits, pad, TRUE);
    width = GFL_FontGetBlockWidth(tmp, wk->font, 0);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(win), x - (width + margin), y, tmp, wk->font,
                                   sTrCardBmpColors[color]);
}

static void TrCardBmp_PrintMoney(TrainerCardWork *wk, BmpWin *win, int x, int margin, int y, StrBuf *tmp, u32 value,
                                 u8 digits, u32 pad, int format, int color) {
    u32 width;

    WordSetNumber(wk->wordSet, 0, value, digits, pad, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, tmp, wk->formats[format]);
    width = GFL_FontGetBlockWidth(tmp, wk->font, 0);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(win), x - (width + margin), y, tmp, wk->font,
                                   sTrCardBmpColors[color]);
}

// The date the adventure started on
static void TrCardBmp_PrintStartDate(TrainerCardWork *wk, BmpWin *win, int x, int margin, int y, StrBuf *tmp, u32 pad,
                                     int color) {
    u32 width;

    WordSetNumber(wk->wordSet, 0, wk->data->startYear + 2000, 4, pad, TRUE);
    WordSetNumber(wk->wordSet, 1, wk->data->startMonth, 2, pad, TRUE);
    WordSetNumber(wk->wordSet, 2, wk->data->startDay, 2, pad, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, tmp, wk->formats[2]);
    width = GFL_FontGetBlockWidth(tmp, wk->font, 0);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(win), x - (width + margin), y, tmp, wk->font,
                                   sTrCardBmpColors[color]);
}

// The date and time of the first Hall of Fame entry
static void TrCardBmp_PrintClearDate(TrainerCardWork *wk, BmpWin *win, int x, int margin, int y, StrBuf *tmp, u32 pad,
                                     int color) {
    u32 width;

    WordSetNumber(wk->wordSet, 0, wk->data->clearYear + 2000, 4, pad, TRUE);
    WordSetNumber(wk->wordSet, 1, wk->data->clearMonth, 2, pad, TRUE);
    WordSetNumber(wk->wordSet, 2, wk->data->clearDay, 2, pad, TRUE);
    WordSetNumber(wk->wordSet, 3, wk->data->clearHour, 2, 2, TRUE);
    WordSetNumber(wk->wordSet, 4, wk->data->clearMinute, 2, 2, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, tmp, wk->formats[3]);
    width = GFL_FontGetBlockWidth(tmp, wk->font, 0);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(win), x - (width + margin), y, tmp, wk->font,
                                   sTrCardBmpColors[color]);
}

static void TrCardBmp_PrintSeenCount(TrainerCardWork *wk, BmpWin *win, int x, int margin, int y, StrBuf *tmp, u32 value,
                                     u32 pad, int color) {
    u32 width;

    WordSetNumber(wk->wordSet, 0, value, 3, pad, TRUE);
    GFL_WordSetFormatStrbuf(wk->wordSet, tmp, wk->formats[4]);
    width = GFL_FontGetBlockWidth(tmp, wk->font, 0);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(win), x - (width + margin), y, tmp, wk->font,
                                   sTrCardBmpColors[color]);
}

// TrCardBmp_PrintNumber with two arguments that nothing reads
static void TrCardBmp_PrintTimeNumber(TrainerCardWork *wk, BmpWin *win, int x, int margin, int y, StrBuf *tmp,
                                      u32 value, u8 digits, u32 pad, int a9, int a10, int color) {
    u32 width;

    GFL_WordSetFormatNumber(tmp, value, digits, pad, TRUE);
    width = GFL_FontGetBlockWidth(tmp, wk->font, 0);
    GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(win), x - (width + margin), y, tmp, wk->font,
                                   sTrCardBmpColors[color]);
}

static void TrCardBmp_PrintRight(TrainerCardWork *wk, BmpWin *win, int x, int margin, int y, StrBuf *strbuf) {
    u32 width = GFL_FontGetBlockWidth(strbuf, wk->font, 0);

    GFL_TextRendererDrawToBitmap(BmpWin_GetBitmap(win), x - (width + margin), y, strbuf, wk->font);
}

// The play time, which keeps running on the player's own card
static void TrCardBmp_PrintPlayTime(TrainerCardWork *wk, BmpWin *win, int y, const TrainerCardData *data,
                                    StrBuf *tmp, int color) {
    int hours;

    if (data->playTime == NULL) {
        TrCardBmp_PrintTimeNumber(wk, win, 216, 0, y, tmp, data->playMinutes, 2, 2, 208, 16, color);
        TrCardBmp_PrintTimeNumber(wk, win, 216, 19, y, tmp, data->playHours, 3, 1, 173, 27, color);
        GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(win), 198, y, wk->colonBuf, wk->font, sTrCardBmpColors[color]);
    } else {
        hours = func_02008cec(data->playTime);
        if (hours > 999) {
            hours = 999;
        }
        TrCardBmp_PrintTimeNumber(wk, win, 216, 0, y, tmp, func_02008cf0(data->playTime), 2, 2, 208, 16, color);
        TrCardBmp_PrintTimeNumber(wk, win, 216, 19, y, tmp, hours, 3, 1, 173, 27, color);
        GFL_TextRendererDrawToBitmapEx(BmpWin_GetBitmap(win), 198, y, wk->colonBuf, wk->font, sTrCardBmpColors[color]);
    }
}

void TrCardBmp_Stub(TrainerCardWork *wk, BmpWin *win, u32 a2, StrBuf *strbuf) {
}
