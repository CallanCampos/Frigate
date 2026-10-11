#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
#include "dpw/dpw_tr.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/net.h"
#include "gfl/net_state.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/personal.h"
#include "worldtrade_local.h"
#include "system/bmp_menulist.h"
#include "system/bmp_winframe.h"
#include "system/wipe.h"

// The Global Trade Station's search screen, where the player sets the Pokémon, gender, level and country to look for,
// asks the server, and picks one of the trainers found on the lower screen. The names are ours, guessed

enum {
    SEARCH_SEQ_START,
    SEARCH_SEQ_MAIN,
    SEARCH_SEQ_END,
    SEARCH_SEQ_END_TO_ENTER,
    SEARCH_SEQ_INPUT_POKENAME_MES,
    SEARCH_SEQ_POKENAME_SELECT_LIST,
    SEARCH_SEQ_POKENAME_SELECT_WAIT,
    SEARCH_SEQ_SEX_SELECT_MES,
    SEARCH_SEQ_SEX_SELECT_LIST,
    SEARCH_SEQ_SEX_SELECT_WAIT,
    SEARCH_SEQ_LEVEL_SELECT_MES,
    SEARCH_SEQ_LEVEL_SELECT_LIST,
    SEARCH_SEQ_LEVEL_SELECT_WAIT,
    SEARCH_SEQ_COUNTRY_SELECT_MES,
    SEARCH_SEQ_COUNTRY_SELECT_LIST,
    SEARCH_SEQ_COUNTRY_SELECT_WAIT,
    SEARCH_SEQ_SEARCH_CHECK,
    SEARCH_SEQ_SERVER_QUERY,
    SEARCH_SEQ_SERVER_RESULT,
    SEARCH_SEQ_SEARCH_RESULT_MESSAGE,
    SEARCH_SEQ_SEARCH_RESULT_MESSAGE_WAIT,
    SEARCH_SEQ_RESULT_TOUCH_MESSAGE,
    SEARCH_SEQ_SERVER_QUERY_FAILURE,
    SEARCH_SEQ_MES_WAIT,
    SEARCH_SEQ_MES_WAIT_KEY,
    SEARCH_SEQ_MES_WAIT_FRAMES,
    SEARCH_SEQ_YESNO,
    SEARCH_SEQ_YESNO_SELECT,
    SEARCH_SEQ_TO_MAIN,
    SEARCH_SEQ_SEARCH_ERROR_MES,
    SEARCH_SEQ_ERROR_DISCONNECT_MES1,
    SEARCH_SEQ_ERROR_DISCONNECT_MES2,
    SEARCH_SEQ_EXCHANGE_SCREEN1,
    SEARCH_SEQ_RETURN_SCREEN2,
    SEARCH_SEQ_EXCHANGE_MAIN,
    SEARCH_SEQ_CURSOR_DECIDE_WAIT,
    SEARCH_SEQ_CURSOR_QUIT_WAIT,
    SEARCH_SEQ_CURSOR_VIEW_WAIT,
};

// Where the cursor is: the conditions on the left, the buttons on the right
enum {
    CURSOR_POS_POKEMON,
    CURSOR_POS_SEX,
    CURSOR_POS_LEVEL,
    CURSOR_POS_NATION,
    CURSOR_POS_VIEW,
    CURSOR_POS_SEARCH,
    CURSOR_POS_BACK,
    CURSOR_POS_NUM,
};

// The sub-process modes of the screens it goes to and comes from
#define SEARCH_MODE_FROM_TITLE 13
#define SEARCH_MODE_PARTNER_RETURN 15
#define PARTNER_MODE_FROM_SEARCH 16
#define ENTER_MODE_DISCONNECT 21

// The text color of the conditions and of the labels
#define SEARCH_INFO_COLOR 0x3c40
#define SEARCH_LABEL_COLOR 0x440

// The frames to wait for the server before giving up
#define SEARCH_TIMEOUT (30 * 60 * 4)

static void Search_BgInit(void);
static void Search_BgExit(void);
static void Search_BgGraphicSet(WorldTradeWork *wk);
static void Search_SetCellActor(WorldTradeWork *wk);
static void Search_DelCellActor(WorldTradeWork *wk);
static void Search_BmpWinInit(WorldTradeWork *wk);
static void Search_BmpWinDelete(WorldTradeWork *wk);
static void Search_InitWork(WorldTradeWork *wk);
static void Search_FreeWork(WorldTradeWork *wk);
static int Search_SubSeqStart(WorldTradeWork *wk);
static int Search_TouchPanelFunc(WorldTradeWork *wk);
static void Search_DecideFunc(WorldTradeWork *wk, int decide);
static int Search_SubSeqMain(WorldTradeWork *wk);
static int Search_SubSeqSearchCheck(WorldTradeWork *wk);
static int Search_SubSeqServerQuery(WorldTradeWork *wk);
static int Search_SubSeqServerResult(WorldTradeWork *wk);
static int Search_SubSeqSearchResultMessage(WorldTradeWork *wk);
static int Search_SubSeqSearchResultMessageWait(WorldTradeWork *wk);
static int Search_SubSeqResultTouchMessage(WorldTradeWork *wk);
static int Search_SubSeqServerQueryFailure(WorldTradeWork *wk);
static int Search_SubSeqErrorDisconnectMessage1(WorldTradeWork *wk);
static int Search_SubSeqErrorDisconnectMessage2(WorldTradeWork *wk);
static int Search_CursorPosGet(WorldTradeWork *wk);
static void Search_CursorMove(WorldTradeWork *wk);
static void Search_TouchCursorMove(WorldTradeWork *wk, int touch);
static int Search_SubSeqEnd(WorldTradeWork *wk);
static int Search_SubSeqEndToEnter(WorldTradeWork *wk);
static int Search_SubSeqInputPokenameMessage(WorldTradeWork *wk);
static int Search_SubSeqPokenameSelectList(WorldTradeWork *wk);
static int Search_SubSeqPokenameSelectWait(WorldTradeWork *wk);
static int Search_SubSeqSexSelectMes(WorldTradeWork *wk);
static int Search_SubSeqSexSelectList(WorldTradeWork *wk);
static int Search_SubSeqSexSelectWait(WorldTradeWork *wk);
static int Search_SubSeqLevelSelectMes(WorldTradeWork *wk);
static int Search_SubSeqLevelSelectList(WorldTradeWork *wk);
static int Search_SubSeqLevelSelectWait(WorldTradeWork *wk);
static int Search_SubSeqCountrySelectMes(WorldTradeWork *wk);
static int Search_SubSeqCountrySelectList(WorldTradeWork *wk);
static int Search_SubSeqCountrySelectWait(WorldTradeWork *wk);
static int Search_SubSeqYesNo(WorldTradeWork *wk);
static int Search_SubSeqYesNoSelect(WorldTradeWork *wk);
static int Search_SubSeqToMain(WorldTradeWork *wk);
static int Search_SubSeqSearchErrorMessage(WorldTradeWork *wk);
static int Search_SubSeqMessageWait(WorldTradeWork *wk);
static int Search_SubSeqMessageWaitKey(WorldTradeWork *wk);
static int Search_SubSeqMessageWaitFrames(WorldTradeWork *wk);
static int Search_SubSeqExchangeScreen1(WorldTradeWork *wk);
static int Search_SubSeqReturnScreen2(WorldTradeWork *wk);
static int Search_SubSeqExchangeMain(WorldTradeWork *wk);
static int Search_SubSeqCursorDecideWait(WorldTradeWork *wk);
static int Search_SubSeqCursorQuitWait(WorldTradeWork *wk);
static int Search_SubSeqCursorViewWait(WorldTradeWork *wk);
static void Search_SubSeqMessagePrint(WorldTradeWork *wk, int msgNo, int wait, int flag, u16 dat);
static void Search_WantLabelPrint(BmpWin **win, BmpWin **countryWin, MsgData *msgManager, WorldTradePrint *print);
static void Search_FriendViewButtonPrint(BmpWin *win, MsgData *msgManager, int flag, WorldTradePrint *print);
static BOOL Search_DpwSearchCompare(const Dpw_Tr_PokemonSearchData *s1, const Dpw_Tr_PokemonSearchData *s2,
                                    int countryCode1, int countryCode2);
