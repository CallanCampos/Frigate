#include "types.h"
#include "constants/arc.h"
#include "constants/pokemon.h"
#include "constants/species.h"
#include "dpw/dpw_tr.h"
#include "gfl/arc.h"
#include "gfl/arc_util.h"
#include "gfl/bg_sys.h"
#include "gfl/bmp.h"
#include "gfl/bmpwin.h"
#include "gfl/clact.h"
#include "gfl/graphics.h"
#include "gfl/gx_layers.h"
#include "gfl/heap.h"
#include "gfl/key.h"
#include "gfl/msg.h"
#include "gfl/sound.h"
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "nitro/os.h"
#include "nnsys/g2d.h"
#include "pml/item.h"
#include "pml/poke_party.h"
#include "save/box.h"
#include "system/bmp_menulist.h"
#include "system/bmp_winframe.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "worldtrade_local.h"

// The Global Trade Station's box screen, where the player picks a Pokémon of the party or the boxes to deposit or to
// offer in a trade. The names are ours, guessed

// The cursor's positions past the page's 30 Pokémon
#define BOX_CURSOR_END_POS 30
#define BOX_CURSOR_TRAY_POS 31

// The sub-process modes the screen is entered with
#define BOX_MODE_DEPOSIT_SELECT 5
#define BOX_MODE_EXCHANGE_SELECT 6

// The box page of the party
#define BOX_TRAY_PARTY 0xff

// A search's gender that takes either
#define SEARCH_GENDER_ANY 3

enum {
    BOX_SEQ_START,
    BOX_SEQ_MAIN,
    BOX_SEQ_END,
    BOX_SEQ_MESSAGE_WAIT,
    BOX_SEQ_MESSAGE_CLEAR_WAIT,
    BOX_SEQ_YESNO,
    BOX_SEQ_YESNO_SELECT,
    BOX_SEQ_SELECT_LIST,
    BOX_SEQ_SELECT_WAIT,
    BOX_SEQ_EXCHANGE_SELECT_LIST,
    BOX_SEQ_EXCHANGE_SELECT_WAIT,
    BOX_SEQ_CBALL_YESNO_MESSAGE,
    BOX_SEQ_CBALL_YESNO,
    BOX_SEQ_CBALL_YESNO_SELECT,
    BOX_SEQ_CBALL_DEPOSIT_YESNO_MESSAGE,
    BOX_SEQ_CBALL_DEPOSIT_YESNO,
    BOX_SEQ_CBALL_DEPOSIT_YESNO_SELECT,
    BOX_SEQ_CANCEL_WAIT,
};

typedef struct {
    u16 x;
    u16 y;
} BoxPos;

static void Box_BgInit(void);
static void Box_BgExit(void);
static void Box_BgGraphicSet(WorldTradeWork *wk);
static void Box_SetCellActor(WorldTradeWork *wk);
static void Box_DelCellActor(WorldTradeWork *wk);
static void Box_BmpWinInit(WorldTradeWork *wk);
static void Box_BmpWinDelete(WorldTradeWork *wk);
static void Box_InitWork(WorldTradeWork *wk);
static void Box_FreeWork(WorldTradeWork *wk);
static int Box_SubSeqStart(WorldTradeWork *wk);
static void Box_DepositDecideFunc(WorldTradeWork *wk);
static void Box_ExchangeDecideFunc(WorldTradeWork *wk);
static int Box_TouchFunc(WorldTradeWork *wk);
static void Box_CancelFunc(WorldTradeWork *wk, int mode);
static int Box_SubSeqMain(WorldTradeWork *wk);
static void Box_CursorControl(WorldTradeWork *wk);
static void Box_CursorPosPrioritySet(ClActor *cursor, int pos);
static int Box_RoundWork(int num, int max, int move);
static int Box_SubSeqSelectList(WorldTradeWork *wk);
static int Box_SubSeqSelectWait(WorldTradeWork *wk);
static int Box_SubSeqExchangeSelectList(WorldTradeWork *wk);
static int Box_SubSeqExchangeSelectWait(WorldTradeWork *wk);
static int Box_SubSeqEnd(WorldTradeWork *wk);
static int Box_SubSeqYesNo(WorldTradeWork *wk);
static int Box_SubSeqYesNoSelect(WorldTradeWork *wk);
static int Box_SubSeqCBallYesNoMessage(WorldTradeWork *wk);
static int Box_SubSeqCBallYesNo(WorldTradeWork *wk);
static int Box_SubSeqCBallYesNoSelect(WorldTradeWork *wk);
static int Box_ExchangeCheck(WorldTradeWork *wk);
static int Box_SubSeqCBallDepositYesNoMessage(WorldTradeWork *wk);
static int Box_SubSeqCBallDepositYesNo(WorldTradeWork *wk);
static int Box_SubSeqCBallDepositYesNoSelect(WorldTradeWork *wk);
static int Box_SubSeqCancelWait(WorldTradeWork *wk);
static int Box_SubSeqMessageWait(WorldTradeWork *wk);
static int Box_SubSeqMessageClearWait(WorldTradeWork *wk);
static void Box_SubSeqMessagePrint(WorldTradeWork *wk, int msgNo, int wait, int flag, u16 dat, int winFlag);
static void *Box_CharDataGetbyHandle(ArcTool *handle, u32 dataIdx, NNSG2dCharacterData **charData, u32 heapId);
static void Box_TransPokeIconCharaPal(int species, int form, int sex, int egg, int no, ClActor *icon, ArcTool *handle,
                                      WorldTradePokeBuf *pbuf);
static void Box_PokemonLevelSet(BoxPkm *pkm, Dpw_Tr_PokemonDataSimple *dat);
static void Box_PokemonIconDraw(WorldTradeWork *wk);
static void Box_PokemonIconSet(BoxPkm *pkm, ClActor *icon, ClActor *itemAct, u16 *no, int pos, ArcTool *handle,
                               Dpw_Tr_PokemonDataSimple *dat, WorldTradePokeBuf *pbuf);
static void Box_NowBoxPageInfoGet(WorldTradeWork *wk, int now);
static BOOL Box_CheckPocket(PokeParty *party, BoxSaveAccessor *box, int tray, int pos);
static u32 Box_PokeRibbonCheck(BoxPkm *pkm);
static BOOL Box_PokeNewItemCheck(BoxPkm *pkm);
static int Box_PokemonCheck(PokeParty *party, BoxSaveAccessor *box, int tray, int pos);
static BOOL Box_CompareSearchData(Dpw_Tr_PokemonDataSimple *poke, Dpw_Tr_PokemonSearchData *search);
static BOOL Box_WantPokeCheck(BoxPkm *pkm, Dpw_Tr_PokemonSearchData *want);
static void Box_MakeExchangePokemonData(Dpw_Tr_Data *dtd, WorldTradeWork *wk);
static void Box_PokeIconPalSet(Dpw_Tr_PokemonDataSimple *box, ClActor **icons, Dpw_Tr_PokemonSearchData *want,
                               WorldTradePokeBuf *pbuf);
static void Box_BoxCountCheck(WorldTradeWork *wk);

