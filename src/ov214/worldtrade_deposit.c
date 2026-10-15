#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/sound.h"
#include "constants/species.h"
#include "constants/version.h"
#include "dpw/dpw_tr.h"
#include "field/unity_tower.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
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
#include "pml/personal.h"
#include "pml/poke_party.h"
#include "save/player_info.h"
#include "save/pokedex.h"
#include "system/bmp_menulist.h"
#include "system/bmp_winframe.h"
#include "system/str_tool.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "worldtrade_local.h"

// The Global Trade Station's deposit screen, where the player says which Pokémon they want for the one they deposit,
// and the printing of Pokémon names, genders, levels and countries that the other screens share. The names are ours,
// guessed

// The sub-process modes the screen leaves with
#define DEPOSIT_MODE_TO_BOX 5
#define DEPOSIT_MODE_UPLOAD 7

// The number of species and one, the size of the regional Pokédex's table
#define MONSNO_TABLE_SIZE 650

// The initials of the Pokémon names
#define HEADWORD_NUM 10

#define LEVEL_SELECT_NUM 12

enum {
    DEPOSIT_SEQ_START,
    DEPOSIT_SEQ_MAIN,
    DEPOSIT_SEQ_END,
    DEPOSIT_SEQ_HEADWORD_SELECT_LIST,
    DEPOSIT_SEQ_HEADWORD_SELECT_WAIT,
    DEPOSIT_SEQ_POKENAME_SELECT_LIST,
    DEPOSIT_SEQ_POKENAME_SELECT_WAIT,
    DEPOSIT_SEQ_SEX_SELECT_MES,
    DEPOSIT_SEQ_SEX_SELECT_LIST,
    DEPOSIT_SEQ_SEX_SELECT_WAIT,
    DEPOSIT_SEQ_LEVEL_SELECT_MES,
    DEPOSIT_SEQ_LEVEL_SELECT_LIST,
    DEPOSIT_SEQ_LEVEL_SELECT_WAIT,
    DEPOSIT_SEQ_DEPOSITOK_MESSAGE,
    DEPOSIT_SEQ_DEPOSIT_YESNO,
    DEPOSIT_SEQ_DEPOSIT_YESNO_WAIT,
    DEPOSIT_SEQ_MES_WAIT,
};

// A level condition and its text
typedef struct {
    u32 msg;
    s16 min;
    s16 max;
} WorldTradeLevelTerm;

// Where the countries of each initial start in the country list
typedef struct {
    u8 start;
    u8 head;
} WorldTradeNationHead;

// An option of a selection list and its value
typedef struct {
    u32 msg;
    s32 value;
} WorldTradeSelectItem;

static void Deposit_SubSeqMessagePrint(WorldTradeWork *wk, int msgNo, int wait, int flag, u16 dat);
static void Deposit_BgInit(void);
static void Deposit_BgExit(void);
static void Deposit_BgGraphicSet(WorldTradeWork *wk);
static void Deposit_BmpWinInit(WorldTradeWork *wk);
static void Deposit_BmpWinDelete(WorldTradeWork *wk);
static void Deposit_SetCellActor(WorldTradeWork *wk);
static void Deposit_DelCellActor(WorldTradeWork *wk);
static void Deposit_InitWork(WorldTradeWork *wk);
static void Deposit_FreeWork(WorldTradeWork *wk);
static int Deposit_SubSeqStart(WorldTradeWork *wk);
static int Deposit_SubSeqMain(WorldTradeWork *wk);
static int Deposit_SubSeqHeadwordSelectList(WorldTradeWork *wk);
static int Deposit_SubSeqHeadwordSelectWait(WorldTradeWork *wk);
static int Deposit_SubSeqPokeNameSelectList(WorldTradeWork *wk);
static int Deposit_SubSeqPokeNameSelectWait(WorldTradeWork *wk);
static int Deposit_SubSeqSexSelectMessage(WorldTradeWork *wk);
static int Deposit_SubSeqSexSelectList(WorldTradeWork *wk);
static int Deposit_SubSeqSexSelectWait(WorldTradeWork *wk);
static int Deposit_SubSeqLevelSelectMessage(WorldTradeWork *wk);
static int Deposit_SubSeqLevelSelectList(WorldTradeWork *wk);
static int Deposit_SubSeqLevelSelectWait(WorldTradeWork *wk);
static int Deposit_SubSeqDepositOkMessage(WorldTradeWork *wk);
static int Deposit_SubSeqDepositOkYesNo(WorldTradeWork *wk);
static int Deposit_SubSeqDepositOkYesNoWait(WorldTradeWork *wk);
static int Deposit_SubSeqEnd(WorldTradeWork *wk);
static int Deposit_SubSeqMessageWait(WorldTradeWork *wk);
static u16 Deposit_GetSexBaseColor(int sex);
static u16 Deposit_GetSexColor(int sex, u16 color);
static void Deposit_DepositPokemonDataMake(Dpw_Tr_Data *dtd, WorldTradeWork *wk);
static int Deposit_PokeNameSortListMake(ListMenuOption **menulist, MsgData *monsNameManager, MsgData *msgManager,
                                        u16 *table, u8 *sinou, int num, int select, PokeDexSave *zukan);
static u32 Deposit_BmpListMain(BmpMenuList *list, u16 *posBackup);

static u16 sNameHeadTable[HEADWORD_NUM + 1] = {
    0, 112, 184, 253, 315, 392, 463, 595, 637, 649,
};

// Not referred to
WorldTradeSelectItem WorldTrade_SexSelectTable[] = {
    { 0x82, 2 },
    { 0x83, 0 },
    { 0x84, 1 },
    { 0x85, BMPMENULIST_CANCEL },
};

static int (*sDepositSubSeqTable[])(WorldTradeWork *wk) = {
    Deposit_SubSeqStart,
    Deposit_SubSeqMain,
    Deposit_SubSeqEnd,
    Deposit_SubSeqHeadwordSelectList,
    Deposit_SubSeqHeadwordSelectWait,
    Deposit_SubSeqPokeNameSelectList,
    Deposit_SubSeqPokeNameSelectWait,
    Deposit_SubSeqSexSelectMessage,
    Deposit_SubSeqSexSelectList,
    Deposit_SubSeqSexSelectWait,
    Deposit_SubSeqLevelSelectMessage,
    Deposit_SubSeqLevelSelectList,
    Deposit_SubSeqLevelSelectWait,
    Deposit_SubSeqDepositOkMessage,
    Deposit_SubSeqDepositOkYesNo,
    Deposit_SubSeqDepositOkYesNoWait,
    Deposit_SubSeqMessageWait,
};