static void Search_SlideScreenVFunc(WorldTradeWork *wk);
static void Search_BgBlendSet(int rate);

// The cursor's position, its animation and its animation when chosen, at each place
static u16 sSearchCursorPos[CURSOR_POS_NUM][4] = {
    { 0, 0, 2, 11 },    { 0, 40, 2, 11 },   { 0, 80, 2, 11 },    { 0, 120, 2, 11 },
    { 192, 32, 3, 12 }, { 192, 72, 3, 12 }, { 192, 112, 3, 12 },
};

static int (*sSearchSubSeqTable[])(WorldTradeWork *wk) = {
    Search_SubSeqStart,
    Search_SubSeqMain,
    Search_SubSeqEnd,
    Search_SubSeqEndToEnter,
    Search_SubSeqInputPokenameMessage,
    Search_SubSeqPokenameSelectList,
    Search_SubSeqPokenameSelectWait,
    Search_SubSeqSexSelectMes,
    Search_SubSeqSexSelectList,
    Search_SubSeqSexSelectWait,
    Search_SubSeqLevelSelectMes,
    Search_SubSeqLevelSelectList,
    Search_SubSeqLevelSelectWait,
    Search_SubSeqCountrySelectMes,
    Search_SubSeqCountrySelectList,
    Search_SubSeqCountrySelectWait,
    Search_SubSeqSearchCheck,
    Search_SubSeqServerQuery,
    Search_SubSeqServerResult,
    Search_SubSeqSearchResultMessage,
    Search_SubSeqSearchResultMessageWait,
    Search_SubSeqResultTouchMessage,
    Search_SubSeqServerQueryFailure,
    Search_SubSeqMessageWait,
    Search_SubSeqMessageWaitKey,
    Search_SubSeqMessageWaitFrames,
    Search_SubSeqYesNo,
    Search_SubSeqYesNoSelect,
    Search_SubSeqToMain,
    Search_SubSeqSearchErrorMessage,
    Search_SubSeqErrorDisconnectMessage1,
    Search_SubSeqErrorDisconnectMessage2,
    Search_SubSeqExchangeScreen1,
    Search_SubSeqReturnScreen2,
    Search_SubSeqExchangeMain,
    Search_SubSeqCursorDecideWait,
    Search_SubSeqCursorQuitWait,
    Search_SubSeqCursorViewWait,
};

int WorldTrade_Search_Init(WorldTradeWork *wk, int seq) {
    WorldTradeInputHeader header;

    Search_InitWork(wk);
    Search_BgInit();
    Search_BgGraphicSet(wk);
    Search_BmpWinInit(wk);
    Search_SetCellActor(wk);
    WorldTrade_SubLcdBgInit(wk, 0, 0);
    WorldTrade_SubLcdWinGraphicSet(wk);
    WorldTrade_SubLcdExplainPut(wk, 4);
    wk->subLcdBgKeep = 0;

    // The header's config is left unset
    header.menuWin = wk->menuWin;
    header.backWin = &wk->backWin;
    header.cursorAct = wk->subCursorAct;
    header.arrowAct[0] = wk->boxArrowAct[0];
    header.arrowAct[1] = wk->boxArrowAct[1];
    header.searchCursorAct = wk->cursorAct;
    header.msgManager = wk->msgManager;
    header.monsNameManager = wk->monsNameManager;
    header.countryNameManager = wk->countryNameManager;
    header.zukan = wk->param->pokedex;
    header.sinouTable = wk->dw->sinouTable;
    wk->inputWork = WorldTrade_Input_Init(&header, 2, INPUT_SITUATION_SEARCH);

    Search_WantLabelPrint(&wk->infoWin[0], wk->countryWin, wk->msgManager, &wk->print);
    Search_FriendViewButtonPrint(wk->infoWin[8], wk->msgManager, wk->subLcdTouchOK, &wk->print);
    WorldTrade_PokeNamePrint(wk->infoWin[1], wk->monsNameManager, wk->search.characterNo, 0, 0, SEARCH_INFO_COLOR,
                             &wk->print);
    WorldTrade_SexPrint(wk->infoWin[3], wk->msgManager, wk->search.gender, 1, 0, 0, SEARCH_INFO_COLOR, &wk->print);
    WorldTrade_WantLevelPrint(
        wk->infoWin[5], wk->msgManager,
        WorldTrade_LevelTermGet(wk->search.level_min, wk->search.level_max, LEVEL_PRINT_TBL_SEARCH), 0, 0,
        SEARCH_INFO_COLOR, LEVEL_PRINT_TBL_SEARCH, &wk->print);
    WorldTrade_CountryPrint(wk->countryWin[1], wk->countryNameManager, wk->msgManager, wk->countryCode, 0, 0,
                            SEARCH_INFO_COLOR, &wk->print);

    wk->vfunc2 = Search_SlideScreenVFunc;
    if (wk->subProcessMode == SEARCH_MODE_FROM_TITLE) {
        GFL_WipeSet(3, 1, 1, 0, 6, 1, HEAPID_WORLDTRADE);
    }
    wk->subprocessSeq = SEARCH_SEQ_START;
    return WT_SEQ_FADEIN;
}

int WorldTrade_Search_Main(WorldTradeWork *wk, int seq) {
    int ret = sSearchSubSeqTable[wk->subprocessSeq](wk);
    int i;

    for (i = 0; i < 8; i++) {
        WorldTrade_ActPos(wk->subAct[i], wk->subActY[i][0], wk->subActY[i][1] + wk->drawOffset);
    }
    WorldTrade_CLACT_PosChange(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][0],
                               sSearchCursorPos[Search_CursorPosGet(wk)][1] - wk->drawOffset);
    WorldTrade_ActPos(wk->promptDsAct, 55, wk->drawOffset + 168);
    return ret;
}

