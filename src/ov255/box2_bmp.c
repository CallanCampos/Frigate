#include "types.h"
#include "app/box2.h"
#include "app/box2_bmp.h"
#include "app/box2_main.h"
#include "app/box2_obj.h"
#include "app/ui/print_msg.h"
#include "constants/arc.h"
#include "constants/items.h"
#include "constants/pokemon.h"
#include "constants/text_banks.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/heap.h"
#include "gfl/msg.h"
#include "gfl/str.h"
#include "pml/item.h"
#include "save/box.h"
#include "system/app_menu_common.h"
#include "system/bgwinfrm.h"
#include "system/bmp_oam.h"
#include "system/bmp_winframe.h"
#include "system/printsys.h"
#include "system/wordset.h"

// The PC box's windows: the Pokémon's data on the upper screen, the buttons of the frames, the box names and counts on
// OAM, and the messages. The ROM doesn't name this file; box2_bmp.c is a guess after box2_main.c. None of these
// functions has a name yet

#define BOX2_WIN_MAX 28

// The colors of the text
#define BOX2_COLOR_NORMAL PRINT_COLOR(1, 2, 0)
#define BOX2_COLOR_MALE PRINT_COLOR(5, 6, 0)
#define BOX2_COLOR_FEMALE PRINT_COLOR(3, 4, 0)
#define BOX2_COLOR_FULL PRINT_COLOR(5, 4, 0)
#define BOX2_COLOR_BUTTON PRINT_COLOR(15, 12, 0)
#define BOX2_COLOR_MSG PRINT_COLOR(1, 2, 15)

typedef struct {
    u8 bg;
    u8 x;
    u8 y;
    u8 width;
    u8 height;
    u8 palette;
} Box2WinData;

// clang-format off
static const Box2WinData sWinData[BOX2_WIN_MAX] = {
    {4, 1, 0, 30, 3, 15},  {4, 1, 6, 8, 2, 15},   {4, 1, 10, 8, 2, 15},  {4, 11, 10, 6, 2, 15},
    {4, 9, 10, 2, 2, 15},  {4, 1, 4, 7, 2, 15},   {4, 2, 14, 8, 2, 15},  {4, 2, 18, 11, 2, 15},
    {4, 2, 22, 12, 2, 15}, {4, 19, 16, 11, 8, 15}, {4, 19, 14, 11, 2, 15}, {4, 1, 12, 8, 2, 15},
    {4, 1, 16, 11, 2, 15}, {4, 1, 20, 12, 2, 15}, {1, 0, 0, 11, 3, 12},  {1, 0, 0, 11, 3, 12},
    {1, 0, 0, 11, 3, 12},  {1, 0, 0, 11, 3, 12},  {1, 0, 0, 11, 3, 12},  {1, 0, 0, 11, 3, 12},
    {1, 0, 9, 11, 3, 1},   {1, 0, 12, 11, 3, 1},  {0, 0, 0, 12, 3, 12},  {0, 0, 0, 12, 3, 12},
    {0, 1, 21, 30, 2, 11}, {0, 1, 19, 18, 4, 11}, {0, 1, 21, 19, 2, 11}, {0, 1, 19, 30, 4, 11},
};

// clang-format on

void Box2Bmp_CreateWindows(Box2SysWork *syswk) {
    const Box2WinData *data;
    u32 i;

    BmpWin_InitAllocator(HEAPID_BOX2_APP);
    data = sWinData;
    for (i = 0; i < BOX2_WIN_MAX; i++) {
        syswk->app->windows[i].window =
            BmpWin_CreateDynamic(data->bg, data->x, data->y, data->width, data->height, data->palette, TRUE);
        data++;
    }
}

void Box2Bmp_FreeWindows(Box2SysWork *syswk) {
    u32 i;

    for (i = 0; i < BOX2_WIN_MAX; i++) {
        BmpWin_Free(syswk->app->windows[i].window);
    }
    BmpWin_FreeAllocator();
}

// Marks a window's characters to be sent to VRAM by Box2Bmp_FlushChars
static void RequestFlushChar(Box2AppWork *app, u32 index) {
    app->flushChar[index / 8] |= 1 << (index % 8);
}

void Box2Bmp_FlushChars(Box2AppWork *app) {
    u32 i;

    for (i = 0; i < BOX2_WIN_MAX; i++) {
        if (app->windows[i].window != NULL) {
            u8 bit = 1 << (i % 8);
            u8 byte = i / 8;

            if (app->flushChar[byte] & bit) {
                BmpWin_FlushChar(app->windows[i].window);
                app->flushChar[byte] ^= bit;
            }
        }
    }
}

