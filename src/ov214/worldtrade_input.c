#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "save/pokedex.h"
#include "worldtrade_local.h"
#include "system/bgwinfrm.h"
#include "system/bmp_menulist.h"

// The Global Trade Station's input of a Pokémon's name, gender and level and of a country, in a window that slides
// over the lower screen, for the deposit and search screens. A name is chosen by its initial's group, then the
// initial, then the name, and a country the same way. The names are ours, guessed

// The steps of the input, by sInputFuncTable
enum {
    WI_SEQ_NONE,
    WI_SEQ_WININ,
    WI_SEQ_WINWAIT,
    WI_SEQ_WINOUT,
    WI_SEQ_WINOFF,
    WI_SEQ_WINOUT_ALL,
    WI_SEQ_HEAD1_INIT,
    WI_SEQ_HEAD1_MAIN,
    WI_SEQ_HEAD1_EXIT,
    WI_SEQ_HEAD1_RETURN,
    WI_SEQ_HEAD2_INIT,
    WI_SEQ_HEAD2_MAIN,
    WI_SEQ_HEAD2_EXIT,
    WI_SEQ_HEAD2_RETURN,
    WI_SEQ_POKENAME_INIT,
    WI_SEQ_POKENAME_MAIN,
    WI_SEQ_POKENAME_CANCEL_EXIT,
    WI_SEQ_POKENAME_EXIT,
    WI_SEQ_NATION_HEAD1_INIT,
    WI_SEQ_NATION_HEAD1_MAIN,
    WI_SEQ_NATION_HEAD1_EXIT,
    WI_SEQ_NATION_HEAD1_RETURN,
    WI_SEQ_NATION_HEAD2_INIT,
    WI_SEQ_NATION_HEAD2_MAIN,
    WI_SEQ_NATION_HEAD2_EXIT,
    WI_SEQ_NATION_HEAD2_RETURN,
    WI_SEQ_NATION_INIT,
    WI_SEQ_NATION_MAIN,
    WI_SEQ_NATION_CANCEL_EXIT,
    WI_SEQ_NATION_EXIT,
    WI_SEQ_SEX_INIT,
    WI_SEQ_SEX_MAIN,
    WI_SEQ_SEX_EXIT,
    WI_SEQ_LEVEL_INIT,
    WI_SEQ_LEVEL_MAIN,
    WI_SEQ_LEVEL_EXIT,
};

// What the select functions return besides a choice
#define INPUT_NULL ((u32) - 1)
#define INPUT_CANCEL ((u32) - 2)

// The Back button of the initial groups
#define INPUT_HEAD_CANCEL 9

// The colors of a choice that can be taken and of one that can't
#define INPUT_COLOR_NORMAL 0x3dc2
#define INPUT_COLOR_GRAY 0x2122

// The BG the window frame is drawn on
#define INPUT_FRAME_BG 2

struct WorldTradeInputWork {
    BmpWin **menuWin;
    BmpWin **backWin;
    ClActor *cursorAct;
    ClActor *arrowAct[2];
    ClActor *searchCursorAct;
    // The window's screen, which slides in and out
    BGWinFrame *bgWinFrm;
    PokeDexSave *zukan;
    MsgData *msgManager;
    MsgData *monsNameManager;
    MsgData *countryNameManager;
    u8 *sinouTable;
    // The Pokémon names, levels or countries listed
    ListMenuOption *nameList;
    WorldTradeNumFont *numFont;
    // The initial's group and the initial chosen, and the results
    s16 head1;
    s16 head2;
    s16 poke;
    s16 nation;
    s8 sex;
    s8 level;
    int listpos;
    int seq;
    int next;
    int bgFrame;
    s16 type;
    s16 page;
    int listMax;
    int situation;
    // Whether any Pokémon of each initial has been seen
    u8 seeCheck[26];
    // The column the cursor left the grid from
    u8 listposBackupX;
    WorldTradePrint print;
};

typedef struct {
    int start;
    int count;
} WorldTradeHeadwordRange;

static void Input_SelectBmpWinAdd(WorldTradeInputWork *wk, int mode);
static void Input_SelectBmpWinDel(WorldTradeInputWork *wk, int mode);
static u32 Input_TouchPanelFunc(WorldTradeInputWork *wk, int mode);
static u32 Input_WordHeadSelectMain(WorldTradeInputWork *wk, u8 *seeCheck);
static u32 Input_Head2DecideFunc(WorldTradeInputWork *wk, int decide);
static u32 Input_WordHead2SelectMain(WorldTradeInputWork *wk, u8 *seeCheck);
static int Input_ListPageNum(int num, int inPage);
static u32 Input_NameDecideFunc(WorldTradeInputWork *wk, int decide);
static void Input_NamePageRefresh(WorldTradeInputWork *wk, int move);
static u32 Input_PokeNameSelectMain(WorldTradeInputWork *wk);
static u32 Input_SexSelectMain(WorldTradeInputWork *wk);
static void Input_LevelPageRefresh(WorldTradeInputWork *wk, int move);
static u32 Input_LevelDecideFunc(WorldTradeInputWork *wk, int decide);
static u32 Input_LevelSelectMain(WorldTradeInputWork *wk);
static u32 Input_NationHead1DecideFunc(WorldTradeInputWork *wk, int decide);
static u32 Input_NationHead1SelectMain(WorldTradeInputWork *wk, u8 *seeCheck);
static u32 Input_NationDecideFunc(WorldTradeInputWork *wk, int decide);
static void Input_NationPageRefresh(WorldTradeInputWork *wk, int move);
static u32 Input_NationSelectMain(WorldTradeInputWork *wk);
static void Input_HeadWord1Init(WorldTradeInputWork *wk, int type, int x);
static void Input_SexSelectInit(WorldTradeInputWork *wk);
static void Input_LevelSelectInit(WorldTradeInputWork *wk);
static void Input_SysPrint(void *frm, BmpWin *win, StrBuf *str, int x, u16 color, WorldTradePrint *print);
static u32 Input_SeqNone(WorldTradeInputWork *wk);
static u32 Input_SeqWinIn(WorldTradeInputWork *wk);
static u32 Input_SeqWinWait(WorldTradeInputWork *wk);
static u32 Input_SeqWinOut(WorldTradeInputWork *wk);
static u32 Input_SeqWinOff(WorldTradeInputWork *wk);
static u32 Input_SeqWinOutAll(WorldTradeInputWork *wk);
static u32 Input_SeqHead1Init(WorldTradeInputWork *wk);
static u32 Input_SeqHead1Main(WorldTradeInputWork *wk);
static u32 Input_SeqHead1Exit(WorldTradeInputWork *wk);
static u32 Input_SeqHead1Return(WorldTradeInputWork *wk);
static u32 Input_SeqHead2Init(WorldTradeInputWork *wk);
static u32 Input_SeqHead2Main(WorldTradeInputWork *wk);
static u32 Input_SeqHead2Exit(WorldTradeInputWork *wk);
static u32 Input_SeqHead2Return(WorldTradeInputWork *wk);
static int Input_PokeNameCountSeen(u8 *sinou, PokeDexSave *zukan, int num, u16 *sortList, int start, int end);
static int Input_PokeNameListMake(ListMenuOption **menulist, MsgData *monsNameManager, MsgData *msgManager, u8 *sinou,
                                  int head, PokeDexSave *zukan);
static void Input_PokeNameListPrint(WorldTradeInputWork *wk, int page, int max);
static void Input_NationListPrint(WorldTradeInputWork *wk, int page, int max);
static u32 Input_SeqPokeNameInit(WorldTradeInputWork *wk);
static u32 Input_SeqPokeNameMain(WorldTradeInputWork *wk);
static u32 Input_SeqPokeNameCancelExit(WorldTradeInputWork *wk);
static u32 Input_SeqPokeNameExit(WorldTradeInputWork *wk);
static u32 Input_SeqNationHead1Init(WorldTradeInputWork *wk);
static u32 Input_SeqNationHead1Main(WorldTradeInputWork *wk);
static u32 Input_SeqNationHead1Exit(WorldTradeInputWork *wk);
static u32 Input_SeqNationHead1Return(WorldTradeInputWork *wk);
static u32 Input_SeqNationHead2Init(WorldTradeInputWork *wk);
static u32 Input_SeqNationHead2Main(WorldTradeInputWork *wk);
static u32 Input_SeqNationHead2Exit(WorldTradeInputWork *wk);
static u32 Input_SeqNationHead2Return(WorldTradeInputWork *wk);
static u32 Input_SeqNationInit(WorldTradeInputWork *wk);
static u32 Input_SeqNationMain(WorldTradeInputWork *wk);
static u32 Input_SeqNationCancelExit(WorldTradeInputWork *wk);
static u32 Input_SeqNationExit(WorldTradeInputWork *wk);
static u32 Input_SeqSexInit(WorldTradeInputWork *wk);
static u32 Input_SeqSexMain(WorldTradeInputWork *wk);
static u32 Input_SeqSexExit(WorldTradeInputWork *wk);
static u32 Input_SeqLevelInit(WorldTradeInputWork *wk);
static u32 Input_SeqLevelMain(WorldTradeInputWork *wk);
static u32 Input_SeqLevelExit(WorldTradeInputWork *wk);
static void Input_SystemPrint(void *frm, MsgData *msgManager, BmpWin *win, int msgNo, int x, u16 color,
                              WorldTradePrint *print);
static void Input_TouchPrint(void *frm, MsgData *msgManager, BmpWin *win, int msgNo, WorldTradePrint *print);
static void Input_PagePrint(void *frm, WorldTradeNumFont *numFont, BmpWin *win, int page, int max);
static int Input_PokeSeeCountSiin(WorldTradeInputWork *wk, int siin);
static BOOL Input_PokeSeeCountCategory(WorldTradeInputWork *wk, int select);
static int Input_NationCountSiin(WorldTradeInputWork *wk, int siin);
static BOOL Input_NationCountCategory(WorldTradeInputWork *wk, int select);

// Where the windows and the cursor's stops of each list are, in characters or pixels
static const u8 sSexWinPos[3][2] = { { 3, 3 }, { 3, 7 }, { 3, 11 } };
static const u8 sLevelWinPos[4][2] = { { 3, 2 }, { 3, 5 }, { 3, 8 }, { 3, 11 } };
static const u8 sSexCursorPos[4][2] = { { 192, 32 }, { 192, 64 }, { 192, 96 }, { 192, 136 } };
static const u8 sLevelCursorPos[5][2] = { { 192, 24 }, { 192, 48 }, { 192, 72 }, { 192, 96 }, { 192, 136 } };
static const u8 sNameCursorPos[5][2] = { { 192, 24 }, { 192, 48 }, { 192, 72 }, { 192, 96 }, { 192, 136 } };
static const u8 sNameWinPos[4][3] = { { 4, 2, 0 }, { 4, 5, 0 }, { 4, 8, 0 }, { 4, 11, 0 } };
static const u8 sNationCursorPos[6][2] = { { 136, 16 }, { 136, 40 },  { 136, 64 },
                                           { 136, 88 }, { 136, 112 }, { 192, 136 } };
static const u8 sNationWinPos[6][2] = { { 3, 1 }, { 3, 4 }, { 3, 7 }, { 3, 10 }, { 3, 13 }, { 24, 17 } };

// The gender each choice searches for: either, male or female, and the Back button
static const int sSexSelectTable[4] = { 2, 0, 1, INPUT_CANCEL };

