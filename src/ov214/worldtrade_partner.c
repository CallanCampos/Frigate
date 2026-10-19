#include "types.h"
#include "constants/arc.h"
#include "constants/species.h"
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
#include "gfl/std.h"
#include "gfl/str.h"
#include "gfl/touchpanel.h"
#include "gfl/ui.h"
#include "nitro/gx.h"
#include "nitro/hw.h"
#include "pml/poke_party.h"
#include "system/bmp_winframe.h"
#include "system/wipe.h"
#include "system/wordset.h"
#include "worldtrade_local.h"

// The Global Trade Station's screen of a Pokémon that a search found, with its trainer, where the player can offer
// a Pokémon for it. The file's name is a guess, as the ROM doesn't give it, and so are the names of its functions

enum {
    PARTNER_SEQ_START,
    PARTNER_SEQ_MAIN,
    PARTNER_SEQ_END,
    PARTNER_SEQ_MESSAGE_WAIT,
    PARTNER_SEQ_YESNO,
    PARTNER_SEQ_YESNO_SELECT,
    PARTNER_SEQ_PAGE_CHANGE,
    PARTNER_SEQ_RETURN_SCREEN1,
    PARTNER_SEQ_EXCHANGE_SCREEN2,
};

// The sub-process modes of the screens it goes to and comes from
#define SEARCH_MODE_PARTNER_RETURN 15
#define PARTNER_MODE_FROM_SEARCH 16
#define PARTNER_MODE_FROM_PARTNER 17
#define BOX_MODE_EXCHANGE_SELECT 6

// How many windows of information the screen has
#define PARTNER_INFOWIN_NUM 16

typedef struct {
    int x;
    int y;
    int w;
    int h;
    int bg;
} PartnerWinPos;

static void Partner_BgInit(int subBg1Offset);
static void Partner_BgExit(void);
static void Partner_BgGraphicSet(WorldTradeWork *wk);
static void Partner_SetCellActor(WorldTradeWork *wk);
static void Partner_DelCellActor(WorldTradeWork *wk);
static void Partner_BmpWinInit(WorldTradeWork *wk);
static void Partner_BmpWinDelete(WorldTradeWork *wk);
static void Partner_InitWork(WorldTradeWork *wk);
static void Partner_FreeWork(WorldTradeWork *wk);
static int Partner_SubSeqStart(WorldTradeWork *wk);
static void Partner_DecidePartner(WorldTradeWork *wk, int result);
static int Partner_SubSeqMain(WorldTradeWork *wk);
static int Partner_SubSeqEnd(WorldTradeWork *wk);
static int Partner_SubSeqYesNo(WorldTradeWork *wk);
static int Partner_SubSeqYesNoSelect(WorldTradeWork *wk);
static void Partner_ChangePage(WorldTradeWork *wk);
static int Partner_SubSeqPageChange(WorldTradeWork *wk);
static int Partner_SubSeqReturnScreen1(WorldTradeWork *wk);
static int Partner_SubSeqExchangeScreen2(WorldTradeWork *wk);
static int Partner_SubSeqMessageWait(WorldTradeWork *wk);
static void Partner_SubSeqMessagePrint(WorldTradeWork *wk, int msgNo, int wait, int flag, u16 dat);
static void Partner_PokeLabelPrint(MsgData *msgManager, BmpWin **win, int msg, WorldTradePrint *print, u16 color);
static void Partner_TouchPrint(MsgData *msgManager, BmpWin **win, int msg, WorldTradePrint *print);
static void Partner_WantPokeInfoPrint(BmpWin **win, MsgData *gtcMsg, MsgData *monsName, Dpw_Tr_PokemonSearchData *dtsd,
                                      WorldTradePrint *print);
static void Partner_TrainerInfoPrint(BmpWin **win, StrBuf *str1, StrBuf *str2, WorldTradePrint *print);
static void Partner_SlideScreenVFunc(WorldTradeWork *wk);