int WorldTrade_Search_End(WorldTradeWork *wk, int seq) {
    wk->vfunc2 = NULL;
    WorldTrade_Input_Exit(wk->inputWork);
    Search_DelCellActor(wk);
    Search_FreeWork(wk);
    Search_BmpWinDelete(wk);
    Search_BgExit();
    if (wk->subNextProcess == WORLDTRADE_TITLE) {
        wk->subLcdBgKeep = 1;
    }
    WorldTrade_SubLcdBgExit(wk);
    func_0204c124(wk->promptDsAct, FALSE);
    WorldTrade_SubProcessUpdate(wk);
    return WT_SEQ_INIT;
}

static void Search_BgInit(void) {
    {
        BGSysLCDConfig config = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_2D };
        u32 enabled = GFL_BGSysGetEnabledBGsB();

        GFL_BGSysSetLCDConfig(&config);
        GFL_BGSysSetEnabledBGsB(enabled);
    }
    {
        BGSetup setup = {
            0,
            0,
            0x800,
            0,
            BGRES_256x256,
            GX_BG_COLORMODE_16,
            GX_BG_SCRBASE(0x0000),
            GX_BG_CHARBASE(0x04000),
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
            GX_BG_SCRBASE(0x0800),
            GX_BG_CHARBASE(0x0c000),
            0x8000,
            GX_BG_EXTPLTT_01,
            3,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(1, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(1);
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
            GX_BG_SCRBASE(0x1000),
            GX_BG_CHARBASE(0x08000),
            0x8000,
            GX_BG_EXTPLTT_01,
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(2);
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
            GX_BG_SCRBASE(0x1800),
            GX_BG_CHARBASE(0x1c000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(3, &setup, BGMODE_TEXT);
        GFL_BGSysClearScr(3);
        GFL_BGSysSetBGEnabled(3, TRUE);
    }
    GFL_BGSysClearCharCore(2, 32, 0, HEAPID_WORLDTRADE);
    GFL_BGSysClearCharCore(0, 32, 0, HEAPID_WORLDTRADE);
    GFL_BGSysClearCharCore(3, 32, 0, HEAPID_WORLDTRADE);
}

static void Search_BgExit(void) {
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysReleaseBG(3);
}

static void Search_BgGraphicSet(WorldTradeWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_WORLDTRADE, HEAPID_WORLDTRADE);

    GFL_G2DIOLoadArcNCLRDefault(arc, 2, 0, 0, 0x60, HEAPID_WORLDTRADE);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1a0, 0x20, HEAPID_WORLDTRADE);
    LoadSysMsgBox(0, 1, 14, 0, HEAPID_WORLDTRADE);
    LoadSysMsgBox(0, 31, 11, 0, HEAPID_WORLDTRADE);
    GFL_BGSysLoadArcNCGRStatic(arc, 11, 1, 0, 0, TRUE, HEAPID_WORLDTRADE);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 23, 1, 0, 0, TRUE, HEAPID_WORLDTRADE);
    GFL_BGSysLoadArcNCGRStatic(arc, 12, 2, 0, 0, TRUE, HEAPID_WORLDTRADE);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x20, 0x20, HEAPID_WORLDTRADE);
    GFL_ArcToolFree(arc);
}

static void Search_SetCellActor(WorldTradeWork *wk) {
    ClActorSetup setup;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = sSearchCursorPos[0][0];
    setup.y = sSearchCursorPos[0][1];
    wk->cursorAct = func_0204c040(wk->clactUnit, wk->clactRes[WT_CLACT_RES_MAIN2][WT_CLACT_RES_CHAR],
                                  wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT],
                                  wk->clactRes[WT_CLACT_RES_MAIN2][WT_CLACT_RES_CELL], &setup, 0, HEAPID_WORLDTRADE);
    func_0204c520(wk->cursorAct, TRUE);
    func_0204c488(wk->cursorAct, sSearchCursorPos[0][2]);
    func_0204c468(wk->cursorAct, 1);
    if (func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, FALSE);
    } else {
        func_0204c124(wk->cursorAct, TRUE);
    }

    setup.x = 160;
    setup.y = 32;
    wk->subCursorAct = func_0204c040(wk->clactUnit, wk->clactRes[WT_CLACT_RES_MAIN2][WT_CLACT_RES_CHAR],
                                     wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT],
                                     wk->clactRes[WT_CLACT_RES_MAIN2][WT_CLACT_RES_CELL], &setup, 0, HEAPID_WORLDTRADE);
    func_0204c488(wk->subCursorAct, 4);
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

    func_02042ba8(FALSE, HEAPID_WORLDTRADE);
}

static void Search_DelCellActor(WorldTradeWork *wk) {
    func_0204c108(wk->cursorAct);
    func_0204c108(wk->subCursorAct);
    func_0204c108(wk->boxArrowAct[0]);
    func_0204c108(wk->boxArrowAct[1]);
}

// The windows of the search's conditions, of its buttons and of its country
static const u16 sSearchCountryWinPos[2][2] = {
    { 1, 16 },
    { 2, 18 },
};

static const u16 sSearchButtonWinPos[3][2] = {
    { 22, 8 },
    { 22, 13 },
    { 22, 3 },
};

static const u16 sSearchInfoWinPos[6][2] = {
    { 1, 1 }, { 2, 3 }, { 1, 6 }, { 2, 8 }, { 1, 11 }, { 2, 13 },
};

static void Search_BmpWinInit(WorldTradeWork *wk) {
    BmpWin *win;
    int i;

    wk->msgWin = BmpWin_CreateDynamic(0, 2, 21, 27, 2, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWin), 0);
    win = wk->msgWin;
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
    GFL_BGSysFillChar(3, 0, 1, 0);

    for (i = 0; i < 6; i++) {
        wk->infoWin[i] = BmpWin_CreateDynamic(3, sSearchInfoWinPos[i][0], sSearchInfoWinPos[i][1], 11, 2, 13, FALSE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->infoWin[i]), 0);
        win = wk->infoWin[i];
        BmpWin_FlushChar(win);
        BmpWin_FlushMap(win);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
    }
    for (i = 0; i < 2; i++) {
        wk->countryWin[i] =
            BmpWin_CreateDynamic(3, sSearchCountryWinPos[i][0], sSearchCountryWinPos[i][1], 28, 2, 13, FALSE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->countryWin[i]), 0);
        win = wk->countryWin[i];
        BmpWin_FlushChar(win);
        BmpWin_FlushMap(win);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
    }
    for (i = 0; i < 3; i++) {
        wk->infoWin[6 + i] =
            BmpWin_CreateDynamic(3, sSearchButtonWinPos[i][0], sSearchButtonWinPos[i][1], 10, 2, 13, FALSE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->infoWin[6 + i]), 0);
        win = wk->infoWin[6 + i];
        BmpWin_FlushChar(win);
        BmpWin_FlushMap(win);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
    }
}

static void Search_BmpWinDelete(WorldTradeWork *wk) {
    int i;

    WorldTrade_PrintClear(&wk->print);
    BmpWin_Free(wk->explainWin);
    BmpWin_Free(wk->msgWin);
    for (i = 0; i < 9; i++) {
        BmpWin_Free(wk->infoWin[i]);
    }
    for (i = 0; i < 2; i++) {
        BmpWin_Free(wk->countryWin[i]);
    }
    GFL_BGSysFreeFilledChar(3, 1, 0);
}

