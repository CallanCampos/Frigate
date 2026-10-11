#include "types.h"
#include "battle/b_app_tool.h"
#include "battle/b_plist_bmp.h"
#include "battle/b_plist_main.h"
#include "constants/arc.h"
#include "constants/moves.h"
#include "constants/text_banks.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "pml/item.h"
#include "pml/waza.h"
#include "system/app_menu_common.h"
#include "system/bmp_winframe.h"
#include "system/gf_font.h"
#include "system/hp_gauge.h"
#include "system/printsys.h"
#include "system/str_tool.h"
#include "system/text_speed.h"
#include "system/wordset.h"

// The battle party list's windows and text (overlay 287). The ROM doesn't name this file: b_plist_bmp.c is a guess
// after the ROM's b_plist_main.c and b_plist_anm.c and the b_plist_bmp.c of Diamond and Pearl. None of these functions
// has a name yet.

#define BPLIST_COLOR_WHITE PRINT_COLOR(15, 14, 0)
#define BPLIST_COLOR_BLACK PRINT_COLOR(1, 2, 0)
#define BPLIST_COLOR_STAT PRINT_COLOR(8, 9, 0)
#define BPLIST_COLOR_MALE PRINT_COLOR(10, 11, 0)
#define BPLIST_COLOR_FEMALE PRINT_COLOR(12, 13, 0)

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} BPlistWinData;

static void BPlistBmp_ToggleDummyWindow(BPlistWork *work);
static void BPlistBmp_FreeDummyWindow(BPlistWork *work);
static void BPlistBmp_ClearPageWindows(BPlistWork *work);
static void BPlistBmp_PrintName(BPlistWork *work, u32 winIdx, u16 pos, u8 x, u8 y);
static void BPlistBmp_PrintLevel(BPlistWork *work, u32 winIdx, u8 pos, u8 x, u8 y);
static void BPlistBmp_PrintHP(BPlistWork *work, u32 winIdx, u8 pos, u8 x, u8 y);
static void BPlistBmp_DrawHPBar(BPlistWork *work, u32 winIdx, u8 pos, u8 x, u8 y);
static void BPlistBmp_PrintAbility(BPlistWork *work, u32 winIdx, u8 pos);
static void BPlistBmp_PrintItem(BPlistWork *work, u32 winIdx, u8 pos);
static void BPlistBmp_PrintMoveName(BPlistWork *work, u32 move, u32 winIdx, u32 msgId, u16 x, s16 y, u32 color);
static void BPlistBmp_PrintTypeLabel(BPlistWork *work, u32 winIdx, u8 x, u8 y);
static void BPlistBmp_PrintInfo(BPlistWork *work, u32 msgId);
static void BPlistBmp_PrintButtonLabel(BPlistWork *work, u32 winIdx, u32 msgId);
static void BPlistBmp_PrintLevelExp(BPlistWork *work, u8 pos);
static void BPlistBmp_PrintAttack(BPlistWork *work, u8 pos);
static void BPlistBmp_PrintDefense(BPlistWork *work, u8 pos);
static void BPlistBmp_PrintSpeed(BPlistWork *work, u8 pos);
static void BPlistBmp_PrintSpAttack(BPlistWork *work, u8 pos);
static void BPlistBmp_PrintSpDefense(BPlistWork *work, u8 pos);
static void BPlistBmp_PrintStatusHP(BPlistWork *work, u8 pos);
static void BPlistBmp_PrintAbilityInfo(BPlistWork *work, u8 pos);
static void BPlistBmp_PrintAccuracyLabel(BPlistWork *work, u32 winIdx);
static void BPlistBmp_PrintAccuracy(BPlistWork *work, u32 winIdx, u32 value);
static void BPlistBmp_PrintPowerLabel(BPlistWork *work, u32 winIdx);
static void BPlistBmp_PrintPower(BPlistWork *work, u32 winIdx, u32 value);
static void BPlistBmp_PrintMoveInfo(BPlistWork *work, u32 winIdx, u32 msgId);
static void BPlistBmp_PrintCategoryLabel(BPlistWork *work, u32 winIdx);
static void BPlistBmp_PrintCategory(BPlistWork *work, u32 winIdx, u32 category);
static void BPlistBmp_PrintMovePP(BPlistWork *work, u32 winIdx, u32 pp, u32 maxPp);
static void BPlistBmp_PrintForgetButton(BPlistWork *work, u32 winIdx);
static void BPlistBmp_PrintPlatePP(BPlistWork *work, BPlistMove *move, u32 winIdx);
static void BPlistBmp_PrintPlateHP(BPlistWork *work, u8 pos);
static void BPlistBmp_PrintPlateLevel(BPlistWork *work, u8 pos);
static void BPlistBmp_PrintCenteredName(BPlistWork *work, u8 pos);
static void BPlistBmp_StartMessageStream(BPlistWork *work);
static void BPlistBmp_DrawPartyPage(BPlistWork *work);
static void BPlistBmp_DrawSelectPage(BPlistWork *work);
static void BPlistBmp_DrawMovesPage(BPlistWork *work);
static void BPlistBmp_DrawStatusPage(BPlistWork *work);
static void BPlistBmp_DrawMoveInfoPage(BPlistWork *work);
static void BPlistBmp_DrawForgetPage(BPlistWork *work);
static void BPlistBmp_DrawForgetInfoPage(BPlistWork *work);
static void BPlistBmp_DrawPPRestorePage(BPlistWork *work);

static const u8 data_ov287_021fae2c[] = { 0, 1, 2, 3, 0xff };
static const u8 data_ov287_021fae31[] = { 0, 1, 2, 3, 4, 0xff };
// Declared in this order for MWCC's sort to lay the two out in the ROM's order
static const u8 data_ov287_021fae3e[] = { 0, 1, 2, 3, 4, 5, 0xff };
static const u8 data_ov287_021fae37[] = { 0, 1, 2, 3, 4, 5, 0xff };
static const u8 data_ov287_021fae45[] = { 0, 1, 2, 3, 4, 5, 0xff };

static const BPlistWinData data_ov287_021fae4c[2] = {
    { 4, 2, 21, 22, 2, 13 },
    { 4, 2, 19, 27, 4, 13 },
};