// The countries in the order of their names
static const u16 sCountryListTbl[] = {
    1,   2,   3,   6,   8,   9,   12,  13,  15,  16,  17,  18,  20,  21,  22,  23,  25,  27,  28,  29,  31,  33,
    34,  35,  36,  40,  42,  43,  45,  47,  48,  49,  51,  53,  54,  58,  60,  61,  62,  63,  64,  71,  72,  73,
    74,  76,  79,  80,  81,  82,  83,  84,  85,  87,  88,  90,  91,  92,  93,  94,  95,  96,  98,  99,  101, 102,
    103, 105, 106, 109, 111, 115, 117, 118, 121, 125, 128, 130, 132, 134, 138, 139, 141, 145, 147, 148, 149, 150,
    151, 155, 156, 157, 160, 161, 163, 164, 166, 167, 173, 174, 181, 185, 186, 188, 189, 190, 191, 194, 170, 195,
    196, 198, 199, 200, 201, 203, 219, 205, 206, 210, 211, 215, 217, 218, 220, 221, 222, 224, 226, 227,
};

const u32 WorldTrade_CountryListNum = NELEMS(sCountryListTbl);

// The texts of the genders, by a search's gender
const u32 WorldTrade_SexStringTable[] = { 0x82, 0x83, 0x84, 0x82 };

// Where the windows of the wanted and the deposited Pokémon are
static const u16 sInfoBmpTable[6][2] = {
    { 0, 3 }, { 1, 5 }, { 2, 7 }, { 0, 11 }, { 1, 13 }, { 2, 15 },
};

static const BmpMenuListHeader sPokeNameListHeader = {
    NULL, NULL, NULL, 10, 6, 4, 8, 0, 2, 1, 15, 2, 0, 0, 1, 0, 0, NULL, 12, 12, 0, NULL, NULL, NULL, 20,
};

static const WorldTradeNationHead sNationHeadTable[28] = {
    { 0, 0 },    { 8, 1 },    { 22, 2 },   { 35, 3 },   { 38, 4 },   { 41, 5 },   { 45, 6 },
    { 56, 7 },   { 59, 8 },   { 66, 9 },   { 69, 10 },  { 71, 11 },  { 75, 12 },  { 83, 13 },
    { 90, 14 },  { 91, 15 },  { 98, 16 },  { 98, 17 },  { 100, 18 }, { 115, 19 }, { 121, 20 },
    { 127, 21 }, { 130, 22 }, { 130, 23 }, { 130, 24 }, { 130, 25 }, { 130, 26 }, { 130, 0xff },
};

int WorldTrade_Deposit_Init(WorldTradeWork *wk, int seq) {
    WorldTradeInputHeader header;

    Deposit_InitWork(wk);
    Deposit_BgInit();
    WorldTrade_SubLcdBgInit(wk, 0, 0);
    Deposit_BgGraphicSet(wk);
    Deposit_BmpWinInit(wk);
    Deposit_SetCellActor(wk);
    GFL_WipeSet(3, 1, 1, 0, 6, 1, HEAPID_WORLDTRADE);
    WorldTrade_WifiIconAdd(wk);

    WorldTrade_PokeWantPrint(wk->msgManager, wk->monsNameManager, wk->wordSet, &wk->infoWin[0], 0, SEARCH_GENDER_ANY,
                             -1, &wk->print);
    WorldTrade_PokeInfoPrint(wk->msgManager, wk->wordSet, &wk->infoWin[3], wk->depositPkm, &wk->post, &wk->print);

    header.menuWin = wk->menuWin;
    header.backWin = &wk->backWin;
    header.cursorAct = wk->subCursorAct;
    header.arrowAct[0] = wk->boxArrowAct[0];
    header.arrowAct[1] = wk->boxArrowAct[1];
    header.searchCursorAct = NULL;
    header.msgManager = wk->msgManager;
    header.monsNameManager = wk->monsNameManager;
    header.countryNameManager = wk->countryNameManager;
    header.zukan = wk->param->pokedex;
    header.sinouTable = wk->dw->sinouTable;
    header.config = wk->param->config;
    wk->inputWork = WorldTrade_Input_Init(&header, 2, 0);

    wk->subprocessSeq = DEPOSIT_SEQ_START;
    wk->subLcdBgKeep = 0;
    return WT_SEQ_FADEIN;
}

int WorldTrade_Deposit_Main(WorldTradeWork *wk, int seq) {
    return sDepositSubSeqTable[wk->subprocessSeq](wk);
}

int WorldTrade_Deposit_End(WorldTradeWork *wk, int seq) {
    Deposit_DelCellActor(wk);
    WorldTrade_Input_Exit(wk->inputWork);
    Deposit_FreeWork(wk);
    Deposit_BmpWinDelete(wk);
    Deposit_BgExit();
    WorldTrade_SubLcdBgExit(wk);
    func_0204c124(wk->promptDsAct, FALSE);
    WorldTrade_SubProcessUpdate(wk);
    return WT_SEQ_INIT;
}

static void Deposit_SubSeqMessagePrint(WorldTradeWork *wk, int msgNo, int wait, int flag, u16 dat) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgManager, msgNo);
    BmpWin *win;

    GFL_WordSetFormatStrbuf(wk->wordSet, wk->talkString, str);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWin), 15);
    win = wk->msgWin;
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
    BmpWin_DrawFrame(wk->msgWin, 0, 1, 14);
    WorldTrade_Print(wk->msgWin, 0, wk->talkString, 0, 0, &wk->print);
    GFL_StrBufFree(str);
}