// Where the cursor goes from each choice of the initials with up, down, left and right
static const u8 sHead2KeyTable[4][4] = { { 3, 3, 2, 1 }, { 3, 3, 0, 2 }, { 3, 3, 1, 0 }, { 2, 2, 3, 3 } };

static const TouchRect sSexTouchTbl[5] = {
    { 24, 44, 136, 248 }, { 56, 76, 136, 248 }, { 88, 108, 136, 248 }, { 133, 154, 197, 250 }, { TOUCH_RECT_END },
};

static const u8 sHead1WinPos[12][2] = {
    { 2, 5 },  { 6, 5 },  { 10, 5 },  { 2, 8 },  { 6, 8 }, { 10, 8 },
    { 2, 11 }, { 6, 11 }, { 10, 11 }, { 9, 17 }, { 2, 2 }, { 1, 2 },
};
static const u8 sHead1CursorPos[12][2] = {
    { 2, 5 },  { 6, 5 },  { 10, 5 },  { 2, 8 },  { 6, 8 }, { 10, 8 },
    { 2, 11 }, { 6, 11 }, { 10, 11 }, { 8, 17 }, { 2, 2 }, { 1, 2 },
};

static const TouchRect sHead2TouchTbl[5] = {
    { 39, 56, 142, 159 }, { 39, 56, 174, 191 }, { 39, 56, 206, 223 }, { 133, 154, 197, 250 }, { TOUCH_RECT_END },
};
static const TouchRect sNameTouchTbl[8] = {
    { 16, 35, 136, 248 },   { 40, 60, 136, 248 },   { 64, 84, 136, 248 },   { 88, 108, 136, 248 },
    { 133, 154, 198, 248 }, { 108, 129, 143, 160 }, { 108, 126, 223, 240 }, { TOUCH_RECT_END },
};

static const u8 sNationHead1CursorPos[11][2] = {
    { 2, 5 }, { 6, 5 }, { 10, 5 }, { 2, 8 }, { 6, 8 }, { 10, 8 }, { 2, 11 }, { 6, 11 }, { 10, 11 }, { 8, 17 }, { 8, 3 },
};

static const TouchRect sNationTouchTbl[9] = {
    { 8, 28, 8, 247 },      { 32, 52, 8, 247 },   { 56, 76, 8, 247 },     { 80, 100, 8, 247 }, { 104, 124, 8, 247 },
    { 133, 154, 195, 248 }, { 124, 145, 77, 96 }, { 124, 145, 173, 190 }, { TOUCH_RECT_END },
};

// Where each group's initials start among the 26
static const int sHeadOffset[9] = { 0, 3, 6, 9, 12, 15, 18, 21, 24 };

static const u8 sHead1KeyTable[10][4] = {
    { 9, 3, 2, 1 }, { 9, 4, 0, 2 }, { 9, 5, 1, 0 }, { 0, 6, 5, 4 }, { 1, 7, 3, 5 },
    { 2, 8, 4, 3 }, { 3, 9, 8, 7 }, { 4, 9, 6, 8 }, { 5, 9, 7, 6 }, { 8, 2, 9, 9 },
};
static const TouchRect sHead1TouchTbl[11] = {
    { 39, 56, 142, 167 },  { 39, 56, 174, 199 },   { 39, 56, 206, 231 },  { 63, 80, 142, 167 },
    { 63, 80, 174, 199 },  { 63, 80, 206, 231 },   { 87, 104, 142, 167 }, { 87, 104, 174, 199 },
    { 87, 104, 206, 231 }, { 133, 154, 197, 250 }, { TOUCH_RECT_END },
};
static const u8 sNationHead1KeyTable[11][4] = {
    { 10, 3, 2, 1 }, { 10, 4, 0, 2 }, { 10, 5, 1, 0 }, { 0, 6, 5, 4 },  { 1, 7, 3, 5 },   { 2, 8, 4, 3 },
    { 3, 9, 8, 7 },  { 4, 9, 6, 8 },  { 5, 9, 7, 6 },  { 8, 10, 9, 9 }, { 9, 0, 10, 10 },
};
static const TouchRect sNationHead1TouchTbl[12] = {
    { 39, 56, 142, 167 },  { 39, 56, 174, 199 },   { 39, 56, 206, 231 },  { 63, 80, 142, 167 },
    { 63, 80, 174, 199 },  { 63, 80, 206, 231 },   { 87, 104, 142, 167 }, { 87, 104, 174, 199 },
    { 87, 104, 206, 231 }, { 133, 154, 197, 250 }, { 16, 35, 136, 248 },  { TOUCH_RECT_END },
};

// The initials of each group, as the first and the count
static const WorldTradeHeadwordRange sHeadwordRange[9] = {
    { 0, 3 }, { 3, 3 }, { 6, 3 }, { 9, 3 }, { 12, 3 }, { 15, 3 }, { 18, 3 }, { 21, 3 }, { 24, 2 },
};

// The message of each initial
static const u32 sHeadwordMsg[27] = {
    0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7,
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 0xe0, 0xe1, 0xe2, 0xe3, 0xf6,
};

static u32 (*sInputFuncTable[])(WorldTradeInputWork *wk) = {
    Input_SeqNone,
    Input_SeqWinIn,
    Input_SeqWinWait,
    Input_SeqWinOut,
    Input_SeqWinOff,
    Input_SeqWinOutAll,
    Input_SeqHead1Init,
    Input_SeqHead1Main,
    Input_SeqHead1Exit,
    Input_SeqHead1Return,
    Input_SeqHead2Init,
    Input_SeqHead2Main,
    Input_SeqHead2Exit,
    Input_SeqHead2Return,
    Input_SeqPokeNameInit,
    Input_SeqPokeNameMain,
    Input_SeqPokeNameCancelExit,
    Input_SeqPokeNameExit,
    Input_SeqNationHead1Init,
    Input_SeqNationHead1Main,
    Input_SeqNationHead1Exit,
    Input_SeqNationHead1Return,
    Input_SeqNationHead2Init,
    Input_SeqNationHead2Main,
    Input_SeqNationHead2Exit,
    Input_SeqNationHead2Return,
    Input_SeqNationInit,
    Input_SeqNationMain,
    Input_SeqNationCancelExit,
    Input_SeqNationExit,
    Input_SeqSexInit,
    Input_SeqSexMain,
    Input_SeqSexExit,
    Input_SeqLevelInit,
    Input_SeqLevelMain,
    Input_SeqLevelExit,
};

static void Input_SelectBmpWinAdd(WorldTradeInputWork *wk, int mode) {
    int i;

    GFL_BGSysFillChar(wk->bgFrame, 0, 1, 0);
    switch (mode) {
    case INPUT_MODE_HEADWORD_1:
        wk->menuWin[14] = BmpWin_CreateDynamic(wk->bgFrame, 9, 17, 6, 2, 1, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[14]), 10);
        for (i = 0; i < 9; i++) {
            wk->menuWin[i] = BmpWin_CreateDynamic(wk->bgFrame, sHead1WinPos[i][0], sHead1WinPos[i][1], 3, 2, 1, 1);
            GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[i]), 10);
        }
        break;
    case INPUT_MODE_HEADWORD_2:
        wk->menuWin[14] = BmpWin_CreateDynamic(wk->bgFrame, 9, 17, 6, 2, 1, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[14]), 10);
        wk->menuWin[0] = BmpWin_CreateDynamic(wk->bgFrame, 2, 2, 3, 2, 1, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[0]), 7);
        for (i = 1; i < 4; i++) {
            wk->menuWin[i] =
                BmpWin_CreateDynamic(wk->bgFrame, sHead1WinPos[i - 1][0], sHead1WinPos[i - 1][1], 2, 2, 1, 1);
            GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[i]), 10);
        }
        break;
    case INPUT_MODE_POKEMON_NAME:
        wk->menuWin[14] = BmpWin_CreateDynamic(wk->bgFrame, 9, 17, 6, 2, 1, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[14]), 10);
        for (i = 0; i < 4; i++) {
            wk->menuWin[i] = BmpWin_CreateDynamic(wk->bgFrame, sNameWinPos[i][0], sNameWinPos[i][1], 8, 2, 1, 1);
            GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[i]), 10);
        }
        wk->menuWin[4] = BmpWin_CreateDynamic(wk->bgFrame, 5, 14, 5, 1, 1, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[4]), 8);
        break;
    case INPUT_MODE_NATION_HEAD1:
        wk->menuWin[14] = BmpWin_CreateDynamic(wk->bgFrame, 9, 17, 6, 2, 1, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[14]), 10);
        for (i = 0; i < 9; i++) {
            wk->menuWin[i] = BmpWin_CreateDynamic(wk->bgFrame, sHead1WinPos[i][0], sHead1WinPos[i][1], 3, 2, 1, 1);
            GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[i]), 10);
        }
        wk->menuWin[15] = BmpWin_CreateDynamic(wk->bgFrame, 1, 2, 9, 2, 1, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[15]), 10);
        break;
    case INPUT_MODE_NATION:
        wk->menuWin[14] = BmpWin_CreateDynamic(wk->bgFrame, 24, 17, 6, 2, 1, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[14]), 10);
        for (i = 0; i < 5; i++) {
            wk->menuWin[i] = BmpWin_CreateDynamic(wk->bgFrame, sNationWinPos[i][0], sNationWinPos[i][1], 23, 2, 1, 1);
            GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[i]), 10);
        }
        wk->menuWin[5] = BmpWin_CreateDynamic(wk->bgFrame, 13, 16, 5, 1, 1, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[5]), 10);
        break;
    case INPUT_MODE_SEX:
        wk->menuWin[14] = BmpWin_CreateDynamic(wk->bgFrame, 9, 17, 6, 2, 1, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[14]), 10);
        for (i = 0; i < 3; i++) {
            wk->menuWin[i] = BmpWin_CreateDynamic(wk->bgFrame, sSexWinPos[i][0], sSexWinPos[i][1], 9, 2, 1, 1);
            GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[i]), 10);
        }
        break;
    case INPUT_MODE_LEVEL:
        wk->menuWin[14] = BmpWin_CreateDynamic(wk->bgFrame, 9, 17, 6, 2, 1, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[14]), 10);
        for (i = 0; i < 4; i++) {
            wk->menuWin[i] = BmpWin_CreateDynamic(wk->bgFrame, sLevelWinPos[i][0], sLevelWinPos[i][1], 11, 2, 1, 1);
            GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[i]), 10);
        }
        wk->menuWin[4] = BmpWin_CreateDynamic(wk->bgFrame, 5, 14, 5, 1, 1, 1);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[4]), 8);
        break;
    }
}

// Clears the windows' characters, and the frame's edge when the window was moved to its place
static inline void Input_ClearFrameEdge(s8 x) {
    if (x == 16) {
        GFL_BGSysFillScrArea(INPUT_FRAME_BG, 5, 16, 1, 1, 15, 16);
        GFL_BGSysFillScrArea(INPUT_FRAME_BG, 6, 17, 1, 15, 15, 16);
        GFL_BGSysLoadScr(INPUT_FRAME_BG);
    }
}

static inline void Input_ClearMenuWin(WorldTradeInputWork *wk, int i) {
    GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[i]), 10);
    BmpWin_FlushChar(wk->menuWin[i]);
    BmpWin_Free(wk->menuWin[i]);
}