static const u8 data_ov287_021fae58[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 0xff };
static const u8 data_ov287_021fae64[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 0xff };
static const u32 data_ov287_021fae74[] = { 0x47, 0x4a, 0x4d, 0x50, 0x53 };
static const u8 data_ov287_021fae88[] = { 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,  11,
                                          12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0xff };
static const u32 data_ov287_021faea0[] = { 0, 1, 2, 3, 4, 5 };

static const BPlistWinData data_ov287_021faeb8[4] = {
    { 5, 10, 4, 12, 3, 9 },
    { 5, 11, 12, 10, 3, 9 },
    { 5, 1, 20, 11, 3, 9 },
    { 5, 14, 20, 11, 3, 9 },
};

static const BPlistWinData data_ov287_021faed0[5] = {
    { 5, 5, 1, 9, 2, 9 },   { 5, 1, 6, 14, 5, 9 },   { 5, 17, 6, 14, 5, 9 },
    { 5, 1, 12, 14, 5, 9 }, { 5, 17, 12, 14, 5, 9 },
};

static const BPlistWinData data_ov287_021faeee[6] = {
    { 5, 5, 1, 9, 2, 9 },   { 5, 1, 6, 14, 5, 9 },   { 5, 17, 6, 14, 5, 9 },
    { 5, 1, 12, 14, 5, 9 }, { 5, 17, 12, 14, 5, 9 }, { 5, 9, 18, 14, 5, 9 },
};

static const BPlistWinData data_ov287_021faf12[6] = {
    { 5, 0, 0, 15, 5, 9 },  { 5, 16, 1, 15, 5, 9 }, { 5, 0, 6, 15, 5, 9 },
    { 5, 16, 7, 15, 5, 9 }, { 5, 0, 12, 15, 5, 9 }, { 5, 16, 13, 15, 5, 9 },
};

static const BPlistWinData data_ov287_021faf36[6] = {
    { 5, 5, 1, 9, 2, 9 },   { 5, 1, 6, 14, 5, 9 },   { 5, 17, 6, 14, 5, 9 },
    { 5, 1, 12, 14, 5, 9 }, { 5, 17, 12, 14, 5, 9 }, { 5, 13, 20, 11, 3, 9 },
};

static const BPlistWinData data_ov287_021faf5c[11] = {
    { 5, 4, 4, 11, 2, 9 },  { 5, 23, 4, 5, 2, 9 }, { 5, 25, 9, 3, 2, 9 }, { 5, 25, 7, 3, 2, 9 },
    { 5, 1, 12, 30, 6, 9 }, { 5, 6, 9, 8, 2, 9 },  { 5, 5, 1, 9, 2, 9 },  { 5, 20, 4, 2, 2, 9 },
    { 5, 16, 9, 8, 2, 9 },  { 5, 16, 7, 8, 2, 9 }, { 5, 4, 7, 9, 2, 9 },
};

static const BPlistWinData data_ov287_021fafa0[12] = {
    { 5, 5, 1, 9, 2, 9 },   { 5, 4, 4, 11, 2, 9 }, { 5, 20, 4, 2, 2, 9 }, { 5, 23, 4, 5, 2, 9 },
    { 5, 16, 9, 8, 2, 9 },  { 5, 16, 7, 8, 2, 9 }, { 5, 25, 9, 3, 2, 9 }, { 5, 25, 7, 3, 2, 9 },
    { 5, 1, 12, 30, 6, 9 }, { 5, 4, 7, 9, 2, 9 },  { 5, 6, 9, 8, 2, 9 },  { 5, 7, 20, 12, 3, 9 },
};

static const BPlistWinData data_ov287_021fafe8[22] = {
    { 5, 5, 1, 9, 2, 9 },   { 5, 1, 9, 11, 2, 9 },   { 5, 2, 11, 18, 4, 9 }, { 5, 4, 16, 12, 2, 9 },
    { 5, 24, 4, 7, 2, 9 },  { 5, 28, 7, 3, 2, 9 },   { 5, 28, 9, 3, 2, 9 },  { 5, 28, 15, 3, 2, 9 },
    { 5, 28, 11, 3, 2, 9 }, { 5, 28, 13, 3, 2, 9 },  { 5, 25, 6, 6, 1, 9 },  { 5, 5, 4, 3, 2, 9 },
    { 5, 13, 6, 6, 2, 9 },  { 5, 21, 4, 2, 2, 9 },   { 5, 21, 7, 6, 2, 9 },  { 5, 21, 9, 6, 2, 9 },
    { 5, 21, 15, 6, 2, 9 }, { 5, 21, 11, 6, 2, 9 },  { 5, 21, 13, 6, 2, 9 }, { 5, 1, 4, 4, 2, 9 },
    { 5, 1, 6, 12, 2, 9 },  { 5, 13, 20, 11, 3, 9 },
};

void BPlistBmp_Init(BPlistWork *work) {
    const BPlistWinData *data = data_ov287_021fae4c;
    u32 i;

    for (i = 0; i < 2; i++) {
        work->msgWins[i].window =
            BmpWin_CreateDynamic(data->bg, data->x, data->y, data->width, data->height, data->palette, TRUE);
        data++;
    }
    BPlistBmp_CreatePageWindows(work, work->page);
}

void BPlistBmp_CreatePageWindows(BPlistWork *work, u8 page) {
    const BPlistWinData *data;
    u32 i;

    BPlistBmp_FreeDummyWindow(work);
    BPlistBmp_ToggleDummyWindow(work);

    switch (page) {
    case 0:
    case 8:
        data = data_ov287_021faf12;
        work->windowCount = 6;
        break;
    case 1:
        data = data_ov287_021faeb8;
        work->windowCount = 4;
        break;
    case 2:
        data = data_ov287_021fafe8;
        work->windowCount = 22;
        break;
    case 3:
        data = data_ov287_021faf36;
        work->windowCount = 6;
        break;
    case 4:
        data = data_ov287_021faf5c;
        work->windowCount = 11;
        break;
    case 5:
        data = data_ov287_021faed0;
        work->windowCount = 5;
        break;
    case 6:
        data = data_ov287_021faeee;
        work->windowCount = 6;
        break;
    case 7:
        data = data_ov287_021fafa0;
        work->windowCount = 12;
        break;
    }

    for (i = 0; i < work->windowCount; i++) {
        work->windows[i].window =
            BmpWin_CreateDynamic(data->bg, data->x, data->y, data->width, data->height, data->palette, TRUE);
        data++;
    }
}