static void Deposit_BgInit(void) {
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf800),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(0, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(0);
        GFL_BGSysSetBGEnabled(0, TRUE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xf000),
            GX_BG_CHARBASE(0x08000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(1, TRUE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe800),
            GX_BG_CHARBASE(0x04000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
        GFL_BGSysFillScrArea(2, 0, 0, 0, 32, 24, 0);
        GFL_BGSysLoadScr(2);
        GFL_BGSysSetBGEnabled(2, TRUE);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0xe000),
            GX_BG_CHARBASE(0x00000),
            0x8000,
            GX_BG_EXTPLTT_01,
            0,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(3);
        GFL_BGSysSetBGEnabled(3, TRUE);
    }
    GFL_BGSysClearCharCore(0, 32, 0, HEAPID_WORLDTRADE);
    GFL_BGSysClearCharCore(3, 32, 0, HEAPID_WORLDTRADE);
}

static void Deposit_BgExit(void) {
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysReleaseBG(3);
}

static void Deposit_BgGraphicSet(WorldTradeWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_WORLDTRADE, HEAPID_WORLDTRADE);

    GFL_BGSysLoadNCLRDefault(ARCID_WORLDTRADE, 0, 0, 0, 0x40, HEAPID_WORLDTRADE);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1a0, 0x20, HEAPID_WORLDTRADE);
    LoadSysMsgBox(0, 1, 14, 0, HEAPID_WORLDTRADE);
    LoadSysMsgBox(0, 31, 11, 0, HEAPID_WORLDTRADE);
    GFL_BGSysLoadNCGRStatic(ARCID_WORLDTRADE, 9, 1, 0, 0, TRUE, HEAPID_WORLDTRADE);
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_WORLDTRADE, 20, 1, 0, 0x600, TRUE, HEAPID_WORLDTRADE);
    GFL_BGSysLoadArcNCGRStatic(arc, 12, 2, 0, 0, TRUE, HEAPID_WORLDTRADE);
    WorldTrade_SubLcdWinGraphicSet(wk);
    GFL_ArcToolFree(arc);
}

static void Deposit_BmpWinInit(WorldTradeWork *wk) {
    BmpWin *win;
    int i;

    wk->msgWin = BmpWin_CreateDynamic(0, 2, 21, 27, 2, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWin), 0);
    win = wk->msgWin;
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));

    for (i = 0; i < 6; i++) {
        wk->infoWin[i] = BmpWin_CreateDynamic(3, sInfoBmpTable[i][0], sInfoBmpTable[i][1], 12, 2, 13, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->infoWin[i]), 0);
        win = wk->infoWin[i];
        BmpWin_FlushChar(win);
        BmpWin_FlushMap(win);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
    }
    WorldTrade_SubLcdExplainPut(wk, 3);
}

static void Deposit_BmpWinDelete(WorldTradeWork *wk) {
    int i;

    WorldTrade_PrintClear(&wk->print);
    BmpWin_Free(wk->explainWin);
    for (i = 0; i < 6; i++) {
        BmpWin_Free(wk->infoWin[i]);
    }
    BmpWin_Free(wk->msgWin);
}

static void Deposit_SetCellActor(WorldTradeWork *wk) {
    ClActorSetup setup;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = 160;
    setup.y = 32;
    wk->subCursorAct = func_0204c040(wk->clactUnit, wk->clactRes[WT_CLACT_RES_MAIN2][WT_CLACT_RES_CHAR],
                                     wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT],
                                     wk->clactRes[WT_CLACT_RES_MAIN2][WT_CLACT_RES_CELL], &setup, 0, HEAPID_WORLDTRADE);
    func_0204c488(wk->subCursorAct, 12);
    func_0204c520(wk->subCursorAct, TRUE);
    func_0204c550(wk->subCursorAct);
    func_0204c124(wk->subCursorAct, FALSE);

    setup.x = 228;
    setup.y = 117;
    wk->boxArrowAct[0] =
        func_0204c040(wk->clactUnit, wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CHAR],
                      wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT],
                      wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CELL], &setup, 0, HEAPID_WORLDTRADE);
    func_0204c488(wk->boxArrowAct[0], 38);
    func_0204c124(wk->boxArrowAct[0], FALSE);

    setup.x = 140;
    wk->boxArrowAct[1] =
        func_0204c040(wk->clactUnit, wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CHAR],
                      wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT],
                      wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CELL], &setup, 0, HEAPID_WORLDTRADE);
    func_0204c488(wk->boxArrowAct[1], 39);
    func_0204c124(wk->boxArrowAct[1], FALSE);
}

static void Deposit_DelCellActor(WorldTradeWork *wk) {
    func_0204c108(wk->subCursorAct);
    func_0204c108(wk->boxArrowAct[0]);
    func_0204c108(wk->boxArrowAct[1]);
}

static void Deposit_InitWork(WorldTradeWork *wk) {
    wk->talkString = GFL_StrBufCreate(180, HEAPID_WORLDTRADE);
    wk->dw = GFL_HeapAllocate(HEAPID_WORLDTRADE, sizeof(WorldTradeDepositWork), FALSE, "worldtrade_deposit.c", 692);
    sys_memset32_fast(0, wk->dw, sizeof(WorldTradeDepositWork));
    wk->dw->nameSortTable = WorldTrade_ZukanSortDataGet(HEAPID_WORLDTRADE, 0, &wk->dw->nameSortNum);
    wk->dw->sinouTable = WorldTrade_SinouZukanDataGet(HEAPID_WORLDTRADE);
    WorldTrade_SelectListPosInit(&wk->selectListPos);
}

static void Deposit_FreeWork(WorldTradeWork *wk) {
    GFL_HeapFree(wk->dw->sinouTable);
    GFL_HeapFree(wk->dw->nameSortTable);
    GFL_HeapFree(wk->dw);
    GFL_StrBufFree(wk->talkString);
}

static int Deposit_SubSeqStart(WorldTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        Deposit_SubSeqMessagePrint(wk, 9, 1, 0, 0xf0f);
        WorldTrade_SetNextSeq(wk, DEPOSIT_SEQ_MES_WAIT, DEPOSIT_SEQ_MAIN);
    }
    return WT_SEQ_MAIN;
}

static int Deposit_SubSeqMain(WorldTradeWork *wk) {
    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        WorldTrade_SubProcessChange(wk, WORLDTRADE_MYBOX, DEPOSIT_MODE_TO_BOX);
        wk->subprocessSeq = DEPOSIT_SEQ_END;
    }
    // Overrides the end, so B only changes the screen that comes after the input
    wk->subprocessSeq = DEPOSIT_SEQ_HEADWORD_SELECT_LIST;
    return WT_SEQ_MAIN;
}