static int (*sPartnerSubSeqTable[])(WorldTradeWork *wk) = {
    Partner_SubSeqStart,       Partner_SubSeqMain,          Partner_SubSeqEnd,
    Partner_SubSeqMessageWait, Partner_SubSeqYesNo,         Partner_SubSeqYesNoSelect,
    Partner_SubSeqPageChange,  Partner_SubSeqReturnScreen1, Partner_SubSeqExchangeScreen2,
};

int WorldTrade_Partner_Init(WorldTradeWork *wk, int seq) {
    Partner_InitWork(wk);
    Partner_BgInit(-32 - wk->drawOffset);
    if (GXx_GetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR) == 0) {
        WorldTrade_SubLcdBgInit(wk, -32 - wk->drawOffset, 1);
    } else {
        WorldTrade_SubLcdBgInit(wk, -32 - wk->drawOffset, 0);
    }
    Partner_BgGraphicSet(wk);
    Partner_BmpWinInit(wk);
    Partner_SetCellActor(wk);

    WorldTrade_MyPokeInfoPrint(wk->msgManager, wk->monsNameManager, wk->wordSet, &wk->infoWin[0],
                               func_0201d624((PartyPkm *)wk->downloadPokemonData[wk->touchTrainerPos].postData),
                               &wk->downloadPokemonData[wk->touchTrainerPos].postSimple, &wk->print);
    WorldTrade_PokeInfoPrint2(wk->msgManager, &wk->infoWin[7], wk->downloadPokemonData[wk->touchTrainerPos].name,
                              (PartyPkm *)wk->downloadPokemonData[wk->touchTrainerPos].postData, &wk->infoWin[12],
                              &wk->print);
    WorldTrade_TransPokeGraphic((PartyPkm *)wk->downloadPokemonData[wk->touchTrainerPos].postData);
    Partner_PokeLabelPrint(wk->msgManager, &wk->infoWin[14], 0x53, &wk->print, 0x440);
    Partner_PokeLabelPrint(wk->msgManager, &wk->infoWin[9], 0x57, &wk->print, 0x3c40);
    if (wk->partnerChange == 0) {
        Partner_TouchPrint(wk->msgManager, &wk->menuWin[0], 0x5e, &wk->print);
        Partner_TouchPrint(wk->msgManager, &wk->menuWin[1], 0x73, &wk->print);
    }
    Partner_ChangePage(wk);
    WorldTrade_SetPartnerCursorPos(wk, wk->touchTrainerPos, -wk->drawOffset);
    wk->vfunc2 = Partner_SlideScreenVFunc;
    GX_SetDispSelect(GX_DISP_SELECT_MAIN_SUB);
    GFL_NetSetIconScreen(TRUE, HEAPID_WORLDTRADE);

    if (wk->subProcessMode == PARTNER_MODE_FROM_PARTNER) {
        if (GXx_GetMasterBrightness_(REG_MASTER_BRIGHT_ADDR) == -16 &&
            GXx_GetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR) != -16) {
            GFL_WipeSet(3, 1, 1, 0, 6, 1, HEAPID_WORLDTRADE);
        } else if (GXx_GetMasterBrightness_(REG_MASTER_BRIGHT_ADDR) != -16 &&
                   GXx_GetMasterBrightness_(REG_DB_MASTER_BRIGHT_ADDR) == -16) {
            GFL_WipeSet(4, 1, 1, 0, 6, 1, HEAPID_WORLDTRADE);
        } else {
            GFL_WipeSet(0, 1, 1, 0, 6, 1, HEAPID_WORLDTRADE);
        }
    }

    wk->subprocessSeq = PARTNER_SEQ_START;
    wk->subLcdBgKeep = 0;
    wk->partnerChange = 0;
    return WT_SEQ_FADEIN;
}

int WorldTrade_Partner_Main(WorldTradeWork *wk, int seq) {
    int ret = sPartnerSubSeqTable[wk->subprocessSeq](wk);
    int i;

    for (i = 0; i < 8; i++) {
        WorldTrade_ActPos(wk->subAct[i], wk->subActY[i][0], wk->subActY[i][1] + wk->drawOffset + 32);
    }
    WorldTrade_CLACT_PosChange(wk->pokemonAct, 208, 58 - wk->drawOffset);
    WorldTrade_SetPartnerCursorPos(wk, wk->touchTrainerPos, wk->drawOffset);
    return ret;
}