static void Search_InitWork(WorldTradeWork *wk) {
    wk->talkString = GFL_StrBufCreate(180, HEAPID_WORLDTRADE);
    wk->titleString = GFL_MsgDataLoadStrbufNew(wk->msgManager, 0x30);
    wk->dw = GFL_HeapAllocate(HEAPID_WORLDTRADE, sizeof(WorldTradeDepositWork), FALSE, "worldtrade_search.c", 849);
    sys_memset32_fast(0, wk->dw, sizeof(WorldTradeDepositWork));
    wk->dw->sinouTable = WorldTrade_SinouZukanDataGet(HEAPID_WORLDTRADE);
    WorldTrade_SelectListPosInit(&wk->selectListPos);
}

static void Search_FreeWork(WorldTradeWork *wk) {
    GFL_HeapFree(wk->dw->sinouTable);
    GFL_HeapFree(wk->dw);
    GFL_StrBufFree(wk->talkString);
    GFL_StrBufFree(wk->titleString);
}

static int Search_SubSeqStart(WorldTradeWork *wk) {
    if (func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, FALSE);
    } else {
        func_0204c124(wk->cursorAct, TRUE);
    }

    if (wk->subProcessMode == SEARCH_MODE_PARTNER_RETURN) {
        if (wk->searchResult != 0) {
            Search_SubSeqMessagePrint(wk, 0x26, 0, 0, 0xf0f);
        } else {
            Search_SubSeqMessagePrint(wk, 8, 1, 0, 0xf0f);
        }
        if (gfxRegGetMasterBrightness(REG_MASTER_BRIGHT_ADDR) == -16) {
            GFL_WipeSet(0, 1, 1, 0, 16, 1, HEAPID_WORLDTRADE);
            wk->subprocessSeq = SEARCH_SEQ_RETURN_SCREEN2;
        } else {
            wk->subprocessSeq = SEARCH_SEQ_MAIN;
        }
    } else {
        Search_SubSeqMessagePrint(wk, 8, 1, 0, 0xf0f);
        WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_MAIN);
    }
    func_0204c124(wk->promptDsAct, TRUE);
    return WT_SEQ_MAIN;
}

static const TouchRect sSearchTouchRects[] = {
    { 3, 41, 2, 99 },     { 43, 81, 2, 99 },    { 83, 121, 2, 99 },    { 123, 161, 2, 99 },
    { 19, 45, 145, 255 }, { 58, 85, 145, 255 }, { 98, 125, 145, 255 }, { TOUCH_RECT_END, 0, 0, 0 },
};

// The column and row of the cursor that each touch rectangle puts it on
static u8 sSearchCursorTable[CURSOR_POS_NUM][2] = {
    { 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 }, { 1, 0 }, { 1, 1 }, { 1, 2 },
};

static int Search_TouchPanelFunc(WorldTradeWork *wk) {
    return func_0203da0c(sSearchTouchRects);
}

static void Search_DecideFunc(WorldTradeWork *wk, int decide) {
    void *personal;
    int sexSelection;

    switch (decide) {
    case CURSOR_POS_POKEMON:
        wk->subprocessSeq = SEARCH_SEQ_INPUT_POKENAME_MES;
        break;
    case CURSOR_POS_SEX:
        if (wk->search.characterNo != 0) {
            personal = PML_PersonalLoad(wk->search.characterNo, 0, HEAPID_WORLDTRADE);
            sexSelection = PML_PersonalGetParam(personal, 20);
            PML_PersonalFree(personal);
            wk->dw->sexSelection = sexSelection;
            if (WorldTrade_SexSelectionCheck(&wk->search, wk->dw->sexSelection)) {
                break;
            }
        }
        wk->subprocessSeq = SEARCH_SEQ_SEX_SELECT_MES;
        break;
    case CURSOR_POS_LEVEL:
        wk->subprocessSeq = SEARCH_SEQ_LEVEL_SELECT_MES;
        break;
    case CURSOR_POS_NATION:
        wk->subprocessSeq = SEARCH_SEQ_COUNTRY_SELECT_MES;
        break;
    case CURSOR_POS_VIEW:
        if (wk->searchResult != 0) {
            wk->subprocessSeq = SEARCH_SEQ_EXCHANGE_SCREEN1;
            GFL_WipeSet(0, 0, 0, 0, 16, 1, HEAPID_WORLDTRADE);
        }
        break;
    case CURSOR_POS_SEARCH:
        wk->subprocessSeq = SEARCH_SEQ_SEARCH_CHECK;
        break;
    case CURSOR_POS_BACK:
        Search_SubSeqMessagePrint(wk, 0xf, 1, 0, 0xf0f);
        WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_YESNO);
        break;
    }
}