static void BPlistBmp_ToggleDummyWindow(BPlistWork *work) {
    if (work->windowSwap == 1) {
        work->dummyWin = BmpWin_CreateDynamic(5, 0, 0, 32, 16, 0, TRUE);
    }
    work->windowSwap ^= 1;
}

static void BPlistBmp_FreeDummyWindow(BPlistWork *work) {
    if (work->dummyWin != NULL) {
        BmpWin_Free(work->dummyWin);
        work->dummyWin = NULL;
    }
}

static void BPlistBmp_ClearPageWindows(BPlistWork *work) {
    u32 i;

    for (i = 0; i < work->windowCount; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(work->windows[i].window), 0);
    }
}

void BPlistBmp_FreePageWindows(BPlistWork *work) {
    u32 i;

    for (i = 0; i < work->windowCount; i++) {
        BmpWin_Free(work->windows[i].window);
    }
}

void BPlistBmp_Exit(BPlistWork *work) {
    u32 i;

    BPlistBmp_FreePageWindows(work);
    for (i = 0; i < 2; i++) {
        BmpWin_Free(work->msgWins[i].window);
    }
    BPlistBmp_FreeDummyWindow(work);
}

void BPlistBmp_DrawPage(BPlistWork *work, u8 page) {
    switch (page) {
    case 0:
        BPlistBmp_DrawPartyPage(work);
        if (work->param->unk1F == 3) {
            BPlistBmp_PrintInfo(work, 7);
        } else if (work->param->unk1F == 2) {
            BPlistBmp_PrintInfo(work, 9);
        } else {
            BPlistBmp_PrintInfo(work, 6);
        }
        break;
    case 8:
        BPlistBmp_DrawPartyPage(work);
        BPlistBmp_PrintInfo(work, 10);
        break;
    case 1:
        BPlistBmp_DrawSelectPage(work);
        break;
    case 2:
        BPlistBmp_DrawStatusPage(work);
        break;
    case 3:
        BPlistBmp_DrawMovesPage(work);
        break;
    case 4:
        BPlistBmp_DrawMoveInfoPage(work);
        break;
    case 5:
        BPlistBmp_DrawPPRestorePage(work);
        break;
    case 6:
        BPlistBmp_DrawForgetPage(work);
        break;
    case 7:
        BPlistBmp_DrawForgetInfoPage(work);
        break;
    }
}

static void BPlistBmp_PrintName(BPlistWork *work, u32 winIdx, u16 pos, u8 x, u8 y) {
    BmpWin *win = work->windows[winIdx].window;
    BPlistPokemon *pokemon;
    StrBuf *str;
    StrBuf *tmpl;
    u16 sexX;
    u8 palette;

    palette = BmpWin_GetPalette(win);
    pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    str = GFL_StrBufCreate(12, work->param->heapId);
    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, data_ov287_021faea0[pos]);
    loadPokemonNicknameToStrbuf(work->wordSet, 0, pokemon->pkm);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    if (palette == 9) {
        PrintWindow_Print(&work->windows[winIdx], work->printQueue, x, y, str, work->param->font, BPLIST_COLOR_WHITE);
    } else {
        PrintWindow_Print(&work->windows[winIdx], work->printQueue, x, y, str, work->param->font, BPLIST_COLOR_WHITE);
    }
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);

    if (pokemon->hideSex || pokemon->isEgg) {
        return;
    }
    if (pokemon->sex == 0) {
        tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 22);
        sexX = BmpWin_GetSizeX(win) * 8 - GFL_FontGetBlockWidth(tmpl, work->param->font, 0);
        if (palette == 9) {
            PrintWindow_Print(&work->windows[winIdx], work->printQueue, sexX, y, tmpl, work->param->font,
                              BPLIST_COLOR_MALE);
        } else {
            PrintWindow_Print(&work->windows[winIdx], work->printQueue, sexX, y, tmpl, work->param->font,
                              BPLIST_COLOR_MALE);
        }
        GFL_StrBufFree(tmpl);
    } else if (pokemon->sex == 1) {
        tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 23);
        sexX = BmpWin_GetSizeX(win) * 8 - GFL_FontGetBlockWidth(tmpl, work->param->font, 0);
        if (palette == 9) {
            PrintWindow_Print(&work->windows[winIdx], work->printQueue, sexX, y, tmpl, work->param->font,
                              BPLIST_COLOR_FEMALE);
        } else {
            PrintWindow_Print(&work->windows[winIdx], work->printQueue, sexX, y, tmpl, work->param->font,
                              BPLIST_COLOR_FEMALE);
        }
        GFL_StrBufFree(tmpl);
    }
}

static void BPlistBmp_PrintLevel(BPlistWork *work, u32 winIdx, u8 pos, u8 x, u8 y) {
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    StrBuf *tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 11);

    WordSetNumber(work->wordSet, 0, pokemon->level, 3, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, tmpl);
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, x, y, work->strBuf, work->smallFont,
                      BPLIST_COLOR_WHITE);
    GFL_StrBufFree(tmpl);
}

static void BPlistBmp_PrintHP(BPlistWork *work, u32 winIdx, u8 pos, u8 x, u8 y) {
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, 12);
    u8 slashWidth = GFL_FontGetBlockWidth(str, work->smallFont, 0);

    x = x - slashWidth / 2;
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, x, y, str, work->smallFont, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(work->msgData, 13);
    WordSetNumber(work->wordSet, 0, pokemon->hp, 3, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, str);
    PrintWindow_Print(&work->windows[winIdx], work->printQueue,
                      x - GFL_FontGetBlockWidth(work->strBuf, work->smallFont, 0), y, work->strBuf, work->smallFont,
                      BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(work->msgData, 13);
    WordSetNumber(work->wordSet, 0, pokemon->maxHp, 3, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, str);
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, x + slashWidth, y, work->strBuf, work->smallFont,
                      BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);
}

static void BPlistBmp_DrawHPBar(BPlistWork *work, u32 winIdx, u8 pos, u8 x, u8 y) {
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    u8 color = 1;
    u8 fill = HPGauge_GetFill(pokemon->hp, pokemon->maxHp, 48);

    switch (HPGauge_GetColor(pokemon->hp, pokemon->maxHp)) {
    case HP_GAUGE_COLOR_NONE:
        return;
    case HP_GAUGE_COLOR_GREEN:
        color = 1;
        break;
    case HP_GAUGE_COLOR_YELLOW:
        color = 3;
        break;
    case HP_GAUGE_COLOR_RED:
        color = 5;
        break;
    }
    GFL_BitmapFillArea(BmpWin_GetBitmap(work->windows[winIdx].window), x, y + 3, fill, 1, color);
    GFL_BitmapFillArea(BmpWin_GetBitmap(work->windows[winIdx].window), x, y + 4, fill, 1, color + 1);
}

