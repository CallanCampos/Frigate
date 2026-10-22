#include "types.h"
#include "constants/arc.h"
#include "constants/sound.h"
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
#include "gfl/net.h"
#include "gfl/sound.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "system/bmp_winframe.h"
#include "system/wipe.h"
#include "worldtrade_local.h"

// The Global Trade Station's title menu, where the player chooses to deposit or check a Pokémon, to search, or to
// leave, and the lower screen's trade room that the other screens share. The file's name is a guess, as the ROM
// doesn't give it, and so are the names of its functions

enum {
    TITLE_SEQ_OPENING,
    TITLE_SEQ_OPENING_MESSAGE,
    TITLE_SEQ_OPENING_FADE,
    TITLE_SEQ_OPENING_FADE_WAIT,
    TITLE_SEQ_OPENING_WAIT,
    TITLE_SEQ_START,
    TITLE_SEQ_MAIN,
    TITLE_SEQ_END_DEMO,
    TITLE_SEQ_END_DEMO_WAIT,
    TITLE_SEQ_END,
    TITLE_SEQ_MES_WAIT,
    TITLE_SEQ_MES_1MIN_WAIT,
    TITLE_SEQ_YESNO,
    TITLE_SEQ_YESNO_SELECT,
    TITLE_SEQ_CURSOR_ANIM_WAIT,
};

// The menu's choices
enum {
    TITLE_MENU_DEPOSIT,
    TITLE_MENU_SEARCH,
    TITLE_MENU_EXIT,
};

typedef struct {
    u16 x;
    u16 y;
} TitleCursorPos;

static void Title_BgInit(void);
static void Title_BgExit(void);
static void Title_DemoBgSet(WorldTradeWork *wk);
static void Title_BgGraphicSet(WorldTradeWork *wk);
static void Title_SetCellActor(WorldTradeWork *wk);
static void Title_DelCellActor(WorldTradeWork *wk);
static void Title_BmpWinInit(WorldTradeWork *wk);
static void Title_BmpWinDelete(WorldTradeWork *wk);
static void Title_InitWork(WorldTradeWork *wk);
static void Title_FreeWork(WorldTradeWork *wk);
static int Title_SubSeqOpening(WorldTradeWork *wk);
static int Title_SubSeqOpeningMessage(WorldTradeWork *wk);
static int Title_SubSeqOpeningFade(WorldTradeWork *wk);
static int Title_SubSeqOpeningFadeWait(WorldTradeWork *wk);
static int Title_SubSeqOpeningWait(WorldTradeWork *wk);
static int Title_SubSeqStart(WorldTradeWork *wk);
static int Title_TouchPanelFunc(WorldTradeWork *wk);
static void Title_DecideFunc(WorldTradeWork *wk, int decide);
static int Title_SubSeqMain(WorldTradeWork *wk);
static int Title_SubSeqEndDemo(WorldTradeWork *wk);
static int Title_SubSeqEndDemoWait(WorldTradeWork *wk);
static int Title_SubSeqEnd(WorldTradeWork *wk);
static int Title_SubSeqYesNo(WorldTradeWork *wk);
static int Title_SubSeqYesNoSelect(WorldTradeWork *wk);
static int Title_SubSeqCursorAnimWait(WorldTradeWork *wk);
static int Title_SubSeqMessageWait(WorldTradeWork *wk);
static int Title_SubSeqMessage1MinWait(WorldTradeWork *wk);
static void Title_MenuPrint(WorldTradeWork *wk);
static void Title_MessagePrint(WorldTradeWork *wk, int msgNo, int wait, int flag, u16 dat);
static void Title_TalkPrint(WorldTradeWork *wk, int msgNo, int wait, int flag, u16 dat);
static void Title_BmpWinPrint(BmpWin *win, MsgData *msgManager, int font, int msgNo, u16 dat, WorldTradePrint *print,
                              u16 color);

static int (*sTitleSubSeqTable[])(WorldTradeWork *wk) = {
    Title_SubSeqOpening,        Title_SubSeqOpeningMessage,
    Title_SubSeqOpeningFade,    Title_SubSeqOpeningFadeWait,
    Title_SubSeqOpeningWait,    Title_SubSeqStart,
    Title_SubSeqMain,           Title_SubSeqEndDemo,
    Title_SubSeqEndDemoWait,    Title_SubSeqEnd,
    Title_SubSeqMessageWait,    Title_SubSeqMessage1MinWait,
    Title_SubSeqYesNo,          Title_SubSeqYesNoSelect,
    Title_SubSeqCursorAnimWait,
};