void Box2Bmp_UpdatePrint(Box2AppWork *app) {
    u32 i;

    PrintQueue_Main(app->printQueue);
    for (i = 0; i < BOX2_WIN_MAX; i++) {
        PrintWindow_Flush(&app->windows[i], app->printQueue);
    }
}

static void PushWindowScreen(PrintWindow *window) {
    BmpWin_FlushMap(window->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window->window));
}

static void ClearWindowScreen(PrintWindow *window) {
    BmpWin_ClearScreen(window->window);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(window->window));
}

// Prints a message into a window
static void PrintMsg(Box2AppWork *app, u32 index, MsgData *msgData, u32 msgId, u32 x, u32 y, Font *font, u16 color) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgData, msgId);

    PrintWindow_Print(&app->windows[index], app->printQueue, x, y, str, font, color);
    GFL_StrBufFree(str);
}

// Prints a message into a window, with the words of the word set
static void PrintFormattedMsg(Box2AppWork *app, u32 index, MsgData *msgData, u32 msgId, u32 x, u32 y, Font *font,
                              u16 color) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgData, msgId);

    GFL_WordSetFormatStrbuf(app->wordSet, app->expandBuf, str);
    PrintWindow_Print(&app->windows[index], app->printQueue, x, y, app->expandBuf, font, color);
    GFL_StrBufFree(str);
}

static MsgData *LoadBoxMsgData(void) {
    return GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_BOX2_BMP, HEAPID_BOX2_APP);
}

void Box2Bmp_PrintTitle(Box2SysWork *syswk) {
    Box2AppWork *app = syswk->app;
    MsgData *msgData;

    GFL_BitmapFill(BmpWin_GetBitmap(app->windows[0].window), 0);
    msgData = LoadBoxMsgData();
    PrintMsg(app, 0, msgData, syswk->param->mode + 38, 0, 8, app->font, BOX2_COLOR_NORMAL);
    GFL_MsgDataFree(msgData);
    PushWindowScreen(&syswk->app->windows[0]);
}

void Box2Bmp_PrintInfoLabels(Box2SysWork *syswk) {
    Box2AppWork *app = syswk->app;

    GFL_BitmapFill(BmpWin_GetBitmap(app->windows[10].window), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(app->windows[11].window), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(app->windows[12].window), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(app->windows[13].window), 0);
    PrintMsg(app, 10, app->msgData, 109, 0, 0, app->font, BOX2_COLOR_NORMAL);
    PrintMsg(app, 11, app->msgData, 100, 0, 0, app->font, BOX2_COLOR_NORMAL);
    PrintMsg(app, 12, app->msgData, 101, 0, 0, app->font, BOX2_COLOR_NORMAL);
    PrintMsg(app, 13, app->msgData, 102, 0, 0, app->font, BOX2_COLOR_NORMAL);
}

// The species
static void PrintInfoSpecies(Box2AppWork *app, Box2PokeInfo *info, u32 index) {
    GFL_BitmapFill(BmpWin_GetBitmap(app->windows[index].window), 0);
    if (info->egg == FALSE) {
        setBoxPokemonSpeciesNameToStrbuf(app->wordSet, 0, info->pkm);
        PrintFormattedMsg(app, index, app->msgData, 0, 0, 0, app->font, BOX2_COLOR_NORMAL);
    } else {
        RequestFlushChar(app, index);
        return;
    }
    PushWindowScreen(&app->windows[index]);
}

// The nickname
static void PrintInfoNickname(Box2AppWork *app, Box2PokeInfo *info, u32 index) {
    GFL_BitmapFill(BmpWin_GetBitmap(app->windows[index].window), 0);
    loadBoxPokemonNameToStrbuf(app->wordSet, 0, info->pkm);
    PrintFormattedMsg(app, index, app->msgData, 1, 0, 0, app->font, BOX2_COLOR_NORMAL);
    PushWindowScreen(&app->windows[index]);
}