static int Search_SubSeqMain(WorldTradeWork *wk) {
    int touch = Search_TouchPanelFunc(wk);
    void *personal;
    int sexSelection;
    int pos;

    if (touch != TOUCH_RECT_NONE) {
        func_0203d564(TRUE);
        Search_TouchCursorMove(wk, touch);
        pos = Search_CursorPosGet(wk);
        switch (pos) {
        case CURSOR_POS_SEX:
            if (wk->search.characterNo != 0) {
                personal = PML_PersonalLoad(wk->search.characterNo, 0, HEAPID_WORLDTRADE);
                sexSelection = PML_PersonalGetParam(personal, 20);
                PML_PersonalFree(personal);
                wk->dw->sexSelection = sexSelection;
                if (WorldTrade_SexSelectionCheck(&wk->search, wk->dw->sexSelection)) {
                    func_0204c124(wk->cursorAct, TRUE);
                    func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][3]);
                    GFL_SndSEPlay(SEQ_SE_BEEP);
                    return WT_SEQ_MAIN;
                }
            }
            func_0204c124(wk->cursorAct, TRUE);
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
            func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][3]);
            wk->subprocessSeq = SEARCH_SEQ_CURSOR_DECIDE_WAIT;
            break;
        case CURSOR_POS_SEARCH:
            if (wk->search.characterNo == 0 ||
                Search_DpwSearchCompare(&wk->search, &wk->searchBackup, wk->countryCode, wk->searchBackupCountryCode)) {
                GFL_SndSEPlay(SEQ_SE_BEEP);
            } else {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            }
            func_0204c124(wk->cursorAct, TRUE);
            func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][3]);
            wk->subprocessSeq = SEARCH_SEQ_CURSOR_DECIDE_WAIT;
            break;
        case CURSOR_POS_VIEW:
            if (wk->searchResult != 0) {
                func_0204c124(wk->cursorAct, TRUE);
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][3]);
                wk->subprocessSeq = SEARCH_SEQ_CURSOR_DECIDE_WAIT;
            } else {
                func_0204c124(wk->cursorAct, TRUE);
                func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][3]);
                GFL_SndSEPlay(SEQ_SE_BEEP);
                wk->subprocessSeq = SEARCH_SEQ_CURSOR_VIEW_WAIT;
            }
            break;
        default:
            func_0204c124(wk->cursorAct, TRUE);
            if (pos == CURSOR_POS_BACK) {
                GFL_SndSEPlay(SEQ_SE_CANCEL1);
            } else {
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
            }
            func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][3]);
            wk->subprocessSeq = SEARCH_SEQ_CURSOR_DECIDE_WAIT;
            break;
        }
    } else {
        if (GCTX_HIDGetPressedKeys() != 0 && func_0203d554() == TRUE) {
            func_0204c124(wk->cursorAct, TRUE);
            func_0203d564(FALSE);
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            return WT_SEQ_MAIN;
        }
        Search_CursorMove(wk);
        if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
            pos = Search_CursorPosGet(wk);
            switch (pos) {
            case CURSOR_POS_SEX:
                if (wk->search.characterNo != 0) {
                    personal = PML_PersonalLoad(wk->search.characterNo, 0, HEAPID_WORLDTRADE);
                    sexSelection = PML_PersonalGetParam(personal, 20);
                    PML_PersonalFree(personal);
                    wk->dw->sexSelection = sexSelection;
                    if (WorldTrade_SexSelectionCheck(&wk->search, wk->dw->sexSelection)) {
                        func_0204c124(wk->cursorAct, TRUE);
                        func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][3]);
                        GFL_SndSEPlay(SEQ_SE_BEEP);
                        return WT_SEQ_MAIN;
                    }
                }
                func_0204c124(wk->cursorAct, TRUE);
                GFL_SndSEPlay(SEQ_SE_DECIDE1);
                func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][3]);
                wk->subprocessSeq = SEARCH_SEQ_CURSOR_DECIDE_WAIT;
                break;
            case CURSOR_POS_SEARCH:
                if (wk->search.characterNo == 0 ||
                    Search_DpwSearchCompare(&wk->search, &wk->searchBackup, wk->countryCode,
                                            wk->searchBackupCountryCode)) {
                    GFL_SndSEPlay(SEQ_SE_BEEP);
                } else {
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                }
                func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][3]);
                wk->subprocessSeq = SEARCH_SEQ_CURSOR_DECIDE_WAIT;
                break;
            case CURSOR_POS_VIEW:
                if (wk->searchResult != 0) {
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                    func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][3]);
                    wk->subprocessSeq = SEARCH_SEQ_CURSOR_DECIDE_WAIT;
                } else {
                    func_0204c124(wk->cursorAct, TRUE);
                    func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][3]);
                    GFL_SndSEPlay(SEQ_SE_BEEP);
                    wk->subprocessSeq = SEARCH_SEQ_CURSOR_VIEW_WAIT;
                }
                break;
            default:
                if (pos == CURSOR_POS_BACK) {
                    GFL_SndSEPlay(SEQ_SE_CANCEL1);
                } else {
                    GFL_SndSEPlay(SEQ_SE_DECIDE1);
                }
                func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][3]);
                wk->subprocessSeq = SEARCH_SEQ_CURSOR_DECIDE_WAIT;
                break;
            }
        } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
            wk->dw->cursorSide = 1;
            wk->dw->rightCursorPos = 2;
            func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][3]);
            wk->subprocessSeq = SEARCH_SEQ_CURSOR_QUIT_WAIT;
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        }
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqSearchCheck(WorldTradeWork *wk) {
    if (wk->search.characterNo == 0) {
        Search_SubSeqMessagePrint(wk, 0xc, 1, 0, 0xf0f);
        WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_MAIN);
    } else if (Search_DpwSearchCompare(&wk->search, &wk->searchBackup, wk->countryCode, wk->searchBackupCountryCode)) {
        Search_SubSeqMessagePrint(wk, 0x27, 1, 0, 0xf0f);
        WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_MAIN);
    } else {
        GFL_SndSEPlay(SEQ_SE_SYS_78);
        wk->unk12E8 = 118;
        Search_SubSeqMessagePrint(wk, 0xd, 1, 0, 0xf0f);
        WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_SERVER_QUERY);
        if (wk->searchResult > 0) {
            WorldTrade_SubLcdMatchObjHide(wk);
        }
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqServerQuery(WorldTradeWork *wk) {
    if (wk->countryCode == 0) {
        DpwTr_Search(&wk->search, SEARCH_POKE_MAX, wk->downloadPokemonData);
    } else {
        Dpw_Tr_PokemonSearchDataEx searchEx;

        sys_memset(&searchEx, 0, sizeof(Dpw_Tr_PokemonSearchDataEx));
        searchEx.characterNo = wk->search.characterNo;
        searchEx.gender = wk->search.gender;
        searchEx.level_min = wk->search.level_min;
        searchEx.level_max = wk->search.level_max;
        searchEx.unused = wk->search.unused;
        searchEx.maxNum = SEARCH_POKE_MAX;
        searchEx.countryCode = wk->countryCode;
        DpwTr_SearchEx(&searchEx, wk->downloadPokemonData);
    }
    wk->searchBackup = wk->search;
    wk->searchBackupCountryCode = wk->countryCode;
    wk->timeoutCount = 0;
    wk->subprocessSeq = SEARCH_SEQ_SERVER_RESULT;
    wk->subLcdTouchOK = 0;
    return WT_SEQ_MAIN;
}