static int Deposit_SubSeqHeadwordSelectList(WorldTradeWork *wk) {
    WorldTrade_Input_Start(wk->inputWork, INPUT_MODE_POKEMON_NAME);
    wk->subprocessSeq = DEPOSIT_SEQ_POKENAME_SELECT_WAIT;
    return WT_SEQ_MAIN;
}

static int Deposit_SubSeqHeadwordSelectWait(WorldTradeWork *wk) {
    switch (Deposit_BmpListMain(wk->bmpListWork, &wk->listpos)) {
    case 0:
        break;
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        BmpMenuList_Free(wk->bmpListWork, &wk->dw->headwordListPos, &wk->dw->headwordPos);
        ListMenuCore_FreeOptionList(wk->menuList);
        wk->subprocessSeq = DEPOSIT_SEQ_POKENAME_SELECT_LIST;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        break;
    case BMPMENULIST_CANCEL:
        BmpMenuList_Free(wk->bmpListWork, &wk->dw->headwordListPos, &wk->dw->headwordPos);
        ListMenuCore_FreeOptionList(wk->menuList);
        WorldTrade_ClearWindow(wk->menuWin[0], 0);
        BmpWin_ClearFrame(wk->msgWin, 0);
        BmpWin_Free(wk->menuWin[0]);
        BmpWin_Free(wk->menuWin[1]);
        WorldTrade_SubProcessChange(wk, WORLDTRADE_MYBOX, DEPOSIT_MODE_TO_BOX);
        wk->subprocessSeq = DEPOSIT_SEQ_END;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
        break;
    }
    return WT_SEQ_MAIN;
}

static int Deposit_SubSeqPokeNameSelectList(WorldTradeWork *wk) {
    wk->bmpListWork = WorldTrade_PokeNameListMake(wk, &wk->menuList, wk->menuWin[1], wk->msgManager,
                                                  wk->monsNameManager, wk->dw, wk->param->pokedex);
    wk->listpos = 0xffff;
    wk->subprocessSeq = DEPOSIT_SEQ_POKENAME_SELECT_WAIT;
    return WT_SEQ_MAIN;
}

BOOL WorldTrade_SexSelectionCheck(Dpw_Tr_PokemonSearchData *dtps, int sexSelection) {
    switch (sexSelection) {
    case 0:
        dtps->gender = 1;
        return TRUE;
    case 254:
        dtps->gender = 2;
        return TRUE;
    case 255:
        dtps->gender = 3;
        return TRUE;
    }
    return FALSE;
}

static int Deposit_SubSeqPokeNameSelectWait(WorldTradeWork *wk) {
    u32 ret = WorldTrade_Input_Main(wk->inputWork);
    int gender;
    void *personal;
    int sexSelection;

    switch (ret) {
    case BMPMENULIST_CANCEL:
        BmpWin_ClearFrame(wk->msgWin, 0);
        WorldTrade_SubProcessChange(wk, WORLDTRADE_MYBOX, DEPOSIT_MODE_TO_BOX);
        wk->subprocessSeq = DEPOSIT_SEQ_END;
        WorldTrade_SelectNameListBackup(&wk->selectListPos, wk->dw->headwordListPos + wk->dw->headwordPos,
                                        wk->dw->nameListPos, wk->dw->namePos);
        break;
    case BMPMENULIST_NULL:
        break;
    default:
        wk->want.characterNo = ret;
        personal = PML_PersonalLoad(ret, 0, HEAPID_WORLDTRADE);
        sexSelection = PML_PersonalGetParam(personal, 20);
        PML_PersonalFree(personal);
        wk->dw->sexSelection = sexSelection;
        if (WorldTrade_SexSelectionCheck(&wk->want, wk->dw->sexSelection)) {
            wk->subprocessSeq = DEPOSIT_SEQ_LEVEL_SELECT_MES;
            gender = wk->want.gender;
        } else {
            wk->subprocessSeq = DEPOSIT_SEQ_SEX_SELECT_MES;
            gender = SEARCH_GENDER_ANY;
        }
        WorldTrade_PokeWantPrint(wk->msgManager, wk->monsNameManager, wk->wordSet, &wk->infoWin[0],
                                 wk->want.characterNo, gender, -1, &wk->print);
        WorldTrade_SelectNameListBackup(&wk->selectListPos, wk->dw->headwordListPos + wk->dw->headwordPos,
                                        wk->dw->nameListPos, wk->dw->namePos);
        break;
    }
    return WT_SEQ_MAIN;
}

static int Deposit_SubSeqSexSelectMessage(WorldTradeWork *wk) {
    Deposit_SubSeqMessagePrint(wk, 10, 1, 0, 0xf0f);
    WorldTrade_SetNextSeq(wk, DEPOSIT_SEQ_MES_WAIT, DEPOSIT_SEQ_SEX_SELECT_LIST);
    return WT_SEQ_MAIN;
}

static int Deposit_SubSeqSexSelectList(WorldTradeWork *wk) {
    wk->listpos = 0xffff;
    WorldTrade_Input_Start(wk->inputWork, INPUT_MODE_SEX);
    wk->subprocessSeq = DEPOSIT_SEQ_SEX_SELECT_WAIT;
    return WT_SEQ_MAIN;
}

static int Deposit_SubSeqSexSelectWait(WorldTradeWork *wk) {
    u32 ret = WorldTrade_Input_Main(wk->inputWork);

    switch (ret) {
    case BMPMENULIST_CANCEL:
        BmpWin_ClearFrame(wk->msgWin, 0);
        wk->subprocessSeq = DEPOSIT_SEQ_START;
        break;
    case 0:
    case 1:
    case 2:
        wk->want.gender = ret + 1;
        wk->subprocessSeq = DEPOSIT_SEQ_LEVEL_SELECT_MES;
        WorldTrade_PokeWantPrint(wk->msgManager, wk->monsNameManager, wk->wordSet, &wk->infoWin[0],
                                 wk->want.characterNo, wk->want.gender, -1, &wk->print);
        break;
    }
    return WT_SEQ_MAIN;
}