// The level
static void PrintInfoLevel(Box2AppWork *app, Box2PokeInfo *info, u32 index) {
    u32 width;

    GFL_BitmapFill(BmpWin_GetBitmap(app->windows[index].window), 0);
    if (info->egg == FALSE) {
        PrintFormattedMsg(app, index, app->msgData, 107, 0, 0, app->font, BOX2_COLOR_NORMAL);
        width = GFL_FontGetBlockWidth(app->expandBuf, app->font, 0);
        WordSetNumber(app->wordSet, 0, info->level, 3, 0, TRUE);
        PrintFormattedMsg(app, index, app->msgData, 103, width, 0, app->font, BOX2_COLOR_NORMAL);
    } else {
        RequestFlushChar(app, index);
        return;
    }
    PushWindowScreen(&app->windows[index]);
}

// The sex
static void PrintInfoSex(Box2AppWork *app, Box2PokeInfo *info, u32 index) {
    GFL_BitmapFill(BmpWin_GetBitmap(app->windows[index].window), 0);
    if (info->egg == FALSE && info->sexPut == TRUE) {
        if (info->sex == GENDER_MALE) {
            PrintMsg(app, index, app->msgData, 95, 0, 0, app->font, BOX2_COLOR_MALE);
        } else if (info->sex == GENDER_FEMALE) {
            PrintMsg(app, index, app->msgData, 96, 0, 0, app->font, BOX2_COLOR_FEMALE);
        } else {
            RequestFlushChar(app, index);
            return;
        }
    } else {
        RequestFlushChar(app, index);
        return;
    }
    PushWindowScreen(&app->windows[index]);
}

// The Pokédex number, in the national or the regional Pokédex
static void PrintInfoDexNumber(Box2SysWork *syswk, Box2PokeInfo *info, u32 index) {
    u32 width;
    u16 number;

    GFL_BitmapFill(BmpWin_GetBitmap(syswk->app->windows[index].window), 0);
    if (info->egg == FALSE) {
        if (syswk->app->nationalDex == TRUE) {
            number = info->species;
        } else {
            number = syswk->app->regionalDex[info->species];
        }
        if (number != 999) {
            PrintFormattedMsg(syswk->app, index, syswk->app->msgData, 108, 0, 0, syswk->app->font, BOX2_COLOR_NORMAL);
            width = GFL_FontGetBlockWidth(syswk->app->expandBuf, syswk->app->font, 0);
            WordSetNumber(syswk->app->wordSet, 0, number, 3, 2, TRUE);
            PrintFormattedMsg(syswk->app, index, syswk->app->msgData, 104, width, 0, syswk->app->font,
                              BOX2_COLOR_NORMAL);
        } else {
            RequestFlushChar(syswk->app, index);
            return;
        }
    } else {
        RequestFlushChar(syswk->app, index);
        return;
    }
    PushWindowScreen(&syswk->app->windows[index]);
}

// The nature
static void PrintInfoNature(Box2AppWork *app, Box2PokeInfo *info, u32 index) {
    GFL_BitmapFill(BmpWin_GetBitmap(app->windows[index].window), 0);
    if (info->egg == FALSE) {
        loadNatureToStrbuf(app->wordSet, 0, info->nature);
        PrintFormattedMsg(app, index, app->msgData, 98, 0, 0, app->font, BOX2_COLOR_NORMAL);
    } else {
        PrintMsg(app, index, app->msgData, 106, 0, 0, app->font, BOX2_COLOR_NORMAL);
    }
    PushWindowScreen(&app->windows[index]);
}

// The ability
static void PrintInfoAbility(Box2AppWork *app, Box2PokeInfo *info, u32 index) {
    GFL_BitmapFill(BmpWin_GetBitmap(app->windows[index].window), 0);
    if (info->egg == FALSE) {
        loadAbilityNameToStrbuf(app->wordSet, 0, info->ability);
        PrintFormattedMsg(app, index, app->msgData, 97, 0, 0, app->font, BOX2_COLOR_NORMAL);
    } else {
        PrintMsg(app, index, app->msgData, 106, 0, 0, app->font, BOX2_COLOR_NORMAL);
    }
    PushWindowScreen(&app->windows[index]);
}

// The held item
static void PrintInfoItem(Box2AppWork *app, Box2PokeInfo *info, u32 index) {
    GFL_BitmapFill(BmpWin_GetBitmap(app->windows[index].window), 0);
    if (info->item != ITEM_NONE) {
        loadItemNameToStrbuf(app->wordSet, 0, info->item);
        PrintFormattedMsg(app, index, app->msgData, 99, 0, 0, app->font, BOX2_COLOR_NORMAL);
    } else {
        PrintMsg(app, index, app->msgData, 105, 0, 0, app->font, BOX2_COLOR_NORMAL);
    }
    PushWindowScreen(&app->windows[index]);
}