static void Input_SelectBmpWinDel(WorldTradeInputWork *wk, int mode) {
    int i;
    s8 x;
    s8 y;

    BGWinFrame_GetPos(wk->bgWinFrm, 0, &x, &y);
    switch (mode) {
    case INPUT_MODE_HEADWORD_1:
        for (i = 0; i < 9; i++) {
            Input_ClearMenuWin(wk, i);
        }
        Input_ClearFrameEdge(x);
        BmpWin_Free(wk->menuWin[14]);
        break;
    case INPUT_MODE_HEADWORD_2:
        for (i = 0; i < 4; i++) {
            Input_ClearMenuWin(wk, i);
        }
        Input_ClearFrameEdge(x);
        BmpWin_Free(wk->menuWin[14]);
        break;
    case INPUT_MODE_POKEMON_NAME:
        for (i = 0; i < 5; i++) {
            Input_ClearMenuWin(wk, i);
        }
        Input_ClearFrameEdge(x);
        BmpWin_Free(wk->menuWin[14]);
        break;
    case INPUT_MODE_NATION_HEAD1:
        for (i = 0; i < 9; i++) {
            Input_ClearMenuWin(wk, i);
        }
        Input_ClearFrameEdge(x);
        BmpWin_Free(wk->menuWin[14]);
        BmpWin_Free(wk->menuWin[15]);
        break;
    case INPUT_MODE_NATION:
        for (i = 0; i < 6; i++) {
            Input_ClearMenuWin(wk, i);
        }
        BmpWin_Free(wk->menuWin[14]);
        break;
    case INPUT_MODE_SEX:
        for (i = 0; i < 3; i++) {
            BmpWin_Free(wk->menuWin[i]);
        }
        BmpWin_Free(wk->menuWin[14]);
        break;
    case INPUT_MODE_LEVEL:
        for (i = 0; i < 5; i++) {
            BmpWin_Free(wk->menuWin[i]);
        }
        BmpWin_Free(wk->menuWin[14]);
        break;
    }
    GFL_BGSysFreeFilledChar(wk->bgFrame, 1, 0);
}

WorldTradeInputWork *WorldTrade_Input_Init(WorldTradeInputHeader *header, int frame, int situation) {
    WorldTradeInputWork *wk =
        GFL_HeapAllocate(HEAPID_WORLDTRADE, sizeof(WorldTradeInputWork), FALSE, "worldtrade_input.c", 914);

    wk->menuWin = header->menuWin;
    wk->backWin = header->backWin;
    wk->cursorAct = header->cursorAct;
    wk->arrowAct[0] = header->arrowAct[0];
    wk->arrowAct[1] = header->arrowAct[1];
    wk->searchCursorAct = header->searchCursorAct;
    wk->msgManager = header->msgManager;
    wk->monsNameManager = header->monsNameManager;
    wk->countryNameManager = header->countryNameManager;
    wk->zukan = header->zukan;
    wk->sinouTable = header->sinouTable;
    wk->bgFrame = frame;
    wk->situation = situation;
    wk->head1 = 0;
    wk->head2 = 0;
    wk->poke = 0;
    wk->nation = 0;
    wk->sex = 0;
    wk->level = 0;
    wk->listpos = 0;
    wk->listposBackupX = 0;
    wk->seq = WI_SEQ_NONE;

    wk->bgWinFrm = BGWinFrame_Create(BGWINFRAME_TRANSFER_VBLANK, 1, HEAPID_WORLDTRADE);
    BGWinFrame_InitFrame(wk->bgWinFrm, 0, 2, 32, 20);
    WorldTrade_CLACT_PosChange(wk->cursorAct, 144, 40);
    func_0204c488(wk->cursorAct, 4);
    func_0204c520(wk->cursorAct, TRUE);
    func_0204c124(wk->cursorAct, FALSE);
    wk->numFont = WorldTrade_NumFontCreate(15, 14, 2, HEAPID_WORLDTRADE);
    WorldTrade_PrintInit(&wk->print, header->config);
    return wk;
}

void WorldTrade_Input_Start(WorldTradeInputWork *wk, int type) {
    wk->type = type;
    switch (type) {
    case INPUT_MODE_POKEMON_NAME:
        wk->listposBackupX = 0;
        wk->listpos = 0;
        wk->head1 = 0;
        wk->head2 = 0;
        wk->seq = WI_SEQ_HEAD1_INIT;
        break;
    case INPUT_MODE_NATION:
        wk->listposBackupX = 0;
        wk->listpos = 0;
        wk->head1 = 0;
        wk->head2 = 0;
        wk->seq = WI_SEQ_NATION_HEAD1_INIT;
        break;
    case INPUT_MODE_SEX:
        wk->listposBackupX = 0;
        wk->listpos = 0;
        wk->seq = WI_SEQ_SEX_INIT;
        break;
    case INPUT_MODE_LEVEL:
        wk->listposBackupX = 0;
        wk->listpos = 0;
        wk->seq = WI_SEQ_LEVEL_INIT;
        break;
    }
}

void WorldTrade_Input_Exit(WorldTradeInputWork *wk) {
    WorldTrade_PrintExit(&wk->print);
    WorldTrade_NumFontDelete(wk->numFont);
    BGWinFrame_Delete(wk->bgWinFrm);
    GFL_HeapFree(wk);
}

u32 WorldTrade_Input_Main(WorldTradeInputWork *wk) {
    u32 ret = sInputFuncTable[wk->seq](wk);

    BGWinFrame_UpdateMoves(wk->bgWinFrm);
    WorldTrade_PrintMain(&wk->print);
    WorldTrade_NumFontMain(wk->numFont);
    return ret;
}

static u32 Input_TouchPanelFunc(WorldTradeInputWork *wk, int mode) {
    switch (mode) {
    case INPUT_MODE_HEADWORD_1:
        return func_0203da0c(sHead1TouchTbl);
    case INPUT_MODE_NATION_HEAD1:
        return func_0203da0c(sNationHead1TouchTbl);
    case INPUT_MODE_POKEMON_NAME:
    case INPUT_MODE_LEVEL:
        return func_0203da0c(sNameTouchTbl);
    case INPUT_MODE_SEX:
        return func_0203da0c(sSexTouchTbl);
    case INPUT_MODE_NATION:
        return func_0203da0c(sNationTouchTbl);
    case INPUT_MODE_HEADWORD_2:
    case INPUT_MODE_NATION_HEAD2:
        return func_0203da0c(sHead2TouchTbl);
    }
}

static u32 Input_WordHeadSelectMain(WorldTradeInputWork *wk, u8 *seeCheck) {
    int pos = wk->listpos;
    u32 ret;
    u32 hit;

    if (pos < INPUT_HEAD_CANCEL) {
        wk->listposBackupX = pos;
    }
    if (GCTX_HIDGetPressedKeys() != 0 && func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, TRUE);
        func_0203d564(FALSE);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return INPUT_NULL;
    }
    if (func_0203d554() == FALSE) {
        if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
            wk->listpos = sHead1KeyTable[wk->listpos][0];
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
            wk->listpos = sHead1KeyTable[wk->listpos][1];
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
            wk->listpos = sHead1KeyTable[wk->listpos][2];
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
            wk->listpos = sHead1KeyTable[wk->listpos][3];
        }
    }
    // Back into the grid from the Back button, at the column it was left from
    if (pos >= INPUT_HEAD_CANCEL && wk->listpos < INPUT_HEAD_CANCEL) {
        if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
            wk->listpos = wk->listposBackupX;
            while (wk->listpos + 3 < INPUT_HEAD_CANCEL) {
                wk->listpos += 3;
            }
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
            wk->listpos = wk->listposBackupX;
            while (wk->listpos - 3 >= 0) {
                wk->listpos -= 3;
            }
        }
    }
    if (pos != wk->listpos) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        WorldTrade_CLACT_PosChange(wk->cursorAct, (sHead1CursorPos[wk->listpos][0] + 16) * 8,
                                   sHead1CursorPos[wk->listpos][1] * 8);
        if (wk->listpos == INPUT_HEAD_CANCEL) {
            func_0204c488(wk->cursorAct, 5);
        } else {
            func_0204c488(wk->cursorAct, 18);
        }
    }

    ret = INPUT_NULL;
    hit = Input_TouchPanelFunc(wk, INPUT_MODE_HEADWORD_1);
    if (hit != (u32)TOUCH_RECT_NONE) {
        WorldTrade_CLACT_PosChange(wk->cursorAct, (sHead1CursorPos[hit][0] + 16) * 8, sHead1CursorPos[hit][1] * 8);
        if (hit == INPUT_HEAD_CANCEL) {
            func_0204c488(wk->cursorAct, 5);
        } else {
            wk->listpos = hit;
            func_0204c488(wk->cursorAct, 18);
        }
        if (hit == INPUT_HEAD_CANCEL) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            ret = INPUT_CANCEL;
        } else if (seeCheck == NULL || seeCheck[hit] != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            ret = hit;
        }
        if (ret == INPUT_NULL && func_0203d554() == FALSE) {
            func_0204c124(wk->cursorAct, FALSE);
        }
        func_0203d564(TRUE);
        return ret;
    }

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        if (wk->listpos == INPUT_HEAD_CANCEL) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            return INPUT_CANCEL;
        }
        GFL_ASSERT(wk->listpos < INPUT_HEAD_CANCEL);
        if (seeCheck == NULL || seeCheck[wk->listpos] != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return wk->listpos;
        }
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        func_0204c488(wk->cursorAct, 5);
        WorldTrade_CLACT_PosChange(wk->cursorAct, 192, 136);
        return INPUT_CANCEL;
    }
    return INPUT_NULL;
}

static u32 Input_Head2DecideFunc(WorldTradeInputWork *wk, int decide) {
    if (decide != 3) {
        if (wk->head1 <= 8) {
            if (decide >= sHeadwordRange[wk->head1].count) {
                decide = INPUT_NULL;
            }
            return decide;
        }
        return decide;
    }
    return INPUT_CANCEL;
}

static u32 Input_WordHead2SelectMain(WorldTradeInputWork *wk, u8 *seeCheck) {
    int pos = wk->listpos;
    // The SDK's u32 is unsigned long, which MWCC indexes with differently than unsigned int
    unsigned long hit;

    if (pos < 3) {
        wk->listposBackupX = pos;
    }
    if (GCTX_HIDGetPressedKeys() != 0 && func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, TRUE);
        func_0203d564(FALSE);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return INPUT_NULL;
    }
    if (func_0203d554() == FALSE) {
        if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
            wk->listpos = sHead2KeyTable[wk->listpos][0];
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
            wk->listpos = sHead2KeyTable[wk->listpos][1];
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
            wk->listpos = sHead2KeyTable[wk->listpos][2];
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
            wk->listpos = sHead2KeyTable[wk->listpos][3];
        }
    }
    if (pos == 3 && wk->listpos < 3) {
        wk->listpos = wk->listposBackupX;
    }
    if (pos != wk->listpos) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        if (wk->listpos == 3) {
            WorldTrade_CLACT_PosChange(wk->cursorAct, 192, 136);
            func_0204c488(wk->cursorAct, 5);
        } else {
            WorldTrade_CLACT_PosChange(wk->cursorAct, (sHead1CursorPos[wk->listpos][0] + 16) * 8,
                                       sHead1CursorPos[wk->listpos][1] * 8);
            func_0204c488(wk->cursorAct, 4);
        }
    }

    hit = Input_TouchPanelFunc(wk, INPUT_MODE_HEADWORD_2);
    if (hit != (u32)TOUCH_RECT_NONE) {
        if (hit == 3) {
            WorldTrade_CLACT_PosChange(wk->cursorAct, 192, 136);
            func_0204c488(wk->cursorAct, 5);
        } else {
            wk->listpos = hit;
            WorldTrade_CLACT_PosChange(wk->cursorAct, (sHead1CursorPos[hit][0] + 16) * 8, sHead1CursorPos[hit][1] * 8);
            func_0204c488(wk->cursorAct, 4);
        }
        hit = Input_Head2DecideFunc(wk, hit);
        if (hit == INPUT_CANCEL) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        } else if (seeCheck == NULL || seeCheck[sHeadOffset[wk->head1] + hit] != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else {
            hit = INPUT_NULL;
            if (func_0203d554() == FALSE) {
                func_0204c124(wk->cursorAct, FALSE);
            }
        }
        func_0203d564(TRUE);
        return hit;
    }

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        hit = Input_Head2DecideFunc(wk, wk->listpos);
        if (hit == INPUT_NULL) {
            return hit;
        }
        if (hit == INPUT_CANCEL) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            return hit;
        }
        if (seeCheck == NULL || seeCheck[sHeadOffset[wk->head1] + hit] != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return hit;
        }
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        func_0204c488(wk->cursorAct, 5);
        WorldTrade_CLACT_PosChange(wk->cursorAct, 192, 136);
        return INPUT_CANCEL;
    }
    return INPUT_NULL;
}