int WorldTrade_Title_Init(WorldTradeWork *wk, int seq) {
    Title_InitWork(wk);
    GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
    Title_BgInit();
    Title_BgGraphicSet(wk);
    Title_BmpWinInit(wk);
    WorldTrade_SubLcdBgInit(wk, 0, FALSE);
    WorldTrade_SubLcdBgGraphicSet(wk);
    if (wk->openingFlag == 0) {
        GFL_BGSysSetBGEnabled(4, FALSE);
    } else {
        WorldTrade_SubLcdWinGraphicSet(wk);
        GFL_BGSysSetBGEnabled(4, TRUE);
    }
    WorldTrade_SubLcdExplainPut(wk, 0);
    wk->subLcdBgKeep = 0;
    Title_SetCellActor(wk);
    WorldTrade_WifiIconAdd(wk);
    Title_MenuPrint(wk);

    if (wk->openingFlag == 0) {
        // The first visit walks the player into the trade room
        Title_DemoBgSet(wk);
        GFL_WipeSet(0, 1, 1, 0, 6, 1, HEAPID_WORLDTRADE);
        wk->subprocessSeq = TITLE_SEQ_OPENING;
        wk->openingFlag = 1;
        WorldTrade_HeroDemo(wk);
    } else {
        if (wk->subOutFlag == 1 && GXS_GetMasterBrightness() != 0) {
            WorldTrade_SetPartnerExchangePos(wk);
            GFL_WipeSet(0, 1, 1, 0, 6, 1, HEAPID_WORLDTRADE);
        } else {
            GFL_WipeSet(3, 1, 1, 0, 6, 1, HEAPID_WORLDTRADE);
        }
        wk->subprocessSeq = TITLE_SEQ_START;
    }
    wk->subOutFlag = 0;
    WorldTrade_SubLcdMatchObjHide(wk);
    return WT_SEQ_FADEIN;
}

int WorldTrade_Title_Main(WorldTradeWork *wk, int seq) {
    return sTitleSubSeqTable[wk->subprocessSeq](wk);
}

int WorldTrade_Title_End(WorldTradeWork *wk, int seq) {
    Title_DelCellActor(wk);
    Title_FreeWork(wk);
    Title_BmpWinDelete(wk);
    Title_BgExit();
    // The search and box screens keep the trade room
    if (wk->subNextProcess == WORLDTRADE_SEARCH || wk->subNextProcess == WORLDTRADE_MYBOX) {
        wk->subLcdBgKeep = 1;
    }
    WorldTrade_SubLcdBgExit(wk);
    ClActor_SetVisible(wk->promptDsAct, FALSE);
    WorldTrade_SubProcessUpdate(wk);
    return WT_SEQ_INIT;
}

static void Title_BgInit(void) {
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
            1,
            GX_BG_AREAOVER_XLU,
            FALSE,
        };
        GFL_BGSysCreateBG(2, &setup, BGMODE_TEXT);
        GFL_BGSysSetBGEnabled(2, TRUE);
    }
    GFL_BGSysClearCharCore(0, 32, 0, HEAPID_WORLDTRADE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_OBJ, TRUE);
    GFL_BGSysSetBGEnabledB(GX_PLANEMASK_OBJ, TRUE);
}