static int Search_SubSeqServerResult(WorldTradeWork *wk) {
    s32 result;

    if (DpwTr_IsFinished()) {
        result = DpwTr_GetResult();
        wk->timeoutCount = 0;
        switch (result) {
        case 0:
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            wk->searchResult = result;
            wk->subprocessSeq = SEARCH_SEQ_SEARCH_RESULT_MESSAGE;
            break;
        case -2:
        case -14:
            GFL_SndStop();
            wk->searchResult = 0;
            wk->subprocessSeq = SEARCH_SEQ_ERROR_DISCONNECT_MES1;
            func_02012154();
            func_020424e4();
            func_02012144();
            func_02042478();
            break;
        case -13:
            GFL_SndStop();
            WorldTrade_ShowFatalError(wk);
            break;
        case -12:
        case -15:
            GFL_SndStop();
            wk->subprocessSeq = SEARCH_SEQ_SERVER_QUERY_FAILURE;
            break;
        }
    } else if (++wk->timeoutCount == SEARCH_TIMEOUT) {
        GFL_SndStop();
        WorldTrade_ShowFatalError(wk);
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqSearchResultMessage(WorldTradeWork *wk) {
    if (wk->unk12E8 == 0) {
        GFL_SndStop();
        WorldTrade_SubLcdMatchObjAppear(wk, wk->searchResult, 1);
        if (wk->searchResult == 0) {
            Search_FriendViewButtonPrint(wk->infoWin[8], wk->msgManager, 0, &wk->print);
        } else {
            Search_FriendViewButtonPrint(wk->infoWin[8], wk->msgManager, 1, &wk->print);
        }
        wk->subprocessSeq = SEARCH_SEQ_SEARCH_RESULT_MESSAGE_WAIT;
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqSearchResultMessageWait(WorldTradeWork *wk) {
    if (wk->searchResult == 0) {
        Search_SubSeqMessagePrint(wk, 0xe, 1, 0, 0xf0f);
        WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_MAIN);
        GFL_SndSEPlay(SEQ_SE_BEEP);
    } else {
        Search_SubSeqMessagePrint(wk, 0x24, 1, 0, 0xf0f);
        WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_RESULT_TOUCH_MESSAGE);
        wk->wait = 0;
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqResultTouchMessage(WorldTradeWork *wk) {
    if (++wk->wait > 45) {
        Search_SubSeqMessagePrint(wk, 0x26, 1, 0, 0xf0f);
        WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_MAIN);
        wk->subLcdTouchOK = 1;
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqServerQueryFailure(WorldTradeWork *wk) {
    wk->searchBackup.characterNo = 0;
    Search_SubSeqMessagePrint(wk, 0x2c, 1, 0, 0xf0f);
    WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_MAIN);
    GFL_SndSEPlay(SEQ_SE_BEEP);
    wk->searchResult = 0;
    Search_FriendViewButtonPrint(wk->infoWin[8], wk->msgManager, 0, &wk->print);
    return WT_SEQ_MAIN;
}

static int Search_SubSeqErrorDisconnectMessage1(WorldTradeWork *wk) {
    Search_SubSeqMessagePrint(wk, 0xab, 4, 0, 0xf0f);
    WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT_KEY, SEARCH_SEQ_ERROR_DISCONNECT_MES2);
    wk->wait = 0;
    GFL_SndSEPlay(SEQ_SE_BEEP);
    return WT_SEQ_MAIN;
}

static int Search_SubSeqErrorDisconnectMessage2(WorldTradeWork *wk) {
    Search_SubSeqMessagePrint(wk, 0xac, 4, 0, 0xf0f);
    WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT_KEY, SEARCH_SEQ_END_TO_ENTER);
    WorldTrade_SubProcessChange(wk, WORLDTRADE_ENTER, ENTER_MODE_DISCONNECT);
    GFL_SndSEPlay(SEQ_SE_BEEP);
    return WT_SEQ_MAIN;
}

static int Search_CursorPosGet(WorldTradeWork *wk) {
    int pos;

    if (wk->dw->cursorSide == 0) {
        pos = wk->dw->leftCursorPos;
    } else {
        pos = wk->dw->rightCursorPos + 4;
    }
    GFL_ASSERT(pos < 7);
    return pos;
}

static void Search_CursorMove(WorldTradeWork *wk) {
    if (GCTX_HIDGetPressedKeys() & PAD_KEY_UP) {
        if (wk->dw->cursorSide == 0) {
            if (wk->dw->leftCursorPos > 0) {
                wk->dw->leftCursorPos--;
                GFL_SndSEPlay(SEQ_SE_SELECT1);
                func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][2]);
            }
        } else if (wk->dw->rightCursorPos > 0) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            wk->dw->rightCursorPos--;
            func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][2]);
        }
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_DOWN) {
        if (wk->dw->cursorSide == 0) {
            if (wk->dw->leftCursorPos < 3) {
                wk->dw->leftCursorPos++;
                GFL_SndSEPlay(SEQ_SE_SELECT1);
                func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][2]);
            }
        } else if (wk->dw->rightCursorPos < 2) {
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            wk->dw->rightCursorPos++;
            func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][2]);
        }
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_RIGHT) {
        if (wk->dw->cursorSide != 1) {
            wk->dw->cursorSide = 1;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][2]);
        } else {
            wk->dw->cursorSide = 1;
        }
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_LEFT) {
        if (wk->dw->cursorSide != 0) {
            wk->dw->cursorSide = 0;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][2]);
        } else {
            wk->dw->cursorSide = 0;
        }
    }
    WorldTrade_CLACT_PosChange(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][0],
                               sSearchCursorPos[Search_CursorPosGet(wk)][1]);
}

static void Search_TouchCursorMove(WorldTradeWork *wk, int touch) {
    if (sSearchCursorTable[touch][0] == 0) {
        wk->dw->cursorSide = 0;
        wk->dw->leftCursorPos = sSearchCursorTable[touch][1];
    } else {
        wk->dw->cursorSide = 1;
        wk->dw->rightCursorPos = sSearchCursorTable[touch][1];
    }
    WorldTrade_CLACT_PosChange(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][0],
                               sSearchCursorPos[Search_CursorPosGet(wk)][1]);
}