static int (*sBoxSubSeqTable[])(WorldTradeWork *wk) = {
    Box_SubSeqStart,
    Box_SubSeqMain,
    Box_SubSeqEnd,
    Box_SubSeqMessageWait,
    Box_SubSeqMessageClearWait,
    Box_SubSeqYesNo,
    Box_SubSeqYesNoSelect,
    Box_SubSeqSelectList,
    Box_SubSeqSelectWait,
    Box_SubSeqExchangeSelectList,
    Box_SubSeqExchangeSelectWait,
    Box_SubSeqCBallYesNoMessage,
    Box_SubSeqCBallYesNo,
    Box_SubSeqCBallYesNoSelect,
    Box_SubSeqCBallDepositYesNoMessage,
    Box_SubSeqCBallDepositYesNo,
    Box_SubSeqCBallDepositYesNoSelect,
    Box_SubSeqCancelWait,
};

// Where the cursor and the Pokémon icons sit, for each position of the cursor
static const BoxPos sCursorPos[] = {
    { 19, 36 },  { 45, 36 },  { 71, 36 },  { 97, 36 },  { 123, 36 },  { 149, 36 },  { 19, 59 },   { 45, 59 },
    { 71, 59 },  { 97, 59 },  { 123, 59 }, { 149, 59 }, { 19, 82 },   { 45, 82 },   { 71, 82 },   { 97, 82 },
    { 123, 82 }, { 149, 82 }, { 19, 105 }, { 45, 105 }, { 71, 105 },  { 97, 105 },  { 123, 105 }, { 149, 105 },
    { 19, 128 }, { 45, 128 }, { 71, 128 }, { 97, 128 }, { 123, 128 }, { 149, 128 }, { 224, 135 }, { 82, 16 },
};

int WorldTrade_Box_Init(WorldTradeWork *wk, int seq) {
    Box_InitWork(wk);
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    Box_BgInit();
    WorldTrade_SubLcdBgInit(wk, 0, 0);
    Box_BgGraphicSet(wk);
    Box_BmpWinInit(wk);
    Box_SetCellActor(wk);
    WorldTrade_SetPartnerExchangePos(wk);

    if (gfxRegGetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR) == 0) {
        GFL_WipeSet(3, 1, 1, 0, 6, 1, HEAPID_WORLDTRADE);
    } else {
        GFL_WipeSet(0, 1, 1, 0, 6, 1, HEAPID_WORLDTRADE);
    }

    Box_NowBoxPageInfoGet(wk, wk->boxTrayNo);
    WorldTrade_WifiIconAdd(wk);
    wk->subprocessSeq = BOX_SEQ_START;
    wk->subLcdBgKeep = 0;
    return WT_SEQ_FADEIN;
}

int WorldTrade_Box_Main(WorldTradeWork *wk, int seq) {
    return sBoxSubSeqTable[wk->subprocessSeq](wk);
}

int WorldTrade_Box_End(WorldTradeWork *wk, int seq) {
    if (gfxRegGetMasterBrightness(REG_DB_MASTER_BRIGHT_ADDR) != 0) {
        WorldTrade_SetPartnerExchangePosIsReturns(wk);
    }
    Box_DelCellActor(wk);
    Box_FreeWork(wk);
    Box_BmpWinDelete(wk);
    Box_BgExit();
    WorldTrade_SubLcdBgExit(wk);
    func_0204c124(wk->promptDsAct, FALSE);
    WorldTrade_SubProcessUpdate(wk);
    return WT_SEQ_INIT;
}

static void Box_BgInit(void) {
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
            1,
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
            GX_BG_CHARBASE(0x08000),
            0x8000,
            GX_BG_EXTPLTT_01,
            2,
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

static void Box_BgExit(void) {
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
    GFL_BGSysReleaseBG(3);
}

static void Box_BgGraphicSet(WorldTradeWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_WORLDTRADE, HEAPID_WORLDTRADE);

    GFL_G2DIOLoadArcNCLRDefault(arc, 1, 0, 0, 0x60, HEAPID_WORLDTRADE);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1a0, 0x20, HEAPID_WORLDTRADE);
    LoadSysMsgBox(0, 1, 14, 0, HEAPID_WORLDTRADE);
    LoadSysMsgBox(0, 31, 11, 0, HEAPID_WORLDTRADE);
    GFL_BGSysLoadArcNCGRStatic(arc, 10, 1, 0, 0xa00, TRUE, HEAPID_WORLDTRADE);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 21, 1, 0, 0x600, TRUE, HEAPID_WORLDTRADE);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 31, 2, 0, 0x600, TRUE, HEAPID_WORLDTRADE);
    GFL_ArcToolFree(arc);

    WorldTrade_SubLcdBgGraphicSet(wk);
    WorldTrade_SubLcdWinGraphicSet(wk);
}

// The arrows by the box name
static const BoxPos sBoxArrowPos[] = {
    { 154, 12 },
    { 14, 12 },
};

static void Box_SetCellActor(WorldTradeWork *wk) {
    ClActorSetup setup;
    int i;

    sys_memset(&setup, 0, sizeof(ClActorSetup));
    setup.x = sCursorPos[wk->boxCursorPos].x;
    setup.y = sCursorPos[wk->boxCursorPos].y;
    wk->cursorAct = func_0204c040(wk->clactUnit, wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CHAR],
                                  wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT],
                                  wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CELL], &setup, 0, HEAPID_WORLDTRADE);
    func_0204c520(wk->cursorAct, TRUE);
    func_0204c488(wk->cursorAct, 4);
    if (func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, FALSE);
    } else {
        func_0204c124(wk->cursorAct, TRUE);
    }
    if (wk->boxCursorPos == BOX_CURSOR_TRAY_POS || (wk->boxCursorPos >= 0 && wk->boxCursorPos <= 5)) {
        func_0204c468(wk->cursorAct, 0);
    } else {
        func_0204c468(wk->cursorAct, 1);
    }

    for (i = 0; i < BOX_POKE_NUM; i++) {
        setup.x = sCursorPos[i].x;
        setup.y = sCursorPos[i].y;
        setup.priority = 30 - i % 6;
        wk->pokeIconAct[i] =
            func_0204c040(wk->clactUnit, wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CHAR],
                          wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT],
                          wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CELL], &setup, 0, HEAPID_WORLDTRADE);
        func_0204c488(wk->pokeIconAct[i], i + 6);
        func_0204c468(wk->pokeIconAct[i], 1);
    }
    for (i = 0; i < BOX_POKE_NUM; i++) {
        setup.x = sCursorPos[i].x;
        setup.y = sCursorPos[i].y + 6;
        setup.priority = 10;
        wk->itemIconAct[i] =
            func_0204c040(wk->clactUnit, wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CHAR],
                          wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT],
                          wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CELL], &setup, 0, HEAPID_WORLDTRADE);
        func_0204c488(wk->itemIconAct[i], 40);
        func_0204c468(wk->itemIconAct[i], 1);
    }
    for (i = 0; i < 6; i++) {
        setup.x = sCursorPos[i].x + 8;
        setup.y = sCursorPos[i].y + 6;
        setup.priority = 10;
        wk->cballAct[i] =
            func_0204c040(wk->clactUnit, wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CHAR],
                          wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT],
                          wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CELL], &setup, 0, HEAPID_WORLDTRADE);
        func_0204c488(wk->cballAct[i], 42);
        func_0204c468(wk->cballAct[i], 1);
    }
    for (i = 0; i < 2; i++) {
        setup.x = sBoxArrowPos[i].x;
        setup.y = sBoxArrowPos[i].y;
        wk->boxArrowAct[i] =
            func_0204c040(wk->clactUnit, wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CHAR],
                          wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT],
                          wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CELL], &setup, 0, HEAPID_WORLDTRADE);
        func_0204c488(wk->boxArrowAct[i], i + 38);
        func_0204c468(wk->boxArrowAct[i], 1);
    }

    setup.x = 239;
    setup.y = 136;
    setup.priority = 100;
    setup.bgPriority = 1;
    wk->fingerAct = func_0204c040(wk->clactUnit, wk->clactRes[WT_CLACT_RES_MAIN2][WT_CLACT_RES_CHAR],
                                  wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT],
                                  wk->clactRes[WT_CLACT_RES_MAIN2][WT_CLACT_RES_CELL], &setup, 0, HEAPID_WORLDTRADE);
    func_0204c488(wk->fingerAct, 12);
    func_0204c520(wk->fingerAct, TRUE);
    func_0204c550(wk->fingerAct);
    func_0204c124(wk->fingerAct, FALSE);

    func_0204c124(wk->promptDsAct, TRUE);
    WorldTrade_ActPos(wk->promptDsAct, 55, 168);
}