static void BPlistBmp_PrintAbility(BPlistWork *work, u32 winIdx, u8 pos) {
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    StrBuf *str = GFL_StrBufCreate(16, work->param->heapId);
    StrBuf *tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 14);

    loadAbilityNameToStrbuf(work->wordSet, 0, pokemon->ability);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);
}

static void BPlistBmp_PrintItem(BPlistWork *work, u32 winIdx, u8 pos) {
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    StrBuf *str;

    if (pokemon->item == 0) {
        str = GFL_MsgDataLoadStrbufNew(work->msgData, 30);
    } else {
        StrBuf *tmpl;

        str = GFL_StrBufCreate(18, work->param->heapId);
        tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 15);
        loadItemNameToStrbuf(work->wordSet, 0, pokemon->item);
        GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
        GFL_StrBufFree(tmpl);
    }
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);
}

static void BPlistBmp_PrintMoveName(BPlistWork *work, u32 move, u32 winIdx, u32 msgId, u16 x, s16 y, u32 color) {
    BmpWin *win = work->windows[winIdx].window;
    StrBuf *str = GFL_StrBufCreate(16, work->param->heapId);
    StrBuf *tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, msgId);

    loadMoveNameToStrbuf(work->wordSet, 0, move);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    if (x == 0xffff) {
        x = (BmpWin_GetSizeX(win) * 8 - GFL_FontGetBlockWidth(str, work->param->font, 0)) / 2;
    }
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, x, y, str, work->param->font, color);
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);
}

static void BPlistBmp_PrintTypeLabel(BPlistWork *work, u32 winIdx, u8 x, u8 y) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, 20);

    PrintWindow_Print(&work->windows[winIdx], work->printQueue, x, y, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);
}

static void BPlistBmp_PrintInfo(BPlistWork *work, u32 msgId) {
    StrBuf *str;

    GFL_BitmapFill(BmpWin_GetBitmap(work->msgWins[0].window), 15);
    str = GFL_MsgDataLoadStrbufNew(work->msgData, msgId);
    PrintWindow_Print(&work->msgWins[0], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_BLACK);
    GFL_StrBufFree(str);
    work->infoWinFramePending = 1;
}

void BPlistBmp_PrintInfo9(BPlistWork *work) {
    BPlistBmp_PrintInfo(work, 9);
}

void BPlistBmp_PrintInfo10(BPlistWork *work) {
    BPlistBmp_PrintInfo(work, 10);
}

static void BPlistBmp_PrintButtonLabel(BPlistWork *work, u32 winIdx, u32 msgId) {
    BmpWin *win = work->windows[winIdx].window;
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, msgId);
    u32 width = GFL_FontGetBlockWidth(str, work->param->font, 0);

    PrintWindow_Print(&work->windows[winIdx], work->printQueue, (BmpWin_GetSizeX(win) * 8 - width) / 2, 5, str,
                      work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);
}