void WorldTrade_SubLcdBgInit(WorldTradeWork *wk, int bg1YOffset, BOOL bg2NoClear) {
    if (wk->subLcdBgInit == 0) {
        wk->subLcdBgInit = 1;
        {
            BGSetup setup = {
                0,
                0,
                0x800,
                0,
                BGRES_256x256,
                GX_BG_COLORMODE_16,
                GX_BG_SCRBASE(0xf000),
                GX_BG_CHARBASE(0x10000),
                0x8000,
                GX_BG_EXTPLTT_01,
                0,
                GX_BG_AREAOVER_XLU,
                FALSE,
            };
            GFL_BGSysCreateBG(4, &setup, BGMODE_TEXT);
            GFL_BGSysFillScrAsync(4, 0);
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
                GX_BG_CHARBASE(0x14000),
                0x8000,
                GX_BG_EXTPLTT_01,
                1,
                GX_BG_AREAOVER_XLU,
                FALSE,
            };
            GFL_BGSysCreateBG(6, &setup, BGMODE_TEXT);
            if (!bg2NoClear) {
                GFL_BGSysFillScrAsync(6, 0);
            }
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
                2,
                GX_BG_AREAOVER_XLU,
                FALSE,
            };
            setup.y = bg1YOffset;
            GFL_BGSysCreateBG(5, &setup, BGMODE_TEXT);
        }
        GFL_BGSysClearCharCore(4, 32, 0, HEAPID_WORLDTRADE);
        {
            BGSetup setup = {
                0,
                0,
                0x800,
                0,
                BGRES_256x256,
                GX_BG_COLORMODE_16,
                GX_BG_SCRBASE(0xd800),
                GX_BG_CHARBASE(0x10000),
                0x8000,
                GX_BG_EXTPLTT_01,
                0,
                GX_BG_AREAOVER_XLU,
                FALSE,
            };
            GFL_BGSysCreateBG(7, &setup, BGMODE_TEXT);
            GFL_BGSysFillScrAsync(7, 0);
        }
    }
    GFL_BGSysSetBGEnabled(4, TRUE);
    GFL_BGSysSetBGEnabled(5, TRUE);
    GFL_BGSysSetBGEnabled(6, TRUE);
    GFL_BGSysSetBGEnabled(7, TRUE);
}

void WorldTrade_SubLcdBgExit(WorldTradeWork *wk) {
    if (wk->subLcdBgInit == 1 && wk->subLcdBgKeep == 0) {
        wk->subLcdBgInit = 0;
        GFL_BGSysReleaseBG(6);
        GFL_BGSysReleaseBG(5);
        GFL_BGSysReleaseBG(4);
        GFL_BGSysReleaseBG(7);
    }
}

static void Title_BgExit(void) {
    GFL_BGSysReleaseBG(2);
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(0);
}

// Hides the menu while the player walks in
static void Title_DemoBgSet(WorldTradeWork *wk) {
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, FALSE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, FALSE);
    ClActor_SetVisible(wk->cursorAct, FALSE);
}

static void Title_BgGraphicSet(WorldTradeWork *wk) {
    ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_WORLDTRADE, HEAPID_WORLDTRADE);

    GFL_G2DIOLoadArcNCLRDefault(arc, 3, 0, 0, 0x60, HEAPID_WORLDTRADE);
    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1a0, 0x20, HEAPID_WORLDTRADE);
    LoadSysMsgBox(0, 1, 14, 0, HEAPID_WORLDTRADE);
    LoadSysMsgBox(0, 31, 11, 0, HEAPID_WORLDTRADE);
    GFL_BGSysLoadArcNCGRStatic(arc, 13, 1, 0, 0, TRUE, HEAPID_WORLDTRADE);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 32, 1, 0, 0x600, TRUE, HEAPID_WORLDTRADE);
    loadBGScrToVramByFileNoReserveNegAlign(arc, 31, 2, 0, 0x600, TRUE, HEAPID_WORLDTRADE);
    GFL_ArcToolFree(arc);
}

// Where the cursor sits on each choice
static const TitleCursorPos sTitleCursorPos[] = {
    { 128, 56 },
    { 128, 96 },
    { 128, 136 },
};

static void Title_SetCellActor(WorldTradeWork *wk) {
    ClActorSetup setup;

    setup.x = sTitleCursorPos[wk->titleCursorPos].x;
    setup.y = sTitleCursorPos[wk->titleCursorPos].y;
    setup.sequence = 0;
    setup.priority = 0;
    setup.bgPriority = 1;
    wk->cursorAct = ClActor_Create(wk->clactUnit, wk->clactRes[WT_CLACT_RES_MAIN2][WT_CLACT_RES_CHAR],
                                   wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT],
                                   wk->clactRes[WT_CLACT_RES_MAIN2][WT_CLACT_RES_CELL], &setup, 0, HEAPID_WORLDTRADE);
    ClActor_SetAutoAnim(wk->cursorAct, TRUE);
    ClActor_SetSequence(wk->cursorAct, 0);
    // The cursor only shows while the keys are used
    if (GCTX_HIDIsUsingTouch() == TRUE) {
        ClActor_SetVisible(wk->cursorAct, FALSE);
    } else {
        ClActor_SetSequence(wk->cursorAct, 0);
        ClActor_SetVisible(wk->cursorAct, TRUE);
    }
    GFL_NetSetIconScreen(FALSE, HEAPID_WORLDTRADE);
}