static void Box_DelCellActor(WorldTradeWork *wk) {
    int i;

    func_0204c108(wk->fingerAct);
    for (i = 0; i < 2; i++) {
        func_0204c108(wk->boxArrowAct[i]);
    }
    func_0204c108(wk->cursorAct);
    for (i = 0; i < BOX_POKE_NUM; i++) {
        func_0204c108(wk->pokeIconAct[i]);
        func_0204c108(wk->itemIconAct[i]);
    }
    for (i = 0; i < 6; i++) {
        func_0204c108(wk->cballAct[i]);
    }
}

static void Box_BmpWinInit(WorldTradeWork *wk) {
    BmpWin *win;

    wk->subWin = BmpWin_CreateDynamic(3, 4, 0, 13, 3, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->subWin), 0);
    win = wk->subWin;
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));

    wk->msgWin = BmpWin_CreateDynamic(0, 2, 21, 27, 2, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWin), 0);
    win = wk->msgWin;
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));

    wk->talkWin = BmpWin_CreateDynamic(0, 2, 19, 27, 4, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->talkWin), 0);
    // BUG: Shows the message window again instead of the talk window
#ifdef BUGFIX
    win = wk->talkWin;
#else
    win = wk->msgWin;
#endif
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));

    wk->menuWin[1] = BmpWin_CreateDynamic(1, 24, 16, 6, 2, 0, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[1]), 4);
    win = wk->menuWin[1];
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
    WorldTrade_SysPrint(wk->menuWin[1], wk->endString, 0, 1, 1, 0x3dc4, &wk->print);

    if (wk->subProcessMode == BOX_MODE_DEPOSIT_SELECT) {
        WorldTrade_SubLcdExplainPut(wk, 3);
    } else {
        WorldTrade_SubLcdExplainPut(wk, 1);
    }
}

static void Box_BmpWinDelete(WorldTradeWork *wk) {
    WorldTrade_PrintClear(&wk->print);
    BmpWin_Free(wk->explainWin);
    BmpWin_Free(wk->talkWin);
    BmpWin_Free(wk->menuWin[1]);
    BmpWin_Free(wk->msgWin);
    BmpWin_Free(wk->subWin);
}

static void Box_InitWork(WorldTradeWork *wk) {
    Box_BoxCountCheck(wk);
    wk->boxTrayNameString = GFL_StrBufCreate(18, HEAPID_WORLDTRADE);
    wk->talkString = GFL_StrBufCreate(180, HEAPID_WORLDTRADE);
    wk->endString = GFL_MsgDataLoadStrbufNew(wk->msgManager, 0x73);
    if (wk->boxCursorPos == BOX_CURSOR_END_POS) {
        wk->boxCursorPos = 0;
    }
    wk->boxWork = GFL_HeapAllocate(HEAPID_WORLDTRADE, sizeof(Dpw_Tr_PokemonDataSimple) * BOX_POKE_NUM, FALSE,
                                   "worldtrade_box.c", 836);
}

static void Box_FreeWork(WorldTradeWork *wk) {
    GFL_HeapFree(wk->boxWork);
    GFL_StrBufFree(wk->boxTrayNameString);
    GFL_StrBufFree(wk->talkString);
    GFL_StrBufFree(wk->endString);
}

static int Box_SubSeqStart(WorldTradeWork *wk) {
    int msgNo;

    if (GFL_WipeIsFinished()) {
        if (wk->subProcessMode == BOX_MODE_DEPOSIT_SELECT) {
            msgNo = 0x15;
        } else if (wk->subProcessMode == BOX_MODE_EXCHANGE_SELECT) {
            msgNo = 0x11;
        }
        Box_SubSeqMessagePrint(wk, msgNo, 1, 0, 0xf0f, 0);
        WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_WAIT, BOX_SEQ_MAIN);
        if (func_0203d554() == TRUE) {
            func_0204c124(wk->cursorAct, FALSE);
        } else {
            func_0204c124(wk->cursorAct, TRUE);
        }
    }
    return WT_SEQ_MAIN;
}

static void Box_DepositDecideFunc(WorldTradeWork *wk) {
    if (wk->boxCursorPos == BOX_CURSOR_END_POS) {
        func_0204c124(wk->fingerAct, TRUE);
        func_0204c56c(wk->fingerAct);
        wk->subprocessSeq = BOX_SEQ_CANCEL_WAIT;
        GFL_SndSEPlay(0x551);
    } else if (wk->boxCursorPos != BOX_CURSOR_TRAY_POS) {
        switch (Box_PokemonCheck(wk->param->myparty, wk->param->mybox, wk->boxTrayNo, wk->boxCursorPos)) {
        case 1:
            GFL_SndSEPlay(0x54c);
            if (Box_CheckPocket(wk->param->myparty, wk->param->mybox, wk->boxTrayNo, wk->boxCursorPos)) {
                WorldTrade_SetBoxPkmNickname(
                    wk->wordSet, 0,
                    WorldTrade_GetPokePtr(wk->param->myparty, wk->param->mybox, wk->boxTrayNo, wk->boxCursorPos));
                Box_SubSeqMessagePrint(wk, 0x16, 1, 0, 0xf0f, 0);
                WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_WAIT, BOX_SEQ_SELECT_LIST);
            } else {
                Box_SubSeqMessagePrint(wk, 0x1a, 1, 0, 0xf0f, 1);
                WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_CLEAR_WAIT, BOX_SEQ_MAIN);
            }
            break;
        case 2:
            GFL_SndSEPlay(0x54c);
            Box_SubSeqMessagePrint(wk, 0x21, 1, 0, 0xf0f, 1);
            WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_CLEAR_WAIT, BOX_SEQ_MAIN);
            break;
        }
    }
}