int WorldTrade_Partner_End(WorldTradeWork *wk, int seq) {
    wk->vfunc2 = NULL;
    Partner_DelCellActor(wk);
    Partner_FreeWork(wk);
    Partner_BmpWinDelete(wk);
    Partner_BgExit();
    WorldTrade_SubLcdBgExit(wk);
    ClActor_SetVisible(wk->promptDsAct, FALSE);
    WorldTrade_SubProcessUpdate(wk);
    if (wk->partnerChange == 0) {
        GFL_NetSetIconScreen(FALSE, HEAPID_WORLDTRADE);
    }
    return WT_SEQ_INIT;
}

static void Partner_BgInit(int subBg1Offset) {
    {
        BGSysLCDConfig config = { GX_DISPMODE_GRAPHICS, GX_BGMODE_0, GX_BGMODE_0, GX_BG0_AS_3D };
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
    GFL_BGSysClearCharCore(2, 32, 0, HEAPID_WORLDTRADE);
    GFL_BGSysSetBGEnabledA(GX_PLANEMASK_BG3, FALSE);
}

static void Partner_BgExit(void) {
    GFL_BGSysReleaseBG(1);
    GFL_BGSysReleaseBG(2);
}

static void Partner_BgGraphicSet(WorldTradeWork *wk) {
    ArcTool *arc;

    GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 0, 0x1a0, 0x20, HEAPID_WORLDTRADE);
    LoadSysMsgBox(4, 1, 2, 0, HEAPID_WORLDTRADE);
    arc = GFL_ArcSysCreateFileHandle(ARCID_WORLDTRADE, HEAPID_WORLDTRADE);
    GFL_BGSysLoadArcNCGRStatic(arc, 16, 1, 0, 0xc00, TRUE, HEAPID_WORLDTRADE);
    GFL_G2DIOLoadArcNCLRDefault(arc, 6, 0, 0, 0x60, HEAPID_WORLDTRADE);
    if (wk->partnerChange == 0) {
        GFL_BGSysLoadArcNCGRStatic(arc, 15, 6, 0, 0, FALSE, HEAPID_WORLDTRADE);
        loadBGScrToVramByFileNoReserveNegAlign(arc, 35, 6, 0, 0, FALSE, HEAPID_WORLDTRADE);
    }
    GFL_ArcToolFree(arc);
    if (wk->partnerChange == 0) {
        GFL_BGSysLoadNCLRDefault(ARCID_FONT, 5, 4, 0x20, 0x20, HEAPID_WORLDTRADE);
    }
}

static void Partner_SetCellActor(WorldTradeWork *wk) {
    ClActorSetup setup;

    MI_CpuFill8(&setup, 0, sizeof(ClActorSetup));
    setup.x = 208;
    setup.y = 58;
    wk->pokemonAct = ClActor_Create(wk->clactUnit, wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CHAR],
                                    wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_PLTT],
                                    wk->clactRes[WT_CLACT_RES_MAIN][WT_CLACT_RES_CELL], &setup, 0, HEAPID_WORLDTRADE);
    func_0204c520(wk->pokemonAct, TRUE);
    ClActor_SetSequence(wk->pokemonAct, 36);
    ClActor_SetVisible(wk->pokemonAct, TRUE);
    GFL_NetSetIconScreen(FALSE, HEAPID_WORLDTRADE);
}

static void Partner_DelCellActor(WorldTradeWork *wk) {
    ClActor_Delete(wk->pokemonAct);
}

// The windows of the Pokémon's and its trainer's information
static const PartnerWinPos sPartnerInfoWinPos[PARTNER_INFOWIN_NUM] = {
    { 2, 1, 12, 2, 2 },  { 7, 4, 9, 2, 2 },   { 11, 1, 7, 2, 2 },  { 14, 1, 7, 2, 2 },
    { 1, 10, 6, 2, 2 },  { 7, 10, 13, 2, 2 }, { 1, 4, 6, 2, 2 },   { 1, 13, 10, 2, 2 },
    { 12, 13, 8, 2, 2 }, { 1, 16, 13, 2, 2 }, { 2, 18, 27, 2, 2 }, { 3, 20, 27, 2, 2 },
    { 1, 7, 5, 2, 2 },   { 7, 7, 9, 2, 2 },   { 1, 1, 24, 2, 7 },  { 2, 3, 27, 2, 7 },
};