// The moves
static void PrintInfoMoves(Box2AppWork *app, Box2PokeInfo *info, u32 index) {
    MsgData *msgData;
    u32 i;

    GFL_BitmapFill(BmpWin_GetBitmap(app->windows[index].window), 0);
    msgData = GFL_MsgSysLoadData(FALSE, ARCID_SYSTEM_MESSAGE, TEXT_BANK_MOVE_NAMES, HEAPID_TAIL(HEAPID_BOX2_APP));
    if (info->egg == FALSE) {
        for (i = 0; i < 4; i++) {
            PrintMsg(app, index, msgData, info->waza[i], 0, i * 16, app->font, BOX2_COLOR_NORMAL);
        }
    } else {
        PrintMsg(app, index, app->msgData, 106, 0, 0, app->font, BOX2_COLOR_NORMAL);
    }
    GFL_MsgDataFree(msgData);
    PushWindowScreen(&app->windows[index]);
}

void Box2Bmp_PrintPokeInfo(Box2SysWork *syswk, Box2PokeInfo *info) {
    PrintInfoSpecies(syswk->app, info, 1);
    PrintInfoNickname(syswk->app, info, 2);
    PrintInfoLevel(syswk->app, info, 3);
    PrintInfoSex(syswk->app, info, 4);
    PrintInfoDexNumber(syswk, info, 5);
    PrintInfoNature(syswk->app, info, 6);
    PrintInfoAbility(syswk->app, info, 7);
    PrintInfoItem(syswk->app, info, 8);
    PrintInfoMoves(syswk->app, info, 9);
    PushWindowScreen(&syswk->app->windows[10]);
    PushWindowScreen(&syswk->app->windows[11]);
    PushWindowScreen(&syswk->app->windows[12]);
    PushWindowScreen(&syswk->app->windows[13]);
}

void Box2Bmp_ClearPokeInfo(Box2AppWork *app) {
    u32 i;

    for (i = 1; i <= 9; i++) {
        ClearWindowScreen(&app->windows[i]);
        GFL_BitmapFill(BmpWin_GetBitmap(app->windows[i].window), 0);
        RequestFlushChar(app, i);
    }
    ClearWindowScreen(&app->windows[10]);
    ClearWindowScreen(&app->windows[11]);
    ClearWindowScreen(&app->windows[12]);
    ClearWindowScreen(&app->windows[13]);
}