static int Deposit_SubSeqLevelSelectMessage(WorldTradeWork *wk) {
    Deposit_SubSeqMessagePrint(wk, 11, 1, 0, 0xf0f);
    WorldTrade_SetNextSeq(wk, DEPOSIT_SEQ_MES_WAIT, DEPOSIT_SEQ_LEVEL_SELECT_LIST);
    return WT_SEQ_MAIN;
}

static int Deposit_SubSeqLevelSelectList(WorldTradeWork *wk) {
    WorldTrade_Input_Start(wk->inputWork, INPUT_MODE_LEVEL);
    wk->listpos = 0xffff;
    wk->subprocessSeq = DEPOSIT_SEQ_LEVEL_SELECT_WAIT;
    return WT_SEQ_MAIN;
}

static int Deposit_SubSeqLevelSelectWait(WorldTradeWork *wk) {
    u32 ret = WorldTrade_Input_Main(wk->inputWork);

    switch (ret) {
    case LEVEL_SELECT_NUM:
    case BMPMENULIST_CANCEL:
        BmpWin_ClearFrame(wk->msgWin, 0);
        if (WorldTrade_SexSelectionCheck(&wk->want, wk->dw->sexSelection)) {
            wk->subprocessSeq = DEPOSIT_SEQ_START;
        } else {
            wk->subprocessSeq = DEPOSIT_SEQ_SEX_SELECT_MES;
        }
        break;
    case BMPMENULIST_NULL:
        break;
    default:
        WorldTrade_LevelMinMaxSet(&wk->want, ret, LEVEL_PRINT_TBL_DEPOSIT);
        wk->subprocessSeq = DEPOSIT_SEQ_DEPOSITOK_MESSAGE;
        WorldTrade_PokeWantPrint(
            wk->msgManager, wk->monsNameManager, wk->wordSet, &wk->infoWin[0], wk->want.characterNo, wk->want.gender,
            WorldTrade_LevelTermGet(wk->want.level_min, wk->want.level_max, LEVEL_PRINT_TBL_DEPOSIT), &wk->print);
        break;
    }
    return WT_SEQ_MAIN;
}

static int Deposit_SubSeqDepositOkMessage(WorldTradeWork *wk) {
    Deposit_SubSeqMessagePrint(wk, 23, 1, 0, 0xf0f);
    WorldTrade_SetNextSeq(wk, DEPOSIT_SEQ_MES_WAIT, DEPOSIT_SEQ_DEPOSIT_YESNO);
    return WT_SEQ_MAIN;
}

static int Deposit_SubSeqDepositOkYesNo(WorldTradeWork *wk) {
    WorldTrade_TouchWinYesNoMake(wk, 20, 0x126, 3, FALSE);
    wk->subprocessSeq = DEPOSIT_SEQ_DEPOSIT_YESNO_WAIT;
    return WT_SEQ_MAIN;
}

static int Deposit_SubSeqDepositOkYesNoWait(WorldTradeWork *wk) {
    u32 ret = WorldTrade_TouchSwMain(wk);

    if (ret == 1) {
        WorldTrade_TouchWinYesNoDel(wk);
        WorldTrade_SubProcessChange(wk, WORLDTRADE_UPLOAD, DEPOSIT_MODE_UPLOAD);
        wk->subprocessSeq = DEPOSIT_SEQ_END;
        wk->subOutFlag = 1;
        Deposit_DepositPokemonDataMake(&wk->uploadPokemonData, wk);
    } else if (ret == 2) {
        WorldTrade_TouchWinYesNoDel(wk);
        WorldTrade_SubProcessChange(wk, WORLDTRADE_MYBOX, DEPOSIT_MODE_TO_BOX);
        wk->subprocessSeq = DEPOSIT_SEQ_END;
    }
    return WT_SEQ_MAIN;
}