static int Input_ListPageNum(int num, int inPage) {
    if (num == 0) {
        return 1;
    }
    return (num + (inPage - 1)) / inPage;
}

static u32 Input_NameDecideFunc(WorldTradeInputWork *wk, int decide) {
    switch (decide) {
    case 4:
        return INPUT_CANCEL;
    case 5:
        Input_NamePageRefresh(wk, -1);
        return INPUT_NULL;
    case 6:
        Input_NamePageRefresh(wk, 1);
        return INPUT_NULL;
    default: {
        int index = decide + wk->page * 4;

        if (index < wk->listMax) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return wk->nameList[index].value;
        }
        return INPUT_NULL;
    }
    }
}

static void Input_NamePageRefresh(WorldTradeInputWork *wk, int move) {
    int max = Input_ListPageNum(wk->listMax, 4) - 1;

    if (max == 0) {
        return;
    }
    if (move < 0) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_0204c520(wk->arrowAct[1], TRUE);
        func_0204c488(wk->arrowAct[1], 39);
        if (wk->page != 0) {
            wk->page--;
        } else {
            wk->page = max;
        }
    } else {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_0204c520(wk->arrowAct[0], TRUE);
        func_0204c488(wk->arrowAct[0], 38);
        if (wk->page < max) {
            wk->page++;
        } else {
            wk->page = 0;
        }
    }
    Input_PokeNameListPrint(wk, wk->page, wk->listMax);
    Input_PagePrint(wk->bgWinFrm, wk->numFont, wk->menuWin[4], wk->page, Input_ListPageNum(wk->listMax, 4));
}

static u32 Input_PokeNameSelectMain(WorldTradeInputWork *wk) {
    int pos = wk->listpos;
    u32 hit;

    if (GCTX_HIDGetPressedKeys() != 0 && func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, TRUE);
        func_0203d564(FALSE);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return INPUT_NULL;
    }
    if (func_0203d554() == FALSE) {
        if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
            if (wk->listpos != 0) {
                wk->listpos--;
            } else {
                wk->listpos = 4;
            }
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
            if (wk->listpos != 4) {
                wk->listpos++;
            } else {
                wk->listpos = 0;
            }
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
            Input_NamePageRefresh(wk, -1);
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
            Input_NamePageRefresh(wk, 1);
        }
    }
    if (pos != wk->listpos) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        WorldTrade_CLACT_PosChange(wk->cursorAct, sNameCursorPos[wk->listpos][0], sNameCursorPos[wk->listpos][1]);
        if (wk->listpos == 4) {
            func_0204c488(wk->cursorAct, 5);
        } else {
            func_0204c488(wk->cursorAct, 6);
        }
    }

    hit = Input_TouchPanelFunc(wk, INPUT_MODE_POKEMON_NAME);
    if (hit != (u32)TOUCH_RECT_NONE) {
        if (hit < 5) {
            WorldTrade_CLACT_PosChange(wk->cursorAct, sNameCursorPos[hit][0], sNameCursorPos[hit][1]);
            if (hit == 4) {
                func_0204c488(wk->cursorAct, 5);
            } else {
                wk->listpos = hit;
                func_0204c488(wk->cursorAct, 6);
            }
        }
        hit = Input_NameDecideFunc(wk, hit);
        if (hit == INPUT_NULL && func_0203d554() == FALSE) {
            func_0204c124(wk->cursorAct, FALSE);
        }
        func_0203d564(TRUE);
        return hit;
    }

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        return Input_NameDecideFunc(wk, wk->listpos);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        WorldTrade_CLACT_PosChange(wk->cursorAct, 192, 136);
        func_0204c488(wk->cursorAct, 5);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return INPUT_CANCEL;
    }
    return INPUT_NULL;
}

static u32 Input_SexSelectMain(WorldTradeInputWork *wk) {
    int pos = wk->listpos;
    u32 hit;
    u32 ret;

    if (GCTX_HIDGetPressedKeys() != 0 && func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, TRUE);
        func_0203d564(FALSE);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return INPUT_NULL;
    }
    if (func_0203d554() == FALSE) {
        if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
            if (wk->listpos != 0) {
                wk->listpos--;
            } else {
                wk->listpos = 3;
            }
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
            if (wk->listpos != 3) {
                wk->listpos++;
            } else {
                wk->listpos = 0;
            }
        }
    }
    if (pos != wk->listpos) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        WorldTrade_CLACT_PosChange(wk->cursorAct, sSexCursorPos[wk->listpos][0], sSexCursorPos[wk->listpos][1]);
        if (wk->listpos == 3) {
            func_0204c488(wk->cursorAct, 5);
        } else {
            func_0204c488(wk->cursorAct, 6);
        }
    }

    hit = Input_TouchPanelFunc(wk, INPUT_MODE_SEX);
    if (hit != (u32)TOUCH_RECT_NONE) {
        func_0203d564(TRUE);
        WorldTrade_CLACT_PosChange(wk->cursorAct, sSexCursorPos[hit][0], sSexCursorPos[hit][1]);
        if (hit == 3) {
            func_0204c488(wk->cursorAct, 5);
        } else {
            func_0204c488(wk->cursorAct, 6);
        }
        ret = sSexSelectTable[hit];
        if (ret == INPUT_CANCEL) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        } else {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        return ret;
    }

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        if (sSexSelectTable[wk->listpos] == INPUT_CANCEL) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        } else {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        return sSexSelectTable[wk->listpos];
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        WorldTrade_CLACT_PosChange(wk->cursorAct, 192, 136);
        func_0204c488(wk->cursorAct, 5);
        return INPUT_CANCEL;
    }
    return INPUT_NULL;
}

static void Input_LevelPageRefresh(WorldTradeInputWork *wk, int move) {
    if (move < 0) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_0204c520(wk->arrowAct[1], TRUE);
        func_0204c488(wk->arrowAct[1], 39);
        if (wk->page != 0) {
            wk->page--;
        } else {
            wk->page = 2;
        }
    } else {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_0204c520(wk->arrowAct[0], TRUE);
        func_0204c488(wk->arrowAct[0], 38);
        if (wk->page < 2) {
            wk->page++;
        } else {
            wk->page = 0;
        }
    }
    Input_PokeNameListPrint(wk, wk->page, wk->listMax);
    Input_PagePrint(wk->bgWinFrm, wk->numFont, wk->menuWin[4], wk->page, Input_ListPageNum(wk->listMax, 4));
}

static u32 Input_LevelDecideFunc(WorldTradeInputWork *wk, int decide) {
    switch (decide) {
    case 4:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return INPUT_CANCEL;
    case 5:
        Input_LevelPageRefresh(wk, -1);
        return INPUT_NULL;
    case 6:
        Input_LevelPageRefresh(wk, 1);
        return INPUT_NULL;
    default:
        if (decide + wk->page * 4 < wk->listMax) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return wk->nameList[decide].value;
        }
        return INPUT_NULL;
    }
}

static u32 Input_LevelSelectMain(WorldTradeInputWork *wk) {
    int pos = wk->listpos;
    u32 hit;

    if (GCTX_HIDGetPressedKeys() != 0 && func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, TRUE);
        func_0203d564(FALSE);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return INPUT_NULL;
    }
    if (func_0203d554() == FALSE) {
        if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
            if (wk->listpos != 0) {
                wk->listpos--;
            } else {
                wk->listpos = 4;
            }
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
            if (wk->listpos != 4) {
                wk->listpos++;
            } else {
                wk->listpos = 0;
            }
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
            Input_LevelPageRefresh(wk, -1);
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
            Input_LevelPageRefresh(wk, 1);
        }
    }
    if (pos != wk->listpos) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        WorldTrade_CLACT_PosChange(wk->cursorAct, sLevelCursorPos[wk->listpos][0], sLevelCursorPos[wk->listpos][1]);
        if (wk->listpos == 4) {
            func_0204c488(wk->cursorAct, 5);
        } else {
            func_0204c488(wk->cursorAct, 6);
        }
    }

    hit = Input_TouchPanelFunc(wk, INPUT_MODE_LEVEL);
    if (hit != (u32)TOUCH_RECT_NONE) {
        if (hit < 5) {
            WorldTrade_CLACT_PosChange(wk->cursorAct, sLevelCursorPos[hit][0], sLevelCursorPos[hit][1]);
            if (hit == 4) {
                func_0204c488(wk->cursorAct, 5);
            } else {
                wk->listpos = hit;
                func_0204c488(wk->cursorAct, 6);
            }
        }
        hit = Input_LevelDecideFunc(wk, hit);
        if (hit == INPUT_NULL && func_0203d554() == FALSE) {
            func_0204c124(wk->cursorAct, FALSE);
        }
        func_0203d564(TRUE);
        return hit;
    }

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        return Input_LevelDecideFunc(wk, wk->listpos);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        WorldTrade_CLACT_PosChange(wk->cursorAct, 192, 136);
        func_0204c488(wk->cursorAct, 5);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return INPUT_CANCEL;
    }
    return INPUT_NULL;
}

static u32 Input_NationHead1DecideFunc(WorldTradeInputWork *wk, int decide) {
    switch (decide) {
    case 9:
        return INPUT_CANCEL;
    case 10:
        return 10;
    }
    return decide;
}