// Draws a button into a window: its frame from the tiles of the box's graphics, an arrow if asked, and the message
// centered on it
static void DrawButton(Box2SysWork *syswk, u32 index, u32 msgId, u16 frame, u16 color, u32 arrow) {
    BmpWin *window = syswk->app->windows[index].window;
    GFLBitmap *bitmap = BmpWin_GetBitmap(window);
    u8 bg = BmpWin_GetBGIndex(window);
    u8 x = BmpWin_GetPosX(window);
    u8 y = BmpWin_GetPosY(window);
    u8 width = BmpWin_GetSizeX(window);
    u8 height = BmpWin_GetSizeY(window);
    NNSG2dCharacterData *chars;
    void *buf;
    GFLBitmap *tile;
    u8 *pixels;
    u8 i;
    StrBuf *str;

    GFL_BitmapFill(bitmap, 4);

    buf = GFL_G2DIOReadBGNCGR(ARCID_BOX2, 62, TRUE, &chars, HEAPID_TAIL(HEAPID_BOX2_APP));
    pixels = chars->pRawData;
    frame *= 0x20;

    tile = GFL_BitmapWrap(&pixels[frame], 8, 8, 0x20, HEAPID_TAIL(HEAPID_BOX2_APP));
    GFL_BitmapCopyArea(tile, bitmap, 0, 0, 0, 0, 8, 8, 0xffff);
    GFL_BitmapFree(tile);

    tile = GFL_BitmapWrap(&pixels[frame + 0x40], 8, 8, 0x20, HEAPID_TAIL(HEAPID_BOX2_APP));
    GFL_BitmapCopyArea(tile, bitmap, 0, 0, (width - 1) * 8, 0, 8, 8, 0xffff);
    GFL_BitmapFree(tile);

    tile = GFL_BitmapWrap(&pixels[frame + 0xc0], 8, 8, 0x20, HEAPID_TAIL(HEAPID_BOX2_APP));
    GFL_BitmapCopyArea(tile, bitmap, 0, 0, 0, (height - 1) * 8, 8, 8, 0xffff);
    GFL_BitmapFree(tile);

    tile = GFL_BitmapWrap(&pixels[frame + 0x100], 8, 8, 0x20, HEAPID_TAIL(HEAPID_BOX2_APP));
    GFL_BitmapCopyArea(tile, bitmap, 0, 0, (width - 1) * 8, (height - 1) * 8, 8, 8, 0xffff);
    GFL_BitmapFree(tile);

    tile = GFL_BitmapWrap(&pixels[frame + 0x60], 8, 8, 0x20, HEAPID_TAIL(HEAPID_BOX2_APP));
    for (i = 1; i < height - 1; i++) {
        GFL_BitmapCopyArea(tile, bitmap, 0, 0, 0, i * 8, 8, 8, 0xffff);
    }
    GFL_BitmapFree(tile);

    tile = GFL_BitmapWrap(&pixels[frame + 0xa0], 8, 8, 0x20, HEAPID_TAIL(HEAPID_BOX2_APP));
    for (i = 1; i < height - 1; i++) {
        GFL_BitmapCopyArea(tile, bitmap, 0, 0, (width - 1) * 8, i * 8, 8, 8, 0xffff);
    }
    GFL_BitmapFree(tile);

    tile = GFL_BitmapWrap(&pixels[frame + 0x20], 8, 8, 0x20, HEAPID_TAIL(HEAPID_BOX2_APP));
    for (i = 1; i < width - 1; i++) {
        GFL_BitmapCopyArea(tile, bitmap, 0, 0, i * 8, 0, 8, 8, 0xffff);
    }
    GFL_BitmapFree(tile);

    tile = GFL_BitmapWrap(&pixels[frame + 0xe0], 8, 8, 0x20, HEAPID_TAIL(HEAPID_BOX2_APP));
    for (i = 1; i < width - 1; i++) {
        GFL_BitmapCopyArea(tile, bitmap, 0, 0, i * 8, (height - 1) * 8, 8, 8, 0xffff);
    }
    GFL_BitmapFree(tile);

    GFL_HeapFree(buf);

    if (arrow == TRUE) {
        buf = GFL_G2DIOReadBGNCGR(getUINarcIdx(), 32, FALSE, &chars, HEAPID_TAIL(HEAPID_BOX2_APP));
        tile = GFL_BitmapWrap((u8 *)chars->pRawData + 0x120, 16, 8, 0x20, HEAPID_TAIL(HEAPID_BOX2_APP));
        GFL_BitmapCopyArea(tile, bitmap, 0, 0, (width - 3) * 8, 8, 16, 8, 5);
        GFL_BitmapFree(tile);
        GFL_HeapFree(buf);
    }

    str = GFL_MsgDataLoadStrbufNew(syswk->app->msgData, msgId);
    PrintStrAligned(&syswk->app->windows[index], syswk->app->printQueue, width * 8 / 2, 4, str, syswk->app->font, color,
                    PRINT_ALIGN_CENTER);
    GFL_StrBufFree(str);
}

static void DrawMenuButton(Box2SysWork *syswk, u32 index, u32 msgId, u32 arrow) {
    DrawButton(syswk, index, msgId, 0, BOX2_COLOR_BUTTON, arrow);
}

static void DrawFrameButton(Box2SysWork *syswk, u32 index, u32 msgId) {
    DrawButton(syswk, index, msgId, 0, BOX2_COLOR_BUTTON, FALSE);
}

// Makes a frame of a window's size on its BG, which shows the window
static void InitWindowFrame(BGWinFrame *frames, u32 index, BmpWin *window) {
    BGWinFrame_InitFrame(frames, index, BmpWin_GetBGIndex(window), BmpWin_GetSizeX(window), BmpWin_GetSizeY(window));
    BGWinFrame_WriteBmpWin(frames, index, window);
}

void Box2Bmp_InitMenuFrames(Box2AppWork *app) {
    InitWindowFrame(app->bgWinFrame, 0, app->windows[14].window);
    InitWindowFrame(app->bgWinFrame, 1, app->windows[15].window);
    InitWindowFrame(app->bgWinFrame, 2, app->windows[16].window);
    InitWindowFrame(app->bgWinFrame, 3, app->windows[17].window);
    InitWindowFrame(app->bgWinFrame, 4, app->windows[18].window);
    InitWindowFrame(app->bgWinFrame, 5, app->windows[19].window);
}