static void Title_DelCellActor(WorldTradeWork *wk) {
    ClActor_Delete(wk->cursorAct);
}

static void Title_BmpWinInit(WorldTradeWork *wk) {
    int i;

    wk->titleWin = BmpWin_CreateDynamic(1, 2, 1, 28, 2, 1, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->titleWin), 10);
    BmpWin_TransferNow(wk->titleWin);
    WorldTrade_PrintColor(wk->titleWin, 0, wk->titleString, 0, 2, 0, 0x35ca, &wk->print);

    for (i = 0; i < 3; i++) {
        wk->menuWin[i] = BmpWin_CreateDynamic(1, 9, i * 5 + 6, 15, 2, 1, TRUE);
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[i]), 1);
        BmpWin_TransferNow(wk->menuWin[i]);
    }

    wk->msgWin = BmpWin_CreateDynamic(0, 2, 21, 27, 2, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWin), 15);
    wk->talkWin = BmpWin_CreateDynamic(0, 2, 19, 27, 4, 13, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->talkWin), 15);
}

static void Title_BmpWinDelete(WorldTradeWork *wk) {
    int i;

    WorldTrade_PrintClear(&wk->print);
    BmpWin_Free(wk->explainWin);
    BmpWin_Free(wk->talkWin);
    BmpWin_Free(wk->msgWin);
    for (i = 0; i < 3; i++) {
        BmpWin_Free(wk->menuWin[i]);
    }
    BmpWin_Free(wk->titleWin);
}

static void Title_InitWork(WorldTradeWork *wk) {
    wk->talkString = GFL_StrBufCreate(180, HEAPID_WORLDTRADE);
    wk->titleString = GFL_MsgDataLoadStrbufNew(wk->msgManager, 0x2d);
}

static void Title_FreeWork(WorldTradeWork *wk) {
    GFL_StrBufFree(wk->talkString);
    GFL_StrBufFree(wk->titleString);
}

// Waits for the player's walk, then checks the server for a trade of the deposited Pokémon
static int Title_SubSeqOpening(WorldTradeWork *wk) {
    if (wk->demoEnd) {
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG0, FALSE);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG1, FALSE);
        GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG2, FALSE);
        WorldTrade_SubProcessChange(wk, WORLDTRADE_UPLOAD, 11);
        wk->subReturnProcess = WORLDTRADE_TITLE;
        wk->subprocessSeq = TITLE_SEQ_END;
    }
    return WT_SEQ_MAIN;
}

static int Title_SubSeqOpeningMessage(WorldTradeWork *wk) {
    WorldTrade_SetNextSeq(wk, TITLE_SEQ_MES_WAIT, TITLE_SEQ_OPENING_FADE);
    return WT_SEQ_MAIN;
}

static int Title_SubSeqOpeningFade(WorldTradeWork *wk) {
    GFL_WipeSet(3, 1, 1, 0, 6, 1, HEAPID_WORLDTRADE);
    wk->subprocessSeq = TITLE_SEQ_OPENING_FADE_WAIT;
    return WT_SEQ_MAIN;
}

static int Title_SubSeqOpeningFadeWait(WorldTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        wk->subprocessSeq = TITLE_SEQ_START;
    }
    return WT_SEQ_MAIN;
}

static int Title_SubSeqOpeningWait(WorldTradeWork *wk) {
    if (GFL_WipeIsFinished()) {
        wk->subprocessSeq = TITLE_SEQ_START;
    }
    return WT_SEQ_MAIN;
}

static int Title_SubSeqStart(WorldTradeWork *wk) {
    Title_MessagePrint(wk, 4, 1, 0, 0xf0f);
    WorldTrade_SetNextSeq(wk, TITLE_SEQ_MES_WAIT, TITLE_SEQ_MAIN);
    ClActor_SetAutoAnim(wk->cursorAct, TRUE);
    WorldTrade_BoxPokeNumGetStart(wk);
    return WT_SEQ_MAIN;
}

static const TouchRect sTitleTouchRects[] = {
    { 40, 71, 24, 231 },
    { 80, 111, 24, 231 },
    { 120, 151, 24, 231 },
    { TOUCH_RECT_END },
};