static u32 Input_NationHead1SelectMain(WorldTradeInputWork *wk, u8 *seeCheck) {
    int pos = wk->listpos;
    u32 hit;

    if (pos < 9) {
        wk->listposBackupX = pos;
    }
    if (GCTX_HIDGetPressedKeys() != 0 && func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, TRUE);
        func_0203d564(FALSE);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return INPUT_NULL;
    }
    if (func_0203d554() == FALSE) {
        if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
            wk->listpos = sNationHead1KeyTable[wk->listpos][0];
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
            wk->listpos = sNationHead1KeyTable[wk->listpos][1];
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
            wk->listpos = sNationHead1KeyTable[wk->listpos][2];
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
            wk->listpos = sNationHead1KeyTable[wk->listpos][3];
        }
    }
    if (pos >= 9 && wk->listpos < 9) {
        if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
            wk->listpos = wk->listposBackupX;
            while (wk->listpos + 3 < 9) {
                wk->listpos += 3;
            }
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
            wk->listpos = wk->listposBackupX;
            while (wk->listpos - 3 >= 0) {
                wk->listpos -= 3;
            }
        }
    }
    if (pos != wk->listpos) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        WorldTrade_CLACT_PosChange(wk->cursorAct, (sNationHead1CursorPos[wk->listpos][0] + 16) * 8,
                                   sNationHead1CursorPos[wk->listpos][1] * 8);
        switch (wk->listpos) {
        case 9:
            func_0204c488(wk->cursorAct, 5);
            break;
        case 10:
            func_0204c488(wk->cursorAct, 6);
            break;
        default:
            func_0204c488(wk->cursorAct, 18);
            break;
        }
    }

    hit = Input_TouchPanelFunc(wk, INPUT_MODE_NATION_HEAD1);
    if (hit != (u32)TOUCH_RECT_NONE) {
        WorldTrade_CLACT_PosChange(wk->cursorAct, (sNationHead1CursorPos[hit][0] + 16) * 8,
                                   sNationHead1CursorPos[hit][1] * 8);
        switch (hit) {
        case 9:
            func_0204c488(wk->cursorAct, 5);
            break;
        case 10:
            func_0204c488(wk->cursorAct, 6);
            break;
        default:
            func_0204c488(wk->cursorAct, 18);
            wk->listpos = hit;
            break;
        }
        hit = Input_NationHead1DecideFunc(wk, hit);
        if (hit == INPUT_CANCEL) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        } else if (hit == 10 || seeCheck == NULL || seeCheck[hit] != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        } else {
            hit = INPUT_NULL;
        }
        if (hit == INPUT_NULL && func_0203d554() == FALSE) {
            func_0204c124(wk->cursorAct, FALSE);
        }
        func_0203d564(TRUE);
        return hit;
    }

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        hit = Input_NationHead1DecideFunc(wk, wk->listpos);
        if (hit == INPUT_CANCEL) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
            return hit;
        }
        if (hit == 10 || seeCheck == NULL || seeCheck[hit] != 0) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return hit;
        }
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        WorldTrade_CLACT_PosChange(wk->cursorAct, 192, 136);
        func_0204c488(wk->cursorAct, 5);
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return INPUT_CANCEL;
    }
    return INPUT_NULL;
}

static u32 Input_NationDecideFunc(WorldTradeInputWork *wk, int decide) {
    switch (decide) {
    case 5:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        return INPUT_CANCEL;
    case 6:
        Input_NationPageRefresh(wk, -1);
        return INPUT_NULL;
    case 7:
        Input_NationPageRefresh(wk, 1);
        return INPUT_NULL;
    default: {
        int index = decide + wk->page * 5;

        if (index < wk->listMax) {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            return wk->nameList[index].value;
        }
        return INPUT_NULL;
    }
    }
}

static void Input_NationPageRefresh(WorldTradeInputWork *wk, int move) {
    int max = Input_ListPageNum(wk->listMax, 5) - 1;

    if (max == 0) {
        return;
    }
    if (move < 0) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_0204c520(wk->arrowAct[1], TRUE);
        func_0204c488(wk->arrowAct[1], 39);
        if (wk->page != 0) {
            wk->page--;
        } else {
            wk->page = max;
        }
    } else {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        func_0204c520(wk->arrowAct[0], TRUE);
        func_0204c488(wk->arrowAct[0], 38);
        if (wk->page < max) {
            wk->page++;
        } else {
            wk->page = 0;
        }
    }
    Input_NationListPrint(wk, wk->page, wk->listMax);
    Input_PagePrint(wk->bgWinFrm, wk->numFont, wk->menuWin[5], wk->page, Input_ListPageNum(wk->listMax, 5));
}

static u32 Input_NationSelectMain(WorldTradeInputWork *wk) {
    int pos = wk->listpos;
    u32 hit;

    if (GCTX_HIDGetPressedKeys() != 0 && func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, TRUE);
        func_0203d564(FALSE);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return INPUT_NULL;
    }
    if (func_0203d554() == FALSE) {
        if (GCTX_HIDGetTypedKeys() & PAD_KEY_UP) {
            if (wk->listpos != 0) {
                wk->listpos--;
            } else {
                wk->listpos = 5;
            }
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_DOWN) {
            if (wk->listpos != 5) {
                wk->listpos++;
            } else {
                wk->listpos = 0;
            }
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_LEFT) {
            Input_NationPageRefresh(wk, -1);
        } else if (GCTX_HIDGetTypedKeys() & PAD_KEY_RIGHT) {
            Input_NationPageRefresh(wk, 1);
        }
    }
    if (pos != wk->listpos) {
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        WorldTrade_CLACT_PosChange(wk->cursorAct, sNationCursorPos[wk->listpos][0], sNationCursorPos[wk->listpos][1]);
        if (wk->listpos == 5) {
            func_0204c488(wk->cursorAct, 5);
        } else {
            func_0204c488(wk->cursorAct, 7);
        }
    }

    hit = Input_TouchPanelFunc(wk, INPUT_MODE_NATION);
    if (hit != (u32)TOUCH_RECT_NONE) {
        if (hit < 6) {
            WorldTrade_CLACT_PosChange(wk->cursorAct, sNationCursorPos[hit][0], sNationCursorPos[hit][1]);
            if (hit == 5) {
                func_0204c488(wk->cursorAct, 5);
            } else {
                wk->listpos = hit;
                func_0204c488(wk->cursorAct, 7);
            }
        }
        hit = Input_NationDecideFunc(wk, hit);
        if (hit == INPUT_NULL && func_0203d554() == FALSE) {
            func_0204c124(wk->cursorAct, FALSE);
        }
        func_0203d564(TRUE);
        return hit;
    }

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        return Input_NationDecideFunc(wk, wk->listpos);
    }
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        WorldTrade_CLACT_PosChange(wk->cursorAct, 192, 136);
        func_0204c488(wk->cursorAct, 5);
        return INPUT_CANCEL;
    }
    return INPUT_NULL;
}

static void Input_HeadWord1Init(WorldTradeInputWork *wk, int type, int x) {
    int i;
    StrBuf *str;
    u16 color;

    BGWinFrame_LoadScreen(wk->bgWinFrm, 0, ARCID_WORLDTRADE, type == INPUT_MODE_NATION_HEAD1 ? 0x1b : 0x1c, TRUE);
    BGWinFrame_GetScreen(wk->bgWinFrm, 0);
    Input_SelectBmpWinAdd(wk, type);
    Input_TouchPrint(wk->bgWinFrm, wk->msgManager, wk->menuWin[14], 0x4a, &wk->print);
    sys_memset(wk->seeCheck, 1, sizeof(wk->seeCheck));
    for (i = 0; i < 9; i++) {
        str = GFL_MsgDataLoadStrbufNew(wk->msgManager, i + 0x74);
        if (type == INPUT_MODE_HEADWORD_1) {
            if (Input_PokeSeeCountCategory(wk, i) == TRUE) {
                color = INPUT_COLOR_NORMAL;
                wk->seeCheck[i] = 1;
            } else {
                color = INPUT_COLOR_GRAY;
                wk->seeCheck[i] = 0;
            }
        } else {
            if (Input_NationCountCategory(wk, i) == TRUE) {
                color = INPUT_COLOR_NORMAL;
                wk->seeCheck[i] = 1;
            } else {
                color = INPUT_COLOR_GRAY;
                wk->seeCheck[i] = 0;
            }
        }
        Input_SysPrint(wk->bgWinFrm, wk->menuWin[i], str, 2, color, &wk->print);
        GFL_StrBufFree(str);
    }
    if (type == INPUT_MODE_NATION_HEAD1) {
        str = GFL_MsgDataLoadStrbufNew(wk->msgManager, 0xbc);
        Input_SysPrint(wk->bgWinFrm, wk->menuWin[15], str, 2, INPUT_COLOR_NORMAL, &wk->print);
        GFL_StrBufFree(str);
    }
    BGWinFrame_Put(wk->bgWinFrm, 0, x, 0);
    BGWinFrame_Show(wk->bgWinFrm, 0);
}

static void Input_SexSelectInit(WorldTradeInputWork *wk) {
    int i;
    StrBuf *str;

    BGWinFrame_LoadScreen(wk->bgWinFrm, 0, ARCID_WORLDTRADE, 0x19, TRUE);
    BGWinFrame_GetScreen(wk->bgWinFrm, 0);
    Input_SelectBmpWinAdd(wk, INPUT_MODE_SEX);
    for (i = 0; i < 3; i++) {
        str = GFL_MsgDataLoadStrbufNew(wk->msgManager, i + 0x82);
        Input_SysPrint(wk->bgWinFrm, wk->menuWin[i], str, 2, INPUT_COLOR_NORMAL, &wk->print);
        GFL_StrBufFree(str);
    }
    Input_TouchPrint(wk->bgWinFrm, wk->msgManager, wk->menuWin[14], 0x4a, &wk->print);
    BGWinFrame_Put(wk->bgWinFrm, 0, 32, 0);
    BGWinFrame_Show(wk->bgWinFrm, 0);
    BGWinFrame_StartMove(wk->bgWinFrm, 0, -4, 0, 4);
}

static void Input_LevelSelectInit(WorldTradeInputWork *wk) {
    BGWinFrame_LoadScreen(wk->bgWinFrm, 0, ARCID_WORLDTRADE, 0x1d, TRUE);
    BGWinFrame_GetScreen(wk->bgWinFrm, 0);
    Input_SelectBmpWinAdd(wk, INPUT_MODE_LEVEL);
    if (wk->situation == INPUT_SITUATION_SEARCH) {
        wk->listMax = WorldTrade_LevelListAdd(&wk->nameList, wk->msgManager, 1);
    } else if (wk->situation == INPUT_SITUATION_DEPOSIT) {
        wk->listMax = WorldTrade_LevelListAdd(&wk->nameList, wk->msgManager, 0);
    }
    Input_PokeNameListPrint(wk, 0, wk->listMax);
    wk->page = 0;
    Input_PagePrint(wk->bgWinFrm, wk->numFont, wk->menuWin[4], wk->page, Input_ListPageNum(wk->listMax, 4));
    Input_TouchPrint(wk->bgWinFrm, wk->msgManager, wk->menuWin[14], 0x4a, &wk->print);
    BGWinFrame_Put(wk->bgWinFrm, 0, 32, 0);
    func_0204c488(wk->cursorAct, 6);
    wk->listposBackupX = 0;
    wk->listpos = 0;
    WorldTrade_CLACT_PosChange(wk->cursorAct, sNameCursorPos[wk->listpos][0], sNameCursorPos[wk->listpos][1]);
    BGWinFrame_Show(wk->bgWinFrm, 0);
    BGWinFrame_StartMove(wk->bgWinFrm, 0, -4, 0, 4);
    wk->seq = WI_SEQ_LEVEL_MAIN;
}

static void Input_SysPrint(void *frm, BmpWin *win, StrBuf *str, int x, u16 color, WorldTradePrint *print) {
    WorldTrade_PrintColor(win, 0, str, x, 0, 0, color, print);
    BmpWin_FlushChar(win);
    BGWinFrame_WriteBmpWin(frm, 0, win);
}

static u32 Input_SeqNone(WorldTradeInputWork *wk) {
    return INPUT_NULL;
}