static void Partner_BmpWinInit(WorldTradeWork *wk) {
    int i;

    wk->msgWin = BmpWin_CreateDynamic(4, 2, 19, 27, 4, 1, TRUE);
    if (wk->partnerChange == 0) {
        wk->menuWin[0] = BmpWin_CreateDynamic(4, 1, 21, 13, 2, 1, TRUE);
        wk->menuWin[1] = BmpWin_CreateDynamic(4, 17, 21, 13, 2, 1, TRUE);
    }

    for (i = 0; i < PARTNER_INFOWIN_NUM; i++) {
        if (sPartnerInfoWinPos[i].bg == 2) {
            wk->infoWin[i] =
                BmpWin_CreateDynamic(sPartnerInfoWinPos[i].bg, sPartnerInfoWinPos[i].x, sPartnerInfoWinPos[i].y,
                                     sPartnerInfoWinPos[i].w, sPartnerInfoWinPos[i].h, 13, TRUE);
        } else {
            wk->infoWin[i] =
                BmpWin_CreateDynamic(sPartnerInfoWinPos[i].bg, sPartnerInfoWinPos[i].x, sPartnerInfoWinPos[i].y,
                                     sPartnerInfoWinPos[i].w, sPartnerInfoWinPos[i].h, 1, TRUE);
        }
        BmpWin_Transfer(wk->infoWin[i]);
    }
}

static void Partner_BmpWinDelete(WorldTradeWork *wk) {
    BmpWin *win;
    int i;

    WorldTrade_PrintClear(&wk->print);
    BmpWin_Free(wk->msgWin);
    if (wk->partnerChange == 0) {
        win = wk->menuWin[1];
        BmpWin_ClearScreen(win);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
        win = wk->menuWin[0];
        BmpWin_ClearScreen(win);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
        BmpWin_Free(wk->menuWin[1]);
        BmpWin_Free(wk->menuWin[0]);
    }
    for (i = 0; i < PARTNER_INFOWIN_NUM; i++) {
        win = wk->infoWin[i];
        BmpWin_ClearScreen(win);
        GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
        BmpWin_Free(wk->infoWin[i]);
    }
}

static void Partner_InitWork(WorldTradeWork *wk) {
    Dpw_Tr_Data *dtd = &wk->downloadPokemonData[wk->touchTrainerPos];

    wk->talkString = GFL_StrBufCreate(180, HEAPID_WORLDTRADE);
    GFL_WordSetClearAll(wk->wordSet);
    if (dtd->countryCode != 0) {
        loadCountryToStrbuf(wk->wordSet, 8, dtd->countryCode);
    }
    if (dtd->localCode != 0) {
        loadCountryAreaToStrbuf(wk->wordSet, 9, dtd->countryCode, dtd->localCode);
    }
    wk->infoString[0] = WorldTrade_ExpandMessage(wk->wordSet, wk->msgManager, 0x58, HEAPID_WORLDTRADE);
    wk->infoString[1] = WorldTrade_ExpandMessage(wk->wordSet, wk->msgManager, 0x59, HEAPID_WORLDTRADE);
}

static void Partner_FreeWork(WorldTradeWork *wk) {
    GFL_StrBufFree(wk->infoString[0]);
    GFL_StrBufFree(wk->infoString[1]);
    GFL_StrBufFree(wk->talkString);
}

static int Partner_SubSeqStart(WorldTradeWork *wk) {
    ClActor_SetVisible(wk->promptDsAct, FALSE);
    if (wk->subProcessMode == PARTNER_MODE_FROM_SEARCH) {
        GFL_WipeSet(0, 1, 1, 0, 16, 1, HEAPID_WORLDTRADE);
        wk->subprocessSeq = PARTNER_SEQ_RETURN_SCREEN1;
    } else {
        wk->subprocessSeq = PARTNER_SEQ_MAIN;
        ClActor_SetVisible(wk->partnerCursorAct, TRUE);
    }
    return WT_SEQ_MAIN;
}