static int Deposit_SubSeqEnd(WorldTradeWork *wk) {
    if (wk->subNextProcess == WORLDTRADE_ENTER) {
        GFL_WipeSet(0, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
        wk->subOutFlag = 1;
    } else {
        GFL_WipeSet(3, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
    }
    wk->subprocessSeq = DEPOSIT_SEQ_START;
    return WT_SEQ_FADEOUT;
}

static int Deposit_SubSeqMessageWait(WorldTradeWork *wk) {
    if (!WorldTrade_PrintIsBusy(&wk->print)) {
        wk->subprocessSeq = wk->subprocessNextSeq;
    }
    return WT_SEQ_MAIN;
}

static u16 Deposit_GetSexBaseColor(int sex) {
    switch (sex) {
    case 0:
        return 0x14c0;
    case 1:
        return 0xc80;
    }
    return 0;
}

void WorldTrade_PokeNamePrint(BmpWin *win, MsgData *nameManager, int monsno, int flag, int y, u16 color,
                              WorldTradePrint *print) {
    StrBuf *str;

    if (monsno != 0) {
        str = GFL_MsgDataLoadStrbufNew(nameManager, monsno);
        WorldTrade_SysPrint(win, str, 0, y, flag, color, print);
        GFL_StrBufFree(str);
    }
}

void WorldTrade_PokeNamePrintNoPut(BmpWin *win, MsgData *nameManager, int monsno, int y, u16 color,
                                   WorldTradePrint *print) {
    StrBuf *str;

    if (monsno != 0) {
        str = GFL_MsgDataLoadStrbufNew(nameManager, monsno);
        WorldTrade_PrintColor(win, 0, str, 0, y, 0, color, print);
        GFL_StrBufFree(str);
    }
}

void WorldTrade_CountryPrint(BmpWin *win, MsgData *nameManager, MsgData *msgManager, int countryCode, int flag, int y,
                             u16 color, WorldTradePrint *print) {
    StrBuf *str;

    if (countryCode != 0) {
        str = GFL_MsgDataLoadStrbufNew(nameManager, countryCode);
        WorldTrade_SysPrint(win, str, 0, y, flag, color, print);
        GFL_StrBufFree(str);
    } else {
        str = GFL_MsgDataLoadStrbufNew(msgManager, 0xbb);
        WorldTrade_SysPrint(win, str, 0, y, flag, color, print);
        GFL_StrBufFree(str);
    }
}

static u16 Deposit_GetSexColor(int sex, u16 color) {
    if (sex == 1) {
        return Deposit_GetSexBaseColor(0);
    }
    if (sex == 2) {
        color = Deposit_GetSexBaseColor(1);
    }
    return color;
}

void WorldTrade_SexPrint(BmpWin *win, MsgData *msgManager, int sex, int flag, int y, int printFlag, u16 color,
                         WorldTradePrint *print) {
    StrBuf *str;

    if (flag == 0 && sex == SEARCH_GENDER_ANY) {
        return;
    }
    str = GFL_MsgDataLoadStrbufNew(msgManager, WorldTrade_SexStringTable[sex]);
    // A print flag past 3 is an x position
    if (printFlag > 3) {
        WorldTrade_SysPrint(win, str, printFlag, y, 0, Deposit_GetSexColor(sex, color), print);
    } else {
        WorldTrade_SysPrint(win, str, 0, y, printFlag, Deposit_GetSexColor(sex, color), print);
    }
    GFL_StrBufFree(str);
}

void WorldTrade_SexPrintNoPut(BmpWin *win, MsgData *msgManager, int sex, int flag, int x, int y, u16 color,
                              WorldTradePrint *print) {
    StrBuf *str;

    if (flag == 0 && sex == SEARCH_GENDER_ANY) {
        return;
    }
    str = GFL_MsgDataLoadStrbufNew(msgManager, WorldTrade_SexStringTable[sex]);
    WorldTrade_PrintColor(win, 0, str, x, y, 0, Deposit_GetSexColor(sex, color), print);
    GFL_StrBufFree(str);
}

void WorldTrade_WantLevelPrint(BmpWin *win, MsgData *msgManager, int level, int flag, int y, u16 color, int tblSelect,
                               WorldTradePrint *print) {
    WorldTrade_WantLevelPrint_XY(win, msgManager, level, flag, 0, y, color, tblSelect, print);
}

static const WorldTradeLevelTerm sSearchLevelMinMaxTable[SEARCH_LEVEL_SELECT_NUM] = {
    { 0xad, 0, 0 },   { 0xae, 1, 10 },  { 0xaf, 11, 20 }, { 0xb0, 21, 30 }, { 0xb1, 31, 40 },  { 0xb2, 41, 50 },
    { 0xb3, 51, 60 }, { 0xb4, 61, 70 }, { 0xb5, 71, 80 }, { 0xb6, 81, 90 }, { 0xb7, 91, 100 },
};

static const WorldTradeLevelTerm sLevelMinMaxTable[LEVEL_SELECT_NUM] = {
    { 0x86, 0, 0 },  { 0x87, 0, 9 },  { 0x88, 10, 0 }, { 0x89, 20, 0 }, { 0x8a, 30, 0 }, { 0x8b, 40, 0 },
    { 0x8c, 50, 0 }, { 0x8d, 60, 0 }, { 0x8e, 70, 0 }, { 0x8f, 80, 0 }, { 0x90, 90, 0 }, { 0x91, 100, 100 },
};

void WorldTrade_WantLevelPrint_XY(BmpWin *win, MsgData *msgManager, int level, int flag, int x, int y, u16 color,
                                  int tblSelect, WorldTradePrint *print) {
    StrBuf *str;
    const WorldTradeLevelTerm *table;

    if (level == -1) {
        return;
    }
    table = sLevelMinMaxTable;
    if (tblSelect != LEVEL_PRINT_TBL_DEPOSIT) {
        table = sSearchLevelMinMaxTable;
    }
    str = GFL_MsgDataLoadStrbufNew(msgManager, table[level].msg);
    WorldTrade_SysPrint(win, str, x, y, flag, color, print);
    GFL_StrBufFree(str);
}

void WorldTrade_PokeWantPrint(MsgData *msgManager, MsgData *monsNameManager, WordSet *wordSet, BmpWin **win, int monsno,
                              int sex, int level, WorldTradePrint *print) {
    StrBuf *str;
    BOOL printSex = TRUE;
    int i;

    str = GFL_MsgDataLoadStrbufNew(msgManager, 0x6b);

    WorldTrade_SysPrint(win[0], str, 1, 0, 0, 0x440, print);
    for (i = 1; i < 3; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(win[i]), 0);
    }
    WorldTrade_PokeNamePrint(win[1], monsNameManager, monsno, 0, 0, 0x440, print);
    // The names of the Nidoran show their gender already
    if (monsno == SPECIES_NIDORAN_F && sex == 2) {
        printSex = FALSE;
    } else if (monsno == SPECIES_NIDORAN_M && sex == 1) {
        printSex = FALSE;
    }
    if (printSex && (sex == 1 || sex == 2)) {
        WorldTrade_SexPrint(win[1], msgManager, sex, 0, 0, 70, 0x440, print);
    }
    WorldTrade_WantLevelPrint(win[2], msgManager, level, 2, 0, 0x440, LEVEL_PRINT_TBL_DEPOSIT, print);
    GFL_StrBufFree(str);
}

void WorldTrade_MyPokeWantPrint(MsgData *msgManager, MsgData *monsNameManager, WordSet *wordSet, BmpWin **win,
                                int monsno, int sex, int level, WorldTradePrint *print) {
    StrBuf *str;
    BOOL printSex = TRUE;
    int i;

    str = GFL_MsgDataLoadStrbufNew(msgManager, 0x6b);

    WorldTrade_SysPrint(win[0], str, 0, 0, 0, 0x440, print);
    for (i = 1; i < 3; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(win[i]), 0);
    }
    WorldTrade_PokeNamePrint(win[1], monsNameManager, monsno, 0, 0, 0x440, print);
    if (monsno == SPECIES_NIDORAN_F && sex == 2) {
        printSex = FALSE;
    } else if (monsno == SPECIES_NIDORAN_M && sex == 1) {
        printSex = FALSE;
    }
    if (printSex && (sex == 1 || sex == 2)) {
        WorldTrade_SexPrint(win[1], msgManager, sex, 0, 0, 70, 0x440, print);
    }
    WorldTrade_WantLevelPrint(win[2], msgManager, level, 0, 0, 0x440, LEVEL_PRINT_TBL_DEPOSIT, print);
    GFL_StrBufFree(str);
}