static u32 Input_SeqWinIn(WorldTradeInputWork *wk) {
    if (BGWinFrame_IsMoving(wk->bgWinFrm, 0) == FALSE) {
        if (func_0203d554() == TRUE) {
            func_0204c124(wk->cursorAct, FALSE);
        } else {
            func_0204c124(wk->cursorAct, TRUE);
        }
        if (wk->type == INPUT_MODE_LEVEL) {
            func_0204c520(wk->arrowAct[0], FALSE);
            func_0204c520(wk->arrowAct[1], FALSE);
            func_0204c488(wk->arrowAct[0], 38);
            func_0204c488(wk->arrowAct[1], 39);
            WorldTrade_CLACT_PosChange(wk->arrowAct[0], 228, 120);
            WorldTrade_CLACT_PosChange(wk->arrowAct[1], 154, 120);
            func_0204c124(wk->arrowAct[0], TRUE);
            func_0204c124(wk->arrowAct[1], TRUE);
        }
        wk->seq = wk->next;
    }
    return INPUT_NULL;
}

static u32 Input_SeqWinWait(WorldTradeInputWork *wk) {
    if (BGWinFrame_IsMoving(wk->bgWinFrm, 0) == FALSE) {
        wk->seq = wk->next;
    }
    return INPUT_NULL;
}

static u32 Input_SeqWinOut(WorldTradeInputWork *wk) {
    if (func_0204c560(wk->cursorAct)) {
        return INPUT_NULL;
    }
    func_0204c124(wk->cursorAct, FALSE);
    func_0204c124(wk->arrowAct[0], FALSE);
    func_0204c124(wk->arrowAct[1], FALSE);
    if (wk->type == INPUT_MODE_NATION && wk->next == WI_SEQ_NATION_CANCEL_EXIT) {
        BGWinFrame_StartMove(wk->bgWinFrm, 0, 6, 0, 6);
    } else {
        BGWinFrame_StartMove(wk->bgWinFrm, 0, 4, 0, 4);
    }
    wk->seq = WI_SEQ_WINOFF;
    return INPUT_NULL;
}

static u32 Input_SeqWinOff(WorldTradeInputWork *wk) {
    if (BGWinFrame_IsMoving(wk->bgWinFrm, 0) == FALSE) {
        BGWinFrame_Hide(wk->bgWinFrm, 0);
        wk->seq = wk->next;
    }
    return INPUT_NULL;
}

static u32 Input_SeqWinOutAll(WorldTradeInputWork *wk) {
    if (func_0204c560(wk->cursorAct)) {
        return INPUT_NULL;
    }
    func_0204c124(wk->cursorAct, FALSE);
    func_0204c124(wk->arrowAct[0], FALSE);
    func_0204c124(wk->arrowAct[1], FALSE);
    BGWinFrame_StartMove(wk->bgWinFrm, 0, 4, 0, 4);
    wk->seq = WI_SEQ_WINWAIT;
    return INPUT_NULL;
}

static u32 Input_SeqHead1Init(WorldTradeInputWork *wk) {
    Input_HeadWord1Init(wk, INPUT_MODE_HEADWORD_1, 32);
    func_0204c488(wk->cursorAct, 18);
    WorldTrade_CLACT_PosChange(wk->cursorAct, 144, 40);
    BGWinFrame_StartMove(wk->bgWinFrm, 0, -4, 0, 4);
    wk->seq = WI_SEQ_WININ;
    wk->next = WI_SEQ_HEAD1_MAIN;
    wk->head1 = -1;
    return INPUT_NULL;
}

static u32 Input_SeqHead1Main(WorldTradeInputWork *wk) {
    u32 ret = Input_WordHeadSelectMain(wk, wk->seeCheck);

    if (ret != INPUT_CANCEL) {
        if (ret <= 8) {
            func_0204c124(wk->cursorAct, TRUE);
            func_0204c488(wk->cursorAct, 19);
            wk->seq = WI_SEQ_HEAD1_EXIT;
            wk->head1 = ret;
        }
    } else {
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        wk->seq = WI_SEQ_WINOUT;
        wk->next = WI_SEQ_HEAD1_EXIT;
        wk->head1 = -1;
    }
    return INPUT_NULL;
}

static u32 Input_SeqHead1Exit(WorldTradeInputWork *wk) {
    if (func_0204c560(wk->cursorAct)) {
        return INPUT_NULL;
    }
    Input_SelectBmpWinDel(wk, INPUT_MODE_HEADWORD_1);
    func_0204c124(wk->cursorAct, FALSE);
    if (wk->head1 >= 0) {
        if (sHeadwordRange[wk->head1].count != 1) {
            wk->seq = WI_SEQ_HEAD2_INIT;
        } else {
            wk->head2 = 0;
            wk->seq = WI_SEQ_POKENAME_INIT;
        }
    } else {
        return INPUT_CANCEL;
    }
    return INPUT_NULL;
}

static u32 Input_SeqHead1Return(WorldTradeInputWork *wk) {
    int i;
    StrBuf *str;
    u16 color;

    BGWinFrame_LoadScreen(wk->bgWinFrm, 0, ARCID_WORLDTRADE, 0x1c, TRUE);
    BGWinFrame_GetScreen(wk->bgWinFrm, 0);
    Input_SelectBmpWinAdd(wk, INPUT_MODE_HEADWORD_1);
    Input_TouchPrint(wk->bgWinFrm, wk->msgManager, wk->menuWin[14], 0x4a, &wk->print);
    sys_memset(wk->seeCheck, 1, sizeof(wk->seeCheck));
    for (i = 0; i < 9; i++) {
        str = GFL_MsgDataLoadStrbufNew(wk->msgManager, i + 0x74);
        if (Input_PokeSeeCountCategory(wk, i) == TRUE) {
            color = INPUT_COLOR_NORMAL;
            wk->seeCheck[i] = 1;
        } else {
            color = INPUT_COLOR_GRAY;
            wk->seeCheck[i] = 0;
        }
        Input_SysPrint(wk->bgWinFrm, wk->menuWin[i], str, 2, color, &wk->print);
        GFL_StrBufFree(str);
    }
    BGWinFrame_Put(wk->bgWinFrm, 0, 16, 0);
    BGWinFrame_Show(wk->bgWinFrm, 0);
    func_0204c488(wk->cursorAct, 18);
    WorldTrade_CLACT_PosChange(wk->cursorAct, (sHead1CursorPos[wk->head1][0] + 16) * 8,
                               sHead1CursorPos[wk->head1][1] * 8);
    wk->listpos = wk->head1;
    func_0204c124(wk->cursorAct, TRUE);
    wk->seq = WI_SEQ_HEAD1_MAIN;
    if (func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, FALSE);
    } else {
        func_0204c124(wk->cursorAct, TRUE);
    }
    return INPUT_NULL;
}

static u32 Input_SeqHead2Init(WorldTradeInputWork *wk) {
    int i;
    u16 color;

    BGWinFrame_LoadScreen(wk->bgWinFrm, 0, ARCID_WORLDTRADE, 0x1a, TRUE);
    BGWinFrame_GetScreen(wk->bgWinFrm, 0);
    Input_SelectBmpWinAdd(wk, INPUT_MODE_HEADWORD_2);
    sys_memset(wk->seeCheck, 1, sizeof(wk->seeCheck));
    Input_SystemPrint(wk->bgWinFrm, wk->msgManager, wk->menuWin[0], wk->head1 + 0x74, 2, INPUT_COLOR_NORMAL,
                      &wk->print);
    for (i = 1; i <= sHeadwordRange[wk->head1].count; i++) {
        if (Input_PokeSeeCountSiin(wk, sHeadOffset[wk->head1] + i - 1) > 0) {
            color = INPUT_COLOR_NORMAL;
            wk->seeCheck[sHeadOffset[wk->head1] + i - 1] = 1;
        } else {
            color = INPUT_COLOR_GRAY;
            wk->seeCheck[sHeadOffset[wk->head1] + i - 1] = 0;
        }
        Input_SystemPrint(wk->bgWinFrm, wk->msgManager, wk->menuWin[i],
                          sHeadwordMsg[sHeadwordRange[wk->head1].start + i - 1], 5, color, &wk->print);
    }
    Input_TouchPrint(wk->bgWinFrm, wk->msgManager, wk->menuWin[14], 0x4a, &wk->print);
    BGWinFrame_Put(wk->bgWinFrm, 0, 16, 0);
    func_0204c488(wk->cursorAct, 4);
    if (wk->head2 < 0) {
        wk->listpos = 0;
    } else {
        wk->listpos = wk->head2;
    }
    WorldTrade_CLACT_PosChange(wk->cursorAct, (sHead1CursorPos[wk->listpos][0] + 16) * 8,
                               sHead1CursorPos[wk->listpos][1] * 8);
    if (func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, FALSE);
    } else {
        func_0204c124(wk->cursorAct, TRUE);
    }
    wk->seq = WI_SEQ_HEAD2_MAIN;
    return INPUT_NULL;
}

static u32 Input_SeqHead2Main(WorldTradeInputWork *wk) {
    u32 ret = Input_WordHead2SelectMain(wk, wk->seeCheck);

    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        wk->seq = WI_SEQ_HEAD2_EXIT;
        wk->head2 = ret;
        break;
    case INPUT_CANCEL:
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        wk->seq = WI_SEQ_HEAD2_EXIT;
        wk->head2 = -1;
        break;
    }
    return INPUT_NULL;
}

static u32 Input_SeqHead2Exit(WorldTradeInputWork *wk) {
    if (func_0204c560(wk->cursorAct)) {
        return INPUT_NULL;
    }
    func_0204c124(wk->cursorAct, FALSE);
    Input_SelectBmpWinDel(wk, INPUT_MODE_HEADWORD_2);
    if (wk->head2 >= 0) {
        wk->seq = WI_SEQ_POKENAME_INIT;
    } else {
        wk->seq = WI_SEQ_HEAD1_RETURN;
    }
    return INPUT_NULL;
}

static u32 Input_SeqHead2Return(WorldTradeInputWork *wk) {
    if (func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, FALSE);
    } else {
        func_0204c124(wk->cursorAct, TRUE);
    }
    wk->seq = WI_SEQ_HEAD2_INIT;
    return INPUT_NULL;
}

static int Input_PokeNameCountSeen(u8 *sinou, PokeDexSave *zukan, int num, u16 *sortList, int start, int end) {
    int i;
    int count = 0;

    for (i = start; i < end; i++) {
        if (PokeDex_IsSeen(zukan, sortList[i])) {
            count++;
        }
    }
    return count;
}

static int Input_PokeNameListMake(ListMenuOption **menulist, MsgData *monsNameManager, MsgData *msgManager, u8 *sinou,
                                  int head, PokeDexSave *zukan) {
    int i;
    int num;
    int start;
    int end;
    int count;
    u16 *sortList = WorldTrade_ZukanSortDataGet(HEAPID_WORLDTRADE, 0, &num);

    WorldTrade_HeadwordRangeGet(head, &start, &end);
    num = end - start;
    count = Input_PokeNameCountSeen(sinou, zukan, num, sortList, start, end);
    *menulist = ListMenuCore_CreateOptionList(count + 1, HEAPID_WORLDTRADE);
    for (i = start; i < end; i++) {
        if (PokeDex_IsSeen(zukan, sortList[i])) {
            ListMenuCore_AppendMsgOption(*menulist, monsNameManager, sortList[i], sortList[i], HEAPID_WORLDTRADE);
        }
    }
    GFL_HeapFree(sortList);
    return count;
}