static int Title_TouchPanelFunc(WorldTradeWork *wk) {
    return GFL_HIDGetTouchPressedRect(sTitleTouchRects);
}

static void Title_DecideFunc(WorldTradeWork *wk, int decide) {
    switch (decide) {
    case TITLE_MENU_DEPOSIT:
        if (wk->depositFlag == 0) {
            WorldTrade_SubProcessChange(wk, WORLDTRADE_MYBOX, 5);
            wk->subprocessSeq = TITLE_SEQ_END;
        } else if (wk->serverWaitTime == 0) {
            // Checks the server, then waits a minute before it may again
            WorldTrade_SubProcessChange(wk, WORLDTRADE_UPLOAD, 11);
            wk->subReturnProcess = WORLDTRADE_MYPOKE;
            wk->subprocessSeq = TITLE_SEQ_END;
            wk->serverWaitTime = 60 * 60;
        } else {
            Title_MessagePrint(wk, 40, 1, 0, 0xf0f);
            WorldTrade_SetNextSeq(wk, TITLE_SEQ_MES_1MIN_WAIT, TITLE_SEQ_START);
            GFL_SndSEPlay(SEQ_SE_BEEP);
            wk->wait = 0;
        }
        break;
    case TITLE_MENU_SEARCH:
        wk->subLcdTouchOK = 0;
        WorldTrade_SubProcessChange(wk, WORLDTRADE_SEARCH, 13);
        wk->subprocessSeq = TITLE_SEQ_END;
        break;
    case TITLE_MENU_EXIT:
        Title_TalkPrint(wk, 7, WorldTrade_GetTalkSpeed(wk), 0, 0xf0f);
        WorldTrade_SetNextSeq(wk, TITLE_SEQ_MES_WAIT, TITLE_SEQ_YESNO);
        break;
    }
}

static int Title_SubSeqMain(WorldTradeWork *wk) {
    int touch = Title_TouchPanelFunc(wk);

    if (touch != TOUCH_RECT_NONE) {
        wk->titleCursorPos = touch;
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        WorldTrade_CLACT_PosChange(wk->cursorAct, sTitleCursorPos[wk->titleCursorPos].x,
                                   sTitleCursorPos[wk->titleCursorPos].y);
        ClActor_SetVisible(wk->cursorAct, TRUE);
        ClActor_SetSequence(wk->cursorAct, 1);
        wk->subprocessSeq = TITLE_SEQ_CURSOR_ANIM_WAIT;
        GCTX_HIDSetUsingTouch(TRUE);
        if (wk->titleCursorPos == TITLE_MENU_EXIT) {
            GFL_SndSEPlay(SEQ_SE_CANCEL1);
        } else {
            GFL_SndSEPlay(SEQ_SE_DECIDE1);
        }
        return WT_SEQ_MAIN;
    }

    // A key in touch mode only shows the cursor
    if (GCTX_HIDGetPressedKeys() && GCTX_HIDIsUsingTouch() == TRUE) {
        ClActor_SetVisible(wk->cursorAct, TRUE);
        GCTX_HIDSetUsingTouch(FALSE);
        GFL_SndSEPlay(SEQ_SE_SELECT1);
        return WT_SEQ_MAIN;
    }

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        wk->titleCursorPos = TITLE_MENU_EXIT;
        WorldTrade_CLACT_PosChange(wk->cursorAct, sTitleCursorPos[wk->titleCursorPos].x,
                                   sTitleCursorPos[wk->titleCursorPos].y);
        ClActor_SetVisible(wk->cursorAct, TRUE);
        ClActor_SetSequence(wk->cursorAct, 1);
        wk->subprocessSeq = TITLE_SEQ_CURSOR_ANIM_WAIT;
        GFL_SndSEPlay(SEQ_SE_CANCEL1);
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        ClActor_SetVisible(wk->cursorAct, TRUE);
        ClActor_SetSequence(wk->cursorAct, 1);
        wk->subprocessSeq = TITLE_SEQ_CURSOR_ANIM_WAIT;
        GFL_SndSEPlay(SEQ_SE_DECIDE1);
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_UP) {
        if (wk->titleCursorPos != 0) {
            wk->titleCursorPos--;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            ClActor_SetVisible(wk->cursorAct, TRUE);
            ClActor_RestartAnim(wk->cursorAct);
            WorldTrade_CLACT_PosChange(wk->cursorAct, sTitleCursorPos[wk->titleCursorPos].x,
                                       sTitleCursorPos[wk->titleCursorPos].y);
        }
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_DOWN) {
        if (wk->titleCursorPos < 2) {
            wk->titleCursorPos++;
            GFL_SndSEPlay(SEQ_SE_SELECT1);
            ClActor_SetVisible(wk->cursorAct, TRUE);
            ClActor_RestartAnim(wk->cursorAct);
            WorldTrade_CLACT_PosChange(wk->cursorAct, sTitleCursorPos[wk->titleCursorPos].x,
                                       sTitleCursorPos[wk->titleCursorPos].y);
        }
    }
    return WT_SEQ_MAIN;
}