static void Box_ExchangeDecideFunc(WorldTradeWork *wk) {
    BoxPkm *pkm;

    if (wk->boxCursorPos == BOX_CURSOR_END_POS) {
        func_0204c124(wk->fingerAct, TRUE);
        func_0204c56c(wk->fingerAct);
        wk->subprocessSeq = BOX_SEQ_CANCEL_WAIT;
        GFL_SndSEPlay(0x551);
    } else if (wk->boxCursorPos != BOX_CURSOR_TRAY_POS) {
        switch (Box_PokemonCheck(wk->param->myparty, wk->param->mybox, wk->boxTrayNo, wk->boxCursorPos)) {
        case 1:
            pkm = WorldTrade_GetPokePtr(wk->param->myparty, wk->param->mybox, wk->boxTrayNo, wk->boxCursorPos);
            if (Box_WantPokeCheck(pkm, &wk->downloadPokemonData[wk->touchTrainerPos].wantSimple)) {
                if (Box_CheckPocket(wk->param->myparty, wk->param->mybox, wk->boxTrayNo, wk->boxCursorPos)) {
                    WorldTrade_SetBoxPkmNickname(wk->wordSet, 0, pkm);
                    Box_SubSeqMessagePrint(wk, 0x12, 1, 0, 0xf0f, 0);
                    WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_WAIT, BOX_SEQ_EXCHANGE_SELECT_LIST);
                    GFL_SndSEPlay(0x54c);
                } else {
                    Box_SubSeqMessagePrint(wk, 0x1a, 1, 0, 0xf0f, 1);
                    WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_CLEAR_WAIT, BOX_SEQ_MAIN);
                }
            } else {
                GFL_SndSEPlay(0x551);
            }
            break;
        case 0:
        case 2:
            GFL_SndSEPlay(0x551);
            break;
        }
    }
}

// The touch areas of the page's Pokémon, then the Quit button and the arrows
static const TouchRect sBoxTouchRects[] = {
    { 23, 49, 8, 30 },      { 23, 49, 34, 56 },   { 23, 49, 60, 82 },    { 23, 49, 86, 108 },    { 23, 49, 112, 134 },
    { 23, 49, 138, 160 },   { 46, 72, 8, 30 },    { 46, 72, 34, 56 },    { 46, 72, 60, 82 },     { 46, 72, 86, 108 },
    { 46, 72, 112, 134 },   { 46, 72, 138, 160 }, { 69, 95, 8, 30 },     { 69, 95, 34, 56 },     { 69, 95, 60, 82 },
    { 69, 95, 86, 108 },    { 69, 95, 112, 134 }, { 69, 95, 138, 160 },  { 92, 118, 8, 30 },     { 92, 118, 34, 56 },
    { 92, 118, 60, 82 },    { 92, 118, 86, 108 }, { 92, 118, 112, 134 }, { 92, 118, 138, 160 },  { 115, 141, 8, 30 },
    { 115, 141, 34, 56 },   { 115, 141, 60, 82 }, { 115, 141, 86, 108 }, { 115, 141, 112, 134 }, { 115, 141, 138, 160 },
    { 123, 148, 186, 245 }, { 4, 28, 146, 162 },  { 4, 28, 10, 26 },     { 0xff, 0, 0, 0 },
};

static int Box_TouchFunc(WorldTradeWork *wk) {
    int ret = func_0203da0c(sBoxTouchRects);

    if (ret != -1) {
        func_0204c124(wk->cursorAct, FALSE);
        func_0203d564(TRUE);
    }
    return ret;
}

static void Box_CancelFunc(WorldTradeWork *wk, int mode) {
    if (mode == BOX_MODE_DEPOSIT_SELECT) {
        WorldTrade_SubProcessChange(wk, WORLDTRADE_TITLE, 0);
        wk->subprocessSeq = BOX_SEQ_END;
    } else if (mode == BOX_MODE_EXCHANGE_SELECT) {
        WorldTrade_SubProcessChange(wk, WORLDTRADE_PARTNER, 0x11);
        wk->subprocessSeq = BOX_SEQ_END;
    }
}

static int Box_SubSeqMain(WorldTradeWork *wk) {
    int touch = Box_TouchFunc(wk);

    if (touch != -1) {
        switch (touch) {
        case 31:
            func_0204c520(wk->boxArrowAct[0], TRUE);
            func_0204c488(wk->boxArrowAct[0], 38);
            wk->boxTrayNo = Box_RoundWork(wk->boxTrayNo, wk->boxCount, 1);
            Box_NowBoxPageInfoGet(wk, wk->boxTrayNo);
            GFL_SndSEPlay(0x548);
            break;
        case 32:
            func_0204c520(wk->boxArrowAct[1], TRUE);
            func_0204c488(wk->boxArrowAct[1], 39);
            wk->boxTrayNo = Box_RoundWork(wk->boxTrayNo, wk->boxCount, -1);
            Box_NowBoxPageInfoGet(wk, wk->boxTrayNo);
            GFL_SndSEPlay(0x548);
            break;
        case BOX_CURSOR_END_POS:
            func_0204c124(wk->fingerAct, TRUE);
            func_0204c56c(wk->fingerAct);
            wk->subprocessSeq = BOX_SEQ_CANCEL_WAIT;
            wk->boxCursorPos = touch;
            GFL_SndSEPlay(0x551);
            Box_CursorPosPrioritySet(wk->cursorAct, wk->boxCursorPos);
            break;
        default:
            wk->boxCursorPos = touch;
            Box_CursorPosPrioritySet(wk->cursorAct, wk->boxCursorPos);
            if (wk->subProcessMode == BOX_MODE_DEPOSIT_SELECT) {
                Box_DepositDecideFunc(wk);
            } else if (wk->subProcessMode == BOX_MODE_EXCHANGE_SELECT) {
                Box_ExchangeDecideFunc(wk);
            }
            break;
        }
    } else {
        Box_CursorControl(wk);
        if (wk->subProcessMode == BOX_MODE_DEPOSIT_SELECT) {
            if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
                func_0204c124(wk->fingerAct, TRUE);
                func_0204c56c(wk->fingerAct);
                wk->subprocessSeq = BOX_SEQ_CANCEL_WAIT;
                GFL_SndSEPlay(0x551);
            } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
                Box_DepositDecideFunc(wk);
            }
        } else if (wk->subProcessMode == BOX_MODE_EXCHANGE_SELECT) {
            if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
                func_0204c124(wk->fingerAct, TRUE);
                func_0204c56c(wk->fingerAct);
                wk->subprocessSeq = BOX_SEQ_CANCEL_WAIT;
                GFL_SndSEPlay(0x551);
            } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
                Box_ExchangeDecideFunc(wk);
            }
        }
    }
    return WT_SEQ_MAIN;
}

// Where the cursor goes from each position, up, down, left and right. 99 and 101 turn the page
static const u8 sCursorMoveTable[][4] = {
    { 31, 6, 5, 1 },    { 31, 7, 0, 2 },    { 31, 8, 1, 3 },    { 31, 9, 2, 4 },    { 31, 10, 3, 5 },
    { 31, 11, 4, 0 },   { 0, 12, 11, 7 },   { 1, 13, 6, 8 },    { 2, 14, 7, 9 },    { 3, 15, 8, 10 },
    { 4, 16, 9, 11 },   { 5, 17, 10, 6 },   { 6, 18, 17, 13 },  { 7, 19, 12, 14 },  { 8, 20, 13, 15 },
    { 9, 21, 14, 16 },  { 10, 22, 15, 17 }, { 11, 23, 16, 12 }, { 12, 24, 23, 19 }, { 13, 25, 18, 20 },
    { 14, 26, 19, 21 }, { 15, 27, 20, 22 }, { 16, 28, 21, 23 }, { 17, 29, 22, 18 }, { 18, 31, 30, 25 },
    { 19, 31, 24, 26 }, { 20, 31, 25, 27 }, { 21, 31, 26, 28 }, { 22, 31, 27, 29 }, { 23, 31, 28, 30 },
    { 30, 30, 29, 24 }, { 26, 2, 99, 101 },
};