static void BPlistBmp_PrintLevelExp(BPlistWork *work, u8 pos) {
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    StrBuf *str;
    StrBuf *tmpl;
    u16 x;

    str = GFL_MsgDataLoadStrbufNew(work->msgData, 33);
    PrintWindow_Print(&work->windows[19], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 34);
    str = GFL_StrBufCreate(8, work->param->heapId);
    WordSetNumber(work->wordSet, 0, pokemon->level, 3, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    PrintWindow_Print(&work->windows[11], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(work->msgData, 35);
    PrintWindow_Print(&work->windows[20], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 36);
    str = GFL_StrBufCreate(14, work->param->heapId);
    if (pokemon->level < 100) {
        WordSetNumber(work->wordSet, 0, pokemon->nextLevelExp - pokemon->exp, 6, NUM_PAD_SPACE, TRUE);
    } else {
        WordSetNumber(work->wordSet, 0, 0, 6, NUM_PAD_SPACE, TRUE);
    }
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    x = BmpWin_GetSizeX(work->windows[12].window) * 8 - GFL_FontGetBlockWidth(str, work->param->font, 0);
    PrintWindow_Print(&work->windows[12], work->printQueue, x, 0, str, work->param->font, BPLIST_COLOR_STAT);
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);
}

static void BPlistBmp_PrintAttack(BPlistWork *work, u8 pos) {
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    StrBuf *str;
    StrBuf *tmpl;
    u8 width;
    u8 x;

    str = GFL_MsgDataLoadStrbufNew(work->msgData, 42);
    PrintWindow_Print(&work->windows[14], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 43);
    str = GFL_StrBufCreate(8, work->param->heapId);
    WordSetNumber(work->wordSet, 0, pokemon->attack, 3, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    width = GFL_FontGetBlockWidth(str, work->param->font, 0);
    x = BmpWin_GetSizeX(work->windows[5].window) * 8 - width;
    PrintWindow_Print(&work->windows[5], work->printQueue, x, 0, str, work->param->font, BPLIST_COLOR_STAT);
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);
}

static void BPlistBmp_PrintDefense(BPlistWork *work, u8 pos) {
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    StrBuf *str;
    StrBuf *tmpl;
    u8 width;
    u8 x;

    str = GFL_MsgDataLoadStrbufNew(work->msgData, 44);
    PrintWindow_Print(&work->windows[15], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 45);
    str = GFL_StrBufCreate(8, work->param->heapId);
    WordSetNumber(work->wordSet, 0, pokemon->defense, 3, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    width = GFL_FontGetBlockWidth(str, work->param->font, 0);
    x = BmpWin_GetSizeX(work->windows[6].window) * 8 - width;
    PrintWindow_Print(&work->windows[6], work->printQueue, x, 0, str, work->param->font, BPLIST_COLOR_STAT);
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);
}
static void BPlistBmp_PrintSpeed(BPlistWork *work, u8 pos) {
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    StrBuf *str;
    StrBuf *tmpl;
    u8 width;
    u8 x;

    str = GFL_MsgDataLoadStrbufNew(work->msgData, 50);
    PrintWindow_Print(&work->windows[16], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 51);
    str = GFL_StrBufCreate(8, work->param->heapId);
    WordSetNumber(work->wordSet, 0, pokemon->speed, 3, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    width = GFL_FontGetBlockWidth(str, work->param->font, 0);
    x = BmpWin_GetSizeX(work->windows[7].window) * 8 - width;
    PrintWindow_Print(&work->windows[7], work->printQueue, x, 0, str, work->param->font, BPLIST_COLOR_STAT);
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);
}
static void BPlistBmp_PrintSpAttack(BPlistWork *work, u8 pos) {
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    StrBuf *str;
    StrBuf *tmpl;
    u8 width;
    u8 x;

    str = GFL_MsgDataLoadStrbufNew(work->msgData, 46);
    PrintWindow_Print(&work->windows[17], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 47);
    str = GFL_StrBufCreate(8, work->param->heapId);
    WordSetNumber(work->wordSet, 0, pokemon->spAttack, 3, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    width = GFL_FontGetBlockWidth(str, work->param->font, 0);
    x = BmpWin_GetSizeX(work->windows[8].window) * 8 - width;
    PrintWindow_Print(&work->windows[8], work->printQueue, x, 0, str, work->param->font, BPLIST_COLOR_STAT);
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);
}
static void BPlistBmp_PrintSpDefense(BPlistWork *work, u8 pos) {
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    StrBuf *str;
    StrBuf *tmpl;
    u8 width;
    u8 x;

    str = GFL_MsgDataLoadStrbufNew(work->msgData, 48);
    PrintWindow_Print(&work->windows[18], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 49);
    str = GFL_StrBufCreate(8, work->param->heapId);
    WordSetNumber(work->wordSet, 0, pokemon->spDefense, 3, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    width = GFL_FontGetBlockWidth(str, work->param->font, 0);
    x = BmpWin_GetSizeX(work->windows[9].window) * 8 - width;
    PrintWindow_Print(&work->windows[9], work->printQueue, x, 0, str, work->param->font, BPLIST_COLOR_STAT);
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);
}
static void BPlistBmp_PrintStatusHP(BPlistWork *work, u8 pos) {
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    StrBuf *str;
    StrBuf *tmpl;
    u32 slashWidth;
    u32 width;
    u16 x;

    str = GFL_MsgDataLoadStrbufNew(work->msgData, 38);
    PrintWindow_Print(&work->windows[13], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);

    str = GFL_MsgDataLoadStrbufNew(work->msgData, 41);
    slashWidth = GFL_FontGetBlockWidth(str, work->param->font, 0);
    x = (BmpWin_GetSizeX(work->windows[4].window) * 8 - slashWidth) / 2;
    PrintWindow_Print(&work->windows[4], work->printQueue, x, 0, str, work->param->font, BPLIST_COLOR_STAT);
    GFL_StrBufFree(str);

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 39);
    str = GFL_StrBufCreate(8, work->param->heapId);
    WordSetNumber(work->wordSet, 0, pokemon->hp, 3, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    width = GFL_FontGetBlockWidth(str, work->param->font, 0);
    PrintWindow_Print(&work->windows[4], work->printQueue, x - width, 0, str, work->param->font, BPLIST_COLOR_STAT);
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 40);
    str = GFL_StrBufCreate(8, work->param->heapId);
    WordSetNumber(work->wordSet, 0, pokemon->maxHp, 3, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    PrintWindow_Print(&work->windows[4], work->printQueue, x + slashWidth, 0, str, work->param->font,
                      BPLIST_COLOR_STAT);
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);
}

static void BPlistBmp_PrintAbilityInfo(BPlistWork *work, u8 pos) {
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_ABILITY_DESCRIPTIONS, work->param->heapId);
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgData, pokemon->ability);

    PrintWindow_Print(&work->windows[2], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_STAT);
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);
}

static void BPlistBmp_PrintAccuracyLabel(BPlistWork *work, u32 winIdx) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, 61);

    PrintWindow_Print(&work->windows[winIdx], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);
}

static void BPlistBmp_PrintAccuracy(BPlistWork *work, u32 winIdx, u32 value) {
    BmpWin *win = work->windows[winIdx].window;
    StrBuf *str;
    StrBuf *tmpl;
    u16 width;
    u16 x;

    if (value == 0) {
        str = GFL_MsgDataLoadStrbufNew(work->msgData, 60);
        width = GFL_FontGetBlockWidth(str, work->param->font, 0);
        x = BmpWin_GetSizeX(win) * 8 - width;
        PrintWindow_Print(&work->windows[winIdx], work->printQueue, x, 0, str, work->param->font, BPLIST_COLOR_STAT);
        GFL_StrBufFree(str);
    } else {
        tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 62);
        str = GFL_StrBufCreate(8, work->param->heapId);
        WordSetNumber(work->wordSet, 0, value, 3, NUM_PAD_NONE, TRUE);
        GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
        width = GFL_FontGetBlockWidth(str, work->param->font, 0);
        x = BmpWin_GetSizeX(win) * 8 - width;
        PrintWindow_Print(&work->windows[winIdx], work->printQueue, x, 0, str, work->param->font, BPLIST_COLOR_STAT);
        GFL_StrBufFree(tmpl);
        GFL_StrBufFree(str);
    }
}

static void BPlistBmp_PrintPowerLabel(BPlistWork *work, u32 winIdx) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, 58);

    PrintWindow_Print(&work->windows[winIdx], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);
}

static void BPlistBmp_PrintPower(BPlistWork *work, u32 winIdx, u32 value) {
    BmpWin *win = work->windows[winIdx].window;
    StrBuf *str;
    StrBuf *tmpl;
    u16 width;
    u16 x;

    if (value <= 1) {
        str = GFL_MsgDataLoadStrbufNew(work->msgData, 60);
        width = GFL_FontGetBlockWidth(str, work->param->font, 0);
        x = BmpWin_GetSizeX(win) * 8 - width;
        PrintWindow_Print(&work->windows[winIdx], work->printQueue, x, 0, str, work->param->font, BPLIST_COLOR_STAT);
        GFL_StrBufFree(str);
    } else {
        tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 59);
        str = GFL_StrBufCreate(8, work->param->heapId);
        WordSetNumber(work->wordSet, 0, value, 3, NUM_PAD_NONE, TRUE);
        GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
        width = GFL_FontGetBlockWidth(str, work->param->font, 0);
        x = BmpWin_GetSizeX(win) * 8 - width;
        PrintWindow_Print(&work->windows[winIdx], work->printQueue, x, 0, str, work->param->font, BPLIST_COLOR_STAT);
        GFL_StrBufFree(tmpl);
        GFL_StrBufFree(str);
    }
}