void Box2Bmp_InitPartyButton(Box2SysWork *syswk) {
    if (syswk->param->mode == 4) {
        DrawFrameButton(syswk, 22, 71);
    } else {
        DrawFrameButton(syswk, 22, 69);
    }
    InitWindowFrame(syswk->app->bgWinFrame, 10, syswk->app->windows[22].window);
}

void Box2Bmp_InitBoxListButton(Box2SysWork *syswk) {
    DrawFrameButton(syswk, 23, 70);
    InitWindowFrame(syswk->app->bgWinFrame, 11, syswk->app->windows[23].window);
}

// Draws a box's name, centered, into a text object
static void DrawTrayName(Box2SysWork *syswk, u32 tray, u32 index) {
    StrBuf *str = GFL_StrBufCreate(20, HEAPID_TAIL(HEAPID_BOX2_APP));
    u32 x;

    loadBoxNameToStrbuf(syswk->param->boxes, tray, str);
    GFL_BitmapFill(syswk->app->fontOam[index].bitmap, 0);
    x = (96 - GFL_FontGetBlockWidth(str, syswk->app->font, 0)) >> 1;
    GFL_TextRendererDrawToBitmapEx(syswk->app->fontOam[index].bitmap, x, 0, str, syswk->app->font, BOX2_COLOR_NORMAL);
    GFL_StrBufFree(str);
}

void Box2Bmp_PutTrayName(Box2SysWork *syswk, u32 tray, u32 index) {
    DrawTrayName(syswk, tray, index);
    BmpOam_ActorBmpTrans(syswk->app->fontOam[index].oam);
}

void Box2Bmp_FlushTrayName(Box2SysWork *syswk) {
    if (syswk->app->unkA5B8 == TRUE) {
        BmpOam_ActorBmpTrans(syswk->app->fontOam[0].oam);
        syswk->app->unkA5B8 = FALSE;
    }
}

void Box2Bmp_DrawScrolledTrayName(Box2SysWork *syswk, u32 mv) {
    u32 tray = mv + syswk->trayScroll;

    if (tray >= syswk->trayMax) {
        tray -= syswk->trayMax;
    }
    DrawTrayName(syswk, tray, 0);
    syswk->app->unkA5B8 = TRUE;
}

// Draws how many Pokémon a box holds into a text object
void Box2Bmp_PutTrayCount(Box2SysWork *syswk, u32 tray, u32 index) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(syswk->app->msgData, 30);
    u32 count = howManyPokesInGeneralAreInBox(syswk->param->boxes, tray);

    WordSetNumber(syswk->app->wordSet, 0, count, 2, 0, TRUE);
    GFL_WordSetFormatStrbuf(syswk->app->wordSet, syswk->app->expandBuf, str);
    GFL_BitmapFill(syswk->app->fontOam[index].bitmap, 0);
    if (count == BOX2_TRAY_POKE_MAX) {
        GFL_TextRendererDrawToBitmapEx(syswk->app->fontOam[index].bitmap, 0, 0, syswk->app->expandBuf,
                                       syswk->app->smallFont, BOX2_COLOR_FULL);
    } else {
        GFL_TextRendererDrawToBitmapEx(syswk->app->fontOam[index].bitmap, 0, 0, syswk->app->expandBuf,
                                       syswk->app->smallFont, BOX2_COLOR_NORMAL);
    }
    GFL_StrBufFree(str);
    BmpOam_ActorBmpTrans(syswk->app->fontOam[index].oam);
}

static void InitMarkingButtons(Box2SysWork *syswk) {
    DrawMenuButton(syswk, 20, 84, FALSE);
    DrawMenuButton(syswk, 21, 85, TRUE);
    BGWinFrame_SetPalette(syswk->app->bgWinFrame, 7, 0, 9, 11, 3, 12);
    BGWinFrame_SetPalette(syswk->app->bgWinFrame, 7, 0, 12, 11, 3, 12);
}

void Box2Bmp_InitMarkingFrame(Box2SysWork *syswk) {
    BGWinFrame_WriteBmpWin(syswk->app->bgWinFrame, 7, syswk->app->windows[20].window);
    BGWinFrame_WriteBmpWin(syswk->app->bgWinFrame, 7, syswk->app->windows[21].window);
    InitMarkingButtons(syswk);
}