void WorldTrade_PokeInfoPrint(MsgData *msgManager, WordSet *wordSet, BmpWin **win, BoxPkm *pkm,
                              Dpw_Tr_PokemonDataSimple *post, WorldTradePrint *print) {
    StrBuf *title;
    StrBuf *levelStr;
    StrBuf *name = GFL_StrBufCreate(11, HEAPID_WORLDTRADE);
    StrBuf *sexStr = GFL_StrBufCreate(11, HEAPID_WORLDTRADE);
    u32 showSex;
    int level;
    int sex;
    int i;

    PML_PkmGetParam(pkm, PKM_PARAM_NICKNAME, name);
    sex = PML_PkmGetParam(pkm, PKM_PARAM_SEX, NULL) + 1;
    level = PML_PkmGetLevel(pkm);
    showSex = PML_PkmGetParam(pkm, 0xad, NULL);
    title = GFL_MsgDataLoadStrbufNew(msgManager, 0x6e);
    WordSetNumber(wordSet, 3, level, 3, 0, TRUE);
    levelStr = WorldTrade_ExpandMessage(wordSet, msgManager, 0x72, HEAPID_WORLDTRADE);
    if (sex != SEARCH_GENDER_ANY) {
        GFL_MsgDataLoadStrbuf(msgManager, WorldTrade_SexStringTable[sex], sexStr);
    }
    for (i = 0; i < 3; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(win[i]), 0);
    }
    WorldTrade_SysPrint(win[0], title, 1, 0, 0, 0x3c40, print);
    WorldTrade_SysPrint(win[1], name, 0, 0, 0, 0x3c40, print);
    WorldTrade_SysPrint(win[2], levelStr, 0, 0, 2, 0x3c40, print);
    if (sex != SEARCH_GENDER_ANY && showSex) {
        WorldTrade_SysPrint(win[1], sexStr, 70, 0, 0, Deposit_GetSexBaseColor(sex - 1), print);
    }
    post->characterNo = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    post->gender = sex;
    post->level = level;
    GFL_StrBufFree(levelStr);
    GFL_StrBufFree(sexStr);
    GFL_StrBufFree(name);
    GFL_StrBufFree(title);
}

u16 *WorldTrade_ZukanSortDataGet(int heapId, int idx, int *num) {
    u32 size;
    u16 *data = GFL_ArcSysReadHeapNewLZGetLen(97, 5, FALSE, heapId, &size);

    *num = size / 2;
    return data;
}

// Where the species of each initial start and end in the sorted list. The name is the one the assert prints
static const u32 ZukanSortHiraTable[27] = {
    0,   28,  62,  112, 147, 167, 184, 226, 249, 253, 260, 282, 315, 372,
    386, 392, 435, 438, 463, 557, 590, 595, 615, 636, 637, 640, 649,
};

void WorldTrade_HeadwordRangeGet(int select, int *start, int *end) {
    // clang-format off
    GFL_ASSERT(select < NELEMS(ZukanSortHiraTable));
    // clang-format on
    *start = ZukanSortHiraTable[select];
    *end = ZukanSortHiraTable[select + 1];
}

u8 *WorldTrade_SinouZukanDataGet(int heapId) {
    u32 i = 0;
    u8 *table = GFL_HeapAllocate(HEAPID_WORLDTRADE, MONSNO_TABLE_SIZE, FALSE, "worldtrade_deposit.c", 1942);
    u32 size;
    u16 *data;
    u32 num;

    sys_memset32_fast(0, table, MONSNO_TABLE_SIZE);
    data = GFL_ArcSysReadHeapNewLZGetLen(97, 4, FALSE, heapId, &size);
    num = size / 2;
    for (; i < num; i++) {
        if (data[i] < MONSNO_TABLE_SIZE) {
            table[data[i]] = 1;
        }
    }
    GFL_HeapFree(data);
    return table;
}

void WorldTrade_PostPokemonBaseDataMake(Dpw_Tr_Data *dtd, WorldTradeWork *wk) {
    // Shaymin and Kyurem go to the server in their base forms
    if (WorldTrade_GetPPorPPP(wk->boxTrayNo)) {
        u32 species = PML_PkmGetParam(wk->depositPkm, PKM_PARAM_SPECIES, NULL);

        if (species == SPECIES_SHAYMIN || species == SPECIES_KYUREM) {
            PokeParty_ChangeForme((PartyPkm *)wk->depositPkm, 0);
        }
        sys_memcpy32_fast(wk->depositPkm, dtd, PokeParty_GetPkmRawSize());
    } else {
        u32 species = PML_PkmGetParam(wk->depositPkm, PKM_PARAM_SPECIES, NULL);

        if (species == SPECIES_SHAYMIN || species == SPECIES_KYUREM) {
            PML_PkmChangeForme(wk->depositPkm, 0);
        }
        WorldTrade_BoxPkmToPartyPkm(wk->depositPkm, (PartyPkm *)dtd);
    }

    wcharsncpy(GetPlayerName(wk->param->mystatus), dtd->name, 8);
    dtd->trainerID = getIDAsUInt(wk->param->mystatus);
    dtd->countryCode = UnityTowerVisitor_GetCountry(wk->param->mystatus);
    dtd->localCode = UnityTowerVisitor_GetProvince(wk->param->mystatus);
    dtd->trainerType = func_02008bf4(wk->param->mystatus);
    dtd->gender = getTrainerGender(wk->param->mystatus);
#ifdef BLACK2
    dtd->versionCode = VERSION_BLACK2;
#else
    dtd->versionCode = VERSION_WHITE2;
#endif
    dtd->langCode = 2;
    dtd->unk126 = getPlayerSurveys(wk->param->wifihistory);
    dtd->unk127 = func_02009ca0(wk->param->wifihistory);
    dtd->unkF7 = func_02009d28(wk->param->wifihistory);
}

static void Deposit_DepositPokemonDataMake(Dpw_Tr_Data *dtd, WorldTradeWork *wk) {
    WorldTrade_PostPokemonBaseDataMake(dtd, wk);
    dtd->postSimple = wk->post;
    dtd->wantSimple = wk->want;
}