static void BPlistBmp_PrintMoveInfo(BPlistWork *work, u32 winIdx, u32 msgId) {
    MsgData *msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BTL_MAIN_13, work->param->heapId);
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgData, msgId);

    PrintWindow_Print(&work->windows[winIdx], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);
    GFL_MsgDataFree(msgData);
}

static void BPlistBmp_PrintCategoryLabel(BPlistWork *work, u32 winIdx) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, 63);

    GFL_FontGetBlockWidth(str, work->param->font, 0);
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);
}

static void BPlistBmp_PrintCategory(BPlistWork *work, u32 winIdx, u32 category) {
    StrBuf *str;

    switch (category) {
    case 0:
        str = GFL_MsgDataLoadStrbufNew(work->msgData, 65);
        break;
    case 1:
        str = GFL_MsgDataLoadStrbufNew(work->msgData, 64);
        break;
    case 2:
        str = GFL_MsgDataLoadStrbufNew(work->msgData, 66);
        break;
    }
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, 0, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);
}

static void BPlistBmp_PrintMovePP(BPlistWork *work, u32 winIdx, u32 pp, u32 maxPp) {
    BmpWin *win = work->windows[winIdx].window;
    StrBuf *str;
    StrBuf *tmpl;
    u32 slashWidth;
    u32 width;
    u32 x;

    str = GFL_MsgDataLoadStrbufNew(work->msgData, 56);
    slashWidth = GFL_FontGetBlockWidth(str, work->param->font, 0);
    x = (BmpWin_GetSizeX(win) * 8 - slashWidth) / 2;
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, x, 0, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 54);
    str = GFL_StrBufCreate(6, work->param->heapId);
    WordSetNumber(work->wordSet, 0, pp, 3, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    width = GFL_FontGetBlockWidth(str, work->param->font, 0);
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, x - width, 0, str, work->param->font,
                      BPLIST_COLOR_WHITE);
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 55);
    str = GFL_StrBufCreate(6, work->param->heapId);
    WordSetNumber(work->wordSet, 0, maxPp, 3, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, x + slashWidth, 0, str, work->param->font,
                      BPLIST_COLOR_WHITE);
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);
}

static void BPlistBmp_PrintForgetButton(BPlistWork *work, u32 winIdx) {
    StrBuf *str;
    u32 width;

    if (work->param->slot == 4) {
        str = GFL_MsgDataLoadStrbufNew(work->msgData, 69);
    } else {
        str = GFL_MsgDataLoadStrbufNew(work->msgData, 68);
    }
    width = GFL_FontGetBlockWidth(str, work->param->font, 0);
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, (96 - width) / 2, 5, str, work->param->font,
                      BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);
}

void BPlistBmp_PrintMessage(BPlistWork *work) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, 70);

    GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, str);
    GFL_StrBufFree(str);
    BPlistBmp_OpenMessage(work);
}

static void BPlistBmp_PrintPlatePP(BPlistWork *work, BPlistMove *move, u32 winIdx) {
    StrBuf *str = GFL_StrBufCreate(6, work->param->heapId);
    StrBuf *tmpl;
    u32 slashWidth;
    u32 width;

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 53);
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, 40, 24, tmpl, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(tmpl);

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 56);
    slashWidth = GFL_FontGetBlockWidth(tmpl, work->param->font, 0);
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, 80, 24, tmpl, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(tmpl);

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 55);
    WordSetNumber(work->wordSet, 0, move->maxPp, 2, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    slashWidth += 80;
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, slashWidth, 24, str, work->param->font,
                      BPLIST_COLOR_WHITE);
    GFL_StrBufFree(tmpl);

    tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, 54);
    WordSetNumber(work->wordSet, 0, move->pp, 2, NUM_PAD_NONE, TRUE);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    width = GFL_FontGetBlockWidth(str, work->param->font, 0);
    PrintWindow_Print(&work->windows[winIdx], work->printQueue, 80 - width, 24, str, work->param->font,
                      BPLIST_COLOR_WHITE);
    GFL_StrBufFree(tmpl);
    GFL_StrBufFree(str);
}

static void BPlistBmp_DrawPartyPage(BPlistWork *work) {
    s16 i;

    work->flushList = data_ov287_021fae37;
    for (i = 0; i < 6; i++) {
        u8 idx;

        GFL_BitmapFill(BmpWin_GetBitmap(work->windows[i].window), 0);
        idx = BPlistMain_GetPartySlot(work, i);
        if (work->pokemon[idx].species == 0) {
            work->windows[i].flushPending = TRUE;
            continue;
        }
        BPlistBmp_PrintName(work, i, i, 32, 7);
        if (!work->pokemon[idx].isEgg) {
            BPlistBmp_PrintPlateHP(work, i);
        }
        if (AppMenuCommon_GetStatusIcon(work->pokemon[idx].pkm) == APP_STATUS_ICON_NONE) {
            BPlistBmp_PrintPlateLevel(work, i);
        }
    }
}

static void BPlistBmp_PrintPlateHP(BPlistWork *work, u8 pos) {
    GFL_BitmapFillArea(BmpWin_GetBitmap(work->windows[pos].window), 92, 32, 24, 8, 0);
    GFL_BitmapFillArea(BmpWin_GetBitmap(work->windows[pos].window), 64, 24, 64, 8, 0);
    BPlistBmp_PrintHP(work, pos, pos, 92, 32);
    BPlistBmp_DrawHPBar(work, pos, pos, 64, 24);
}

static void BPlistBmp_PrintPlateLevel(BPlistWork *work, u8 pos) {
    if (!work->pokemon[BPlistMain_GetPartySlot(work, pos)].isEgg) {
        BPlistBmp_PrintLevel(work, pos, pos, 8, 32);
    }
}