static void Input_PokeNameListPrint(WorldTradeInputWork *wk, int page, int max) {
    int i;

    for (i = 0; i < 4; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[i]), 10);
        if (i + page * 4 < max) {
            Input_SysPrint(wk->bgWinFrm, wk->menuWin[i], wk->nameList[i + page * 4].text, 0, INPUT_COLOR_NORMAL,
                           &wk->print);
        } else {
            BmpWin_FlushChar(wk->menuWin[i]);
            BGWinFrame_WriteBmpWin(wk->bgWinFrm, 0, wk->menuWin[i]);
        }
    }
}

static void Input_NationListPrint(WorldTradeInputWork *wk, int page, int max) {
    int i;

    for (i = 0; i < 5; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[i]), 10);
        if (i + page * 5 < max) {
            Input_SysPrint(wk->bgWinFrm, wk->menuWin[i], wk->nameList[i + page * 5].text, 0, INPUT_COLOR_NORMAL,
                           &wk->print);
        } else {
            BmpWin_FlushChar(wk->menuWin[i]);
            BGWinFrame_WriteBmpWin(wk->bgWinFrm, 0, wk->menuWin[i]);
        }
    }
}

static u32 Input_SeqPokeNameInit(WorldTradeInputWork *wk) {
    u8 pageMax;

    BGWinFrame_LoadScreen(wk->bgWinFrm, 0, ARCID_WORLDTRADE, 0x1d, TRUE);
    BGWinFrame_GetScreen(wk->bgWinFrm, 0);
    Input_SelectBmpWinAdd(wk, INPUT_MODE_POKEMON_NAME);
    wk->listMax = Input_PokeNameListMake(&wk->nameList, wk->monsNameManager, wk->msgManager, wk->sinouTable,
                                         wk->head2 + sHeadOffset[wk->head1], wk->zukan);
    wk->page = 0;
    Input_PokeNameListPrint(wk, 0, wk->listMax);
    pageMax = Input_ListPageNum(wk->listMax, 4);
    Input_PagePrint(wk->bgWinFrm, wk->numFont, wk->menuWin[4], wk->page, pageMax);
    Input_TouchPrint(wk->bgWinFrm, wk->msgManager, wk->menuWin[14], 0x4a, &wk->print);
    BGWinFrame_Put(wk->bgWinFrm, 0, 16, 0);
    func_0204c488(wk->cursorAct, 6);
    wk->listpos = 0;
    WorldTrade_CLACT_PosChange(wk->cursorAct, sNameCursorPos[wk->listpos][0], sNameCursorPos[wk->listpos][1]);
    WorldTrade_CLACT_PosChange(wk->arrowAct[0], 228, 120);
    WorldTrade_CLACT_PosChange(wk->arrowAct[1], 154, 120);
    if (func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, FALSE);
    } else {
        func_0204c124(wk->cursorAct, TRUE);
    }
    if (pageMax == 1) {
        func_0204c488(wk->arrowAct[0], 61);
        func_0204c488(wk->arrowAct[1], 62);
    } else {
        func_0204c488(wk->arrowAct[0], 38);
        func_0204c488(wk->arrowAct[1], 39);
    }
    func_0204c520(wk->arrowAct[0], FALSE);
    func_0204c520(wk->arrowAct[1], FALSE);
    func_0204c124(wk->arrowAct[0], TRUE);
    func_0204c124(wk->arrowAct[1], TRUE);
    wk->seq = WI_SEQ_POKENAME_MAIN;
    return INPUT_NULL;
}

static u32 Input_SeqPokeNameMain(WorldTradeInputWork *wk) {
    u32 ret = Input_PokeNameSelectMain(wk);

    switch (ret) {
    case INPUT_CANCEL:
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        wk->seq = WI_SEQ_POKENAME_CANCEL_EXIT;
        wk->poke = -1;
        break;
    case INPUT_NULL:
        break;
    default:
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        wk->seq = WI_SEQ_WINOUT;
        wk->next = WI_SEQ_POKENAME_EXIT;
        wk->poke = ret;
        break;
    }
    return INPUT_NULL;
}

static u32 Input_SeqPokeNameCancelExit(WorldTradeInputWork *wk) {
    if (func_0204c560(wk->cursorAct)) {
        return INPUT_NULL;
    }
    func_0204c124(wk->cursorAct, FALSE);
    func_0204c124(wk->arrowAct[0], FALSE);
    func_0204c124(wk->arrowAct[1], FALSE);
    Input_SelectBmpWinDel(wk, INPUT_MODE_POKEMON_NAME);
    ListMenuCore_FreeOptionList(wk->nameList);
    if (sHeadwordRange[wk->head1].count != 1) {
        wk->seq = WI_SEQ_HEAD2_RETURN;
    } else {
        wk->seq = WI_SEQ_HEAD1_RETURN;
    }
    return INPUT_NULL;
}

static u32 Input_SeqPokeNameExit(WorldTradeInputWork *wk) {
    if (func_0204c560(wk->cursorAct)) {
        return INPUT_NULL;
    }
    func_0204c124(wk->cursorAct, FALSE);
    Input_SelectBmpWinDel(wk, INPUT_MODE_POKEMON_NAME);
    ListMenuCore_FreeOptionList(wk->nameList);
    return wk->poke;
}

static u32 Input_SeqNationHead1Init(WorldTradeInputWork *wk) {
    Input_HeadWord1Init(wk, INPUT_MODE_NATION_HEAD1, 32);
    func_0204c488(wk->cursorAct, 18);
    BGWinFrame_StartMove(wk->bgWinFrm, 0, -4, 0, 4);
    wk->seq = WI_SEQ_WININ;
    wk->next = WI_SEQ_NATION_HEAD1_MAIN;
    wk->listpos = 10;
    wk->head1 = -1;
    WorldTrade_CLACT_PosChange(wk->cursorAct, (sNationHead1CursorPos[wk->listpos][0] + 16) * 8,
                               sNationHead1CursorPos[wk->listpos][1] * 8);
    func_0204c488(wk->cursorAct, 6);
    return INPUT_NULL;
}

static u32 Input_SeqNationHead1Main(WorldTradeInputWork *wk) {
    u32 ret = Input_NationHead1SelectMain(wk, wk->seeCheck);

    switch (ret) {
    case INPUT_CANCEL:
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        wk->seq = WI_SEQ_WINOUT;
        wk->next = WI_SEQ_NATION_HEAD1_EXIT;
        wk->head1 = -1;
        break;
    case 10:
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        wk->seq = WI_SEQ_WINOUT;
        wk->next = WI_SEQ_NATION_HEAD1_EXIT;
        wk->head1 = -2;
        break;
    case INPUT_NULL:
        break;
    default:
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, 19);
        wk->head1 = ret;
        wk->seq = WI_SEQ_NATION_HEAD1_EXIT;
        break;
    }
    return INPUT_NULL;
}

static u32 Input_SeqNationHead1Exit(WorldTradeInputWork *wk) {
    if (func_0204c560(wk->cursorAct)) {
        return INPUT_NULL;
    }
    func_0204c124(wk->cursorAct, FALSE);
    Input_SelectBmpWinDel(wk, INPUT_MODE_NATION_HEAD1);
    if (wk->head1 < 0) {
        if (wk->head1 == -1) {
            return INPUT_CANCEL;
        }
        // Any country
        if (wk->head1 == -2) {
            return 0;
        }
    } else if (sHeadwordRange[wk->head1].count != 1) {
        wk->seq = WI_SEQ_NATION_HEAD2_INIT;
    } else {
        // Only one initial: straight to the countries
        wk->head2 = 0;
        BGWinFrame_StartMove(wk->bgWinFrm, 0, -4, 0, 3);
        if (wk->searchCursorAct != NULL) {
            func_0204c124(wk->searchCursorAct, FALSE);
        }
        func_0204c488(wk->cursorAct, 7);
        wk->seq = WI_SEQ_WINWAIT;
        BGWinFrame_LoadScreen(wk->bgWinFrm, 0, ARCID_WORLDTRADE, 0x18, TRUE);
        wk->next = WI_SEQ_NATION_INIT;
    }
    return INPUT_NULL;
}

static u32 Input_SeqNationHead1Return(WorldTradeInputWork *wk) {
    Input_HeadWord1Init(wk, INPUT_MODE_NATION_HEAD1, 16);
    func_0204c488(wk->cursorAct, 18);
    func_0204c124(wk->cursorAct, TRUE);
    wk->listpos = wk->head1;
    WorldTrade_CLACT_PosChange(wk->cursorAct, (sNationHead1CursorPos[wk->listpos][0] + 16) * 8,
                               sNationHead1CursorPos[wk->listpos][1] * 8);
    if (func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, FALSE);
    } else {
        func_0204c124(wk->cursorAct, TRUE);
    }
    wk->seq = WI_SEQ_NATION_HEAD1_MAIN;
    return INPUT_NULL;
}

static u32 Input_SeqNationHead2Init(WorldTradeInputWork *wk) {
    int i;
    u16 color;

    BGWinFrame_LoadScreen(wk->bgWinFrm, 0, ARCID_WORLDTRADE, 0x1a, TRUE);
    BGWinFrame_GetScreen(wk->bgWinFrm, 0);
    Input_SelectBmpWinAdd(wk, INPUT_MODE_HEADWORD_2);
    sys_memset(wk->seeCheck, 1, sizeof(wk->seeCheck));
    Input_SystemPrint(wk->bgWinFrm, wk->msgManager, wk->menuWin[0], wk->head1 + 0x74, 2, INPUT_COLOR_NORMAL,
                      &wk->print);
    for (i = 1; i < 4; i++) {
        if (Input_NationCountSiin(wk, sHeadOffset[wk->head1] + i - 1) > 0) {
            color = INPUT_COLOR_NORMAL;
            wk->seeCheck[sHeadOffset[wk->head1] + i - 1] = 1;
        } else {
            color = INPUT_COLOR_GRAY;
            wk->seeCheck[sHeadOffset[wk->head1] + i - 1] = 0;
        }
        Input_SystemPrint(wk->bgWinFrm, wk->msgManager, wk->menuWin[i],
                          sHeadwordMsg[sHeadwordRange[wk->head1].start + i - 1], 5, color, &wk->print);
    }
    Input_TouchPrint(wk->bgWinFrm, wk->msgManager, wk->menuWin[14], 0x4a, &wk->print);
    BGWinFrame_Put(wk->bgWinFrm, 0, 16, 0);
    func_0204c488(wk->cursorAct, 4);
    if (wk->head2 < 0) {
        wk->listpos = 0;
    } else {
        wk->listpos = wk->head2;
    }
    WorldTrade_CLACT_PosChange(wk->cursorAct, (sHead1CursorPos[wk->listpos][0] + 16) * 8,
                               sHead1CursorPos[wk->listpos][1] * 8);
    if (func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, FALSE);
    } else {
        func_0204c124(wk->cursorAct, TRUE);
    }
    if (wk->searchCursorAct != NULL) {
        func_0204c124(wk->searchCursorAct, TRUE);
    }
    wk->seq = WI_SEQ_NATION_HEAD2_MAIN;
    return INPUT_NULL;
}

static u32 Input_SeqNationHead2Main(WorldTradeInputWork *wk) {
    u32 ret = Input_WordHead2SelectMain(wk, wk->seeCheck);

    switch (ret) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        wk->seq = WI_SEQ_NATION_HEAD2_EXIT;
        wk->head2 = ret;
        break;
    case INPUT_CANCEL:
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        wk->seq = WI_SEQ_NATION_HEAD2_EXIT;
        wk->head2 = -1;
        break;
    }
    return INPUT_NULL;
}