static void Box_CursorControl(WorldTradeWork *wk) {
    BOOL moved = FALSE;
    int move = 0;

    if (GCTX_HIDGetPressedKeys() != 0 && func_0203d554() == TRUE) {
        func_0204c124(wk->cursorAct, TRUE);
        func_0203d564(FALSE);
        GFL_SndSEPlay(0x548);
        return;
    }

    if (GCTX_HIDGetPressedKeys() & PAD_KEY_UP) {
        move = 1;
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_DOWN) {
        move = 2;
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_LEFT) {
        move = 3;
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_RIGHT) {
        move = 4;
    }

    if (move != 0) {
        u8 next = sCursorMoveTable[wk->boxCursorPos][move - 1];

        if (next != wk->boxCursorPos) {
            if (next == 99 || next == 101) {
                u8 arrow = next == 101 ? 0 : 1;

                func_0204c520(wk->boxArrowAct[arrow], TRUE);
                func_0204c488(wk->boxArrowAct[arrow], arrow + 38);
                wk->boxTrayNo = Box_RoundWork(wk->boxTrayNo, wk->boxCount, next - 100);
                Box_NowBoxPageInfoGet(wk, wk->boxTrayNo);
                GFL_SndSEPlay(0x548);
            } else {
                moved = TRUE;
                wk->boxCursorPos = next;
            }
        }
    }
    if (moved) {
        GFL_SndSEPlay(0x548);
    }
    Box_CursorPosPrioritySet(wk->cursorAct, wk->boxCursorPos);
}

static void Box_CursorPosPrioritySet(ClActor *cursor, int pos) {
    WorldTrade_CLACT_PosChange(cursor, sCursorPos[pos].x, sCursorPos[pos].y);
    if (pos == BOX_CURSOR_TRAY_POS || (pos >= 0 && pos <= 5)) {
        func_0204c468(cursor, 0);
    } else {
        func_0204c468(cursor, 1);
    }
}

// Turns the page from num by move, through the party's page between the last box and the first
static int Box_RoundWork(int num, int max, int move) {
    if (num == BOX_TRAY_PARTY) {
        if (move > 0) {
            return 0;
        } else if (move < 0) {
            return max - 1;
        }
    } else {
        num += move;
        if (num < 0) {
            return BOX_TRAY_PARTY;
        }
        if (num == max) {
            return BOX_TRAY_PARTY;
        }
    }
    return num;
}

static int Box_SubSeqSelectList(WorldTradeWork *wk) {
    wk->menuList = ListMenuCore_CreateOptionList(3, HEAPID_WORLDTRADE);
    ListMenuCore_AppendMsgOption(wk->menuList, wk->msgManager, 0x68, 1, HEAPID_WORLDTRADE);
    ListMenuCore_AppendMsgOption(wk->menuList, wk->msgManager, 0x69, 2, HEAPID_WORLDTRADE);
    ListMenuCore_AppendMsgOption(wk->menuList, wk->msgManager, 0x6a, 3, HEAPID_WORLDTRADE);
    WorldTrade_SelBoxInit(wk, 0, 3, 20);
    wk->subprocessSeq = BOX_SEQ_SELECT_WAIT;
    return WT_SEQ_MAIN;
}

static int Box_SubSeqSelectWait(WorldTradeWork *wk) {
    u32 ret = WorldTrade_SelBoxMain(wk);
    BoxPkm *pkm;

    switch (ret) {
    case 1:
        WorldTrade_SelBoxEnd(wk);
        ListMenuCore_FreeOptionList(wk->menuList);
        BmpWin_ClearFrame(wk->msgWin, 0);
        wk->subprocessSeq = BOX_SEQ_END;
        WorldTrade_SubProcessChange(wk, WORLDTRADE_STATUS, BOX_MODE_DEPOSIT_SELECT);
        break;
    case 2:
        WorldTrade_SelBoxEnd(wk);
        ListMenuCore_FreeOptionList(wk->menuList);
        pkm = WorldTrade_GetPokePtr(wk->param->myparty, wk->param->mybox, wk->boxTrayNo, wk->boxCursorPos);
        if (Box_PokeRibbonCheck(pkm)) {
            Box_SubSeqMessagePrint(wk, 0x2b, 1, 0, 0xf0f, 1);
            WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_CLEAR_WAIT, BOX_SEQ_MAIN);
        } else if (hasPokemonChangedForm(pkm)) {
            Box_SubSeqMessagePrint(wk, 0xbf, 1, 0, 0xf0f, 1);
            WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_CLEAR_WAIT, BOX_SEQ_MAIN);
        } else if (Box_PokeNewItemCheck(pkm)) {
            Box_SubSeqMessagePrint(wk, 0xc1, 1, 0, 0xf0f, 1);
            WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_CLEAR_WAIT, BOX_SEQ_MAIN);
        } else {
            if (WorldTrade_GetPPorPPP(wk->boxTrayNo)) {
                PokeParty_GetPkm(wk->param->myparty, wk->boxCursorPos);
            }
            wk->depositPkm =
                WorldTrade_GetPokePtr(wk->param->myparty, wk->param->mybox, wk->boxTrayNo, wk->boxCursorPos);
            wk->subprocessSeq = BOX_SEQ_END;
            WorldTrade_SubProcessChange(wk, WORLDTRADE_DEPOSIT, 0);
        }
        break;
    case 3:
    case BMPMENULIST_CANCEL:
        WorldTrade_SelBoxEnd(wk);
        ListMenuCore_FreeOptionList(wk->menuList);
        BmpWin_ClearFrame(wk->msgWin, 0);
        wk->subprocessSeq = BOX_SEQ_START;
        break;
    }
    return WT_SEQ_MAIN;
}

static int Box_SubSeqExchangeSelectList(WorldTradeWork *wk) {
    wk->menuList = ListMenuCore_CreateOptionList(3, HEAPID_WORLDTRADE);
    ListMenuCore_AppendMsgOption(wk->menuList, wk->msgManager, 0x5d, 1, HEAPID_WORLDTRADE);
    ListMenuCore_AppendMsgOption(wk->menuList, wk->msgManager, 0x5e, 2, HEAPID_WORLDTRADE);
    ListMenuCore_AppendMsgOption(wk->menuList, wk->msgManager, 0x5f, 3, HEAPID_WORLDTRADE);
    WorldTrade_SelBoxInit(wk, 0, 3, 20);
    wk->subprocessSeq = BOX_SEQ_EXCHANGE_SELECT_WAIT;
    return WT_SEQ_MAIN;
}