static int Deposit_PokeNameSortListMake(ListMenuOption **menulist, MsgData *monsNameManager, MsgData *msgManager,
                                        u16 *table, u8 *sinou, int num, int select, PokeDexSave *zukan) {
    int count = 0;
    int start = sNameHeadTable[select];
    int max = sNameHeadTable[select + 1] - start;
    int i;

    for (i = 0; i < max; i++) {
        if (PokeDex_IsSeen(zukan, table[start + i])) {
            count++;
        }
    }
    *menulist = ListMenuCore_CreateOptionList(count + 1, HEAPID_WORLDTRADE);
    for (i = 0; i < max; i++) {
        if (PokeDex_IsSeen(zukan, table[start + i])) {
            ListMenuCore_AppendMsgOption(*menulist, monsNameManager, table[start + i], table[start + i],
                                         HEAPID_WORLDTRADE);
        }
    }
    ListMenuCore_AppendMsgOption(*menulist, msgManager, 0x85, BMPMENULIST_CANCEL, HEAPID_WORLDTRADE);
    return count + 1;
}

BmpMenuList *WorldTrade_PokeNameListMake(WorldTradeWork *wk, ListMenuOption **menulist, BmpWin *win,
                                         MsgData *msgManager, MsgData *monsNameManager, WorldTradeDepositWork *dw,
                                         PokeDexSave *zukan) {
    BmpMenuListHeader header;
    int head;
    int num;

    GFL_BitmapFill(BmpWin_GetBitmap(win), 15);
    head = dw->headwordListPos + dw->headwordPos;
    num = Deposit_PokeNameSortListMake(menulist, monsNameManager, msgManager, dw->nameSortTable, dw->sinouTable,
                                       dw->nameSortNum, head, zukan);
    header = sPokeNameListHeader;
    header.count = num;
    header.options = *menulist;
    BmpWin_DrawFrame(win, 0, 31, 11);
    return BmpMenuList_Create(&header, wk->selectListPos.nameList[head], wk->selectListPos.namePos[head],
                              HEAPID_WORLDTRADE);
}

int WorldTrade_LevelListAdd(ListMenuOption **menulist, MsgData *msgManager, int tblSelect) {
    int i;
    int num;
    const WorldTradeLevelTerm *table;

    if (tblSelect == LEVEL_PRINT_TBL_DEPOSIT) {
        table = sLevelMinMaxTable;
        num = LEVEL_SELECT_NUM;
    } else {
        table = sSearchLevelMinMaxTable;
        num = SEARCH_LEVEL_SELECT_NUM;
    }
    *menulist = ListMenuCore_CreateOptionList(num, HEAPID_WORLDTRADE);
    for (i = 0; i < num; i++) {
        ListMenuCore_AppendMsgOption(*menulist, msgManager, table[i].msg, i, HEAPID_WORLDTRADE);
    }
    return num;
}

void WorldTrade_LevelMinMaxSet(Dpw_Tr_PokemonSearchData *dtps, int index, int tblSelect) {
    const WorldTradeLevelTerm *table;

    // clang-format off
    if (tblSelect == LEVEL_PRINT_TBL_DEPOSIT) {
        table = sLevelMinMaxTable;
        GFL_ASSERT(index<(LEVEL_SELECT_NUM));
    } else {
        table = sSearchLevelMinMaxTable;
        GFL_ASSERT(index<(SEARCH_LEVEL_SELECT_NUM));
    }
    // clang-format on
    dtps->level_min = table[index].min;
    dtps->level_max = table[index].max;
}

int WorldTrade_LevelTermGet(int min, int max, int tblSelect) {
    int i;
    int num;
    const WorldTradeLevelTerm *table;

    if (tblSelect == LEVEL_PRINT_TBL_DEPOSIT) {
        table = sLevelMinMaxTable;
        num = LEVEL_SELECT_NUM;
    } else {
        table = sSearchLevelMinMaxTable;
        num = SEARCH_LEVEL_SELECT_NUM;
    }
    for (i = 0; i < num; i++) {
        if (min == table[i].min && max == table[i].max) {
            return i;
        }
    }
    return 0;
}

void WorldTrade_CountryCodeSet(WorldTradeWork *wk, int countryCode) {
    if (countryCode == 0) {
        wk->countryCode = 0;
    } else if (countryCode - 1 < WorldTrade_CountryListNum) {
        wk->countryCode = sCountryListTbl[countryCode - 1];
    }
}

int WorldTrade_NationSortListNumGet(int start, int *number) {
    int i;

    for (i = 0; i < 28; i++) {
        if (start == sNationHeadTable[i].head) {
            *number = sNationHeadTable[i].start;
            return sNationHeadTable[i + 1].start - sNationHeadTable[i].start;
        }
    }
    return 0;
}

int WorldTrade_NationSortListMake(ListMenuOption **menulist, MsgData *countryNameManager, int start) {
    int first;
    int num = WorldTrade_NationSortListNumGet(start, &first);
    int i;

    *menulist = ListMenuCore_CreateOptionList(num, HEAPID_WORLDTRADE);
    // The loop test is a 0/1 value, which keeps the loop entered at its test (docs/matching.md)
    for (i = 0; i < num ? TRUE : FALSE; i++) {
        ListMenuCore_AppendMsgOption(*menulist, countryNameManager, sCountryListTbl[first + i], first + i + 1,
                                     HEAPID_WORLDTRADE);
    }
    return num;
}

static u32 Deposit_BmpListMain(BmpMenuList *list, u16 *posBackup) {
    u16 pos;
    u32 ret = BmpMenuList_Update(list);

    BmpMenuList_GetCursorIndex(list, &pos);
    if (*posBackup != pos) {
        if (*posBackup != 0xffff) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
        }
        *posBackup = pos;
    }
    return ret;
}

void WorldTrade_SelectListPosInit(WorldTradeSelectListPos *slp) {
    int i;

    for (i = 0; i < 10; i++) {
        slp->nameList[i] = 0;
        slp->namePos[i] = 0;
    }
    slp->headList = 0;
    slp->headPos = 0;
}

void WorldTrade_SelectNameListBackup(WorldTradeSelectListPos *slp, int head, int list, int pos) {
    slp->nameList[head] = list;
    slp->namePos[head] = pos;
}