static u32 Input_SeqNationHead2Exit(WorldTradeInputWork *wk) {
    if (func_0204c560(wk->cursorAct)) {
        return INPUT_NULL;
    }
    func_0204c124(wk->cursorAct, FALSE);
    Input_SelectBmpWinDel(wk, INPUT_MODE_HEADWORD_2);
    if (wk->head2 < 0) {
        wk->seq = WI_SEQ_NATION_HEAD1_RETURN;
    } else {
        BGWinFrame_StartMove(wk->bgWinFrm, 0, -4, 0, 3);
        if (wk->searchCursorAct != NULL) {
            func_0204c124(wk->searchCursorAct, FALSE);
        }
        func_0204c488(wk->cursorAct, 7);
        wk->seq = WI_SEQ_WINWAIT;
        BGWinFrame_LoadScreen(wk->bgWinFrm, 0, ARCID_WORLDTRADE, 0x18, TRUE);
        wk->next = WI_SEQ_NATION_INIT;
    }
    return INPUT_NULL;
}

static u32 Input_SeqNationHead2Return(WorldTradeInputWork *wk) {
    wk->seq = WI_SEQ_NATION_HEAD2_INIT;
    wk->listpos = wk->head2;
    if (func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, FALSE);
    } else {
        func_0204c124(wk->cursorAct, TRUE);
    }
    return INPUT_NULL;
}

static u32 Input_SeqNationInit(WorldTradeInputWork *wk) {
    u8 pageMax;

    BGWinFrame_LoadScreen(wk->bgWinFrm, 0, ARCID_WORLDTRADE, 0x18, TRUE);
    Input_SelectBmpWinAdd(wk, INPUT_MODE_NATION);
    wk->listMax =
        WorldTrade_NationSortListMake(&wk->nameList, wk->countryNameManager, wk->head2 + sHeadOffset[wk->head1]);
    Input_NationListPrint(wk, 0, wk->listMax);
    wk->page = 0;
    pageMax = Input_ListPageNum(wk->listMax, 5);
    Input_PagePrint(wk->bgWinFrm, wk->numFont, wk->menuWin[5], wk->page, pageMax);
    Input_TouchPrint(wk->bgWinFrm, wk->msgManager, wk->menuWin[14], 0x4a, &wk->print);
    BGWinFrame_Put(wk->bgWinFrm, 0, 1, 0);
    wk->listpos = 0;
    WorldTrade_CLACT_PosChange(wk->cursorAct, sNationCursorPos[wk->listpos][0], sNationCursorPos[wk->listpos][1]);
    WorldTrade_CLACT_PosChange(wk->arrowAct[0], 176, 136);
    WorldTrade_CLACT_PosChange(wk->arrowAct[1], 88, 136);
    if (func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, FALSE);
    } else {
        func_0204c124(wk->cursorAct, TRUE);
    }
    if (pageMax == 1) {
        func_0204c488(wk->arrowAct[0], 61);
        func_0204c488(wk->arrowAct[1], 62);
    } else {
        func_0204c488(wk->arrowAct[0], 38);
        func_0204c488(wk->arrowAct[1], 39);
    }
    func_0204c520(wk->arrowAct[0], FALSE);
    func_0204c520(wk->arrowAct[1], FALSE);
    func_0204c124(wk->arrowAct[0], TRUE);
    func_0204c124(wk->arrowAct[1], TRUE);
    wk->seq = WI_SEQ_NATION_MAIN;
    return INPUT_NULL;
}

static u32 Input_SeqNationMain(WorldTradeInputWork *wk) {
    u32 ret = Input_NationSelectMain(wk);

    switch (ret) {
    case INPUT_CANCEL:
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        BGWinFrame_LoadScreen(wk->bgWinFrm, 0, ARCID_WORLDTRADE, 0x18, TRUE);
        wk->seq = WI_SEQ_WINOUT_ALL;
        wk->next = WI_SEQ_NATION_CANCEL_EXIT;
        break;
    case INPUT_NULL:
        break;
    default:
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        wk->seq = WI_SEQ_WINOUT;
        wk->next = WI_SEQ_NATION_EXIT;
        wk->nation = ret;
        break;
    }
    return INPUT_NULL;
}

static u32 Input_SeqNationCancelExit(WorldTradeInputWork *wk) {
    if (func_0204c560(wk->cursorAct)) {
        return INPUT_NULL;
    }
    func_0204c124(wk->cursorAct, FALSE);
    func_0204c124(wk->arrowAct[0], FALSE);
    func_0204c124(wk->arrowAct[1], FALSE);
    Input_SelectBmpWinDel(wk, INPUT_MODE_NATION);
    ListMenuCore_FreeOptionList(wk->nameList);
    if (sHeadwordRange[wk->head1].count != 1) {
        wk->seq = WI_SEQ_NATION_HEAD2_RETURN;
    } else {
        wk->seq = WI_SEQ_NATION_HEAD1_RETURN;
    }
    return INPUT_NULL;
}

static u32 Input_SeqNationExit(WorldTradeInputWork *wk) {
    if (func_0204c560(wk->cursorAct)) {
        return INPUT_NULL;
    }
    func_0204c124(wk->cursorAct, FALSE);
    func_0204c124(wk->arrowAct[0], FALSE);
    func_0204c124(wk->arrowAct[1], FALSE);
    Input_SelectBmpWinDel(wk, INPUT_MODE_NATION);
    ListMenuCore_FreeOptionList(wk->nameList);
    return wk->nation;
}

static u32 Input_SeqSexInit(WorldTradeInputWork *wk) {
    wk->listpos = 0;
    Input_SexSelectInit(wk);
    func_0204c488(wk->cursorAct, 6);
    WorldTrade_CLACT_PosChange(wk->cursorAct, sSexCursorPos[wk->listpos][0], sSexCursorPos[wk->listpos][1]);
    wk->seq = WI_SEQ_WININ;
    wk->next = WI_SEQ_SEX_MAIN;
    return INPUT_NULL;
}

static u32 Input_SeqSexMain(WorldTradeInputWork *wk) {
    u32 ret = Input_SexSelectMain(wk);

    switch (ret) {
    case INPUT_CANCEL:
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        wk->seq = WI_SEQ_WINOUT;
        wk->next = WI_SEQ_SEX_EXIT;
        wk->sex = INPUT_CANCEL;
        break;
    case INPUT_NULL:
        break;
    default:
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        wk->seq = WI_SEQ_WINOUT;
        wk->next = WI_SEQ_SEX_EXIT;
        wk->sex = ret;
        break;
    }
    return INPUT_NULL;
}

static u32 Input_SeqSexExit(WorldTradeInputWork *wk) {
    if (func_0204c560(wk->cursorAct)) {
        return INPUT_NULL;
    }
    func_0204c124(wk->cursorAct, FALSE);
    Input_SelectBmpWinDel(wk, INPUT_MODE_SEX);
    return wk->sex;
}

static u32 Input_SeqLevelInit(WorldTradeInputWork *wk) {
    wk->listpos = 0;
    Input_LevelSelectInit(wk);
    func_0204c488(wk->cursorAct, 6);
    WorldTrade_CLACT_PosChange(wk->cursorAct, sLevelCursorPos[wk->listpos][0], sLevelCursorPos[wk->listpos][1]);
    wk->seq = WI_SEQ_WININ;
    wk->next = WI_SEQ_LEVEL_MAIN;
    return INPUT_NULL;
}

static u32 Input_SeqLevelMain(WorldTradeInputWork *wk) {
    u32 ret = Input_LevelSelectMain(wk);

    switch (ret) {
    case INPUT_CANCEL:
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        wk->seq = WI_SEQ_WINOUT;
        wk->next = WI_SEQ_LEVEL_EXIT;
        wk->level = INPUT_CANCEL;
        break;
    case INPUT_NULL:
        break;
    default:
        func_0204c124(wk->cursorAct, TRUE);
        func_0204c488(wk->cursorAct, func_0204c4a0(wk->cursorAct) + 9);
        wk->seq = WI_SEQ_WINOUT;
        wk->next = WI_SEQ_LEVEL_EXIT;
        wk->level = wk->nameList[ret + wk->page * 4].value;
        break;
    }
    return INPUT_NULL;
}

static u32 Input_SeqLevelExit(WorldTradeInputWork *wk) {
    if (func_0204c560(wk->cursorAct)) {
        return INPUT_NULL;
    }
    func_0204c124(wk->cursorAct, FALSE);
    Input_SelectBmpWinDel(wk, INPUT_MODE_LEVEL);
    ListMenuCore_FreeOptionList(wk->nameList);
    return wk->level;
}

static void Input_SystemPrint(void *frm, MsgData *msgManager, BmpWin *win, int msgNo, int x, u16 color,
                              WorldTradePrint *print) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgManager, msgNo);

    Input_SysPrint(frm, win, str, x, color, print);
    GFL_StrBufFree(str);
}

static void Input_TouchPrint(void *frm, MsgData *msgManager, BmpWin *win, int msgNo, WorldTradePrint *print) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgManager, msgNo);

    WorldTrade_PrintColor(win, 0, str, 0, 0, 0, INPUT_COLOR_NORMAL, print);
    BmpWin_FlushChar(win);
    BGWinFrame_WriteBmpWin(frm, 0, win);
    GFL_StrBufFree(str);
}

static void Input_PagePrint(void *frm, WorldTradeNumFont *numFont, BmpWin *win, int page, int max) {
    GFL_BitmapFill(BmpWin_GetBitmap(win), 8);
    WorldTrade_NumFontPrintNumber(numFont, page + 1, 2, 1, win, 0, 0);
    WorldTrade_NumFontPrintSlash(numFont, 0, win, 16, 0);
    WorldTrade_NumFontPrintNumber(numFont, max, 2, 0, win, 24, 0);
    BmpWin_FlushChar(win);
    BGWinFrame_WriteBmpWin(frm, 0, win);
}

static int Input_PokeSeeCountSiin(WorldTradeInputWork *wk, int siin) {
    int num;
    int start;
    int end;
    int count;
    u16 *sortList = WorldTrade_ZukanSortDataGet(HEAPID_WORLDTRADE, 0, &num);

    WorldTrade_HeadwordRangeGet(siin, &start, &end);
    num = end - start;
    count = Input_PokeNameCountSeen(wk->sinouTable, wk->zukan, num, sortList, start, end);
    GFL_HeapFree(sortList);
    return count;
}

static BOOL Input_PokeSeeCountCategory(WorldTradeInputWork *wk, int select) {
    int start = sHeadOffset[select];
    int i;

    if (sHeadwordRange[select].count > 0) {
        for (i = 0; i < sHeadwordRange[select].count; i++) {
            if (Input_PokeSeeCountSiin(wk, start + i) > 0) {
                return TRUE;
            }
        }
    } else if (Input_PokeSeeCountSiin(wk, start) > 0) {
        return TRUE;
    }
    return FALSE;
}

static int Input_NationCountSiin(WorldTradeInputWork *wk, int siin) {
    int number;

    return WorldTrade_NationSortListNumGet(siin, &number);
}

static BOOL Input_NationCountCategory(WorldTradeInputWork *wk, int select) {
    int start = sHeadOffset[select];
    int i;

    if (sHeadwordRange[select].count > 0) {
        for (i = 0; i < sHeadwordRange[select].count; i++) {
            if (Input_NationCountSiin(wk, start + i) > 0) {
                return TRUE;
            }
        }
    } else if (Input_NationCountSiin(wk, start) > 0) {
        return TRUE;
    }
    return FALSE;
}