static void Partner_DecidePartner(WorldTradeWork *wk, int result) {
    if (result != wk->touchTrainerPos && result >= 0) {
        ClActor_SetSequence(wk->subAct[result + 1], result * 4 + 16);
        wk->subprocessSeq = PARTNER_SEQ_END;
        WorldTrade_SubProcessChange(wk, WORLDTRADE_PARTNER, PARTNER_MODE_FROM_PARTNER);
        wk->touchTrainerPos = result;
        WorldTrade_SetPartnerCursorPos(wk, result, 0);
        GFL_SndSEPlay(0x54c);
        wk->subLcdBgKeep = 1;
        wk->partnerChange = 1;
    }
}

// The two buttons: the Pokémon's trainer and Back
static const TouchRect sPartnerTouchRects[] = {
    { 162, 191, 3, 125 },
    { 162, 191, 130, 253 },
    { TOUCH_RECT_END, 0, 0, 0 },
};

// The trainer that left and right go to, from each of the found trainers
static const u8 sPartnerKeyTbl[SEARCH_POKE_MAX][2] = {
    { 1, 2 }, { 3, 0 }, { 0, 4 }, { 5, 1 }, { 2, 6 }, { 5, 3 }, { 4, 6 },
};

static int Partner_SubSeqMain(WorldTradeWork *wk) {
    int next;
    int ret;

    if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_A) {
        Partner_SubSeqMessagePrint(wk, 0x10, 1, 0, 0xf0f);
        WorldTrade_SetNextSeq(wk, PARTNER_SEQ_MESSAGE_WAIT, PARTNER_SEQ_YESNO);
        GFL_SndSEPlay(0x54c);
    } else if (GCTX_HIDGetPressedKeys() & PAD_BUTTON_B) {
        GFL_WipeSet(0, 0, 0, 0, 16, 1, HEAPID_WORLDTRADE);
        wk->subprocessSeq = PARTNER_SEQ_EXCHANGE_SCREEN2;
        GFL_SndSEPlay(0x551);
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_LEFT) {
        next = sPartnerKeyTbl[wk->touchTrainerPos][0];
        if (wk->touchTrainerPos != next && wk->searchResult >= next + 1) {
            Partner_DecidePartner(wk, next);
        }
    } else if (GCTX_HIDGetPressedKeys() & PAD_KEY_RIGHT) {
        next = sPartnerKeyTbl[wk->touchTrainerPos][1];
        if (wk->touchTrainerPos != next && wk->searchResult >= next + 1) {
            Partner_DecidePartner(wk, next);
        }
    } else {
        ret = WorldTrade_SubLcdObjHitCheck(wk->searchResult);
        if (ret != -1) {
            Partner_DecidePartner(wk, ret);
        }
        ret = GFL_HIDGetTouchPressedRect(sPartnerTouchRects);
        if (ret == 0) {
            Partner_SubSeqMessagePrint(wk, 0x10, 1, 0, 0xf0f);
            WorldTrade_SetNextSeq(wk, PARTNER_SEQ_MESSAGE_WAIT, PARTNER_SEQ_YESNO);
            GFL_SndSEPlay(0x54c);
        } else if (ret == 1) {
            GFL_WipeSet(0, 0, 0, 0, 16, 1, HEAPID_WORLDTRADE);
            wk->subprocessSeq = PARTNER_SEQ_EXCHANGE_SCREEN2;
            GFL_SndSEPlay(0x551);
        }
    }
    return WT_SEQ_MAIN;
}