static void BPlistBmp_DrawSelectPage(BPlistWork *work) {
    int result;
    u32 msgId;

    work->flushList = data_ov287_021fae2c;
    BPlistBmp_ClearPageWindows(work);
    BPlistBmp_PrintCenteredName(work, work->param->partyIndex);
    result = BPlistMain_GetSwitchError(work);
    if (result == 2) {
        msgId = 26;
    } else if (result == 3) {
        msgId = 27;
    } else if (result == 4) {
        msgId = 28;
    } else if (result == 1) {
        msgId = 29;
    } else {
        msgId = 21;
    }
    BPlistBmp_PrintButtonLabel(work, 1, msgId);
    if (!work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].isEgg) {
        BPlistBmp_PrintButtonLabel(work, 2, 24);
        BPlistBmp_PrintButtonLabel(work, 3, 25);
    } else {
        BmpWin_Transfer(work->windows[2].window);
        BmpWin_Transfer(work->windows[3].window);
    }
}

static void BPlistBmp_PrintCenteredName(BPlistWork *work, u8 pos) {
    BmpWin *win = work->windows[0].window;
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, pos)];
    StrBuf *str = GFL_StrBufCreate(12, work->param->heapId);
    StrBuf *tmpl = GFL_MsgDataLoadStrbufNew(work->msgData, data_ov287_021faea0[pos]);
    StrBuf *sexStr = NULL;
    u8 nameWidth;
    u8 sexWidth;
    u8 gap;
    u8 x;

    loadPokemonNicknameToStrbuf(work->wordSet, 0, pokemon->pkm);
    GFL_WordSetFormatStrbuf(work->wordSet, str, tmpl);
    GFL_StrBufFree(tmpl);

    if (!pokemon->hideSex && !pokemon->isEgg) {
        if (pokemon->sex == 0) {
            sexStr = GFL_MsgDataLoadStrbufNew(work->msgData, 22);
        } else if (pokemon->sex == 1) {
            sexStr = GFL_MsgDataLoadStrbufNew(work->msgData, 23);
        }
    }

    nameWidth = GFL_FontGetBlockWidth(str, work->param->font, 0);
    sexWidth = 0;
    if (sexStr == NULL) {
        gap = 0;
    } else {
        sexWidth = GFL_FontGetBlockWidth(sexStr, work->param->font, 0);
        gap = 8;
    }
    x = (BmpWin_GetSizeX(win) * 8 - nameWidth - sexWidth - gap) / 2;
    PrintWindow_Print(&work->windows[0], work->printQueue, x, 7, str, work->param->font, BPLIST_COLOR_WHITE);
    GFL_StrBufFree(str);

    if (sexStr != NULL) {
        if (pokemon->sex == 0) {
            PrintWindow_Print(&work->windows[0], work->printQueue, x + nameWidth + gap, 8, sexStr, work->param->font,
                              BPLIST_COLOR_MALE);
        } else {
            PrintWindow_Print(&work->windows[0], work->printQueue, x + nameWidth + gap, 8, sexStr, work->param->font,
                              BPLIST_COLOR_FEMALE);
        }
        GFL_StrBufFree(sexStr);
    }
}

static void BPlistBmp_DrawMovesPage(BPlistWork *work) {
    u32 i;

    work->flushList = data_ov287_021fae3e;
    BPlistBmp_ClearPageWindows(work);
    BPlistBmp_PrintName(work, 0, work->param->partyIndex, 0, 0);
    for (i = 0; i < 4; i++) {
        BPlistMove *move = &work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].moves[i];

        if (move->move == 0) {
            work->windows[1 + i].flushPending = TRUE;
            continue;
        }
        BPlistBmp_PrintMoveName(work, move->move, 1 + i, data_ov287_021fae74[i], 0xffff, 7, BPLIST_COLOR_WHITE);
        BPlistBmp_PrintPlatePP(work, move, 1 + i);
    }
    BPlistBmp_PrintButtonLabel(work, 5, 24);
}

static void BPlistBmp_DrawStatusPage(BPlistWork *work) {
    work->flushList = data_ov287_021fae88;
    BPlistBmp_ClearPageWindows(work);
    BPlistBmp_PrintName(work, 0, work->param->partyIndex, 0, 0);
    BPlistBmp_PrintStatusHP(work, work->param->partyIndex);
    BPlistBmp_DrawHPBar(work, 10, work->param->partyIndex, 0, 0);
    BmpWin_FlushChar(work->windows[10].window);
    BPlistBmp_PrintLevelExp(work, work->param->partyIndex);
    BPlistBmp_PrintAttack(work, work->param->partyIndex);
    BPlistBmp_PrintDefense(work, work->param->partyIndex);
    BPlistBmp_PrintSpeed(work, work->param->partyIndex);
    BPlistBmp_PrintSpAttack(work, work->param->partyIndex);
    BPlistBmp_PrintSpDefense(work, work->param->partyIndex);
    BPlistBmp_PrintAbility(work, 1, work->param->partyIndex);
    BPlistBmp_PrintItem(work, 3, work->param->partyIndex);
    BPlistBmp_PrintAbilityInfo(work, work->param->partyIndex);
    BPlistBmp_PrintButtonLabel(work, 21, 25);
}

static void BPlistBmp_DrawMoveInfoPage(BPlistWork *work) {
    BPlistParam *param;
    BPlistMove *move;

    work->flushList = data_ov287_021fae58;
    BPlistBmp_ClearPageWindows(work);
    param = work->param;
    move = &work->pokemon[BPlistMain_GetPartySlot(work, param->partyIndex)].moves[param->slot];
    BPlistBmp_PrintName(work, 6, param->partyIndex, 0, 0);
    BPlistBmp_PrintTypeLabel(work, 7, 0, 0);
    BPlistBmp_PrintMoveName(work, move->move, 0, data_ov287_021fae74[work->param->slot], 0, 0, BPLIST_COLOR_WHITE);
    BPlistBmp_PrintAccuracyLabel(work, 8);
    BPlistBmp_PrintAccuracy(work, 2, move->accuracy);
    BPlistBmp_PrintPowerLabel(work, 9);
    BPlistBmp_PrintPower(work, 3, move->power);
    BPlistBmp_PrintMoveInfo(work, 4, move->move);
    BPlistBmp_PrintCategoryLabel(work, 10);
    BPlistBmp_PrintCategory(work, 5, move->category);
    BPlistBmp_PrintMovePP(work, 1, move->pp, move->maxPp);
}