void Box2Bmp_OpenMenu(Box2SysWork *syswk, const Box2MenuItem *items, u32 count) {
    u16 width, height;
    u32 i = 0;

    BGWinFrame_GetSize(syswk->app->bgWinFrame, 0, &width, &height);
    for (; i < count; i++) {
        const Box2MenuItem *item = &items[count - 1 - i];

        if (item->type == 0) {
            DrawMenuButton(syswk, 19 - i, item->msgId, FALSE);
        } else {
            DrawMenuButton(syswk, 19 - i, item->msgId, TRUE);
        }
        BGWinFrame_SetPalette(syswk->app->bgWinFrame, 5 - i, 0, 0, width, height, 12);
    }
    for (; count < 6; count++) {
        u32 index = 19 - count;

        GFL_BitmapFill(BmpWin_GetBitmap(syswk->app->windows[index].window), 0);
        RequestFlushChar(syswk->app, index);
    }
}

static void DrawMsgWindowFrame(Box2SysWork *syswk, BmpWin *window) {
    GFL_BitmapFill(BmpWin_GetBitmap(window), 15);
    BmpWin_DrawFrame(window, 2, syswk->app->cursorChars, 10);
}

void Box2Bmp_ClearMsgWindow(Box2AppWork *app, u32 index) {
    BmpWin_ClearFrame(app->windows[index].window, 1);
}

void Box2Bmp_ClearMsgWindowNow(Box2AppWork *app, u32 index) {
    BmpWin_ClearFrame(app->windows[index].window, 0);
}

// Prints a message of the box's messages into a message window
static void PrintBoxMsg(Box2SysWork *syswk, u32 msgId, u32 index) {
    MsgData *msgData = LoadBoxMsgData();

    DrawMsgWindowFrame(syswk, syswk->app->windows[index].window);
    PrintFormattedMsg(syswk->app, index, msgData, msgId, 0, 0, syswk->app->font, BOX2_COLOR_MSG);
    PushWindowScreen(&syswk->app->windows[index]);
    GFL_MsgDataFree(msgData);
    Box2Obj_HideButtons(syswk);
}

void Box2Bmp_PrintContinue(Box2SysWork *syswk, u32 index) {
    if (syswk->param->mode == BOX2_MODE_DREAM_WORLD) {
        PrintBoxMsg(syswk, 53, index);
    } else {
        PrintBoxMsg(syswk, 11, index);
    }
}

void Box2Bmp_PrintTakeItem(Box2SysWork *syswk, u32 item, u32 index) {
    loadItemNameToStrbuf(syswk->app->wordSet, 0, item);
    PrintBoxMsg(syswk, 21, index);
}

void Box2Bmp_PrintMailCantTake(Box2SysWork *syswk, u32 index) {
    PrintBoxMsg(syswk, 22, index);
}

void Box2Bmp_PrintMailCantMove(Box2SysWork *syswk, u32 index) {
    PrintBoxMsg(syswk, 44, index);
}

void Box2Bmp_PrintTookItem(Box2SysWork *syswk, u32 item, u32 index) {
    loadItemNameToStrbuf(syswk->app->wordSet, 0, item);
    PrintBoxMsg(syswk, 14, index);
}

void Box2Bmp_PrintBagFull(Box2SysWork *syswk, u32 index) {
    PrintBoxMsg(syswk, 13, index);
}

void Box2Bmp_PrintGiveItem(Box2SysWork *syswk, u32 item, u32 index) {
    if (item == ITEM_NONE) {
        loadItemNameToStrbuf(syswk->app->wordSet, 0, ITEM_GRISEOUS_ORB);
        PrintBoxMsg(syswk, 45, index);
    } else if (PML_ItemIsMail(item) == TRUE) {
        PrintBoxMsg(syswk, 49, index);
    } else {
        loadItemNameToStrbuf(syswk->app->wordSet, 0, item);
        PrintBoxMsg(syswk, 15, index);
    }
}

void Box2Bmp_PrintTouchMark(Box2SysWork *syswk, u32 index) {
    PrintBoxMsg(syswk, 33, index);
}

void Box2Bmp_PrintReleaseMsg(Box2SysWork *syswk, u32 type, u32 index) {
    u32 msgId;

    switch (type) {
    case 0:
        msgId = 2;
        break;
    case 1:
        msgId = 3;
        loadBoxPokemonNameToStrbuf(syswk->app->wordSet, 0, Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos));
        break;
    case 2:
        msgId = 4;
        loadBoxPokemonNameToStrbuf(syswk->app->wordSet, 0, Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos));
        break;
    case 3:
        msgId = 28;
        break;
    case 7:
        msgId = 29;
        break;
    case 4:
        msgId = 30;
        loadBoxPokemonNameToStrbuf(syswk->app->wordSet, 0, Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos));
        break;
    case 5:
        msgId = 31;
        break;
    case 6:
        msgId = 6;
        break;
    }
    PrintBoxMsg(syswk, msgId, index);
}