static int Partner_SubSeqEnd(WorldTradeWork *wk) {
    ClActor_SetVisible(wk->partnerCursorAct, FALSE);
    if (wk->subProcessMode != SEARCH_MODE_PARTNER_RETURN) {
        if (wk->subProcessMode == BOX_MODE_EXCHANGE_SELECT) {
            GFL_WipeSet(0, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
        } else {
            GFL_WipeSet(3, 0, 0, 0, 6, 1, HEAPID_WORLDTRADE);
        }
    }
    wk->subprocessSeq = PARTNER_SEQ_START;
    return WT_SEQ_FADEOUT;
}

static int Partner_SubSeqYesNo(WorldTradeWork *wk) {
    WorldTrade_TouchWinYesNoMakeEx(wk, 18, 0x12e, 3, 4, TRUE);
    wk->subprocessSeq = PARTNER_SEQ_YESNO_SELECT;
    return WT_SEQ_MAIN;
}

static int Partner_SubSeqYesNoSelect(WorldTradeWork *wk) {
    u32 ret = WorldTrade_TouchSwMain(wk);

    if (ret == 1) {
        WorldTrade_TouchWinYesNoDel(wk);
        wk->subprocessSeq = PARTNER_SEQ_END;
        WorldTrade_SubProcessChange(wk, WORLDTRADE_MYBOX, BOX_MODE_EXCHANGE_SELECT);
        Partner_PokeLabelPrint(wk->msgManager, &wk->infoWin[14], 0x53, &wk->print, 0x440);
        Partner_PokeLabelPrint(wk->msgManager, &wk->infoWin[9], 0x57, &wk->print, 0x3c40);
        Partner_ChangePage(wk);
    } else if (ret == 2) {
        WorldTrade_TouchWinYesNoDel(wk);
        BmpWin_ClearFrame(wk->msgWin, 0);
        wk->subprocessSeq = PARTNER_SEQ_MAIN;
        Partner_TouchPrint(wk->msgManager, &wk->menuWin[0], 0x5e, &wk->print);
        Partner_TouchPrint(wk->msgManager, &wk->menuWin[1], 0x73, &wk->print);
    }
    return WT_SEQ_MAIN;
}

static void Partner_ChangePage(WorldTradeWork *wk) {
    loadBGScrToVramByNarcNoReserveNegAlign(ARCID_WORLDTRADE, 30, 1, 0, 0x800, TRUE, HEAPID_WORLDTRADE);
    Partner_TrainerInfoPrint(&wk->infoWin[10], wk->infoString[0], wk->infoString[1], &wk->print);
    Partner_WantPokeInfoPrint(&wk->infoWin[15], wk->msgManager, wk->monsNameManager,
                              &wk->downloadPokemonData[wk->touchTrainerPos].wantSimple, &wk->print);
}

static int Partner_SubSeqPageChange(WorldTradeWork *wk) {
    Partner_ChangePage(wk);
    wk->subprocessSeq = PARTNER_SEQ_MAIN;
    return WT_SEQ_MAIN;
}

// Slides the screens in from the search screen
static int Partner_SubSeqReturnScreen1(WorldTradeWork *wk) {
    wk->drawOffset++;
    if (GFL_WipeIsFinished()) {
        wk->drawOffset = 0;
        wk->subprocessSeq = PARTNER_SEQ_MAIN;
        ClActor_SetVisible(wk->partnerCursorAct, TRUE);
    }
    return WT_SEQ_MAIN;
}

// Slides the screens back to the search screen
static int Partner_SubSeqExchangeScreen2(WorldTradeWork *wk) {
    wk->drawOffset--;
    if (GFL_WipeIsFinished()) {
        GX_SetDispSelect(GX_DISP_SELECT_SUB_MAIN);
        wk->subprocessSeq = PARTNER_SEQ_END;
        WorldTrade_SubProcessChange(wk, WORLDTRADE_SEARCH, SEARCH_MODE_PARTNER_RETURN);
        wk->drawOffset = 16;
    }
    return WT_SEQ_MAIN;
}

static int Partner_SubSeqMessageWait(WorldTradeWork *wk) {
    if (!WorldTrade_PrintIsBusy(&wk->print)) {
        wk->subprocessSeq = wk->subprocessNextSeq;
    }
    return WT_SEQ_MAIN;
}

static void Partner_SubSeqMessagePrint(WorldTradeWork *wk, int msgNo, int wait, int flag, u16 dat) {
    BmpWin *win;

    GFL_MsgDataLoadStrbuf(wk->msgManager, msgNo, wk->talkString);
    GFL_BitmapFill(BmpWin_GetBitmap(wk->msgWin), 15);
    BmpWin_DrawFrame(wk->msgWin, 0, 1, 2);
    WorldTrade_Print(wk->msgWin, 0, wk->talkString, 0, 0, &wk->print);
    win = wk->msgWin;
    BmpWin_FlushChar(win);
    BmpWin_FlushMap(win);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(win));
}