static int Box_SubSeqExchangeSelectWait(WorldTradeWork *wk) {
    u32 ret = WorldTrade_SelBoxMain(wk);
    BoxPkm *pkm;

    switch (ret) {
    case 1:
        WorldTrade_SelBoxEnd(wk);
        ListMenuCore_FreeOptionList(wk->menuList);
        wk->subprocessSeq = BOX_SEQ_END;
        WorldTrade_SubProcessChange(wk, WORLDTRADE_STATUS, BOX_MODE_EXCHANGE_SELECT);
        break;
    case 2:
        WorldTrade_SelBoxEnd(wk);
        ListMenuCore_FreeOptionList(wk->menuList);
        BmpWin_ClearFrame(wk->msgWin, 0);
        pkm = WorldTrade_GetPokePtr(wk->param->myparty, wk->param->mybox, wk->boxTrayNo, wk->boxCursorPos);
        if (Box_PokeRibbonCheck(pkm)) {
            Box_SubSeqMessagePrint(wk, 0x2b, 1, 0, 0xf0f, 1);
            WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_CLEAR_WAIT, BOX_SEQ_MAIN);
        } else if (hasPokemonChangedForm(pkm)) {
            Box_SubSeqMessagePrint(wk, 0xbf, 1, 0, 0xf0f, 1);
            WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_CLEAR_WAIT, BOX_SEQ_MAIN);
        } else if (Box_PokeNewItemCheck(pkm)) {
            Box_SubSeqMessagePrint(wk, 0xc1, 1, 0, 0xf0f, 1);
            WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_CLEAR_WAIT, BOX_SEQ_MAIN);
        } else {
            if (WorldTrade_GetPPorPPP(wk->boxTrayNo)) {
                PokeParty_GetPkm(wk->param->myparty, wk->boxCursorPos);
            }
            // Shaymin's Sky Forme reverts when traded
            if (PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL) == SPECIES_SHAYMIN) {
                PML_PkmChangeForme(pkm, 0);
            }
            Box_ExchangeCheck(wk);
        }
        break;
    case 3:
    case BMPMENULIST_CANCEL:
        WorldTrade_SelBoxEnd(wk);
        ListMenuCore_FreeOptionList(wk->menuList);
        BmpWin_ClearFrame(wk->msgWin, 0);
        wk->subprocessSeq = BOX_SEQ_START;
        break;
    }
    return WT_SEQ_MAIN;
}

static int Box_SubSeqEnd(WorldTradeWork *wk) {
    if (wk->subNextProcess == WORLDTRADE_ENTER || wk->subNextProcess == WORLDTRADE_STATUS ||
        wk->subNextProcess == WORLDTRADE_PARTNER) {
        GFL_WipeSet(0, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
        wk->subOutFlag = 1;
    } else {
        GFL_WipeSet(3, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
    }
    wk->subprocessSeq = BOX_SEQ_START;
    return WT_SEQ_FADEOUT;
}

static int Box_SubSeqYesNo(WorldTradeWork *wk) {
    WorldTrade_TouchWinYesNoMake(wk, 20, 0x1ad, 8, TRUE);
    wk->subprocessSeq = BOX_SEQ_YESNO_SELECT;
    return WT_SEQ_MAIN;
}

static int Box_SubSeqYesNoSelect(WorldTradeWork *wk) {
    u32 ret = WorldTrade_TouchSwMain(wk);

    GFL_ASSERT(0);
    if (ret == 1) {
        WorldTrade_TouchWinYesNoDel(wk);
        wk->subprocessSeq = BOX_SEQ_END;
        WorldTrade_SubProcessChange(wk, WORLDTRADE_ENTER, 0);
    } else if (ret == 2) {
        WorldTrade_TouchWinYesNoDel(wk);
        wk->subprocessSeq = BOX_SEQ_START;
    }
    return WT_SEQ_MAIN;
}

static int Box_SubSeqCBallYesNoMessage(WorldTradeWork *wk) {
    Box_SubSeqMessagePrint(wk, 0x19, 1, 0, 0xf0f, 1);
    WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_WAIT, BOX_SEQ_CBALL_YESNO);
    return WT_SEQ_MAIN;
}

static int Box_SubSeqCBallYesNo(WorldTradeWork *wk) {
    WorldTrade_TouchWinYesNoMake(wk, 18, 0x1ad, 8, TRUE);
    wk->subprocessSeq = BOX_SEQ_CBALL_YESNO_SELECT;
    return WT_SEQ_MAIN;
}

static int Box_SubSeqCBallYesNoSelect(WorldTradeWork *wk) {
    u32 ret = WorldTrade_TouchSwMain(wk);

    if (ret == 1) {
        WorldTrade_TouchWinYesNoDel(wk);
        Box_ExchangeCheck(wk);
    } else if (ret == 2) {
        WorldTrade_TouchWinYesNoDel(wk);
        BmpWin_ClearFrame(wk->talkWin, 0);
        wk->subprocessSeq = BOX_SEQ_MAIN;
    }
    return WT_SEQ_MAIN;
}

static int Box_ExchangeCheck(WorldTradeWork *wk) {
    if (WorldTrade_PokemonMailCheck((PartyPkm *)wk->downloadPokemonData[wk->touchTrainerPos].postData) &&
        wk->boxTrayNo != BOX_TRAY_PARTY && PokeParty_GetPkmCount(wk->param->myparty) == 6) {
        Box_SubSeqMessagePrint(wk, 0x29, 1, 0, 0xf0f, 1);
        WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_CLEAR_WAIT, BOX_SEQ_MAIN);
        return FALSE;
    }
    wk->depositPkm = WorldTrade_GetPokePtr(wk->param->myparty, wk->param->mybox, wk->boxTrayNo, wk->boxCursorPos);
    wk->subprocessSeq = BOX_SEQ_END;
    wk->subOutFlag = 1;
    WorldTrade_SubProcessChange(wk, WORLDTRADE_UPLOAD, 9);
    Box_MakeExchangePokemonData(&wk->uploadPokemonData, wk);
    wk->searchResult = 0;
    return TRUE;
}

static int Box_SubSeqCBallDepositYesNoMessage(WorldTradeWork *wk) {
    Box_SubSeqMessagePrint(wk, 0x19, 1, 0, 0xf0f, 1);
    WorldTrade_SetNextSeq(wk, BOX_SEQ_MESSAGE_WAIT, BOX_SEQ_CBALL_DEPOSIT_YESNO);
    return WT_SEQ_MAIN;
}

static int Box_SubSeqCBallDepositYesNo(WorldTradeWork *wk) {
    WorldTrade_TouchWinYesNoMake(wk, 18, 0x1ad, 8, TRUE);
    wk->subprocessSeq = BOX_SEQ_CBALL_DEPOSIT_YESNO_SELECT;
    return WT_SEQ_MAIN;
}

static int Box_SubSeqCBallDepositYesNoSelect(WorldTradeWork *wk) {
    u32 ret = WorldTrade_TouchSwMain(wk);

    if (ret == 1) {
        WorldTrade_TouchWinYesNoDel(wk);
        wk->depositPkm = WorldTrade_GetPokePtr(wk->param->myparty, wk->param->mybox, wk->boxTrayNo, wk->boxCursorPos);
        wk->subprocessSeq = BOX_SEQ_END;
        WorldTrade_SubProcessChange(wk, WORLDTRADE_DEPOSIT, 0);
    } else if (ret == 2) {
        WorldTrade_TouchWinYesNoDel(wk);
        BmpWin_ClearFrame(wk->talkWin, 0);
        wk->subprocessSeq = BOX_SEQ_MAIN;
    }
    return WT_SEQ_MAIN;
}

static int Box_SubSeqCancelWait(WorldTradeWork *wk) {
    if (!func_0204c560(wk->fingerAct)) {
        Box_CancelFunc(wk, wk->subProcessMode);
    }
    return WT_SEQ_MAIN;
}