void Box2Bmp_PrintPrompt(Box2SysWork *syswk, u32 type, u32 index) {
    u32 msgId;

    switch (type) {
    case 0:
        msgId = 7;
        break;
    case 1:
        msgId = 8;
        break;
    case 2:
        msgId = 9;
        break;
    case 3:
        msgId = 10;
        break;
    }
    PrintBoxMsg(syswk, msgId, index);
}

void Box2Bmp_PrintPokeMoveMsg(Box2SysWork *syswk, u32 pos, u32 type, u32 index) {
    u32 msgId;

    switch (type) {
    case 0:
        msgId = 16;
        break;
    case 1:
        msgId = 0;
        loadBoxPokemonNameToStrbuf(syswk->app->wordSet, 0, Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos));
        break;
    case 2:
        msgId = 5;
        break;
    case 3:
        msgId = 17;
        break;
    case 4:
        msgId = 12;
        break;
    case 5:
        msgId = 27;
        break;
    }
    PrintBoxMsg(syswk, msgId, index);
}

void Box2Bmp_PrintItemMsg(Box2SysWork *syswk, u32 type, u32 index) {
    u32 msgId;

    switch (type) {
    case 0:
        msgId = 26;
        break;
    case 1:
        msgId = 23;
        loadItemNameToStrbuf(syswk->app->wordSet, 0, syswk->app->getItem);
        break;
    case 2:
        msgId = 24;
        loadItemNameToStrbuf(syswk->app->wordSet, 0, syswk->app->getItem);
        break;
    case 3:
        msgId = 25;
        loadItemNameToStrbuf(syswk->app->wordSet, 0, syswk->app->getItem);
        break;
    case 4:
        msgId = 22;
        break;
    case 5:
        msgId = 32;
        break;
    case 6:
        msgId = 13;
        break;
    case 7:
        msgId = 48;
        break;
    }
    PrintBoxMsg(syswk, msgId, index);
}

void Box2Bmp_PrintDenyMsg(Box2SysWork *syswk, u32 type, u32 index) {
    u32 msgId;

    switch (type) {
    case 2:
        msgId = 46;
        break;
    case 3:
        msgId = 47;
        break;
    case 4:
        msgId = 50;
        break;
    case 6:
        msgId = 52;
        break;
    }
    PrintBoxMsg(syswk, msgId, index);
}

void Box2Bmp_PrintTuckIn(Box2SysWork *syswk) {
    loadBoxPokemonNameToStrbuf(syswk->app->wordSet, 0, Box2Main_GetBoxPkm(syswk, syswk->tray, syswk->pos));
    PrintBoxMsg(syswk, 51, 27);
}

void Box2Bmp_PrintBattleBoxLocked(Box2SysWork *syswk, u32 a1) {
    if (a1 == 0) {
        PrintBoxMsg(syswk, 54, 27);
    } else {
        PrintBoxMsg(syswk, 55, 27);
    }
}

// Draws how many Pokémon the picked range holds into a text object
void Box2Bmp_PutRangeCount(Box2SysWork *syswk) {
    u16 rowWidth = Box2Main_GetRowWidth(syswk, syswk->pos);
    u16 count = 0;
    u16 x, y;
    StrBuf *str;

    for (y = 0; y < syswk->app->rangeHeight; y++) {
        for (x = 0; x < syswk->app->rangeWidth; x++) {
            if (syswk->app->rangeFlags[y * rowWidth + x] != 0) {
                count++;
            }
        }
    }
    str = GFL_MsgDataLoadStrbufNew(syswk->app->msgData, 30);
    WordSetNumber(syswk->app->wordSet, 0, count, 2, 1, TRUE);
    GFL_WordSetFormatStrbuf(syswk->app->wordSet, syswk->app->expandBuf, str);
    GFL_BitmapFill(syswk->app->fontOam[9].bitmap, 0);
    GFL_TextRendererDrawToBitmapEx(syswk->app->fontOam[9].bitmap, 0, 0, syswk->app->expandBuf, syswk->app->smallFont,
                                   BOX2_COLOR_NORMAL);
    GFL_StrBufFree(str);
    BmpOam_ActorBmpTrans(syswk->app->fontOam[9].oam);
}