static void Partner_PokeLabelPrint(MsgData *msgManager, BmpWin **win, int msg, WorldTradePrint *print, u16 color) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgManager, msg);

    WorldTrade_SysPrint(win[0], str, 0, 2, 0, color, print);
    GFL_StrBufFree(str);
}

static void Partner_TouchPrint(MsgData *msgManager, BmpWin **win, int msg, WorldTradePrint *print) {
    StrBuf *str = GFL_MsgDataLoadStrbufNew(msgManager, msg);
    BmpWin *window;

    WorldTrade_TouchPrint(win[0], str, 0, 0, 1, 0x3c40, print);
    window = win[0];
    BmpWin_FlushChar(window);
    BmpWin_FlushMap(window);
    GFL_BGSysLoadScr(BmpWin_GetBGIndex(window));
    GFL_StrBufFree(str);
}

static void Partner_WantPokeInfoPrint(BmpWin **win, MsgData *gtcMsg, MsgData *monsName, Dpw_Tr_PokemonSearchData *dtsd,
                                      WorldTradePrint *print) {
    BOOL showSex;

    GFL_BitmapFill(BmpWin_GetBitmap(win[0]), 0);
    WorldTrade_PokeNamePrintNoPut(win[0], monsName, dtsd->characterNo, 0, 0x440, print);
    // The Nidoran's names show their gender already
    showSex = TRUE;
    if (dtsd->characterNo == SPECIES_NIDORAN_M || dtsd->characterNo == SPECIES_NIDORAN_F) {
        showSex = FALSE;
    }
    if (showSex) {
        WorldTrade_SexPrintNoPut(win[0], gtcMsg, dtsd->gender, 0, 73, 0, 0x440, print);
    }
    WorldTrade_WantLevelPrint_XY(win[0], gtcMsg,
                                 WorldTrade_LevelTermGet(dtsd->level_min, dtsd->level_max, LEVEL_PRINT_TBL_DEPOSIT), 0,
                                 104, 0, 0x440, LEVEL_PRINT_TBL_DEPOSIT, print);
}

static void Partner_TrainerInfoPrint(BmpWin **win, StrBuf *str1, StrBuf *str2, WorldTradePrint *print) {
    GFL_BitmapFill(BmpWin_GetBitmap(win[0]), 0);
    GFL_BitmapFill(BmpWin_GetBitmap(win[1]), 0);
    if (str1 != NULL) {
        WorldTrade_SysPrint(win[0], str1, 0, 0, 0, 0x440, print);
    }
    if (str2 != NULL) {
        WorldTrade_SysPrint(win[1], str2, 0, 0, 0, 0x440, print);
    }
}

static void Partner_SlideScreenVFunc(WorldTradeWork *wk) {
    GFL_BGSysMoveBG(0, BG_MOVE_SET_Y, wk->drawOffset);
    GFL_BGSysMoveBG(1, BG_MOVE_SET_Y, wk->drawOffset);
    GFL_BGSysMoveBG(2, BG_MOVE_SET_Y, wk->drawOffset);
    GFL_BGSysMoveBG(3, BG_MOVE_SET_Y, wk->drawOffset);
    GFL_BGSysMoveBG(4, BG_MOVE_SET_Y, -wk->drawOffset);
    GFL_BGSysMoveBG(5, BG_MOVE_SET_Y, -32 - wk->drawOffset);
    GFL_BGSysMoveBG(6, BG_MOVE_SET_Y, -wk->drawOffset);
    GFL_BGSysMoveBG(7, BG_MOVE_SET_Y, -wk->drawOffset);
}