static int Title_SubSeqEndDemo(WorldTradeWork *wk) {
    WorldTrade_ReturnHeroDemo(wk);
    wk->subprocessSeq = TITLE_SEQ_END_DEMO_WAIT;
    wk->demoEnd = 0;
    return WT_SEQ_MAIN;
}

static int Title_SubSeqEndDemoWait(WorldTradeWork *wk) {
    if (wk->demoEnd) {
        wk->subprocessSeq = TITLE_SEQ_END;
    }
    return WT_SEQ_MAIN;
}

static int Title_SubSeqEnd(WorldTradeWork *wk) {
    if (wk->subNextProcess == WORLDTRADE_ENTER) {
        GFL_WipeSet(0, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
    } else {
        GFL_WipeSet(3, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
    }
    wk->subprocessSeq = TITLE_SEQ_OPENING;
    return WT_SEQ_FADEOUT;
}

static int Title_SubSeqYesNo(WorldTradeWork *wk) {
    WorldTrade_TouchWinYesNoMake(wk, 18, 0x102, 3, TRUE);
    wk->subprocessSeq = TITLE_SEQ_YESNO_SELECT;
    return WT_SEQ_MAIN;
}

static inline void Title_ClearWin(BmpWin *win) {
    BmpWin_ClearScreen(win);
    GFL_BGSysQueueScrLoad(BmpWin_GetBGIndex(win));
}

static int Title_SubSeqYesNoSelect(WorldTradeWork *wk) {
    u32 ret = WorldTrade_TouchSwMain(wk);

    if (ret == 1) {
        // Leaves the station
        WorldTrade_TouchWinYesNoDel(wk);
        BmpWin_ClearFrame(wk->talkWin, 2);
        Title_ClearWin(wk->talkWin);
        Title_ClearWin(wk->explainWin);
        GFL_BGSysSetBGEnabled(6, FALSE);
        ClActor_SetVisible(wk->promptDsAct, FALSE);
        WorldTrade_SubProcessChange(wk, WORLDTRADE_ENTER, 20);
        wk->subprocessSeq = TITLE_SEQ_END_DEMO;
    } else if (ret == 2) {
        WorldTrade_TouchWinYesNoDel(wk);
        BmpWin_ClearFrame(wk->talkWin, 2);
        Title_ClearWin(wk->talkWin);
        ClActor_SetAutoAnim(wk->cursorAct, TRUE);
        wk->subprocessSeq = TITLE_SEQ_START;
        ClActor_SetVisible(wk->cursorAct, TRUE);
    }
    return WT_SEQ_MAIN;
}

static int Title_SubSeqCursorAnimWait(WorldTradeWork *wk) {
    if (!ClActor_IsAnimActive(wk->cursorAct)) {
        ClActor_SetSequence(wk->cursorAct, 0);
        if (GCTX_HIDIsUsingTouch() == TRUE) {
            ClActor_SetVisible(wk->cursorAct, FALSE);
        } else {
            ClActor_SetVisible(wk->cursorAct, TRUE);
        }
        Title_DecideFunc(wk, wk->titleCursorPos);
    }
    return WT_SEQ_MAIN;
}

static int Title_SubSeqMessageWait(WorldTradeWork *wk) {
    if (!WorldTrade_PrintIsBusy(&wk->print)) {
        wk->subprocessSeq = wk->subprocessNextSeq;
    }
    return WT_SEQ_MAIN;
}

static int Title_SubSeqMessage1MinWait(WorldTradeWork *wk) {
    if (!WorldTrade_PrintIsBusy(&wk->print)) {
        wk->wait++;
        if (wk->wait > 45) {
            wk->wait = 0;
            wk->subprocessSeq = wk->subprocessNextSeq;
        }
    }
    return WT_SEQ_MAIN;
}

// The choices' texts, without and with a Pokémon deposited
static const u32 sTitleMenuMsgTable[][3] = {
    { 0x2f, 0x30, 0x31 },
    { 0x2e, 0x30, 0x31 },
};

static void Title_MenuPrint(WorldTradeWork *wk) {
    const u32 *msgs = sTitleMenuMsgTable[wk->depositFlag];
    int i;

    for (i = 0; i < 3; i++) {
        GFL_BitmapFill(BmpWin_GetBitmap(wk->menuWin[i]), 1);
        Title_BmpWinPrint(wk->menuWin[i], wk->msgManager, 0, msgs[i], 0, &wk->print, 0x3dc1);
    }
}

static void Title_MessagePrint(WorldTradeWork *wk, int msgNo, int wait, int flag, u16 dat) {
    GFL_MsgDataLoadStrbuf(wk->msgManager, msgNo, wk->talkString);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWin), 15);
    BmpWin_TransferNow(wk->msgWin);
    BmpWin_DrawFrame(wk->msgWin, 0, 1, 14);
    WorldTrade_Print(wk->msgWin, 0, wk->talkString, 0, 0, &wk->print);
}