static int Box_SubSeqMessageWait(WorldTradeWork *wk) {
    if (!WorldTrade_PrintIsBusy(&wk->print)) {
        wk->subprocessSeq = wk->subprocessNextSeq;
    }
    return WT_SEQ_MAIN;
}

static int Box_SubSeqMessageClearWait(WorldTradeWork *wk) {
    if (!WorldTrade_PrintIsBusy(&wk->print)) {
        if (GCTX_HIDGetPressedKeys() || func_0203da48()) {
            BmpWin_ClearFrame(wk->talkWin, 0);
            wk->subprocessSeq = wk->subprocessNextSeq;
        }
    }
    return WT_SEQ_MAIN;
}

static void Box_SubSeqMessagePrint(WorldTradeWork *wk, int msgNo, int wait, int flag, u16 dat, int winFlag) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(wk->msgManager, msgNo);
    BmpWin *win;

    GFL_WordSetFormatStrbuf(wk->wordSet, wk->talkString, str);
    if (winFlag == 0) {
        win = wk->msgWin;
    } else {
        win = wk->talkWin;
    }
    GFL_BitmapFill(BmpWin_GetBitmap(win), 15);
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
    BmpWin_DrawFrame(win, 0, 1, 14);
    WorldTrade_Print(win, 0, wk->talkString, 0, 0, &wk->print);
    GFL_StrBufFree(str);
}

static void *Box_CharDataGetbyHandle(ArcTool *handle, u32 dataIdx, NNSG2dCharacterData **charData, u32 heapId) {
    void *file = GFL_ArcToolReadHeapNew(handle, dataIdx, heapId);

    if (file != NULL) {
        if (!NNS_G2dGetUnpackedBGCharacterData(file, charData)) {
            GFL_HeapFree(file);
            return NULL;
        }
    }
    return file;
}

static void Box_TransPokeIconCharaPal(int species, int form, int sex, int egg, int no, ClActor *icon, ArcTool *handle,
                                      WorldTradePokeBuf *pbuf) {
    NNSG2dCharacterData *charData;
    void *file =
        Box_CharDataGetbyHandle(handle, PokeParty_GetIconIndex(species, form, sex, egg), &charData, HEAPID_WORLDTRADE);

    sys_memcpy32_fast(charData->rawData, pbuf->chars, 0x200);
    pbuf->charOffset = (no * 16 + 28) * 32;
    pbuf->icon = icon;
    pbuf->palette = PokeIcon_GetPalette(species, form, sex, egg) + 5;
    GFL_HeapFree(file);
}

static void Box_PokemonLevelSet(BoxPkm *pkm, Dpw_Tr_PokemonDataSimple *dat) {
    dat->level = PML_PkmGetLevel(pkm);
}

static void Box_PokemonIconDraw(WorldTradeWork *wk) {
    int i;
    WorldTradePokeBuf *pbuf = wk->boxIcon;

    for (i = 0; i < BOX_POKE_NUM; i++, pbuf++) {
        if (pbuf->icon != NULL) {
            cp15_flushDC(pbuf->chars, 0x200);
            gfxUploadObjCharA(pbuf->chars, pbuf->charOffset, 0x200);
            func_0204c378(pbuf->icon, (u8)pbuf->palette, 1);
        }
    }
    GFL_HeapFree(wk->boxIcon);
}

static void Box_PokemonIconSet(BoxPkm *pkm, ClActor *icon, ClActor *itemAct, u16 *no, int pos, ArcTool *handle,
                               Dpw_Tr_PokemonDataSimple *dat, WorldTradePokeBuf *pbuf) {
    int exists;
    int item;
    int egg;
    int form;

    PML_PkmDecrypt(pkm);
    exists = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL);
    *no = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    form = PML_PkmGetParam(pkm, PKM_PARAM_FORM, NULL);
    egg = PML_PkmGetParam(pkm, PKM_PARAM_IS_EGG, NULL);
    item = PML_PkmGetParam(pkm, PKM_PARAM_ITEM, NULL);
    dat->characterNo = *no;
    dat->gender = PML_PkmGetParam(pkm, PKM_PARAM_SEX, NULL) + 1;
    if (egg) {
        dat->level = 0;
    }
    PML_PkmReEncrypt(pkm, TRUE);

    if (exists) {
        Box_TransPokeIconCharaPal(*no, form, dat->gender - 1, egg, pos, icon, handle, pbuf);
        func_0204c124(icon, TRUE);
        if (item) {
            func_0204c124(itemAct, TRUE);
            if (PML_ItemIsMail(item)) {
                func_0204c488(itemAct, 41);
            } else {
                func_0204c488(itemAct, 40);
            }
        } else {
            func_0204c124(itemAct, FALSE);
        }
    } else {
        func_0204c124(icon, FALSE);
        func_0204c124(itemAct, FALSE);
        pbuf->icon = NULL;
    }
}

static void Box_NowBoxPageInfoGet(WorldTradeWork *wk, int now) {
    BoxSaveAccessor *box = wk->param->mybox;
    u16 no[BOX_POKE_NUM];
    ArcTool *handle;
    WorldTradePokeBuf *pbuf;
    BoxPkm *pkm;
    int num;
    u16 i = 0;

    pbuf = GFL_HeapAllocate(HEAPID_TAIL(HEAPID_USER), sizeof(WorldTradePokeBuf) * BOX_POKE_NUM, FALSE,
                            "worldtrade_box.c", 2185);
    wk->boxIcon = pbuf;
    handle = GFL_ArcSysCreateFileHandle(7, HEAPID_WORLDTRADE);

    if (now >= 0 && now < wk->boxCount) {
        for (i = 0; i < BOX_POKE_NUM; i++) {
            Box_PokemonLevelSet(BoxSaveAccessor_GetPkm(box, now, i), &wk->boxWork[i]);
        }
        for (i = 0; i < BOX_POKE_NUM; i++) {
            wk->boxWork[i].characterNo = 0;
            Box_PokemonIconSet(BoxSaveAccessor_GetPkm(box, now, i), wk->pokeIconAct[i], wk->itemIconAct[i], &no[i], i,
                               handle, &wk->boxWork[i], &pbuf[i]);
            if (i < 6) {
                func_0204c124(wk->cballAct[i], FALSE);
            }
        }
        loadBoxNameToStrbuf(box, now, wk->boxTrayNameString);
    } else if (now == BOX_TRAY_PARTY) {
        num = PokeParty_GetPkmCount(wk->param->myparty);
        for (i = 0; i < num; i++) {
            pkm = func_0201d624(PokeParty_GetPkm(wk->param->myparty, i));
            Box_PokemonLevelSet(pkm, &wk->boxWork[i]);
            Box_PokemonIconSet(pkm, wk->pokeIconAct[i], wk->itemIconAct[i], &no[i], i, handle, &wk->boxWork[i],
                               &pbuf[i]);
            func_0204c124(wk->cballAct[i], FALSE);
        }
        for (; i < BOX_POKE_NUM; i++) {
            wk->boxWork[i].characterNo = 0;
            func_0204c124(wk->pokeIconAct[i], FALSE);
            func_0204c124(wk->itemIconAct[i], FALSE);
            pbuf[i].icon = NULL;
            if (i < 6) {
                func_0204c124(wk->cballAct[i], FALSE);
            }
        }
        GFL_MsgDataLoadStrbuf(wk->msgManager, 0x62, wk->boxTrayNameString);
    } else {
        // "BOX out of range %d"
        GFL_ASSERT_MSG(FALSE, "BOX\x94\xcd\x88\xcd\x8a\x4f%d\n", now);
    }
    GFL_ArcToolFree(handle);

    GFL_BitmapFill(BmpWin_GetBitmap(wk->subWin), 0);
    WorldTrade_SysPrint(wk->subWin, wk->boxTrayNameString, 0, 5, 1, 0x440, &wk->print);
    if (wk->subProcessMode == BOX_MODE_EXCHANGE_SELECT) {
        Box_PokeIconPalSet(wk->boxWork, wk->pokeIconAct, &wk->downloadPokemonData[wk->touchTrainerPos].wantSimple,
                           pbuf);
    }
    wk->vfunc = Box_PokemonIconDraw;
}