static void BPlistBmp_DrawForgetPage(BPlistWork *work) {
    u32 i;
    BPlistMove newMove;

    work->flushList = data_ov287_021fae45;
    BPlistBmp_ClearPageWindows(work);
    BPlistBmp_PrintName(work, 0, work->param->partyIndex, 0, 0);
    for (i = 0; i < 4; i++) {
        BPlistMove *move = &work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].moves[i];

        if (move->move == 0) {
            work->windows[1 + i].flushPending = TRUE;
            continue;
        }
        BPlistBmp_PrintMoveName(work, move->move, 1 + i, data_ov287_021fae74[i], 0xffff, 7, BPLIST_COLOR_WHITE);
        BPlistBmp_PrintPlatePP(work, move, 1 + i);
    }
    BPlistBmp_PrintMoveName(work, work->param->move, 5, data_ov287_021fae74[4], 0xffff, 7, BPLIST_COLOR_WHITE);
    newMove.pp = PML_MoveGetParam(work->param->move, MOVE_PARAM_PP);
    newMove.maxPp = newMove.pp;
    BPlistBmp_PrintPlatePP(work, &newMove, 5);
}

static void BPlistBmp_DrawForgetInfoPage(BPlistWork *work) {
    u8 slot;

    work->flushList = data_ov287_021fae64;
    BPlistBmp_ClearPageWindows(work);
    BPlistBmp_PrintName(work, 0, work->param->partyIndex, 0, 0);
    BPlistBmp_PrintTypeLabel(work, 2, 0, 0);
    BPlistBmp_PrintAccuracyLabel(work, 4);
    BPlistBmp_PrintPowerLabel(work, 5);
    BPlistBmp_PrintCategoryLabel(work, 9);
    slot = work->param->slot;
    if (slot < 4) {
        BPlistMove *move = &work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].moves[slot];

        BPlistBmp_PrintMoveName(work, move->move, 1, data_ov287_021fae74[slot], 0, 0, BPLIST_COLOR_WHITE);
        BPlistBmp_PrintAccuracy(work, 6, move->accuracy);
        BPlistBmp_PrintPower(work, 7, move->power);
        BPlistBmp_PrintMoveInfo(work, 8, move->move);
        BPlistBmp_PrintCategory(work, 10, move->category);
        BPlistBmp_PrintMovePP(work, 3, move->pp, move->maxPp);
    } else {
        u32 pp = PML_MoveGetParam(work->param->move, MOVE_PARAM_PP);

        BPlistBmp_PrintMoveName(work, work->param->move, 1, data_ov287_021fae74[4], 0, 0, BPLIST_COLOR_WHITE);
        BPlistBmp_PrintMoveInfo(work, 8, work->param->move);
        if (PML_MoveIsAlwaysHit(work->param->move) == TRUE) {
            BPlistBmp_PrintAccuracy(work, 6, 0);
        } else {
            BPlistBmp_PrintAccuracy(work, 6, PML_MoveGetParam(work->param->move, MOVE_PARAM_ACCURACY));
        }
        BPlistBmp_PrintPower(work, 7, PML_MoveGetParam(work->param->move, MOVE_PARAM_POWER));
        BPlistBmp_PrintCategory(work, 10, PML_MoveGetParam(work->param->move, MOVE_PARAM_CATEGORY));
        BPlistBmp_PrintMovePP(work, 3, pp, pp);
    }
    BPlistBmp_PrintForgetButton(work, 11);
}

static void BPlistBmp_DrawPPRestorePage(BPlistWork *work) {
    u32 i;

    work->flushList = data_ov287_021fae31;
    BPlistBmp_ClearPageWindows(work);
    BPlistBmp_PrintName(work, 0, work->param->partyIndex, 0, 0);
    for (i = 0; i < 4; i++) {
        BPlistMove *move = &work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)].moves[i];

        if (move->move == 0) {
            work->windows[1 + i].flushPending = TRUE;
            continue;
        }
        BPlistBmp_PrintMoveName(work, move->move, 1 + i, data_ov287_021fae74[i], 0xffff, 7, BPLIST_COLOR_WHITE);
        BPlistBmp_PrintPlatePP(work, move, 1 + i);
    }
    if (GetItemParam(work->param->item, ITEM_PARAM_PP_RESTORE_ALL, work->param->heapId) == 0) {
        BPlistBmp_PrintInfo(work, 104);
    }
}

void BPlistBmp_OpenMessage(BPlistWork *work) {
    BmpWin_DrawFrame(work->msgWins[1].window, 2, 1, 14);
    GFL_BitmapFill(BmpWin_GetBitmap(work->msgWins[1].window), 15);
    BPlistBmp_StartMessageStream(work);
}

static void BPlistBmp_StartMessageStream(BPlistWork *work) {
    GFL_TextRndUpdateColorIndexLUT(1, 2, 0);
    work->printStream = func_02022268(work->msgWins[1].window, 0, 0, work->strBuf, work->param->font, func_02017bcc(),
                                      work->tcbExMgr, 10, work->param->heapId, 15);
    BmpWin_Transfer(work->msgWins[1].window);
}

void BPlistBmp_SetEmbargoMessage(BPlistWork *work) {
    BPlistPokemon *pokemon = &work->pokemon[BPlistMain_GetPartySlot(work, work->param->partyIndex)];
    StrBuf *str = GFL_MsgDataLoadStrbufNew(work->msgData, 105);

    loadPokemonNicknameToStrbuf(work->wordSet, 0, pokemon->pkm);
    loadMoveNameToStrbuf(work->wordSet, 1, MOVE_EMBARGO);
    GFL_WordSetFormatStrbuf(work->wordSet, work->strBuf, str);
    GFL_StrBufFree(str);
}

void BPlistBmp_FlushWindows(BPlistWork *work) {
    BAppTool_FlushPrintWindows(work->msgWins, work->printQueue, 2);
    BAppTool_FlushPrintWindows(work->windows, work->printQueue, work->windowCount);
}

void BPlistBmp_TransferPage(BPlistWork *work) {
    BAppTool_QueueWindowScreens(work->windows, work->flushList);
    BPlistBmp_DrawInfoFrame(work);
}

void BPlistBmp_DrawInfoFrame(BPlistWork *work) {
    if (work->infoWinFramePending == 1) {
        BmpWin_DrawFrame(work->msgWins[0].window, 2, 1, 14);
        BAppTool_QueueWindowScreen(&work->msgWins[0]);
        work->infoWinFramePending = 0;
    }
}