static int Search_SubSeqEnd(WorldTradeWork *wk) {
    if (wk->drawOffset == 0) {
        GFL_WipeSet(3, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
    }
    wk->subprocessSeq = SEARCH_SEQ_START;
    return WT_SEQ_FADEOUT;
}

static int Search_SubSeqEndToEnter(WorldTradeWork *wk) {
    DpwTr_End();
    WorldTrade_TimeIconDel(wk);
    GFL_WipeSet(0, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
    wk->subprocessSeq = SEARCH_SEQ_START;
    return WT_SEQ_FADEOUT;
}

static int Search_SubSeqInputPokenameMessage(WorldTradeWork *wk) {
    Search_SubSeqMessagePrint(wk, 9, 1, 0, 0xf0f);
    WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_POKENAME_SELECT_LIST);
    return WT_SEQ_MAIN;
}

static int Search_SubSeqPokenameSelectList(WorldTradeWork *wk) {
    WorldTrade_Input_Start(wk->inputWork, INPUT_MODE_POKEMON_NAME);
    Search_BgBlendSet(9);
    wk->subprocessSeq = SEARCH_SEQ_POKENAME_SELECT_WAIT;
    return WT_SEQ_MAIN;
}

static int Search_SubSeqPokenameSelectWait(WorldTradeWork *wk) {
    u32 ret = WorldTrade_Input_Main(wk->inputWork);
    void *personal;
    int sexSelection;

    switch (ret) {
    case BMPMENULIST_CANCEL:
        Search_BgBlendSet(0);
        wk->subprocessSeq = SEARCH_SEQ_START;
        WorldTrade_SelectNameListBackup(&wk->selectListPos, wk->dw->headwordListPos + wk->dw->headwordPos,
                                        wk->dw->nameListPos, wk->dw->namePos);
        break;
    case BMPMENULIST_NULL:
        break;
    default:
        Search_BgBlendSet(0);
        wk->search.characterNo = ret;
        wk->subprocessSeq = SEARCH_SEQ_START;
        GFL_BitmapFill(BmpWin_GetBitmap(wk->infoWin[1]), 0);
        WorldTrade_PokeNamePrint(wk->infoWin[1], wk->monsNameManager, ret, 0, 0, SEARCH_INFO_COLOR, &wk->print);
        personal = PML_PersonalLoad(ret, 0, HEAPID_WORLDTRADE);
        sexSelection = PML_PersonalGetParam(personal, 20);
        PML_PersonalFree(personal);
        wk->dw->sexSelection = sexSelection;
        WorldTrade_SelectNameListBackup(&wk->selectListPos, wk->dw->headwordListPos + wk->dw->headwordPos,
                                        wk->dw->nameListPos, wk->dw->namePos);
        if (WorldTrade_SexSelectionCheck(&wk->search, wk->dw->sexSelection)) {
            GFL_BitmapFill(BmpWin_GetBitmap(wk->infoWin[3]), 0);
            WorldTrade_SexPrint(wk->infoWin[3], wk->msgManager, wk->search.gender, 1, 0, 0, SEARCH_INFO_COLOR,
                                &wk->print);
        }
        break;
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqSexSelectMes(WorldTradeWork *wk) {
    Search_SubSeqMessagePrint(wk, 10, 1, 0, 0xf0f);
    WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_SEX_SELECT_LIST);
    return WT_SEQ_MAIN;
}

static int Search_SubSeqSexSelectList(WorldTradeWork *wk) {
    WorldTrade_Input_Start(wk->inputWork, INPUT_MODE_SEX);
    Search_BgBlendSet(9);
    wk->subprocessSeq = SEARCH_SEQ_SEX_SELECT_WAIT;
    return WT_SEQ_MAIN;
}

static int Search_SubSeqSexSelectWait(WorldTradeWork *wk) {
    u32 ret = WorldTrade_Input_Main(wk->inputWork);
    BmpWin *win;

    switch (ret) {
    case BMPMENULIST_CANCEL:
        Search_BgBlendSet(0);
        BmpWin_ClearFrame(wk->msgWin, 0);
        win = wk->msgWin;
        BmpWin_FlushChar(win);
        BmpWin_FlushMap(win);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
        wk->subprocessSeq = SEARCH_SEQ_START;
        break;
    case 0:
    case 1:
    case 2:
        Search_BgBlendSet(0);
        wk->search.gender = ret + 1;
        wk->subprocessSeq = SEARCH_SEQ_START;
        GFL_BitmapFill(BmpWin_GetBitmap(wk->infoWin[3]), 0);
        WorldTrade_SexPrint(wk->infoWin[3], wk->msgManager, wk->search.gender, 1, 0, 0, SEARCH_INFO_COLOR, &wk->print);
        break;
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqLevelSelectMes(WorldTradeWork *wk) {
    Search_SubSeqMessagePrint(wk, 11, 1, 0, 0xf0f);
    WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_LEVEL_SELECT_LIST);
    return WT_SEQ_MAIN;
}

static int Search_SubSeqLevelSelectList(WorldTradeWork *wk) {
    wk->listpos = 0xffff;
    WorldTrade_Input_Start(wk->inputWork, INPUT_MODE_LEVEL);
    Search_BgBlendSet(9);
    wk->subprocessSeq = SEARCH_SEQ_LEVEL_SELECT_WAIT;
    return WT_SEQ_MAIN;
}

static int Search_SubSeqLevelSelectWait(WorldTradeWork *wk) {
    u32 ret = WorldTrade_Input_Main(wk->inputWork);

    switch (ret) {
    case SEARCH_LEVEL_SELECT_NUM:
    case BMPMENULIST_CANCEL:
        Search_BgBlendSet(0);
        wk->subprocessSeq = SEARCH_SEQ_START;
        break;
    case BMPMENULIST_NULL:
        break;
    default:
        Search_BgBlendSet(0);
        WorldTrade_LevelMinMaxSet(&wk->search, ret, LEVEL_PRINT_TBL_SEARCH);
        wk->subprocessSeq = SEARCH_SEQ_START;
        GFL_BitmapFill(BmpWin_GetBitmap(wk->infoWin[5]), 0);
        WorldTrade_WantLevelPrint(wk->infoWin[5], wk->msgManager, ret, 0, 0, SEARCH_INFO_COLOR, LEVEL_PRINT_TBL_SEARCH,
                                  &wk->print);
        break;
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqCountrySelectMes(WorldTradeWork *wk) {
    Search_SubSeqMessagePrint(wk, 0xbe, 1, 0, 0xf0f);
    WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_COUNTRY_SELECT_LIST);
    return WT_SEQ_MAIN;
}

static int Search_SubSeqCountrySelectList(WorldTradeWork *wk) {
    wk->listpos = 0xffff;
    WorldTrade_Input_Start(wk->inputWork, INPUT_MODE_NATION);
    Search_BgBlendSet(9);
    wk->subprocessSeq = SEARCH_SEQ_COUNTRY_SELECT_WAIT;
    return WT_SEQ_MAIN;
}

static int Search_SubSeqCountrySelectWait(WorldTradeWork *wk) {
    u32 ret = WorldTrade_Input_Main(wk->inputWork);
    BmpWin *win;

    if (ret != BMPMENULIST_NULL) {
        if (ret == BMPMENULIST_CANCEL || ret == WorldTrade_CountryListNum + 1) {
            Search_BgBlendSet(0);
            BmpWin_ClearFrame(wk->msgWin, 0);
            win = wk->msgWin;
            BmpWin_FlushChar(win);
            BmpWin_FlushMap(win);
            GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
            wk->subprocessSeq = SEARCH_SEQ_START;
        } else {
            Search_BgBlendSet(0);
            WorldTrade_CountryCodeSet(wk, ret);
            wk->subprocessSeq = SEARCH_SEQ_START;
            GFL_BitmapFill(BmpWin_GetBitmap(wk->countryWin[1]), 0);
            WorldTrade_CountryPrint(wk->countryWin[1], wk->countryNameManager, wk->msgManager, wk->countryCode, 0, 0,
                                    SEARCH_INFO_COLOR, &wk->print);
        }
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqYesNo(WorldTradeWork *wk) {
    WorldTrade_TouchWinYesNoMake(wk, 20, 0x120, 3, TRUE);
    wk->subprocessSeq = SEARCH_SEQ_YESNO_SELECT;
    return WT_SEQ_MAIN;
}

static int Search_SubSeqYesNoSelect(WorldTradeWork *wk) {
    u32 ret = WorldTrade_TouchSwMain(wk);

    if (ret == 1) {
        WorldTrade_TouchWinYesNoDel(wk);
        wk->subprocessSeq = SEARCH_SEQ_END;
        WorldTrade_SubProcessChange(wk, WORLDTRADE_TITLE, 0);
        if (wk->searchResult > 0) {
            WorldTrade_SubLcdMatchObjHide(wk);
        }
        wk->searchResult = 0;
    } else if (ret == 2) {
        WorldTrade_TouchWinYesNoDel(wk);
        wk->subprocessSeq = SEARCH_SEQ_START;
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqToMain(WorldTradeWork *wk) {
    wk->subprocessSeq = SEARCH_SEQ_MAIN;
    return WT_SEQ_MAIN;
}

static int Search_SubSeqSearchErrorMessage(WorldTradeWork *wk) {
    Search_SubSeqMessagePrint(wk, 0x9e, 1, 0, 0xf0f);
    WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_MAIN);
    return WT_SEQ_MAIN;
}

static int Search_SubSeqMessageWait(WorldTradeWork *wk) {
    if (!WorldTrade_PrintIsBusy(&wk->print)) {
        wk->subprocessSeq = wk->subprocessNextSeq;
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqMessageWaitKey(WorldTradeWork *wk) {
    if (!WorldTrade_PrintIsBusy(&wk->print)) {
        if (GCTX_HIDGetPressedKeys() || func_0203da48()) {
            wk->subprocessSeq = wk->subprocessNextSeq;
        }
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqMessageWaitFrames(WorldTradeWork *wk) {
    if (!WorldTrade_PrintIsBusy(&wk->print)) {
        if (++wk->wait > 45) {
            wk->wait = 0;
            wk->subprocessSeq = wk->subprocessNextSeq;
        }
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqExchangeScreen1(WorldTradeWork *wk) {
    wk->drawOffset++;
    if (GFL_WipeIsFinished()) {
        GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
        wk->drawOffset = -16;
        if (wk->subLcdTouchOK) {
            wk->subprocessSeq = SEARCH_SEQ_END;
            WorldTrade_SubProcessChange(wk, WORLDTRADE_PARTNER, PARTNER_MODE_FROM_SEARCH);
            wk->touchTrainerPos = 0;
        }
        func_0204c124(wk->promptDsAct, FALSE);
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqReturnScreen2(WorldTradeWork *wk) {
    wk->drawOffset--;
    if (GFL_WipeIsFinished()) {
        wk->drawOffset = 0;
        wk->subprocessSeq = SEARCH_SEQ_MAIN;
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqExchangeMain(WorldTradeWork *wk) {
    int touch;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        GFL_WipeSet(0, 0, 0, 0, 16, 1, HEAPID_WORLDTRADE);
    } else {
        touch = WorldTrade_SubLcdObjHitCheck(wk->searchResult);
        if (wk->subLcdTouchOK && touch >= 0) {
            func_0204c488(wk->subAct[touch + 1], touch * 4 + 16);
            wk->subprocessSeq = SEARCH_SEQ_END;
            WorldTrade_SubProcessChange(wk, WORLDTRADE_PARTNER, 0);
            wk->touchTrainerPos = touch;
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqCursorDecideWait(WorldTradeWork *wk) {
    if (!func_0204c560(wk->cursorAct)) {
        func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][2]);
        if (func_0203d554() == TRUE) {
            func_0204c124(wk->cursorAct, FALSE);
        } else {
            func_0204c124(wk->cursorAct, TRUE);
        }
        Search_DecideFunc(wk, Search_CursorPosGet(wk));
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqCursorQuitWait(WorldTradeWork *wk) {
    if (!func_0204c560(wk->cursorAct)) {
        func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][2]);
        if (func_0203d554() == TRUE) {
            func_0204c124(wk->cursorAct, FALSE);
        } else {
            func_0204c124(wk->cursorAct, TRUE);
        }
        Search_SubSeqMessagePrint(wk, 0xf, 1, 0, 0xf0f);
        WorldTrade_SetNextSeq(wk, SEARCH_SEQ_MES_WAIT, SEARCH_SEQ_YESNO);
    }
    return WT_SEQ_MAIN;
}

static int Search_SubSeqCursorViewWait(WorldTradeWork *wk) {
    if (!func_0204c560(wk->cursorAct)) {
        func_0204c488(wk->cursorAct, sSearchCursorPos[Search_CursorPosGet(wk)][2]);
        if (func_0203d554() == TRUE) {
            func_0204c124(wk->cursorAct, FALSE);
        } else {
            func_0204c124(wk->cursorAct, TRUE);
        }
        wk->subprocessSeq = SEARCH_SEQ_MAIN;
    }
    return WT_SEQ_MAIN;
}

static void Search_SubSeqMessagePrint(WorldTradeWork *wk, int msgNo, int wait, int flag, u16 dat) {
    BmpWin *win;

    GFL_MsgDataLoadStrbuf(wk->msgManager, msgNo, wk->talkString);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWin), 15);
    BmpWin_DrawFrame(wk->msgWin, 0, 1, 14);
    WorldTrade_Print(wk->msgWin, 0, wk->talkString, 0, 0, &wk->print);
    win = wk->msgWin;
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
}

static void Search_WantLabelPrint(BmpWin **win, BmpWin **countryWin, MsgData *msgManager, WorldTradePrint *print) {
    StrBuf *str;

    str = GFL_MsgDataLoadStrbufNew(msgManager, 0x43);
    WorldTrade_SysPrint(win[0], str, 0, 0, 0, SEARCH_LABEL_COLOR, print);
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(msgManager, 0x45);
    WorldTrade_SysPrint(win[2], str, 0, 0, 0, SEARCH_LABEL_COLOR, print);
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(msgManager, 0x47);
    WorldTrade_SysPrint(win[4], str, 0, 0, 0, SEARCH_LABEL_COLOR, print);
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(msgManager, 0xb9);
    WorldTrade_SysPrint(countryWin[0], str, 0, 0, 0, SEARCH_LABEL_COLOR, print);
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(msgManager, 0x49);
    WorldTrade_TouchPrint(win[6], str, 0, 0, 0, SEARCH_LABEL_COLOR, print);
    GFL_StrBufFree(str);
    str = GFL_MsgDataLoadStrbufNew(msgManager, 0xfc);
    WorldTrade_TouchPrint(win[7], str, 0, 0, 0, SEARCH_LABEL_COLOR, print);
    GFL_StrBufFree(str);
}

// The button that shows the trainers found, lit only when there are some
static void Search_FriendViewButtonPrint(BmpWin *win, MsgData *msgManager, int flag, WorldTradePrint *print) {
    StrBuf *str;
    u16 color = SEARCH_INFO_COLOR;
    u8 palette;

    if (flag) {
        color = SEARCH_LABEL_COLOR;
        palette = 0;
    } else {
        palette = 2;
    }
    GFL_BGSysSetScrPaletteNo(1, 17, 2, 15, 4, palette);
    GFL_BGSysLoadScr(1);
    str = GFL_MsgDataLoadStrbufNew(msgManager, 0x4b);
    WorldTrade_TouchPrint(win, str, 0, 0, 0, color, print);
    GFL_StrBufFree(str);
}

// Whether a search is the same as the last one
static BOOL Search_DpwSearchCompare(const Dpw_Tr_PokemonSearchData *s1, const Dpw_Tr_PokemonSearchData *s2,
                                    int countryCode1, int countryCode2) {
    if (s1->characterNo == s2->characterNo && s1->gender == s2->gender && s1->level_min == s2->level_min &&
        s1->level_max == s2->level_max && countryCode1 == countryCode2) {
        return TRUE;
    }
    return FALSE;
}

static void Search_SlideScreenVFunc(WorldTradeWork *wk) {
    GFL_BGSysMoveBG(0, BG_MOVE_SET_Y, wk->drawOffset);
    GFL_BGSysMoveBG(1, BG_MOVE_SET_Y, wk->drawOffset);
    GFL_BGSysMoveBG(2, BG_MOVE_SET_Y, wk->drawOffset);
    GFL_BGSysMoveBG(3, BG_MOVE_SET_Y, wk->drawOffset);
    GFL_BGSysMoveBG(4, BG_MOVE_SET_Y, -wk->drawOffset);
    GFL_BGSysMoveBG(5, BG_MOVE_SET_Y, -wk->drawOffset);
    GFL_BGSysMoveBG(6, BG_MOVE_SET_Y, -wk->drawOffset);
    GFL_BGSysMoveBG(7, BG_MOVE_SET_Y, -wk->drawOffset);
}

// Darkens the main screen behind the input
static void Search_BgBlendSet(int rate) {
    if (rate) {
        gfxRegSetBrightnessBlend(REG_BLDCNT_ADDR, GX_BLEND_PLANEMASK_BG1 | GX_BLEND_PLANEMASK_BG3, rate);
    } else {
        G2_BlendNone();
    }
}