BOOL WorldTrade_GetPPorPPP(int tray) {
    if (tray == BOX_TRAY_PARTY) {
        return TRUE;
    }
    return FALSE;
}

BoxPkm *WorldTrade_GetPokePtr(PokeParty *party, BoxSaveAccessor *box, int tray, int pos) {
    if (WorldTrade_GetPPorPPP(tray)) {
        if (pos > PokeParty_GetPkmCount(party) - 1) {
            return NULL;
        }
        return func_0201d624(PokeParty_GetPkm(party, pos));
    }
    return BoxSaveAccessor_GetPkm(box, tray, pos);
}

int WorldTrade_GetBoxPokeNum(PokeParty *party, BoxSaveAccessor *box, int tray) {
    if (WorldTrade_GetPPorPPP(tray)) {
        return PokeParty_GetPkmCount(party);
    }
    return howManyPokesInGeneralAreInBox(box, tray);
}

// Whether the Pokémon may leave: not the party's last one
static BOOL Box_CheckPocket(PokeParty *party, BoxSaveAccessor *box, int tray, int pos) {
    if (WorldTrade_GetPPorPPP(tray) && PokeParty_GetPkmCount(party) < 2) {
        return FALSE;
    }
    return TRUE;
}

// The ribbons that keep a Pokémon from the Global Trade Station
static const u32 sRibbonCheckTable[] = {
    0x69, 0x6a, 0x6b, 0x6c, 0x33, 0x34, 0x2c, 0x2f, 0x30, 0x31, 0x32, 0x66, 0x67, 0x68, 0x2e,
};

static u32 Box_PokeRibbonCheck(BoxPkm *pkm) {
    BOOL flag = PML_PkmDecrypt(pkm);
    int i;
    u32 ret;

    for (i = 0; i < NELEMS(sRibbonCheckTable); i++) {
        ret = PML_PkmGetParam(pkm, sRibbonCheckTable[i], NULL);
        if (ret == TRUE) {
            break;
        }
    }
    PML_PkmReEncrypt(pkm, flag);
    return ret;
}

static BOOL Box_PokeNewItemCheck(BoxPkm *pkm) {
    BOOL flag = PML_PkmDecrypt(pkm);

    PML_PkmGetParam(pkm, PKM_PARAM_ITEM, NULL);
    PML_PkmReEncrypt(pkm, flag);
    return FALSE;
}

// 0 for no Pokémon, 2 for an egg, 1 for a Pokémon
static int Box_PokemonCheck(PokeParty *party, BoxSaveAccessor *box, int tray, int pos) {
    BoxPkm *pkm = WorldTrade_GetPokePtr(party, box, tray, pos);

    if (pkm == NULL) {
        return 0;
    }
    if (PML_PkmGetParam(pkm, PKM_PARAM_SPECIES_VALID, NULL) == 0) {
        return 0;
    }
    if (PML_PkmGetParam(pkm, 0xaa, NULL)) {
        return 2;
    }
    return 1;
}

static BOOL Box_CompareSearchData(Dpw_Tr_PokemonDataSimple *poke, Dpw_Tr_PokemonSearchData *search) {
    if (poke->characterNo != search->characterNo) {
        return FALSE;
    }
    if (search->gender != SEARCH_GENDER_ANY && search->gender != poke->gender) {
        return FALSE;
    }
    if (poke->level == 0) {
        return FALSE;
    }
    if (search->level_min != 0 && search->level_min > poke->level) {
        return FALSE;
    }
    if (search->level_max != 0 && search->level_max < poke->level) {
        return FALSE;
    }
    return TRUE;
}

static BOOL Box_WantPokeCheck(BoxPkm *pkm, Dpw_Tr_PokemonSearchData *want) {
    Dpw_Tr_PokemonDataSimple poke;

    poke.characterNo = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    poke.gender = PML_PkmGetParam(pkm, PKM_PARAM_SEX, NULL) + 1;
    poke.level = PML_PkmGetLevel(pkm);
    return Box_CompareSearchData(&poke, want);
}

static void Box_MakeExchangePokemonData(Dpw_Tr_Data *dtd, WorldTradeWork *wk) {
    Dpw_Tr_PokemonDataSimple post;
    Dpw_Tr_PokemonSearchData want;
    BoxPkm *pkm;

    post.characterNo = PML_PkmGetParam(wk->depositPkm, PKM_PARAM_SPECIES, NULL);
    post.gender = PML_PkmGetParam(wk->depositPkm, PKM_PARAM_SEX, NULL) + 1;
    post.level = PML_PkmGetLevel(wk->depositPkm);
    dtd->postSimple = post;
    WorldTrade_PostPokemonBaseDataMake(dtd, wk);

    pkm = func_0201d624((PartyPkm *)wk->downloadPokemonData[wk->touchTrainerPos].postData);
    want.characterNo = PML_PkmGetParam(pkm, PKM_PARAM_SPECIES, NULL);
    want.gender = PML_PkmGetParam(pkm, PKM_PARAM_SEX, NULL) + 1;
    want.level_min = 0;
    want.level_max = 0;
    dtd->wantSimple = want;
}

// Shows the Pokémon that the trade partner doesn't want with darker icons
static void Box_PokeIconPalSet(Dpw_Tr_PokemonDataSimple *box, ClActor **icons, Dpw_Tr_PokemonSearchData *want,
                               WorldTradePokeBuf *pbuf) {
    int i;

    for (i = 0; i < BOX_POKE_NUM; i++) {
        if (box[i].characterNo != 0 && !Box_CompareSearchData(&box[i], want)) {
            pbuf[i].palette += 3;
        }
    }
}

BOOL WorldTrade_PokemonMailCheck(PartyPkm *pkm) {
    BOOL ret = FALSE;

    if (PML_ItemIsMail(PokeParty_GetParam(pkm, PKM_PARAM_ITEM, NULL))) {
        ret = TRUE;
    }
    return ret;
}

// Unlocks more boxes once every box holds a Pokémon
static void Box_BoxCountCheck(WorldTradeWork *wk) {
    u32 i;

    wk->boxCount = BoxSaveAccessor_GetAvailableBoxCount(wk->param->mybox);
    if (wk->boxCount < 24) {
        for (i = 0; i < wk->boxCount; i++) {
            if (howManyPokesInGeneralAreInBox(wk->param->mybox, i) == 0) {
                break;
            }
        }
        if (i == wk->boxCount) {
            wk->boxCount = BoxSaveAccessor_UnlockMoreBoxes(wk->param->mybox);
        }
    }
}