static void Title_TalkPrint(WorldTradeWork *wk, int msgNo, int wait, int flag, u16 dat) {
    GFL_MsgDataLoadStrbuf(wk->msgManager, msgNo, wk->talkString);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->talkWin), 15);
    BmpWin_DrawFrame(wk->talkWin, 0, 1, 14);
    WorldTrade_Print(wk->talkWin, 0, wk->talkString, 0, 0, &wk->print);
    BmpWin_TransferNow(wk->talkWin);
}

static void Title_BmpWinPrint(BmpWin *win, MsgData *msgManager, int font, int msgNo, u16 dat, WorldTradePrint *print,
                              u16 color) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgManager, msgNo);

    WorldTrade_PrintColor(win, font, str, 0, 0, 0, color, print);
    GFL_StrBufFree(str);
}

void WorldTrade_SubLcdBgGraphicSet(WorldTradeWork *wk) {
    if (wk->subLcdBgInit == 1 && wk->subLcdBgKeep == 0) {
        ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_WORLDTRADE, HEAPID_WORLDTRADE);

        GFL_G2DIOLoadArcNCLR(arc, 4, 4, 0x160, 0x160, 0xa0, HEAPID_WORLDTRADE);
        GFL_BGSysLoadArcNCGRStatic(arc, 14, 5, 0, 0x1c00, FALSE, HEAPID_WORLDTRADE);
        GFL_G2DIOLoadNSCRAsync(arc, 33, 5, 0, 0, 0, FALSE, HEAPID_WORLDTRADE);
        GFL_ArcToolFree(arc);
        GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x20, 0x20, HEAPID_WORLDTRADE);
    }
}

void WorldTrade_SubLcdWinGraphicSet(WorldTradeWork *wk) {
    if (wk->subLcdBgInit == 1 && wk->subLcdBgKeep == 0) {
        ArcTool *arc = GFL_ArcSysCreateFileHandle(ARCID_WORLDTRADE, HEAPID_WORLDTRADE);

        GFL_G2DIOLoadArcNCLRDefault(arc, 5, 4, 0, 0x20, HEAPID_WORLDTRADE);
        GFL_BGSysLoadArcNCGRStatic(arc, 15, 6, 0, 0, FALSE, HEAPID_WORLDTRADE);
        GFL_G2DIOLoadNSCRAsync(arc, 34, 6, 0, 0, 0, FALSE, HEAPID_WORLDTRADE);
        GFL_ArcToolFree(arc);
    }
    ClActor_SetVisible(wk->promptDsAct, TRUE);
}

void WorldTrade_SubLcdExplainPut(WorldTradeWork *wk, int explain) {
    wk->explainWin = BmpWin_CreateDynamic(4, 12, 19, 18, 4, 1, TRUE);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->explainWin), 0);
    WorldTrade_ExplainPrint(wk->explainWin, wk->msgManager, explain, &wk->print);
    BmpWin_Transfer(wk->explainWin);
}
